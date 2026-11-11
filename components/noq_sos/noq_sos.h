#pragma once

#include <cstdint>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esphome/core/component.h"
#include "esphome/components/web_server_base/web_server_base.h"

namespace esphome::noq_sos {

static constexpr uint8_t MAX_PEOPLE = 6;

struct Person {
  int64_t id;      // Telegram chat id (a person or a group)
  char label[33];  // shown on the page only
};

// Kept in NVS under its own namespace, so forgetting the Wi-Fi network leaves it in place.
struct Settings {
  uint32_t magic;
  char name[49];    // who wears the pendant, used in the messages
  char token[80];   // Telegram bot token
  char bot[41];     // bot username, from getMe
  uint8_t lang;     // message language: 0 Turkish, 1 English
  int8_t hour;      // daily check-in hour (local), -1 = off
  int16_t tz_min;   // local time = UTC + tz_min
  uint8_t count;
  Person people[MAX_PEOPLE];
  // 0.2.0: settings saved by 0.1.x end here and are read with the defaults below
  uint16_t poll_min;  // minutes between Telegram command checks while asleep, 0 = only on presses and check-ins
  uint8_t menu;       // version of the command menu registered with Telegram (setMyCommands)
};

// Battery log (0.3.0), kept in NVS: the last charge, rest readings since then (at most one per 6 hours, thinned when
// full) and how long the previous charges lasted. Everything the estimate needs stays on the pendant.
static constexpr uint8_t BAT_POINTS = 12;
static constexpr uint8_t BAT_PAST = 3;
struct BatPoint {
  int64_t t;  // UTC
  int32_t mv;
};
struct BatLog {
  uint32_t magic;
  uint16_t charges;    // charges seen since the log started
  uint8_t full;        // the last charge reached full
  uint8_t seen;        // the pendant saw it end; 0 = found on a wake-up (the time is that wake-up)
  int64_t charged_at;  // UTC of the last charge, 0 = none yet
  int32_t last_mv;     // last rest reading kept
  uint8_t n;           // points since the last charge
  BatPoint pts[BAT_POINTS];
  float past_days[BAT_PAST];   // how long the previous charges lasted, newest first (0 = none)
  uint8_t past_end[BAT_PAST];  // percent left when the next charge came
};

enum class Job : uint8_t { NONE, ALERT, CHECKIN, LOW_BATTERY, TEST, TOKEN, POLL };
enum class Mode : uint8_t { AWAKE, BUTTON, TIMER };

class NoqSos : public Component, public AsyncWebHandler {
 public:
  explicit NoqSos(web_server_base::WebServerBase *base) : base_(base) {}
  void set_button_pin(int pin) { this->button_pin_ = pin; }
  void set_led_pin(int pin, bool inverted) {
    this->led_pin_ = pin;
    this->led_inverted_ = inverted;
  }
  void set_buzzer_pin(int pin) { this->buzzer_pin_ = pin; }
  void set_battery(int pin, float ratio) {
    this->battery_pin_ = pin;
    this->battery_ratio_ = ratio;
  }
  void set_setup_window(uint32_t ms) { this->setup_window_ = ms; }

  void setup() override;
  void loop() override;
  void dump_config() override;
  // Before Wi-Fi and the captive portal, so the page handler is first in line.
  float get_setup_priority() const override { return setup_priority::WIFI + 2.0f; }

  bool canHandle(AsyncWebServerRequest *request) const override;
  void handleRequest(AsyncWebServerRequest *request) override;

  void worker();

 protected:
  static bool setup_open_();
  bool configured_() const { return this->cfg_.token[0] != 0 && this->cfg_.count > 0; }
  void load_settings_();
  void save_settings_();

  void out_(bool on);
  void play_(const uint16_t *steps, uint8_t len);
  void fx_tick_(uint32_t now);
  void button_tick_(uint32_t now);
  void battery_tick_(uint32_t now);
  void battery_load_();
  void battery_save_();
  void battery_wake_(int mv);
  void battery_point_(int mv);
  bool battery_charged_(bool full, bool seen);
  float battery_days_left_() const;
  std::string battery_text_(const Settings &c, uint8_t lang);
  int read_battery_mv_();
  void kesif_press_(uint32_t now);

