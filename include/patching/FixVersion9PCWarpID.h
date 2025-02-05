#pragma once
#ifndef FIXVERSION9PCWARPID_H
#define FIXVERSION9PCWARPID_H

#include "core/SaveBinary.h"
#include "core/SymbolDatabase.h"
#include "core/PatcherConstants.h"
#include "core/CommonPatchFunctions.h"

namespace fixVersion9PCWarpIDNamespace {
	using namespace fixVersion9PCWarpIDNamespace;

	constexpr int PLAINBADGE = 2;
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

	bool fixVersion9PCWarpID(SaveBinary& oldsave, SaveBinary& patchedsave);
}

#endif
