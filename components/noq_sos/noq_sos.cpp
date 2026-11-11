#include "noq_sos.h"
#include "portal_page.h"
#include "panel_page.h"
#include "icon_png.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <strings.h>

#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "driver/usb_serial_jtag.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_attr.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "nvs.h"

#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/components/captive_portal/captive_portal.h"
#include "esphome/components/json/json_util.h"
#include "esphome/components/wifi/wifi_component.h"

namespace esphome::noq_sos {

static const char *const TAG = "noq_sos";
static const char *const NVS_NS = "noq_sos";
static constexpr uint32_t SETTINGS_MAGIC = 0x53305301;
static constexpr uint32_t RTC_MAGIC = 0x52545303;
static constexpr uint32_t BATLOG_MAGIC = 0x42415401;
static constexpr size_t MAX_BODY = 20 * 1024;
static constexpr uint8_t MENU_VERSION = 2;  // bump when the command list changes: registered again on the next check
static constexpr uint16_t POLL_DEFAULT = 60;

// Survives deep sleep (not a power loss): what still has to be delivered and when the next check-in is due.
struct RtcState {
  uint32_t magic;
  uint16_t pending;     // presses not yet delivered
  uint8_t sent_mask;    // recipients that already have the pending alert
  uint8_t tries;        // delivery rounds that failed for the pending alert
  int64_t pressed_at;   // UTC of the first undelivered press, 0 if the clock was unknown
  int64_t next_check;   // UTC of the next daily check-in, 0 if not known yet
  uint8_t timer_checkin;  // the last sleep was timed for the check-in (used when the clock is unknown)
  uint8_t low_sent;     // low battery message sent for this discharge
  uint8_t last_ok;      // the last alert reached everyone
  int64_t last_alert;   // UTC of the last delivered alert
  uint32_t last_wifi_ms;  // last alert from sleep: press to home network (ms since the app started)
  uint32_t last_sent_ms;  // last alert: press to Telegram's answer
  int64_t offset;         // next Telegram update to read; also in NVS, so a power loss does not answer old commands again
};
static RTC_DATA_ATTR RtcState rtc_state;

// feedback patterns: on, off, on … (ms)
static const uint16_t FX_PRESS[] = {150};
static const uint16_t FX_SENT[] = {70, 110, 70, 110, 70};
static const uint16_t FX_FAIL[] = {900};
static const uint16_t FX_HELLO[] = {40};
// discovery: one short = good, two = fair, three = weak; FX_FAIL (one long) = no network
static const uint16_t FX_K2[] = {150, 220, 150};
static const uint16_t FX_K3[] = {150, 220, 150, 220, 150};

static NoqSos *s_self = nullptr;

static int64_t now_utc() {
  time_t t = time(nullptr);
  return t > 1700000000 ? static_cast<int64_t>(t) : 0;
}

static std::string hhmm(int64_t utc, int tz_min) {
  if (utc == 0)
    return "";
  time_t local = static_cast<time_t>(utc + tz_min * 60);
  struct tm tm {};
  gmtime_r(&local, &tm);
  char b[8];
  snprintf(b, sizeof(b), "%02d:%02d", tm.tm_hour, tm.tm_min);
  return b;
}

static std::string jesc(const char *s) {
  std::string o;
  for (const unsigned char *p = reinterpret_cast<const unsigned char *>(s); *p; p++) {
    unsigned char c = *p;
    if (c == '"' || c == '\\') {
      o += '\\';
      o += static_cast<char>(c);
    } else if (c == '\n') {
      o += "\\n";
    } else if (c < 0x20) {
      char b[8];
      snprintf(b, sizeof(b), "\\u%04x", c);
      o += b;
    } else {
      o += static_cast<char>(c);
    }
  }
  return o;
}

// Copies at most n-1 bytes without cutting a UTF-8 character in half.
static void copy_utf8(char *dst, size_t n, const std::string &src) {
  size_t len = std::min(src.size(), n - 1);
  while (len > 0 && len < src.size() && (static_cast<unsigned char>(src[len]) & 0xC0) == 0x80)
    len--;
  memcpy(dst, src.data(), len);
  dst[len] = 0;
}

static constexpr int BAT_MAX_MV = 4400;     // above this the reading is not a single LiPo cell
static constexpr int BAT_SPREAD_MV = 300;   // eight samples further apart than this: the pin is floating

static float battery_percent_f(int mv) {
  // resting LiPo curve, close enough for "charge it soon" and for the trend
  static const int MV[] = {4200, 4100, 4000, 3900, 3800, 3750, 3700, 3650, 3600, 3500, 3300};
  static const int PC[] = {100, 90, 80, 65, 50, 40, 30, 20, 12, 5, 0};
  if (mv >= MV[0])
    return 100.0f;
  for (int i = 1; i < 11; i++) {
    if (mv >= MV[i])
      return PC[i] + static_cast<float>(PC[i - 1] - PC[i]) * (mv - MV[i]) / (MV[i - 1] - MV[i]);
  }
  return 0.0f;
}

static int battery_percent(int mv) { return static_cast<int>(battery_percent_f(mv)); }

// ---- Telegram Bot API ---------------------------------------------------------------------------------------------

class Telegram {
 public:
  explicit Telegram(const char *token) : token_(token) {}
  ~Telegram() {
    if (this->client_ != nullptr)
      esp_http_client_cleanup(this->client_);
  }
  // HTTP status, or -1 when the request did not get through (no network, TLS, timeout)
  int call(const char *method, const std::string &body) {
    std::string url = "https://api.telegram.org/bot" + this->token_ + "/" + method;
    this->body_.clear();
    if (this->client_ == nullptr) {
      esp_http_client_config_t cfg{};
      cfg.url = url.c_str();
      cfg.method = HTTP_METHOD_POST;
      cfg.timeout_ms = 12000;
      cfg.crt_bundle_attach = esp_crt_bundle_attach;
      cfg.event_handler = &Telegram::on_event;
      cfg.user_data = &this->body_;
      cfg.buffer_size = 2048;
      cfg.buffer_size_tx = 1024;
      cfg.keep_alive_enable = true;
      this->client_ = esp_http_client_init(&cfg);
      if (this->client_ == nullptr)
        return -1;
    } else {
      esp_http_client_set_url(this->client_, url.c_str());
    }
    esp_http_client_set_method(this->client_, HTTP_METHOD_POST);
    esp_http_client_set_header(this->client_, "Content-Type", "application/json");
    esp_http_client_set_post_field(this->client_, body.data(), static_cast<int>(body.size()));
    esp_err_t err = esp_http_client_perform(this->client_);
    if (err != ESP_OK) {
      ESP_LOGW(TAG, "Telegram %s: %s", method, esp_err_to_name(err));
      return -1;
    }
    int status = esp_http_client_get_status_code(this->client_);
    if (status != 200)
      ESP_LOGW(TAG, "Telegram %s: HTTP %d", method, status);
    return status;
  }
  const std::string &body() const { return this->body_; }
  bool ok() const { return this->body_.find("\"ok\":true") != std::string::npos; }

  // 0 delivered, 1 worth retrying (network, rate limit, server), 2 will not work (blocked, chat gone, bad token)
  // reply_to: message id to answer in the chat (a command in a group), 0 = a new message
  int send(int64_t chat, const std::string &text, bool silent, int64_t reply_to = 0) {
    char head[48];
    snprintf(head, sizeof(head), "{\"chat_id\":%lld,\"text\":\"", static_cast<long long>(chat));
    std::string body = head + jesc(text.c_str()) + "\",\"disable_notification\":" + (silent ? "true" : "false");
    if (reply_to != 0) {
      char r[96];
      snprintf(r, sizeof(r), ",\"reply_parameters\":{\"message_id\":%lld,\"allow_sending_without_reply\":true}",
               static_cast<long long>(reply_to));
      body += r;
    }
    body += "}";
    int status = this->call("sendMessage", body);
    if (status == 200 && this->ok())
      return 0;
    if (status == 400 || status == 401 || status == 403 || status == 404)
      return 2;
    return 1;
  }

