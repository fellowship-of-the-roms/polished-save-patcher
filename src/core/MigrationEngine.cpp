#include "core/MigrationEngine.h"
#include "core/Logging.h"
#include "core/PatcherConstants.h"
#include <unordered_map>

bool validateChecksums(
	const SaveBinary& save,
	const SymbolDatabase& sym
) {
	// Verify main checksum
	uint16_t save_checksum = save.getWord(SAVE_CHECKSUM_ABS_ADDRESS);
	uint16_t calculated_checksum = calculateSaveChecksum(save, sym.getSRAMAddress("sGameData"), sym.getSRAMAddress("sGameDataEnd"));
	if (save_checksum != calculated_checksum) {
		js_error << "Checksum mismatch! Expected: " << std::hex << calculated_checksum << ", got: " << save_checksum << std::endl;
		return false;
	}

	// Verify backup checksum
	uint16_t backup_checksum = save.getWord(SAVE_BACKUP_CHECKSUM_ABS_ADDRESS);
	uint16_t calculated_backup_checksum = calculateSaveChecksum(save, sym.getSRAMAddress("sBackupGameData"), sym.getSRAMAddress("sBackupGameDataEnd"));
	if (backup_checksum != calculated_backup_checksum) {
		js_error << "Backup checksum mismatch! Expected: " << std::hex << calculated_backup_checksum << ", got: " << backup_checksum << std::endl;
		return false;
	}

	return true;
}

bool checkPlayerInPokemonCenter2F(
	SaveBinary::Iterator& it,
	const SymbolDatabase& sym
) {
	uint8_t map_group = it.getByte(sym.getMapDataAddress("wMapGroup"));
	it.next();
	uint8_t map_num = it.getByte();
	if (map_group != MON_CENTER_2F_GROUP || map_num != MON_CENTER_2F_MAP) {
		js_error << "Player is not in the PKMN Center 2nd Floor. Go to where you heal in game, and head upstairs. Then re-save your game and try again." << std::endl;
		return false;
	}
	return true;
}

void remapEventFlags(
	SaveBinary::Iterator& itSrc,
	SaveBinary::Iterator& itDst,
	const SymbolDatabase& symSrc,
	const SymbolDatabase& symDst,
	const FlagMapping* mappings,
	size_t numMappings,
	int numEvents,
	SourceDest& sd
) {
	// Build lookup table from source flag index to destination flag index
	std::unordered_map<uint16_t, uint16_t> flagMap;
	flagMap.reserve(numMappings);
	for (size_t i = 0; i < numMappings; i++) {
		flagMap[mappings[i].src] = mappings[i].dst;
	}

	// Clear destination event flags
	js_info << "Clearing destination event flags..." << std::endl;
	clearDataBlock(sd, symDst.getPlayerDataAddress("wEventFlags"), flag_array(numEvents));

	// Remap each set source flag to its destination
	js_info << "Remapping event flags..." << std::endl;
	for (int i = 0; i < numEvents; i++) {
		if (isFlagBitSet(itSrc, symSrc.getPlayerDataAddress("wEventFlags"), i)) {
			auto it = flagMap.find(static_cast<uint16_t>(i));
			if (it != flagMap.end()) {
				uint16_t dstIndex = it->second;
				if (dstIndex != INVALID_EVENT_FLAG) {
					setFlagBit(itDst, symDst.getPlayerDataAddress("wEventFlags"), dstIndex);
					if (static_cast<uint16_t>(i) != dstIndex) {
						js_info << "Event flag " << std::dec << i << " mapped to " << dstIndex << std::endl;
					}
				}
			} else {
				js_warning << "Event flag " << i << " not found in mapping table." << std::endl;
			}
		}
	}
}

void remapKeyItems(
	SaveBinary::Iterator& itSrc,
	SaveBinary::Iterator& itDst,
	const SymbolDatabase& symSrc,
	const SymbolDatabase& symDst,
	const ItemMapping* mappings,
	size_t numMappings,
	int maxKeyItems,
	SourceDest& sd
) {
	// Build lookup table
	std::unordered_map<uint8_t, uint8_t> itemMap;
	itemMap.reserve(numMappings);
	for (size_t i = 0; i < numMappings; i++) {
		itemMap[mappings[i].src] = mappings[i].dst;
	}

	// Clear destination key items space
	js_info << "Clearing destination wKeyItems space..." << std::endl;
	clearDataBlock(sd, symDst.getPlayerDataAddress("wKeyItems"), symDst.getPlayerDataAddress("wKeyItemsEnd") - symDst.getPlayerDataAddress("wKeyItems"));

	// Copy source key items to destination
	js_info << "Copying source wKeyItems to destination wKeyItems..." << std::endl;
	copyDataBlock(sd, symSrc.getPlayerDataAddress("wKeyItems"), symDst.getPlayerDataAddress("wKeyItems"), symSrc.getPlayerDataAddress("wKeyItemsEnd") - symSrc.getPlayerDataAddress("wKeyItems"));

	// Remap key items in-place
	js_info << "Remapping wKeyItems..." << std::endl;
	itSrc.seek(symSrc.getPlayerDataAddress("wKeyItems"));
	itDst.seek(symDst.getPlayerDataAddress("wKeyItems"));
	for (int i = 0; i < maxKeyItems; i++) {
		uint8_t srcItem = itSrc.getByte();
		if (srcItem == 0x00) {
			break;
		}
		auto it = itemMap.find(srcItem);
		if (it != itemMap.end()) {
			uint8_t dstItem = it->second;
			if (dstItem == 0xFF) {
				js_error << "Key item " << std::hex << static_cast<int>(srcItem) << " not found in destination key item list." << std::endl;
			} else if (srcItem != dstItem) {
				itDst.setByte(dstItem);
				js_info << "Key item " << std::hex << static_cast<int>(srcItem) << " converted to " << std::hex << static_cast<int>(dstItem) << std::endl;
			}
		} else {
			js_error << "Key item " << std::hex << static_cast<int>(srcItem) << " not found in mapping table." << std::endl;
		}
		itSrc.next();
		itDst.next();
	}
}

