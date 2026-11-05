"""NOQ SOS for ESPHome: a call pendant with one button.

The board sleeps. A press wakes it, it joins the home Wi-Fi and sends a Telegram message straight to the people chosen on
the device's own page. The bot belongs to the family; nothing of ours sits in between.

Pages, served by the device itself:
  setup network open   the NOQopen Wi-Fi setup page (the stock captive portal keeps /config.json and /wifisave)
  home network         the NOQ SOS page at / and its small JSON API under /sos/
"""
import esphome.codegen as cg
from esphome.components.esp32 import include_builtin_idf_component, require_certificate_bundle
from esphome.components import web_server_base
from esphome.components.web_server_base import CONF_WEB_SERVER_BASE_ID
import esphome.config_validation as cv
from esphome.const import CONF_ID

DEPENDENCIES = ["esp32", "wifi", "captive_portal"]
AUTO_LOAD = ["web_server_base", "json"]

CONF_BUTTON_PIN = "button_pin"
CONF_LED_PIN = "led_pin"
CONF_LED_INVERTED = "led_inverted"
CONF_BUZZER_PIN = "buzzer_pin"
CONF_BATTERY_PIN = "battery_pin"
CONF_BATTERY_RATIO = "battery_ratio"
CONF_SETUP_WINDOW = "setup_window"

noq_sos_ns = cg.esphome_ns.namespace("noq_sos")
NoqSos = noq_sos_ns.class_("NoqSos", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(NoqSos),
        cv.GenerateID(CONF_WEB_SERVER_BASE_ID): cv.use_id(web_server_base.WebServerBase),
        # wakes the board from deep sleep, so it must be an RTC GPIO (0-21 on the ESP32-S3); button to GND
        cv.Required(CONF_BUTTON_PIN): cv.int_range(min=0, max=21),
        cv.Optional(CONF_LED_PIN, default=-1): cv.int_range(min=-1, max=48),
        cv.Optional(CONF_LED_INVERTED, default=True): cv.boolean,
        # active buzzer or a vibration motor driver, high = on; -1 = none
        cv.Optional(CONF_BUZZER_PIN, default=-1): cv.int_range(min=-1, max=48),
        # ADC1 pin (GPIO1-10) reading the cell through a divider; -1 = no battery reading
        cv.Optional(CONF_BATTERY_PIN, default=-1): cv.int_range(min=-1, max=10),
        cv.Optional(CONF_BATTERY_RATIO, default=2.0): cv.float_range(min=1.0, max=10.0),
        # how long the device stays awake for setup after power-on, a reset or a firmware install
        cv.Optional(CONF_SETUP_WINDOW, default="10min"): cv.positive_time_period_milliseconds,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    base = await cg.get_variable(config[CONF_WEB_SERVER_BASE_ID])
    var = cg.new_Pvariable(config[CONF_ID], base)
    await cg.register_component(var, config)
    cg.add(var.set_button_pin(config[CONF_BUTTON_PIN]))
    cg.add(var.set_led_pin(config[CONF_LED_PIN], config[CONF_LED_INVERTED]))
    cg.add(var.set_buzzer_pin(config[CONF_BUZZER_PIN]))
    cg.add(var.set_battery(config[CONF_BATTERY_PIN], config[CONF_BATTERY_RATIO]))
    cg.add(var.set_setup_window(config[CONF_SETUP_WINDOW]))
    # ESPHome leaves most IDF components out of the build; the pendant needs these
    for name in ("esp_adc", "esp_http_client", "esp-tls", "esp_driver_usb_serial_jtag", "nvs_flash"):
        include_builtin_idf_component(name)
    require_certificate_bundle()