 protected:
  static esp_err_t on_event(esp_http_client_event_t *e) {
    if (e->event_id == HTTP_EVENT_ON_DATA && e->user_data != nullptr) {
      auto *s = static_cast<std::string *>(e->user_data);
      if (s->size() + e->data_len <= MAX_BODY)
        s->append(static_cast<const char *>(e->data), e->data_len);
    }
    return ESP_OK;
  }
  std::string token_;
  std::string body_;
  esp_http_client_handle_t client_{nullptr};
};

// ---- message texts ------------------------------------------------------------------------------------------------

struct Texts {
  const char *help;        // alert headline, {n} = name
  const char *help_anon;
  const char *pressed_at;  // time of the press
  const char *presses;     // pressed several times, %u
  const char *late;        // delivered late, %u minutes
  const char *battery;     // %d percent
  const char *checkin;
  const char *checkin_anon;
  const char *low;
  const char *test;
  const char *welcome;  // reply to someone who starts the bot during setup
  const char *full;
};
static const Texts TEXTS[2] = {
    {"🆘 {n} yardım istiyor!", "🆘 Yardım isteniyor!", "Düğmeye basıldığı saat: ", "Düğmeye %u kez basıldı.",
     "Bildirim %u dakika gecikti: kolye o sırada internete bağlanamadı.", "Pil: %%%d",
     "NOQ SOS · {n} · kolye çalışıyor.", "NOQ SOS · kolye çalışıyor.",
     "🔋 NOQ SOS · {n}: kolyenin pili azaldı (%%%d). Kolyeyi şarja takın.",
     "NOQ SOS deneme mesajı ✓\n{n} düğmeye bastığında bildirim bu sohbete gelecek.",
     "✓ Bağlandı. {n} kolyedeki düğmeye bastığında bildirim bu sohbete gelecek. Kolyenin durumunu /durum, pilini /pil ile sorabilirsin.",
     "Bu kolye en çok 6 sohbete bildirim gönderebilir; liste dolu. Kolyenin sayfasından birini çıkarıp yeniden dene."},
    {"🆘 {n} needs help!", "🆘 Help needed!", "Button pressed at ", "The button was pressed %u times.",
     "Delivered %u minutes late: the pendant could not reach the internet at the time.", "Battery: %d%%",
     "NOQ SOS · {n} · the pendant is working.", "NOQ SOS · the pendant is working.",
     "🔋 NOQ SOS · {n}: the pendant battery is low (%d%%). Please charge it.",
     "NOQ SOS test message ✓\nWhen {n} presses the button, the alert comes to this chat.",
     "✓ Connected. When {n} presses the pendant's button, the alert comes to this chat. Ask for its status with /status, its battery with /battery.",
     "This pendant can alert up to 6 chats and the list is full. Remove one on the pendant's page and try again."},
};

// ---- battery texts (0.3.0) ----------------------------------------------------------------------------------------

struct BatTexts {
  const char *charging;  // volts, hundredths
  const char *full;      // volts, hundredths
  const char *last;      // date
  const char *last_full;
  const char *approx;  // date
  const char *left;    // days
  const char *left_one;
  const char *left_calc;
  const char *past;          // days
  const char *checkin_left;  // days
  const char *months[12];
};
static const BatTexts BAT_TEXTS[2] = {
    {"🔌 Şarj oluyor, ≈%%%d (%d,%02d V)", "🔋 Pil dolu (%d,%02d V), şarjda", "🗓 Son şarj: %s", ", dolu", "%s civarı",
     "⏳ Tahmini: yaklaşık %d gün daha", "⏳ Tahmini: yaklaşık bir gün kaldı",
     "⏳ Tahmini: ilk günün ölçümlerinden sonra", "↺ Önceki şarj %d gün gitti.", " · yaklaşık %d gün",
     {"Oca", "Şub", "Mar", "Nis", "May", "Haz", "Tem", "Ağu", "Eyl", "Eki", "Kas", "Ara"}},
    {"🔌 Charging, ≈%d%% (%d.%02d V)", "🔋 Battery full (%d.%02d V), on the charger", "🗓 Last charge: %s", ", to full",
     "around %s", "⏳ Estimate: about %d more days", "⏳ Estimate: about a day left",
     "⏳ Estimate: after the first day of readings", "↺ The previous charge lasted %d days.", " · about %d days",
     {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"}},
};

static std::string day_time(int64_t utc, int tz_min, const BatTexts &bt) {
  time_t local = static_cast<time_t>(utc + tz_min * 60);
  struct tm tm {};
  gmtime_r(&local, &tm);
  char b[32];
  snprintf(b, sizeof(b), "%d %s %02d:%02d", tm.tm_mday, bt.months[tm.tm_mon % 12], tm.tm_hour, tm.tm_min);
  return b;
}

static std::string with_name(const char *tpl, const char *name) {
  std::string s = tpl;
  size_t at = s.find("{n}");
  if (at != std::string::npos)
    s.replace(at, 3, name[0] ? name : "NOQ SOS");
  return s;
}

static std::string fmt(const char *tpl, int v) {
  char b[200];
  snprintf(b, sizeof(b), tpl, v);
  return b;
}

// ---- Telegram commands (0.2.0, Umut's request) --------------------------------------------------------------------
// Turkish and English names; the answer comes in the language of the command. In a group "/durum@bot" is for this
// bot only; another bot's command is left alone. Only chats on the list get an answer.

enum Cmd : uint8_t { CMD_NONE, CMD_STATUS, CMD_BATTERY, CMD_HELP, CMD_START, CMD_WAKE };

struct CmdTexts {
  const char *battery;  // percent, volts, hundredths
  const char *no_battery;
  const char *bad_battery;  // volts, hundredths
  const char *wifi;  // network, signal word, dBm
  const char *good;
  const char *fair;
  const char *weak;
  const char *last;  // time, outcome
  const char *last_ok;
  const char *last_part;
  const char *last_none;
  const char *pending;
  const char *checkin;  // hour
  const char *checkin_off;
  const char *poll;  // minutes
  const char *poll_off;
  const char *version;
  const char *help;
  const char *help_poll;  // minutes
  const char *help_nopoll;
};
static const CmdTexts CMD_TEXTS[2] = {
    {"🔋 Pil: %%%d (%d,%02d V)", "🔋 Pil ölçümü yok: kartta pil bölücüsü takılı değil.",
     "🔋 Pil ölçülemiyor: D0 %d,%02d V okuyor, tek hücre en çok 4,2 V olur. D0 ile GND arasındaki 1 MΩ'u ve GND bağlantısını kontrol edin.",
     "📶 Wi-Fi: %s, sinyal %s (%d dBm)",
     "iyi", "orta", "zayıf", "🆘 Son bildirim: %s, %s", "herkese ulaştı", "herkese ulaşmadı", "🆘 Henüz bildirim gönderilmedi.",
     "⏳ Gönderilmeyi bekleyen bildirim var; kolye yeniden deniyor.", "⏰ Günlük yoklama: %02d:00", "⏰ Günlük yoklama: kapalı",
     "🕑 Uyurken komutlara %u dakikada bir bakar.", "🕑 Uyurken komutlara yalnız düğmeye basılınca ve yoklamada bakar.",
     "Yazılım %s", "NOQ SOS komutları\n/durum: kolyenin durumu\n/pil: pil seviyesi\n/uyan: 10 dakika uyanık kalsın, sayfası açılsın\n/yardim: bu liste",
     "Kolye uyuyorsa komutu en geç %u dakika içinde cevaplar.",
     "Kolye uyuyorsa komutu düğmeye basılınca ya da günlük yoklamada cevaplar."},
    {"🔋 Battery: %d%% (%d.%02d V)", "🔋 No battery reading: the battery divider is not fitted.",
     "🔋 Cannot read the battery: D0 reads %d.%02d V, a single cell is 4.2 V at most. Check the 1 MΩ between D0 and GND and its ground.",
     "📶 Wi-Fi: %s, %s signal (%d dBm)",
     "good", "fair", "weak", "🆘 Last alert: %s, %s", "reached everyone", "did not reach everyone", "🆘 No alert sent yet.",
     "⏳ An alert is waiting to go out; the pendant keeps trying.", "⏰ Daily check-in: %02d:00", "⏰ Daily check-in: off",
     "🕑 While asleep it checks for commands every %u minutes.",
     "🕑 While asleep it checks for commands only on a press and at the check-in.", "Firmware %s",
     "NOQ SOS commands\n/status: the pendant's status\n/battery: battery level\n/wake: stay awake 10 minutes, open its page\n/help: this list",
     "If the pendant is asleep, it answers within %u minutes.",
     "If the pendant is asleep, it answers on the next press or at the daily check-in."},
};

// "/durum", "/Status@noqsos_bot", "/start sos" → command and its language (255: the pendant's message language)
static uint8_t parse_cmd(const char *text, const char *bot, uint8_t &lang) {
  if (text[0] != '/')
    return CMD_NONE;
  const char *p = text + 1, *e = p;
  while (*e && *e != ' ' && *e != '@' && *e != '\n')
    e++;
  std::string name(p, e - p);
  for (char &ch : name)
    ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
  if (*e == '@') {
    const char *a = e + 1, *z = a;
    while (*z && *z != ' ' && *z != '\n')
      z++;
    size_t len = z - a;
    if (len != strlen(bot) || strncasecmp(a, bot, len) != 0)
      return CMD_NONE;  // meant for another bot in the group
  }
  static const struct {
    const char *word;
    uint8_t cmd;
    uint8_t lang;
  } NAMES[] = {{"durum", CMD_STATUS, 0},  {"status", CMD_STATUS, 1}, {"pil", CMD_BATTERY, 0},  {"battery", CMD_BATTERY, 1},
               {"yardim", CMD_HELP, 0},   {"yardım", CMD_HELP, 0},   {"komutlar", CMD_HELP, 0}, {"help", CMD_HELP, 1},
               {"commands", CMD_HELP, 1}, {"start", CMD_START, 255}, {"uyan", CMD_WAKE, 0},  {"wake", CMD_WAKE, 1}};
  for (const auto &n : NAMES) {
    if (name == n.word) {
      lang = n.lang;
      return n.cmd;
    }
  }
  return CMD_NONE;
}

// the "/" menu in Telegram: English for everyone, Turkish for Turkish-language apps (both lists are understood anyway)
static const char *const MENU_EN =
    "{\"commands\":[{\"command\":\"status\",\"description\":\"The pendant's status\"},"
    "{\"command\":\"battery\",\"description\":\"Battery level\"},{\"command\":\"wake\",\"description\":\"Stay awake 10 minutes, open its page\"},"
    "{\"command\":\"help\",\"description\":\"Commands\"}]}";
static const char *const MENU_TR =
    "{\"commands\":[{\"command\":\"durum\",\"description\":\"Kolyenin durumu\"},"
    "{\"command\":\"pil\",\"description\":\"Pil seviyesi\"},{\"command\":\"uyan\",\"description\":\"10 dakika uyanık kalsın, sayfası açılsın\"},"
    "{\"command\":\"yardim\",\"description\":\"Komutlar\"}],"
    "\"language_code\":\"tr\"}";

// ---- settings -----------------------------------------------------------------------------------------------------

void NoqSos::load_settings_() {
  Settings s{};
  nvs_handle_t h;
  bool ok = false;
  if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
    size_t n = sizeof(s);
    ok = nvs_get_blob(h, "cfg", &s, &n) == ESP_OK && s.magic == SETTINGS_MAGIC &&
         (n == sizeof(s) || n == offsetof(Settings, poll_min));
    if (ok && n != sizeof(s)) {
      s.poll_min = POLL_DEFAULT;  // saved by 0.1.x: bot and people stay, command checks start at the default
      s.menu = 0;
    }
    nvs_close(h);
  }
  if (!ok) {
    s = Settings{};
    s.magic = SETTINGS_MAGIC;
    s.hour = 10;
    s.tz_min = 180;  // Türkiye until the page sets the phone's time zone
    s.poll_min = POLL_DEFAULT;
  }
  if (s.poll_min != 0 && s.poll_min != 15 && s.poll_min != 30 && s.poll_min != 60 && s.poll_min != 180)
    s.poll_min = POLL_DEFAULT;
  s.count = std::min<uint8_t>(s.count, MAX_PEOPLE);
  this->cfg_ = s;
}

void NoqSos::save_offset_(int64_t offset) {
  nvs_handle_t h;
  if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK)
    return;
  nvs_set_i64(h, "off", offset);
  nvs_commit(h);
  nvs_close(h);
}

void NoqSos::save_settings_() {
  nvs_handle_t h;
  if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) {
    ESP_LOGW(TAG, "Settings not saved: NVS unavailable");
    return;
  }
  nvs_set_blob(h, "cfg", &this->cfg_, sizeof(this->cfg_));
  nvs_commit(h);
  nvs_close(h);
}

