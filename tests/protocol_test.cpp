#include "../components/fritzbox_aha/aha_protocol.h"
#include <cassert>
#include <iostream>
using namespace esphome::fritzbox_aha::protocol;
int main() {
  uint32_t encoded;
  assert(no_command(9.9f) && !encode_target(9.9f, encoded));
  assert(!no_command(9.5f) && !no_command(10.0f) && !no_command(NAN));
  for (float value : {8.0f, 22.0f, 22.5f, 28.0f}) {
    assert(encode_target(value, encoded) && encoded == static_cast<uint32_t>(value * 2));
  }
  for (float value : {7.5f, 28.5f, 22.25f, INFINITY, NAN}) assert(!encode_target(value, encoded));
  std::string response, method;
  assert(make_response("2$10000$5A1711$2000$5A1722", "1example!", response, method, nullptr));
  assert(method == "PBKDF2");
  assert(response == "5A1722$1798a1672bca7c6463d6b245f82b53703b0f50813401b03e4045a5861e689adb");
  assert(make_response("1234567z", "äbc", response, method, nullptr));
  assert(method == "MD5" && response == "1234567z-9e224a41eeefa284df7bb0f26c2913e2");
  std::string replacement;
  assert(make_response("abcdef12", "€😀", response, method, nullptr));
  assert(make_response("abcdef12", "..", replacement, method, nullptr));
  assert(response == replacement);
  for (auto c : {"2$0$ab$1$ab", "2$1000001$ab$1$ab", "2$1$xyz$1$ab", "2$1$a$1$ab", "2$1$ab$1$ab$extra", "", "a<&"})
    assert(!make_response(c, "pw", response, method, nullptr));
  assert(form_encode("m&= +ü$") == "m%26%3D%20%2B%C3%BC%24");
  assert(valid_sid("1234567890abcdef"));
  assert(!valid_sid("0000000000000000") && !valid_sid("a123"));
  uint32_t n;
  assert(number("3600", n, 3600) && n == 3600);
  assert(!number("3601", n, 3600) && !number("99999999999999999999999", n, 3600));
  XmlReader xml;
  XmlResult result;
  assert(xml.parse("<?xml version='1.0'?><devicelist version='1'/>", "devicelist", result));
  assert(result.device_count == 0);
  assert(xml.parse("<devicelist><device identifier='123'><name>A &amp; B</name></device><group/><device/></devicelist>", "devicelist", result));
  assert(result.device_count == 2);
  assert(xml.parse("<SessionInfo><SID>1234567890abcdef</SID><Challenge>abc</Challenge><BlockTime>0</BlockTime></SessionInfo>", "SessionInfo", result));
  assert(result.sid == "1234567890abcdef" && result.block_time == "0");
  assert(!xml.parse("<SessionInfo><SID>abc</SID><SID>def</SID></SessionInfo>", "SessionInfo", result));
  assert(!xml.parse("<SessionInfo><SID><x/></SID></SessionInfo>", "SessionInfo", result));
  for (auto body : {"", "inval", "<html/>", "<devicelist><device></devicelist>", "<devicelist>", "<devicelist/><devicelist/>",
                    "<!DOCTYPE devicelist [<!ENTITY x 'abc'>]><devicelist>&x;</devicelist>"})
    assert(!xml.parse(body, "devicelist", result));
  assert(!xml.parse(std::string(65537, ' '), "devicelist", result));
  assert(xml.parse(R"(<devicelist><group><hkr><tsoll>22</tsoll></hkr></group>
    <device identifier="test-ain" productname="FRITZ!DECT 302"><name>Büro &amp; Test</name><present>1</present>
    <battery>90</battery><temperature><celsius>213</celsius></temperature>
    <hkr><tist>42</tist><tsoll>45</tsoll><battery>80</battery><batterylow>0</batterylow><errorcode>0</errorcode></hkr></device>
    <device identifier="socket"><temperature><celsius>200</celsius></temperature></device></devicelist>)", "devicelist", result));
  assert(result.device_count == 2 && result.thermostats.size() == 1);
  auto t = result.thermostats[0];
  assert(t.ain == "test-ain" && t.name == "Büro & Test" && t.product == "FRITZ!DECT 302" && t.present == "1");
  assert(std::abs(actual_temperature(t) - 21.3f) < 0.001f && target_temperature(t) == 22.5f && battery_percent(t) == 80);
  assert(target_mode(t) == "Temperaturregelung" && t.battery_low == "0" && t.error == "0");
  t.celsius = "-55"; assert(actual_temperature(t) == -5.5f);
  t.celsius = ""; assert(actual_temperature(t) == 21);
  t.tist = "254"; assert(std::isnan(actual_temperature(t)));
  t.tsoll = "253"; assert(std::isnan(target_temperature(t)) && target_mode(t) == "Aus");
  t.tsoll = "254"; assert(std::isnan(target_temperature(t)) && target_mode(t) == "Ein");
  for (auto value : {"", "255", "15", "57", "garbage"}) {
    t.tsoll = value; assert(std::isnan(target_temperature(t)) && target_mode(t) == "Unbekannt");
  }
  t.battery = "101"; assert(std::isnan(battery_percent(t)));
  assert(xml.parse("<devicelist><device identifier='offline'><present>0</present><hkr><tist/><tsoll/></hkr></device></devicelist>", "devicelist", result));
  assert(result.thermostats.size() == 1 && std::isnan(actual_temperature(result.thermostats[0])));
  assert(!xml.parse("<devicelist><device><hkr/></device></devicelist>", "devicelist", result));
  assert(xml.parse("<devicelist/>", "devicelist", result) && result.thermostats.empty());
  std::string many = "<devicelist>";
  for (int j = 0; j < 33; ++j) many += "<device identifier='x'><hkr/></device>";
  assert(!xml.parse(many + "</devicelist>", "devicelist", result));
  std::cout << "PASS: thermostat XML, units, unknown/off/on, battery, device/group separation and bounds\n";
  std::cout << "PASS: AVM PBKDF2/MD5 vectors, Unicode fallback, challenge bounds, form encoding, SID, XML empty/device/group/error/DTD tests\n";
}
