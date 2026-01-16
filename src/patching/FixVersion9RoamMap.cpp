#include "patching/FixVersion9RoamMap.h"

#include "core/SymbolDatabaseContents.h"

namespace fixVersion9RoamMapNamespace {
	bool fixVersion9RoamMap(SaveBinary& oldsave, SaveBinary& patchedsave) {

		// copy the old save file to the new save file
		patchedsave = oldsave;

		// create the iterator
		SaveBinary::Iterator itnew(patchedsave, 0);

		// Load the version 9 sym file
		SymbolDatabase sym9(version9_sym_gz_data, version9_sym_gz_len);

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

		roam_struct_v9 roammon;
		js_info << "Fix wRoamMon1..." << std::endl;
		roammon = loadStruct<roam_struct_v9>(itnew, sym9.getPokemonDataAddress("wRoamMon1"));
		if (roammon.species == 0x00) {
			roammon.setMap(std::make_tuple(-1, -1));
			js_info << "wRoamMon1 species is 0x00, setting map to (-1, -1)" << std::endl;
		}
		if (std::get<0>(roammon.getMap()) == 0x00 && std::get<1>(roammon.getMap()) == 0x00) {
			roammon.setMap(std::make_tuple(-1, -1));
			js_info << "wRoamMon1 group/map is (0,0), setting map to (-1, -1)" << std::endl;
		}
		writeStruct<roam_struct_v9>(itnew, sym9.getPokemonDataAddress("wRoamMon1"), roammon);

		js_info << "Fix wRoamMon2..." << std::endl;
		roammon = loadStruct<roam_struct_v9>(itnew, sym9.getPokemonDataAddress("wRoamMon2"));
		if (roammon.species == 0x00) {
			roammon.setMap(std::make_tuple(-1, -1));
			js_info << "wRoamMon2 species is 0x00, setting map to (-1, -1)" << std::endl;
		}
		if (std::get<0>(roammon.getMap()) == 0x00 && std::get<1>(roammon.getMap()) == 0x00) {
			roammon.setMap(std::make_tuple(-1, -1));
			js_info << "wRoamMon2 group/map is (0,0), setting map to (-1, -1)" << std::endl;
		}
		writeStruct<roam_struct_v9>(itnew, sym9.getPokemonDataAddress("wRoamMon2"), roammon);

		js_info << "Fix wRoamMon3..." << std::endl;
		roammon = loadStruct<roam_struct_v9>(itnew, sym9.getPokemonDataAddress("wRoamMon3"));
		if (roammon.species == 0x00) {
			roammon.setMap(std::make_tuple(-1, -1));
			js_info << "wRoamMon3 species is 0x00, setting map to (-1, -1)" << std::endl;
		}
		if (std::get<0>(roammon.getMap()) == 0x00 && std::get<1>(roammon.getMap()) == 0x00) {
			roammon.setMap(std::make_tuple(-1, -1));
			js_info << "wRoamMon3 group/map is (0,0), setting map to (-1, -1)" << std::endl;
		}
		writeStruct<roam_struct_v9>(itnew, sym9.getPokemonDataAddress("wRoamMon3"), roammon);

		// write the new checksums to the version 9 save file
		js_info << "Write new checksums..." << std::endl;
		uint16_t new_checksum = calculateSaveChecksum(patchedsave, sym9.getSRAMAddress("sGameData"), sym9.getSRAMAddress("sGameDataEnd"));
		patchedsave.setWord(SAVE_CHECKSUM_ABS_ADDRESS, new_checksum);

		// write new backup checksums to the version 9 save file
		js_info << "Write new backup checksums..." << std::endl;
		uint16_t new_backup_checksum = calculateSaveChecksum(patchedsave, sym9.getSRAMAddress("sBackupGameData"), sym9.getSRAMAddress("sBackupGameDataEnd"));
		patchedsave.setWord(SAVE_BACKUP_CHECKSUM_ABS_ADDRESS, new_backup_checksum);

		js_info << "Sucessfully applied the PGO Battle Events Fix!" << std::endl;
		return true;

	}
}