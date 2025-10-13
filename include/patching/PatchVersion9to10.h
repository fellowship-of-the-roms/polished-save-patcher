#ifndef PATCHVERSION9TO10_H
#define PATCHVERSION9TO10_H

#include "core/SaveBinary.h"

namespace patchVersion9to10Namespace {
	using namespace patchVersion9to10Namespace;
	constexpr uint16_t INVALID_EVENT_FLAG = -1;
	constexpr int NUM_EVENTS = 0x8ff;
	constexpr uint8_t TEXT_DELAY_MASK = 0x03;
	constexpr uint8_t ralphName[] = { 0x91, 0xA0, 0xAB, 0xAF, 0xA7, 0x53 };
	constexpr int MAIL_MSG_LENGTH = 0x20;
	constexpr int PLAYER_NAME_LENGTH = 8;
	constexpr int PARTY_LENGTH = 6;
	constexpr int MAILBOX_CAPACITY = 10;
	constexpr int NO_EXP_OPT = 2;
	constexpr int RESET_INIT_OPTS = 7;
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

#pragma pack(push, 1)
	struct mailmsg_struct_v10 {
		uint8_t message[MAIL_MSG_LENGTH];
		uint8_t message_end;
		uint8_t author[PLAYER_NAME_LENGTH];
		uint16_t nationality;
		uint16_t author_id;
		uint8_t species;
		uint8_t type;
	};
#pragma pack(pop)

	// Converts a version 9 event flag to a version 10 event flag
	uint16_t mapV9EventFlagToV10(uint16_t v9);

	bool patchVersion9to10(SaveBinary& save9, SaveBinary& save10);

	mailmsg_struct_v10 convertMailmsgV9toV10(const mailmsg_struct_v10& mailmsg);

	std::vector<uint8_t> decodeV9ToChar(const uint8_t* data, size_t length);
}

#endif