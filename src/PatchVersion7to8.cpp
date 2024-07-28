#include "PatchVersion7to8.h"
#include "CommonPatchFunctions.h"
#include "SymbolDatabase.h"
#include "Logging.h"

bool patchVersion7to8(SaveBinary& save7, SaveBinary& save8) {
	// copy the old save file to the new save file
	save8 = save7;

	// create the iterators
	SaveBinary::Iterator it7(save7, 0);
	SaveBinary::Iterator it8(save8, 0);

	// Load the version 7 and version 8 sym files
	SymbolDatabase sym7(VERSION_7_SYMBOL_FILE);
	SymbolDatabase sym8(VERSION_8_SYMBOL_FILE);

	SourceDest sd = {it7, it8, sym7, sym8};

	// get the checksum word from the version 7 save file
	uint16_t save_checksum = save7.getWord(SAVE_CHECKSUM_ABS_ADDRESS);

	// verify the checksum of the version 7 file matches the calculated checksum
	// calculate the checksum from lookup symbol name "sGameData" to "sGameDataEnd"
	uint16_t calculated_checksum = calculateChecksum(save7, sym7.getSRAMAddress("sGameData"), sym7.getSRAMAddress("sGameDataEnd"));
	if (save_checksum != calculated_checksum) {
		js_error <<  "Checksum mismatch! Expected: " << std::hex << calculated_checksum << ", got: " << save_checksum << std::endl;
		return false;
	}

	// check the backup checksum word from the version 7 save file
	uint16_t backup_checksum = save7.getWord(SAVE_BACKUP_CHECKSUM_ABS_ADDRESS);
	// verify the backup checksum of the version 7 file matches the calculated checksum
	// calculate the checksum from lookup symbol name "sBackupGameData" to "sBackupGameDataEnd"
	uint16_t calculated_backup_checksum = calculateChecksum(save7, sym7.getSRAMAddress("sBackupGameData"), sym7.getSRAMAddress("sBackupGameDataEnd"));
	if (backup_checksum != calculated_backup_checksum) {
		js_error <<  "Backup checksum mismatch! Expected: " << std::hex << calculated_backup_checksum << ", got: " << backup_checksum << std::endl;
		return false;
	}

	// check if the player in the Mon Healing Center P.C. 2nd Floor
	uint8_t map_group = it7.getByte(sym7.getMapDataAddress("wMapGroup"));
	it7.next();
	uint8_t map_num = it7.getByte();
	if (map_group != MON_CENTER_2F_GROUP && map_num != MON_CENTER_2F_MAP) {
		js_error <<  "Player is not in the Mon Healing Center P.C. 2nd Floor. Go to where you heal in game, and head upstairs. Then re-save your game and try again." << std::endl;
		return false;
	}

	// Create vector to store seen mons
	std::vector<uint16_t> seen_mons;
	// Create vector to store caught mons
	std::vector<uint16_t> caught_mons;

	migrateBoxData(sd, "sNewBox");
	migrateBoxData(sd, "sBackupNewBox");

	// copy sBoxMons1 to sBoxMons1A
	js_info <<  "Copying from sBoxMons1 to sBoxMons1A..." << std::endl;
	copyDataBlock(sd, sym7.getSRAMAddress("sBoxMons1"), sym8.getSRAMAddress("sBoxMons1A"), MONDB_ENTRIES_A_V8 * SAVEMON_STRUCT_LENGTH);

	clearBox(it8, sym8, "sBoxMons1B", MONDB_ENTRIES_B_V8);
	clearBox(it8, sym8, "sBoxMons1C", MONDB_ENTRIES_C_V8);

	// copy sBoxMons2 to SBoxMons2A
	js_info <<  "Copying from sBoxMons2 to sBoxMons2A..." << std::endl;
	copyDataBlock(sd, sym7.getSRAMAddress("sBoxMons2"), sym8.getSRAMAddress("sBoxMons2A"), MONDB_ENTRIES_A_V8 * SAVEMON_STRUCT_LENGTH);

	clearBox(it8, sym8, "sBoxMons2B", MONDB_ENTRIES_B_V8);
	clearBox(it8, sym8, "sBoxMons2C", MONDB_ENTRIES_C_V8);

	// Patching sBoxMons1A if checksums match
	js_info <<  "Checking sBoxMons1A checksums..." << std::endl;
	for (int i = 0; i < MONDB_ENTRIES_A_V8; i++) {
		it8.seek(sym8.getSRAMAddress("sBoxMons1A") + i * SAVEMON_STRUCT_LENGTH);
		uint16_t calc_checksum = calculateNewboxChecksum(save8, sym8.getSRAMAddress("sBoxMons1A") + i * SAVEMON_STRUCT_LENGTH);
		uint16_t cur_checksum = extractStoredNewboxChecksum(save8, sym8.getSRAMAddress("sBoxMons1A") + i * SAVEMON_STRUCT_LENGTH);
		if (calc_checksum == cur_checksum) {
			// Get values from version 7 save file
			uint16_t species = it8.getByte();
			it8.next();
			uint8_t item = it8.getByte();
			uint8_t caught_location = it8.getByte(sym8.getSRAMAddress("sBoxMons1A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_CAUGHTLOCATION);
			uint8_t caught_ball = it8.getByte(sym8.getSRAMAddress("sBoxMons1A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_CAUGHTBALL) & CAUGHT_BALL_MASK;
			// convert species & form
			convertSpeciesAndForm(sd, sym8.getSRAMAddress("sBoxMons1A"), i, SAVEMON_STRUCT_LENGTH, SAVEMON_EXTSPECIES, species, seen_mons, caught_mons);
			// convert item
			uint8_t item_v8 = mapV7ItemToV8(item);
			if (item_v8 == 0xFF) {
				js_error <<  "Item " << std::hex << static_cast<int>(item) << " not found in version 8 item list." << std::endl;
			} else {
				if (item != item_v8) {
					js_info <<  "Item " << std::hex << static_cast<int>(item) << " converted to " << std::hex << static_cast<int>(item_v8) << std::endl;
				}
				it8.setByte(sym8.getSRAMAddress("sBoxMons1A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_ITEM, item_v8);
			}
			// convert caught location
			uint8_t caught_location_v8 = mapV7LandmarkToV8(caught_location);
			if (caught_location_v8 == 0xFF) {
				js_error <<  "Landmark " << std::hex << static_cast<int>(caught_location) << " not found in version 8 landmark list." << std::endl;
			} else {
				if (caught_location != caught_location_v8) {
					js_info <<  "Landmark " << std::hex << static_cast<int>(caught_location) << " converted to " << std::hex << static_cast<int>(caught_location_v8) << std::endl;
				}
				it8.setByte(sym8.getSRAMAddress("sBoxMons1A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_CAUGHTLOCATION, caught_location_v8);
			}
			// convert caught ball
			uint8_t caught_ball_v8 = mapV7ItemToV8(caught_ball);
			if (caught_ball_v8 == 0xFF) {
				js_error <<  "Ball " << std::hex << static_cast<int>(caught_ball) << " not found in version 8 item list." << std::endl;
			} else {
				if (caught_ball != caught_ball_v8) {
					js_info <<  "Ball " << std::hex << static_cast<int>(caught_ball) << " converted to " << std::hex << static_cast<int>(caught_ball_v8) << std::endl;
				}
				uint8_t caught_ball_byte = it8.getByte(sym8.getSRAMAddress("sBoxMons1A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_CAUGHTBALL);
				caught_ball_byte &= ~CAUGHT_BALL_MASK;
				caught_ball_byte |= caught_ball_v8 & CAUGHT_BALL_MASK;
				it8.setByte(caught_ball_v8);
			}
			// write the new checksum
			writeNewboxChecksum(save8, sym8.getSRAMAddress("sBoxMons1A") + i * SAVEMON_STRUCT_LENGTH);
		}
	}

	// Patching sBoxMons2A if checksums match
	js_info <<  "Checking sBoxMons2A checksums..." << std::endl;
	for (int i = 0; i < MONDB_ENTRIES_A_V8; i++) {
		it8.seek(sym8.getSRAMAddress("sBoxMons2A") + i * SAVEMON_STRUCT_LENGTH);
		uint16_t calc_checksum = calculateNewboxChecksum(save8, sym8.getSRAMAddress("sBoxMons2A") + i * SAVEMON_STRUCT_LENGTH);
		uint16_t cur_checksum = extractStoredNewboxChecksum(save8, sym8.getSRAMAddress("sBoxMons2A") + i * SAVEMON_STRUCT_LENGTH);
		if (calc_checksum == cur_checksum) {
			// Get values from version 7 save file
			uint16_t species = it8.getByte();
			it8.next();
			uint8_t item = it8.getByte();
			uint8_t caught_location = it8.getByte(sym8.getSRAMAddress("sBoxMons2A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_CAUGHTLOCATION);
			uint8_t caught_ball = it8.getByte(sym8.getSRAMAddress("sBoxMons2A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_CAUGHTBALL) & CAUGHT_BALL_MASK;
			// convert species and form
			convertSpeciesAndForm(sd, sym8.getSRAMAddress("sBoxMons2A"), i, SAVEMON_STRUCT_LENGTH, SAVEMON_EXTSPECIES, species, seen_mons, caught_mons);
			// convert item
			uint8_t item_v8 = mapV7ItemToV8(item);
			if (item_v8 == 0xFF) {
				js_error <<  "Item " << std::hex << static_cast<int>(item) << " not found in version 8 item list." << std::endl;
			} else {
				if (item != item_v8) {
					js_info <<  "Item " << std::hex << static_cast<int>(item) << " converted to " << std::hex << static_cast<int>(item_v8) << std::endl;
				}
				it8.setByte(sym8.getSRAMAddress("sBoxMons2A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_ITEM, item_v8);
			}
			// convert caught location
			uint8_t caught_location_v8 = mapV7LandmarkToV8(caught_location);
			if (caught_location_v8 == 0xFF) {
				js_error <<  "Landmark " << std::hex << static_cast<int>(caught_location) << " not found in version 8 landmark list." << std::endl;
			} else {
				if (caught_location != caught_location_v8) {
					js_info <<  "Landmark " << std::hex << static_cast<int>(caught_location) << " converted to " << std::hex << static_cast<int>(caught_location_v8) << std::endl;
				}
				it8.setByte(sym8.getSRAMAddress("sBoxMons2A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_CAUGHTLOCATION, caught_location_v8);
			}
			// convert caught ball
			uint8_t caught_ball_v8 = mapV7ItemToV8(caught_ball);
			if (caught_ball_v8 == 0xFF) {
				js_error <<  "Ball " << std::hex << static_cast<int>(caught_ball) << " not found in version 8 item list." << std::endl;
			} else {
				if (caught_ball != caught_ball_v8) {
					js_info <<  "Ball " << std::hex << static_cast<int>(caught_ball) << " converted to " << std::hex << static_cast<int>(caught_ball_v8) << std::endl;
				}
				uint8_t caught_ball_byte = it8.getByte(sym8.getSRAMAddress("sBoxMons2A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_CAUGHTBALL);
				caught_ball_byte &= ~CAUGHT_BALL_MASK;
				caught_ball_byte |= caught_ball_v8 & CAUGHT_BALL_MASK;
				it8.setByte(caught_ball_v8);
			}
			// write the new checksum
			writeNewboxChecksum(save8, sym8.getSRAMAddress("sBoxMons2A") + i * SAVEMON_STRUCT_LENGTH);
		}
	}

	// copy from sLinkBattleResults to sLinkBattleStatsEnd
	js_info <<  "Copying from sLinkBattleResults to sLinkBattleStatsEnd..." << std::endl;
	copyDataBlock(sd, sym7.getSRAMAddress("sLinkBattleResults"), sym8.getSRAMAddress("sLinkBattleResults"), sym7.getSRAMAddress("sLinkBattleStatsEnd") - sym7.getSRAMAddress("sLinkBattleResults"));

	// copy from sBattleTowerChallengeState to (sBT_OTMonParty3 + BATTLETOWER_PARTYDATA_SIZE + 1)
	js_info <<  "Copying from sBattleTowerChallengeState to sBT_OTMonParty3..." << std::endl;
	copyDataBlock(sd, sym7.getSRAMAddress("sBattleTowerChallengeState"), sym8.getSRAMAddress("sBattleTowerChallengeState"), sym7.getSRAMAddress("sBT_OTMonParty3") + BATTLETOWER_PARTYDATA_SIZE + 1 - sym7.getSRAMAddress("sBattleTowerChallengeState"));

	// copy from sPartyMail to sSaveVersion
	js_info <<  "Copying from sPartyMail to sSaveVersion..." << std::endl;
	copyDataBlock(sd, sym7.getSRAMAddress("sPartyMail"), sym8.getSRAMAddress("sPartyMail"), sym7.getSRAMAddress("sSaveVersion") - sym7.getSRAMAddress("sPartyMail"));

	// copy sUpgradeStep to sWritingBackup
	js_info <<  "Copying from sUpgradeStep to sWritingBackup..." << std::endl;
	copyDataBlock(sd, sym7.getSRAMAddress("sUpgradeStep"), sym8.getSRAMAddress("sUpgradeStep"), sym7.getSRAMAddress("sWritingBackup") + 1 - sym7.getSRAMAddress("sUpgradeStep"));

	// copy sRTCStatusFlags to sLuckyIDNumber
	js_info <<  "Copying from sRTCStatusFlags to sLuckyIDNumber..." << std::endl;
	copyDataBlock(sd, sym7.getSRAMAddress("sRTCStatusFlags"), sym8.getSRAMAddress("sRTCStatusFlags"), sym7.getSRAMAddress("sLuckyIDNumber") + 2 - sym7.getSRAMAddress("sRTCStatusFlags"));

	// copy sOptions to sGameData
	js_info <<  "Copying from sOptions to sGameData..." << std::endl;
	copyDataBlock(sd, sym7.getSRAMAddress("sOptions"), sym8.getSRAMAddress("sOptions"), sym7.getSRAMAddress("sGameData") - sym7.getSRAMAddress("sOptions"));

	// copy bytes from wPlayerData to wObjectStructs - 1 from version 7 to version 8
	js_info <<  "Copying from wPlayerData to wObjectStructs..." << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wPlayerData"), sym8.getPlayerDataAddress("wPlayerData"), sym7.getPlayerDataAddress("wObjectStructs") - sym7.getPlayerDataAddress("wPlayerData"));
	
	js_info <<  "Patching Object Structs..." << std::endl;

	// version 8 expanded each object struct by 1 byte to add the palette index byte at the end.
	// we need to copy the lower nybble of OBJECT_PALETTE_V7 to the new OBJECT_PAL_INDEX_V8
	// and then copy the rest of the object struct from version 7 to version 8
	for (int i = 0; i < NUM_OBJECT_STRUCTS; i++) {
		it7.seek(sym7.getPlayerDataAddress("wObjectStructs") + i * OBJECT_LENGTH_V7);
		it8.seek(sym8.getPlayerDataAddress("wObjectStructs") + i * OBJECT_LENGTH_V8);

		// string is equal to "wObject" + string(i) + "Structs"
		std::string objectStruct;
		if (i == 0) {
			objectStruct = "wPlayerStruct";
		} else {
			objectStruct = "wObject" + std::to_string(i) + "Struct";
		}
		// assert that current address is equal to objectStruct
		if (it7.getAddress() != sym7.getPlayerDataAddress(objectStruct)) {
			js_error <<  "Unexpected address for " << objectStruct << " in version 7 save file: " << std::hex << it7.getAddress() << ", expected: " << sym7.getPlayerDataAddress(objectStruct) << std::endl;
		}
		if (it8.getAddress() != sym8.getPlayerDataAddress(objectStruct)) {
			js_error <<  "Unexpected address for " << objectStruct << " in version 8 save file: " << std::hex << it8.getAddress() << ", expected: " << sym8.getPlayerDataAddress(objectStruct) << std::endl;
		}
		it8.copy(it7, OBJECT_LENGTH_V7);
		// copy the lower nybble of OBJECT_PALETTE_V7 to OBJECT_PAL_INDEX_V8
		uint8_t palette = save7.getByte(sym7.getPlayerDataAddress("wObjectStructs") + i * OBJECT_LENGTH_V7 + OBJECT_PALETTE_V7) & 0x0F;
		js_info <<  objectStruct << " Palette: " << std::hex << static_cast<int>(palette) << std::endl;
		it8.setByte(palette);
	}

	// copy bytes from wObjectStructsEnd to wBattleFactorySwapCount
	js_info <<  "Copying from wObjectStructsEnd to wBattleFactorySwapCount..." << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wObjectStructsEnd"), sym8.getPlayerDataAddress("wObjectStructsEnd"), sym7.getPlayerDataAddress("wBattleFactorySwapCount") + 1 - sym7.getPlayerDataAddress("wObjectStructsEnd"));

	// copy bytes from wMapObjects to wEnteredMapFromContinue
	js_info <<  "Copying from wMapObjects to wEnteredMapFromContinue..." << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wMapObjects"), sym8.getPlayerDataAddress("wMapObjects"), sym7.getPlayerDataAddress("wEnteredMapFromContinue") - sym7.getPlayerDataAddress("wMapObjects"));
	
	js_info <<  "Copy wEnteredMapFromContinue" << std::endl;
	// copy it7 wEnteredMapFromContinue to it8 wEnteredMapFromContinue
	// assert that the address is correct
	if (it7.getAddress() != sym7.getPlayerDataAddress("wEnteredMapFromContinue")) {
		js_error <<  "Unexpected address for wEnteredMapFromContinue in version 7 save file: " << std::hex << it7.getAddress() << std::endl;
	}
	it8.setByte(sym8.getPlayerDataAddress("wEnteredMapFromContinue"), it7.getByte());
	js_info <<  "Copy wStatusFlags3" << std::endl;
	// copy it7 wStatusFlags3 to it8 wStatusFlags3
	copyDataByte(sd, sym7.getPlayerDataAddress("wStatusFlags3"), sym8.getPlayerDataAddress("wStatusFlags3"));

	js_info <<  "Copying from wTimeOfDayPal to wTMsHMsEnd..." << std::endl;
	// copy from it7 wTimeOfDayPal to it7 wTMsHMsEnd
	copyDataBlock(sd, sym7.getPlayerDataAddress("wTimeOfDayPal"), sym8.getPlayerDataAddress("wTimeOfDayPal"), sym7.getPlayerDataAddress("wTMsHMsEnd") - sym7.getPlayerDataAddress("wTimeOfDayPal"));

	js_info <<  "Patching wTMsHMs..." << std::endl;
	// set it8 wKeyItems -> wKeyItemsEnd to 0x00
	// assert that the address is correct
	if (it8.getAddress() != sym8.getPlayerDataAddress("wKeyItems")) {
		js_error <<  "Unexpected address for wKeyItems in version 8 save file: " << std::hex << it8.getAddress() << std::endl;
	}
	while(it8.getAddress() < sym8.getPlayerDataAddress("wKeyItemsEnd")) {
		it8.setByte(0x00);
		it8.next();
	}

	js_info <<  "Patching wKeyItems..." << std::endl;
	it7.seek(sym7.getPlayerDataAddress("wKeyItems"));
	it8.seek(sym8.getPlayerDataAddress("wKeyItems"));
	// it7 wKeyItems is a bit flag array of NUM_KEY_ITEMS_V7 bits. If v7 bit is set, lookup the bit index in the map and write the index to the next byte in it8.
	for (int i = 0; i < NUM_KEY_ITEMS_V7; i++) {
		// check if the bit is set
		if (it7.getByte() & (1 << (i % 8))) {
			// get the key item index is equal to the bit index
			uint8_t keyItemIndex = i;
			// map the version 7 key item to the version 8 key item
			uint8_t keyItemIndexV8 = mapV7KeyItemToV8(keyItemIndex);
			// if the key item is found write to the next byte in it8.
			if (keyItemIndexV8 != 0xFF) {
				// print found key itemv7 and converted key itemv8
				if (keyItemIndex != keyItemIndexV8){
					js_info <<  "Key Item " << std::hex << static_cast<int>(keyItemIndex) << " converted to " << std::hex << static_cast<int>(keyItemIndexV8) << std::endl;
				}
				it8.setByte(keyItemIndexV8);
				it8.next();
			} else {
				// warn we couldn't find v7 key item in v8
				js_error <<  "Key Item " << std::hex << keyItemIndex << " not found in version 8 key item list." << std::endl;
			}
		}
		if (i % 8 == 7) {
			it7.next();
		}
	}
	// write 0x00 to the end of wKeyItems
	it8.setByte(0x00);

	js_info <<  "Copy wNumItems..." << std::endl;
	// Copy it7 wNumItems to it8 wNumItems
	it7.seek(sym7.getPlayerDataAddress("wNumItems"));
	it8.seek(sym8.getPlayerDataAddress("wNumItems"));
	// save the number of items
	uint8_t numItemsv7 = it7.getByte();
	uint8_t numItemsv8 = 0;
	it8.setByte(numItemsv7);
	it7.next();
	it8.next();
	// wItems is in the structure of ITEM_ID, QUANTITY. With an ITEM_ID of 0xFF indicating the end of the list.
	// we need to convert the ITEM_ID from version 7 to version 8 and copy the QUANTITY.
	js_info <<  "Patching wItems..." << std::endl;
	// assert that the address is correct
	if (it7.getAddress() != sym7.getPlayerDataAddress("wItems")) {
		js_error <<  "Unexpected address for wItems in version 7 save file: " << std::hex << it7.getAddress() << std::endl;
	}
	if (it8.getAddress() != sym8.getPlayerDataAddress("wItems")) {
		js_error <<  "Unexpected address for wItems in version 8 save file: " << std::hex << it8.getAddress() << std::endl;
	}
	// for numItemsv7, convert the ITEM_ID from version 7 to version 8 and copy the QUANTITY
	for (int i = 0; i < numItemsv7 + 1; i++) {
		// get the ITEM_ID from version 7
		uint8_t itemIDV7 = it7.getByte();
		it7.next();
		if (itemIDV7 == 0xFF) {
			it8.setByte(0xFF);
			break;
		}
		// map the version 7 item to the version 8 item
		uint8_t itemIDV8 = mapV7ItemToV8(itemIDV7);
		// if the item is found write to the next byte in it8.
		if (itemIDV8 != 0xFF) {
			// print found itemv7 and converted itemv8
			if (itemIDV7 != itemIDV8){
				js_info <<  "Item ID " << std::hex << static_cast<int>(itemIDV7) << " converted to " << std::hex << static_cast<int>(itemIDV8) << std::endl;
			}
			numItemsv8++;
			it8.setByte(itemIDV8);
			it8.next();
			// copy the quantity
			it8.setByte(it7.getByte());
			it7.next();
			it8.next();
		} else {
			// warn we couldn't find v7 item in v8
			js_error <<  "Item ID " << std::hex << itemIDV7 << " not found in version 8 item list." << std::endl;
			// skip this v7 item and move to the next v7 item
			it7.next();
			it7.next();
		}
	}
	// update the number of items v8
	it8.setByte(sym8.getPlayerDataAddress("wNumItems"), numItemsv8);

	js_info <<  "Copy wNumMedicine..." << std::endl;
	// Copy it7 wNumMedicine to it8 wNumMedicine
	it7.seek(sym7.getPlayerDataAddress("wNumMedicine"));
	it8.seek(sym8.getPlayerDataAddress("wNumMedicine"));
	// save the number of medcine items
	uint8_t numMedicinev7 = it7.getByte();
	uint8_t numMedicinev8 = 0;
	it8.setByte(numMedicinev7);
	it7.next();
	it8.next();
	// wMedicine is in the structure of ITEM_ID, QUANTITY. With an ITEM_ID of 0xFF indicating the end of the list.
	// we need to convert the ITEM_ID from version 7 to version 8 and copy the QUANTITY.
	js_info <<  "Patching wMedicine..." << std::endl;
	// assert that the address is correct
	if (it7.getAddress() != sym7.getPlayerDataAddress("wMedicine")) {
		js_error <<  "Unexpected address for wMedicine in version 7 save file: " << std::hex << it7.getAddress() << std::endl;
	}
	// for numMedicinev7, convert the ITEM_ID from version 7 to version 8 and copy the QUANTITY
	for (int i = 0; i < numMedicinev7 + 1; i++) {
		// get the ITEM_ID from version 7
		uint8_t itemIDV7 = it7.getByte();
		it7.next();
		if (itemIDV7 == 0xFF) {
			it8.setByte(0xFF);
			break;
		}
		// map the version 7 item to the version 8 item
		uint8_t itemIDV8 = mapV7ItemToV8(itemIDV7);
		// if the item is found write to the next byte in it8.
		if (itemIDV8 != 0xFF) {
			// print found itemv7 and converted itemv8
			if (itemIDV7 != itemIDV8){
				js_info <<  "Item ID " << std::hex << static_cast<int>(itemIDV7) << " converted to " << std::hex << static_cast<int>(itemIDV8) << std::endl;
			}
			numMedicinev8++;
			it8.setByte(itemIDV8);
			it8.next();
			// copy the quantity
			it8.setByte(it7.getByte());
			it7.next();
			it8.next();
		} else {
			// warn we couldn't find v7 item in v8
			js_error <<  "Item ID " << std::hex << itemIDV7 << " not found in version 8 item list." << std::endl;
			// skip this v7 item and move to the next v7 item
			it7.next();
			it7.next();
		}
	}
	// update the number of medicine v8
	it8.setByte(sym8.getPlayerDataAddress("wNumMedicine"), numMedicinev8);

	js_info <<  "Copy wNumBalls..." << std::endl;
	// Copy it7 wNumBalls to it8 wNumBalls
	it7.seek(sym7.getPlayerDataAddress("wNumBalls"));
	it8.seek(sym8.getPlayerDataAddress("wNumBalls"));
	// save the number of ball items
	uint8_t numBallsv7 = it7.getByte();
	uint8_t numBallsV8 = 0;
	it8.setByte(numBallsv7);
	it7.next();
	it8.next();
	// wBalls is in the structure of ITEM_ID, QUANTITY. With an ITEM_ID of 0xFF indicating the end of the list.
	// we need to convert the ITEM_ID from version 7 to version 8 and copy the QUANTITY.
	js_info <<  "Patching wBalls..." << std::endl;
	// assert that the address is correct
	if (it7.getAddress() != sym7.getPlayerDataAddress("wBalls")) {
		js_error <<  "Unexpected address for wBalls in version 7 save file: " << std::hex << it7.getAddress() << std::endl;
	}
	// for numBallsv7, convert the ITEM_ID from version 7 to version 8 and copy the QUANTITY
	for (int i = 0; i < numBallsv7 + 1; i++) {
		// get the ITEM_ID from version 7
		uint8_t itemIDV7 = it7.getByte();
		it7.next();
		if (itemIDV7 == 0xFF) {
			it8.setByte(0xFF);
			break;
		}
		// map the version 7 item to the version 8 item
		uint8_t itemIDV8 = mapV7ItemToV8(itemIDV7);
		// if the item is found write to the next byte in it8.
		if (itemIDV8 != 0xFF) {
			// print found itemv7 and converted itemv8
			if (itemIDV7 != itemIDV8){
				js_info <<  "Item ID " << std::hex << static_cast<int>(itemIDV7) << " converted to " << std::hex << static_cast<int>(itemIDV8) << std::endl;
			}
			numBallsV8++;
			it8.setByte(itemIDV8);
			it8.next();
			// copy the quantity
			it8.setByte(it7.getByte());
			it7.next();
			it8.next();
		} else {
			// warn we couldn't find v7 item in v8
			js_error <<  "Item ID " << std::hex << itemIDV7 << " not found in version 8 item list." << std::endl;
			// skip this v7 item and move to the next v7 item
			it7.next();
			it7.next();
		}
	}
	// update the number of balls v8
	it8.setByte(sym8.getPlayerDataAddress("wNumBalls"), numBallsV8);

	js_info <<  "Copy wNumBerries..." << std::endl;
	// Copy it7 wNumBerries to it8 wNumBerries
	it7.seek(sym7.getPlayerDataAddress("wNumBerries"));
	it8.seek(sym8.getPlayerDataAddress("wNumBerries"));
	// save the number of berry items
	uint8_t numBerriesv7 = it7.getByte();
	uint8_t numBerriesv8 = 0;
	it8.setByte(numBerriesv7);
	it7.next();
	it8.next();
	// wBerries is in the structure of ITEM_ID, QUANTITY. With an ITEM_ID of 0xFF indicating the end of the list.
	// we need to convert the ITEM_ID from version 7 to version 8 and copy the QUANTITY.
	js_info <<  "Patching wBerries..." << std::endl;
	// assert that the address is correct
	if (it7.getAddress() != sym7.getPlayerDataAddress("wBerries")) {
		js_error <<  "Unexpected address for wBerries in version 7 save file: " << std::hex << it7.getAddress() << std::endl;
	}
	// for numBerriesv7, convert the ITEM_ID from version 7 to version 8 and copy the QUANTITY
	for (int i = 0; i < numBerriesv7 + 1; i++) {
		// get the ITEM_ID from version 7
		uint8_t itemIDV7 = it7.getByte();
		it7.next();
		if (itemIDV7 == 0xFF) {
			it8.setByte(0xFF);
			break;
		}
		// map the version 7 item to the version 8 item
		uint8_t itemIDV8 = mapV7ItemToV8(itemIDV7);
		// if the item is found write to the next byte in it8.
		if (itemIDV8 != 0xFF) {
			// print found itemv7 and converted itemv8
			if (itemIDV7 != itemIDV8){
				js_info <<  "Item ID " << std::hex << static_cast<int>(itemIDV7) << " converted to " << std::hex << static_cast<int>(itemIDV8) << std::endl;
			}
			numBerriesv8++;
			it8.setByte(itemIDV8);
			it8.next();
			// copy the quantity
			it8.setByte(it7.getByte());
			it7.next();
			it8.next();
		} else {
			// warn we couldn't find v7 item in v8
			js_error <<  "Item ID " << std::hex << itemIDV7 << " not found in version 8 item list." << std::endl;
			// skip this v7 item and move to the next v7 item
			it7.next();
			it7.next();
		}
	}
	// update the number of berries v8
	it8.setByte(sym8.getPlayerDataAddress("wNumBerries"), numBerriesv8);

	js_info <<  "Copy wNumPCItems..." << std::endl;
	// Copy it7 wNumPCItems to it8 wNumPCItems
	it7.seek(sym7.getPlayerDataAddress("wNumPCItems"));
	it8.seek(sym8.getPlayerDataAddress("wNumPCItems"));
	// save the number of pc items
	uint8_t numPCItems = it7.getByte();
	uint8_t numPCItemsV8 = 0;
	it8.setByte(numPCItems);
	it7.next();
	it8.next();
	// wPCItems is in the structure of ITEM_ID, QUANTITY. With an ITEM_ID of 0xFF indicating the end of the list.
	// we need to convert the ITEM_ID from version 7 to version 8 and copy the QUANTITY.
	js_info <<  "Patching wPCItems..." << std::endl;
	// assert that the address is correct
	if (it7.getAddress() != sym7.getPlayerDataAddress("wPCItems")) {
		js_error <<  "Unexpected address for wPCItems in version 7 save file: " << std::hex << it7.getAddress() << std::endl;
	}
	// for numPCItems, convert the ITEM_ID from version 7 to version 8 and copy the QUANTITY
	for (int i = 0; i < numPCItems + 1; i++) {
		// get the ITEM_ID from version 7
		uint8_t itemIDV7 = it7.getByte();
		it7.next();
		if (itemIDV7 == 0xFF) {
			it8.setByte(0xFF);
			break;
		}
		// map the version 7 item to the version 8 item
		uint8_t itemIDV8 = mapV7ItemToV8(itemIDV7);
		// if the item is found write to the next byte in it8.
		if (itemIDV8 != 0xFF) {
			// print found itemv7 and converted itemv8
			if (itemIDV7 != itemIDV8){
				js_info <<  "Item ID " << std::hex << static_cast<int>(itemIDV7) << " converted to " << std::hex << static_cast<int>(itemIDV8) << std::endl;
			}
			numPCItemsV8++;
			it8.setByte(itemIDV8);
			it8.next();
			// copy the quantity
			it8.setByte(it7.getByte());
			it7.next();
			it8.next();
		} else {
			// warn we couldn't find v7 item in v8
			js_error <<  "Item ID " << std::hex << itemIDV7 << " not found in version 8 item list." << std::endl;
			// skip this v7 item and move to the next v7 item
			it7.next();
			it7.next();
		}
	}
	// update the number of pc items v8
	it8.setByte(sym8.getPlayerDataAddress("wNumPCItems"), numPCItemsV8);

	js_info <<  "Copy wApricorns..." << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wApricorns"), sym8.getPlayerDataAddress("wApricorns"), NUM_APRICORNS);

	js_info <<  "Copy from w****gearFlags to wAlways0SceneID..." << std::endl;
	it8.copy(it7, sym7.getPlayerDataAddress("wAlways0SceneID") - sym7.getPlayerDataAddress("wPokegearFlags"));

	js_info <<  "copy from wAlways0SceneID to wEcru****HouseSceneID + 1..." << std::endl;
	it8.copy(it7, sym7.getPlayerDataAddress("wEcruteakHouseSceneID") + 1 - sym7.getPlayerDataAddress("wAlways0SceneID"));

	// clear wEcruteakPokecenter1FSceneID as it is no longer used
	js_info <<  "Clear wEcru********center1FSceneID..." << std::endl;
	it8.setByte(0x00);
	it7.next();
	it8.next();

	// copy from wElmsLabSceneID to wEventFlags
	js_info <<  "Copy from wE***LabSceneID to wEventFlags..." << std::endl;
	it8.copy(it7, sym7.getPlayerDataAddress("wEventFlags") - sym7.getPlayerDataAddress("wElmsLabSceneID"));

	// clear it8 wEventFlags
	js_info <<  "Clear wEventFlags..." << std::endl;
	it8.seek(sym8.getPlayerDataAddress("wEventFlags"));
	for (int i = 0; i < NUM_EVENTS; i++) {
		it8.setByte(0x00);
		it8.next();
	}
	it8.seek(sym8.getPlayerDataAddress("wEventFlags"));

	// wEventFlags is a flag_array of NUM_EVENTS bits. If v7 bit is set, lookup the bit index in the map and set the corresponding bit in v8
	js_info <<  "Patching wEventFlags..." << std::endl;
	for (int i = 0; i < NUM_EVENTS; i++) {
		// seek to the byte containing the bit
		it7.seek(sym7.getPlayerDataAddress("wEventFlags") + i / 8);
		// check if the bit is set
		if (it7.getByte() & (1 << (i % 8))) {
			// get the event flag index is equal to the bit index
			uint16_t eventFlagIndex = i;
			// map the version 7 event flag to the version 8 event flag
			uint16_t eventFlagIndexV8 = mapV7EventFlagToV8(eventFlagIndex);
			// if the event flag is found set the corresponding bit in it8
			if (eventFlagIndexV8 != INVALID_EVENT_FLAG) {
				// print found event flagv7 and converted event flagv8
				if (eventFlagIndex != eventFlagIndexV8){
					js_info <<  "Event Flag " << std::dec << eventFlagIndex << " converted to " << eventFlagIndexV8 << std::endl;
				}
				// seek to the byte containing the bit
				it8.seek(sym8.getPlayerDataAddress("wEventFlags") + eventFlagIndexV8 / 8);
				// set the bit
				it8.setByte(it8.getByte() | (1 << (eventFlagIndexV8 % 8)));
			} else {
				// warn we couldn't find v7 event flag in v8
				js_warning <<  "Event Flag " << eventFlagIndex << " not found in version 8 event flag list." << std::endl;
			}
		}
	}

	// copy v7 wCurBox to v8 wCurBox
	js_info <<  "Copy wCurBox..." << std::endl;
	copyDataByte(sd, sym7.getPlayerDataAddress("wCurBox"), sym8.getPlayerDataAddress("wCurBox"));

	// set it8 wUsedObjectPals to 0x00
	js_info <<  "Clear wUsedObjectPals..." << std::endl;
	it8.seek(sym8.getPlayerDataAddress("wUsedObjectPals"));
	for (int i = 0; i < 0x10; i++) {
		it8.setByte(0x00);
		it8.next();
	}

	// set it8 wLoadedObjPal0-7 to -1
	js_info <<  "Clear wLoadedObjPal0-7..." << std::endl;
	it8.seek(sym8.getPlayerDataAddress("wLoadedObjPal0"));
	for (int i = 0; i < 8; i++) {
		it8.setByte(0xFF);
		it8.next();
	}

	// copy from wCelebiEvent to wCurMapCallbacksPointer + 2
	js_info <<  "Copy from wCel***Event to wCurMapCallbacksPointer + 2..." << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wCelebiEvent"), sym8.getPlayerDataAddress("wCelebiEvent"), sym7.getPlayerDataAddress("wCurMapCallbacksPointer") + 2 - sym7.getPlayerDataAddress("wCelebiEvent"));

	// copy from wDecoBed to wFruitTreeFlags
	js_info <<  "Copy from wDecoBed to wFruitTreeFlags..." << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wDecoBed"), sym8.getPlayerDataAddress("wDecoBed"), sym7.getPlayerDataAddress("wFruitTreeFlags") - sym7.getPlayerDataAddress("wDecoBed"));

	// Copy wFruitTreeFlags
	js_info <<  "Copy wFruitTreeFlags..." << std::endl;
	it8.copy(it7, NUM_FRUIT_TREES_V7 + 7 / 8);

	// Clear wNuzlockeLandmarkFlags
	js_info <<  "Clear wNuzlockeLandmarkFlags..." << std::endl;
	it8.seek(sym8.getPlayerDataAddress("wNuzlockeLandmarkFlags"));
	for (int i = 0; i < NUM_LANDMARKS_V8 + 7 / 8; i++) {
		it8.setByte(0x00);
		it8.next();
	}

	// wNuzlockeLandmarkFlags is a flag_array of NUM_LANDMARKS bits. If v7 bit is set, lookup the bit index in the map and set the corresponding bit in v8
	js_info <<  "Patching wNuzlockeLandmarkFlags..." << std::endl;
	it7.seek(sym7.getPlayerDataAddress("wNuzlockeLandmarkFlags"));
	it8.seek(sym8.getPlayerDataAddress("wNuzlockeLandmarkFlags"));
	for (int i = 0; i < NUM_LANDMARKS_V7; i++) {
		// check if the bit is set
		if (it7.getByte() & (1 << (i % 8))) {
			// get the landmark flag index is equal to the bit index
			uint8_t landmarkFlagIndex = i;
			// map the version 7 landmark flag to the version 8 landmark flag
			uint8_t landmarkFlagIndexV8 = mapV7LandmarkToV8(landmarkFlagIndex);
			// if the landmark flag is found set the corresponding bit in it8
			if (landmarkFlagIndexV8 != 0xFF) {
				// print found landmark flagv7 and converted landmark flagv8
				if (landmarkFlagIndex != landmarkFlagIndexV8){
					js_info <<  "Landmark Flag " << std::hex << static_cast<int>(landmarkFlagIndex) << " converted to " << std::hex << static_cast<int>(landmarkFlagIndexV8) << std::endl;
				}
				// seek to the byte containing the bit
				it8.seek(sym8.getPlayerDataAddress("wNuzlockeLandmarkFlags") + i / 8);
				// set the bit
				it8.setByte(it8.getByte() | (1 << (i % 8)));
			}
		}
		if (i % 8 == 7) {
			it7.next();
		}
	}

	// clear wHiddenGrottoContents to wCurHiddenGrotto
	js_info <<  "Clear wHiddenGrottoContents to wCurHiddenGrotto..." << std::endl;
	it8.seek(sym8.getPlayerDataAddress("wHiddenGrottoContents"));
	while (it8.getAddress() <= sym8.getPlayerDataAddress("wCurHiddenGrotto")) {
		it8.setByte(0x00);
		it8.next();
	}


	// copy from wLuckyNumberDayBuffer to wPhoneList
	js_info <<  "Copy from wLuckyNumberDayBuffer to wPhoneList..." << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wLuckyNumberDayBuffer"), sym8.getPlayerDataAddress("wLuckyNumberDayBuffer"), sym7.getPlayerDataAddress("wPhoneList") - sym7.getPlayerDataAddress("wLuckyNumberDayBuffer"));

	// Clear v8 wPhoneList
	js_info <<  "Clear wPhoneList..." << std::endl;
	it8.seek(sym8.getPlayerDataAddress("wPhoneList"));
	for (int i = 0; i < NUM_PHONE_CONTACTS_V8 + 7 / 8; i++) {
		it8.setByte(0x00);
		it8.next();
	}

	// wPhoneList has been converted to a bit flag array in version 8.
	// for each byte in v7 up to CONTACT_LIST_SIZE_V7, if the byte is non-zero, set the corresponding bit in v8
	js_info <<  "Patching wPhoneList..." << std::endl;
	it7.seek(sym7.getPlayerDataAddress("wPhoneList"));
	it8.seek(sym8.getPlayerDataAddress("wPhoneList"));
	for (int i = 0; i < CONTACT_LIST_SIZE_V7; i++) {
		// check if the byte is non-zero
		if (it7.getByte() != 0x00) {
			// map the version 7 contact to the version 8 contact
			uint8_t contactIndexV8 = it7.getByte();
			// print found contact v7 index
			js_info <<  "Found Contact Index " << std::hex << static_cast<int>(contactIndexV8) << std::endl;
			// seek to the byte containing the bit
			contactIndexV8--; // bit index starts at 0 not 1
			it8.seek(sym8.getPlayerDataAddress("wPhoneList") + (contactIndexV8 / 8));
			// set the bit
			it8.setByte(it8.getByte() | (1 << (contactIndexV8 % 8)));
		}
		it7.next();
	}

	// copy from wParkBallsRemaining to wPlayerDataEnd
	js_info <<  "Copy from wParkBallsRemaining to wPlayerDataEnd..." << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wParkBallsRemaining"), sym8.getPlayerDataAddress("wParkBallsRemaining"), sym7.getPlayerDataAddress("wPlayerDataEnd") - sym7.getPlayerDataAddress("wParkBallsRemaining"));

	// wVisitedSpawns is a flag_array of NUM_SPAWNS bits. If v7 bit is set, lookup the bit index in the map and set the corresponding bit in v8
	js_info <<  "Patching wVisitedSpawns..." << std::endl;
	it7.seek(sym7.getMapDataAddress("wVisitedSpawns"));
	it8.seek(sym8.getMapDataAddress("wVisitedSpawns"));
	// print current address
	js_info <<  "Current Address: " << std::hex << it7.getAddress() << std::endl;
	for (int i = 0; i < NUM_SPAWNS_V7; i++) {
		// check if the bit is set
		if (it7.getByte() & (1 << (i % 8))) {
			// get the spawn index is equal to the bit index
			uint8_t spawnIndex = i;
			// map the version 7 spawn to the version 8 spawn
			uint8_t spawnIndexV8 = mapV7SpawnToV8(spawnIndex);
			// if the spawn is found set the corresponding bit in it8
			if (spawnIndexV8 != 0xFF) {
				// print found spawnv7 and converted spawnv8
				if (spawnIndex != spawnIndexV8){
					js_info <<  "Spawn " << std::hex << static_cast<int>(spawnIndex) << " converted to " << std::hex << static_cast<int>(spawnIndexV8) << std::endl;
				}
				// seek to the byte containing the bit
				it8.seek(sym8.getMapDataAddress("wVisitedSpawns") + i / 8);
				// set the bit
				it8.setByte(it8.getByte() | (1 << (i % 8)));
			}
		}
		if (i % 8 == 7) {
			it7.next();
		}
	}

	// Copy from wDigWarpNumber to wCurMapDataEnd
	js_info <<  "Copy from wDigWarpNumber to wCurMapDataEnd..." << std::endl;
	copyDataBlock(sd, sym7.getMapDataAddress("wDigWarpNumber"), sym8.getMapDataAddress("wDigWarpNumber"), sym7.getMapDataAddress("wCurMapDataEnd") - sym7.getMapDataAddress("wDigWarpNumber"));

	// map the v7 wDigMapGroup and wDigMapNumber to v8 wDigMapGroup and wDigMapNumber
	js_info <<  "Map wDigMapGroup and wDigMapNumber..." << std::endl;
	it7.seek(sym7.getMapDataAddress("wDigMapGroup"));
	it8.seek(sym8.getMapDataAddress("wDigMapGroup"));
	uint8_t digMapGroupV7 = it7.getByte();
	it7.next();
	uint8_t digMapNumberV7 = it7.getByte();
	// create the tuple for the dig map group and number
	std::tuple<uint8_t, uint8_t> digMapV8 = mapv7toV8(digMapGroupV7, digMapNumberV7);
	// print found dig map group and number v7 and converted dig map group and number v8
	if (digMapGroupV7 != std::get<0>(digMapV8) || digMapNumberV7 != std::get<1>(digMapV8)){
		js_info <<  "Dig Map Group " << std::hex << static_cast<int>(digMapGroupV7) << " and Number " << std::hex << static_cast<int>(digMapNumberV7) << " converted to Group " << std::hex << static_cast<int>(std::get<0>(digMapV8)) << " and Number " << std::hex << static_cast<int>(std::get<1>(digMapV8)) << std::endl;
	}
	// write the dig map group and number to v8
	it8.setByte(std::get<0>(digMapV8));
	it8.next();
	it8.setByte(std::get<1>(digMapV8));

	// map the v7 wBackupMapGroup and wBackupMapNumber to v8 wBackupMapGroup and wBackupMapNumber
	js_info <<  "Map wBackupMapGroup and wBackupMapNumber..." << std::endl;
	it7.seek(sym7.getMapDataAddress("wBackupMapGroup"));
	it8.seek(sym8.getMapDataAddress("wBackupMapGroup"));
	uint8_t backupMapGroupV7 = it7.getByte();
	it7.next();
	uint8_t backupMapNumberV7 = it7.getByte();
	// create the tuple for the backup map group and number
	std::tuple<uint8_t, uint8_t> backupMapV8 = mapv7toV8(backupMapGroupV7, backupMapNumberV7);
	// print found backup map group and number v7 and converted backup map group and number v8
	if (backupMapGroupV7 != std::get<0>(backupMapV8) || backupMapNumberV7 != std::get<1>(backupMapV8)){
		js_info <<  "Backup Map Group " << std::hex << static_cast<int>(backupMapGroupV7) << " and Number " << std::hex << static_cast<int>(backupMapNumberV7) << " converted to Group " << std::hex << static_cast<int>(std::get<0>(backupMapV8)) << " and Number " << std::hex << static_cast<int>(std::get<1>(backupMapV8)) << std::endl;
	}
	// write the backup map group and number to v8
	it8.setByte(std::get<0>(backupMapV8));
	it8.next();
	it8.setByte(std::get<1>(backupMapV8));

	// map the v7 wLastSpawnMapGroup and wLastSpawnMapNumber to v8 wLastSpawnMapGroup and wLastSpawnMapNumber
	js_info <<  "Map wLastSpawnMapGroup and wLastSpawnMapNumber..." << std::endl;
	it7.seek(sym7.getMapDataAddress("wLastSpawnMapGroup"));
	it8.seek(sym8.getMapDataAddress("wLastSpawnMapGroup"));
	uint8_t lastSpawnMapGroupV7 = it7.getByte();
	it7.next();
	uint8_t lastSpawnMapNumberV7 = it7.getByte();
	// create the tuple for the last spawn map group and number
	std::tuple<uint8_t, uint8_t> lastSpawnMapV8 = mapv7toV8(lastSpawnMapGroupV7, lastSpawnMapNumberV7);
	// print found last spawn map group and number v7 and converted last spawn map group and number v8
	if (lastSpawnMapGroupV7 != std::get<0>(lastSpawnMapV8) || lastSpawnMapNumberV7 != std::get<1>(lastSpawnMapV8)){
		js_info <<  "Last Spawn Map Group " << std::hex << static_cast<int>(lastSpawnMapGroupV7) << " and Number " << std::hex << static_cast<int>(lastSpawnMapNumberV7) << " converted to Group " << std::hex << static_cast<int>(std::get<0>(lastSpawnMapV8)) << " and Number " << std::hex << static_cast<int>(std::get<1>(lastSpawnMapV8)) << std::endl;
	}
	// write the last spawn map group and number to v8
	it8.setByte(std::get<0>(lastSpawnMapV8));
	it8.next();
	it8.setByte(std::get<1>(lastSpawnMapV8));

	// map the v7 wMapGroup and wMapNumber to v8 wMapGroup and wMapNumber
	js_info <<  "Map wMapGroup and wMapNumber..." << std::endl;
	it7.seek(sym7.getMapDataAddress("wMapGroup"));
	it8.seek(sym8.getMapDataAddress("wMapGroup"));
	uint8_t mapGroupV7 = it7.getByte();
	it7.next();
	uint8_t mapNumberV7 = it7.getByte();
	// create the tuple for the map group and number
	std::tuple<uint8_t, uint8_t> mapV8 = mapv7toV8(mapGroupV7, mapNumberV7);
	// print found map group and number v7 and converted map group and number v8
	if (mapGroupV7 != std::get<0>(mapV8) || mapNumberV7 != std::get<1>(mapV8)){
		js_info <<  "Map Group " << std::hex << static_cast<int>(mapGroupV7) << " and Number " << std::hex << static_cast<int>(mapNumberV7) << " converted to Group " << std::hex << static_cast<int>(std::get<0>(mapV8)) << " and Number " << std::hex << static_cast<int>(std::get<1>(mapV8)) << std::endl;
	}
	// write the map group and number to v8
	it8.setByte(std::get<0>(mapV8));
	it8.next();
	it8.setByte(std::get<1>(mapV8));

	// Copy wPartyCount
	js_info <<  "Copy wPartyCount..." << std::endl;
	copyDataByte(sd, sym7.getPokemonDataAddress("wPartyCount"), sym8.getPokemonDataAddress("wPartyCount"));

	// copy wPartyMons PARTYMON_STRUCT_LENGTH * PARTY_LENGTH
	js_info <<  "Copy wPartyMons..." << std::endl;
	copyDataBlock(sd, sym7.getPokemonDataAddress("wPartyMons"), sym8.getPokemonDataAddress("wPartyMons"), PARTYMON_STRUCT_LENGTH * PARTY_LENGTH);

	// fix the party mon species
	js_info <<  "Fix party mon species..." << std::endl;
	it8.seek(sym8.getPokemonDataAddress("wPartyMons"));
	for (int i = 0; i < PARTY_LENGTH; i++) {
		uint16_t species = it8.getByte(sym8.getPokemonDataAddress("wPartyMons") + i * PARTYMON_STRUCT_LENGTH);
		if (species == 0x00) {
			continue;
		}
		convertSpeciesAndForm(sd, sym8.getPokemonDataAddress("wPartyMons"), i, PARTYMON_STRUCT_LENGTH, MON_EXTSPECIES, species, seen_mons, caught_mons);
	}

	// fix the party mon items
	js_info <<  "Fix party mon items..." << std::endl;
	it8.seek(sym8.getPokemonDataAddress("wPartyMons"));
	for (int i = 0; i < PARTY_LENGTH; i++) {
		it8.seek(sym8.getPokemonDataAddress("wPartyMons") + i * PARTYMON_STRUCT_LENGTH + MON_ITEM);
		uint8_t item = it8.getWord();
		if (item == 0x00) {
			continue;
		}
		uint8_t itemV8 = mapV7ItemToV8(item);
		// warn if the item was not found
		if (itemV8 == 0xFF) {
			js_error <<  "Item " << std::hex << static_cast<int>(item) << " not found in version 8 item list." << std::endl;
			continue;
		}
		// print found itemv7 and converted itemv8
		if (item != itemV8){
			js_info <<  "Item " << std::hex << static_cast<int>(item) << " converted to " << std::hex << static_cast<int>(itemV8) << std::endl;
		}
		it8.setByte(itemV8);
	}

	// fix party mon caught ball
	js_info <<  "Fix party mon caught ball..." << std::endl;
	it8.seek(sym8.getPokemonDataAddress("wPartyMons"));
	for (int i = 0; i < PARTY_LENGTH; i++) {
		uint8_t caughtBall = it8.getByte(sym8.getPokemonDataAddress("wPartyMons") + i * PARTYMON_STRUCT_LENGTH + MON_CAUGHTBALL) & CAUGHT_BALL_MASK;
		uint8_t caughtBallV8 = mapV7ItemToV8(caughtBall);
		// warn if the caught ball was not found
		if (caughtBallV8 == 0xFF) {
			js_error <<  "Caught Ball " << std::hex << caughtBall << " not found in version 8 item list." << std::endl;
			continue;
		}
		// print found caught ballv7 and converted caught ballv8
		if (caughtBall != caughtBallV8){
			js_info <<  "Caught Ball " << std::hex << static_cast<int>(caughtBall) << " converted to " << std::hex << caughtBallV8 << std::endl;
		}
		uint8_t currentCaughtBall = it8.getByte();
		currentCaughtBall &= ~CAUGHT_BALL_MASK;
		currentCaughtBall |= caughtBallV8 & CAUGHT_BALL_MASK;
		it8.setByte(currentCaughtBall);
	}

	// fix party mon caught locations
	js_info <<  "Fix party mon caught locations..." << std::endl;
	it8.seek(sym8.getPokemonDataAddress("wPartyMons"));
	for (int i = 0; i < PARTY_LENGTH; i++) {
		it8.seek(sym8.getPokemonDataAddress("wPartyMons") + i * PARTYMON_STRUCT_LENGTH + MON_CAUGHTLOCATION);
		uint8_t caughtLoc = it8.getByte(sym8.getPokemonDataAddress("wPartyMons") + i * PARTYMON_STRUCT_LENGTH + MON_CAUGHTLOCATION);
		uint8_t caughtLocV8 = mapV7LandmarkToV8(caughtLoc);
		// warn if the caught location was not found
		if (caughtLocV8 == 0xFF) {
			js_error <<  "Caught Location " << std::hex << caughtLoc << " not found in version 8 caught location list." << std::endl;
			continue;
		}
		// print found caught locationv7 and converted caught locationv8
		if (caughtLoc != caughtLocV8){
			js_info <<  "Caught Location " << std::hex << static_cast<int>(caughtLoc) << " converted to " << std::hex << caughtLocV8 << std::endl;
		}
		it8.setByte(caughtLocV8);
	}

	// copy wPartyMonOTs
	js_info <<  "Copy wPartyMonOTs..." << std::endl;
	copyDataBlock(sd, sym7.getPokemonDataAddress("wPartyMonOTs"), sym8.getPokemonDataAddress("wPartyMonOTs"), PARTY_LENGTH * (PLAYER_NAME_LENGTH + 3));

	// copy wPartyMonNicknames PARTY_LENGTH * MON_NAME_LENGTH
	js_info <<  "Copy wPartyMonNicknames..." << std::endl;
	copyDataBlock(sd, sym7.getPokemonDataAddress("wPartyMonNicknames"), sym8.getPokemonDataAddress("wPartyMonNicknames"), MON_NAME_LENGTH * PARTY_LENGTH);

	// We will convert the pokedex caught/seen flags last as we need the full list of seen/caught mons

	// copy wUnlockedUnowns
	js_info <<  "Copy wUnlockedUnowns..." << std::endl;
	copyDataByte(sd, sym7.getPokemonDataAddress("wUnlockedUnowns"), sym8.getPokemonDataAddress("wUnlockedUnowns"));

	// copy wDayCareMan to wBugContestSecondPartySpecies - 54
	js_info <<  "Copy wDayCareMan to wBugContestSecondPartySpecies - 54..." << std::endl;
	copyDataBlock(sd, sym7.getPokemonDataAddress("wDayCareMan"), sym8.getPokemonDataAddress("wDayCareMan"), sym7.getPokemonDataAddress("wBugContestSecondPartySpecies") - 54 - sym7.getPokemonDataAddress("wDayCareMan"));

	// fix wBreedMon1Species and wBreedMon1ExtSpecies
	js_info <<  "Fix wBreedMon1Species..." << std::endl;
	uint16_t species = it8.getByte(sym8.getPokemonDataAddress("wBreedMon1Species"));
	if (species != 0x00) {
		convertSpeciesAndForm(sd, sym8.getPokemonDataAddress("wBreedMon1Species"), 0, PARTYMON_STRUCT_LENGTH, MON_EXTSPECIES, species, seen_mons, caught_mons);
	}

	// fix wBreedMon1Item
	js_info <<  "Fix wBreedMon1Item..." << std::endl;
	uint8_t item = it8.getByte(sym8.getPokemonDataAddress("wBreedMon1Item"));
	if (item != 0x00) {
		uint8_t itemV8 = mapV7ItemToV8(item);
		// warn if the item was not found
		if (itemV8 == 0xFF) {
			js_error <<  "Item " << std::hex << static_cast<int>(item) << " not found in version 8 item list." << std::endl;
		}
		// print found itemv7 and converted itemv8
		if (item != itemV8){
			js_info <<  "Item " << std::hex << static_cast<int>(item) << " converted to " << std::hex << static_cast<int>(itemV8) << std::endl;
		}
		it8.setByte(itemV8);
	}

	// fix wBreedMon1CaughtBall
	js_info <<  "Fix wBreedMon1CaughtBall..." << std::endl;
	uint8_t caughtBall = it8.getByte(sym8.getPokemonDataAddress("wBreedMon1CaughtBall")) & CAUGHT_BALL_MASK;
	uint8_t caughtBallV8 = mapV7ItemToV8(caughtBall);
	// warn if the caught ball was not found
	if (caughtBallV8 == 0xFF) {
		js_error <<  "Caught Ball " << std::hex << caughtBall << " not found in version 8 item list." << std::endl;
	}
	// print found caught ballv7 and converted caught ballv8
	if (caughtBall != caughtBallV8){
		js_info <<  "Caught Ball " << std::hex << static_cast<int>(caughtBall) << " converted to " << std::hex << caughtBallV8 << std::endl;
	}
	uint8_t currentCaughtBall = it8.getByte();
	currentCaughtBall &= ~CAUGHT_BALL_MASK;
	currentCaughtBall |= caughtBallV8 & CAUGHT_BALL_MASK;
	it8.setByte(currentCaughtBall);

	// fix wBreedMon1CaughtLocation
	js_info <<  "Fix wBreedMon1CaughtLocation..." << std::endl;
	uint8_t caughtLoc = it8.getByte(sym8.getPokemonDataAddress("wBreedMon1CaughtLocation"));
	uint8_t caughtLocV8 = mapV7LandmarkToV8(caughtLoc);
	// warn if the caught location was not found
	if (caughtLocV8 == 0xFF) {
		js_error <<  "Caught Location " << std::hex << caughtLoc << " not found in version 8 caught location list." << std::endl;
	}
	// print found caught locationv7 and converted caught locationv8
	if (caughtLoc != caughtLocV8){
		js_info <<  "Caught Location " << std::hex << static_cast<int>(caughtLoc) << " converted to " << std::hex << caughtLocV8 << std::endl;
	}
	it8.setByte(caughtLocV8);

	// fix wBreedMon2Species and wBreedMon2ExtSpecies
	js_info <<  "Fix wBreedMon2Species..." << std::endl;
	species = it8.getByte(sym8.getPokemonDataAddress("wBreedMon2Species"));
	if (species != 0x00) {
		convertSpeciesAndForm(sd, sym8.getPokemonDataAddress("wBreedMon2Species"), 0, PARTYMON_STRUCT_LENGTH, MON_EXTSPECIES, species, seen_mons, caught_mons);
	}

	// fix wBreedMon2Item
	js_info <<  "Fix wBreedMon2Item..." << std::endl;
	item = it8.getByte(sym8.getPlayerDataAddress("wBreedMon2Item"));
	if (item != 0x00) {
		uint8_t itemV8 = mapV7ItemToV8(item);
		// warn if the item was not found
		if (itemV8 == 0xFF) {
			js_error <<  "Item " << std::hex << static_cast<int>(item) << " not found in version 8 item list." << std::endl;
		}
		// print found itemv7 and converted itemv8
		if (item != itemV8){
			js_info <<  "Item " << std::hex << static_cast<int>(item) << " converted to " << std::hex << static_cast<int>(itemV8) << std::endl;
		}
		it8.setByte(itemV8);
	}

	// fix wBreedMon2CaughtBall
	js_info <<  "Fix wBreedMon2CaughtBall..." << std::endl;
	caughtBall = it8.getByte(sym8.getPokemonDataAddress("wBreedMon2CaughtBall")) & CAUGHT_BALL_MASK;
	caughtBallV8 = mapV7ItemToV8(caughtBall);
	// warn if the caught ball was not found
	if (caughtBallV8 == 0xFF) {
		js_error <<  "Caught Ball " << std::hex << caughtBall << " not found in version 8 item list." << std::endl;
	}
	// print found caught ballv7 and converted caught ballv8
	if (caughtBall != caughtBallV8){
		js_info <<  "Caught Ball " << std::hex << static_cast<int>(caughtBall) << " converted to " << std::hex << caughtBallV8 << std::endl;
	}
	currentCaughtBall = it8.getByte();
	currentCaughtBall &= ~CAUGHT_BALL_MASK;
	currentCaughtBall |= caughtBallV8 & CAUGHT_BALL_MASK;
	it8.setByte(currentCaughtBall);

	// fix wBreedMon2CaughtLocation
	js_info <<  "Fix wBreedMon2CaughtLocation..." << std::endl;
	caughtLoc = it8.getByte(sym8.getPokemonDataAddress("wBreedMon2CaughtLocation"));
	caughtLocV8 = mapV7LandmarkToV8(caughtLoc);
	// warn if the caught location was not found
	if (caughtLocV8 == 0xFF) {
		js_error <<  "Caught Location " << std::hex << caughtLoc << " not found in version 8 caught location list." << std::endl;
	}
	// print found caught locationv7 and converted caught locationv8
	if (caughtLoc != caughtLocV8){
		js_info <<  "Caught Location " << std::hex << static_cast<int>(caughtLoc) << " converted to " << std::hex << caughtLocV8 << std::endl;
	} else {
		js_info <<  "Caught Location " << std::hex << static_cast<int>(caughtLoc) << " not converted." << std::endl;
	}
	it8.setByte(caughtLocV8);

	// Clear space from wLevelUpMonNickname to wBugContestBackupPartyCount in it8
	js_info <<  "Clear space from wLevelUpMonNickname to wBugContestBackupPartyCount..." << std::endl;
	it8.seek(sym8.getPokemonDataAddress("wLevelUpMonNickname"));
	while (it8.getAddress() < sym8.getPokemonDataAddress("wBugContestBackupPartyCount")) {
		it8.setByte(0x00);
		it8.next();
	}

	// copy wBugContestBackupPartyCount
	js_info <<  "Copy wBugContestBackupPartyCount..." << std::endl;
	copyDataByte(sd, sym7.getPokemonDataAddress("wBugContestSecondPartySpecies"), sym8.getPokemonDataAddress("wBugContestBackupPartyCount"));

	// copy from wContestMon to wPokemonDataEnd
	js_info <<  "Copy from wContestMon to w****monDataEnd..." << std::endl;
	copyDataBlock(sd, sym7.getPokemonDataAddress("wContestMon"), sym8.getPokemonDataAddress("wContestMon"), sym7.getPokemonDataAddress("wPokemonDataEnd") - sym7.getPokemonDataAddress("wContestMon"));

	// fix wContestMonSpecies and wContestMonExtSpecies
	js_info <<  "Fix wContestMonSpecies..." << std::endl;
	species = it8.getByte(sym8.getPokemonDataAddress("wContestMonSpecies"));
	if (species != 0x00) {
		convertSpeciesAndForm(sd, sym8.getPokemonDataAddress("wContestMonSpecies"), 0, PARTYMON_STRUCT_LENGTH, MON_EXTSPECIES, species, seen_mons, caught_mons);
	}

	// fix wContestMonItem
	js_info <<  "Fix wContestMonItem..." << std::endl;
	item = it8.getByte(sym8.getPokemonDataAddress("wContestMonItem"));
	if (item != 0x00) {
		uint8_t itemV8 = mapV7ItemToV8(item);
		// warn if the item was not found
		if (itemV8 == 0xFF) {
			js_error <<  "Item " << std::hex << static_cast<int>(item) << " not found in version 8 item list." << std::endl;
		}
		// print found itemv7 and converted itemv8
		if (item != itemV8){
			js_info <<  "Item " << std::hex << static_cast<int>(item) << " converted to " << std::hex << static_cast<int>(itemV8) << std::endl;
		}
		it8.setByte(itemV8);
	}

	// fix wContestMonCaughtBall
	js_info <<  "Fix wContestMonCaughtBall..." << std::endl;
	caughtBall = it8.getByte(sym8.getPokemonDataAddress("wContestMonCaughtBall")) & CAUGHT_BALL_MASK;
	caughtBallV8 = mapV7ItemToV8(caughtBall);
	// warn if the caught ball was not found
	if (caughtBallV8 == 0xFF) {
		js_error <<  "Caught Ball " << std::hex << caughtBall << " not found in version 8 item list." << std::endl;
	}
	// print found caught ballv7 and converted caught ballv8
	if (caughtBall != caughtBallV8){
		js_info <<  "Caught Ball " << std::hex << static_cast<int>(caughtBall) << " converted to " << std::hex << caughtBallV8 << std::endl;
	}
	currentCaughtBall = it8.getByte();
	currentCaughtBall &= ~CAUGHT_BALL_MASK;
	currentCaughtBall |= caughtBallV8 & CAUGHT_BALL_MASK;
	it8.setByte(currentCaughtBall);

	// fix wContestMonCaughtLocation
	js_info <<  "Fix wContestMonCaughtLocation..." << std::endl;
	caughtLoc = it8.getByte(sym8.getPokemonDataAddress("wContestMonCaughtLocation"));
	caughtLocV8 = mapV7LandmarkToV8(caughtLoc);
	// warn if the caught location was not found
	if (caughtLocV8 == 0xFF) {
		js_error <<  "Caught Location " << std::hex << caughtLoc << " not found in version 8 caught location list." << std::endl;
	}
	// print found caught locationv7 and converted caught locationv8
	if (caughtLoc != caughtLocV8){
		js_info <<  "Caught Location " << std::hex << static_cast<int>(caughtLoc) << " converted to " << std::hex << caughtLocV8 << std::endl;
	}
	it8.setByte(caughtLocV8);

	// map the version 7 wDunsparceMapGroup and wDunsparceMapNumber to the version 8 wDunsparceMapGroup and wDunsparceMapNumber
	js_info <<  "Map wDunsp****MapGroup and wDunsp****MapNumber..." << std::endl;
	it7.seek(sym7.getPokemonDataAddress("wDunsparceMapGroup"));
	it8.seek(sym8.getPokemonDataAddress("wDunsparceMapGroup"));
	uint8_t dunsparceMapGroup = it7.getByte();
	it7.next();
	uint8_t dunsparceMapNumber = it7.getByte();
	// create tuple to store the map group and map number
	std::tuple<uint8_t, uint8_t> dunsparceMap = mapv7toV8(dunsparceMapGroup, dunsparceMapNumber);
	// print found dunsparce mapv7 and converted dunsparce mapv8
	if (dunsparceMapGroup != std::get<0>(dunsparceMap) || dunsparceMapNumber != std::get<1>(dunsparceMap)){
		js_info <<  "Dunsp**** Map " << std::hex << static_cast<int>(dunsparceMapGroup) << " " << std::hex << static_cast<int>(dunsparceMapNumber) << " converted to " << std::hex << static_cast<int>(std::get<0>(dunsparceMap)) << " " << std::hex << static_cast<int>(std::get<1>(dunsparceMap)) << std::endl;
	}
	// write the map group and map number
	it8.setByte(std::get<0>(dunsparceMap));
	it8.next();
	it8.setByte(std::get<1>(dunsparceMap));

	// map the version 7 wRoamMons_CurMapNumber and wRoamMons_CurMapGroup to the version 8 wRoamMons_CurMapNumber and wRoamMons_CurMapGroup
	js_info <<  "Map wRoamMons_CurMapNumber and wRoamMons_CurMapGroup..." << std::endl;
	it7.seek(sym7.getPokemonDataAddress("wRoamMons_CurMapNumber"));
	it8.seek(sym8.getPokemonDataAddress("wRoamMons_CurMapNumber"));
	uint8_t roamMons_CurMapNumber = it7.getByte();
	it7.next();
	uint8_t roamMons_CurMapGroup = it7.getByte();
	// create tuple to store the map group and map number
	std::tuple<uint8_t, uint8_t> roamMons_CurMap = mapv7toV8(roamMons_CurMapGroup, roamMons_CurMapNumber);
	// print found roamMons_CurMapv7 and converted roamMons_CurMapv8
	if (roamMons_CurMapGroup != std::get<0>(roamMons_CurMap) || roamMons_CurMapNumber != std::get<1>(roamMons_CurMap)){
		js_info <<  "RoamMons_CurMap " << std::hex << static_cast<int>(roamMons_CurMapGroup) << " " << std::hex << static_cast<int>(roamMons_CurMapNumber) << " converted to " << std::hex << static_cast<int>(std::get<0>(roamMons_CurMap)) << " " << std::hex << static_cast<int>(std::get<1>(roamMons_CurMap)) << std::endl;
	}
	// write the map group and map number
	it8.setByte(std::get<0>(roamMons_CurMap));
	it8.next();
	it8.setByte(std::get<1>(roamMons_CurMap));

	// map the version 7 wRoamMons_LastMapNumber and wRoamMons_LastMapGroup to the version 8 wRoamMons_LastMapNumber and wRoamMons_LastMapGroup
	js_info <<  "Map wRoamMons_LastMapNumber and wRoamMons_LastMapGroup..." << std::endl;
	it7.seek(sym7.getPokemonDataAddress("wRoamMons_LastMapNumber"));
	it8.seek(sym8.getPokemonDataAddress("wRoamMons_LastMapNumber"));
	uint8_t roamMons_LastMapNumber = it7.getByte();
	it7.next();
	uint8_t roamMons_LastMapGroup = it7.getByte();
	// create tuple to store the map group and map number
	std::tuple<uint8_t, uint8_t> roamMons_LastMap = mapv7toV8(roamMons_LastMapGroup, roamMons_LastMapNumber);
	// print found roamMons_LastMapv7 and converted roamMons_LastMapv8
	if (roamMons_LastMapGroup != std::get<0>(roamMons_LastMap) || roamMons_LastMapNumber != std::get<1>(roamMons_LastMap)){
		js_info <<  "RoamMons_LastMap " << std::hex << static_cast<int>(roamMons_LastMapGroup) << " " << std::hex << static_cast<int>(roamMons_LastMapNumber) << " converted to " << std::hex << static_cast<int>(std::get<0>(roamMons_LastMap)) << " " << std::hex << static_cast<int>(std::get<1>(roamMons_LastMap)) << std::endl;
	}
	// write the map group and map number
	it8.setByte(std::get<0>(roamMons_LastMap));
	it8.next();
	it8.setByte(std::get<1>(roamMons_LastMap));

	// copy sCheckValue2
	js_info <<  "Copy sCheckValue2..." << std::endl;
	copyDataByte(sd, sym7.getSRAMAddress("sCheckValue2"), sym8.getSRAMAddress("sCheckValue2"));

	// copy it8 Save to it8 Backup Save
	js_info <<  "Copy Save to Backup Save..." << std::endl;
	for (int i = 0; i < sym8.getSRAMAddress("sCheckValue2") + 1 - sym8.getSRAMAddress("sOptions"); i++) {
		save8.setByte(sym8.getSRAMAddress("sBackupOptions") + i, save8.getByte(sym8.getSRAMAddress("sOptions") + i));
	}

	// copy from sHallOfFame to sHallOfFameEnd
	js_info <<  "Copy from sHallOfFame to sHallOfFameEnd..." << std::endl;
	copyDataBlock(sd, sym7.getSRAMAddress("sHallOfFame"), sym8.getSRAMAddress("sHallOfFame"), sym8.getSRAMAddress("sHallOfFameEnd") - sym8.getSRAMAddress("sHallOfFame"));

	// fix the hall of fame mon species
	js_info <<  "Fix hall of fame mon species..." << std::endl;
	for (int i = 0; i < NUM_HOF_TEAMS_V8; i++) {
		for (int j = 0; j < PARTY_LENGTH; j++){
			it8.seek(sym8.getSRAMAddress("sHallOfFame01Mon1") + i * HOF_LENGTH);
			uint16_t species = it8.getByte(it8.getAddress() + j * HOF_MON_LENGTH);
			if (species == 0x00) {
				continue;
			}
			convertSpeciesAndForm(sd, sym8.getSRAMAddress("sHallOfFame01Mon1") + i * HOF_LENGTH, j, HOF_MON_LENGTH, HOF_MON_EXTSPECIES, species, seen_mons, caught_mons);
		}
	}

	// wPokedexCaught is a flag_array of NUM_POKEMON_V7 bits. If v7 bit is set, lookup the bit index in the map and set the corresponding bit in v8
	js_info <<  "Patching w****dexCaught..." << std::endl;
	it7.seek(sym7.getPokemonDataAddress("wPokedexCaught"));
	it8.seek(sym8.getPokemonDataAddress("wPokedexCaught"));
	for (int i = 0; i < NUM_POKEMON_V7; i++) {
		// check if the bit is set
		if (it7.getByte() & (1 << (i % 8))) {
			// get the pokemon index is equal to the bit index
			uint16_t pokemonIndex = i + 1;
			// map the version 7 pokemon to the version 8 pokemon
			uint16_t pokemonIndexV8 = mapV7PkmnToV8(pokemonIndex) - 1;
			// if the pokemon is found set the corresponding bit in it8
			if (pokemonIndexV8 != INVALID_SPECIES) {
				// print found pokemonv7 and converted pokemonv8
				if (pokemonIndex != pokemonIndexV8 + 1){
					js_info <<  "Mon " << std::hex << static_cast<int>(pokemonIndex) << " converted to " << std::hex << static_cast<int>(pokemonIndexV8) << std::endl;
				}
				// seek to the byte containing the bit
				it8.seek(sym8.getPokemonDataAddress("wPokedexCaught") + pokemonIndexV8 / 8);
				// set the bit
				it8.setByte(it8.getByte() | (1 << (pokemonIndexV8 % 8)));
			}
		}
		if (i % 8 == 7) {
			it7.next();
		}
	}
	// for each caught mon in vector caught_mons, set the corresponding bit in it8
	for (uint16_t mon : caught_mons){
		uint16_t monIndexV8 = mon - 1;
		it8.seek(sym8.getPokemonDataAddress("wPokedexCaught") + monIndexV8 / 8);
		it8.setByte(it8.getByte() | (1 << (monIndexV8 % 8)));
		js_info <<  "Found caught mon " << std::hex << static_cast<int>(mon) << std::endl;
	}

	// wPokedexSeen is a flag_array of NUM_POKEMON_V7 bits. If v7 bit is set, lookup the bit index in the map and set the corresponding bit in v8
	js_info <<  "Patching w****dexSeen..." << std::endl;
	it7.seek(sym7.getPokemonDataAddress("wPokedexSeen"));
	it8.seek(sym8.getPokemonDataAddress("wPokedexSeen"));
	for (int i = 0; i < NUM_POKEMON_V7; i++) {
		// check if the bit is set
		if (it7.getByte() & (1 << (i % 8))) {
			// get the pokemon index is equal to the bit index
			uint16_t pokemonIndex = i + 1;
			// map the version 7 pokemon to the version 8 pokemon
			uint16_t pokemonIndexV8 = mapV7PkmnToV8(pokemonIndex) - 1;
			// if the pokemon is found set the corresponding bit in it8
			if (pokemonIndexV8 != INVALID_SPECIES) {
				// print found pokemonv7 and converted pokemonv8
				if (pokemonIndex != pokemonIndexV8 + 1){
					js_info <<  "Mon " << std::hex << static_cast<int>(pokemonIndex) << " converted to " << std::hex << static_cast<int>(pokemonIndexV8) << std::endl;
				}
				// seek to the byte containing the bit
				it8.seek(sym8.getPokemonDataAddress("wPokedexSeen") + pokemonIndexV8 / 8);
				// set the bit
				it8.setByte(it8.getByte() | (1 << (pokemonIndexV8 % 8)));
			}
		}
		if (i % 8 == 7) {
			it7.next();
		}
	}
	// for each seen mon in vector seen_mons, set the corresponding bit in it8
	for (uint16_t mon : seen_mons){
		uint16_t monIndexV8 = mon - 1;
		it8.seek(sym8.getPokemonDataAddress("wPokedexSeen") + monIndexV8 / 8);
		it8.setByte(it8.getByte() | (1 << (monIndexV8 % 8)));
		js_info <<  "Found seen mon " << std::hex << static_cast<int>(mon) << std::endl;
	}

	// write the new save version number big endian
	js_info <<  "Write new save version number..." << std::endl;
	uint16_t new_save_version = 0x08;
	save8.setWordBE(SAVE_VERSION_ABS_ADDRESS, new_save_version);

	// write new checksums to the version 8 save file
	js_info <<  "Write new checksums..." << std::endl;
	uint16_t new_checksum = calculateChecksum(save8, sym8.getSRAMAddress("sGameData"), sym8.getSRAMAddress("sGameDataEnd"));
	save8.setWord(SAVE_CHECKSUM_ABS_ADDRESS, new_checksum);

	// write new backup checksums to the version 8 save file
	js_info <<  "Write new backup checksums..." << std::endl;
	uint16_t new_backup_checksum = calculateChecksum(save8, sym8.getSRAMAddress("sBackupGameData"), sym8.getSRAMAddress("sBackupGameDataEnd"));
	save8.setWord(SAVE_BACKUP_CHECKSUM_ABS_ADDRESS, new_backup_checksum);

	// write the modified save file to the output file and print success message
	js_info <<  "Save file patched successfully!" << std::endl;
	return true;
}

uint16_t calculateNewboxChecksum(const SaveBinary& save, uint32_t startAddress) {
	uint16_t checksum = 127;

	// Process bytes 0x00 to 0x1F
	for (int i = 0; i <= 0x1F; ++i){
		checksum += save.getByte(startAddress + i) * (i + 1);
	}

	// Process bytes 0x20 to 0x30
	for (int i = 0x20; i <= 0x30; ++i){
		checksum += (save.getByte(startAddress + i) & 0x7F) * (i + 2);
	}

	// Clamp to 2 bytes
	checksum &= 0xFFFF;

	return checksum;
}

uint16_t extractStoredNewboxChecksum(const SaveBinary& save, uint32_t startAddress) {
	uint16_t storedChecksum = 0;

	// Read the most significant bits from 0x20 to 0x30
	for (int i = 0; i <= 0xF; ++i){
		uint8_t msb = (save.getByte(startAddress + 0x20 + i) & 0x80) >> 7;
		storedChecksum |= (msb << (0xF - i));
	}
	return storedChecksum;
}

void writeNewboxChecksum(SaveBinary& save, uint32_t startAddress) {
	uint16_t checksum = calculateNewboxChecksum(save, startAddress);

	// write the most significant bits from 0x20 to 0x30
	for (int i = 0; i <= 0xF; ++i) {
		uint8_t byte = save.getByte(startAddress + 0x20 + i);
		byte &= 0x7F; // clear the most significant bit
		byte |= ((checksum >> (0xF - i)) & 0x1) << 7; // set the most significant bit
		save.setByte(startAddress + 0x20 + i, byte);
	}
}

void writeDefaultBoxName(SaveBinary::Iterator& it, int boxNum) {
	// Writes the default box name for the given box number
	// '  BOX XX' where XX is the box number
	uint8_t box_name_char[] = {0x7f, 0x7f, 0x81, 0xae, 0xb7, 0x7f, static_cast<uint8_t>(0xe0 + (boxNum / 10)), static_cast<uint8_t>(0xe0 + (boxNum % 10))};
	for (uint8_t box_char : box_name_char) {
		it.setByte(box_char);
		it.next();
	}
}

void migrateBoxData(SourceDest &sd, const std::string &prefix) {
	// Clear the boxes
	js_info << "Clearing v8 " << prefix << " boxes..." << std::endl;
	for (int n = 1; n < NUM_BOXES_V8 + 1; n++) {
		sd.destSave.seek(sd.destSym.getSRAMAddress(prefix + std::to_string(n)));
		for (int i = 0; i < NEWBOX_SIZE; i++) {
			sd.destSave.setByte(0x00);
			sd.destSave.next();
		}
	}

	// Copy the boxes
	for (int n = 1; n < NUM_BOXES_V7 + 1; n++) {
		js_info << "Copying v7 " << prefix << n << " to v8..." << std::endl;
		copyDataBlock(sd, sd.sourceSym.getSRAMAddress(prefix + std::to_string(n)), sd.destSym.getSRAMAddress(prefix + std::to_string(n)), NEWBOX_SIZE);
	}

	// Write default box names from NUM_BOXES_V7 + 1 to NUM_BOXES_V8
	js_info <<  "Writing " << prefix << " default box names..." << std::endl;
	for (int n = NUM_BOXES_V7 + 1; n < NUM_BOXES_V8 + 1; n++) {
		sd.destSave.seek(sd.destSym.getSRAMAddress(prefix + std::to_string(n) + "Name"));
		js_info <<  "Writing default box name for " << prefix << n << "..." << std::endl;
		writeDefaultBoxName(sd.destSave, n);
	}

	// convert pc box themes
	js_info <<  "Converting " << prefix << " box themes..." << std::endl;
	for (int n = 1; n < NUM_BOXES_V8 + 1; n++) {
		uint8_t theme = sd.destSave.getByte(sd.destSym.getSRAMAddress(prefix + std::to_string(n) + "Theme"));
		uint8_t theme_v8 = mapV7ThemeToV8(theme);
		if (theme != theme_v8) {
			js_info <<  "Theme " << std::hex << static_cast<int>(theme) << " converted to " << std::hex << static_cast<int>(theme_v8) << std::endl;
			sd.destSave.setByte(theme_v8);
		}
	}
}

void clearBox(SaveBinary::Iterator &it8, const SymbolDatabase &sym8, const std::string &boxName, int numEntries) {
	js_info << "Clearing " << boxName << "..." << std::endl;
	it8.seek(sym8.getSRAMAddress(boxName));
	for (int i = 0; i < numEntries; i++) {
		for (int j = 0; j < SAVEMON_STRUCT_LENGTH; j++) {
			it8.setByte(0x00);
			it8.next();
		}
	}
}

void convertSpeciesAndForm(SourceDest &sd, uint32_t base_address, int i, int struct_length, int extspecies_offset, uint16_t species, std::vector<uint16_t> &seen_mons, std::vector<uint16_t> &caught_mons) {
	// convert species & form
	uint16_t species_v8 = mapV7PkmnToV8(species);
	if (species_v8 == INVALID_SPECIES) {
		js_error <<  "Species " << std::hex << species << " not found in version 8 mon list." << std::endl;
		return;
	} else {
		if (species != species_v8) {
			js_info <<  "Species " << std::hex << species << " converted to " << std::hex << species_v8 << std::endl;
		}
		sd.destSave.setByte(base_address + i * struct_length, species_v8 & 0xFF);
		uint8_t personality = sd.destSave.getByte(base_address + i * struct_length + extspecies_offset);
		personality &= ~EXTSPECIES_MASK;
		personality |= (species_v8 >> 8) << MON_EXTSPECIES_F;
		uint8_t form = personality & FORM_MASK;
		uint16_t extspecies_v8 = mapV7SpeciesFormToV8Extspecies(species, form);
		if (extspecies_v8 != INVALID_SPECIES) {
			seen_mons.push_back(extspecies_v8);
			caught_mons.push_back(extspecies_v8);
		}
		if (species_v8 == MAGIKARP_V8) {
			form = mapV7MagikarpFormToV8(form);
			personality &= ~FORM_MASK;
			personality |= form;
		}
		if (species_v8 == GYARADOS_V8) {
			if (form == GYARADOS_RED_FORM_V7){
				form = GYARADOS_RED_FORM_V8;
				personality &= ~FORM_MASK;
				personality |= form;
			}
		}
		sd.destSave.setByte(personality);
	}
}