// ---- setup / loop -------------------------------------------------------------------------------------------------

static void worker_entry(void *arg) { static_cast<NoqSos *>(arg)->worker(); }

void NoqSos::setup() {
  s_self = this;
  this->boot_ms_ = millis();
  this->lock_ = xSemaphoreCreateMutex();
  this->load_settings_();
  this->battery_load_();
  if (this->battery_pin_ >= 0) {
    // this component sets up before Wi-Fi: the cell at rest, not under the radio's current
    this->battery_rest_mv_ = this->read_battery_mv_();
    if (this->battery_rest_mv_ > 0) {
      this->battery_mv_ = this->battery_rest_mv_;
      this->battery_pct_ = battery_percent(this->battery_rest_mv_);
      this->battery_read_ = this->boot_ms_ ? this->boot_ms_ : 1;
    }
  }
  if (rtc_state.magic != RTC_MAGIC) {
    rtc_state = RtcState{};
    rtc_state.magic = RTC_MAGIC;
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
      int64_t off = 0;
      if (nvs_get_i64(h, "off", &off) == ESP_OK)
        rtc_state.offset = off;  // after a power loss: commands already answered are not read again
      nvs_close(h);
    }
  }

  gpio_num_t b = static_cast<gpio_num_t>(this->button_pin_);
  rtc_gpio_deinit(b);  // back to a digital pin after the deep sleep wake-up configuration
  gpio_config_t in{};
  in.pin_bit_mask = 1ULL << this->button_pin_;
  in.mode = GPIO_MODE_INPUT;
  in.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&in);
  if (this->led_pin_ >= 0) {
    gpio_reset_pin(static_cast<gpio_num_t>(this->led_pin_));
    gpio_set_direction(static_cast<gpio_num_t>(this->led_pin_), GPIO_MODE_OUTPUT);
  }
  if (this->buzzer_pin_ >= 0) {
    gpio_hold_dis(static_cast<gpio_num_t>(this->buzzer_pin_));
    gpio_reset_pin(static_cast<gpio_num_t>(this->buzzer_pin_));
    gpio_set_direction(static_cast<gpio_num_t>(this->buzzer_pin_), GPIO_MODE_OUTPUT);
  }
  this->out_(false);

  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_EXT0: {
      this->mode_ = Mode::BUTTON;
      rtc_state.pending++;
      if (rtc_state.pressed_at == 0)
        rtc_state.pressed_at = now_utc();
      this->last_press_ = this->boot_ms_;
      this->btn_down_ = true;  // the waking press: armed again once released
      this->play_(FX_PRESS, 1);
      break;
    }
    case ESP_SLEEP_WAKEUP_TIMER:
      this->mode_ = Mode::TIMER;
      break;
    default:
      // power-on, reset, a firmware install: stay up for setup
      this->mode_ = Mode::AWAKE;
      this->awake_until_ = this->boot_ms_ + this->setup_window_;
      this->btn_armed_ = gpio_get_level(b) != 0;
      this->play_(FX_HELLO, 1);
      break;
  }
  ESP_LOGI(TAG, "Woke: %s, pending %u", this->mode_ == Mode::BUTTON ? "button" : this->mode_ == Mode::TIMER ? "timer" : "power-on",
           rtc_state.pending);

  xTaskCreate(worker_entry, "noq_sos", 10240, this, 5, &this->task_);
  // keep the HTTP server up on the home network too; on its own it only runs while the captive portal is open
  this->base_->init();
  this->base_->add_handler_without_auth(this);
}

void NoqSos::dump_config() {
  ESP_LOGCONFIG(TAG, "NOQ SOS:\n  Button GPIO%d, LED GPIO%d, buzzer GPIO%d, battery GPIO%d\n  Bot: %s, recipients: %u",
                this->button_pin_, this->led_pin_, this->buzzer_pin_, this->battery_pin_,
                this->cfg_.bot[0] ? this->cfg_.bot : "(not set)", this->cfg_.count);
}

bool NoqSos::setup_open_() {
  auto *portal = captive_portal::global_captive_portal;
  return portal != nullptr && portal->is_active();
}

void NoqSos::keep_awake_(uint32_t ms) {
  uint32_t until = millis() + ms;
  if (this->mode_ != Mode::AWAKE || static_cast<int32_t>(until - this->awake_until_) > 0)
    this->awake_until_ = until;
  this->mode_ = Mode::AWAKE;
}

void NoqSos::loop() {
  const uint32_t now = millis();
  auto *wifi = wifi::global_wifi_component;
  if (this->first_loop_) {
    this->first_loop_ = false;
    // a configured device never falls back to the open setup network; without a home network it waits for setup
    if (wifi->has_sta())
      wifi->set_ap_timeout(0);
    else
      this->keep_awake_(30 * 60 * 1000);
  }
  this->fx_tick_(now);
  this->button_tick_(now);
  if (this->kesif_until_ != 0 && static_cast<int32_t>(now - this->kesif_until_) >= 0) {
    this->kesif_until_ = 0;  // the button sends alerts again
    ESP_LOGI(TAG, "Discovery ended");
  }
  this->battery_tick_(now);
  if (this->battery_rest_mv_ > 0 && !this->battery_point_done_ && now - this->boot_ms_ >= 1500 && now_utc() != 0) {
    this->battery_point_done_ = true;
    this->battery_wake_(this->battery_rest_mv_);
    if (!usb_serial_jtag_is_connected() && !this->battery_charging_)
      this->battery_point_(this->battery_rest_mv_);
  }

  if (this->reboot_at_ != 0 && static_cast<int32_t>(now - this->reboot_at_) >= 0) {
    App.run_safe_shutdown_hooks();
    if (this->forget_wifi_) {
      // ESPHome keeps the home network in its own NVS namespace; the pendant settings stay in theirs
      nvs_handle_t h;
      if (nvs_open("esphome", NVS_READWRITE, &h) == ESP_OK) {
        nvs_erase_all(h);
        nvs_commit(h);
        nvs_close(h);
      }
    }
    esp_restart();
  }

  bool online = wifi->is_connected();
  if (online && this->online_since_ == 0)
    this->online_since_ = now;
  if (online && this->mode_ == Mode::BUTTON && this->wifi_ms_ == 0)
    this->wifi_ms_ = now;
  if (!online)
    this->online_since_ = 0;

  this->listen_ = this->mode_ == Mode::AWAKE && online && this->cfg_.token[0] != 0 && !this->sleep_now_ &&
                  this->reboot_at_ == 0;
  if (!this->job_busy_) {
    this->finish_job_();
    if (online && this->configured_()) {
      if (rtc_state.pending > 0 && this->alert_attempts_ < 3 && static_cast<int32_t>(now - this->next_attempt_) >= 0) {
        this->alert_attempts_++;
        this->start_job_(Job::ALERT);
      } else if (rtc_state.pending == 0 && !this->low_done_ && this->battery_pct_ >= 0 && this->battery_pct_ <= 20 &&
                 !rtc_state.low_sent && !this->battery_charging_) {
        this->low_done_ = true;
        this->start_job_(Job::LOW_BATTERY);
      } else if (rtc_state.pending == 0 && !this->checkin_done_ && this->checkin_due_(now)) {
        this->checkin_done_ = true;
        this->start_job_(Job::CHECKIN);
      } else if (this->mode_ != Mode::AWAKE && !this->poll_done_ &&
                 (rtc_state.pending == 0 || this->alert_attempts_ >= 3)) {
        // woke from sleep: answer the Telegram commands that came in meanwhile (awake, the worker listens all along)
        this->poll_done_ = true;
        this->start_job_(Job::POLL);
      }
    }
  }
  if (this->battery_pct_ >= 40)
    rtc_state.low_sent = 0;

  // ---- sleep decision
  if (this->job_busy_ || this->fx_ != nullptr || this->reboot_at_ != 0)
    return;
  if (usb_serial_jtag_is_connected() && !this->sleep_now_)
    return;  // on a computer (installer, logs): stay up
  if (this->btn_down_ && now - this->btn_since_ < 30000)
    return;  // sleep only once the button is released (a press would wake it at once)
  bool work_left = online && this->configured_() &&
                   ((rtc_state.pending > 0 && this->alert_attempts_ < 3) || this->checkin_due_(now) ||
                    (!this->low_done_ && this->battery_pct_ >= 0 && this->battery_pct_ <= 20 && !rtc_state.low_sent &&
                     !this->battery_charging_) ||
                    (this->mode_ != Mode::AWAKE && !this->poll_done_));
  if (this->mode_ == Mode::AWAKE) {
    if (!this->sleep_now_ && static_cast<int32_t>(now - this->awake_until_) < 0)
      return;
    if (work_left && !this->sleep_now_)
      return;
  } else {
    bool timeout = now - this->boot_ms_ > (this->mode_ == Mode::BUTTON ? 45000u : 30000u);
    if (work_left && !timeout)
      return;
    if (!online && !timeout && this->configured_())
      return;  // still joining the network
  }
  this->go_sleep_();
}

