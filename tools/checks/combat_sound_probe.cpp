#include "../../src/machine/combat_sound_probe.h"
#include <cassert>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>
static std::vector<std::string> read(const char *path) {
 std::ifstream file(path); std::string line; std::vector<std::string> lines;
 while(std::getline(file,line))lines.push_back(line);
 return lines;
}
int main() {
 combat_sound_probe p;
 char path[]="/tmp/vf-sound-probe-XXXXXX";
 int fd=mkstemp(path);assert(fd>=0);close(fd);
 p.contact(1,1,0x18a7c,1,100,90,100,9,9,2); // default off
 assert(p.save(path));assert(read(path).size()==3);
 p.capture(true);
 p.contact(100,2,0x18a7c,1,100,80,100,9,9,2);
 p.voice(120,2,0x600a96,0x40e0,true,0x10,31,9,9,2);
 p.contact(140,3,0x18a7c,2,0,0,100,9,17,0); // post-KO zero store retained
 for(unsigned note:{0x22,0x28,0x29}) p.voice(150,3,0x600a96,0x40e0,true,note,0,9,9,2);
 p.voice(150,3,0x600a96,0x40e0,false,0x10,0,9,9,2); // sequencer
 p.voice(150,3,0x600a96,0x40d0,true,0x10,0,9,9,2); // crowd / announcer
 p.voice(150,3,0x600a96,0x40e1,true,0x10,0,9,9,2); // not an entry
 p.voice(150,3,0x600a96,0x40e0,true,0x100,0,9,9,2);
 p.voice(150,3,0x600a96,0x40e0,true,0x10,32,9,9,2);
 p.capture(false); p.contact(200,4,0x18a7c,1,80,70,100,9,9,2);
 assert(p.save(path));auto lines=read(path);assert(lines.size()==6);
 assert(lines[3]=="0,contact_store,100,2,00018a7c,1,100,80,100,9,9,2,-1,,-1");
 assert(lines[4]=="1,voice_start_request,120,2,00600a96,0,-1,-1,0,9,9,2,14,10,31");
 assert(lines[5]=="2,contact_store,140,3,00018a7c,2,0,0,100,9,17,0,-1,,-1");
 p.capture(true);
 for(unsigned i=0;i<4100;++i)p.contact(i,i,0x18a7c,1,0,0,100,9,17,0);
 assert(p.save(path));lines=read(path);assert(lines.size()==4099);
 assert(lines[1].find("overwritten=4")!=std::string::npos);
 assert(lines[3].starts_with("4,contact_store,4,4,"));
 assert(lines.back().starts_with("4099,contact_store,4099,4099,"));
 p.clear();assert(p.active());assert(p.save(path));assert(read(path).size()==3);
 assert(!p.save("/nonexistent-vf-sound/path"));unlink(path);
}
