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
	uint16_t calculated_checksum = calculateSaveChecksum(save7, sym7.getSRAMAddress("sGameData"), sym7.getSRAMAddress("sGameDataEnd"));
	if (save_checksum != calculated_checksum) {
		js_error <<  "Checksum mismatch! Expected: " << std::hex << calculated_checksum << ", got: " << save_checksum << std::endl;
		return false;
	}

	// check the backup checksum word from the version 7 save file
	uint16_t backup_checksum = save7.getWord(SAVE_BACKUP_CHECKSUM_ABS_ADDRESS);
	// verify the backup checksum of the version 7 file matches the calculated checksum
	// calculate the checksum from lookup symbol name "sBackupGameData" to "sBackupGameDataEnd"
	uint16_t calculated_backup_checksum = calculateSaveChecksum(save7, sym7.getSRAMAddress("sBackupGameData"), sym7.getSRAMAddress("sBackupGameDataEnd"));
	if (backup_checksum != calculated_backup_checksum) {
		js_error <<  "Backup checksum mismatch! Expected: " << std::hex << calculated_backup_checksum << ", got: " << backup_checksum << std::endl;
		return false;
	}

	// check if the player in the PKMN Center 2nd Floor
	uint8_t map_group = it7.getByte(sym7.getMapDataAddress("wMapGroup"));
	it7.next();
	uint8_t map_num = it7.getByte();
	if (map_group != MON_CENTER_2F_GROUP || map_num != MON_CENTER_2F_MAP) {
		js_error <<  "Player is not in the PKMN Center 2nd Floor. Go to where you heal in game, and head upstairs. Then re-save your game and try again." << std::endl;
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

	js_info << "Clearing " << "sBoxMons1B" << "..." << std::endl;
	clearDataBlock(sd, sym8.getSRAMAddress("sBoxMons1B"), MONDB_ENTRIES_B_V8 * SAVEMON_STRUCT_LENGTH);
	js_info << "Clearing " << "sBoxMons1C" << "..." << std::endl;
	clearDataBlock(sd, sym8.getSRAMAddress("sBoxMons1C"), MONDB_ENTRIES_C_V8 * SAVEMON_STRUCT_LENGTH);

	// copy sBoxMons2 to SBoxMons2A
	js_info <<  "Copying from sBoxMons2 to sBoxMons2A..." << std::endl;
	copyDataBlock(sd, sym7.getSRAMAddress("sBoxMons2"), sym8.getSRAMAddress("sBoxMons2A"), MONDB_ENTRIES_A_V8 * SAVEMON_STRUCT_LENGTH);

	js_info << "Clearing " << "sBoxMons2B" << "..." << std::endl;
	clearDataBlock(sd, sym8.getSRAMAddress("sBoxMons2B"), MONDB_ENTRIES_B_V8 * SAVEMON_STRUCT_LENGTH);
	js_info << "Clearing " << "sBoxMons2C" << "..." << std::endl;
	clearDataBlock(sd, sym8.getSRAMAddress("sBoxMons2C"), MONDB_ENTRIES_C_V8 * SAVEMON_STRUCT_LENGTH);

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
			convertSpeciesAndForm(sd, sym8.getSRAMAddress("sBoxMons1A"), i, SAVEMON_STRUCT_LENGTH, SAVEMON_EXTSPECIES, SAVEMON_MOVES, species, seen_mons, caught_mons);
			// convert item
			convertItem(sd, sym8.getSRAMAddress("sBoxMons1A"), i, SAVEMON_STRUCT_LENGTH, SAVEMON_ITEM, item);
			// convert caught location
			convertCaughtLocation(sd, sym8.getSRAMAddress("sBoxMons1A"), i, SAVEMON_STRUCT_LENGTH, SAVEMON_CAUGHTLOCATION, caught_location);
			// convert caught ball
			convertCaughtBall(sd, sym8.getSRAMAddress("sBoxMons1A"), i, SAVEMON_STRUCT_LENGTH, SAVEMON_CAUGHTBALL, caught_ball);
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
			convertSpeciesAndForm(sd, sym8.getSRAMAddress("sBoxMons2A"), i, SAVEMON_STRUCT_LENGTH, SAVEMON_EXTSPECIES, SAVEMON_MOVES, species, seen_mons, caught_mons);
			// convert item
			convertItem(sd, sym8.getSRAMAddress("sBoxMons2A"), i, SAVEMON_STRUCT_LENGTH, SAVEMON_ITEM, item);
			// convert caught location
			convertCaughtLocation(sd, sym8.getSRAMAddress("sBoxMons2A"), i, SAVEMON_STRUCT_LENGTH, SAVEMON_CAUGHTLOCATION, caught_location);
			// convert caught ball
			convertCaughtBall(sd, sym8.getSRAMAddress("sBoxMons2A"), i, SAVEMON_STRUCT_LENGTH, SAVEMON_CAUGHTBALL, caught_ball);
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

	// Reset NUZLOCKE bit to off; this becomes the affection option.
	js_info <<  "Resetting NUZLOCKE bit..." << std::endl;
	it8.resetBit(sym8.getOptionsAddress("wInitialOptions"), AFFECTION_OPT); // previously NUZLOCKE_OPT
	// Make sure wInitialOptions2 is clear in v8
	js_info <<  "Clearing wInitialOptions2..." << std::endl;
	it8.next();
	it8.setByte(0);
	// Set EVS_OPT_CLASSIC in wInitialOptions2 that is the lower two bits of wInitialOptions2 is 0b01
	js_info <<  "Setting EVS_OPT_CLASSIC..." << std::endl;
	it8.setBit(EVS_OPT_CLASSIC);
	// assert only bit 0 is set in the integer EVS_OPT_CLASSIC
	if (EVS_OPT_CLASSIC != 0b1) {
		js_error <<  "EVS_OPT_CLASSIC is not 0b01. Adjust logic in PatchVersion7to8." << std::endl;
	}
	// Reset Initial Options so the game asks the player to set them again.
	js_info <<  "Resetting Initial Options..." << std::endl;
	it8.setBit(RESET_INIT_OPTS);

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

	// copy it7 wEnteredMapFromContinue to it8 wEnteredMapFromContinue
	js_info <<  "Copy wEnteredMapFromContinue" << std::endl;
	copyDataByte(sd, sym7.getPlayerDataAddress("wEnteredMapFromContinue"), sym8.getPlayerDataAddress("wEnteredMapFromContinue"));

	js_info <<  "Copy wStatusFlags3" << std::endl;
	// copy it7 wStatusFlags3 to it8 wStatusFlags3
	copyDataByte(sd, sym7.getPlayerDataAddress("wStatusFlags3"), sym8.getPlayerDataAddress("wStatusFlags3"));

	js_info <<  "Copying from wTimeOfDayPal to wBadgesEnd..." << std::endl;
	// copy from it7 wTimeOfDayPal to it7 wTMsHMsEnd
	copyDataBlock(sd, sym7.getPlayerDataAddress("wTimeOfDayPal"), sym8.getPlayerDataAddress("wTimeOfDayPal"), sym7.getPlayerDataAddress("wBadgesEnd") - sym7.getPlayerDataAddress("wTimeOfDayPal"));

	// clear it8 wPokemonJournals
	js_info <<  "Clearing wPokemonJournals..." << std::endl;
	clearDataBlock(sd, sym8.getPlayerDataAddress("wPokemonJournals"), sym8.getPlayerDataAddress("wPokemonJournalsEnd") - sym8.getPlayerDataAddress("wPokemonJournals"));

	// copy it7 wPokemonJournals to it8 wPokemonJournals
	js_info <<  "Copying wPokemonJournals..." << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wPokemonJournals"), sym8.getPlayerDataAddress("wPokemonJournals"), sym7.getPlayerDataAddress("wPokemonJournalsEnd") - sym7.getPlayerDataAddress("wPokemonJournals"));

	// copy TMsHMs
	js_info <<  "Copying from wTMsHMs to wTMsHMsEnd..." << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wTMsHMs"), sym8.getPlayerDataAddress("wTMsHMs"), sym7.getPlayerDataAddress("wTMsHMsEnd") - sym7.getPlayerDataAddress("wTMsHMs"));

	// clear it8 wKeyItems
	js_info <<  "Clearing wKeyItems..." << std::endl;
	clearDataBlock(sd, sym8.getPlayerDataAddress("wKeyItems"), sym8.getPlayerDataAddress("wKeyItemsEnd") - sym8.getPlayerDataAddress("wKeyItems"));

	js_info <<  "Patching wKeyItems..." << std::endl;
	it7.seek(sym7.getPlayerDataAddress("wKeyItems"));
	it8.seek(sym8.getPlayerDataAddress("wKeyItems"));
	// it7 wKeyItems is a bit flag array of NUM_KEY_ITEMS_V7 bits. If v7 bit is set, lookup the bit index in the map and write the index to the next byte in it8.
	for (int i = 0; i < NUM_KEY_ITEMS_V7; i++) {
		// check if the bit is set
		if (isFlagBitSet(it7, sym7.getPlayerDataAddress("wKeyItems"), i)) {
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
	}
	// write 0x00 to the end of wKeyItems
	it8.setByte(0x00);

	convertItemList(sd, sym7.getPlayerDataAddress("wNumItems"), sym7.getPlayerDataAddress("wItems"), sym8.getPlayerDataAddress("wNumItems"), sym8.getPlayerDataAddress("wItems"), "wItems");
	convertItemList(sd, sym7.getPlayerDataAddress("wNumMedicine"), sym7.getPlayerDataAddress("wMedicine"), sym8.getPlayerDataAddress("wNumMedicine"), sym8.getPlayerDataAddress("wMedicine"), "wMedicine");
	convertItemList(sd, sym7.getPlayerDataAddress("wNumBalls"), sym7.getPlayerDataAddress("wBalls"), sym8.getPlayerDataAddress("wNumBalls"), sym8.getPlayerDataAddress("wBalls"), "wBalls");
	convertItemList(sd, sym7.getPlayerDataAddress("wNumBerries"), sym7.getPlayerDataAddress("wBerries"), sym8.getPlayerDataAddress("wNumBerries"), sym8.getPlayerDataAddress("wBerries"), "wBerries");
	convertItemList(sd, sym7.getPlayerDataAddress("wNumPCItems"), sym7.getPlayerDataAddress("wPCItems"), sym8.getPlayerDataAddress("wNumPCItems"), sym8.getPlayerDataAddress("wPCItems"), "wPCItems");

	js_info <<  "Copy wApricorns..." << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wApricorns"), sym8.getPlayerDataAddress("wApricorns"), NUM_APRICORNS);

	js_info <<  "Copy from w****gearFlags to wAlways0SceneID..." << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wPokegearFlags"), sym8.getPlayerDataAddress("wPokegearFlags"), sym7.getPlayerDataAddress("wAlways0SceneID") - sym7.getPlayerDataAddress("wPokegearFlags"));

	js_info <<  "copy from wAlways0SceneID to wEcru****HouseSceneID + 1..." << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wAlways0SceneID"), sym8.getPlayerDataAddress("wAlways0SceneID"), sym7.getPlayerDataAddress("wEcruteakHouseSceneID") + 1 - sym7.getPlayerDataAddress("wAlways0SceneID"));

	// clear wEcruteakPokecenter1FSceneID as it is no longer used
	js_info <<  "Clear wEcru********center1FSceneID..." << std::endl;
	it8.setByte(sym8.getPlayerDataAddress("wEcruteakHouseSceneID") + 1, 0x00);

	// copy from wElmsLabSceneID to wEventFlags
	js_info <<  "Copy from wE***LabSceneID to wEventFlags..." << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wElmsLabSceneID"), sym8.getPlayerDataAddress("wElmsLabSceneID"), sym7.getPlayerDataAddress("wEventFlags") - sym7.getPlayerDataAddress("wElmsLabSceneID"));

	// clear it8 wEventFlags
	js_info <<  "Clear wEventFlags..." << std::endl;
	clearDataBlock(sd, sym8.getPlayerDataAddress("wEventFlags"), NUM_EVENTS / 8);

	it8.seek(sym8.getPlayerDataAddress("wEventFlags"));
	// wEventFlags is a flag_array of NUM_EVENTS bits. If v7 bit is set, lookup the bit index in the map and set the corresponding bit in v8
	js_info <<  "Patching wEventFlags..." << std::endl;
	for (int i = 0; i < NUM_EVENTS; i++) {
		// check if the bit is set
		if (isFlagBitSet(it7, sym7.getPlayerDataAddress("wEventFlags"), i)) {
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
				setFlagBit(it8, sym8.getPlayerDataAddress("wEventFlags"), eventFlagIndexV8);
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
	clearDataBlock(sd, sym8.getPlayerDataAddress("wUsedObjectPals"), 0x10);

	// set it8 wLoadedObjPal0-7 to -1
	js_info <<  "Set wLoadedObjPal0-7 to -1..." << std::endl;
	fillDataBlock(sd, sym8.getPlayerDataAddress("wLoadedObjPal0"), 8, 0xFF);

	// copy from wCelebiEvent to wCurMapCallbacksPointer + 2
	js_info <<  "Copy from wCel***Event to wCurMapCallbacksPointer + 2..." << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wCelebiEvent"), sym8.getPlayerDataAddress("wCelebiEvent"), sym7.getPlayerDataAddress("wCurMapCallbacksPointer") + 2 - sym7.getPlayerDataAddress("wCelebiEvent"));

	// copy from wDecoBed to wFruitTreeFlags
	js_info <<  "Copy from wDecoBed to wFruitTreeFlags..." << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wDecoBed"), sym8.getPlayerDataAddress("wDecoBed"), sym7.getPlayerDataAddress("wFruitTreeFlags") - sym7.getPlayerDataAddress("wDecoBed"));

	// Copy wFruitTreeFlags
	js_info <<  "Copy wFruitTreeFlags..." << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wFruitTreeFlags"), sym8.getPlayerDataAddress("wFruitTreeFlags"), flag_array(NUM_FRUIT_TREES_V7));

	it8.seek(sym8.getPlayerDataAddress("wHiddenGrottoContents") - flag_array(NUM_LANDMARKS_V8));

	// Clear wNuzlockeLandmarkFlags
	js_info <<  "Clear wNuzlockeLandmarkFlags..." << std::endl;
	clearDataBlock(sd, it8.getAddress(), flag_array(NUM_LANDMARKS_V8));

	// clear wHiddenGrottoContents to wCurHiddenGrotto
	js_info <<  "Clear wHiddenGrottoContents to wCurHiddenGrotto..." << std::endl;
	clearDataBlock(sd, sym8.getPlayerDataAddress("wHiddenGrottoContents"), sym8.getPlayerDataAddress("wCurHiddenGrotto") - sym8.getPlayerDataAddress("wHiddenGrottoContents"));

	// copy from wLuckyNumberDayBuffer to wPhoneList
	js_info <<  "Copy from wLuckyNumberDayBuffer to wPhoneList..." << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wLuckyNumberDayBuffer"), sym8.getPlayerDataAddress("wLuckyNumberDayBuffer"), sym7.getPlayerDataAddress("wPhoneList") - sym7.getPlayerDataAddress("wLuckyNumberDayBuffer"));

	// Clear v8 wPhoneList
	js_info <<  "Clear wPhoneList..." << std::endl;
	clearDataBlock(sd, sym8.getPlayerDataAddress("wPhoneList"), flag_array(NUM_PHONE_CONTACTS_V8));

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
			// set the bit
			setFlagBit(it8, sym8.getPlayerDataAddress("wPhoneList"), contactIndexV8);
		}
		it7.next();
	}

	// copy from wParkBallsRemaining to wPlayerDataEnd
	js_info <<  "Copy from wParkBallsRemaining to wPlayerDataEnd..." << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wParkBallsRemaining"), sym8.getPlayerDataAddress("wParkBallsRemaining"), sym7.getPlayerDataAddress("wPlayerDataEnd") - sym7.getPlayerDataAddress("wParkBallsRemaining"));


	// clear wVisitedSpawns in v8 before patching
	js_info <<  "Clear wVisitedSpawns..." << std::endl;
	clearDataBlock(sd, sym8.getMapDataAddress("wVisitedSpawns"), NUM_SPAWNS_V8 / 8);

	// wVisitedSpawns is a flag_array of NUM_SPAWNS bits. If v7 bit is set, lookup the bit index in the map and set the corresponding bit in v8
	js_info <<  "Patching wVisitedSpawns..." << std::endl;
	it7.seek(sym7.getMapDataAddress("wVisitedSpawns"));
	it8.seek(sym8.getMapDataAddress("wVisitedSpawns"));
	// print current address
	js_info <<  "Current Address: " << std::hex << it7.getAddress() << std::endl;
	for (int i = 0; i < NUM_SPAWNS_V7; i++) {
		// check if the bit is set
		if (isFlagBitSet(it7, sym7.getMapDataAddress("wVisitedSpawns"), i)) {
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
				// set the bit
				setFlagBit(it8, sym8.getMapDataAddress("wVisitedSpawns"), spawnIndexV8);
			}
		}
	}

	// Copy from wDigWarpNumber to wCurMapDataEnd
	js_info <<  "Copy from wDigWarpNumber to wCurMapDataEnd..." << std::endl;
	copyDataBlock(sd, sym7.getMapDataAddress("wDigWarpNumber"), sym8.getMapDataAddress("wDigWarpNumber"), sym7.getMapDataAddress("wCurMapDataEnd") - sym7.getMapDataAddress("wDigWarpNumber"));

	mapAndWriteMapGroupNumber(sd, sym7.getMapDataAddress("wDigMapGroup"), sym8.getMapDataAddress("wDigMapGroup"), sym7.getMapDataAddress("wDigMapNumber"), sym8.getMapDataAddress("wDigMapNumber"), "wDigMap");
	mapAndWriteMapGroupNumber(sd, sym7.getMapDataAddress("wBackupMapGroup"), sym8.getMapDataAddress("wBackupMapGroup"), sym7.getMapDataAddress("wBackupMapNumber"), sym8.getMapDataAddress("wBackupMapNumber"), "wBackupMap");
	mapAndWriteMapGroupNumber(sd, sym7.getMapDataAddress("wLastSpawnMapGroup"), sym8.getMapDataAddress("wLastSpawnMapGroup"), sym7.getMapDataAddress("wLastSpawnMapNumber"), sym8.getMapDataAddress("wLastSpawnMapNumber"), "wLastSpawnMap");
	mapAndWriteMapGroupNumber(sd, sym7.getMapDataAddress("wMapGroup"), sym8.getMapDataAddress("wMapGroup"), sym7.getMapDataAddress("wMapNumber"), sym8.getMapDataAddress("wMapNumber"), "wMap");

	// Copy wPartyCount
	js_info <<  "Copy wPartyCount..." << std::endl;
	copyDataByte(sd, sym7.getPokemonDataAddress("wPartyCount"), sym8.getPokemonDataAddress("wPartyCount"));

	// copy wPartyMons PARTYMON_STRUCT_LENGTH * PARTY_LENGTH
	js_info <<  "Copy wPartyMons..." << std::endl;
	copyDataBlock(sd, sym7.getPokemonDataAddress("wPartyMons"), sym8.getPokemonDataAddress("wPartyMons"), PARTYMON_STRUCT_LENGTH * PARTY_LENGTH);

	// fix the party mon species
	js_info <<  "Fix party mon species..." << std::endl;
	for (int i = 0; i < PARTY_LENGTH; i++) {
		uint16_t species = it8.getByte(sym8.getPokemonDataAddress("wPartyMons") + i * PARTYMON_STRUCT_LENGTH);
		if (species == 0x00) {
			continue;
		}
		convertSpeciesAndForm(sd, sym8.getPokemonDataAddress("wPartyMons"), i, PARTYMON_STRUCT_LENGTH, MON_EXTSPECIES, MON_MOVES, species, seen_mons, caught_mons);
	}

	// fix the party mon items
	js_info <<  "Fix party mon items..." << std::endl;
	for (int i = 0; i < PARTY_LENGTH; i++) {
		uint8_t item = it8.getByte(sym8.getPokemonDataAddress("wPartyMons") + i * PARTYMON_STRUCT_LENGTH + MON_ITEM);
		convertItem(sd, sym8.getPokemonDataAddress("wPartyMons"), i, PARTYMON_STRUCT_LENGTH, MON_ITEM, item);
	}

	// fix party mon caught ball
	js_info <<  "Fix party mon caught ball..." << std::endl;
	for (int i = 0; i < PARTY_LENGTH; i++) {
		uint8_t caughtBall = it8.getByte(sym8.getPokemonDataAddress("wPartyMons") + i * PARTYMON_STRUCT_LENGTH + MON_CAUGHTBALL) & CAUGHT_BALL_MASK;
		convertCaughtBall(sd, sym8.getPokemonDataAddress("wPartyMons"), i, PARTYMON_STRUCT_LENGTH, MON_CAUGHTBALL, caughtBall);
	}

	// fix party mon caught locations
	js_info <<  "Fix party mon caught locations..." << std::endl;
	for (int i = 0; i < PARTY_LENGTH; i++) {
		uint8_t caughtLoc = it8.getByte(sym8.getPokemonDataAddress("wPartyMons") + i * PARTYMON_STRUCT_LENGTH + MON_CAUGHTLOCATION);
		convertCaughtLocation(sd, sym8.getPokemonDataAddress("wPartyMons"), i, PARTYMON_STRUCT_LENGTH, MON_CAUGHTLOCATION, caughtLoc);
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
		convertSpeciesAndForm(sd, sym8.getPokemonDataAddress("wBreedMon1"), 0, PARTYMON_STRUCT_LENGTH, MON_EXTSPECIES, MON_MOVES, species, seen_mons, caught_mons);
	}

	// fix wBreedMon1Item
	js_info <<  "Fix wBreedMon1Item..." << std::endl;
	uint8_t item = it8.getByte(sym8.getPokemonDataAddress("wBreedMon1Item"));
	if (item != 0x00) {
		convertItem(sd, sym8.getPokemonDataAddress("wBreedMon1"), 0, PARTYMON_STRUCT_LENGTH, MON_ITEM, item);
	}

	// fix wBreedMon1CaughtBall
	js_info <<  "Fix wBreedMon1CaughtBall..." << std::endl;
	uint8_t caughtBall = it8.getByte(sym8.getPokemonDataAddress("wBreedMon1CaughtBall")) & CAUGHT_BALL_MASK;
	convertCaughtBall(sd, sym8.getPokemonDataAddress("wBreedMon1"), 0, PARTYMON_STRUCT_LENGTH, MON_CAUGHTBALL, caughtBall);

	// fix wBreedMon1CaughtLocation
	js_info <<  "Fix wBreedMon1CaughtLocation..." << std::endl;
	uint8_t caughtLoc = it8.getByte(sym8.getPokemonDataAddress("wBreedMon1CaughtLocation"));
	convertCaughtLocation(sd, sym8.getPokemonDataAddress("wBreedMon1"), 0, PARTYMON_STRUCT_LENGTH, MON_CAUGHTLOCATION, caughtLoc);

	// fix wBreedMon2Species and wBreedMon2ExtSpecies
	js_info <<  "Fix wBreedMon2Species..." << std::endl;
	species = it8.getByte(sym8.getPokemonDataAddress("wBreedMon2Species"));
	if (species != 0x00) {
		convertSpeciesAndForm(sd, sym8.getPokemonDataAddress("wBreedMon2Species"), 0, PARTYMON_STRUCT_LENGTH, MON_EXTSPECIES, MON_MOVES, species, seen_mons, caught_mons);
	}

	// fix wBreedMon2Item
	js_info <<  "Fix wBreedMon2Item..." << std::endl;
	item = it8.getByte(sym8.getPlayerDataAddress("wBreedMon2Item"));
	if (item != 0x00) {
		convertItem(sd, sym8.getPokemonDataAddress("wBreedMon2"), 0, PARTYMON_STRUCT_LENGTH, MON_ITEM, item);
	}

	// fix wBreedMon2CaughtBall
	js_info <<  "Fix wBreedMon2CaughtBall..." << std::endl;
	caughtBall = it8.getByte(sym8.getPokemonDataAddress("wBreedMon2CaughtBall")) & CAUGHT_BALL_MASK;
	convertCaughtBall(sd, sym8.getPokemonDataAddress("wBreedMon2"), 0, PARTYMON_STRUCT_LENGTH, MON_CAUGHTBALL, caughtBall);

	// fix wBreedMon2CaughtLocation
	js_info <<  "Fix wBreedMon2CaughtLocation..." << std::endl;
	caughtLoc = it8.getByte(sym8.getPokemonDataAddress("wBreedMon2CaughtLocation"));
	convertCaughtLocation(sd, sym8.getPokemonDataAddress("wBreedMon2"), 0, PARTYMON_STRUCT_LENGTH, MON_CAUGHTLOCATION, caughtLoc);

	// Clear space from wLevelUpMonNickname to wBugContestBackupPartyCount in it8
	js_info <<  "Clear space from wLevelUpMonNickname to wBugContestBackupPartyCount..." << std::endl;
	clearDataBlock(sd, sym8.getPokemonDataAddress("wLevelUpMonNickname"), sym8.getPokemonDataAddress("wBugContestBackupPartyCount") - sym8.getPokemonDataAddress("wLevelUpMonNickname"));

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
		convertSpeciesAndForm(sd, sym8.getPokemonDataAddress("wContestMonSpecies"), 0, PARTYMON_STRUCT_LENGTH, MON_EXTSPECIES, MON_MOVES, species, seen_mons, caught_mons);
	}

	// fix wContestMonItem
	js_info <<  "Fix wContestMonItem..." << std::endl;
	item = it8.getByte(sym8.getPokemonDataAddress("wContestMonItem"));
	if (item != 0x00) {
		convertItem(sd, sym8.getPokemonDataAddress("wContestMon"), 0, PARTYMON_STRUCT_LENGTH, MON_ITEM, item);
	}

	// fix wContestMonCaughtBall
	js_info <<  "Fix wContestMonCaughtBall..." << std::endl;
	caughtBall = it8.getByte(sym8.getPokemonDataAddress("wContestMonCaughtBall")) & CAUGHT_BALL_MASK;
	convertCaughtBall(sd, sym8.getPokemonDataAddress("wContestMon"), 0, PARTYMON_STRUCT_LENGTH, MON_CAUGHTBALL, caughtBall);

	// fix wContestMonCaughtLocation
	js_info <<  "Fix wContestMonCaughtLocation..." << std::endl;
	caughtLoc = it8.getByte(sym8.getPokemonDataAddress("wContestMonCaughtLocation"));
	convertCaughtLocation(sd, sym8.getPokemonDataAddress("wContestMon"), 0, PARTYMON_STRUCT_LENGTH, MON_CAUGHTLOCATION, caughtLoc);

	mapAndWriteMapGroupNumber(sd, sym7.getPokemonDataAddress("wDunsparceMapGroup"), sym8.getPokemonDataAddress("wDunsparceMapGroup"), sym7.getPokemonDataAddress("wDunsparceMapNumber"), sym8.getPokemonDataAddress("wDunsparceMapNumber"), "wDunsp****");

	// Clear wRoamMons_CurMapNumber to wRoamMons_LastMapGroup in version 8
	// we will do this by clearing the 4 bytes before wBestMagikarpLengthMm since the symbols are not present in version 8
	js_info <<  "Clearing wRoamMons_CurMapNumber to wRoamMons_LastMapGroup..." << std::endl;
	it8.setByte(sym8.getPokemonDataAddress("wBestMagikarpLengthMm") - 1, 0x00);
	it8.setByte(sym8.getPokemonDataAddress("wBestMagikarpLengthMm") - 2, 0x00);
	it8.setByte(sym8.getPokemonDataAddress("wBestMagikarpLengthMm") - 3, 0x00);
	it8.setByte(sym8.getPokemonDataAddress("wBestMagikarpLengthMm") - 4, 0x00);

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
		if (isFlagBitSet(it7, sym7.getPokemonDataAddress("wPokedexCaught"), i)) {
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
				// set the bit
				setFlagBit(it8, sym8.getPokemonDataAddress("wPokedexCaught"), pokemonIndexV8);
			}
		}
	}
	// for each caught mon in vector caught_mons, set the corresponding bit in it8
	for (uint16_t mon : caught_mons){
		uint16_t monIndexV8 = mon - 1;
		setFlagBit(it8, sym8.getPokemonDataAddress("wPokedexCaught"), monIndexV8);
		js_info <<  "Found caught mon " << std::hex << static_cast<int>(mon) << std::endl;
	}

	// wPokedexSeen is a flag_array of NUM_POKEMON_V7 bits. If v7 bit is set, lookup the bit index in the map and set the corresponding bit in v8
	js_info <<  "Patching w****dexSeen..." << std::endl;
	it7.seek(sym7.getPokemonDataAddress("wPokedexSeen"));
	it8.seek(sym8.getPokemonDataAddress("wPokedexSeen"));
	for (int i = 0; i < NUM_POKEMON_V7; i++) {
		// check if the bit is set
		if (isFlagBitSet(it7, sym7.getPokemonDataAddress("wPokedexSeen"), i)) {
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
				// set the bit
				setFlagBit(it8, sym8.getPokemonDataAddress("wPokedexSeen"), pokemonIndexV8);
			}
		}
	}
	// for each seen mon in vector seen_mons, set the corresponding bit in it8
	for (uint16_t mon : seen_mons){
		uint16_t monIndexV8 = mon - 1;
		setFlagBit(it8, sym8.getPokemonDataAddress("wPokedexSeen"), monIndexV8);
		js_info <<  "Found seen mon " << std::hex << static_cast<int>(mon) << std::endl;
	}

	// set v8 wCurMapSceneScriptCount and wCurMapCallbackCount to 0
	// set v8 wCurMapSceneScriptPointer word to 0
	// this is done to prevent the game from running any map scripts on load
	js_info <<  "Set wCurMapSceneScriptCount and wCurMapCallbackCount to 0..." << std::endl;
	it8.seek(sym8.getPlayerDataAddress("wCurMapSceneScriptCount"));
	it8.setByte(0);
	it8.seek(sym8.getPlayerDataAddress("wCurMapCallbackCount"));
	it8.setByte(0);
	js_info <<  "Set wCurMapSceneScriptPointer to 0..." << std::endl;
	it8.seek(sym8.getPlayerDataAddress("wCurMapSceneScriptPointer"));
	it8.setWord(0);

	// write the new save version number big endian
	js_info <<  "Write new save version number..." << std::endl;
	uint16_t new_save_version = 0x08;
	save8.setWordBE(SAVE_VERSION_ABS_ADDRESS, new_save_version);

	// write new checksums to the version 8 save file
	js_info <<  "Write new checksums..." << std::endl;
	uint16_t new_checksum = calculateSaveChecksum(save8, sym8.getSRAMAddress("sGameData"), sym8.getSRAMAddress("sGameDataEnd"));
	save8.setWord(SAVE_CHECKSUM_ABS_ADDRESS, new_checksum);

	// write new backup checksums to the version 8 save file
	js_info <<  "Write new backup checksums..." << std::endl;
	uint16_t new_backup_checksum = calculateSaveChecksum(save8, sym8.getSRAMAddress("sBackupGameData"), sym8.getSRAMAddress("sBackupGameDataEnd"));
	save8.setWord(SAVE_BACKUP_CHECKSUM_ABS_ADDRESS, new_backup_checksum);

	// write the modified save file to the output file and print success message
	js_info <<  "Sucessfully patched to 3.0.0 save version 8!" << std::endl;
	return true;
}

