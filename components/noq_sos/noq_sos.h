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
};

enum class Job : uint8_t { NONE, ALERT, CHECKIN, LOW_BATTERY, TEST, TOKEN };
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
  int read_battery_mv_();

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
  void poll_updates_();

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
  uint32_t battery_read_{0};
  void *adc_{nullptr};
  void *cali_{nullptr};
  int adc_channel_{-1};

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
  // while setup is open the worker listens for people who start the bot and adds them
  volatile bool listen_{false};
  int64_t update_offset_{0};

  // per boot
  uint8_t alert_attempts_{0};
  uint32_t next_attempt_{0};
  bool checkin_done_{false};
  bool low_done_{false};
};

}  // namespace esphome::noq_sos
