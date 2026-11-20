"""Compile the production admission/worker/result/MQTT ingress methods against fake IO.

The scheduler and methods come from the checked-out sources. Only platform IO,
time, and HTTP responses are simulated. No MQTT broker or gateway is contacted.
"""
from pathlib import Path
import subprocess
import tempfile
import sys

root = Path(__file__).resolve().parents[1]
component = root / "components/fritzbox_aha"
cpp = (component / "fritzbox_aha.cpp").read_text()
web = (component / "gateway_web.cpp").read_text()


def method(source, first, following):
    return source[source.index(first):source.index(following, source.index(first))]


production = "\n".join([
    "void FritzboxAha::consume_result_() {\n" +
    method(cpp, "  CommandResult result;\n  if (command_results_", "  Snapshot next;") + "\n}\n",
    "void FritzboxAha::receive_(MqttEvent *e) {auto *self = this; switch(1) {\n" +
    method(web, "    case MQTT_EVENT_DATA: {", "    default: break;") + "\n}}\n",
    method(cpp, "void FritzboxAha::worker_run_()", "void FritzboxAha::request_target_temperature"),
    method(cpp, "void FritzboxAha::command_result_(", "void FritzboxAha::report_("),
    method(web, "bool FritzboxAha::queue_temperature_(", "bool FritzboxAha::status_led_ready"),
    method(web, "void FritzboxAha::mqtt_loop_()", "  uint32_t now = web_millis();") + "\n}\n",
])
preamble = r"""
#include "command_buffer.h"
#include "aha_protocol.h"
#include <cassert>
#include <atomic>
#include <cstdio>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>
using namespace esphome::fritzbox_aha;
struct Stop {};
uint32_t ticks = 100;
uint32_t worker_millis() { return ticks; }
uint32_t web_millis() { return ticks; }
constexpr uint32_t portMAX_DELAY = UINT32_MAX;
constexpr int pdTRUE = 1;
uint32_t pdMS_TO_TICKS(uint32_t n) { return n; }
struct Semaphore { bool mutex = false, signaled = false; };
using SemaphoreHandle_t = Semaphore*;
int xSemaphoreGive(SemaphoreHandle_t s) { if (!s->mutex) s->signaled = true; return pdTRUE; }
std::function<void()> on_wait;
int xSemaphoreTake(SemaphoreHandle_t s, uint32_t timeout) {
  if (s->mutex) return pdTRUE;
  if (s->signaled) { s->signaled = false; return pdTRUE; }
  if (timeout == portMAX_DELAY) throw Stop{};
  ticks += timeout;
  if (on_wait) on_wait();
  return 0;
}
struct Input {
  char topic[161] = "", payload[33] = "";
  uint32_t revision = 0, created_at = 0;
  bool retained = false, duplicate = false;
};
constexpr int MQTT_EVENT_DATA = 1;
struct MqttEvent {
  const char *topic, *data;
  int topic_len = 0, data_len = 0, total_data_len = 0, current_data_offset = 0;
  bool retain = false, dup = false;
};
struct ResultMessage { char message[160] = ""; };
struct Queue {
  std::deque<Input> inputs;
  std::deque<gateway::CommandBuffer::Command> commands;
  std::string result;
};
using QueueHandle_t = Queue*;
QueueHandle_t xQueueCreate(int, size_t) { assert(false); return nullptr; }
int xQueueReceive(QueueHandle_t q, Input *input, int) {
  if (q->inputs.empty()) return 0;
  *input = q->inputs.front(); q->inputs.pop_front(); return pdTRUE;
}
int xQueueSend(QueueHandle_t q, Input *i, int) {
  if(q->inputs.size()==32)return 0;
  q->inputs.push_back(*i);return pdTRUE;
}
int xQueueSend(QueueHandle_t q, gateway::CommandBuffer::Command *c, int) {
  if(!q->commands.empty())return 0;
  q->commands.push_back(*c);return pdTRUE;
}
void xQueueOverwrite(QueueHandle_t q, ResultMessage *r) { q->result = r->message; }
int xQueueReceive(QueueHandle_t q, ResultMessage *r, int) {
  if(q->result.empty())return 0;
  std::snprintf(r->message,sizeof(r->message),"%s",q->result.c_str());q->result.clear();return pdTRUE;
}
struct FritzboxAha {
  using Command = gateway::CommandBuffer::Command;
  using MqttInput = Input;
  using CommandResult = ResultMessage;
  struct HttpResult { bool complete; int status; std::string body; };
  struct Sensor { void publish_state(const char*) {} };
  gateway::CommandBuffer command_buffer_;
  Semaphore mutex{true}, request;
  SemaphoreHandle_t web_mutex_ = &mutex, requests_ = &request;
  std::shared_ptr<const gateway::Config> runtime_config_;
  std::atomic<bool> command_busy_{false}, network_ready_{true}, mqtt_connected_{true}, mqtt_test_requested_{false};
  struct Firmware { bool installing = false; } firmware_state_;
  int firmware_action_ = 0;
  Queue legacy_commands;
  QueueHandle_t commands_ = &legacy_commands;
  struct Observation { bool connected=true, authenticated=true; int device_count=8; } web_last_;
  uint32_t web_revision_=7, web_observation_at_=0;
  std::vector<protocol::Thermostat> web_devices_;
  Queue queue, results; QueueHandle_t command_results_ = &results, mqtt_inputs_ = &queue;
  Sensor *command_status_ = nullptr;
  std::string active_command_ain_, web_command_message_, web_command_ain_, mqtt_last_input_;
  std::string sid_ = "test";
  uint32_t retry_at_ = 0;
  std::atomic<uint32_t> mqtt_active_revision_{7};
  uint32_t mqtt_rx_ = 0;
  uint32_t mqtt_accepted_ = 0, mqtt_rejected_ = 0, mqtt_ignored_ = 0;
  std::vector<protocol::Thermostat> discovered_;
  std::vector<std::pair<std::string,float>> writes;
  std::function<void(FritzboxAha&)> on_discovery, on_write;
  uint32_t read_delay = 0, write_delay = 0;
  bool fail_write = false, fail_readback = false;
  bool login_ok = true; int preflight_failures = 0;
  bool wrote = false;
  std::shared_ptr<const gateway::Config> config_copy_() { return runtime_config_; }
  void apply_runtime_config_() {}
  void cycle_() {}
  bool mqtt_test_() { return true; }
  void mqtt_message_(const char*) {}
  bool login_() { if (login_ok) sid_ = "renewed"; return login_ok; }
  bool discover_devices_() {
    ticks += read_delay;
    if (on_discovery) on_discovery(*this);
    if (wrote && fail_readback) { wrote = false; return false; }
    if (preflight_failures-- > 0) { retry_at_ = ticks + 5000; return false; }
    return true;
  }
  HttpResult http_(const std::string &path) {
    assert(path.find("setswitch") != std::string::npos || path.find("sethkrtsoll") != std::string::npos);
    const auto command = command_buffer_.active();
    writes.emplace_back(command.ain, command.target);
    ticks += write_delay; wrote = true;
    if (on_write) on_write(*this);
    if (!fail_write) for (auto &d : discovered_) if (d.ain == command.ain) {
      d.tsoll = std::to_string(int(command.target * 2));
      d.switch_state = command.target == 1 ? "1" : "0";
    }
    return {!fail_write, fail_write ? 0 : 200, fail_write ? "" : "OK"};
  }
  bool queue_temperature_(const std::string&,float,std::string&,uint32_t=UINT32_MAX,bool=false,uint32_t=UINT32_MAX);
  void worker_run_();
  void consume_result_();
  void receive_(MqttEvent*);
  void execute_command_(const Command&);
  void command_result_(const std::string&,const char *state="finished");
  void mqtt_loop_();
  void run() { try { worker_run_(); } catch (const Stop&) {} }
  void input(int index, std::string value, bool retained = false) {
    const auto &d = runtime_config_->devices[index];
    Input i; std::snprintf(i.topic,sizeof(i.topic),"%s",d.command_topic.c_str());
    std::snprintf(i.payload,sizeof(i.payload),"%s",value.c_str());
    i.created_at = ticks; i.revision = runtime_config_->revision; i.retained = retained;
    queue.inputs.push_back(i); mqtt_loop_();
  }
};
FritzboxAha gateway_with(int count) {
  ticks = 100; on_wait = {};
  FritzboxAha f; auto c = std::make_shared<gateway::Config>(); c->revision = 7;
  for (int i = 0; i < count; ++i) {
    gateway::Device d; d.ain = std::to_string(i); d.enabled = true;
    d.command_topic = "test/" + d.ain; c->devices.push_back(d);
    protocol::Thermostat t; t.ain = d.ain; t.present = "1"; t.tsoll = "40"; f.discovered_.push_back(t);
  }
  f.runtime_config_ = c;
  return f;
}
"""
# Avoid moving the simulated platform object, which contains atomics.
preamble = preamble.replace("FritzboxAha gateway_with(int count) {", "std::unique_ptr<FritzboxAha> gateway_with(int count) {")
preamble = preamble.replace("FritzboxAha f; auto c", "auto result = std::make_unique<FritzboxAha>(); auto &f = *result; auto c")
preamble = preamble.replace("return f;", "return result;")

