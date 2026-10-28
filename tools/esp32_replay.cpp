#include "../firmware/esp32/include/protocol.hpp"
#include <iostream>
int main(){
    nova::Game game(97);game.reset();char buffer[8192];bool first=true;
    std::cout<<'[';
    for(int i=0;i<1800;++i){
        game.tick({std::sin(i*.014f)*.8f,0,true,false,false});
        if(i%3==0){auto n=nova::snapshot(game,buffer,sizeof(buffer));if(!n)return 1;std::cout<<(first?"":",")<<buffer;first=false;}
        if(game.state==nova::State::over)game.reset();
    }
    std::cout<<"]\n";
}
