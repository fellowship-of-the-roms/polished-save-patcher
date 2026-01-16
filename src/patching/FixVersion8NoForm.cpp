#include "patching/FixVersion8NoForm.h"

#include "core/SymbolDatabaseContents.h"

namespace fixVersion8NoFormNamespace {

	bool fixVersion8NoForm(SaveBinary& oldsave, SaveBinary& patchedsave) {
		using namespace fixVersion8NoFormNamespace;

		// copy the old save file to the new save file
		patchedsave = oldsave;

		// create the iterators
		SaveBinary::Iterator itnew(patchedsave, 0);

		// Load the version 7 and version 8 sym files
		SymbolDatabase sym8(version8_sym_data, version8_sym_len);

		// get the checksum word from the save file
		uint16_t save_checksum = patchedsave.getWord(SAVE_CHECKSUM_ABS_ADDRESS);

		// verify the save version number
		uint16_t saveVersion = oldsave.getWordBE(SAVE_VERSION_ABS_ADDRESS);
		if (saveVersion != 0x08) {
			js_error << "Unsupported save version: " << std::hex << saveVersion << std::endl;
			return false;
		}

		// verify the checksum of the file matches the calculated checksum
		// calculate the checksum from lookup symbol name "sGameData" to "sGameDataEnd"
		uint16_t calculated_checksum = calculateSaveChecksum(patchedsave, sym8.getSRAMAddress("sGameData"), sym8.getSRAMAddress("sGameDataEnd"));
		if (save_checksum != calculated_checksum) {
			js_error << "sGameData: " << std::hex << sym8.getSRAMAddress("sGameData") << std::endl;
			js_error << "sGameDataEnd: " << std::hex << sym8.getSRAMAddress("sGameDataEnd") << std::endl;
			js_error << "Checksum mismatch! Expected: " << std::hex << calculated_checksum << ", got: " << save_checksum << std::endl;
			return false;
		}

		// check the backup checksum word from the version 7 save file
		uint16_t backup_checksum = patchedsave.getWord(SAVE_BACKUP_CHECKSUM_ABS_ADDRESS);
		// verify the backup checksum of the version 7 file matches the calculated checksum
		// calculate the checksum from lookup symbol name "sBackupGameData" to "sBackupGameDataEnd"
		uint16_t calculated_backup_checksum = calculateSaveChecksum(patchedsave, sym8.getSRAMAddress("sBackupGameData"), sym8.getSRAMAddress("sBackupGameDataEnd"));
		if (backup_checksum != calculated_backup_checksum) {
			js_error << "Backup checksum mismatch! Expected: " << std::hex << calculated_backup_checksum << ", got: " << backup_checksum << std::endl;
			return false;
		}

		savemon_struct_v8 savemon;
		// Patching sBoxMons1A if checksums match
		js_info << "Checking sBoxMons1A checksums..." << std::endl;
		for (int i = 0; i < MONDB_ENTRIES_A_V8; i++) {
			itnew.seek(sym8.getSRAMAddress("sBoxMons1A") + i * sizeof(savemon_struct_v8));
			uint16_t calc_checksum = calculateNewboxChecksum(patchedsave, sym8.getSRAMAddress("sBoxMons1A") + i * sizeof(savemon_struct_v8));
			uint16_t cur_checksum = extractStoredNewboxChecksum(patchedsave, sym8.getSRAMAddress("sBoxMons1A") + i * sizeof(savemon_struct_v8));
			if (calc_checksum == cur_checksum) {
				savemon = patchSavemonV8(loadStruct<savemon_struct_v8>(itnew, sym8.getSRAMAddress("sBoxMons1A") + i * sizeof(savemon_struct_v8)));
				writeStruct<savemon_struct_v8>(itnew, sym8.getSRAMAddress("sBoxMons1A") + i * sizeof(savemon_struct_v8), savemon);
				// write the new checksum
				writeNewboxChecksum(patchedsave, sym8.getSRAMAddress("sBoxMons1A") + i * sizeof(savemon_struct_v8));
			}
		}

		// Patching sBoxMons1B if checksums match
		js_info << "Checking sBoxMons1B checksums..." << std::endl;
		for (int i = 0; i < MONDB_ENTRIES_B_V8; i++) {
			itnew.seek(sym8.getSRAMAddress("sBoxMons1B") + i * sizeof(savemon_struct_v8));
			uint16_t calc_checksum = calculateNewboxChecksum(patchedsave, sym8.getSRAMAddress("sBoxMons1B") + i * sizeof(savemon_struct_v8));
			uint16_t cur_checksum = extractStoredNewboxChecksum(patchedsave, sym8.getSRAMAddress("sBoxMons1B") + i * sizeof(savemon_struct_v8));
			if (calc_checksum == cur_checksum) {
				savemon = patchSavemonV8(loadStruct<savemon_struct_v8>(itnew, sym8.getSRAMAddress("sBoxMons1B") + i * sizeof(savemon_struct_v8)));
				writeStruct<savemon_struct_v8>(itnew, sym8.getSRAMAddress("sBoxMons1B") + i * sizeof(savemon_struct_v8), savemon);
				// write the new checksum
				writeNewboxChecksum(patchedsave, sym8.getSRAMAddress("sBoxMons1B") + i * sizeof(savemon_struct_v8));
			}
		}

		// Patching sBoxMons1C if checksums match
		js_info << "Checking sBoxMons1C checksums..." << std::endl;
		for (int i = 0; i < MONDB_ENTRIES_C_V8; i++) {
			itnew.seek(sym8.getSRAMAddress("sBoxMons1C") + i * sizeof(savemon_struct_v8));
			uint16_t calc_checksum = calculateNewboxChecksum(patchedsave, sym8.getSRAMAddress("sBoxMons1C") + i * sizeof(savemon_struct_v8));
			uint16_t cur_checksum = extractStoredNewboxChecksum(patchedsave, sym8.getSRAMAddress("sBoxMons1C") + i * sizeof(savemon_struct_v8));
			if (calc_checksum == cur_checksum) {
				savemon = patchSavemonV8(loadStruct<savemon_struct_v8>(itnew, sym8.getSRAMAddress("sBoxMons1C") + i * sizeof(savemon_struct_v8)));
				writeStruct<savemon_struct_v8>(itnew, sym8.getSRAMAddress("sBoxMons1C") + i * sizeof(savemon_struct_v8), savemon);
				// write the new checksum
				writeNewboxChecksum(patchedsave, sym8.getSRAMAddress("sBoxMons1C") + i * sizeof(savemon_struct_v8));
			}
		}

		// Patching sBoxMons2A if checksums match
		js_info << "Checking sBoxMons2A checksums..." << std::endl;
		for (int i = 0; i < MONDB_ENTRIES_A_V8; i++) {
			itnew.seek(sym8.getSRAMAddress("sBoxMons2A") + i * sizeof(savemon_struct_v8));
			uint16_t calc_checksum = calculateNewboxChecksum(patchedsave, sym8.getSRAMAddress("sBoxMons2A") + i * sizeof(savemon_struct_v8));
			uint16_t cur_checksum = extractStoredNewboxChecksum(patchedsave, sym8.getSRAMAddress("sBoxMons2A") + i * sizeof(savemon_struct_v8));
			if (calc_checksum == cur_checksum) {
				savemon = patchSavemonV8(loadStruct<savemon_struct_v8>(itnew, sym8.getSRAMAddress("sBoxMons2A") + i * sizeof(savemon_struct_v8)));
				writeStruct<savemon_struct_v8>(itnew, sym8.getSRAMAddress("sBoxMons2A") + i * sizeof(savemon_struct_v8), savemon);
				// write the new checksum
				writeNewboxChecksum(patchedsave, sym8.getSRAMAddress("sBoxMons2A") + i * sizeof(savemon_struct_v8));
			}
		}

		// Patching sBoxMons2B if checksums match
		js_info << "Checking sBoxMons2B checksums..." << std::endl;
		for (int i = 0; i < MONDB_ENTRIES_B_V8; i++) {
			itnew.seek(sym8.getSRAMAddress("sBoxMons2B") + i * sizeof(savemon_struct_v8));
			uint16_t calc_checksum = calculateNewboxChecksum(patchedsave, sym8.getSRAMAddress("sBoxMons2B") + i * sizeof(savemon_struct_v8));
			uint16_t cur_checksum = extractStoredNewboxChecksum(patchedsave, sym8.getSRAMAddress("sBoxMons2B") + i * sizeof(savemon_struct_v8));
			if (calc_checksum == cur_checksum) {
				savemon = patchSavemonV8(loadStruct<savemon_struct_v8>(itnew, sym8.getSRAMAddress("sBoxMons2B") + i * sizeof(savemon_struct_v8)));
				writeStruct<savemon_struct_v8>(itnew, sym8.getSRAMAddress("sBoxMons2B") + i * sizeof(savemon_struct_v8), savemon);
				// write the new checksum
				writeNewboxChecksum(patchedsave, sym8.getSRAMAddress("sBoxMons2B") + i * sizeof(savemon_struct_v8));
			}
		}

		// Patching sBoxMons2C if checksums match
		js_info << "Checking sBoxMons2C checksums..." << std::endl;
		for (int i = 0; i < MONDB_ENTRIES_C_V8; i++) {
			itnew.seek(sym8.getSRAMAddress("sBoxMons2C") + i * sizeof(savemon_struct_v8));
			uint16_t calc_checksum = calculateNewboxChecksum(patchedsave, sym8.getSRAMAddress("sBoxMons2C") + i * sizeof(savemon_struct_v8));
			uint16_t cur_checksum = extractStoredNewboxChecksum(patchedsave, sym8.getSRAMAddress("sBoxMons2C") + i * sizeof(savemon_struct_v8));
			if (calc_checksum == cur_checksum) {
				savemon = patchSavemonV8(loadStruct<savemon_struct_v8>(itnew, sym8.getSRAMAddress("sBoxMons2C") + i * sizeof(savemon_struct_v8)));
				writeStruct<savemon_struct_v8>(itnew, sym8.getSRAMAddress("sBoxMons2C") + i * sizeof(savemon_struct_v8), savemon);
				// write the new checksum
				writeNewboxChecksum(patchedsave, sym8.getSRAMAddress("sBoxMons2C") + i * sizeof(savemon_struct_v8));
			}
		}

		breedmon_struct_v8 breedmon;
		// fix and copy wBreedMon1
		js_info << "fix and copy wBreedMon1..." << std::endl;
		uint16_t species = itnew.getByte(sym8.getPokemonDataAddress("wBreedMon1Species"));
		if (species != 0x00) {
			breedmon = patchBreedmonV8(loadStruct<breedmon_struct_v8>(itnew, sym8.getPokemonDataAddress("wBreedMon1")));
			writeStruct<breedmon_struct_v8>(itnew, sym8.getPokemonDataAddress("wBreedMon1"), breedmon);
		}

		// fix and copy wBreedMon2
		js_info << "fix and copy wBreedMon2..." << std::endl;
		species = itnew.getByte(sym8.getPokemonDataAddress("wBreedMon2Species"));
		if (species != 0x00) {
			breedmon = patchBreedmonV8(loadStruct<breedmon_struct_v8>(itnew, sym8.getPokemonDataAddress("wBreedMon2")));
			writeStruct<breedmon_struct_v8>(itnew, sym8.getPokemonDataAddress("wBreedMon2"), breedmon);
		}

		party_struct_v8 partymon;
		// fix the party mons
		js_info << "Fix party mons..." << std::endl;
		for (int i = 0; i < PARTY_LENGTH; i++) {
			uint16_t species = itnew.getByte(sym8.getPokemonDataAddress("wPartyMons") + i * sizeof(party_struct_v8));
			if (species == 0x00) {
				continue;
			}
			partymon = patchPartyV8(loadStruct<party_struct_v8>(itnew, sym8.getPokemonDataAddress("wPartyMons") + i * sizeof(party_struct_v8)));
			writeStruct<party_struct_v8>(itnew, sym8.getPokemonDataAddress("wPartyMons") + i * sizeof(party_struct_v8), partymon);
		}

		// fix wContestMonSpecies and wContestMonExtSpecies
		js_info << "Fix wContestMon..." << std::endl;
		species = itnew.getByte(sym8.getPokemonDataAddress("wContestMonSpecies"));
		if (species != 0x00) {
			partymon = patchPartyV8(loadStruct<party_struct_v8>(itnew, sym8.getPokemonDataAddress("wContestMon")));
			writeStruct<party_struct_v8>(itnew, sym8.getPokemonDataAddress("wContestMon"), partymon);
		}

		roam_struct_v8 roammon;
		js_info << "Fix wRoamMon1..." << std::endl;
		species = itnew.getByte(sym8.getPokemonDataAddress("wRoamMon1Species"));
		if (species != 0x00) {
			roammon = patchRoamV8(loadStruct<roam_struct_v8>(itnew, sym8.getPokemonDataAddress("wRoamMon1")));
			writeStruct<roam_struct_v8>(itnew, sym8.getPokemonDataAddress("wRoamMon1"), roammon);
		}

		js_info << "Fix wRoamMon2..." << std::endl;
		species = itnew.getByte(sym8.getPokemonDataAddress("wRoamMon2Species"));
		if (species != 0x00) {
			roammon = patchRoamV8(loadStruct<roam_struct_v8>(itnew, sym8.getPokemonDataAddress("wRoamMon2")));
			writeStruct<roam_struct_v8>(itnew, sym8.getPokemonDataAddress("wRoamMon2"), roammon);
		}

		js_info << "Fix wRoamMon3..." << std::endl;
		species = itnew.getByte(sym8.getPokemonDataAddress("wRoamMon3Species"));
		if (species != 0x00) {
			roammon = patchRoamV8(loadStruct<roam_struct_v8>(itnew, sym8.getPokemonDataAddress("wRoamMon3")));
			writeStruct<roam_struct_v8>(itnew, sym8.getPokemonDataAddress("wRoamMon3"), roammon);
		}

		// write new checksums to the version 8 save file
		js_info << "Write new checksums..." << std::endl;
		uint16_t new_checksum = calculateSaveChecksum(patchedsave, sym8.getSRAMAddress("sGameData"), sym8.getSRAMAddress("sGameDataEnd"));
		patchedsave.setWord(SAVE_CHECKSUM_ABS_ADDRESS, new_checksum);

		// write new backup checksums to the version 8 save file
		js_info << "Write new backup checksums..." << std::endl;
		uint16_t new_backup_checksum = calculateSaveChecksum(patchedsave, sym8.getSRAMAddress("sBackupGameData"), sym8.getSRAMAddress("sBackupGameDataEnd"));
		patchedsave.setWord(SAVE_BACKUP_CHECKSUM_ABS_ADDRESS, new_backup_checksum);

		// write the modified save file to the output file and print success message
		js_info << "Sucessfully patched to 3.0.0 save version 8!" << std::endl;

		return true;
	}