// ---- feedback, button, battery ------------------------------------------------------------------------------------

void NoqSos::out_(bool on) {
  if (this->led_pin_ >= 0)
    gpio_set_level(static_cast<gpio_num_t>(this->led_pin_), on != this->led_inverted_);
  if (this->buzzer_pin_ >= 0)
    gpio_set_level(static_cast<gpio_num_t>(this->buzzer_pin_), on);
}

void NoqSos::play_(const uint16_t *steps, uint8_t len) {
  this->fx_ = steps;
  this->fx_len_ = len;
  this->fx_i_ = 0;
  this->fx_next_ = millis() + steps[0];
  this->out_(true);
}

void NoqSos::fx_tick_(uint32_t now) {
  if (this->fx_ == nullptr || static_cast<int32_t>(now - this->fx_next_) < 0)
    return;
  this->fx_i_++;
  if (this->fx_i_ >= this->fx_len_) {
    this->fx_ = nullptr;
    this->out_(false);
    return;
  }
  this->out_(this->fx_i_ % 2 == 0);
  this->fx_next_ = now + this->fx_[this->fx_i_];
}

void NoqSos::button_tick_(uint32_t now) {
  bool down = gpio_get_level(static_cast<gpio_num_t>(this->button_pin_)) == 0;
  if (down != this->btn_down_) {
    if (now - this->btn_edge_ < 40)
      return;  // contact bounce
    this->btn_edge_ = now;
    this->btn_down_ = down;
    if (down)
      this->btn_since_ = now;
    if (!down) {
      this->btn_armed_ = true;
      return;
    }
    if (!this->btn_armed_ || now - this->last_press_ < 3000)
      return;
    this->last_press_ = now;
    if (this->kesif_until_ != 0) {  // discovery: the press reports the signal here, no alert
      this->kesif_press_(now);
      return;
    }
    this->alert_from_wake_ = false;
    xSemaphoreTake(this->lock_, portMAX_DELAY);
    rtc_state.pending++;
    if (rtc_state.pressed_at == 0)
      rtc_state.pressed_at = now_utc();
    xSemaphoreGive(this->lock_);
    this->alert_attempts_ = 0;
    this->next_attempt_ = now;
    this->play_(this->configured_() ? FX_PRESS : FX_FAIL, 1);
    ESP_LOGI(TAG, "Button pressed, pending %u", rtc_state.pending);
  } else if (!down) {
    this->btn_since_ = now;
  }
}

void NoqSos::kesif_press_(uint32_t now) {
  auto *wifi = wifi::global_wifi_component;
  int rssi = wifi->is_connected() ? wifi->wifi_rssi() : 0;
  if (rssi == 0)
    this->play_(FX_FAIL, 1);
  else if (rssi >= -60)
    this->play_(FX_PRESS, 1);
  else if (rssi >= -70)
    this->play_(FX_K2, 3);
  else
    this->play_(FX_K3, 5);
  if (this->kesif_n_ == KESIF_MAX) {
    for (uint8_t i = 1; i < KESIF_MAX; i++)
      this->kesif_pts_[i - 1] = this->kesif_pts_[i];
    this->kesif_n_--;
  }
  this->kesif_pts_[this->kesif_n_].utc = now_utc();
  this->kesif_pts_[this->kesif_n_].rssi = static_cast<int8_t>(std::max(rssi, -127));
  this->kesif_n_++;
  ESP_LOGI(TAG, "Discovery spot %u: %d dBm", this->kesif_n_, rssi);
}

int NoqSos::read_battery_mv_() {
  if (this->battery_pin_ < 0)
    return -1;
  if (this->adc_ == nullptr) {
    adc_unit_t unit;
    adc_channel_t ch;
    if (adc_oneshot_io_to_channel(this->battery_pin_, &unit, &ch) != ESP_OK || unit != ADC_UNIT_1)
      return -1;
    adc_oneshot_unit_handle_t h;
    adc_oneshot_unit_init_cfg_t ucfg{};
    ucfg.unit_id = ADC_UNIT_1;
    if (adc_oneshot_new_unit(&ucfg, &h) != ESP_OK)
      return -1;
    adc_oneshot_chan_cfg_t ccfg{};
    ccfg.atten = ADC_ATTEN_DB_12;
    ccfg.bitwidth = ADC_BITWIDTH_DEFAULT;
    adc_oneshot_config_channel(h, ch, &ccfg);
    adc_cali_handle_t cali = nullptr;
    adc_cali_curve_fitting_config_t cc{};
    cc.unit_id = ADC_UNIT_1;
    cc.chan = ch;
    cc.atten = ADC_ATTEN_DB_12;
    cc.bitwidth = ADC_BITWIDTH_DEFAULT;
    adc_cali_create_scheme_curve_fitting(&cc, &cali);
    this->adc_ = h;
    this->cali_ = cali;
    this->adc_channel_ = ch;
  }
  int sum = 0, n = 0, lo = 1 << 30, hi = -1;
  for (int i = 0; i < 8; i++) {
    int raw = 0, mv = 0;
    if (adc_oneshot_read(static_cast<adc_oneshot_unit_handle_t>(this->adc_), static_cast<adc_channel_t>(this->adc_channel_),
                         &raw) != ESP_OK)
      continue;
    if (this->cali_ != nullptr)
      adc_cali_raw_to_voltage(static_cast<adc_cali_handle_t>(this->cali_), raw, &mv);
    else
      mv = raw * 3100 / 4095;
    sum += mv;
    lo = std::min(lo, mv);
    hi = std::max(hi, mv);
    n++;
  }
  if (n == 0)
    return -1;
  int mv = static_cast<int>(sum / n * this->battery_ratio_);
  if (mv < 2500)
    return -1;  // nothing on the pin: no divider fitted
  // A single cell never reads above ~4.35 V, not even on the charger, and a divider with its 100 nF holds still. Above that,
  // or wandering, the pin is not seeing the cell (the lower resistor or its ground open, or the divider not connected):
  // no percentage, no charge, nothing logged.
  if (mv > BAT_MAX_MV || static_cast<int>((hi - lo) * this->battery_ratio_) > BAT_SPREAD_MV) {
    this->battery_fault_mv_ = mv;
    return -2;
  }
  this->battery_fault_mv_ = 0;
  return mv;
}

void NoqSos::battery_tick_(uint32_t now) {
  bool power = usb_serial_jtag_is_connected();
  uint32_t every = (power || this->battery_charging_) ? 15000 : 60000;
  if (this->battery_pin_ < 0 || (this->battery_read_ != 0 && now - this->battery_read_ < every))
    return;
  if (this->battery_read_ == 0 && now - this->boot_ms_ < 150)
    return;
  this->battery_read_ = now;
  int mv = this->read_battery_mv_();
  // Charging: on a computer's USB, or the cell keeps rising while awake (a wall charger). The XIAO's charge LED is
  // not wired to a pin, so this is all the board can know. On its own the cell only falls.
  if (mv > 0 && this->battery_mv_ > 0) {
    int d = mv - this->battery_mv_;
    if (d >= 8 && this->battery_rise_ < 3)
      this->battery_rise_++;
    else if (d <= -8)
      this->battery_rise_ = 0;
  }
  this->battery_mv_ = mv;
  this->battery_pct_ = mv < 0 ? -1 : battery_percent(mv);
  bool was = this->battery_charging_;
  this->battery_charging_ = mv > 0 && (power || this->battery_rise_ >= 2);
  if (!was && this->battery_charging_) {
    this->battery_charge_since_ = now;
    this->battery_charge_logged_ = false;
  }
  if (this->battery_charging_ && !this->battery_charge_logged_ && mv >= 4170)
    this->battery_charge_logged_ = this->battery_charged_(true, true);
  else if (was && !this->battery_charging_ && !this->battery_charge_logged_ && mv > 0 &&
           now - this->battery_charge_since_ >= 10 * 60 * 1000)
    this->battery_charge_logged_ = this->battery_charged_(mv >= 4100, true);  // taken off before full
}

void NoqSos::battery_load_() {
  BatLog l{};
  size_t n = sizeof(l);
  nvs_handle_t h;
  if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
    if (nvs_get_blob(h, "bat", &l, &n) == ESP_OK && n == sizeof(l) && l.magic == BATLOG_MAGIC)
      this->bat_ = l;
    nvs_close(h);
  }
  bool bozuk = this->bat_.last_mv > BAT_MAX_MV;
  for (uint8_t i = 0; i < this->bat_.n && i < BAT_POINTS; i++)
    bozuk = bozuk || this->bat_.pts[i].mv > BAT_MAX_MV;
  if (this->bat_.magic != BATLOG_MAGIC || bozuk) {  // a log written from impossible readings (charges, days) is not kept
    this->bat_ = BatLog{};
    this->bat_.magic = BATLOG_MAGIC;
    if (bozuk)
      this->battery_save_();
  }
}

void NoqSos::battery_save_() {
  nvs_handle_t h;
  if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK)
    return;
  nvs_set_blob(h, "bat", &this->bat_, sizeof(this->bat_));
  nvs_commit(h);
  nvs_close(h);
}

// A wake-up's rest reading: a jump since the last one means it was charged while asleep (a wall charger).
void NoqSos::battery_wake_(int mv) {
  if (this->bat_.last_mv > 0 && mv - this->bat_.last_mv >= 150 && mv >= 3900)
    this->battery_charged_(mv >= 4100, false);
}

// At most one point every 6 hours, from an hour after a charge (the surface charge is gone by then).
void NoqSos::battery_point_(int mv) {
  BatLog &l = this->bat_;
  int64_t t = now_utc();
  if (t == 0 || (l.charged_at != 0 && t - l.charged_at < 3600) || (l.n > 0 && t - l.pts[l.n - 1].t < 6 * 3600))
    return;
  if (l.n == BAT_POINTS) {  // full: keep the first point and every other one, so the span still reaches the charge
    uint8_t j = 1;
    for (uint8_t i = 2; i < l.n; i += 2)
      l.pts[j++] = l.pts[i];
    l.n = j;
  }
  l.pts[l.n].t = t;
  l.pts[l.n].mv = mv;
  l.n++;
  l.last_mv = mv;
  this->battery_save_();
}