  bool start_job_(Job job, const std::string &arg = "");
  void finish_job_();
  bool checkin_due_(uint32_t now);
  uint32_t sleep_seconds_();
  void go_sleep_();
  void keep_awake_(uint32_t ms);

  std::string state_json_();
  void send_state_(AsyncWebServerRequest *request, int code = 200);

  bool run_alert_(const Settings &c, std::string &msg);
  bool run_broadcast_(const Settings &c, Job job, std::string &msg);
  bool run_token_(const std::string &token, std::string &msg);
  // setup: also adds whoever starts the bot; timeout: Telegram long poll in seconds (0 = answer what is waiting)
  void poll_updates_(bool setup, int timeout);
  std::string command_text_(const Settings &c, uint8_t cmd, uint8_t lang);
  void save_offset_(int64_t offset);

  web_server_base::WebServerBase *base_;
  int button_pin_{-1};
  int led_pin_{-1};
  bool led_inverted_{true};
  int buzzer_pin_{-1};
  int battery_pin_{-1};
  float battery_ratio_{2.0f};
  uint32_t setup_window_{600000};

  Settings cfg_{};
  Mode mode_{Mode::AWAKE};
  uint32_t boot_ms_{0};
  volatile uint32_t awake_until_{0};
  uint32_t online_since_{0};
  uint32_t wifi_ms_{0};
  bool alert_from_wake_{true};
  bool first_loop_{true};
  uint32_t reboot_at_{0};
  bool forget_wifi_{false};
  volatile bool sleep_now_{false};

  // button (polled while awake)
  bool btn_down_{false};
  bool btn_armed_{false};
  uint32_t btn_edge_{0};
  uint32_t btn_since_{0};
  uint32_t last_press_{0};

  // battery
  int battery_mv_{-1};
  int battery_pct_{-1};
  int8_t battery_rise_{0};
  bool battery_charging_{false};
  int battery_rest_mv_{-1};  // read in setup(), before Wi-Fi
  int battery_fault_mv_{0};  // last impossible reading (divider fault), shown instead of a percentage
  bool battery_point_done_{false};
  bool battery_charge_logged_{false};
  uint32_t battery_charge_since_{0};
  BatLog bat_{};
  uint32_t battery_read_{0};
  void *adc_{nullptr};
  void *cali_{nullptr};
  int adc_channel_{-1};

  // discovery mode (0.3.0): walk the house and press in each spot; the light tells the Wi-Fi signal, no alert goes out.
  // Opened from the page only, ends by itself after 20 minutes (or on sleep), so the button is never left silenced.
  static constexpr uint8_t KESIF_MAX = 24;
  struct KesifPoint {
    int64_t utc;
    int8_t rssi;  // 0 = no network
  };
  uint32_t kesif_until_{0};
  uint8_t kesif_n_{0};
  KesifPoint kesif_pts_[KESIF_MAX]{};

  // feedback sequence: even steps on, odd steps off (ms)
  const uint16_t *fx_{nullptr};
  uint8_t fx_len_{0};
  uint8_t fx_i_{0};
  uint32_t fx_next_{0};

  // worker task: one job at a time, network calls block there instead of in the main loop
  TaskHandle_t task_{nullptr};
  SemaphoreHandle_t lock_{nullptr};
  volatile bool job_busy_{false};
  Job job_{Job::NONE};
  std::string job_arg_;
  Job last_job_{Job::NONE};
  bool last_ok_{false};
  bool last_seen_{true};
  std::string last_msg_;
  // while setup is open the worker listens for people who start the bot and adds them, and answers commands
  volatile bool listen_{false};

  // per boot
  uint8_t alert_attempts_{0};
  uint32_t next_attempt_{0};
  bool checkin_done_{false};
  bool low_done_{false};
  bool poll_done_{false};  // a wake from sleep reads waiting Telegram commands once
};

}  // namespace esphome::noq_sos
