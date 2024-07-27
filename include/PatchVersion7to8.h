#ifndef PATCHVERSION7TO8_H
#define PATCHVERSION7TO8_H

#include "SaveBinary.h"
#include "SymbolDatabase.h"
#include "PatcherConstants.h"
#include <iostream>

// converts a version 7 key item to a version 8 key item
uint8_t mapV7KeyItemToV8(uint8_t v7);

// converts a version 7 item to a version 8 item
uint8_t mapV7ItemToV8(uint8_t v7);

// converts a version 7 event flag to a version 8 event flag
uint16_t mapV7EventFlagToV8(uint16_t v7);

// converts a version 7 landmark to a version 8 landmark
uint8_t mapV7LandmarkToV8(uint8_t v7);

// converts a version 7 spawn to a version 8 spawn
uint8_t mapV7SpawnToV8(uint8_t v7);

// converts a version 7 pokemon to a version 8 pokemon
uint16_t mapV7PkmnToV8(uint16_t v7);

struct TupleHash {
	template <typename T>
	std::size_t operator()(const T& tuple) const {
		auto [first, second] = tuple;
		return std::hash<unsigned char>()(first) ^ std::hash<unsigned char>()(second);
	}
};

std::tuple<uint8_t, uint8_t> mapv7toV8(uint8_t v7_group, uint8_t v7_map);

// Converts a version 7 (uint16_t species, uint8_t form) tuple to a version 8 unint16_t extspecies
// If the species is not in the map, it returns 0xFFFF; We only care about species that have a form
uint16_t mapV7SpeciesFormToV8Extspecies(uint16_t species, uint8_t form);

// converts a version 7 magikarp form to a version 8 magikarp form
uint8_t mapV7MagikarpFormToV8(uint8_t v7);

// converts a version 7 theme to a version 8 theme
uint8_t mapV7ThemeToV8(uint8_t v7);

uint16_t calculateNewboxChecksum(const SaveBinary& save, uint32_t startAddress);

uint16_t extractStoredNewboxChecksum(const SaveBinary& save, uint32_t startAddress);

void writeNewboxChecksum(SaveBinary& save, uint32_t startAddress);

void writeDefaultBoxName(SaveBinary::Iterator& it, int boxNum);

void migrateBoxData(SaveBinary::Iterator &it7, SaveBinary::Iterator &it8, const SymbolDatabase &sym7, const SymbolDatabase &sym8, const std::string &prefix);

void clearBox(SaveBinary::Iterator &it8, const SymbolDatabase &sym8, const std::string &boxName, int numEntries);

// bool patchVersion7to8 takes in arguments SaveBinary save7 and SaveBinary save8
bool patchVersion7to8(SaveBinary& save7, SaveBinary& save8);

#endif