bool NoqSos::battery_charged_(bool full, bool seen) {
  BatLog &l = this->bat_;
  int64_t t = now_utc();
  if (t == 0)
    return false;
  if (l.charged_at != 0 && t - l.charged_at < 6 * 3600) {  // the same charge seen again (wake-up, then full)
    l.full = l.full || full;
    l.seen = l.seen || seen;
  } else {
    if (l.charged_at != 0 && t - l.charged_at >= 12 * 3600) {  // the previous charge lasted until now
      for (int i = BAT_PAST - 1; i > 0; i--) {
        l.past_days[i] = l.past_days[i - 1];
        l.past_end[i] = l.past_end[i - 1];
      }
      l.past_days[0] = static_cast<float>(t - l.charged_at) / 86400.0f;
      l.past_end[0] = static_cast<uint8_t>(battery_percent(l.n ? l.pts[l.n - 1].mv : l.last_mv));
    }
    if (l.charges < 65535)
      l.charges++;
    l.full = full;
    l.seen = seen;
  }
  l.charged_at = t;
  l.n = 0;
  l.last_mv = this->battery_mv_ > 0 ? this->battery_mv_ : l.last_mv;
  this->battery_save_();
  ESP_LOGI(TAG, "Battery charged%s (%s), %d mV", full ? " to full" : "", seen ? "seen" : "found on wake-up", this->battery_mv_);
  return true;
}

// Days left from the fall of the rest readings since the last charge (least squares, percent against days).
float NoqSos::battery_days_left_() const {
  const BatLog &l = this->bat_;
  if (l.n < 2 || l.pts[l.n - 1].t - l.pts[0].t < 20 * 3600)
    return -1.0f;
  double sx = 0, sy = 0, sxx = 0, sxy = 0;
  for (uint8_t i = 0; i < l.n; i++) {
    double x = static_cast<double>(l.pts[i].t - l.pts[0].t) / 86400.0, y = battery_percent_f(l.pts[i].mv);
    sx += x;
    sy += y;
    sxx += x * x;
    sxy += x * y;
  }
  double den = l.n * sxx - sx * sx;
  if (den <= 0)
    return -1.0f;
  double slope = (l.n * sxy - sx * sy) / den;  // percent per day, negative while it runs down
  double pct = battery_percent_f(l.pts[l.n - 1].mv);
  double left = slope < -0.05 ? pct / -slope : 365.0;
  int64_t t = now_utc();
  if (t != 0)
    left -= static_cast<double>(t - l.pts[l.n - 1].t) / 86400.0;
  return static_cast<float>(std::min(std::max(left, 0.0), 365.0));
}

std::string NoqSos::battery_text_(const Settings &c, uint8_t lang) {
  const CmdTexts &tx = CMD_TEXTS[lang ? 1 : 0];
  const BatTexts &bt = BAT_TEXTS[lang ? 1 : 0];
  char b[200];
  int mv = this->battery_mv_;
  if (mv == -2) {
    snprintf(b, sizeof(b), tx.bad_battery, this->battery_fault_mv_ / 1000, this->battery_fault_mv_ % 1000 / 10);
    return b;
  }
  if (mv <= 0)
    return tx.no_battery;
  if (this->battery_charging_ && mv >= 4170)
    snprintf(b, sizeof(b), bt.full, mv / 1000, mv % 1000 / 10);
  else if (this->battery_charging_)
    snprintf(b, sizeof(b), bt.charging, this->battery_pct_, mv / 1000, mv % 1000 / 10);
  else
    snprintf(b, sizeof(b), tx.battery, this->battery_pct_, mv / 1000, mv % 1000 / 10);
  std::string o = b;
  const BatLog &l = this->bat_;
  if (l.charged_at != 0) {
    std::string d = day_time(l.charged_at, c.tz_min, bt);
    if (!l.seen) {
      snprintf(b, sizeof(b), bt.approx, d.c_str());
      d = b;
    }
    snprintf(b, sizeof(b), bt.last, d.c_str());
    o += "\n" + std::string(b) + (l.full ? bt.last_full : "");
  }
  if (!this->battery_charging_) {
    float left = this->battery_days_left_();
    if (left >= 1.5f) {
      snprintf(b, sizeof(b), bt.left, static_cast<int>(left + 0.5f));
      o += "\n" + std::string(b);
    } else if (left >= 0.0f) {
      o += "\n" + std::string(bt.left_one);
    } else {
      o += "\n" + std::string(bt.left_calc);
    }
  }
  if (l.past_days[0] > 0.0f) {
    snprintf(b, sizeof(b), bt.past, std::max(1, static_cast<int>(l.past_days[0] + 0.5f)));
    o += "\n" + std::string(b);
  }
  return o;
}

// ---- jobs (worker task) -------------------------------------------------------------------------------------------

bool NoqSos::start_job_(Job job, const std::string &arg) {
  if (this->job_busy_ || this->task_ == nullptr)
    return false;
  this->job_ = job;
  this->job_arg_ = arg;
  this->job_busy_ = true;
  xTaskNotifyGive(this->task_);
  return true;
}

void NoqSos::worker() {
  for (;;) {
    if (ulTaskNotifyTake(pdTRUE, this->listen_ ? pdMS_TO_TICKS(100) : pdMS_TO_TICKS(1000)) == 0) {
      if (this->listen_)
        this->poll_updates_(true, 3);
      continue;
    }
    xSemaphoreTake(this->lock_, portMAX_DELAY);
    Settings c = this->cfg_;
    Job job = this->job_;
    std::string arg = this->job_arg_;
    xSemaphoreGive(this->lock_);
    std::string msg;
    bool ok = false;
    switch (job) {
      case Job::ALERT:
        ok = this->run_alert_(c, msg);
        break;
      case Job::CHECKIN:
      case Job::LOW_BATTERY:
      case Job::TEST:
        ok = this->run_broadcast_(c, job, msg);
        break;
      case Job::TOKEN:
        ok = this->run_token_(arg, msg);
        break;
      case Job::POLL:
        this->poll_updates_(false, 0);
        ok = true;
        break;
      default:
        break;
    }
    xSemaphoreTake(this->lock_, portMAX_DELAY);
    this->last_job_ = job;
    this->last_ok_ = ok;
    this->last_msg_ = msg;
    this->last_seen_ = false;
    this->job_busy_ = false;
    xSemaphoreGive(this->lock_);
  }
}

// Main loop side of a finished job: feedback and what to do next.
void NoqSos::finish_job_() {
  if (this->last_seen_)
    return;
  this->last_seen_ = true;
  uint32_t now = millis();
  switch (this->last_job_) {
    case Job::ALERT:
      if (this->last_ok_) {
        this->play_(FX_SENT, 5);
        // the waking press counts from the start of the app; a press while awake from the press itself
        rtc_state.last_sent_ms = this->mode_ == Mode::BUTTON && this->alert_from_wake_ ? now : now - this->last_press_;
        rtc_state.last_wifi_ms = this->mode_ == Mode::BUTTON && this->alert_from_wake_ ? this->wifi_ms_ : 0;
        this->alert_from_wake_ = false;
        ESP_LOGI(TAG, "Alert delivered %ums after the press (network at %ums)", static_cast<unsigned>(rtc_state.last_sent_ms),
                 static_cast<unsigned>(rtc_state.last_wifi_ms));
      } else {
        this->next_attempt_ = now + 5000;
        if (this->alert_attempts_ >= 3)
          this->play_(FX_FAIL, 1);
      }
      break;
    case Job::CHECKIN:
      if (this->last_ok_) {
        rtc_state.next_check = 0;  // recomputed from the clock below
        int64_t t = now_utc();
        if (t != 0 && this->cfg_.hour >= 0) {
          int64_t local = t + this->cfg_.tz_min * 60;
          int64_t next = local - local % 86400 + this->cfg_.hour * 3600;
          if (next <= local + 3600)
            next += 86400;
          rtc_state.next_check = next - this->cfg_.tz_min * 60;
        }
      }
      break;
    case Job::LOW_BATTERY:
      if (this->last_ok_)
        rtc_state.low_sent = 1;
      break;
    case Job::TEST:
      this->play_(this->last_ok_ ? FX_SENT : FX_FAIL, this->last_ok_ ? 5 : 1);
      break;
    default:
      break;
  }
}

bool NoqSos::run_alert_(const Settings &c, std::string &msg) {
  xSemaphoreTake(this->lock_, portMAX_DELAY);
  uint16_t presses = rtc_state.pending;
  int64_t pressed = rtc_state.pressed_at;
  uint8_t mask = rtc_state.sent_mask;
  xSemaphoreGive(this->lock_);
  if (presses == 0)
    return true;

  const Texts &tx = TEXTS[c.lang ? 1 : 0];
  int64_t t = now_utc();
  std::string text = c.name[0] ? with_name(tx.help, c.name) : std::string(tx.help_anon);
  if (pressed != 0)
    text += "\n" + std::string(tx.pressed_at) + hhmm(pressed, c.tz_min);
  if (presses > 1)
    text += "\n" + fmt(tx.presses, presses);
  if (this->battery_pct_ >= 0)
    text += "\n" + fmt(tx.battery, this->battery_pct_);
  if (pressed != 0 && t != 0 && t - pressed >= 120)
    text += "\n" + fmt(tx.late, static_cast<int>((t - pressed) / 60));

  Telegram tg(c.token);
  int retry = 0;
  for (uint8_t i = 0; i < c.count; i++) {
    if (mask & (1u << i))
      continue;
    int r = tg.send(c.people[i].id, text, false);
    if (r == 0) {
      mask |= 1u << i;
    } else if (r == 2) {
      mask |= 1u << i;  // blocked the bot or left the group: retrying will not help
      ESP_LOGW(TAG, "Recipient %u does not accept messages from the bot", i);
    } else {
      retry++;
    }
  }

  xSemaphoreTake(this->lock_, portMAX_DELAY);
  bool done = retry == 0;
  if (done) {
    rtc_state.pending = rtc_state.pending > presses ? rtc_state.pending - presses : 0;  // presses during the send stay
    rtc_state.sent_mask = 0;
    rtc_state.tries = 0;
    rtc_state.pressed_at = rtc_state.pending ? now_utc() : 0;
    rtc_state.last_alert = t;
    rtc_state.last_ok = 1;
  } else {
    rtc_state.sent_mask = mask;
    if (rtc_state.tries < 255)
      rtc_state.tries++;
    rtc_state.last_ok = 0;
  }
  xSemaphoreGive(this->lock_);
  ESP_LOGI(TAG, "Alert %s (%u to retry)", done ? "delivered" : "not delivered", retry);
  msg = done ? "sent" : "retry";
  return done;
}

