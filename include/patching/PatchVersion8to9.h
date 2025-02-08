#ifndef PATCHVERSION8TO9_H
#define PATCHVERSION8TO9_H

#include "core/SaveBinary.h"

namespace patchVersion8to9Namespace {
	using namespace patchVersion8to9Namespace;
	constexpr int NUM_KEY_ITEMS_V9 = 0x26;
	constexpr uint16_t INVALID_EVENT_FLAG = -1;
	constexpr int NUM_EVENTS = 0x8ff;
	constexpr int PLAINBADGE = 2;
	constexpr int EVENT_BEAT_CANDELA = 0x596;
	constexpr int EVENT_BEAT_BLANCHE = 0x597;
	constexpr int EVENT_BEAT_SPARK = 0x598;
	constexpr std::pair<uint8_t, uint8_t> GOLDENROD_POKECOM_CENTER_1F = { 11, 24 };
	constexpr std::pair<uint8_t, uint8_t> PLAYERS_HOUSE_1F = { 24, 6 };
	constexpr std::pair<uint8_t, uint8_t> validPCWarpIDs[] = {
		{  1,  1 }, // OLIVINE_POKECENTER_1F
		{  2,  3 }, // MAHOGANY_POKECENTER_1F
		{  4,  3 }, // ECRUTEAK_POKECENTER_1F
		{  5,  6 }, // BLACKTHORN_POKECENTER_1F
		{  6,  1 }, // CINNABAR_POKECENTER_1F
		{  7,  4 }, // CERULEAN_POKECENTER_1F
		{  7,  8 }, // ROUTE_10_POKECENTER_1F
		{  8,  1 }, // AZALEA_POKECENTER_1F
		{ 10,  8 }, // VIOLET_POKECENTER_1F
		{ 10, 11 }, // ROUTE_32_POKECENTER_1F
		{ 11, 24 }, // GOLDENROD_POKECOM_CENTER_1F
		{ 12,  5 }, // VERMILION_POKECENTER_1F
		{ 14,  3 }, // ROUTE_3_POKECENTER_1F
		{ 14,  8 }, // PEWTER_POKECENTER_1F
		{ 16,  3 }, // INDIGO_PLATEAU_POKECENTER_1F
		{ 17, 12 }, // FUCHSIA_POKECENTER_1F
		{ 18,  6 }, // LAVENDER_POKECENTER_1F
		{ 19,  3 }, // SILVER_CAVE_POKECENTER_1F
		{ 21, 20 }, // CELADON_POKECENTER_1F
		{ 22,  5 }, // CIANWOOD_POKECENTER_1F
		{ 23, 11 }, // VIRIDIAN_POKECENTER_1F
		{ 25,  4 }, // SAFFRON_POKECENTER_1F
		{ 26,  6 }, // CHERRYGROVE_POKECENTER_1F
		{ 31,  8 }, // SHAMOUTI_POKECENTER_1F
		{ 36,  5 }, // SNOWTOP_POKECENTER_1F
	};

	// Converts a version 7 event flag to a version 8 event flag
	uint16_t mapV8EventFlagToV9(uint16_t v8);

	// converts a version 8 key item to a version 8 key item
	uint8_t mapV8KeyItemToV9(uint8_t v8);

	// bool patchVersion8to9 takes in arguments SaveBinary save7 and SaveBinary save8
	bool patchVersion8to9(SaveBinary& save8, SaveBinary& save9);
}


#endif