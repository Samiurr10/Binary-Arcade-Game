#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace nova {
constexpr float width = 480, height = 720, step = 1.0f / 60.0f;
enum class State : uint8_t { title, playing, paused, over };
struct Input { float x = 0, y = 0; bool fire = false, start = false, pause = false; };
struct Rock { float x=0,y=0,vx=0,vy=0,r=0,angle=0,spin=0; uint16_t id=0; uint8_t hp=0; };
struct Shot { float x=0,y=0; bool active=false; };
struct Particle { float x=0,y=0,vx=0,vy=0,life=0; uint8_t kind=0; };
struct Pickup { float x=0,y=0; bool active=false; };
class Game {
public:
    State state=State::title;
    float x=width/2, y=height-100, vx=0, vy=0, invincible=0, shake=0;
    uint32_t ticks=0, score=0, best=0, games=0;
    uint16_t combo=0;
    uint8_t shields=3;
    std::array<Rock,18> rocks{};
    std::array<Shot,24> shots{};
    std::array<Particle,64> particles{};
    Pickup pickup{};
    explicit Game(uint32_t seed=0xC0FFEE): rng(seed ? seed : 1) {}
    void reset() {
        x=width/2; y=height-100; vx=vy=invincible=shake=0;
        ticks=score=combo=0; shields=3; rocks={}; shots={}; particles={}; pickup={};
        spawnTimer=0.6f; fireTimer=0; comboTimer=0; nextId=1;
        state=State::playing;
    }
    void suspend() { if(state==State::playing) state=State::paused; }
    void tick(Input in) {
        const bool startEdge=in.start&&!lastStart, pauseEdge=in.pause&&!lastPause;
        lastStart=in.start; lastPause=in.pause;
        if(startEdge && (state==State::title || state==State::over)) { reset(); return; }
        if(pauseEdge) {
            if(state==State::playing) state=State::paused;
            else if(state==State::paused) state=State::playing;
        }
        if(state!=State::playing) return;
        ++ticks;
        in.x=std::isfinite(in.x)?std::clamp(in.x,-1.0f,1.0f):0;
        in.y=std::isfinite(in.y)?std::clamp(in.y,-1.0f,1.0f):0;
        const float magnitude=std::sqrt(in.x*in.x+in.y*in.y);
        if(magnitude>1) {in.x/=magnitude; in.y/=magnitude;}
        vx+=(in.x*300-vx)*0.18f; vy+=(in.y*280-vy)*0.18f;
        x=std::clamp(x+vx*step,26.0f,width-26); y=std::clamp(y+vy*step,80.0f,height-32);
        invincible=std::max(0.0f,invincible-step); shake=std::max(0.0f,shake-step*18);
        comboTimer-=step; if(comboTimer<=0) combo=0;
        fireTimer-=step;
        if(in.fire && fireTimer<=0) {
            for(auto& s:shots) if(!s.active) { s={x,y-22,true}; fireTimer=0.13f; break; }
        }
        const float difficulty=std::min(8.0f,ticks/1800.0f);
        spawnTimer-=step;
        if(spawnTimer<=0) {
            spawnTimer=std::max(0.23f,0.9f-difficulty*0.075f);
            for(auto& r:rocks) if(!r.hp) {
                r.r=14+random()%20; r.x=r.r+random()%static_cast<int>(width-2*r.r);
                r.y=-r.r; r.vx=static_cast<int>(random()%61)-30;
                r.vy=75+difficulty*17+random()%45; r.hp=r.r>25?3:1;
                r.angle=random()%628/100.0f; r.spin=(static_cast<int>(random()%201)-100)/100.0f;
                r.id=nextId++; break;
            }
        }
        for(auto& p:particles) if(p.life>0) {p.life-=step; p.x+=p.vx*step; p.y+=p.vy*step; p.vx*=0.97f; p.vy*=0.97f;}
        for(auto& r:rocks) if(r.hp) {
            r.x+=r.vx*step; r.y+=r.vy*step; r.angle+=r.spin*step;
            if(r.x<r.r) {r.x=r.r; r.vx=std::abs(r.vx);}
            if(r.x>width-r.r) {r.x=width-r.r; r.vx=-std::abs(r.vx);}
            if(r.y-r.r>height) {r.hp=0; combo=0;}
        }
        // Swept vertical segment: fast shots cannot tunnel through small rocks.
        for(auto& s:shots) if(s.active) {
            const float oldY=s.y; s.y-=640*step;
            Rock* hit=nullptr; float nearest=1e9f;
            for(auto& r:rocks) if(r.hp) {
                const float dx=std::abs(s.x-r.x), radius=r.r+3;
                if(dx>radius) continue;
                const float half=std::sqrt(radius*radius-dx*dx);
                if(oldY>=r.y-half && s.y<=r.y+half) {
                    const float distance=std::max(0.0f,oldY-(r.y+half));
                    if(distance<nearest) {nearest=distance; hit=&r;}
                }
            }
            if(hit) {
                s.active=false;
                if(--hit->hp==0) {
                    combo=std::min<uint16_t>(combo+1,20); comboTimer=2.5f;
                    score+=100+25*(combo-1); best=std::max(best,score); shake=3;
                    burst(hit->x,hit->y,24,0);
                    if(!pickup.active && random()%7==0) pickup={hit->x,hit->y,true};
                } else burst(s.x,s.y,5,1);
            }
            if(s.y<0) s.active=false;
        }
        for(auto& r:rocks) if(r.hp && invincible<=0) {
            const float dx=x-r.x,dy=y-r.y, radius=r.r+11;
            if(dx*dx+dy*dy<radius*radius) {
                r.hp=0; --shields; combo=0; invincible=1.5f; shake=10; burst(x,y,32,1);
                if(shields==0) {state=State::over; ++games; best=std::max(best,score); return;}
            }
        }
        if(pickup.active) {
            pickup.y+=90*step;
            if(std::abs(x-pickup.x)<25 && std::abs(y-pickup.y)<25) {
                shields=std::min<uint8_t>(3,shields+1); score+=50; best=std::max(best,score);
                burst(x,y,16,2); pickup.active=false;
            } else if(pickup.y>height+20) pickup.active=false;
        }
        if(ticks%60==0) {score+=10; best=std::max(best,score);}
    }
private:
    uint32_t rng;
    uint16_t nextId=1;
    bool lastStart=false,lastPause=false;
    float spawnTimer=0.6f,fireTimer=0,comboTimer=0;
    uint32_t random() {rng^=rng<<13; rng^=rng>>17; rng^=rng<<5; return rng;}
    void burst(float px,float py,int count,uint8_t kind) {
        for(auto& p:particles) if(p.life<=0 && count>0) {
            --count; float a=random()%628/100.0f, speed=40+random()%190;
            p={px,py,std::cos(a)*speed,std::sin(a)*speed,0.25f+random()%60/100.0f,kind};
        }
    }
};
} // namespace nova