bool NoqSos::run_broadcast_(const Settings &c, Job job, std::string &msg) {
  if (c.token[0] == 0 || c.count == 0) {
    msg = "no_people";
    return false;
  }
  const Texts &tx = TEXTS[c.lang ? 1 : 0];
  std::string text;
  bool silent = false;
  if (job == Job::CHECKIN) {
    text = c.name[0] ? with_name(tx.checkin, c.name) : std::string(tx.checkin_anon);
    if (this->battery_pct_ >= 0) {
      text += " " + fmt(tx.battery, this->battery_pct_);
      float left = this->battery_charging_ ? -1.0f : this->battery_days_left_();
      if (left >= 1.5f)
        text += fmt(BAT_TEXTS[c.lang ? 1 : 0].checkin_left, static_cast<int>(left + 0.5f));
    }
    silent = true;
  } else if (job == Job::LOW_BATTERY) {
    text = fmt(with_name(tx.low, c.name).c_str(), std::max(this->battery_pct_, 0));
    float left = this->battery_days_left_();
    const BatTexts &bt = BAT_TEXTS[c.lang ? 1 : 0];
    if (left >= 1.5f)
      text += "\n" + fmt(bt.left, static_cast<int>(left + 0.5f));
    else if (left >= 0.0f)
      text += "\n" + std::string(bt.left_one);
  } else {
    text = with_name(tx.test, c.name);
  }
  Telegram tg(c.token);
  int sent = 0;
  for (uint8_t i = 0; i < c.count; i++)
    if (tg.send(c.people[i].id, text, silent) == 0)
      sent++;
  char b[16];
  snprintf(b, sizeof(b), "%d/%u", sent, c.count);
  msg = b;
  return sent > 0;
}

bool NoqSos::run_token_(const std::string &token, std::string &msg) {
  Telegram tg(token.c_str());
  int status = tg.call("getMe", "{}");
  if (status != 200 || !tg.ok()) {
    msg = status < 0 ? "network" : "token";
    return false;
  }
  std::string user;
  json::parse_json(tg.body(), [&user](JsonObject root) -> bool {
    user = root["result"]["username"] | "";
    return true;
  });
  if (user.empty()) {
    msg = "token";
    return false;
  }
  xSemaphoreTake(this->lock_, portMAX_DELAY);
  bool same = strncmp(this->cfg_.token, token.c_str(), sizeof(this->cfg_.token)) == 0;
  copy_utf8(this->cfg_.token, sizeof(this->cfg_.token), token);
  copy_utf8(this->cfg_.bot, sizeof(this->cfg_.bot), user);
  if (!same) {
    this->cfg_.count = 0;  // a new bot cannot write to the old bot's chats
    this->cfg_.menu = 0;   // and has no command menu yet
    rtc_state.offset = 0;
    this->save_offset_(0);
  }
  this->save_settings_();
  xSemaphoreGive(this->lock_);
  msg = user;
  return true;
}

// Telegram updates. While setup is open (the pendant's page, or USB) anyone who taps Start in the bot or adds it to a group
// becomes a recipient and gets a reply, so nobody has to find and pick chats by hand. Always, setup or a wake from sleep:
// commands from chats on the list are answered as a reply to the command (/durum /pil /yardim, /status /battery /help;
// 0.2.0, Umut's request). Outside setup nobody new is added and strangers get no answer.
void NoqSos::poll_updates_(bool setup, int timeout) {
  xSemaphoreTake(this->lock_, portMAX_DELAY);
  Settings c = this->cfg_;
  int64_t offset = rtc_state.offset;
  xSemaphoreGive(this->lock_);
  if (c.token[0] == 0)
    return;
  Telegram tg(c.token);
  if (c.menu != MENU_VERSION) {
    bool ok = tg.call("setMyCommands", MENU_EN) == 200 && tg.ok();
    ok = ok && tg.call("setMyCommands", MENU_TR) == 200 && tg.ok();
    if (ok) {
      xSemaphoreTake(this->lock_, portMAX_DELAY);
      this->cfg_.menu = MENU_VERSION;
      this->save_settings_();
      xSemaphoreGive(this->lock_);
      ESP_LOGI(TAG, "Command menu registered");
    }
  }
  char body[120];
  snprintf(body, sizeof(body), "{\"offset\":%lld,\"timeout\":%d,\"allowed_updates\":[\"message\",\"my_chat_member\"]}",
           static_cast<long long>(offset), timeout);
  int status = tg.call("getUpdates", body);
  if (status != 200 || !tg.ok()) {
    if (setup)
      vTaskDelay(pdMS_TO_TICKS(status == 409 ? 15000 : 3000));  // 409: the bot has a webhook elsewhere
    return;
  }
  struct Ask {
    int64_t chat;
    int64_t msg;
    uint8_t cmd;
    uint8_t lang;
  };
  Person hits[8]{};
  Ask asks[8]{};
  uint8_t n = 0, na = 0;
  int64_t last = -1;
  json::parse_json(tg.body(), [&](JsonObject root) -> bool {
    JsonArray updates = root["result"];
    for (JsonObject u : updates) {
      int64_t uid = u["update_id"] | static_cast<int64_t>(-1);
      last = std::max(last, uid);
      const char *text = u["message"]["text"] | "";
      uint8_t lang = 255;
      uint8_t cmd = parse_cmd(text, c.bot, lang);
      if (cmd != CMD_NONE && !(setup && cmd == CMD_START) && na < 8) {
        int64_t chat = u["message"]["chat"]["id"] | static_cast<int64_t>(0);
        bool dup = chat == 0;
        for (uint8_t k = 0; k < na; k++)
          dup = dup || (asks[k].chat == chat && asks[k].cmd == cmd);  // the same question twice gets one answer
        if (!dup) {
          asks[na].chat = chat;
          asks[na].msg = u["message"]["message_id"] | static_cast<int64_t>(0);
          asks[na].cmd = cmd == CMD_START ? static_cast<uint8_t>(CMD_HELP) : cmd;  // Start outside setup: the command list
          asks[na].lang = lang == 255 ? c.lang : lang;
          na++;
        }
      }
      if (!setup)
        continue;
      JsonObject chat;
      if (cmd == CMD_START) {
        chat = u["message"]["chat"];
      } else {
        const char *st = u["my_chat_member"]["new_chat_member"]["status"] | "";
        if (strcmp(st, "member") == 0 || strcmp(st, "administrator") == 0)
          chat = u["my_chat_member"]["chat"];
      }
      if (chat.isNull() || n >= 8)
        continue;
      int64_t id = chat["id"] | static_cast<int64_t>(0);
      bool dup = id == 0;
      for (uint8_t k = 0; k < n; k++)
        dup = dup || hits[k].id == id;  // adding to a group brings both a member update and a /start
      if (dup)
        continue;
      std::string label = chat["title"] | "";
      if (label.empty()) {
        label = chat["first_name"] | "";
        const char *ln = chat["last_name"] | "";
        if (ln[0])
          label += std::string(" ") + ln;
      }
      if (label.empty())
        label = std::string("@") + (chat["username"] | "");
      hits[n].id = id;
      copy_utf8(hits[n].label, sizeof(hits[n].label), label);
      n++;
    }
    return true;
  });
  if (last >= 0) {
    xSemaphoreTake(this->lock_, portMAX_DELAY);
    rtc_state.offset = last + 1;
    xSemaphoreGive(this->lock_);
    this->save_offset_(last + 1);
  }
  const Texts &tx = TEXTS[c.lang ? 1 : 0];
  for (uint8_t i = 0; i < n; i++) {
    bool full = false;
    xSemaphoreTake(this->lock_, portMAX_DELAY);
    bool known = false;
    for (uint8_t k = 0; k < this->cfg_.count; k++)
      known = known || this->cfg_.people[k].id == hits[i].id;
    if (!known && this->cfg_.count < MAX_PEOPLE) {
      this->cfg_.people[this->cfg_.count++] = hits[i];
      this->save_settings_();
      ESP_LOGI(TAG, "Recipient added (%u)", this->cfg_.count);
    } else if (!known) {
      full = true;
    }
    xSemaphoreGive(this->lock_);
    // one reply per chat a minute, however many updates it sent
    static int64_t replied_id[4] = {0, 0, 0, 0};
    static uint32_t replied_at[4] = {0, 0, 0, 0};
    static uint8_t replied_i = 0;
    bool recent = false;
    for (uint8_t k = 0; k < 4; k++)
      recent = recent || (replied_id[k] == hits[i].id && millis() - replied_at[k] < 60000);
    if (recent)
      continue;
    replied_id[replied_i] = hits[i].id;
    replied_at[replied_i] = millis();
    replied_i = (replied_i + 1) % 4;
    tg.send(hits[i].id, full ? std::string(tx.full) : with_name(tx.welcome, c.name), false);
  }
  for (uint8_t i = 0; i < na; i++) {
    xSemaphoreTake(this->lock_, portMAX_DELAY);
    Settings now = this->cfg_;
    xSemaphoreGive(this->lock_);
    bool known = false;
    for (uint8_t k = 0; k < now.count; k++)
      known = known || now.people[k].id == asks[i].chat;
    if (!known)
      continue;
    tg.send(asks[i].chat, this->command_text_(now, asks[i].cmd, asks[i].lang), false, asks[i].msg);
    ESP_LOGI(TAG, "Answered a command (%u)", asks[i].cmd);
  }
}

