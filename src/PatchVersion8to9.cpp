#include "PatchVersion8to9.h"
#include "CommonPatchFunctions.h"
#include "SymbolDatabase.h"
#include "Logging.h"

bool patchVersion8to9(SaveBinary& save8, SaveBinary& save9) {
	// copy the old save file to the new save file
	save9 = save8;

	// create the iterators
	SaveBinary::Iterator it8(save8, 0);
	SaveBinary::Iterator it9(save9, 0);

	// Load the version 8 and version 9 sym files
	SymbolDatabase sym8(VERSION_8_SYMBOL_FILE);
	SymbolDatabase sym9(VERSION_9_SYMBOL_FILE);

	SourceDest sd = {it8, it9, sym8, sym9};

	// get the checksum word from the version 8 save file
	uint16_t save_checksum = save8.getWord(SAVE_CHECKSUM_ABS_ADDRESS);

	// verify the checksum of the version 8 file matches the calculated checksum
	uint16_t calculated_checksum = calculateSaveChecksum(save8, sym8.getSRAMAddress("sGameData"), sym8.getSRAMAddress("sGameDataEnd"));
	if (save_checksum != calculated_checksum) {
		js_error <<  "Checksum mismatch! Expected: " << std::hex << calculated_checksum << ", got: " << save_checksum << std::endl;
		return false;
	}

	// check the backup checksum word from the version 8 save file
	uint16_t backup_checksum = save8.getWord(SAVE_BACKUP_CHECKSUM_ABS_ADDRESS);
	uint16_t calculated_backup_checksum = calculateSaveChecksum(save8, sym8.getSRAMAddress("sBackupGameData"), sym8.getSRAMAddress("sBackupGameDataEnd"));
	if (backup_checksum != calculated_backup_checksum) {
		js_error <<  "Backup checksum mismatch! Expected: " << std::hex << calculated_backup_checksum << ", got: " << backup_checksum << std::endl;
		return false;
	}

	// check if the player is in the Mon Healing Center P.C. 2nd Floor
	uint8_t map_group = it8.getByte(sym8.getMapDataAddress("wMapGroup"));
	it8.next();
	uint8_t map_num = it8.getByte();
	if (map_group != MON_CENTER_2F_GROUP || map_num != MON_CENTER_2F_MAP) {
		js_error <<  "Player is not in the Mon Healing Center P.C. 2nd Floor. Go to where you heal in game, and head upstairs. Then re-save your game and try again." << std::endl;
		return false;
	}

	// Copy from address sBoxMons1B to wKeyItemsEnd
	js_info <<  "Copying from sBoxMons1B to wKeyItemsEnd" << std::endl;
	copyDataBlock(sd, sym8.getSRAMAddress("sBoxMons1B"), sym9.getSRAMAddress("sBoxMons1B"), sym8.getPlayerDataAddress("wKeyItemsEnd") - sym8.getSRAMAddress("sBoxMons1B"));

	it9.setByte(sym9.getPlayerDataAddress("wKeyItemsEnd") - 1, 0x00);

	// copy from wKeyItemsEnd to wMooMooBerries
	js_info <<  "Copying from wKeyItemsEnd to wMooMooBerries" << std::endl;
	copyDataBlock(sd, sym8.getPlayerDataAddress("wKeyItemsEnd"), sym9.getPlayerDataAddress("wKeyItemsEnd"), sym8.getPlayerDataAddress("wMooMooBerries") - sym8.getPlayerDataAddress("wKeyItemsEnd"));

	it9.setByte(sym9.getPlayerDataAddress("wMooMooBerries") - 1, 0x00);

	// Copy from wMooMooBerries to wEmotePal + 1
	js_info <<  "Copying from wMooMooBerries to wEmotePal + 1" << std::endl;
	copyDataBlock(sd, sym8.getPlayerDataAddress("wMooMooBerries"), sym9.getPlayerDataAddress("wMooMooBerries"), sym8.getPlayerDataAddress("wEmotePal") + 1 - sym8.getPlayerDataAddress("wMooMooBerries"));

	clearDataBlock(sd, sym9.getPlayerDataAddress("wEmotePal") + 1, sym9.getPlayerDataAddress("wWingAmounts") - sym9.getPlayerDataAddress("wEmotePal") + 1);

	// Copy from wWingAmounts to wEndPokedexCaught
	js_info <<  "Copying from wWingAmounts to wEndPokedexCaught" << std::endl;
	copyDataBlock(sd, sym8.getPlayerDataAddress("wWingAmounts"), sym9.getPlayerDataAddress("wWingAmounts"), sym8.getPlayerDataAddress("wEndPokedexCaught") - sym8.getPlayerDataAddress("wWingAmounts"));

	it9.setByte(sym9.getPlayerDataAddress("wEndPokedexCaught") - 1, 0x00);

	// Copy from wPokedexSeen to wEndPokedexFlags
	js_info <<  "Copying from wPokedexSeen to wEndPokedexFlags" << std::endl;
	copyDataBlock(sd, sym8.getPlayerDataAddress("wPokedexSeen"), sym9.getPlayerDataAddress("wPokedexSeen"), sym8.getPlayerDataAddress("wEndPokedexFlags") - sym8.getPlayerDataAddress("wPokedexSeen"));

	it9.setByte(sym9.getPlayerDataAddress("wEndPokedexFlags") - 1, 0x00);

	// Copy from wUnlockedUnowns to MIN_SAVE_SIZE
	js_info <<  "Copying from wUnlockedUnowns to MIN_SAVE_SIZE" << std::endl;
	copyDataBlock(sd, sym8.getPlayerDataAddress("wUnlockedUnowns"), sym9.getPlayerDataAddress("wUnlockedUnowns"), MIN_SAVE_SIZE - sym8.getPlayerDataAddress("wUnlockedUnowns"));

	js_info << "Patching wKeyItems..." << std::endl;
	it8.seek(sym8.getPlayerDataAddress("wKeyItems"));
	it9.seek(sym9.getPlayerDataAddress("wKeyItems"));
	// while it8.getByte() != 0x00
	for (int i = 0; i < NUM_KEY_ITEMS_V9; i++) {
		uint8_t key_item_v8 = it8.getByte();
		if (key_item_v8 == 0x00) {
			break;
		}
		uint8_t key_item_v9 = mapV8KeyItemToV9(key_item_v8);
		if (key_item_v9 == 0xFF) {
			js_error <<  "Key Item " << std::hex << key_item_v9 << " not found in version 9 key item list." << std::endl;
		} else {
			if (key_item_v8 != key_item_v9) {
				it9.setByte(key_item_v9);
				js_info <<  "Key Item " << std::hex << static_cast<int>(key_item_v8) << " converted to " << std::hex << static_cast<int>(key_item_v9) << std::endl;
			}
		}
		// it8.next()
		it8.next();
		// it9.next()
		it9.next();
	}

	// copy sGameData to sBackupGameData
//	js_info <<  "Copying sGameData to sBackupGameData" << std::endl;
//	for (int i = sym9.getSRAMAddress("sGameData"); i < sym9.getSRAMAddress("sGameDataEnd"); i++) {
//		it9.setByte(sym9.getSRAMAddress("sBackupGameData") + i, it9.getByte(sym9.getSRAMAddress("sGameData") + i));
//	}

	// set v9 wCurMapSceneScriptCount and wCurMapCallbackCount to 0
	// set v9 wCurMapSceneScriptPointer word to 0
	// this is done to prevent the game from running any map scripts on load
	js_info <<  "Set wCurMapSceneScriptCount and wCurMapCallbackCount to 0..." << std::endl;
	it9.seek(sym9.getPlayerDataAddress("wCurMapSceneScriptCount"));
	it9.setByte(0);
	it9.seek(sym9.getPlayerDataAddress("wCurMapCallbackCount"));
	it9.setByte(0);
	js_info <<  "Set wCurMapSceneScriptPointer to 0..." << std::endl;
	it9.seek(sym9.getPlayerDataAddress("wCurMapSceneScriptPointer"));
	it9.setWord(0);

	// write the new save version number big endian
	js_info <<  "Writing the new save version number" << std::endl;
	uint16_t new_save_version = 0x09;
	save9.setWordBE(SAVE_VERSION_ABS_ADDRESS, new_save_version);

	// write the new checksums to the version 9 save file
	js_info <<  "Writing the new checksums" << std::endl;
	uint16_t new_checksum = calculateSaveChecksum(save9, sym9.getSRAMAddress("sGameData"), sym9.getSRAMAddress("sGameDataEnd"));
	save9.setWord(SAVE_CHECKSUM_ABS_ADDRESS, new_checksum);

	// write new backup checksums to the version 9 save file
	uint16_t new_backup_checksum = calculateSaveChecksum(save9, sym9.getSRAMAddress("sBackupGameData"), sym9.getSRAMAddress("sBackupGameDataEnd"));
	save9.setWord(SAVE_BACKUP_CHECKSUM_ABS_ADDRESS, new_backup_checksum);

	js_info <<  "Sucessfully patched to 3.1.0 save version 9!" << std::endl;
	return true;

}