void validateAndFixWarp(
	SaveBinary::Iterator& itDst,
	const SymbolDatabase& symDst,
	const WarpValidationConfig& config
) {
	uint8_t prev_map_group = itDst.getByte(symDst.getMapDataAddress("wBackupMapGroup"));
	uint8_t prev_map_num = itDst.getByte(symDst.getMapDataAddress("wBackupMapNumber"));

	bool valid_prev_map = false;
	for (size_t i = 0; i < config.numValidPCWarps; i++) {
		if (prev_map_group == config.validPCWarps[i].group && prev_map_num == config.validPCWarps[i].map) {
			valid_prev_map = true;
			break;
		}
	}

	if (!valid_prev_map) {
		js_warning << "Player's previous map is not a valid PKMN Center Warp ID! We will reset it to one." << std::endl;
		if (isFlagBitSet(itDst, symDst.getPlayerDataAddress("wJohtoBadges"), config.badgeBitIndex)) {
			js_warning << "Player has the PLAINBADGE, the stairs will now take you to Goldenrod PKMN Center." << std::endl;
			itDst.setByte(symDst.getMapDataAddress("wBackupWarpNumber"), config.badgeFallbackWarpNum);
			itDst.setByte(symDst.getMapDataAddress("wBackupMapGroup"), config.badgeFallback.group);
			itDst.setByte(symDst.getMapDataAddress("wBackupMapNumber"), config.badgeFallback.map);
		} else {
			js_warning << "Player does not have the PLAINBADGE, the stairs will now warp you to your house." << std::endl;
			itDst.setByte(symDst.getMapDataAddress("wBackupWarpNumber"), config.noBadgeFallbackWarpNum);
			itDst.setByte(symDst.getMapDataAddress("wBackupMapGroup"), config.noBadgeFallback.group);
			itDst.setByte(symDst.getMapDataAddress("wBackupMapNumber"), config.noBadgeFallback.map);
		}
	} else {
		js_info << "Player's previous map is a valid PKMN Center warp ID. No need to fix the warp ID." << std::endl;
	}
}

void resetMapScripts(
	SaveBinary::Iterator& itDst,
	const SymbolDatabase& symDst
) {
	js_info << "Set wCurMapSceneScriptCount and wCurMapCallbackCount to 0..." << std::endl;
	itDst.seek(symDst.getPlayerDataAddress("wCurMapSceneScriptCount"));
	itDst.setByte(0);
	itDst.seek(symDst.getPlayerDataAddress("wCurMapCallbackCount"));
	itDst.setByte(0);
	js_info << "Set wCurMapSceneScriptPointer to 0..." << std::endl;
	itDst.seek(symDst.getPlayerDataAddress("wCurMapSceneScriptPointer"));
	itDst.setWord(0);
}

void finalizeSave(
	SaveBinary& saveDst,
	const SymbolDatabase& symDst,
	uint16_t newVersion,
	SourceDest& sd
) {
	// Write new save version number (big endian)
	js_info << "Writing new save version number..." << std::endl;
	saveDst.setWordBE(SAVE_VERSION_ABS_ADDRESS, newVersion);

	// Copy sGameData to sBackupGameData
	js_info << "Copying sGameData to sBackupGameData..." << std::endl;
	copyDataBlock(sd, symDst.getSRAMAddress("sGameData"), symDst.getSRAMAddress("sBackupGameData"), symDst.getSRAMAddress("sGameDataEnd") - symDst.getSRAMAddress("sGameData"));

	// Recalculate and write main checksum
	js_info << "Writing new checksums..." << std::endl;
	uint16_t new_checksum = calculateSaveChecksum(saveDst, symDst.getSRAMAddress("sGameData"), symDst.getSRAMAddress("sGameDataEnd"));
	saveDst.setWord(SAVE_CHECKSUM_ABS_ADDRESS, new_checksum);

	// Recalculate and write backup checksum
	uint16_t new_backup_checksum = calculateSaveChecksum(saveDst, symDst.getSRAMAddress("sBackupGameData"), symDst.getSRAMAddress("sBackupGameDataEnd"));
	saveDst.setWord(SAVE_BACKUP_CHECKSUM_ABS_ADDRESS, new_backup_checksum);
}
