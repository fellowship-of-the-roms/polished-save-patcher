#include "patching/FixVersion9MagikarpPlainForm.h"

namespace fixVersion9MagikarpPlainFormNamespace {

	bool fixVersion9MagikarpPlainForm(SaveBinary& oldsave, SaveBinary& patchedsave) {
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

		savemon_struct_v9 savemon;
		// Patching sBoxMons1A if checksums match
		js_info << "Checking sBoxMons1A checksums..." << std::endl;
		for (int i = 0; i < MONDB_ENTRIES_A_V9; i++) {
			itnew.seek(sym9.getSRAMAddress("sBoxMons1A") + i * sizeof(savemon_struct_v9));
			uint16_t calc_checksum = calculateNewboxChecksum(patchedsave, sym9.getSRAMAddress("sBoxMons1A") + i * sizeof(savemon_struct_v9));
			uint16_t cur_checksum = extractStoredNewboxChecksum(patchedsave, sym9.getSRAMAddress("sBoxMons1A") + i * sizeof(savemon_struct_v9));
			if (calc_checksum == cur_checksum) {
				savemon = patchSavemonV9(loadStruct<savemon_struct_v9>(itnew, sym9.getSRAMAddress("sBoxMons1A") + i * sizeof(savemon_struct_v9)));
				writeStruct<savemon_struct_v9>(itnew, sym9.getSRAMAddress("sBoxMons1A") + i * sizeof(savemon_struct_v9), savemon);
				// write the new checksum
				writeNewboxChecksum(patchedsave, sym9.getSRAMAddress("sBoxMons1A") + i * sizeof(savemon_struct_v9));
			}
		}

		// Patching sBoxMons1B if checksums match
		js_info << "Checking sBoxMons1B checksums..." << std::endl;
		for (int i = 0; i < MONDB_ENTRIES_B_V9; i++) {
			itnew.seek(sym9.getSRAMAddress("sBoxMons1B") + i * sizeof(savemon_struct_v9));
			uint16_t calc_checksum = calculateNewboxChecksum(patchedsave, sym9.getSRAMAddress("sBoxMons1B") + i * sizeof(savemon_struct_v9));
			uint16_t cur_checksum = extractStoredNewboxChecksum(patchedsave, sym9.getSRAMAddress("sBoxMons1B") + i * sizeof(savemon_struct_v9));
			if (calc_checksum == cur_checksum) {
				savemon = patchSavemonV9(loadStruct<savemon_struct_v9>(itnew, sym9.getSRAMAddress("sBoxMons1B") + i * sizeof(savemon_struct_v9)));
				writeStruct<savemon_struct_v9>(itnew, sym9.getSRAMAddress("sBoxMons1B") + i * sizeof(savemon_struct_v9), savemon);
				// write the new checksum
				writeNewboxChecksum(patchedsave, sym9.getSRAMAddress("sBoxMons1B") + i * sizeof(savemon_struct_v9));
			}
		}

		// Patching sBoxMons1C if checksums match
		js_info << "Checking sBoxMons1C checksums..." << std::endl;
		for (int i = 0; i < MONDB_ENTRIES_C_V9; i++) {
			itnew.seek(sym9.getSRAMAddress("sBoxMons1C") + i * sizeof(savemon_struct_v9));
			uint16_t calc_checksum = calculateNewboxChecksum(patchedsave, sym9.getSRAMAddress("sBoxMons1C") + i * sizeof(savemon_struct_v9));
			uint16_t cur_checksum = extractStoredNewboxChecksum(patchedsave, sym9.getSRAMAddress("sBoxMons1C") + i * sizeof(savemon_struct_v9));
			if (calc_checksum == cur_checksum) {
				savemon = patchSavemonV9(loadStruct<savemon_struct_v9>(itnew, sym9.getSRAMAddress("sBoxMons1C") + i * sizeof(savemon_struct_v9)));
				writeStruct<savemon_struct_v9>(itnew, sym9.getSRAMAddress("sBoxMons1C") + i * sizeof(savemon_struct_v9), savemon);
				// write the new checksum
				writeNewboxChecksum(patchedsave, sym9.getSRAMAddress("sBoxMons1C") + i * sizeof(savemon_struct_v9));
			}
		}

		// Patching sBoxMons2A if checksums match
		js_info << "Checking sBoxMons2A checksums..." << std::endl;
		for (int i = 0; i < MONDB_ENTRIES_A_V9; i++) {
			itnew.seek(sym9.getSRAMAddress("sBoxMons2A") + i * sizeof(savemon_struct_v9));
			uint16_t calc_checksum = calculateNewboxChecksum(patchedsave, sym9.getSRAMAddress("sBoxMons2A") + i * sizeof(savemon_struct_v9));
			uint16_t cur_checksum = extractStoredNewboxChecksum(patchedsave, sym9.getSRAMAddress("sBoxMons2A") + i * sizeof(savemon_struct_v9));
			if (calc_checksum == cur_checksum) {
				savemon = patchSavemonV9(loadStruct<savemon_struct_v9>(itnew, sym9.getSRAMAddress("sBoxMons2A") + i * sizeof(savemon_struct_v9)));
				writeStruct<savemon_struct_v9>(itnew, sym9.getSRAMAddress("sBoxMons2A") + i * sizeof(savemon_struct_v9), savemon);
				// write the new checksum
				writeNewboxChecksum(patchedsave, sym9.getSRAMAddress("sBoxMons2A") + i * sizeof(savemon_struct_v9));
			}
		}

		// Patching sBoxMons2B if checksums match
		js_info << "Checking sBoxMons2B checksums..." << std::endl;
		for (int i = 0; i < MONDB_ENTRIES_B_V9; i++) {
			itnew.seek(sym9.getSRAMAddress("sBoxMons2B") + i * sizeof(savemon_struct_v9));
			uint16_t calc_checksum = calculateNewboxChecksum(patchedsave, sym9.getSRAMAddress("sBoxMons2B") + i * sizeof(savemon_struct_v9));
			uint16_t cur_checksum = extractStoredNewboxChecksum(patchedsave, sym9.getSRAMAddress("sBoxMons2B") + i * sizeof(savemon_struct_v9));
			if (calc_checksum == cur_checksum) {
				savemon = patchSavemonV9(loadStruct<savemon_struct_v9>(itnew, sym9.getSRAMAddress("sBoxMons2B") + i * sizeof(savemon_struct_v9)));
				writeStruct<savemon_struct_v9>(itnew, sym9.getSRAMAddress("sBoxMons2B") + i * sizeof(savemon_struct_v9), savemon);
				// write the new checksum
				writeNewboxChecksum(patchedsave, sym9.getSRAMAddress("sBoxMons2B") + i * sizeof(savemon_struct_v9));
			}
		}

		// Patching sBoxMons2C if checksums match
		js_info << "Checking sBoxMons2C checksums..." << std::endl;
		for (int i = 0; i < MONDB_ENTRIES_C_V9; i++) {
			itnew.seek(sym9.getSRAMAddress("sBoxMons2C") + i * sizeof(savemon_struct_v9));
			uint16_t calc_checksum = calculateNewboxChecksum(patchedsave, sym9.getSRAMAddress("sBoxMons2C") + i * sizeof(savemon_struct_v9));
			uint16_t cur_checksum = extractStoredNewboxChecksum(patchedsave, sym9.getSRAMAddress("sBoxMons2C") + i * sizeof(savemon_struct_v9));
			if (calc_checksum == cur_checksum) {
				savemon = patchSavemonV9(loadStruct<savemon_struct_v9>(itnew, sym9.getSRAMAddress("sBoxMons2C") + i * sizeof(savemon_struct_v9)));
				writeStruct<savemon_struct_v9>(itnew, sym9.getSRAMAddress("sBoxMons2C") + i * sizeof(savemon_struct_v9), savemon);
				// write the new checksum
				writeNewboxChecksum(patchedsave, sym9.getSRAMAddress("sBoxMons2C") + i * sizeof(savemon_struct_v9));
			}
		}

		breedmon_struct_v9 breedmon;
		// fix and copy wBreedMon1
		js_info << "fix and copy wBreedMon1..." << std::endl;
		uint16_t species = itnew.getByte(sym9.getPokemonDataAddress("wBreedMon1Species"));
		if (species != 0x00) {
			breedmon = patchBreedmonV9(loadStruct<breedmon_struct_v9>(itnew, sym9.getPokemonDataAddress("wBreedMon1")));
			writeStruct<breedmon_struct_v9>(itnew, sym9.getPokemonDataAddress("wBreedMon1"), breedmon);
		}

		// fix and copy wBreedMon2
		js_info << "fix and copy wBreedMon2..." << std::endl;
		species = itnew.getByte(sym9.getPokemonDataAddress("wBreedMon2Species"));
		if (species != 0x00) {
			breedmon = patchBreedmonV9(loadStruct<breedmon_struct_v9>(itnew, sym9.getPokemonDataAddress("wBreedMon2")));
			writeStruct<breedmon_struct_v9>(itnew, sym9.getPokemonDataAddress("wBreedMon2"), breedmon);
		}

		party_struct_v9 partymon;
		// fix the party mons
		js_info << "Fix party mons..." << std::endl;
		for (int i = 0; i < PARTY_LENGTH; i++) {
			uint16_t species = itnew.getByte(sym9.getPokemonDataAddress("wPartyMons") + i * sizeof(party_struct_v9));
			if (species == 0x00) {
				continue;
			}
			partymon = patchPartyV9(loadStruct<party_struct_v9>(itnew, sym9.getPokemonDataAddress("wPartyMons") + i * sizeof(party_struct_v9)));
			writeStruct<party_struct_v9>(itnew, sym9.getPokemonDataAddress("wPartyMons") + i * sizeof(party_struct_v9), partymon);
		}

		// fix wContestMon
		js_info << "fix wContestMon..." << std::endl;
		species = itnew.getByte(sym9.getPokemonDataAddress("wContestMonSpecies"));
		if (species != 0x00) {
			partymon = patchPartyV9(loadStruct<party_struct_v9>(itnew, sym9.getPokemonDataAddress("wContestMon")));
			writeStruct<party_struct_v9>(itnew, sym9.getPokemonDataAddress("wContestMon"), partymon);
		}

		// write new checksums to the version 9 save file
		js_info << "Write new checksums..." << std::endl;
		uint16_t new_checksum = calculateSaveChecksum(patchedsave, sym9.getSRAMAddress("sGameData"), sym9.getSRAMAddress("sGameDataEnd"));
		patchedsave.setWord(SAVE_CHECKSUM_ABS_ADDRESS, new_checksum);

		// write new backup checksums to the version 9 save file
		js_info << "Write new backup checksums..." << std::endl;
		uint16_t new_backup_checksum = calculateSaveChecksum(patchedsave, sym9.getSRAMAddress("sBackupGameData"), sym9.getSRAMAddress("sBackupGameDataEnd"));
		patchedsave.setWord(SAVE_BACKUP_CHECKSUM_ABS_ADDRESS, new_backup_checksum);

		// write the modified save file to the output file and print success message
		js_info << "Writing the modified save file..." << std::endl;
		return true;
	}

