#ifndef PATCHVERSION7TO8_H
#define PATCHVERSION7TO8_H

#include "SaveBinary.h"
#include "SymbolDatabase.h"
#include "PatcherConstants.h"
#include "CommonPatchFunctions.h"
#include <iostream>

namespace {
	constexpr int NUM_OBJECT_STRUCTS = 13;
	constexpr int OBJECT_PALETTE_V7 = 0x06;
	constexpr int OBJECT_PALETTE_V8 = 0x06;
	constexpr int OBJECT_PAL_INDEX_V8 = 0x21;
	constexpr int OBJECT_LENGTH_V7 = 0x21;
	constexpr int OBJECT_LENGTH_V8 = 0x22;
	constexpr int NUM_KEY_ITEMS_V7 = 0x1D;
	constexpr int NUM_KEY_ITEMS_V8 = 0x24;
	constexpr int NUM_APRICORNS = 0x07;
	constexpr int NUM_EVENTS = 0x8ff;
	constexpr int NUM_FRUIT_TREES_V7 = 0x23;
	constexpr int NUM_LANDMARKS_V7 = 0x90;
	constexpr int NUM_LANDMARKS_V8 = 0x91;
	constexpr int CONTACT_LIST_SIZE_V7 = 30;
	constexpr int NUM_PHONE_CONTACTS_V8 = 0x25;
	constexpr int NUM_SPAWNS_V7 = 30;
	constexpr int NUM_SPAWNS_V8 = 34;
	constexpr int PARTYMON_STRUCT_LENGTH = 0x30;
	constexpr int PARTY_LENGTH = 6;
	constexpr int PLAYER_NAME_LENGTH = 8;
	constexpr int MON_NAME_LENGTH = 11;
	constexpr uint8_t EXTSPECIES_MASK = 0b00100000;
	constexpr uint8_t FORM_MASK = 0b00011111;
	constexpr int MON_EXTSPECIES = 0x15;
	constexpr int MON_EXTSPECIES_F = 5;
	constexpr uint8_t CAUGHT_BALL_MASK = 0b00011111;
	constexpr int MON_ITEM = 0x01;
	constexpr int MON_FORM = 0x15;
	constexpr int MON_CAUGHTBALL = 0x1c;
	constexpr int MON_CAUGHTLOCATION = 0x1e;
	constexpr int NUM_POKEMON_V7 = 0xfe;
	constexpr int MONDB_ENTRIES_V7 = 167;
	constexpr int MONDB_ENTRIES_A_V8 = 167;
	constexpr int MONDB_ENTRIES_B_V8 = 28;
	constexpr int MONDB_ENTRIES_C_V8 = 12;
	constexpr int MONDB_ENTRIES_V8 = MONDB_ENTRIES_A_V8 + MONDB_ENTRIES_B_V8 + MONDB_ENTRIES_C_V8;
	constexpr int SAVEMON_STRUCT_LENGTH = 0x31;
	constexpr int MONS_PER_BOX = 20;
	constexpr int MIN_MONDB_SLACK = 10;
	constexpr int NUM_BOXES_V7 = (MONDB_ENTRIES_V7 * 2 - MIN_MONDB_SLACK) / MONS_PER_BOX;
	constexpr int NUM_BOXES_V8 = (MONDB_ENTRIES_V8 * 2 - MIN_MONDB_SLACK) / MONS_PER_BOX;
	constexpr int BOX_NAME_LENGTH = 9;
	constexpr int NEWBOX_SIZE = MONS_PER_BOX + ((MONS_PER_BOX + 7) / 8) + BOX_NAME_LENGTH + 1;
	constexpr int SAVEMON_EXTSPECIES = 0x15;
	constexpr int SAVEMON_ITEM = 0x01;
	constexpr int SAVEMON_FORM = 0x15;
	constexpr int SAVEMON_CAUGHTBALL = 0x19;
	constexpr int SAVEMON_CAUGHTLOCATION = 0x1b;
	constexpr int BATTLETOWER_PARTYDATA_SIZE = 6;
	constexpr int NUM_HOF_TEAMS_V8 = 10;
	constexpr int HOF_MON_LENGTH = 1 + 2 + 2 + 1 + (MON_NAME_LENGTH - 1); // species, id, personality, level, nick
	constexpr int HOF_LENGTH = 1 + HOF_MON_LENGTH * PARTY_LENGTH + 1; // win count, party, terminator
	constexpr int HOF_MON_EXTSPECIES = 0x04;
	constexpr uint16_t INVALID_SPECIES = -1;
	constexpr uint16_t INVALID_EVENT_FLAG = -1;
	constexpr uint8_t MAGIKARP_V8 = 0x81;
	constexpr uint8_t GYARADOS_V8 = 0x82;
	constexpr uint8_t GYARADOS_RED_FORM_V7 = 0x11;
	constexpr uint8_t GYARADOS_RED_FORM_V8 = 0x15;
	constexpr int AFFECTION_OPT = 5;
	constexpr int RESET_INIT_OPTS = 7;
}

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

