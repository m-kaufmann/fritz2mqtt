#include "../components/fritzbox_aha/firmware_policy.h"
#include <cassert>
#include <iostream>
using namespace esphome::fritzbox_aha::firmware;
int main() {
 Candidate c; c.current="0.1.0-dev"; c.latest="0.1.0";
 c.url="https://m-kaufmann.github.io/fritz2mqtt/firmware/esp32-4mb-v1/0.1.0/firmware.ota.bin";
 c.md5=std::string(32,'a'); c.verified=true; c.available=true; c.checked_at=1000;
 assert(reject(c,1000).empty());
 for (auto v : {"0.0.9","0.1.0-dev","0.1","01.1.0","1.2.3.4","1.2.10000","1.2.3-rc.1","99999999999999999999.0.0"}) {
  auto bad=c; bad.latest=v; assert(!reject(bad,1000).empty());
 }
 auto stable=c; stable.current="0.1.0"; assert(!reject(stable,1000).empty());
 for (auto url : {"http://m-kaufmann.github.io/fritz2mqtt/firmware/esp32-4mb-v1/0.1.0/firmware.ota.bin", "https://example.com/firmware.ota.bin", "https://m-kaufmann.github.io/fritz2mqtt/firmware/esp32-4mb-v1/0.1.0/../firmware.ota.bin", "https://m-kaufmann.github.io/fritz2mqtt/firmware/esp32-8mb-v1/0.1.0/firmware.ota.bin"}) {
  auto bad=c; bad.url=url; assert(!reject(bad,1000).empty());
 }
 auto bad=c; bad.md5=""; assert(!reject(bad,1000).empty()); bad.md5=std::string(32,'z'); assert(!reject(bad,1000).empty());
 for(int i=0;i<5;++i){bad=c;if(i==0)bad.failed=true;if(i==1)bad.checking=true;if(i==2)bad.installing=true;if(i==3)bad.verified=false;if(i==4)bad.available=false;assert(!reject(bad,1000).empty());}
 assert(!reject(c,601001).empty());
 c.checked_at=0xfffffff0U; assert(reject(c,100).empty()); // millis wrap
 std::cout<<"Firmware policy: versions, trust origin/profile, freshness, failure and busy gates PASS\n";
}