	savemon_struct_v9 patchSavemonV9(const savemon_struct_v9& savemon) {
		savemon_struct_v9 newsavemon = savemon;
		if (newsavemon.species == MAGIKARP_V9 && newsavemon.personality[1] == 0xFF) {
			js_warning << "Found Magikarp with corrupted personality byte. Fixing, but gender information has been lost!" << std::endl;
			js_warning << "Alternatively, you can repatch from your original save file." << std::endl;
			newsavemon.personality[1] = 0x01;
		}
		return newsavemon;
	}

	breedmon_struct_v9 patchBreedmonV9(const breedmon_struct_v9& breedmon) {
		breedmon_struct_v9 newbreedmon = breedmon;
		if (newbreedmon.species == MAGIKARP_V9 && newbreedmon.personality[1] == 0xFF) {
			js_warning << "Found Magikarp with corrupted personality byte. Fixing, but gender information has been lost!" << std::endl;
			js_warning << "Alternatively, you can repatch from your original save file." << std::endl;
			newbreedmon.personality[1] = 0x01;
		}
		return newbreedmon;
	}

	party_struct_v9 patchPartyV9(const party_struct_v9& partymon) {
		party_struct_v9 newpartymon = partymon;
		if (newpartymon.breedmon.species == MAGIKARP_V9 && newpartymon.breedmon.personality[1] == 0xFF) {
			js_warning << "Found Magikarp with corrupted personality byte. Fixing, but gender information has been lost!" << std::endl;
			js_warning << "Alternatively, you can repatch from your original save file." << std::endl;
			newpartymon.breedmon.personality[1] = 0x01;
		}
		return newpartymon;
	}

}