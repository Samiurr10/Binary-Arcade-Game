#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Preferences.h>
#include "arcade.hpp"
#include "protocol.hpp"
#include "web_ui.hpp"

#ifndef ARCADE_JOYSTICK
#define ARCADE_JOYSTICK 0
#endif
// Use ADC1 for axes: Freenove's standalone lesson uses ADC2 GPIO14/13,
// but those can return failed reads while this firmware's Wi-Fi AP is active.
constexpr int joyX=1, joyY=2, joySwitch=12;
static_assert(joyX >= 1 && joyX <= 10 && joyY >= 1 && joyY <= 10,
              "Wi-Fi joystick axes must use ESP32-S3 ADC1 GPIOs");
WebServer http(80);
WebSocketsServer sockets(81);
Preferences prefs;
nova::Game game;
nova::Input controls;
int owner=-1;
uint32_t lastInput=0;
bool storageReady=false;
char packet[8192];
struct Save {uint32_t version,best,games,check;};
uint32_t checksum(const Save& s) {return s.version^s.best^s.games^0x4e4f5641;}

void releaseController() {
    owner=-1; controls={};
    if(!ARCADE_JOYSTICK) game.suspend();
}
void onSocket(uint8_t client,WStype_t type,uint8_t* data,size_t length) {
    if(type==WStype_DISCONNECTED && owner==client) releaseController();
    if(type!=WStype_TEXT) return;
    if(length==5 && memcmp(data,"claim",5)==0) {
        if(owner<0 || owner==client) {
            owner=client; lastInput=millis();
            sockets.sendTXT(client,"{\"role\":\"controller\"}");
        } else sockets.sendTXT(client,"{\"role\":\"viewer\"}");
    } else if(owner==client) {
        nova::Input next;
        if(nova::parseInput(reinterpret_cast<const char*>(data),length,next)) {controls=next;lastInput=millis();}
    }
}
void setup() {
    Serial.begin(115200);
    game=nova::Game(esp_random());
    storageReady=prefs.begin("binary-arcade",false);
    Save saved{};
    if(storageReady && prefs.getBytesLength("save")==sizeof(saved)) {
        prefs.getBytes("save",&saved,sizeof(saved));
        if(saved.version==1 && saved.check==checksum(saved)) {game.best=saved.best;game.games=saved.games;}
    }
    if(!storageReady) Serial.println("NVS unavailable: scores will not persist");
    if(ARCADE_JOYSTICK) {pinMode(joySwitch,INPUT_PULLUP);analogReadResolution(12);}
    WiFi.mode(WIFI_AP);
    WiFi.setSleep(false);
    char ssid[32]; snprintf(ssid,sizeof(ssid),"BinaryArcade-%04X",static_cast<unsigned>(ESP.getEfuseMac()&0xffff));
    if(!WiFi.softAP(ssid,"arcade-play",1,0,4)) {Serial.println("Wi-Fi AP failed"); while(true) delay(1000);}
    http.on("/",HTTP_GET,[] {http.sendHeader("Cache-Control","no-store");http.send_P(200,"text/html; charset=utf-8",web_ui);});
    http.onNotFound([] {http.send(404,"text/plain","Not found");});
    http.begin(); sockets.begin(); sockets.onEvent(onSocket);
    Serial.printf("BINARY_ARCADE_READY\nWi-Fi: %s\nPassword: arcade-play\nOpen http://192.168.4.1\n",ssid);
}
void loop() {
    http.handleClient(); sockets.loop();
    const uint32_t now=millis();
    if(owner>=0 && now-lastInput>400) {controls={}; if(!ARCADE_JOYSTICK) game.suspend();}
    if(owner>=0 && now-lastInput>5000) {sockets.sendTXT(owner,"{\"role\":\"viewer\"}"); releaseController();}
    static uint32_t last=micros(), accumulator=0,lastFrame=0;
    const uint32_t current=micros();
    accumulator+=std::min<uint32_t>(current-last,100000); last=current;
    // Bound catch-up work so a slow network cannot starve the watchdog.
    int steps=0;
    while(accumulator>=16667 && steps++<6) {
        accumulator-=16667;
        nova::Input input=controls;
        if(ARCADE_JOYSTICK) {
            auto axis=[](int pin) {float v=(analogRead(pin)-2048)/1800.0f;return std::abs(v)<0.12f?0.0f:std::clamp(v,-1.0f,1.0f);};
            input.x=axis(joyX);input.y=axis(joyY);
            input.fire=digitalRead(joySwitch)==LOW || controls.fire;
            // Hold joystick switch to shoot; press it to start/retry.
            input.start=input.fire || controls.start;
        }
        auto previous=game.state; game.tick(input);
        if(previous==nova::State::playing && game.state==nova::State::over && storageReady) {
            Save s{1,game.best,game.games,0};s.check=checksum(s);
            if(prefs.putBytes("save",&s,sizeof(s))!=sizeof(s)) Serial.println("NVS save failed");
        }
    }
    if(now-lastFrame>=40) {
        lastFrame=now;
        size_t length=nova::snapshot(game,packet,sizeof(packet));
        if(length) sockets.broadcastTXT(packet,length);
    }
    delay(1);
}
