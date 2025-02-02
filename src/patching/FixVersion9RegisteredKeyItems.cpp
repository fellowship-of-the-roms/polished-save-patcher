#include "patching/FixVersion9RegisteredKeyItems.h"

namespace fixVersion9RegisteredKeyItemsNamespace {
	bool fixVersion9RegisteredKeyItems(SaveBinary& oldsave, SaveBinary& patchedsave) {
		using namespace fixVersion9RegisteredKeyItemsNamespace;
		
		// copy the old save file to the new save file
		patchedsave = oldsave;

		// create the iterators
		SaveBinary::Iterator itnew(patchedsave, 0);

		// Load the version 9 sym file
		SymbolDatabase sym9(VERSION_9_SYMBOL_FILE);

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

		// fix wRegisteredItems...
		js_info << "Fix wRegisteredItems..." << std::endl;
		for (int i = 0; i < 4; i++) {
			uint8_t item = itnew.getByte(sym9.getPokemonDataAddress("wRegisteredItems") + i);
			if (item == 0xff) {
				// found a bad 0xff, set it to 0x00
				js_info << "Found bad 0xff at wRegisteredItems[" << i << "], setting to 0x00" << std::endl;
				itnew.setByte(sym9.getPokemonDataAddress("wRegisteredItems") + i, 0x00);
			}
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
		js_info << "Sucessfully applied the Version 9 Registered Key Items fix!" << std::endl;
		return true;
	}
}