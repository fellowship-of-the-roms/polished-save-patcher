#ifndef PATCHVERSION8TO9_H
#define PATCHVERSION8TO9_H

#include "core/SaveBinary.h"

namespace patchVersion8to9Namespace {
	using namespace patchVersion8to9Namespace;
	constexpr int NUM_KEY_ITEMS_V9 = 0x26;
	constexpr uint16_t INVALID_EVENT_FLAG = -1;
	constexpr int NUM_EVENTS = 0x8ff;

	// Converts a version 7 event flag to a version 8 event flag
	uint16_t mapV8EventFlagToV9(uint16_t v8);

	// converts a version 8 key item to a version 8 key item
	uint8_t mapV8KeyItemToV9(uint8_t v8);

	// bool patchVersion8to9 takes in arguments SaveBinary save7 and SaveBinary save8
	bool patchVersion8to9(SaveBinary& save8, SaveBinary& save9);
}


#endif