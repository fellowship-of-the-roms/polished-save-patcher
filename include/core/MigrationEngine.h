#ifndef MIGRATION_ENGINE_H
#define MIGRATION_ENGINE_H

#include "core/MigrationSchema.h"
#include "core/SaveBinary.h"
#include "core/SymbolDatabase.h"
#include "core/CommonPatchFunctions.h"

// Validates both main and backup save checksums.
// Returns true if both checksums match, false otherwise.
bool validateChecksums(
	const SaveBinary& save,
	const SymbolDatabase& sym
);

// Checks that the player is in the PKMN Center 2nd Floor.
// Returns true if the location matches, false otherwise.
bool checkPlayerInPokemonCenter2F(
	SaveBinary::Iterator& it,
	const SymbolDatabase& sym
);

// Remaps event flags from source to destination save using the given mapping table.
// Clears destination flags first, then sets each mapped flag.
void remapEventFlags(
	SaveBinary::Iterator& itSrc,
	SaveBinary::Iterator& itDst,
	const SymbolDatabase& symSrc,
	const SymbolDatabase& symDst,
	const FlagMapping* mappings,
	size_t numMappings,
	int numEvents,
	SourceDest& sd
);

// Remaps key items from source to destination save using the given mapping table.
// Copies and clears the destination key item space first, then applies the mapping.
void remapKeyItems(
	SaveBinary::Iterator& itSrc,
	SaveBinary::Iterator& itDst,
	const SymbolDatabase& symSrc,
	const SymbolDatabase& symDst,
	const ItemMapping* mappings,
	size_t numMappings,
	int maxKeyItems,
	SourceDest& sd
);

// Validates the player's previous map (warp destination) against a list of
// valid Pokemon Center locations. If invalid, resets to a fallback location
// based on badge status.
void validateAndFixWarp(
	SaveBinary::Iterator& itDst,
	const SymbolDatabase& symDst,
	const WarpValidationConfig& config
);

// Resets map scene script and callback counts/pointers to 0.
// This prevents the game from running map scripts on load after patching.
void resetMapScripts(
	SaveBinary::Iterator& itDst,
	const SymbolDatabase& symDst
);

// Writes the new save version, copies game data to backup, and recalculates
// both main and backup checksums. This is the final step of every migration.
void finalizeSave(
	SaveBinary& saveDst,
	const SymbolDatabase& symDst,
	uint16_t newVersion,
	SourceDest& sd
);

#endif // MIGRATION_ENGINE_H
