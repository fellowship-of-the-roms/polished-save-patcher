#ifndef PATCHVERSION9TO10_H
#define PATCHVERSION9TO10_H

#include "core/SaveBinary.h"

namespace patchVersion9to10Namespace {
	using namespace patchVersion9to10Namespace;
	constexpr uint16_t INVALID_EVENT_FLAG = -1;
	constexpr int NUM_EVENTS = 0x8ff;
	constexpr uint8_t TEXT_DELAY_MASK = 0x03;
	constexpr uint8_t ralphName[] = { 0x91, 0xA0, 0xAB, 0xAF, 0xA7, 0x53 };

	// Converts a version 9 event flag to a version 10 event flag
	uint16_t mapV9EventFlagToV10(uint16_t v9);

	bool patchVersion9to10(SaveBinary& save9, SaveBinary& save10);
}

#endif