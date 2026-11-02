"""FRITZ2MQTT AHA login/discovery adapter, tested against ESPHome 2026.6.4."""
import voluptuous as vol
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, sensor, text_sensor, esp32, socket
from esphome.components.http_request.update import HttpRequestUpdate
from esphome.const import CONF_ID, CONF_PASSWORD, CONF_USERNAME

DEPENDENCIES = ["esp32", "wifi", "web_server"]
AUTO_LOAD = ["binary_sensor", "sensor", "text_sensor", "json", "web_server_base"]
ns = cg.esphome_ns.namespace("fritzbox_aha")
FritzboxAha = ns.class_("FritzboxAha", cg.PollingComponent)

CONFIG_SCHEMA = cv.Schema({
    cv.Optional("storage_namespace", default="fritz2mqtt"): cv.All(cv.string_strict, vol.Match(r"^[A-Za-z0-9_]{1,15}$")),
    cv.Optional("firmware_update"): cv.use_id(HttpRequestUpdate),
    cv.GenerateID(): cv.declare_id(FritzboxAha),
    cv.Optional("host", default="0.0.0.0"): cv.ipv4address,
    cv.Optional(CONF_USERNAME, default=""): cv.string_strict,
    cv.Optional(CONF_PASSWORD, default=""): cv.string_strict,
    cv.Optional("thermostat_ain", default=""): cv.All(cv.string_strict, cv.Length(max=256)),
    cv.Optional("allow_temperature_control", default=False): cv.boolean,
    cv.Optional("thermostat_command_status"): text_sensor.text_sensor_schema(),
    cv.Optional("diagnostic"): text_sensor.text_sensor_schema(),
    cv.Optional("authentication_method"): text_sensor.text_sensor_schema(),
    cv.Optional("thermostat_name"): text_sensor.text_sensor_schema(),
    cv.Optional("thermostat_identifier"): text_sensor.text_sensor_schema(),
    cv.Optional("thermostat_model"): text_sensor.text_sensor_schema(),
    cv.Optional("thermostat_status"): text_sensor.text_sensor_schema(),
    cv.Optional("thermostat_mode"): text_sensor.text_sensor_schema(),
    cv.Optional("thermostat_battery_status"): text_sensor.text_sensor_schema(),
    cv.Optional("device_count"): sensor.sensor_schema(accuracy_decimals=0),
    cv.Optional("thermostat_temperature"): sensor.sensor_schema(unit_of_measurement="°C", device_class="temperature", accuracy_decimals=1),
    cv.Optional("thermostat_target_temperature"): sensor.sensor_schema(unit_of_measurement="°C", device_class="temperature", accuracy_decimals=1),
    cv.Optional("thermostat_battery"): sensor.sensor_schema(unit_of_measurement="%", device_class="battery", accuracy_decimals=0),
    cv.Optional("connected"): binary_sensor.binary_sensor_schema(),
    cv.Optional("authenticated"): binary_sensor.binary_sensor_schema(),
}).extend(cv.polling_component_schema("60s"))

CONFIG_SCHEMA = cv.All(
    CONFIG_SCHEMA,
    socket.consume_sockets(2, "fritzbox_aha"),  # AHA HTTP + native MQTT
    socket.consume_sockets(1, "fritzbox_aha_ntp", socket.SocketType.UDP),
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    if "firmware_update" in config:
        cg.add_define("USE_FRITZ2MQTT_UPDATE")
        updater = await cg.get_variable(config["firmware_update"])
        cg.add(var.set_firmware_update(updater))
    cg.add(var.set_storage_namespace(config["storage_namespace"]))
    cg.add(var.set_host(str(config["host"])))
    cg.add(var.set_username(config[CONF_USERNAME]))
    cg.add(var.set_password(config[CONF_PASSWORD]))
    cg.add(var.set_thermostat_ain(config["thermostat_ain"]))
    cg.add(var.set_allow_temperature_control(config["allow_temperature_control"]))
    esp32.include_builtin_idf_component("esp_http_client")
    esp32.include_builtin_idf_component("mqtt")
    esp32.add_idf_sdkconfig_option("CONFIG_MBEDTLS_CERTIFICATE_BUNDLE", True)
    esp32.add_idf_sdkconfig_option("CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_FULL", True)
    esp32.add_idf_sdkconfig_option("CONFIG_MQTT_OUTBOX_EXPIRED_TIMEOUT_MS", 10000)
    esp32.add_idf_component(name="espressif/expat", ref="2.9.0")
    for key in ("thermostat_command_status", "diagnostic", "authentication_method", "thermostat_name", "thermostat_identifier", "thermostat_model", "thermostat_status", "thermostat_mode", "thermostat_battery_status"):
        if key in config:
            entity = await text_sensor.new_text_sensor(config[key])
            cg.add(getattr(var, "set_" + key)(entity))
    for key in ("device_count", "thermostat_temperature", "thermostat_target_temperature", "thermostat_battery"):
        if key in config:
            entity = await sensor.new_sensor(config[key])
            cg.add(getattr(var, "set_" + key)(entity))
    for key in ("connected", "authenticated"):
        if key in config:
            entity = await binary_sensor.new_binary_sensor(config[key])
            cg.add(getattr(var, "set_" + key)(entity))
