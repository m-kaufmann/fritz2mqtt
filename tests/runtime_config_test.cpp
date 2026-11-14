#include "../components/fritzbox_aha/runtime_config.h"
#include <cassert>
#include <iostream>
using namespace esphome::fritzbox_aha::gateway;
int main() {
  Config c; c.host="192.0.2.1"; c.username="test"; c.password="test";
  assert(validate(c).empty());
  Device a; a.ain="00000 0000001"; a.name="Bad"; a.unit="3"; a.base="hkth_bad";
  a.command_topic="fritz2mqtt/site/3/d_s_o/hkth_bad_t_neu";
  a.actual_topic="fritz2mqtt/site/3/hkth_bad_t_ist";
  c.devices.push_back(a); assert(validate(c).empty());
  auto b=a; b.ain="00000 0000002"; b.name="Zimmer";
  c.devices.push_back(b); assert(!validate(c).empty());
  c.devices.back().command_topic=""; c.devices.back().actual_topic=""; assert(validate(c).empty());
  assert(find(c,b.ain)->name=="Zimmer" && !find(c,"missing"));
  c.devices.back().ain=a.ain; assert(!validate(c).empty());
  c.devices.pop_back(); c.devices[0].actual_topic="fritz2mqtt/#"; assert(!validate(c).empty());
  c.devices.clear(); c.location="A/B"; assert(!validate(c).empty());
  for (auto ip : {"", "1.2.3", "1.2.3.256", "1.2.3.4.5", "1..3.4", "1.2.3.4x"}) assert(!ipv4(ip));
  assert(ipv4("192.0.2.1"));
  assert(!topic("a/+")); assert(!topic("a//b")); assert(topic("fritz2mqtt/site/3/hkth_t_soll"));
  c.location="AK3"; c.mqtt_port=0; assert(!validate(c).empty());
  c.mqtt_port=8883; c.password=""; assert(validate(c).empty());  // WLAN-only onboarding is allowed.
  c.password="test";
  c.devices.clear();
  for(int i=0;i<9;i++){Device d;d.ain="TEST-"+std::to_string(i);d.name="Test";c.devices.push_back(d);}
  assert(!validate(c).empty());c.devices.pop_back();assert(validate(c).empty());
  c.devices.clear();
  c.wifi_configured=true;assert(!validate(c).empty());
  c.wifi[0]={"Werkstatt","abcdefgh"};c.wifi[1]={"Einsatzort","12345678"};assert(validate(c).empty());
  c.wifi[1].ssid="Werkstatt";assert(!validate(c).empty());
  c.wifi[1].ssid="Einsatzort";c.wifi[1].password="short";assert(!validate(c).empty());
  c.wifi[1].password="";assert(validate(c).empty());  // Explicit open network
  c.wifi[1].ssid=std::string(33,'x');assert(!validate(c).empty());
  c.wifi[1]={"","12345678"};assert(!validate(c).empty());
  c.wifi[1]={"",""};assert(validate(c).empty());
  std::cout<<"PASS: runtime config, multi-device AIN routing, topic uniqueness, bounds and malformed input\n";
}
