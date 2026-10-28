#include "arcade.hpp"
#include "protocol.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
void check(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
int main(){
    using namespace nova;
    Game game(1);game.tick({0,0,false,true,false});check(game.state==State::playing,"start flight");
    game.tick({0,0,false,false,true});auto tick=game.ticks;
    for(int i=0;i<20;++i)game.tick({0,0,true,false,true});
    check(game.state==State::paused&&game.ticks==tick,"held pause has one edge and freezes simulation");
    game.tick({});game.tick({0,0,false,false,true});check(game.state==State::playing,"resume");
    game.reset();game.rocks[0]={game.x,game.y,0,0,20,0,0,1,1};game.tick({});
    check(game.shields==2&&game.invincible>0,"collision consumes one shield");
    game.rocks[1]={game.x,game.y,0,0,20,0,0,2,1};game.tick({});check(game.shields==2,"invulnerability prevents stacked damage");
    game.invincible=0;game.shields=1;game.tick({});check(game.state==State::over&&game.games==1,"final shield ends round exactly once");
    for(int i=0;i<100;++i) { game.tick({}); }
    check(game.games==1,"game-over ticks do not count new games");
    game.best=1234;game.reset();check(game.best==1234&&game.shields==3,"restart preserves best and resets shields");
    game.rocks[0]={240,300,0,0,3,0,0,7,1};game.shots[0]={240,306,true};game.tick({});
    check(game.rocks[0].hp==0&&game.score==100&&!game.shots[0].active,"swept laser hits small target");
    game.reset();game.rocks[0]={240,300,0,0,20,0,0,7,1};game.shots[0]={280,306,true};game.tick({});
    check(game.rocks[0].hp==1,"near miss does not hit target");
    game.pickup={game.x,game.y,true};game.shields=2;game.tick({});check(game.shields==3&&!game.pickup.active,"pickup restores shield");
    game.tick({std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()});
    check(std::isfinite(game.x)&&std::isfinite(game.y),"nonfinite inputs cannot corrupt state");
    Input input;std::string valid="i,-1,0.5,1,0,0";check(parseInput(valid.data(),valid.size(),input)&&input.fire,"valid network input");
    for(auto bad:{"i,nan,0,1,0,0","i,2,0,0,0,0","i,0,0,2,0,0","i,0,0,1,0,0tail","i,0,0,1","claim"})
        check(!parseInput(bad,std::strlen(bad),input),"reject malformed network controls");
    char output[8192];
    for(auto& r:game.rocks)r={479,719,0,0,33,10000,0,65535,3};
    for(auto& p:game.particles)p={479,719,0,0,.9f,2};
    for(auto& s:game.shots)s={479,719,true};
    auto length=snapshot(game,output,sizeof(output));check(length>0&&length<sizeof(output)&&output[length-1]=='}',"maximum snapshot fits buffer");
    char small[8];check(snapshot(game,small,sizeof(small))==0,"undersized snapshot fails safely");
    Game a(99),b(99);a.reset();b.reset();
    for(int i=0;i<20000;++i){Input v{(i%120<60?-.7f:.7f),0,true,false,false};a.tick(v);b.tick(v);
        check(a.x>=18&&a.x<=width-18&&std::isfinite(a.x),"player stays in bounds");
        check(a.score==b.score&&a.x==b.x&&a.state==b.state,"deterministic simulation");
        if(a.state==State::over){a.reset();b.reset();}
    }
    std::cout<<"All ESP32 gameplay/protocol tests passed\n";
}
