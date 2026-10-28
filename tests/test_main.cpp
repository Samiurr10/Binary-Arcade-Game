#include "binary_arcade/bus.hpp"
#include "binary_arcade/crc.hpp"
#include "binary_arcade/flash_iap.hpp"
#include "binary_arcade/game.hpp"
#include "binary_arcade/system.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int failures = 0;

void require(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void test_crc_known_vector() {
    const std::vector<std::uint8_t> bytes = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    require(binary_arcade::crc16_ccitt(bytes) == 0x29B1, "CRC-16/CCITT known vector");
}

void test_framebuffer_pack_and_ascii() {
    binary_arcade::Framebuffer fb;
    fb.set(0, 0, true);
    fb.set(15, 7, true);
    const auto packed = fb.pack();
    require(packed[0] == 0x01, "framebuffer packs low byte");
    require(packed[15] == 0x80, "framebuffer packs high byte");
    require(fb.to_ascii().front() == '#', "framebuffer ascii render marks lit pixels");
}

void test_game_movement_fire_and_game_over() {
    binary_arcade::GameEngine game(7);
    const auto start = game.player_x();
    auto result = game.tick(binary_arcade::Input::left);
    require(result.state == binary_arcade::GameState::running, "game remains running after move");
    require(game.player_x() == start - 1, "game accepts left input");
    result = game.tick(binary_arcade::Input::right);
    require(result.state == binary_arcade::GameState::running, "game remains running after second move");
    require(game.player_x() == start, "game accepts right input");

    for (int i = 0; i < 80 && game.state() == binary_arcade::GameState::running; ++i) {
        result = game.tick(i % 2 == 0 ? binary_arcade::Input::fire : binary_arcade::Input::none);
    }
    require(game.score() > 0, "game advances score");
}

void test_i2c_display_frame() {
    binary_arcade::RecordingI2cBus bus;
    binary_arcade::I2cDisplay display(bus);
    binary_arcade::Framebuffer fb;
    fb.set(3, 2, true);
    require(display.flush(fb), "i2c flush acknowledges");
    require(bus.writes.size() == 1, "i2c records one write");
    require(bus.writes[0].address == 0x3C, "i2c uses display address");
    require(bus.writes[0].bytes[0] == 0xA5, "i2c frame sync byte");
    require(bus.writes[0].bytes[1] == binary_arcade::Framebuffer::packed_size,
            "i2c frame length");
}

void test_uart_telemetry() {
    binary_arcade::RecordingUartSink sink;
    binary_arcade::UartTelemetry telemetry(sink);
    telemetry.emit_score(42, true);
    const auto text = sink.text();
    require(text.find("score=42") != std::string::npos, "uart includes score");
    require(text.find("state=game_over") != std::string::npos, "uart includes state");
    require(text.find("crc=") != std::string::npos, "uart includes checksum");
}

void test_spi_sprite_read() {
    std::vector<std::uint8_t> rom(64);
    for (std::size_t i = 0; i < rom.size(); ++i) {
        rom[i] = static_cast<std::uint8_t>(i);
    }
    binary_arcade::MemorySpiDevice device(rom);
    binary_arcade::SpiSpriteRom sprites(device);
    const auto sprite = sprites.read_sprite(2, 4);
    require(sprite.size() == 4, "spi returns requested sprite bytes");
    require(sprite[0] == 32 && sprite[3] == 35, "spi offsets by sprite id");
}

void test_flash_iap_persists_latest_and_wears_pages() {
    binary_arcade::FlashIap flash(2, 240);
    require(!flash.load_latest().has_value(), "empty flash has no record");
    require(flash.save({1, 100, 1}), "flash saves first record");
    require(flash.save({2, 140, 2}), "flash saves second record");
    const auto latest = flash.load_latest();
    require(latest.has_value(), "flash loads latest record");
    require(latest->sequence == 2 && latest->high_score == 140,
            "flash selects highest sequence");

    for (std::uint32_t i = 3; i < 30; ++i) {
        require(flash.save({i, i * 10, i}), "flash keeps saving across page rotation");
    }
    const auto wear = flash.wear_counts();
    require(wear[0] > 0 || wear[1] > 0, "flash erases pages after log fills");
}

void test_system_integrates_drivers_and_flash() {
    binary_arcade::GameEngine game(1);
    binary_arcade::RecordingI2cBus i2c;
    binary_arcade::RecordingUartSink uart;
    binary_arcade::I2cDisplay display(i2c);
    binary_arcade::UartTelemetry telemetry(uart);
    binary_arcade::FlashIap flash;
    binary_arcade::ArcadeSystem system(game, display, telemetry, flash);
    binary_arcade::TickResult result{binary_arcade::GameState::running, 0, false};

    for (int i = 0; i < 100 && game.state() == binary_arcade::GameState::running; ++i) {
        result = system.tick(i % 3 == 0 ? binary_arcade::Input::left : binary_arcade::Input::right);
    }
    require(!i2c.writes.empty(), "system flushes display each tick");
    require(!uart.bytes.empty(), "system emits telemetry each tick");
    require(game.state() == binary_arcade::GameState::game_over, "system reaches game over");
    require(flash.load_latest().has_value(), "system persists high score on game over");
}


void test_regressions() {
    using namespace binary_arcade;
    GameEngine edge(7);
    for(int i=0;i<8;++i) (void)edge.tick(Input::left|Input::fire);
    const auto before=edge.player_x();
    (void)edge.tick(Input::left|Input::right|Input::fire);
    require(edge.player_x()==before, "opposite buttons cancel at the left boundary");
    GameEngine lanes(1);
    unsigned seen=0;
    for(int i=0;i<200;++i) {
        (void)lanes.tick(Input::fire);
        auto packed=lanes.framebuffer().pack();
        seen |= packed[0] | (static_cast<unsigned>(packed[1])<<8U);
    }
    require(seen==0xffffU,"random obstacles reach every lane instead of repeating low LCG bits");
    FlashIap flash;
    require(flash.save({0xfffffffeU,100000,10}), "seed persisted stats");
    GameEngine game(1);
    RecordingI2cBus bus; RecordingUartSink sink;
    I2cDisplay display(bus); UartTelemetry telemetry(sink);
    ArcadeSystem system(game,display,telemetry,flash);
    for(int round=0;round<3;++round) {
        game.reset(1);
        for(int i=0;i<100 && game.state()==GameState::running;++i)
            (void)system.tick(i%3==0?Input::left:Input::right);
        require(game.state()==GameState::game_over,"regression round terminates");
        (void)system.tick(Input::none);
        require(system.current_save().games_played==static_cast<unsigned>(11+round),
                "count each completed game once even without a new high score");
    }
    require(flash.load_latest()->sequence==1,"load newest sequence across uint32 rollover");
    require(flash.load_latest()->games_played==13,"restore all completed rounds");
}

}  // namespace

int main() {
    test_regressions();
    test_crc_known_vector();
    test_framebuffer_pack_and_ascii();
    test_game_movement_fire_and_game_over();
    test_i2c_display_frame();
    test_uart_telemetry();
    test_spi_sprite_read();
    test_flash_iap_persists_latest_and_wears_pages();
    test_system_integrates_drivers_and_flash();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All binary arcade tests passed\n";
    return EXIT_SUCCESS;
}
