#ifndef MIGRATION_SCHEMA_H
#define MIGRATION_SCHEMA_H

#include <cstdint>
#include <cstddef>
#include <utility>

// A mapping from a source event flag index to a destination event flag index.
// Used to remap event flag bits between save versions.
struct FlagMapping {
	uint16_t src;
	uint16_t dst;
};

// Sentinel value indicating an event flag has no mapping in the target version.
constexpr uint16_t INVALID_EVENT_FLAG = static_cast<uint16_t>(-1);

// A mapping from a source item ID to a destination item ID.
// Used for key item and regular item remapping between save versions.
struct ItemMapping {
	uint8_t src;
	uint8_t dst;
};

// A map location identified by group and map number.
struct MapLocation {
	uint8_t group;
	uint8_t map;
};

// Configuration for warp ID validation and fallback behavior.
// When a save is patched, the player's previous map (warp destination)
// is validated against a list of known Pokemon Center 1F locations.
// If invalid, it falls back based on whether the player has a badge.
struct WarpValidationConfig {
	const MapLocation* validPCWarps;
	size_t numValidPCWarps;
	int badgeBitIndex;               // Badge bit to check (e.g. PLAINBADGE)
	MapLocation badgeFallback;       // Fallback if player has the badge
	uint8_t badgeFallbackWarpNum;
	MapLocation noBadgeFallback;     // Fallback if player lacks the badge
	uint8_t noBadgeFallbackWarpNum;
};

#endif // MIGRATION_SCHEMA_H
