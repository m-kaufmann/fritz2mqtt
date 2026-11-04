#include "../components/fritzbox_aha/runtime_config.h"
#include <cassert>
#include <iostream>
using namespace esphome::fritzbox_aha::gateway;
int main(){
 float v=0;
 for(auto s:{"9.9","9.90"," 9.900\n"})assert(parse_target_payload(s,v)==TargetPayload::IGNORE);
 for(auto s:{"8","20.0","20.5","28"," 18.50\r\n"})assert(parse_target_payload(s,v)==TargetPayload::TARGET);
 for(auto s:{""," ","nan","inf","1e1","20x","20,5","7.5","28.5","20.1","-20","+20","20.",".5","9999999999999999","{\"value\":20}"})assert(parse_target_payload(s,v)==TargetPayload::INVALID);
 assert(parse_target_payload(std::string("20\0x",4),v)==TargetPayload::INVALID);
 Config c;Device a;a.ain="A";a.enabled=true;a.command_topic="site/11/d_s_o/a";Device b=a;b.ain="B";b.command_topic="site/12/d_s_o/b";c.devices={a,b};
 assert(command_device(c,"site/11/d_s_o/a")->ain=="A");assert(command_device(c,"site/12/d_s_o/b")->ain=="B");
 assert(!command_device(c,"site/11/d_s_o/ax"));assert(!command_device(c,""));c.devices[0].enabled=false;assert(!command_device(c,a.command_topic));
 std::cout<<"PASS: MQTT numeric parsing, 9.9 sentinel, malformed/oversize values, strict per-device command routing and disabled mapping\n";
}