scenarios = r"""
int main() {
  for (int n : {3, 8}) {
    auto p = gateway_with(n); auto &f = *p;
    for (int repeat = 0; repeat < 12; ++repeat) {
      const size_t before = f.writes.size();
      for (int j = 0; j < n; ++j) f.input((j + repeat) % n, "16");
      // All submissions coalesce into one binary wakeup.
      assert(f.mqtt_accepted_ == uint32_t(n * (repeat + 1)));
      assert(f.command_busy_);
      f.run();
      assert(f.writes.size() == before + n && !f.command_busy_);
      for (int j = 0; j < n; ++j)
        assert(f.writes[before+j].first == std::to_string((j+repeat)%n));
    }
    for (int j = 0; j < n; ++j) assert(f.command_buffer_.diagnostic(std::to_string(j))->attempted == 12);
  }
  {
    auto p = gateway_with(8); auto &f = *p;
    for(int i=0;i<8;++i) f.input(i,"16");
    f.input(4,"18"); f.input(4,"21.5"); f.input(4,"9.9");
    assert(f.command_buffer_.pending()==8);
    f.run();
    assert(f.writes.size()==8 && f.writes[4].first=="4" && f.writes[4].second==21.5f);
    assert(f.command_buffer_.diagnostic("4")->replaced==2 && f.mqtt_ignored_==1);
  }
  {
    auto p=gateway_with(3); auto &f=*p;
    f.input(0,"16");f.input(1,"16");f.input(2,"16");
    bool delivered=false;
    f.on_discovery=[&](auto &g){if(!delivered){delivered=true;g.input(0,"22");g.input(1,"9.9");}};
    f.run();
    assert(f.writes.size()==3 && f.writes[0].first=="1" && f.writes.back()==std::make_pair(std::string("0"),22.f));
    assert(f.command_buffer_.diagnostic("0")->attempted==1);
  }
  {
    auto p=gateway_with(3); auto &f=*p;
    f.input(0,"16");f.input(1,"16");f.input(2,"16");
    bool delivered=false;
    f.on_write=[&](auto &g){if(!delivered){delivered=true;g.input(0,"22");g.input(0,"9.9");g.input(1,"18");}};
    f.run();
    assert(f.writes.size()==4 && f.writes[0].second==16 && f.writes[1].second==18 && f.writes.back().second==22);
  }
  {
    auto p=gateway_with(8);auto &f=*p;
    f.read_delay=4000;f.write_delay=8000;
    for(int i=0;i<8;++i)f.input(i,"16");
    f.run();
    assert(f.writes.size()==8 && ticks < gateway::CommandBuffer::MAX_AGE_MS);
  }
  {
    auto p=gateway_with(8);auto &f=*p;
    f.preflight_failures=2;f.sid_.clear();
    for(int i=0;i<8;++i)f.input(i,"16");
    f.run();assert(f.writes.size()==8 && !f.command_busy_);
    assert(f.command_buffer_.diagnostic("0")->deferred>0);
  }
  {
    auto p=gateway_with(3);auto &f=*p;
    f.fail_write=true;f.fail_readback=true;f.write_delay=20000;
    for(int i=0;i<3;++i)f.input(i,"16");
    f.run();assert(f.writes.size()==3 && !f.command_busy_);
    for(int i=0;i<3;++i)assert(f.command_buffer_.diagnostic(std::to_string(i))->attempted==1);
  }
  {
    auto p=gateway_with(8);auto &f=*p;f.network_ready_=false;
    for(int i=0;i<8;++i)f.input(i,"16");
    f.run();assert(f.writes.empty() && !f.command_busy_);
    for(int i=0;i<8;++i)assert(f.command_buffer_.diagnostic(std::to_string(i))->expired==1);
  }
  {
    auto p=gateway_with(3);auto &f=*p;
    for(int i=0;i<3;++i)f.input(i,"16");
    auto next=std::make_shared<gateway::Config>(*f.runtime_config_);
    ++next->revision;f.runtime_config_=next;
    f.run();assert(f.writes.empty());
    for(int i=0;i<3;++i)assert(f.command_buffer_.diagnostic(std::to_string(i))->rejected==1);
  }
  {
    auto p=gateway_with(3);auto &f=*p;f.input(0,"16");
    f.on_discovery=[](auto &g){auto next=std::make_shared<gateway::Config>(*g.runtime_config_);++next->revision;g.runtime_config_=next;};
    f.run();assert(f.writes.empty() && !f.command_busy_);
  }
  {
    auto p=gateway_with(3);auto &f=*p;
    f.input(0,"nan");f.input(1,"19.1");f.input(2,"20",true);
    assert(f.mqtt_rejected_==3 && !f.command_busy_);
    for(int i=0;i<3;++i)assert(f.command_buffer_.diagnostic(std::to_string(i))->rejected==1);
  }
  {
    auto p=gateway_with(3);auto &f=*p;f.input(0,"16");
    f.read_delay=gateway::CommandBuffer::MAX_AGE_MS;
    f.run();assert(f.writes.empty());
    assert(f.command_buffer_.diagnostic("0")->expired==1);
  }
  {
    auto p=gateway_with(3);auto &f=*p;
    f.input(0,"16");f.input(1,"16");f.preflight_failures=1;
    bool delivered=false;
    f.on_discovery=[&](auto &g){if(!delivered){delivered=true;g.input(0,"22");g.input(0,"9.9");}};
    f.run();assert(f.writes.size()==2);
    for(const auto &write:f.writes)if(write.first=="0")assert(write.second==22);
    assert(f.command_buffer_.diagnostic("0")->superseded==1);
  }
  {
    auto p=gateway_with(3);auto &f=*p;f.sid_.clear();f.login_ok=false;
    for(int i=0;i<3;++i)f.input(i,"16");
    f.run();assert(f.writes.empty());
    for(int i=0;i<3;++i)assert(f.command_buffer_.diagnostic(std::to_string(i))->expired==1);
  }
  {
    auto p=gateway_with(3);auto &f=*p;
    f.discovered_[0].present="0";
    f.discovered_[1].error="1";
    f.input(0,"16");f.input(1,"16");f.input(2,"16");
    f.run();assert(f.writes.size()==1 && f.writes[0].first=="2");
    assert(f.command_buffer_.diagnostic("0")->rejected==1 && f.command_buffer_.diagnostic("1")->rejected==1);
  }
  {
    auto p=gateway_with(3);auto &f=*p;
    auto c=std::make_shared<gateway::Config>(*f.runtime_config_);c->devices[0].outlet=true;f.runtime_config_=c;
    f.discovered_[0].outlet=true;f.discovered_[0].switch_state="0";
    f.input(0,"1");f.input(0,"9.9");f.input(1,"16");
    f.run();assert(f.writes.size()==2 && f.writes[0].second==1);
  }
  {
    auto p=gateway_with(3);auto &f=*p;
    for(int i=0;i<3;++i)f.input(i,"16");
    f.on_discovery=[](auto &g){g.consume_result_();assert(g.command_busy_);};
    f.run();assert(f.writes.size()==3 && !f.command_busy_);
    f.consume_result_();assert(!f.command_busy_);
  }
  {
    auto p=gateway_with(3);auto &f=*p;f.input(0,"16");
    f.input(0,"20",true);f.input(0,"nan");f.input(0,"9.9");
    f.run();assert(f.writes.size()==1 && f.writes[0].second==16);
  }
  {
    auto p=gateway_with(8);auto &f=*p;
    for(int i=0;i<8;++i) {
      const auto &topic=f.runtime_config_->devices[i].command_topic;
      MqttEvent e{topic.c_str(),"16",int(topic.size()),2,2};
      f.receive_(&e);
    }
    assert(f.queue.inputs.size()==8);
    while(!f.queue.inputs.empty())f.mqtt_loop_();
    f.run();assert(f.writes.size()==8 && f.mqtt_rejected_==0);
  }
  {
    auto p=gateway_with(3);auto &f=*p;
    MqttEvent e{"test/0","16",6,2,2};
    for(int i=0;i<33;++i)f.receive_(&e);
    assert(f.queue.inputs.size()==32 && f.mqtt_rejected_==1);
    assert(f.command_buffer_.diagnostic("0")->overflow==1);
    e.current_data_offset=1;f.receive_(&e);
    assert(f.command_buffer_.diagnostic("0")->rejected==2);
    while(!f.queue.inputs.empty())f.mqtt_loop_();
    f.run();assert(f.writes.size()==1);
  }
  {
    auto p=gateway_with(8);auto &f=*p;
    for(int i=0;i<8;++i)f.input(i,"16");
    int groups=0;
    f.on_write=[&](auto &g) {
      if(g.writes.size()%8==1 && groups<4) {
        ++groups;
        for(int i=0;i<8;++i)g.input(i,std::to_string(16+groups));
      }
    };
    f.run();
    assert(groups==4);
    for(int i=0;i<8;++i) {
      const auto *d=f.command_buffer_.diagnostic(std::to_string(i));
      assert(d->attempted>=4 && d->attempted<=5 && d->last_attempted_target==20);
    }
  }
  std::cout<<"PASS: production MQTT admission, worker wakeup/drain, per-device latest waiting target, neutral input, failures, age, revisions and fair repeated 3/8-device groups\n";
}
"""
if len(sys.argv) > 1:
    if len(sys.argv) != 3 or sys.argv[1] != "--baseline":
        raise SystemExit("Usage: command_pipeline_test.py [--baseline COMMIT]")
    baseline_web = subprocess.check_output(
        ["git", "show", sys.argv[2] + ":components/fritzbox_aha/gateway_web.cpp"],
        cwd=root, text=True,
    )
    baseline_admission = method(
        baseline_web, "bool FritzboxAha::queue_temperature_(", "bool FritzboxAha::status_led_ready"
    )
    # The old signature lacks received_at. Supply the default-only parameter to
    # the extracted definition so the same host class declaration can be reused.
    baseline_admission = baseline_admission.replace(
        "bool outlet) {", "bool outlet, uint32_t received_at) {"
    )
    production = production.replace(
        method(web, "bool FritzboxAha::queue_temperature_(", "bool FritzboxAha::status_led_ready"),
        baseline_admission,
    )
    scenarios = r"""
int main() {
  auto p=gateway_with(3);auto &f=*p;f.web_devices_=f.discovered_;
  for(int i=0;i<3;++i)f.input(i,"16");
  assert(f.mqtt_accepted_==1 && f.mqtt_rejected_==2);
  assert(f.legacy_commands.commands.size()==1);
  std::cout<<"REPRODUCED: original admission accepts 1 of 3 devices and discards 2 while busy\n";
}
"""
with tempfile.TemporaryDirectory() as folder:
    source = Path(folder) / "pipeline.cpp"
    source.write_text(preamble + production + scenarios)
    binary = Path(folder) / "pipeline"
    subprocess.run(["clang++", "-std=c++17", "-fsanitize=address,undefined",
                    "-Wno-deprecated-declarations", "-I", str(component),
                    "-I", str(root / "tests"), str(source), "-lexpat", "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