	savemon_struct_v8 patchSavemonV8(const savemon_struct_v8& savemon) {
		savemon_struct_v8 newsavemon = savemon;
		if (newsavemon.getForm() == 0x00) {
			newsavemon.setForm(0x01);
			js_info << "Patched form from 0x00 to 0x01" << std::endl;
		}
		return newsavemon;
	}

	breedmon_struct_v8 patchBreedmonV8(const breedmon_struct_v8& breedmon) {
		breedmon_struct_v8 newbreedmon = breedmon;
		if (newbreedmon.getForm() == 0x00) {
			newbreedmon.setForm(0x01);
			js_info << "Patched form from 0x00 to 0x01" << std::endl;
		}
		return newbreedmon;
	}

	party_struct_v8 patchPartyV8(const party_struct_v8& party) {
		party_struct_v8 newparty = party;
		if (newparty.breedmon.getForm() == 0x00) {
			newparty.breedmon.setForm(0x01);
			js_info << "Patched form from 0x00 to 0x01" << std::endl;
		}
		return newparty;
	}

	roam_struct_v8 patchRoamV8(const roam_struct_v8& roam) {
		roam_struct_v8 newroam = roam;
		if (newroam.getForm() == 0x00) {
			newroam.setForm(0x01);
			js_info << "Patched form from 0x00 to 0x01" << std::endl;
		}
		return newroam;
	}
}