#pragma once
#include <array>
#include <cstdint>
#include <cstdio>

// Opt-in scalar metadata only. The emulation thread owns this recorder; callers
// must join it before changing capture state or exporting. No allocation or I/O
// occurs in record(). Sound requests are candidates, never confirmed impacts.
class combat_sound_probe {
 struct Row {
  uint64_t sequence, time_us, frame;
  uint32_t pc;
  int player, before, after, maximum, mode, round, live, note, slot;
 };
 std::array<Row,4096> rows{};
 unsigned next=0, count=0;
 uint64_t total=0;
 bool enabled=false;
 void record(Row row) {
  if (!enabled) return;
  row.sequence=total++;
  rows[next]=row;
  next=(next+1)%rows.size();
  if(count<rows.size())++count;
 }
public:
 bool active() const { return enabled; }
 void clear() { next=count=0; total=0; }
 void capture(bool value) { if(value) clear(); enabled=value; }
 void contact(uint64_t us,uint64_t frame,uint32_t pc,int player,int before,int after,
              int maximum,int mode,int round,int live) {
  record({0,us,frame,pc,player,before,after,maximum,mode,round,live,-1,-1});
 }
 void voice(uint64_t us,uint64_t frame,uint32_t pc,uint32_t channel_pointer,
            bool direct,unsigned note,unsigned slot,int mode,int round,int live) {
  // Same sound-program metadata used by voice_group(). Channel 14 also
  // contains swishes; exclude the three known weather notes, not unknown IDs.
  if(!enabled || !direct || channel_pointer!=0x40e0 || note>255 || slot>31 ||
     note==0x22 || note==0x28 || note==0x29) return;
  record({0,us,frame,pc,0,-1,-1,0,mode,round,live,int(note),int(slot)});
 }
 bool save(const char *path) const {
  FILE *f=fopen(path,"w"); if(!f)return false;
  fprintf(f,"# VF2 scalar sound/contact observations v1; sound rows are voice-start requests, not confirmed audible hits. No ROM, audio or memory dump.\n");
  fprintf(f,"# overwritten=%llu; capture=%s; timestamps use emulated microseconds, not speaker playback time.\n",
          (unsigned long long)(total-count),enabled?"on":"off");
  fprintf(f,"sequence,kind,time_us,frame,pc_hex,player,before,after,maximum,mode,round,live,channel,note_hex,slot\n");
  for(unsigned i=0;i<count;++i) {
   const auto &r=rows[(next+rows.size()-count+i)%rows.size()];
   fprintf(f,"%llu,%s,%llu,%llu,%08x,%d,%d,%d,%d,%d,%d,%d,%d,",
    (unsigned long long)r.sequence,r.note<0?"contact_store":"voice_start_request",
    (unsigned long long)r.time_us,(unsigned long long)r.frame,r.pc,
    r.player,r.before,r.after,r.maximum,r.mode,r.round,r.live,r.note<0?-1:14);
   if(r.note>=0)fprintf(f,"%02x",r.note);
   fprintf(f,",%d\n",r.slot);
  }
  bool ok=!ferror(f); return fclose(f)==0 && ok;
 }
};