std::string NoqSos::command_text_(const Settings &c, uint8_t cmd, uint8_t lang) {
  const CmdTexts &tx = CMD_TEXTS[lang ? 1 : 0];
  char b[200];
  if (cmd == CMD_WAKE) {
    // the pendant is wrapped up and RESET is out of reach: this is how its page opens later (battery, people, settings)
    static const char *const WAKE[2] = {
        "☀️ Kolye 10 dakika uyanık. Ev Wi-Fi'sindeyken sayfası: http://%s.local\nSayfa açık kaldıkça uyumaz.",
        "☀️ The pendant is awake for 10 minutes. On the home Wi-Fi its page is http://%s.local\nIt stays awake while the page is open."};
    this->keep_awake_(10 * 60 * 1000);
    snprintf(b, sizeof(b), WAKE[lang ? 1 : 0], App.get_name().c_str());
    return b;
  }
  std::string bat = this->battery_text_(c, lang);
  if (cmd == CMD_BATTERY)
    return bat;
  if (cmd != CMD_STATUS) {
    if (c.poll_min > 0)
      snprintf(b, sizeof(b), tx.help_poll, static_cast<unsigned>(c.poll_min));
    else
      snprintf(b, sizeof(b), "%s", tx.help_nopoll);
    return std::string(tx.help) + "\n\n" + b;
  }
  std::string o = "NOQ SOS";
  if (c.name[0])
    o += std::string(" · ") + c.name;
  o += "\n" + bat;
  auto *wifi = wifi::global_wifi_component;
  if (wifi->is_connected()) {
    char ssid[wifi::SSID_BUFFER_SIZE];
    int rssi = wifi->wifi_rssi();
    snprintf(b, sizeof(b), tx.wifi, wifi->wifi_ssid_to(ssid), rssi >= -60 ? tx.good : rssi >= -70 ? tx.fair : tx.weak, rssi);
    o += "\n" + std::string(b);
  }
  if (rtc_state.pending > 0) {
    o += "\n" + std::string(tx.pending);
  } else if (rtc_state.last_alert != 0) {
    snprintf(b, sizeof(b), tx.last, hhmm(rtc_state.last_alert, c.tz_min).c_str(), rtc_state.last_ok ? tx.last_ok : tx.last_part);
    o += "\n" + std::string(b);
  } else {
    o += "\n" + std::string(tx.last_none);
  }
  if (c.hour >= 0)
    snprintf(b, sizeof(b), tx.checkin, c.hour);
  else
    snprintf(b, sizeof(b), "%s", tx.checkin_off);
  o += "\n" + std::string(b);
  if (c.poll_min > 0)
    snprintf(b, sizeof(b), tx.poll, static_cast<unsigned>(c.poll_min));
  else
    snprintf(b, sizeof(b), "%s", tx.poll_off);
  o += "\n" + std::string(b);
#ifdef ESPHOME_PROJECT_VERSION
  snprintf(b, sizeof(b), tx.version, ESPHOME_PROJECT_VERSION);
  o += "\n" + std::string(b);
#endif
  return o;
}

// ---- sleep --------------------------------------------------------------------------------------------------------

bool NoqSos::checkin_due_(uint32_t now) {
  if (this->checkin_done_ || this->cfg_.hour < 0)
    return false;
  int64_t t = now_utc();
  if (t == 0) {
    // no clock yet: a sleep timed for the check-in counts once the network has had time to set the clock
    return this->mode_ == Mode::TIMER && rtc_state.timer_checkin && this->online_since_ != 0 &&
           now - this->online_since_ > 8000;
  }
  if (rtc_state.next_check == 0) {
    int64_t local = t + this->cfg_.tz_min * 60;
    int64_t next = local - local % 86400 + this->cfg_.hour * 3600;
    if (next <= local + 60)
      next += 86400;
    rtc_state.next_check = next - this->cfg_.tz_min * 60;
    return false;
  }
  return t >= rtc_state.next_check - 90;
}

uint32_t NoqSos::sleep_seconds_() {
  static const uint16_t BACKOFF[] = {60, 120, 300, 600};
  uint32_t s = 24 * 3600;
  rtc_state.timer_checkin = 0;
  if (rtc_state.pending > 0 && this->configured_()) {
    s = BACKOFF[std::min<uint8_t>(rtc_state.tries, 3)];
  } else if (this->cfg_.hour >= 0 && this->configured_()) {
    int64_t t = now_utc();
    if (t != 0 && rtc_state.next_check != 0) {
      int64_t d = rtc_state.next_check - t;
      s = static_cast<uint32_t>(std::max<int64_t>(60, std::min<int64_t>(d, 26 * 3600)));
    }
    rtc_state.timer_checkin = 1;
  }
  // Telegram commands: wake this often to answer them (0.2.0); a sooner check-in or retry keeps its own time
  if (this->cfg_.poll_min > 0 && this->configured_() && rtc_state.pending == 0 && this->cfg_.poll_min * 60u < s) {
    s = this->cfg_.poll_min * 60u;
    rtc_state.timer_checkin = 0;
  }
  return s;
}

void NoqSos::go_sleep_() {
  if (!this->configured_())
    rtc_state.pending = 0;  // nobody to tell; a press after setup starts afresh
  uint32_t s = this->sleep_seconds_();
  ESP_LOGI(TAG, "Sleeping %us (pending %u)", static_cast<unsigned>(s), rtc_state.pending);
  this->out_(false);
  App.run_safe_shutdown_hooks();
  if (this->buzzer_pin_ >= 0) {
    gpio_set_level(static_cast<gpio_num_t>(this->buzzer_pin_), 0);
    gpio_hold_en(static_cast<gpio_num_t>(this->buzzer_pin_));
    gpio_deep_sleep_hold_en();
  }
  gpio_num_t b = static_cast<gpio_num_t>(this->button_pin_);
  if (gpio_get_level(b) != 0) {
    esp_sleep_enable_ext0_wakeup(b, 0);
    rtc_gpio_pullup_en(b);
    rtc_gpio_pulldown_dis(b);
  }  // a button stuck down would wake it at once: timer only until it is released
  esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(s) * 1000000ULL);
  esp_deep_sleep_start();
}

// ---- pages and API ------------------------------------------------------------------------------------------------

// The token out of whatever was pasted (BotFather's whole message, line breaks): digits, ':', 35 URL-safe characters.
static std::string pick_token(const std::string &text) {
  auto safe = [](char ch) { return isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '-'; };
  for (size_t i = 0; i < text.size(); i++) {
    if (text[i] != ':')
      continue;
    size_t a = i;
    while (a > 0 && isdigit(static_cast<unsigned char>(text[a - 1])))
      a--;
    size_t b = i + 1;
    while (b < text.size() && safe(text[b]))
      b++;
    if (i - a >= 6 && b - i - 1 >= 35)
      return text.substr(a, i - a + 36);  // the secret part is 35 characters; anything glued after it is not
  }
  return "";
}

static bool starts(const StringRef &url, const char *prefix) {
  size_t n = strlen(prefix);
  return url.size() >= n && strncmp(url.c_str(), prefix, n) == 0;
}

bool NoqSos::canHandle(AsyncWebServerRequest *request) const {
  char buf[AsyncWebServerRequest::URL_BUF_SIZE];
  StringRef url = request->url_to(buf);
  if (request->method() == HTTP_GET) {
    if (url == "/icon.png" || url == "/manifest.webmanifest")
      return true;
    if (setup_open_())
      return !(url == "/config.json" || url == "/wifisave");
    return url == "/" || url == "/index.html" || url == "/sos/state";
  }
  return request->method() == HTTP_POST && !setup_open_() && starts(url, "/sos/");
}

static std::string manifest_json() {
  return "{\"name\":\"NOQ SOS\",\"short_name\":\"NOQ SOS\",\"start_url\":\"/\",\"scope\":\"/\",\"display\":\"standalone\","
         "\"background_color\":\"#f5f5f5\",\"theme_color\":\"#f5f5f5\","
         "\"icons\":[{\"src\":\"/icon.png\",\"sizes\":\"192x192\",\"type\":\"image/png\",\"purpose\":\"any\"}]}";
}

static const char *job_name(Job j) {
  switch (j) {
    case Job::ALERT:
      return "alert";
    case Job::CHECKIN:
      return "checkin";
    case Job::LOW_BATTERY:
      return "low";
    case Job::TEST:
      return "test";
    case Job::TOKEN:
      return "token";
    case Job::POLL:
      return "poll";
    default:
      return "";
  }
}

