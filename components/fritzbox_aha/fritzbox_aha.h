#pragma once

#include <string>
#include <cmath>
#include <memory>
#include <atomic>
#include "mqtt_client.h"
#include "aha_protocol.h"
#include "runtime_config.h"
#include "http_request_body.h"
#include "esphome/components/web_server_base/web_server_base.h"
#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

namespace esphome::fritzbox_aha {

class FritzboxAha : public PollingComponent, public AsyncWebHandler {
 public:
  void set_storage_namespace(const std::string &v) { storage_namespace_ = v; }
  void set_host(const std::string &v) { host_ = v; }
  void set_username(const std::string &v) { username_ = v; }
  void set_password(const std::string &v) { password_ = v; }
  void set_diagnostic(text_sensor::TextSensor *v) { diagnostic_ = v; }
  void set_authentication_method(text_sensor::TextSensor *v) { method_sensor_ = v; }
  void set_device_count(sensor::Sensor *v) { count_sensor_ = v; }
  void set_connected(binary_sensor::BinarySensor *v) { connected_sensor_ = v; }
  void set_authenticated(binary_sensor::BinarySensor *v) { authenticated_sensor_ = v; }
  void set_thermostat_ain(const std::string &v) { thermostat_ain_ = v; }
  void set_thermostat_name(text_sensor::TextSensor *v) { thermostat_name_ = v; }
  void set_thermostat_identifier(text_sensor::TextSensor *v) { thermostat_identifier_ = v; }
  void set_thermostat_model(text_sensor::TextSensor *v) { thermostat_model_ = v; }
  void set_thermostat_status(text_sensor::TextSensor *v) { thermostat_status_ = v; }
  void set_thermostat_mode(text_sensor::TextSensor *v) { thermostat_mode_ = v; }
  void set_thermostat_battery_status(text_sensor::TextSensor *v) { thermostat_battery_status_ = v; }
  void set_thermostat_temperature(sensor::Sensor *v) { thermostat_temperature_ = v; }
  void set_thermostat_target_temperature(sensor::Sensor *v) { thermostat_target_temperature_ = v; }
  void set_thermostat_battery(sensor::Sensor *v) { thermostat_battery_ = v; }
  void set_allow_temperature_control(bool v) { allow_control_ = v; }
  void set_thermostat_command_status(text_sensor::TextSensor *v) { command_status_ = v; }
  void request_target_temperature(float value);
  // Read only; call from the ESPHome main loop (e.g. an interval lambda).
  bool status_led_ready();
  bool canHandle(AsyncWebServerRequest *request) const override;
  void handleRequest(AsyncWebServerRequest *request) override;
  void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) override;
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::WIFI + 1.0f; }

 private:
  // The IDF HTTP server dispatches these callbacks serially on its HTTP task.
  gateway::HttpRequestBody request_body_;
  AsyncWebServerRequest *body_request_ = nullptr;
  struct Snapshot {
    char diagnostic[128] = "Start";
    char method[12] = "Unbekannt";
    bool connected = false;
    bool authenticated = false;
    int device_count = -1;
    char name[257] = "", ain[257] = "", product[257] = "";
    char thermostat_status[128] = "Keine aktuellen Daten";
    char mode[32] = "Unbekannt", battery_status[32] = "Unbekannt";
    float temperature = NAN, target = NAN, battery = NAN;
  };
  struct Command { char ain[257] = ""; float target = NAN; uint32_t created_at = 0; uint32_t revision = 0; };
  struct CommandResult { char message[160] = ""; };
  bool allow_control_ = false;
  std::atomic<bool> command_busy_{false};
  SemaphoreHandle_t web_mutex_ = nullptr;
  std::shared_ptr<const gateway::Config> runtime_config_;
  uint32_t applied_revision_ = 0;
  Snapshot web_last_;
  uint32_t web_observation_at_ = 0, web_revision_ = 0;
  std::vector<protocol::Thermostat> discovered_, web_devices_;
  std::string web_command_ain_, web_command_message_ = "Bereit", active_command_ain_, csrf_token_;
  std::atomic<bool> wifi_apply_requested_{false};
  void apply_wifi_();
  void wifi_loop_();
  struct MqttInput {
    char topic[161] = "", payload[33] = "";
    uint32_t revision = 0, created_at = 0;
    bool retained = false, duplicate = false;
  };
  esp_mqtt_client_handle_t mqtt_client_ = nullptr;
  QueueHandle_t mqtt_inputs_ = nullptr;
  std::shared_ptr<const gateway::Config> mqtt_config_;
  std::string mqtt_client_id_, mqtt_status_text_ = "Start", mqtt_last_input_ = "Noch keine Nachricht";
  std::atomic<bool> mqtt_connected_{false}, mqtt_test_requested_{false};
  std::atomic<uint32_t> mqtt_generation_{0}, mqtt_active_revision_{0}, mqtt_rx_{0}, mqtt_ignored_{0},
      mqtt_rejected_{0}, mqtt_accepted_{0}, mqtt_acks_{0}, mqtt_subacks_{0};
  uint32_t mqtt_seen_generation_ = 0, mqtt_last_observation_ = 0, mqtt_next_check_ = 0;
  unsigned mqtt_subscriptions_ = 0;
  void mqtt_loop_();
  void mqtt_publish_state_();
  void mqtt_message_(const std::string &message);
  bool mqtt_test_();
  static void mqtt_event_(void *arg, esp_event_base_t base, int32_t id, void *data);
  void init_web_();
  void apply_runtime_config_();
  std::shared_ptr<const gateway::Config> config_copy_();
  bool queue_temperature_(const std::string &ain, float target, std::string &error, uint32_t expected_revision = UINT32_MAX);
  std::string status_json_();
  std::string config_json_(const gateway::Config &config, bool secrets);
  bool decode_config_(const std::string &body, const gateway::Config &previous, gateway::Config &out, std::string &error);
  bool save_config_(const gateway::Config &config);
  void serve_json_(AsyncWebServerRequest *request, int code, const std::string &body);
  uint32_t observation_at_ = 0;
  QueueHandle_t commands_ = nullptr, command_results_ = nullptr;
  text_sensor::TextSensor *command_status_ = nullptr;
  void execute_command_(const Command &command);
  void command_result_(const std::string &message);
  struct HttpResult {
    bool complete = false;
    int status = 0;
    int error = 0;
    const char *stage = "init";
    std::string body;
  };
  std::string storage_namespace_ = "fritz2mqtt";
  std::string host_, username_, password_, thermostat_ain_;
  Snapshot thermostat_;
  text_sensor::TextSensor *thermostat_name_ = nullptr;
  text_sensor::TextSensor *thermostat_identifier_ = nullptr;
  text_sensor::TextSensor *thermostat_model_ = nullptr;
  text_sensor::TextSensor *thermostat_status_ = nullptr;
  text_sensor::TextSensor *thermostat_mode_ = nullptr;
  text_sensor::TextSensor *thermostat_battery_status_ = nullptr;
  sensor::Sensor *thermostat_temperature_ = nullptr;
  sensor::Sensor *thermostat_target_temperature_ = nullptr;
  sensor::Sensor *thermostat_battery_ = nullptr;

  // Session material belongs exclusively to the worker; never exposed as an entity.
  std::string sid_, method_ = "Unbekannt";
  uint32_t retry_at_ = 0;
  uint8_t failures_ = 0;
  QueueHandle_t results_ = nullptr;
  SemaphoreHandle_t requests_ = nullptr;
  TaskHandle_t worker_ = nullptr;
  text_sensor::TextSensor *diagnostic_ = nullptr, *method_sensor_ = nullptr;
  sensor::Sensor *count_sensor_ = nullptr;
  binary_sensor::BinarySensor *connected_sensor_ = nullptr, *authenticated_sensor_ = nullptr;
  Snapshot last_;
  bool have_last_ = false;

  static void worker_entry_(void *arg);
  void worker_run_();
  void cycle_();
  bool login_();
  bool discover_devices_();
  bool refresh_session_();
  HttpResult http_(const std::string &path, const std::string &post = "");
  void report_(const std::string &message, bool connected, bool authenticated, int count = -1);
  void backoff_(uint32_t minimum_seconds = 0);
  static void crypto_yield_() { vTaskDelay(1); }
};

}  // namespace esphome::fritzbox_aha
