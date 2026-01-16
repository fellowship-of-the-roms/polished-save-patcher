#include "patching/FixVersion9PCWarpID.h"

#include "core/SymbolDatabaseContents.h"

namespace fixVersion9PCWarpIDNamespace {
	bool fixVersion9PCWarpID(SaveBinary& oldsave, SaveBinary& patchedsave) {
		using namespace fixVersion9PCWarpIDNamespace;

		// copy the old save file to the new save file
		patchedsave = oldsave;

		// create the iterators
		SaveBinary::Iterator itnew(patchedsave, 0);

		// Load the version 9 sym file
		SymbolDatabase sym9(version9_sym_data, version9_sym_len);

		// get the checksum word from the save file
		uint16_t save_checksum = patchedsave.getWord(SAVE_CHECKSUM_ABS_ADDRESS);

		// verify the save version number
		uint16_t saveVersion = oldsave.getWordBE(SAVE_VERSION_ABS_ADDRESS);
		if (saveVersion != 0x09) {
			js_error << "Unsupported save version: " << std::hex << saveVersion << std::endl;
			return false;
		}

		// verify the checksum of the file matches the calculated checksum
		// calculate the checksum from lookup symbol name "sGameData" to "sGameDataEnd"
		uint16_t calculated_checksum = calculateSaveChecksum(patchedsave, sym9.getSRAMAddress("sGameData"), sym9.getSRAMAddress("sGameDataEnd"));
		if (save_checksum != calculated_checksum) {
			js_error << "sGameData: " << std::hex << sym9.getSRAMAddress("sGameData") << std::endl;
			js_error << "sGameDataEnd: " << std::hex << sym9.getSRAMAddress("sGameDataEnd") << std::endl;
			js_error << "Checksum mismatch! Expected: " << std::hex << calculated_checksum << ", got: " << save_checksum << std::endl;
			return false;
		}

		// check the backup checksum word from the version 9 save file
		uint16_t backup_checksum = patchedsave.getWord(SAVE_BACKUP_CHECKSUM_ABS_ADDRESS);
		// verify the backup checksum of the version 9 file matches the calculated checksum
		// calculate the checksum from lookup symbol name "sBackupGameData" to "sBackupGameDataEnd"
		uint16_t calculated_backup_checksum = calculateSaveChecksum(patchedsave, sym9.getSRAMAddress("sBackupGameData"), sym9.getSRAMAddress("sBackupGameDataEnd"));
		if (backup_checksum != calculated_backup_checksum) {
			js_error << "Backup checksum mismatch! Expected: " << std::hex << calculated_backup_checksum << ", got: " << backup_checksum << std::endl;
			return false;
		}

		// check if the player in the PKMN Center 2nd Floor
		uint8_t map_group = itnew.getByte(sym9.getMapDataAddress("wMapGroup"));
		itnew.next();
		uint8_t map_num = itnew.getByte();
		if (map_group != MON_CENTER_2F_GROUP || map_num != MON_CENTER_2F_MAP) {
			js_error << "Player is not in the PKMN Center 2nd Floor. Go to where you heal in game, and head upstairs. Then re-save your game and try again." << std::endl;
			return false;
		}

		uint8_t prev_map_group = itnew.getByte(sym9.getMapDataAddress("wBackupMapGroup"));
		uint8_t prev_map_num = itnew.getByte(sym9.getMapDataAddress("wBackupMapNumber"));
		// check if the previous map is a valid PC warp ID in the validPCWarpIDs array
		bool valid_prev_map = false;
		for (auto& validPCWarpID : validPCWarpIDs) {
			if (prev_map_group == validPCWarpID.first && prev_map_num == validPCWarpID.second) {
				valid_prev_map = true;
				break;
			}
		}
		if (valid_prev_map) {
			js_info << "Player's previous map is a valid PKMN Center warp ID. No need to fix the warp ID." << std::endl;
			return true;
		}
		js_warning << "Player's previous map is not a valid PKMN Center Warp ID! We will reset it to one." << std::endl;
		if (isFlagBitSet(itnew, sym9.getPlayerDataAddress("wJohtoBadges"), PLAINBADGE)) {
			js_warning << "Player has the PLAINBADGE, the stairs will now take you to Goldenrod PKMN Center." << std::endl;
			itnew.setByte(sym9.getMapDataAddress("wBackupWarpNumber"), 4);
			itnew.setByte(sym9.getMapDataAddress("wBackupMapGroup"), GOLDENROD_POKECOM_CENTER_1F.first);
			itnew.setByte(sym9.getMapDataAddress("wBackupMapNumber"), GOLDENROD_POKECOM_CENTER_1F.second);
		} else {
			js_warning << "Player does not have the PLAINBADGE, the stairs will now warp you to your house." << std::endl;
			itnew.setByte(sym9.getMapDataAddress("wBackupWarpNumber"), 3);
			itnew.setByte(sym9.getMapDataAddress("wBackupMapGroup"), PLAYERS_HOUSE_1F.first);
			itnew.setByte(sym9.getMapDataAddress("wBackupMapNumber"), PLAYERS_HOUSE_1F.second);
		}

		// write the new checksums to the version 9 save file
		js_info << "Write new checksums..." << std::endl;
		uint16_t new_checksum = calculateSaveChecksum(patchedsave, sym9.getSRAMAddress("sGameData"), sym9.getSRAMAddress("sGameDataEnd"));
		patchedsave.setWord(SAVE_CHECKSUM_ABS_ADDRESS, new_checksum);

		// write new backup checksums to the version 9 save file
		js_info << "Write new backup checksums..." << std::endl;
		uint16_t new_backup_checksum = calculateSaveChecksum(patchedsave, sym9.getSRAMAddress("sBackupGameData"), sym9.getSRAMAddress("sBackupGameDataEnd"));
		patchedsave.setWord(SAVE_BACKUP_CHECKSUM_ABS_ADDRESS, new_backup_checksum);

		// write the modified save file to the output file and print success message
		js_info << "Sucessfully applied the Version 9 PKMN Center Warp ID Fix!" << std::endl;
		return true;

	}
}
