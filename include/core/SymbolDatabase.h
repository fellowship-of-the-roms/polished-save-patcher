#ifndef SYMBOLDATABASE_H
#define SYMBOLDATABASE_H

#include <string>
#include <vector>
#include <unordered_map>

// This class is used to store the symbols that are used in the save patcher.
// each symbol consists of bank, address, and name.
// it is initiated by inputing the path to the symbol file.

class SymbolDatabase {
public:
	struct Symbol {
		uint8_t bank;
		uint16_t address;
		std::string name;
	};

	// Constructor
	SymbolDatabase(const std::string& symbolFilePath);
	SymbolDatabase(const unsigned char* buffer, size_t length);
	// Destructor
	~SymbolDatabase();
	// Get the symbol by name
	const Symbol* getSymbol(const std::string& name) const;
	// Returns if symbol by name is within rom
	bool isROM(const std::string& name) const;
	// Returns if symbol by name is within vram
	bool isVRAM(const std::string& name) const;
	// Returns if symbol by name is within sram
	bool isSRAM(const std::string& name) const;
	// Returns if symbol by name is within wram
	bool isWRAM(const std::string& name) const;
	// Returns if symbol by name is within hram
	bool isHRAM(const std::string& name) const;
	// Returns abosolute sram address (up to 2MiB) of symbol by name
	uint32_t getSRAMAddress(const std::string& name) const;

	// Todo: These may get moved to CommonPatchFunctions.h
	// Returns the distance of the wram symbol from wOptions and adds it to the address of sOptions
	uint32_t getOptionsAddress(const std::string& wram_symbol_name) const;
	// Returns the distance of the wram symbol from wPlayerData and adds it to the address of sPlayerData
	uint32_t getPlayerDataAddress(const std::string& wram_symbol_name) const;
	// Returns the distance of the wram symbol from wMapData and adds it to the address of sMapData
	uint32_t getMapDataAddress(const std::string& wram_symbol_name) const;
	// Returns the distance of the wram symbol from wPokemonData and adds it to the address of sPokemonData
	uint32_t getPokemonDataAddress(const std::string& wram_symbol_name) const;

private:
	std::unordered_map<std::string, Symbol> m_symbols;
	std::string decompressGzip(const std::string& compressedData);
	void processCompressedString(std::string& compressedData);
};

#endif // SYMBOLDATABASE_H
