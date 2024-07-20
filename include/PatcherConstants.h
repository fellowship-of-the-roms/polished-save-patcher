#ifndef PATCHER_CONSTANTS_H
#define PATCHER_CONSTANTS_H

#include <cstdint> // Add this include directive

// SRAM constants
constexpr uint16_t SRAM_BANK_SIZE = 0x2000;
constexpr uint16_t SRAM_START_ADDRESS = 0xA000;
constexpr uint8_t NUM_SRAM_BANKS = 4;
constexpr uint8_t SAVE_VERSION_SRAM_BANK = 0x00;
constexpr uint16_t SAVE_VERSION_ADDRESS = 0xABE2;
constexpr uint32_t SAVE_VERSION_ABS_ADDRESS = SAVE_VERSION_SRAM_BANK * SRAM_BANK_SIZE + SAVE_VERSION_ADDRESS - SRAM_START_ADDRESS;
constexpr uint8_t SAVE_CHECKSUM_SRAM_BANK = 0x01;
constexpr uint16_t SAVE_CHECKSUM_ADDRESS = 0xAD0D;
constexpr uint32_t SAVE_CHECKSUM_ABS_ADDRESS = SAVE_CHECKSUM_SRAM_BANK * SRAM_BANK_SIZE + SAVE_CHECKSUM_ADDRESS - SRAM_START_ADDRESS;
constexpr uint8_t SAVE_BACKUP_CHECKSUM_SRAM_BANK = 0x00;
constexpr uint16_t SAVE_BACKUP_CHECKSUM_ADDRESS = 0xBF0D;
constexpr uint32_t SAVE_BACKUP_CHECKSUM_ABS_ADDRESS = SAVE_BACKUP_CHECKSUM_SRAM_BANK * SRAM_BANK_SIZE + SAVE_BACKUP_CHECKSUM_ADDRESS - SRAM_START_ADDRESS;

// ROM constants
constexpr uint16_t ROM0_START_ADDRESS = 0x0000;
constexpr uint16_t ROMX_START_ADDRESS = 0x4000;
constexpr uint8_t NUM_ROMX_BANKS = 127;
constexpr uint16_t ROMX_BANK_SIZE = 0x4000;

// VRAM constants
constexpr uint16_t VRAM_START_ADDRESS = 0x8000;
constexpr uint8_t NUM_VRAM_BANKS = 2;
constexpr uint16_t VRAM_BANK_SIZE = 0x2000;

// WRAM constants
constexpr uint16_t WRAM0_START_ADDRESS = 0xC000;
constexpr uint16_t WRAMX_START_ADDRESS = 0xD000;
constexpr uint8_t NUM_WRAM_BANKS = 7;
constexpr uint16_t WRAM_BANK_SIZE = 0x1000;

// ECHO RAM constants.
constexpr uint16_t ECHO_WRAM_START_ADDRESS = 0xE000;

// HRAM constants
constexpr uint16_t HRAM_START_ADDRESS = 0xFF80;
constexpr uint8_t NUM_HRAM_BANKS = 1;

// SAVE constants
constexpr uint32_t MIN_SAVE_SIZE = SRAM_BANK_SIZE * NUM_SRAM_BANKS;

// Symbol file locations
constexpr const char* VERSION_7_SYMBOL_FILE = "resources/version7/version7.sym";
constexpr const char* VERSION_8_SYMBOL_FILE = "resources/version8/version8.sym";

// ANSI escape codes
const std::string RED_TEXT = "\033[31m";
const std::string YELLOW_TEXT = "\033[33m";
const std::string RESET_TEXT = "\033[0m";

#endif