// Calculate the newbox checksum for the given mon
// Reference: https://github.com/Rangi42/polishedcrystal/blob/9bit/docs/newbox_format.md#checksum
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

// Extract the stored newbox checksum for the given mon
// Reference: https://github.com/Rangi42/polishedcrystal/blob/9bit/docs/newbox_format.md#checksum
uint16_t extractStoredNewboxChecksum(const SaveBinary& save, uint32_t startAddress) {
	uint16_t storedChecksum = 0;

	// Read the most significant bits from 0x20 to 0x30
	for (int i = 0; i <= 0xF; ++i){
		uint8_t msb = (save.getByte(startAddress + 0x20 + i) & 0x80) >> 7;
		storedChecksum |= (msb << (0xF - i));
	}
	return storedChecksum;
}

// Write the newbox checksum for the given mon
// Reference: https://github.com/Rangi42/polishedcrystal/blob/9bit/docs/newbox_format.md#checksum
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

// Writes the default box name for the given box number
void writeDefaultBoxName(SaveBinary::Iterator& it, int boxNum) {
	// '  BOX XX' where XX is the box number
	uint8_t box_name_char[] = {0x7f, 0x7f, 0x81, 0xae, 0xb7, 0x7f, static_cast<uint8_t>(0xe0 + (boxNum / 10)), static_cast<uint8_t>(0xe0 + (boxNum % 10))};
	for (uint8_t box_char : box_name_char) {
		it.setByte(box_char);
		it.next();
	}
}