// converts a version 7 Pokémon index to a version 8 Pokémon index
uint16_t mapV7PkmnToV8(uint16_t v7);

// struct for hashing tuples
struct TupleHash {
	template <typename T>
	std::size_t operator()(const T& tuple) const {
		auto [first, second] = tuple;
		return std::hash<unsigned char>()(first) ^ std::hash<unsigned char>()(second);
	}
};

// converts a version 7 (uint8_t group, uint8_t map) tuple to a version 8 (uint8_t group, uint8_t map) tuple
std::tuple<uint8_t, uint8_t> mapv7toV8(uint8_t v7_group, uint8_t v7_map);

// Converts a version 7 (uint16_t species, uint8_t form) tuple to a version 8 unint16_t extspecies
uint16_t mapV7SpeciesFormToV8Extspecies(uint16_t species, uint8_t form);

// converts a version 7 magikarp form to a version 8 magikarp form
uint8_t mapV7MagikarpFormToV8(uint8_t v7);

// converts a version 7 theme to a version 8 theme
uint8_t mapV7ThemeToV8(uint8_t v7);

// Calculate the newbox checksum for the given mon
uint16_t calculateNewboxChecksum(const SaveBinary& save, uint32_t startAddress);

// Extract the stored newbox checksum for the given mon
uint16_t extractStoredNewboxChecksum(const SaveBinary& save, uint32_t startAddress);

// Write the newbox checksum for the given mon
void writeNewboxChecksum(SaveBinary& save, uint32_t startAddress);

// Writes the default box name for the given box number
void writeDefaultBoxName(SaveBinary::Iterator& it, int boxNum);

// Migrate the newbox box data from version 7 to version 8
void migrateBoxData(SourceDest &sd, const std::string &prefix);

// converts the species and form for a given mon
void convertSpeciesAndForm(SourceDest &sd, uint32_t base_address, int i, int struct_length, int extspecies_offset, uint16_t species, std::vector<uint16_t> &seen_mons, std::vector<uint16_t> &caught_mons);

// converts the item for a given mon
void convertItem(SourceDest &sd, uint32_t base_address, int i, int struct_length, int item_offset, uint8_t item);

// converts the caught location for a given mon
void convertCaughtLocation(SourceDest &sd, uint32_t base_address, int i, int struct_length, int caught_location_offset, uint8_t caught_location);

// converts the caught ball for a given mon
void convertCaughtBall(SourceDest &sd, uint32_t base_address, int i, int struct_length, int caught_ball_offset, uint8_t caught_ball);

// bool patchVersion7to8 takes in arguments SaveBinary save7 and SaveBinary save8
bool patchVersion7to8(SaveBinary& save7, SaveBinary& save8);

#endif
