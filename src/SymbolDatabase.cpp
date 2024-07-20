// include "SymbolDatabase.h"
#include "SymbolDatabase.h"
#include "PatcherConstants.h"
#include "Logging.h"
#include <fstream>
#include <iostream>
#include <regex>

// Constructor
SymbolDatabase::SymbolDatabase(const std::string& symbolFilePath) {
	// symbols are in the format of:
	// two_digit_hex_bank:four_digit_hex_address symbol_name
	// lines that do not match the above format should be ignored.

	std::ifstream file(symbolFilePath);
	if (!file.is_open()) {
		js_error <<  "Failed to open symbol file: " << symbolFilePath << std::endl;
		return;
	}

	std::regex symbolRegex(R"((\w{2}):(\w{4})\s([A-Za-z0-9_.]+))");
	std::smatch match;

	std::string line;
	while (std::getline(file, line)) {
		if (!std::regex_match(line, match, symbolRegex)) {
			continue;
		}

		Symbol symbol;
		symbol.bank = std::stoi(match[1].str(), nullptr, 16);
		symbol.address = std::stoi(match[2].str(), nullptr, 16);
		symbol.name = match[3].str();

		m_symbols[symbol.name] = symbol;
	}
}

// Destructor
SymbolDatabase::~SymbolDatabase() {
}

// Get the symbol by name
const SymbolDatabase::Symbol* SymbolDatabase::getSymbol(const std::string& name) const {
	auto it = m_symbols.find(name);
	if (it == m_symbols.end()) {
		return nullptr;
	}
	return &it->second;
}

// Returns if symbol by name is within rom
bool SymbolDatabase::isROM(const std::string& name) const {
	// ROM is from 0x0000 to 0x7FFF
	const Symbol* symbol = getSymbol(name);
	if (symbol == nullptr) {
		return false;
	}
	return symbol->address < VRAM_START_ADDRESS;
}

// Returns if symbol by name is within vram
bool SymbolDatabase::isVRAM(const std::string& name) const {
	// VRAM is from 0x8000 to 0x9FFF
	const Symbol* symbol = getSymbol(name);
	if (symbol == nullptr) {
		return false;
	}
	return symbol->address >= VRAM_START_ADDRESS && symbol->address < SRAM_START_ADDRESS;
}

// Returns if symbol by name is within sram
bool SymbolDatabase::isSRAM(const std::string& name) const {
	// SRAM is from 0xA000 to 0xBFFF
	const Symbol* symbol = getSymbol(name);
	if (symbol == nullptr) {
		return false;
	}
	return symbol->address >= SRAM_START_ADDRESS && symbol->address < WRAM0_START_ADDRESS;
}

// Returns if symbol by name is within wram
bool SymbolDatabase::isWRAM(const std::string& name) const {
	// WRAM is from 0xC000 to 0xDFFF
	const Symbol* symbol = getSymbol(name);
	if (symbol == nullptr) {
		return false;
	}
	return symbol->address >= WRAM0_START_ADDRESS && symbol->address < ECHO_WRAM_START_ADDRESS;
}

// Returns if symbol by name is within hram
bool SymbolDatabase::isHRAM(const std::string& name) const {
	// HRAM is from 0xFF80 to 0xFFFF
	const Symbol* symbol = getSymbol(name);
	if (symbol == nullptr) {
		return false;
	}
	return symbol->address >= HRAM_START_ADDRESS;
}

// Returns abosolute sram address (up to 2MiB) of symbol by name
uint32_t SymbolDatabase::getSRAMAddress(const std::string& name) const {
	// check is symbol is in SRAM
	if (!isSRAM(name)) {
		return 0;
	}
	// absolute address is bank * 0x2000 + address - 0xA000
	const Symbol* symbol = getSymbol(name);
	return symbol->bank * SRAM_BANK_SIZE + symbol->address - SRAM_START_ADDRESS;
}

uint32_t SymbolDatabase::getPlayerDataAddress(const std::string& wram_symbol_name) const {
	// check if symbol is in WRAM
	if (!isWRAM(wram_symbol_name)) {
		js_error <<  "Symbol " << wram_symbol_name << " is not in WRAM" << std::endl;
		return 0;
	}
	// we need to find the equivalent SRAM address of the WRAM symbol
	// this is done by adding the distance between the wram symbol and wPlayerData to sPlayerData
	
	// get the address of the wPlayerData symbol
	const Symbol* wPlayerData = getSymbol("wPlayerData");
	if (wPlayerData == nullptr) {
		return 0;
	}
	// calculate the distance between wram_symbol and wPlayerData error if result is negative
	const Symbol* wram_symbol = getSymbol(wram_symbol_name);
	if (wram_symbol == nullptr) {
		return 0;
	}
	int32_t distance = wram_symbol->address - wPlayerData->address;
	if (distance < 0) {
		js_error <<  "Symbol " << wram_symbol_name << " is before wPlayerData" << std::endl;
		return 0;
	}

	// calculate the absolute address of the player data
	return getSRAMAddress("sPlayerData") + distance;
}

uint32_t SymbolDatabase::getMapDataAddress(const std::string& wram_symbol_name) const {
	// check if symbol is in WRAM
	if (!isWRAM(wram_symbol_name)) {
		js_error <<  "Symbol " << wram_symbol_name << " is not in WRAM" << std::endl;
		return 0;
	}
	// we need to find the equivalent SRAM address of the WRAM symbol
	// this is done by adding the distance between the wram symbol and wCurMapData to sMapData
	
	// get the address of the wMapData symbol
	const Symbol* wMapData = getSymbol("wCurMapData");
	if (wMapData == nullptr) {
		js_error <<  "Symbol wCurMapData not found" << std::endl;
		return 0;
	}
	// calculate the distance between wram_symbol and wMapData error if result is negative
	const Symbol* wram_symbol = getSymbol(wram_symbol_name);
	if (wram_symbol == nullptr) {
		return 0;
	}
	int32_t distance = wram_symbol->address - wMapData->address;
	if (distance < 0) {
		js_error <<  "Symbol " << wram_symbol_name << " is before wCurMapData" << std::endl;
		return 0;
	}

	// calculate the absolute address of the map data
	return getSRAMAddress("sMapData") + distance;
}

uint32_t SymbolDatabase::getPokemonDataAddress(const std::string& wram_symbol_name) const {
	// check if symbol is in WRAM
	if (!isWRAM(wram_symbol_name)) {
		js_error <<  "Symbol " << wram_symbol_name << " is not in WRAM" << std::endl;
		return 0;
	}
	// we need to find the equivalent SRAM address of the WRAM symbol
	// this is done by adding the distance between the wram symbol and wPokemonData to sPokemonData
	
	// get the address of the wPokemonData symbol
	const Symbol* wPokemonData = getSymbol("wPokemonData");
	if (wPokemonData == nullptr) {
		return 0;
	}
	// calculate the distance between wram_symbol and wPokemonData error if result is negative
	const Symbol* wram_symbol = getSymbol(wram_symbol_name);
	if (wram_symbol == nullptr) {
		return 0;
	}
	int32_t distance = wram_symbol->address - wPokemonData->address;
	if (distance < 0) {
		js_error <<  "Symbol " << wram_symbol_name << " is before wPokemonData" << std::endl;
		return 0;
	}

	// calculate the absolute address of the pokemon data
	return getSRAMAddress("sPokemonData") + distance;
}