// Migrate the newbox box data from version 7 to version 8
void migrateBoxData(SourceDest &sd, const std::string &prefix) {
	// Clear the boxes
	js_info << "Clearing v8 " << prefix << " boxes..." << std::endl;
	for (int n = 1; n < NUM_BOXES_V8 + 1; n++) {
		clearDataBlock(sd, sd.destSym.getSRAMAddress(prefix + std::to_string(n)), NEWBOX_SIZE);
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

// converts the species and form for a given mon; check moves for pikachu surf and fly
void convertSpeciesAndForm(SourceDest &sd, uint32_t base_address, int i, int struct_length, int extspecies_offset, int moves_offset, uint16_t species, std::vector<uint16_t> &seen_mons, std::vector<uint16_t> &caught_mons) {
	uint16_t species_v8 = mapV7PkmnToV8(species);
	if (species_v8 == PIKACHU_V8){
		// for NUM_MOVES, scan for SURF_V7 and FLY_V7
		// if found, set form to PIKACHU_SURF_FORM_V7 or PIKACHU_FLY_FORM_V7 respectively
		uint8_t personality = sd.destSave.getByte(base_address + i * struct_length + extspecies_offset);
		uint8_t form = personality & FORM_MASK;
		for (int j = 0; j < NUM_MOVES; j++) {
			uint8_t move = sd.destSave.getByte(base_address + i * struct_length + moves_offset + j);
			if (move == SURF_V7) {
				form = PIKACHU_SURF_FORM_V7; // we use v7 cause we will let the convertSpeciesAndForm function handle the conversion
				break;
			} else if (move == FLY_V7) {
				form = PIKACHU_FLY_FORM_V7;
				break;
			}
		}
		personality &= ~FORM_MASK;
		personality |= form;
		sd.destSave.setByte(base_address + i * struct_length + extspecies_offset, personality);
	}
	convertSpeciesAndForm(sd, base_address, i, struct_length, extspecies_offset, species, seen_mons, caught_mons);
}

// converts the species and form for a given mon
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
		form = personality & FORM_MASK;
		js_info << "Species " << std::hex << species << " Form " << std::hex << static_cast<int>(form) << " ExtSpecies " << std::hex << extspecies_v8 << std::endl;
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

// converts the item for a given mon
void convertItem(SourceDest &sd, uint32_t base_address, int i, int struct_length, int item_offset, uint8_t item) {
	uint8_t item_v8 = mapV7ItemToV8(item);
	if (item_v8 == 0xFF) {
		js_error <<  "Item " << std::hex << static_cast<int>(item) << " not found in version 8 item list." << std::endl;
	} else {
		if (item != item_v8) {
			js_info <<  "Item " << std::hex << static_cast<int>(item) << " converted to " << std::hex << static_cast<int>(item_v8) << std::endl;
		}
		sd.destSave.setByte(base_address + i * struct_length + item_offset, item_v8);
	}
}

// a helper function to convert item lists
void convertItemList(SourceDest &sd, uint32_t numItemsAddr7, uint32_t itemsAddr7, uint32_t numItemsAddr8, uint32_t itemsAddr8, const std::string &itemListName){
	js_info << "Copy " << itemListName << "..." << std::endl;

	// Set iterators to the number-of-items address
	sd.sourceSave.seek(numItemsAddr7);
	uint8_t numItemsV7 = sd.sourceSave.getByte();
	uint8_t numItemsV8 = 0;
	sd.destSave.setByte(numItemsAddr8, numItemsV7);
	sd.sourceSave.next();
	sd.destSave.next();

	js_info << "Patching " << itemListName << "..." << std::endl;
	for (int i = 0; i < numItemsV7 + 1; i++) {
		uint8_t itemIDV7 = sd.sourceSave.getByte();
		sd.sourceSave.next();
		if (itemIDV7 == 0xFF) {
			sd.destSave.setByte(0xFF);
			break;
		}

		uint8_t itemIDV8 = mapV7ItemToV8(itemIDV7);
		if (itemIDV8 != 0xFF) {
			if (itemIDV7 != itemIDV8) {
				js_info << "Item " << std::hex << static_cast<int>(itemIDV7) << " converted to " << std::hex << static_cast<int>(itemIDV8) << std::endl;
			}
			numItemsV8++;
			sd.destSave.setByte(itemIDV8);
			sd.destSave.next();
			// copy quantity
			sd.destSave.setByte(sd.sourceSave.getByte());
			sd.sourceSave.next();
			sd.destSave.next();
		} else {
			js_error << "Item " << std::hex << static_cast<int>(itemIDV7) << " not found in version 8 item list." << std::endl;
			// skip quantity
			sd.sourceSave.next();
			sd.destSave.next();
		}
	}
	// Update number of items in v8
	sd.destSave.setByte(numItemsAddr8, numItemsV8);
}

// converts the caught location for a given mon
void convertCaughtLocation(SourceDest &sd, uint32_t base_address, int i, int struct_length, int caught_location_offset, uint8_t caught_location) {
	uint8_t caught_location_v8 = mapV7LandmarkToV8(caught_location);
	if (caught_location_v8 == 0xFF) {
		js_error <<  "Landmark " << std::hex << static_cast<int>(caught_location) << " not found in version 8 landmark list." << std::endl;
	} else {
		if (caught_location != caught_location_v8) {
			js_info <<  "Landmark " << std::hex << static_cast<int>(caught_location) << " converted to " << std::hex << static_cast<int>(caught_location_v8) << std::endl;
		}
		sd.destSave.setByte(base_address + i * struct_length + caught_location_offset, caught_location_v8);
	}
}

// converts the caught ball for a given mon
void convertCaughtBall(SourceDest &sd, uint32_t base_address, int i, int struct_length, int caught_ball_offset, uint8_t caught_ball) {
	uint8_t caught_ball_v8 = mapV7ItemToV8(caught_ball);
	if (caught_ball_v8 == 0xFF) {
		js_error <<  "Ball " << std::hex << static_cast<int>(caught_ball) << " not found in version 8 item list." << std::endl;
	} else {
		if (caught_ball != caught_ball_v8) {
			js_info <<  "Ball " << std::hex << static_cast<int>(caught_ball) << " converted to " << std::hex << static_cast<int>(caught_ball_v8) << std::endl;
		}
		uint8_t caught_ball_byte = sd.destSave.getByte(base_address + i * struct_length + caught_ball_offset);
		caught_ball_byte &= ~CAUGHT_BALL_MASK;
		caught_ball_byte |= caught_ball_v8 & CAUGHT_BALL_MASK;
		sd.destSave.setByte(caught_ball_v8);
	}
}

// Helper to map a map group/number pair
void mapAndWriteMapGroupNumber(SourceDest &sd, uint32_t mapGroupAddr7, uint32_t mapGroupAddr8, uint32_t mapNumberAddr7, uint32_t mapNumberAddr8, const std::string &mapName) {
	uint8_t groupV7 = sd.sourceSave.getByte(mapGroupAddr7);
	uint8_t numberV7 = sd.sourceSave.getByte(mapNumberAddr7);

	auto v8Map = mapv7toV8(groupV7, numberV7);
	if (groupV7 != std::get<0>(v8Map) || numberV7 != std::get<1>(v8Map)) {
		js_info << mapName << " Group " << std::hex << static_cast<int>(groupV7)
				<< " Number " << std::hex << static_cast<int>(numberV7)
				<< " converted to Group " << std::hex << (int)std::get<0>(v8Map)
				<< " Number " << std::hex << (int)std::get<1>(v8Map) << std::endl;
	}
	sd.destSave.setByte(mapGroupAddr8, std::get<0>(v8Map));
	sd.destSave.setByte(mapNumberAddr8, std::get<1>(v8Map));
}