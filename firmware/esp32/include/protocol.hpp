#pragma once
#include "arcade.hpp"
#include <cstdarg>
#include <cstdio>
#include <cstring>
namespace nova {
inline bool parseInput(const char* data, size_t length, Input& output) {
    if(length==0 || length>80 || std::memchr(data,0,length)) return false;
    char text[81]; std::memcpy(text,data,length); text[length]=0;
    Input candidate; int fire,start,pause,used=0;
    if(std::sscanf(text,"i,%f,%f,%d,%d,%d%n",&candidate.x,&candidate.y,&fire,&start,&pause,&used)!=5 ||
       used!=static_cast<int>(length) || !std::isfinite(candidate.x) || !std::isfinite(candidate.y) ||
       std::abs(candidate.x)>1 || std::abs(candidate.y)>1 || fire<0 || fire>1 || start<0 || start>1 || pause<0 || pause>1) return false;
    candidate.fire=fire; candidate.start=start; candidate.pause=pause; output=candidate; return true;
}
class JsonWriter {
public:
    JsonWriter(char* buffer,size_t capacity):p(buffer),cap(capacity){}
    void add(const char* format,...) {
        if(failed) return;
        va_list args; va_start(args,format); int n=std::vsnprintf(p+size,cap-size,format,args); va_end(args);
        if(n<0 || static_cast<size_t>(n)>=cap-size) {failed=true; return;} size+=n;
    }
    size_t result() const {return failed?0:size;}
private:
    char* p; size_t cap,size=0; bool failed=false;
};
inline size_t snapshot(const Game& g,char* buffer,size_t capacity) {
    JsonWriter w(buffer,capacity);
    w.add("{\"v\":1,\"state\":%u,\"tick\":%lu,\"score\":%lu,\"best\":%lu,\"combo\":%u,\"shields\":%u,\"x\":%.1f,\"y\":%.1f,\"vx\":%.1f,\"inv\":%.2f,\"shake\":%.1f,\"rocks\":[",
      static_cast<unsigned>(g.state),static_cast<unsigned long>(g.ticks),static_cast<unsigned long>(g.score),static_cast<unsigned long>(g.best),g.combo,g.shields,g.x,g.y,g.vx,g.invincible,g.shake);
    bool first=true;
    for(const auto& r:g.rocks) if(r.hp) {w.add("%s[%u,%.1f,%.1f,%.1f,%.2f,%u]",first?"":",",r.id,r.x,r.y,r.r,r.angle,r.hp);first=false;}
    w.add("],\"shots\":[");first=true;
    for(const auto& s:g.shots) if(s.active) {w.add("%s[%.1f,%.1f]",first?"":",",s.x,s.y);first=false;}
    w.add("],\"particles\":[");first=true;
    for(const auto& p:g.particles) if(p.life>0) {w.add("%s[%.1f,%.1f,%.2f,%u]",first?"":",",p.x,p.y,p.life,p.kind);first=false;}
    w.add("],\"pickup\":[%.1f,%.1f,%u]}",g.pickup.x,g.pickup.y,g.pickup.active?1:0);
    return w.result();
}
} // namespace nova
