#ifndef PATCHVERSION7TO8_H
#define PATCHVERSION7TO8_H

#include "SaveBinary.h"
#include "SymbolDatabase.h"
#include "PatcherConstants.h"
#include <iostream>

// converts a version 7 key item to a version 8 key item
uint8_t mapv7KeyItemtoV8(uint8_t v7);

// converts a version 7 item to a version 8 item
uint8_t mapv7ItemtoV8(uint8_t v7);

// converts a version 7 event flag to a version 8 event flag
uint16_t mapv7EventFlagtoV8(uint16_t v7);

// converts a version 7 landmark to a version 8 landmark
uint8_t mapv7LandmarktoV8(uint8_t v7);

// converts a version 7 spawn to a version 8 spawn
uint8_t mapv7SpawntoV8(uint8_t v7);

// converts a version 7 pokemon to a version 8 pokemon
uint16_t mapv7PkmntoV8(uint16_t v7);

struct TupleHash {
	template <typename T>
	std::size_t operator()(const T& tuple) const {
		auto [first, second] = tuple;
		return std::hash<unsigned char>()(first) ^ std::hash<unsigned char>()(second);
	}
};

std::tuple<uint8_t, uint8_t> mapv7toV8(uint8_t v7_group, uint8_t v7_map);

// converts a version 7 magikarp form to a version 8 magikarp form
uint8_t mapv7MagikarpFormToV8(uint8_t v7);

// converts a version 7 theme to a version 8 theme
uint8_t mapv7ThemeToV8(uint8_t v7);

uint16_t calculateNewboxChecksum(const SaveBinary& save, uint32_t startAddress);

uint16_t extractStoredNewboxChecksum(const SaveBinary& save, uint32_t startAddress);

void writeNewboxChecksum(SaveBinary& save, uint32_t startAddress);

bool inPokecenter(uint8_t v7_group, uint8_t v7_map);

// bool patchVersion7to8 takes in arguments SaveBinary save7 and SaveBinary save8
bool patchVersion7to8(SaveBinary& save7, SaveBinary& save8);

#endif