std::string NoqSos::state_json_() {
  xSemaphoreTake(this->lock_, portMAX_DELAY);
  Settings c = this->cfg_;
  bool busy = this->job_busy_;
  Job job = busy ? this->job_ : this->last_job_;
  bool ok = this->last_ok_;
  std::string msg = this->last_msg_;
  xSemaphoreGive(this->lock_);

  char b[160];
  std::string o = "{\"v\":\"";
#ifdef ESPHOME_PROJECT_VERSION
  o += ESPHOME_PROJECT_VERSION;
#endif
  o += "\",\"host\":\"" + jesc(App.get_name().c_str()) + "\",\"name\":\"" + jesc(c.name) + "\",\"lang\":\"" +
       (c.lang ? "en" : "tr") + "\"";
  snprintf(b, sizeof(b), ",\"hour\":%d,\"tz\":%d,\"poll\":%u,\"token\":%s,\"bot\":\"", c.hour, c.tz_min,
           static_cast<unsigned>(c.poll_min), c.token[0] ? "true" : "false");
  o += b + jesc(c.bot) + "\",\"people\":[";
  for (uint8_t i = 0; i < c.count; i++) {
    snprintf(b, sizeof(b), "%s{\"id\":\"%lld\",\"label\":\"", i ? "," : "", static_cast<long long>(c.people[i].id));
    o += b + jesc(c.people[i].label) + "\"}";
  }
  o += "],\"job\":{\"kind\":\"" + std::string(job_name(job)) + "\",\"state\":\"" +
       (busy ? "busy" : job == Job::NONE ? "idle" : ok ? "ok" : "failed") + "\",\"msg\":\"" + jesc(msg.c_str()) + "\"}";
  if (this->battery_mv_ > 0) {
    const BatLog &l = this->bat_;
    snprintf(b, sizeof(b), ",\"battery\":{\"mv\":%d,\"pct\":%d,\"charging\":%s,\"full\":%s,\"charges\":%u", this->battery_mv_,
             this->battery_pct_, this->battery_charging_ ? "true" : "false",
             this->battery_charging_ && this->battery_mv_ >= 4170 ? "true" : "false", static_cast<unsigned>(l.charges));
    o += b;
    if (l.charged_at != 0) {
      snprintf(b, sizeof(b), ",\"charged_at\":%lld,\"charged_full\":%s,\"charged_seen\":%s", static_cast<long long>(l.charged_at),
               l.full ? "true" : "false", l.seen ? "true" : "false");
      o += b;
    } else {
      o += ",\"charged_at\":null";
    }
    float left = this->battery_charging_ ? -1.0f : this->battery_days_left_();
    if (left >= 0.0f) {
      snprintf(b, sizeof(b), ",\"days_left\":%.1f", left);
      o += b;
    } else {
      o += ",\"days_left\":null";
    }
    o += ",\"past\":[";
    for (uint8_t i = 0; i < BAT_PAST && l.past_days[i] > 0.0f; i++) {
      snprintf(b, sizeof(b), "%s{\"days\":%.1f,\"end\":%u}", i ? "," : "", l.past_days[i], static_cast<unsigned>(l.past_end[i]));
      o += b;
    }
    o += "]}";
  } else if (this->battery_mv_ == -2) {
    snprintf(b, sizeof(b), ",\"battery\":{\"fault\":true,\"mv\":%d}", this->battery_fault_mv_);
    o += b;
  } else {
    o += ",\"battery\":null";
  }
  if (this->kesif_until_ != 0 || this->kesif_n_ > 0) {
    uint32_t left = this->kesif_until_ != 0 ? (this->kesif_until_ - millis()) / 1000 : 0;
    snprintf(b, sizeof(b), ",\"kesif\":{\"left\":%u,\"points\":[", static_cast<unsigned>(left));
    o += b;
    for (uint8_t i = 0; i < this->kesif_n_; i++) {
      snprintf(b, sizeof(b), "%s{\"t\":%lld,\"rssi\":%d}", i ? "," : "", static_cast<long long>(this->kesif_pts_[i].utc),
               this->kesif_pts_[i].rssi);
      o += b;
    }
    o += "]}";
  } else {
    o += ",\"kesif\":null";
  }
  auto *wifi = wifi::global_wifi_component;
  if (wifi->is_connected()) {
    char ssid[wifi::SSID_BUFFER_SIZE];
    o += ",\"wifi\":{\"ssid\":\"" + jesc(wifi->wifi_ssid_to(ssid)) + "\"";
    snprintf(b, sizeof(b), ",\"rssi\":%d}", wifi->wifi_rssi());
    o += b;
  } else {
    o += ",\"wifi\":null";
  }
  int32_t left = static_cast<int32_t>(this->awake_until_ - millis());
  int64_t t = now_utc();
  snprintf(b, sizeof(b), ",\"awake\":%ld,\"usb\":%s,\"pending\":%u", static_cast<long>(left > 0 ? left / 1000 : 0),
           usb_serial_jtag_is_connected() ? "true" : "false", rtc_state.pending);
  o += b;
  o += ",\"clock\":" + (t ? "\"" + hhmm(t, c.tz_min) + "\"" : std::string("null"));
  snprintf(b, sizeof(b), ",\"timing\":{\"wifi\":%u,\"sent\":%u}", static_cast<unsigned>(rtc_state.last_wifi_ms),
           static_cast<unsigned>(rtc_state.last_sent_ms));
  o += b;
  o += ",\"last\":" + (rtc_state.last_alert ? "{\"at\":\"" + hhmm(rtc_state.last_alert, c.tz_min) + "\",\"ok\":" +
                                                   (rtc_state.last_ok ? "true" : "false") + "}"
                                             : std::string("null"));
  o += "}";
  return o;
}

void NoqSos::send_state_(AsyncWebServerRequest *request, int code) {
  std::string json = this->state_json_();
  auto *r = request->beginResponse(code, "application/json", json);
  r->addHeader("Cache-Control", "no-store");
  request->send(r);
}

void NoqSos::handleRequest(AsyncWebServerRequest *request) {
  char buf[AsyncWebServerRequest::URL_BUF_SIZE];
  StringRef url = request->url_to(buf);
  if (url == "/icon.png") {
    auto *icon = request->beginResponse(200, "image/png", ICON_PNG, sizeof(ICON_PNG));
    icon->addHeader("Cache-Control", "max-age=604800");
    request->send(icon);
    return;
  }
  if (url == "/manifest.webmanifest") {
    std::string json = manifest_json();
    request->send(200, "application/manifest+json", json.c_str());
    return;
  }
  // somebody is setting the pendant up: do not fall asleep under them
  this->keep_awake_(5 * 60 * 1000);
  this->sleep_now_ = false;

  if (request->method() == HTTP_GET && !starts(url, "/sos/")) {
    bool setup = setup_open_();
    auto *r = request->beginResponse(200, "text/html", setup ? PAGE_GZ : PANEL_GZ, setup ? sizeof(PAGE_GZ) : sizeof(PANEL_GZ));
    r->addHeader("Content-Encoding", "gzip");
    r->addHeader("Cache-Control", "no-store");
    request->send(r);
    return;
  }
  if (url == "/sos/state") {
    this->send_state_(request);
    return;
  }

  if (url == "/sos/settings") {
    xSemaphoreTake(this->lock_, portMAX_DELAY);
    if (request->hasArg("name"))
      copy_utf8(this->cfg_.name, sizeof(this->cfg_.name), request->arg("name"));
    if (request->hasArg("lang"))
      this->cfg_.lang = request->arg("lang") == "en" ? 1 : 0;
    if (request->hasArg("hour")) {
      int h = atoi(request->arg("hour").c_str());
      this->cfg_.hour = static_cast<int8_t>(h >= 0 && h <= 23 ? h : -1);
      rtc_state.next_check = 0;
    }
    if (request->hasArg("tz")) {
      int tz = atoi(request->arg("tz").c_str());
      if (tz >= -720 && tz <= 840) {
        this->cfg_.tz_min = static_cast<int16_t>(tz);
        rtc_state.next_check = 0;
      }
    }
    if (request->hasArg("poll")) {
      int p = atoi(request->arg("poll").c_str());
      if (p == 0 || p == 15 || p == 30 || p == 60 || p == 180)
        this->cfg_.poll_min = static_cast<uint16_t>(p);
    }
    this->save_settings_();
    xSemaphoreGive(this->lock_);
    this->send_state_(request);
    return;
  }
  if (url == "/sos/remove") {
    long long id = atoll(request->arg("id").c_str());
    xSemaphoreTake(this->lock_, portMAX_DELAY);
    uint8_t k = 0;
    for (uint8_t i = 0; i < this->cfg_.count; i++)
      if (this->cfg_.people[i].id != id)
        this->cfg_.people[k++] = this->cfg_.people[i];
    this->cfg_.count = k;
    this->save_settings_();
    xSemaphoreGive(this->lock_);
    this->send_state_(request);
    return;
  }
  if (url == "/sos/token") {
    std::string raw = request->arg("value");
    std::string token = pick_token(raw);
    if (token.empty() && !raw.empty()) {
      request->send(400, "application/json", "{\"error\":\"token\"}");
      return;
    }
    if (token.empty()) {
      xSemaphoreTake(this->lock_, portMAX_DELAY);
      this->cfg_.token[0] = 0;
      this->cfg_.bot[0] = 0;
      this->cfg_.count = 0;
      this->save_settings_();
      xSemaphoreGive(this->lock_);
      this->send_state_(request);
      return;
    }
    if (token.size() >= sizeof(this->cfg_.token) || token.find(':') == std::string::npos) {
      request->send(400, "application/json", "{\"error\":\"token\"}");
      return;
    }
    this->send_state_(request, this->start_job_(Job::TOKEN, token) ? 202 : 409);
    return;
  }
  if (url == "/sos/kesif") {
    if (request->arg("on") == "1") {
      this->kesif_until_ = millis() + 20 * 60 * 1000;
      if (this->kesif_until_ == 0)
        this->kesif_until_ = 1;
      this->kesif_n_ = 0;
      this->keep_awake_(20 * 60 * 1000);
    } else {
      this->kesif_until_ = 0;
    }
    this->send_state_(request);
    return;
  }
  if (url == "/sos/test") {
    this->send_state_(request, this->start_job_(Job::TEST) ? 202 : 409);
    return;
  }
  if (url == "/sos/sleep") {
    this->sleep_now_ = true;
    this->awake_until_ = millis();
    this->send_state_(request);
    return;
  }
  if (url == "/sos/wifi-reset") {
    this->forget_wifi_ = true;
    this->reboot_at_ = millis() + 800;
    this->send_state_(request);
    return;
  }
  request->send(404, "application/json", "{\"error\":\"not_found\"}");
}

}  // namespace esphome::noq_sos
