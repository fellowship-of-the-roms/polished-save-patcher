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