#pragma once
#include <cstdint>
#include <array>
#include <fstream>

class Chip8 {
    public:
        Chip8();
        void cycle(); //Contain fetch-decode-execute
        bool loadROM(const char* filename);

        // CPU & MEMORY (4KB)
        std::array<uint8_t,4096> memory{};

        // 16 Registers (V0 - VF)
        std::array<uint8_t,16> V{};

        // Index register
        uint16_t I{};

        // Program counter
        uint16_t pc{};

        // Stack
        std::array<uint16_t,16> stack{};

        // Stack pointer
        uint8_t sp{};

        // Timer
        uint8_t delayTimer{};
        uint8_t soundTimer{};

        // Display & input
        std::array<uint32_t,64*32> display{};
        std::array<uint8_t,16>keypad{};
};