// converts a version 8 key item to a version 8 key item
uint8_t mapV8KeyItemToV9(uint8_t v8) {
	std::unordered_map<uint8_t, uint8_t> indexMap = {
		{0x01, 0x01},  // BICYCLE
		{0x02, 0x02},  // OLD_ROD
		{0x03, 0x03},  // GOOD_ROD
		{0x04, 0x04},  // SUPER_ROD
		{0x05, 0x05},  // ITEMFINDER
		{0x06, 0x06},  // COIN_CASE
		{0x07, 0x07},  // APRICORN_BOX
		{0x08, 0x08},  // WING_CASE
		{0x09, 0x0a},  // TYPE_CHART
		{0x0a, 0x0b},  // GBC_SOUNDS
		{0x0b, 0x0c},  // BLUE_CARD
		{0x0c, 0x0d},  // SQUIRTBOTTLE
		{0x0d, 0x0e},  // SILPHSCOPE2
		{0x0e, 0x0f},  // MYSTERY_EGG
		{0x0f, 0x10},  // SECRETPOTION
		{0x10, 0x11},  // GO_GOGGLES
		{0x11, 0x12},  // RED_SCALE
		{0x12, 0x13},  // CARD_KEY
		{0x13, 0x14},  // BASEMENT_KEY
		{0x14, 0x15},  // LOST_ITEM
		{0x15, 0x16},  // MACHINE_PART
		{0x16, 0x17},  // RAINBOW_WING
		{0x17, 0x18},  // SILVER_WING
		{0x18, 0x19},  // CLEAR_BELL
		{0x19, 0x1a},  // GS_BALL
		{0x1a, 0x1b},  // S_S_TICKET
		{0x1b, 0x1c},  // PASS
		{0x1c, 0x1d},  // ORANGETICKET
		{0x1d, 0x1e},  // MYSTICTICKET
		{0x1e, 0x1f},  // OLD_SEA_MAP
		{0x1f, 0x20},  // HARSH_LURE
		{0x20, 0x21},  // POTENT_LURE
		{0x21, 0x22},  // MALIGN_LURE
		{0x22, 0x23},  // SHINY_CHARM
		{0x23, 0x24},  // OVAL_CHARM
		{0x24, 0x25},  // CATCH_CHARM
	};

	// return the corresponding version 8 key item or 0xFF if not found
	return indexMap.find(v8) != indexMap.end() ? indexMap[v8] : 0xFF;
}