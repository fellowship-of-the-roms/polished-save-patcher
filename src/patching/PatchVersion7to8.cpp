#include "patching/PatchVersion7to8.h"
#include "core/CommonPatchFunctions.h"
#include "core/SymbolDatabase.h"
#include "core/Logging.h"

namespace patchVersion7to8Namespace {

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
		js_error << "sGameData: " << std::hex << sym7.getSRAMAddress("sGameData") << std::endl;
		js_error << "sGameDataEnd: " << std::hex << sym7.getSRAMAddress("sGameDataEnd") << std::endl;
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

	// Due to a change in map blocks for the SHAMOUTI_POKECENTER, we don't support saving here.
	uint8_t prev_map_group = it7.getByte(sym7.getMapDataAddress("wBackupMapGroup"));
	uint8_t prev_map_num = it7.getByte(sym7.getMapDataAddress("wBackupMapNumber"));
	if (prev_map_group == SHAMOUTI_POKECENTER_1F.first && prev_map_num == SHAMOUTI_POKECENTER_1F.second) {
		js_error << "Due to a change in map blocks, we cannot support saving in the Shamouti PKMN center!" << std::endl;
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
	copyDataBlock(sd, sym7.getSRAMAddress("sBoxMons1"), sym8.getSRAMAddress("sBoxMons1A"), MONDB_ENTRIES_A_V8 * sizeof(savemon_struct_v8));

	js_info << "Clearing " << "sBoxMons1B" << "..." << std::endl;
	clearDataBlock(sd, sym8.getSRAMAddress("sBoxMons1B"), MONDB_ENTRIES_B_V8 * sizeof(savemon_struct_v8));
	js_info << "Clearing " << "sBoxMons1C" << "..." << std::endl;
	clearDataBlock(sd, sym8.getSRAMAddress("sBoxMons1C"), MONDB_ENTRIES_C_V8 * sizeof(savemon_struct_v8));

	// copy sBoxMons2 to SBoxMons2A
	js_info <<  "Copying from sBoxMons2 to sBoxMons2A..." << std::endl;
	copyDataBlock(sd, sym7.getSRAMAddress("sBoxMons2"), sym8.getSRAMAddress("sBoxMons2A"), MONDB_ENTRIES_A_V8 * sizeof(savemon_struct_v8));

	js_info << "Clearing " << "sBoxMons2B" << "..." << std::endl;
	clearDataBlock(sd, sym8.getSRAMAddress("sBoxMons2B"), MONDB_ENTRIES_B_V8 * sizeof(savemon_struct_v8));
	js_info << "Clearing " << "sBoxMons2C" << "..." << std::endl;
	clearDataBlock(sd, sym8.getSRAMAddress("sBoxMons2C"), MONDB_ENTRIES_C_V8 * sizeof(savemon_struct_v8));

	savemon_struct_v8 savemon;
	// Patching sBoxMons1A if checksums match
	js_info <<  "Checking sBoxMons1A checksums..." << std::endl;
	for (int i = 0; i < MONDB_ENTRIES_A_V8; i++) {
		it8.seek(sym8.getSRAMAddress("sBoxMons1A") + i * sizeof(savemon_struct_v8));
		uint16_t calc_checksum = calculateNewboxChecksum(save8, sym8.getSRAMAddress("sBoxMons1A") + i * sizeof(savemon_struct_v8));
		uint16_t cur_checksum = extractStoredNewboxChecksum(save8, sym8.getSRAMAddress("sBoxMons1A") + i * sizeof(savemon_struct_v8));
		if (calc_checksum == cur_checksum) {
			savemon = convertSavemonV7toV8(loadStruct<savemon_struct_v8>(it8, sym8.getSRAMAddress("sBoxMons1A") + i * sizeof(savemon_struct_v8)), seen_mons, caught_mons);
			writeStruct<savemon_struct_v8>(it8, sym8.getSRAMAddress("sBoxMons1A") + i * sizeof(savemon_struct_v8), savemon);
			// write the new checksum
			writeNewboxChecksum(save8, sym8.getSRAMAddress("sBoxMons1A") + i * sizeof(savemon_struct_v8));
		}
	}

	// Patching sBoxMons2A if checksums match
	js_info <<  "Checking sBoxMons2A checksums..." << std::endl;
	for (int i = 0; i < MONDB_ENTRIES_A_V8; i++) {
		it8.seek(sym8.getSRAMAddress("sBoxMons2A") + i * sizeof(savemon_struct_v8));
		uint16_t calc_checksum = calculateNewboxChecksum(save8, sym8.getSRAMAddress("sBoxMons2A") + i * sizeof(savemon_struct_v8));
		uint16_t cur_checksum = extractStoredNewboxChecksum(save8, sym8.getSRAMAddress("sBoxMons2A") + i * sizeof(savemon_struct_v8));
		if (calc_checksum == cur_checksum) {
			savemon = convertSavemonV7toV8(loadStruct<savemon_struct_v8>(it8, sym8.getSRAMAddress("sBoxMons2A") + i * sizeof(savemon_struct_v8)), seen_mons, caught_mons);
			writeStruct<savemon_struct_v8>(it8, sym8.getSRAMAddress("sBoxMons2A") + i * sizeof(savemon_struct_v8), savemon);
			// write the new checksum
			writeNewboxChecksum(save8, sym8.getSRAMAddress("sBoxMons2A") + i * sizeof(savemon_struct_v8));
		}
	}

	// copy from [sLinkBattleResults, sLinkBattleStatsEnd)
	js_info << "Copying from [sLinkBattleResults, sLinkBattleStatsEnd)" << std::endl;
	copyDataBlock(sd, sym7.getSRAMAddress("sLinkBattleResults"), sym8.getSRAMAddress("sLinkBattleResults"), sym7.getSRAMAddress("sLinkBattleStatsEnd") - sym7.getSRAMAddress("sLinkBattleResults"));

	// copy from [sBattleTowerChallengeState, sBT_OTMonParty3 + BATTLETOWER_PARTYDATA_SIZE]
	js_info << "Copying from [sBattleTowerChallengeState, sBT_OTMonParty3 + BATTLETOWER_PARTYDATA_SIZE]" << std::endl;
	copyDataBlock(sd, sym7.getSRAMAddress("sBattleTowerChallengeState"), sym8.getSRAMAddress("sBattleTowerChallengeState"), sym7.getSRAMAddress("sBT_OTMonParty3") + BATTLETOWER_PARTYDATA_SIZE + 1 - sym7.getSRAMAddress("sBattleTowerChallengeState"));

	// copy from [sPartyMail, sSaveVersion)
	js_info << "Copying from [sPartyMail, sSaveVersion)" << std::endl;
	copyDataBlock(sd, sym7.getSRAMAddress("sPartyMail"), sym8.getSRAMAddress("sPartyMail"), sym7.getSRAMAddress("sSaveVersion") - sym7.getSRAMAddress("sPartyMail"));

	// Fix sPartyMail
	mailmsg_struct_v8 mailmsg;
	js_info << "Fixing sPartyMail..." << std::endl;
	for (int i = 0; i < PARTY_LENGTH; i++) {
		mailmsg = convertMailmsgV7toV8(loadStruct<mailmsg_struct_v8>(it8, sym8.getSRAMAddress("sPartyMail") + i * sizeof(mailmsg_struct_v8)));
		writeStruct<mailmsg_struct_v8>(it8, sym8.getSRAMAddress("sPartyMail") + i * sizeof(mailmsg_struct_v8), mailmsg);
	}

	// Fix sPartyMailBackup
	js_info << "Fixing sPartyMailBackup..." << std::endl;
	for (int i = 0; i < PARTY_LENGTH; i++) {
		mailmsg = convertMailmsgV7toV8(loadStruct<mailmsg_struct_v8>(it8, sym8.getSRAMAddress("sPartyMailBackup") + i * sizeof(mailmsg_struct_v8)));
		writeStruct<mailmsg_struct_v8>(it8, sym8.getSRAMAddress("sPartyMailBackup") + i * sizeof(mailmsg_struct_v8), mailmsg);
	}

	// Fix sMailbox
	js_info << "Fixing sMailbox..." << std::endl;
	for (int i = 0; i < MAILBOX_CAPACITY; i++) {
		mailmsg = convertMailmsgV7toV8(loadStruct<mailmsg_struct_v8>(it8, sym8.getSRAMAddress("sMailbox") + i * sizeof(mailmsg_struct_v8)));
		writeStruct<mailmsg_struct_v8>(it8, sym8.getSRAMAddress("sMailbox") + i * sizeof(mailmsg_struct_v8), mailmsg);
	}

	// Fix sMailboxBackup
	js_info << "Fixing sMailboxBackup..." << std::endl;
	for (int i = 0; i < MAILBOX_CAPACITY; i++) {
		mailmsg = convertMailmsgV7toV8(loadStruct<mailmsg_struct_v8>(it8, sym8.getSRAMAddress("sMailboxBackup") + i * sizeof(mailmsg_struct_v8)));
		writeStruct<mailmsg_struct_v8>(it8, sym8.getSRAMAddress("sMailboxBackup") + i * sizeof(mailmsg_struct_v8), mailmsg);
	}

	// copy from [sUpgradeStep, sWritingBackup]
	js_info << "Copying from [sUpgradeStep, sWritingBackup + 1]" << std::endl;
	copyDataBlock(sd, sym7.getSRAMAddress("sUpgradeStep"), sym8.getSRAMAddress("sUpgradeStep"), sym7.getSRAMAddress("sWritingBackup") + 1 - sym7.getSRAMAddress("sUpgradeStep"));

	// copy from [sRTCStatusFlags, sLuckyIDNumber]
	js_info << "Copying from [sRTCStatusFlags, sLuckyIDNumber]" << std::endl;
	copyDataBlock(sd, sym7.getSRAMAddress("sRTCStatusFlags"), sym8.getSRAMAddress("sRTCStatusFlags"), sym7.getSRAMAddress("sLuckyIDNumber") + 2 - sym7.getSRAMAddress("sRTCStatusFlags"));

	// copy from [sOptions, sGameData)
	js_info <<  "Copying from [sOptions, sGameData)" << std::endl;
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

	// Reset Initial Options so the game asks the player to set them again.
	js_info <<  "Resetting Initial Options..." << std::endl;
	it8.setBit(RESET_INIT_OPTS);

	// copy from [wPlayerData, wObjectStructs)
	js_info <<  "Copying from [wPlayerData, wObjectStructs)" << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wPlayerData"), sym8.getPlayerDataAddress("wPlayerData"), sym7.getPlayerDataAddress("wObjectStructs") - sym7.getPlayerDataAddress("wPlayerData"));

	// clear unused bytes after wRTC, [wRTC + 4, wRTC + 8)
	js_info << "Clearing 4 unused bytes after wRTC" << std::endl;
	clearDataBlock(sd, sym8.getPlayerDataAddress("wRTC") + 4, 4);

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

	// copy from [wStoneTableAddress, wBattleFactorySwapCount]
	js_info <<  "Copying from [wStoneTableAddress, wBattleFactorySwapCount]" << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wObjectStructsEnd"), sym8.getPlayerDataAddress("wObjectStructsEnd"), sym7.getPlayerDataAddress("wBattleFactorySwapCount") + 1 - sym7.getPlayerDataAddress("wObjectStructsEnd"));

	// copy from [wMapObjects, wEnteredMapFromContinue)
	js_info <<  "Copying from [wMapObjects, wEnteredMapFromContinue)" << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wMapObjects"), sym8.getPlayerDataAddress("wMapObjects"), sym7.getPlayerDataAddress("wEnteredMapFromContinue") - sym7.getPlayerDataAddress("wMapObjects"));

	// copy it7 wEnteredMapFromContinue to it8 wEnteredMapFromContinue
	js_info <<  "Copy wEnteredMapFromContinue" << std::endl;
	copyDataByte(sd, sym7.getPlayerDataAddress("wEnteredMapFromContinue"), sym8.getPlayerDataAddress("wEnteredMapFromContinue"));

	js_info <<  "Copy wStatusFlags3" << std::endl;
	// copy it7 wStatusFlags3 to it8 wStatusFlags3
	copyDataByte(sd, sym7.getPlayerDataAddress("wStatusFlags3"), sym8.getPlayerDataAddress("wStatusFlags3"));

	// copy from [wTimeOfDayPal, wBadgesEnd)
	js_info <<  "Copying from [wTimeOfDayPal, wBadgesEnd)" << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wTimeOfDayPal"), sym8.getPlayerDataAddress("wTimeOfDayPal"), sym7.getPlayerDataAddress("wBadgesEnd") - sym7.getPlayerDataAddress("wTimeOfDayPal"));

	// clear unused bytes after wTimeOfDayPal, [wTimeOfDayPal + 1, wTimeOfDayPal + 5)
	js_info << "Clearing 4 unused bytes after wTimeOfDayPal" << std::endl;
	clearDataBlock(sd, sym8.getPlayerDataAddress("wTimeOfDayPal") + 1, 4);

	// clear save 8 [wPokemonJournals, wPokemonJournalsEnd)
	js_info <<  "Clearing save 8 [wPok****Journals, wPok****JournalsEnd)" << std::endl;
	clearDataBlock(sd, sym8.getPlayerDataAddress("wPokemonJournals"), sym8.getPlayerDataAddress("wPokemonJournalsEnd") - sym8.getPlayerDataAddress("wPokemonJournals"));

	// copy from [wPokemonJournals, wPokemonJournalsEnd)
	js_info <<  "Copying from [wPok****Journals, wPok****JournalsEnd)" << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wPokemonJournals"), sym8.getPlayerDataAddress("wPokemonJournals"), sym7.getPlayerDataAddress("wPokemonJournalsEnd") - sym7.getPlayerDataAddress("wPokemonJournals"));

	// copy from [wTMsHMs, wTMsHMsEnd)
	js_info <<  "Copying from [wTMsHMs, wTMsHMsEnd)" << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wTMsHMs"), sym8.getPlayerDataAddress("wTMsHMs"), sym7.getPlayerDataAddress("wTMsHMsEnd") - sym7.getPlayerDataAddress("wTMsHMs"));

	// clear save 8 wKeyItems
	js_info <<  "Clearing save 8 [wKeyItems, wKeyItemsEnd)" << std::endl;
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

	// copy from [wApricorns, wApricorns + NUM_APRICORNS)
	js_info << "Copying from [wApricorns, wApricorns + NUM_APRICORNS)" << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wApricorns"), sym8.getPlayerDataAddress("wApricorns"), NUM_APRICORNS);

	// copy from [wPokegearFlags, wAlways0SceneID)
	js_info <<  "Copy from [wPok*gearFlags, wAlways0SceneID)" << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wPokegearFlags"), sym8.getPlayerDataAddress("wPokegearFlags"), sym7.getPlayerDataAddress("wAlways0SceneID") - sym7.getPlayerDataAddress("wPokegearFlags"));

	// clear byte before wMooMooBerries
	js_info << "Clearing byte before wMooMooBerries..." << std::endl;
	it8.setByte(sym8.getPlayerDataAddress("wMooMooBerries") - 1, 0x00);

	// copy from [wAlways0SceneID, wEcruteakHouseSceneID]
	js_info <<  "Copy from [wAlways0SceneID, wEcru****HouseSceneID]" << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wAlways0SceneID"), sym8.getPlayerDataAddress("wAlways0SceneID"), sym7.getPlayerDataAddress("wEcruteakHouseSceneID") + 1 - sym7.getPlayerDataAddress("wAlways0SceneID"));

	// clear wEcruteakPokecenter1FSceneID as it is no longer used
	js_info <<  "Clear wEcru********center1FSceneID..." << std::endl;
	it8.setByte(sym8.getPlayerDataAddress("wEcruteakHouseSceneID") + 1, 0x00);

	// copy from [wElmsLabSceneID, wEventFlags)
	js_info <<  "Copy from [wE***LabSceneID, wEventFlags)" << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wElmsLabSceneID"), sym8.getPlayerDataAddress("wElmsLabSceneID"), sym7.getPlayerDataAddress("wEventFlags") - sym7.getPlayerDataAddress("wElmsLabSceneID"));

	// clear it8 wEventFlags
	js_info <<  "Clearing save 8 [wEventFalgs, wEventFlags + flag_array(NUM_EVENTS))" << std::endl;
	clearDataBlock(sd, sym8.getPlayerDataAddress("wEventFlags"), flag_array(NUM_EVENTS));

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

	// Initialize EVENT_CRYS_IN_NAVEL_ROCK
	js_info << "Initialize EVENT_CRYS_IN_NAVEL_ROCK..." << std::endl;
	setFlagBit(it8, sym8.getPlayerDataAddress("wEventFlags"), EVENT_CRYS_IN_NAVEL_ROCK);

	// copy v7 wCurBox to v8 wCurBox
	js_info <<  "Copy wCurBox" << std::endl;
	copyDataByte(sd, sym7.getPlayerDataAddress("wCurBox"), sym8.getPlayerDataAddress("wCurBox"));

	// clear from [wUsedObjectPals, wNeededPalIndex]
	js_info <<  "Clear from [wUsedObjectPals, wNeededPalIndex]" << std::endl;
	clearDataBlock(sd, sym8.getPlayerDataAddress("wUsedObjectPals"), sym8.getPlayerDataAddress("wNeededPalIndex") + 1 - sym8.getPlayerDataAddress("wUsedObjectPals"));

	// set it8 wLoadedObjPal0-7 to -1
	js_info <<  "Set wLoadedObjPal0-7 to -1..." << std::endl;
	fillDataBlock(sd, sym8.getPlayerDataAddress("wLoadedObjPal0"), 8, 0xFF);

	// clear 70 bytes after wEmotePal
	js_info << "Clear 70 bytes after wEmotePal..." << std::endl;
	clearDataBlock(sd, sym8.getPlayerDataAddress("wEmotePal") + 1, 70);

	// copy from [wCelebiEvent, wCurMapCallbacksPointer]
	js_info <<  "Copy from [wCel***Event, wCurMapCallbacksPointer]" << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wCelebiEvent"), sym8.getPlayerDataAddress("wCelebiEvent"), sym7.getPlayerDataAddress("wCurMapCallbacksPointer") + 2 - sym7.getPlayerDataAddress("wCelebiEvent"));

	// clear byte before wDecoBed
	js_info << "Clear unused byte before wDecoBed..." << std::endl;
	it8.setByte(sym8.getPlayerDataAddress("wDecoBed") - 1, 0x00);

	// copy from wDecoBed to wFruitTreeFlags
	js_info <<  "Copy from wDecoBed to wFruitTreeFlags..." << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wDecoBed"), sym8.getPlayerDataAddress("wDecoBed"), sym7.getPlayerDataAddress("wFruitTreeFlags") - sym7.getPlayerDataAddress("wDecoBed"));

	// Copy wFruitTreeFlags
	js_info <<  "Copy wFruitTreeFlags..." << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wFruitTreeFlags"), sym8.getPlayerDataAddress("wFruitTreeFlags"), flag_array(NUM_FRUIT_TREES_V7));

	// clear 19 bytes after wFruitTreeFlags
	js_info << "Clear 19 bytes after wFruitTreeFlags..." << std::endl;
	clearDataBlock(sd, sym8.getPlayerDataAddress("wFruitTreeFlags") + flag_array(NUM_FRUIT_TREES_V8), 19);

	// Clear wNuzlockeLandmarkFlags
	js_info <<  "Clear wNuzlockeLandmarkFlags..." << std::endl;
	clearDataBlock(sd, it8.getAddress(), flag_array(NUM_LANDMARKS_V8));

	// Clear from [wHiddenGrottoContents, wCurHiddenGrotto]
	js_info <<  "Clear from [wHiddenGrottoContents, wCurHiddenGrotto]" << std::endl;
	clearDataBlock(sd, sym8.getPlayerDataAddress("wHiddenGrottoContents"), sym8.getPlayerDataAddress("wCurHiddenGrotto") + 1 - sym8.getPlayerDataAddress("wHiddenGrottoContents"));

	// copy from [wLuckyNumberDayBuffer, wPhoneList)
	js_info <<  "Copy from [wLuckyNumberDayBuffer, wPhoneList)" << std::endl;
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

	// clear unused byte after wPhoneList (wPhoneListEnd)
	js_info << "Clear unused byte after wPhoneList..." << std::endl;
	it8.setByte(sym8.getPlayerDataAddress("wPhoneListEnd"), 0x00);

	// copy from [wParkBallsRemaining, wPlayerDataEnd)
	js_info <<  "Copy from [wParkBallsRemaining, wPlayerDataEnd)" << std::endl;
	copyDataBlock(sd, sym7.getPlayerDataAddress("wParkBallsRemaining"), sym8.getPlayerDataAddress("wParkBallsRemaining"), sym7.getPlayerDataAddress("wPlayerDataEnd") - sym7.getPlayerDataAddress("wParkBallsRemaining"));

	// clear wVisitedSpawns in v8 before patching
	js_info <<  "Clear wVisitedSpawns..." << std::endl;
	clearDataBlock(sd, sym8.getMapDataAddress("wVisitedSpawns"), flag_array(NUM_SPAWNS_V8));

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

	// Copy from [wDigWarpNumber, wCurMapDataEnd)
	js_info <<  "Copy from [wDigWarpNumber, wCurMapDataEnd)" << std::endl;
	copyDataBlock(sd, sym7.getMapDataAddress("wDigWarpNumber"), sym8.getMapDataAddress("wDigWarpNumber"), sym7.getMapDataAddress("wCurMapDataEnd") - sym7.getMapDataAddress("wDigWarpNumber"));

	mapAndWriteMapGroupNumber(sd, sym7.getMapDataAddress("wDigMapGroup"), sym8.getMapDataAddress("wDigMapGroup"), sym7.getMapDataAddress("wDigMapNumber"), sym8.getMapDataAddress("wDigMapNumber"), "wDigMap");
	mapAndWriteMapGroupNumber(sd, sym7.getMapDataAddress("wBackupMapGroup"), sym8.getMapDataAddress("wBackupMapGroup"), sym7.getMapDataAddress("wBackupMapNumber"), sym8.getMapDataAddress("wBackupMapNumber"), "wBackupMap");
	mapAndWriteMapGroupNumber(sd, sym7.getMapDataAddress("wLastSpawnMapGroup"), sym8.getMapDataAddress("wLastSpawnMapGroup"), sym7.getMapDataAddress("wLastSpawnMapNumber"), sym8.getMapDataAddress("wLastSpawnMapNumber"), "wLastSpawnMap");
	mapAndWriteMapGroupNumber(sd, sym7.getMapDataAddress("wMapGroup"), sym8.getMapDataAddress("wMapGroup"), sym7.getMapDataAddress("wMapNumber"), sym8.getMapDataAddress("wMapNumber"), "wMap");

	// Copy wPartyCount
	js_info <<  "Copy wPartyCount..." << std::endl;
	copyDataByte(sd, sym7.getPokemonDataAddress("wPartyCount"), sym8.getPokemonDataAddress("wPartyCount"));

	// clear 7 unused bytes after wPartyCount
	js_info << "Clear 7 unused bytes after wPartyCount..." << std::endl;
	clearDataBlock(sd, sym8.getPokemonDataAddress("wPartyCount") + 1, 7);

	// copy wPartyMons sizeof(party_struct_v7) * PARTY_LENGTH
	js_info <<  "Copy wPartyMons..." << std::endl;
	copyDataBlock(sd, sym7.getPokemonDataAddress("wPartyMons"), sym8.getPokemonDataAddress("wPartyMons"), sizeof(party_struct_v8) * PARTY_LENGTH);

	party_struct_v8 partymon;
	// fix the party mons
	js_info <<  "Fix party mons..." << std::endl;
	for (int i = 0; i < PARTY_LENGTH; i++) {
		uint16_t species = it8.getByte(sym8.getPokemonDataAddress("wPartyMons") + i * sizeof(party_struct_v8));
		if (species == 0x00) {
			continue;
		}
		partymon = convertPartyV7toV8(loadStruct<party_struct_v8>(it8, sym8.getPokemonDataAddress("wPartyMons") + i * sizeof(party_struct_v8)), seen_mons, caught_mons);
		writeStruct<party_struct_v8>(it8, sym8.getPokemonDataAddress("wPartyMons") + i * sizeof(party_struct_v8), partymon);
	}

	// copy wPartyMonOTs
	js_info <<  "Copy wPartyMonOTs..." << std::endl;
	copyDataBlock(sd, sym7.getPokemonDataAddress("wPartyMonOTs"), sym8.getPokemonDataAddress("wPartyMonOTs"), PARTY_LENGTH * (PLAYER_NAME_LENGTH + 3));

	// copy wPartyMonNicknames PARTY_LENGTH * MON_NAME_LENGTH
	js_info <<  "Copy wPartyMonNicknames..." << std::endl;
	copyDataBlock(sd, sym7.getPokemonDataAddress("wPartyMonNicknames"), sym8.getPokemonDataAddress("wPartyMonNicknames"), MON_NAME_LENGTH * PARTY_LENGTH);

	// clear unused byte after wPartyMonNicknames
	js_info << "Clear unused byte after wPartyMonNicknames..." << std::endl;
	it8.setByte(sym8.getPokemonDataAddress("wPartyMonNicknamesEnd"), 0x00);

	// We will convert the pokedex caught flags last as we need the full list of caught mons

	// clear unused byte after wPokedexCaught
	js_info << "Clear unused byte after wPok*dexCaught..." << std::endl;
	it8.setByte(sym8.getPokemonDataAddress("wEndPokedexCaught"), 0x00);

	// We will convert the pokedex seen flags last as we need the full list of seen mons

	// clear unused byte after wPokedexSeen
	js_info << "Clear unused byte after wPok*dexSeen..." << std::endl;
	it8.setByte(sym8.getPokemonDataAddress("wEndPokedexSeen"), 0x00);

	// copy wUnlockedUnowns
	js_info <<  "Copy wUnlockedUnowns..." << std::endl;
	copyDataByte(sd, sym7.getPokemonDataAddress("wUnlockedUnowns"), sym8.getPokemonDataAddress("wUnlockedUnowns"));

	// clear 2 unused bytes after wUnlockedUnowns
	js_info << "Clear 2 unused bytes after wUnlockedUnowns..." << std::endl;
	clearDataBlock(sd, sym8.getPokemonDataAddress("wUnlockedUnowns") + 1, 2);

	// Copy from [wDayCareMan, wBreedMon2 + sizeof(breed_struct_mon)
	js_info <<  "Copy [wDayCareMan, wBreedMon2 + sizeof(breed_struct_mon)" << std::endl;
	copyDataBlock(sd, sym7.getPokemonDataAddress("wDayCareMan"), sym8.getPokemonDataAddress("wDayCareMan"), sym7.getPokemonDataAddress("wBreedMon2") + sizeof(breedmon_struct_v8) - sym7.getPokemonDataAddress("wDayCareMan"));

	breedmon_struct_v8 breedmon;
	// fix and copy wBreedMon1
	js_info <<  "fix and copy wBreedMon1..." << std::endl;
	uint16_t species = it8.getByte(sym8.getPokemonDataAddress("wBreedMon1Species"));
	if (species != 0x00) {
		breedmon = convertBreedmonV7toV8(loadStruct<breedmon_struct_v8>(it8, sym8.getPokemonDataAddress("wBreedMon1")), seen_mons, caught_mons);
		writeStruct<breedmon_struct_v8>(it8, sym8.getPokemonDataAddress("wBreedMon1"), breedmon);
	}

	// fix wBreedMon2...
	js_info <<  "Fix wBreedMon2Species..." << std::endl;
	species = it8.getByte(sym8.getPokemonDataAddress("wBreedMon2Species"));
	if (species != 0x00) {
		breedmon = convertBreedmonV7toV8(loadStruct<breedmon_struct_v8>(it8, sym8.getPokemonDataAddress("wBreedMon2")), seen_mons, caught_mons);
		writeStruct<breedmon_struct_v8>(it8, sym8.getPokemonDataAddress("wBreedMon2"), breedmon);
	}

	// Clear from [wLevelUpMonNickname to wBugContestBackupPartyCount)
	js_info <<  "Clear from [wLevelUpMonNickname to wBugContestBackupPartyCount)" << std::endl;
	clearDataBlock(sd, sym8.getPokemonDataAddress("wLevelUpMonNickname"), sym8.getPokemonDataAddress("wBugContestBackupPartyCount") - sym8.getPokemonDataAddress("wLevelUpMonNickname"));

	// Clear wBugContestBackupPartyCount
	js_info <<  "Clear wBugContestBackupPartyCount..." << std::endl;
	it8.setByte(sym8.getPokemonDataAddress("wBugContestBackupPartyCount"), 0x00);

	// copy from wContestMon to wPokemonDataEnd
	js_info <<  "Copy from wContestMon to w****monDataEnd..." << std::endl;
	copyDataBlock(sd, sym7.getPokemonDataAddress("wContestMon"), sym8.getPokemonDataAddress("wContestMon"), sym7.getPokemonDataAddress("wPokemonDataEnd") - sym7.getPokemonDataAddress("wContestMon"));

	// fix wContestMonSpecies and wContestMonExtSpecies
	js_info <<  "Fix wContestMon..." << std::endl;
	species = it8.getByte(sym8.getPokemonDataAddress("wContestMonSpecies"));
	if (species != 0x00) {
		partymon = convertPartyV7toV8(loadStruct<party_struct_v8>(it8, sym8.getPokemonDataAddress("wContestMon")), seen_mons, caught_mons);
		writeStruct<party_struct_v8>(it8, sym8.getPokemonDataAddress("wContestMon"), partymon);
	}

	mapAndWriteMapGroupNumber(sd, sym7.getPokemonDataAddress("wDunsparceMapGroup"), sym8.getPokemonDataAddress("wDunsparceMapGroup"), sym7.getPokemonDataAddress("wDunsparceMapNumber"), sym8.getPokemonDataAddress("wDunsparceMapNumber"), "wDunsp****");

	roam_struct_v8 roammon;
	js_info << "Fix wRoamMon1..." << std::endl;
	species = it8.getByte(sym8.getPokemonDataAddress("wRoamMon1Species"));
	if (species != 0x00) {
		js_info << "wRoamMon1Species is 0x" << std::hex << static_cast<int>(species) << " converting struct" << std::endl;
		roammon = convertRoamV7toV8(loadStruct<roam_struct_v8>(it8, sym8.getPokemonDataAddress("wRoamMon1")));
		writeStruct<roam_struct_v8>(it8, sym8.getPokemonDataAddress("wRoamMon1"), roammon);
	}
	else {
		js_info << "wRoamMon1Species is 0x00, setting map to -1, -1" << std::endl;
		roammon = loadStruct<roam_struct_v8>(it8, sym8.getPokemonDataAddress("wRoamMon1"));
		roammon.setMap(std::tuple <uint8_t, uint8_t>(-1, -1));
		writeStruct<roam_struct_v8>(it8, sym8.getPokemonDataAddress("wRoamMon1"), roammon);
	}

	js_info << "Fix wRoamMon2..." << std::endl;
	species = it8.getByte(sym8.getPokemonDataAddress("wRoamMon2Species"));
	if (species != 0x00) {
		js_info << "wRoamMon2Species is 0x" << std::hex << static_cast<int>(species) << " converting struct" << std::endl;
		roammon = convertRoamV7toV8(loadStruct<roam_struct_v8>(it8, sym8.getPokemonDataAddress("wRoamMon2")));
		writeStruct<roam_struct_v8>(it8, sym8.getPokemonDataAddress("wRoamMon2"), roammon);
	}
	else {
		js_info << "wRoamMon2Species is 0x00, setting map to -1, -1" << std::endl;
		roammon = loadStruct<roam_struct_v8>(it8, sym8.getPokemonDataAddress("wRoamMon2"));
		roammon.setMap(std::tuple <uint8_t, uint8_t>(-1, -1));
		writeStruct<roam_struct_v8>(it8, sym8.getPokemonDataAddress("wRoamMon2"), roammon);
	}

	js_info << "Fix wRoamMon3..." << std::endl;
	species = it8.getByte(sym8.getPokemonDataAddress("wRoamMon3Species"));
	if (species != 0x00) {
		js_info << "wRoamMon3Species is 0x" << std::hex << static_cast<int>(species) << " converting struct" << std::endl;
		roammon = convertRoamV7toV8(loadStruct<roam_struct_v8>(it8, sym8.getPokemonDataAddress("wRoamMon3")));
		writeStruct<roam_struct_v8>(it8, sym8.getPokemonDataAddress("wRoamMon3"), roammon);
	} else {
		js_info << "wRoamMon3Species is 0x00, setting map to -1, -1" << std::endl;
		roammon = loadStruct<roam_struct_v8>(it8, sym8.getPokemonDataAddress("wRoamMon3"));
		roammon.setMap(std::tuple <uint8_t, uint8_t>(-1, -1));
		writeStruct<roam_struct_v8>(it8, sym8.getPokemonDataAddress("wRoamMon3"), roammon);
	}

	// clear 4 unused bytes after wRoamMon3
	js_info << "Clear 4 unused bytes after wRoamMon3..." << std::endl;
	clearDataBlock(sd, sym8.getPokemonDataAddress("wRoamMon3") + 1, 4);

	// fix wRegisteredItems...
	js_info << "Fix wRegisteredItems..." << std::endl;
	for (int i = 0; i < 4; i++) {
		uint8_t item = it8.getByte(sym8.getPokemonDataAddress("wRegisteredItems") + i);
		if (item == 0x00) { continue; }
		item = mapV7KeyItemToV8(item - 1);
		if (item != 0xFF) {
			it8.setByte(sym8.getPokemonDataAddress("wRegisteredItems") + i, item);
		} else {
			js_warning << "Registered Item " << std::hex << static_cast<int>(it8.getByte(sym8.getPokemonDataAddress("wRegisteredItems") + i)) << " not found in version 8 key item list." << std::endl;
			it8.setByte(sym8.getPokemonDataAddress("wRegisteredItems") + i, 0x00);
		}
	}

	// copy sCheckValue2
	js_info <<  "Copy sCheckValue2..." << std::endl;
	copyDataByte(sd, sym7.getSRAMAddress("sCheckValue2"), sym8.getSRAMAddress("sCheckValue2"));

	// copy it8 Save to it8 Backup Save
	js_info <<  "Copy Main Save to Backup Save..." << std::endl;
	for (int i = 0; i < sym8.getSRAMAddress("sCheckValue2") + 1 - sym8.getSRAMAddress("sOptions"); i++) {
		save8.setByte(sym8.getSRAMAddress("sBackupOptions") + i, save8.getByte(sym8.getSRAMAddress("sOptions") + i));
	}

	// copy from sHallOfFame to sHallOfFameEnd
	js_info <<  "Copy from sHallOfFame to sHallOfFameEnd..." << std::endl;
	copyDataBlock(sd, sym7.getSRAMAddress("sHallOfFame"), sym8.getSRAMAddress("sHallOfFame"), sym8.getSRAMAddress("sHallOfFameEnd") - sym8.getSRAMAddress("sHallOfFame"));

	hofmon_struct_v8 hofmon;
	// fix the hall of fame mon species
	js_info <<  "Fix hall of fame mon..." << std::endl;
	for (int i = 0; i < NUM_HOF_TEAMS_V8; i++) {
		for (int j = 0; j < PARTY_LENGTH; j++){
			it8.seek(sym8.getSRAMAddress("sHallOfFame01Mon1") + i * HOF_LENGTH);
			uint16_t species = it8.getByte(it8.getAddress() + j * sizeof(hofmon_struct_v8));
			if (species == 0x00) {
				continue;
			}
			hofmon = convertHofmonV7toV8(loadStruct<hofmon_struct_v8>(it8, sym8.getSRAMAddress("sHallOfFame01Mon1") + i * HOF_LENGTH + j * sizeof(hofmon_struct_v8)), seen_mons, caught_mons);
			writeStruct<hofmon_struct_v8>(it8, sym8.getSRAMAddress("sHallOfFame01Mon1") + i * HOF_LENGTH + j * sizeof(hofmon_struct_v8), hofmon);
		}
	}

	// clear wPokedexCaught in v8 before patching
	js_info << "Clear w****dexCaught..." << std::endl;
	clearDataBlock(sd, sym8.getPokemonDataAddress("wPokedexCaught"), flag_array(NUM_UNIQUE_POKEMON_V8));


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
					js_info <<  "dex Caught Mon " << std::hex << static_cast<int>(pokemonIndex) << " converted to " << std::hex << static_cast<int>(pokemonIndexV8) << std::endl;
				}
				// set the bit
				setFlagBit(it8, sym8.getPokemonDataAddress("wPokedexCaught"), pokemonIndexV8);
			}
		}
	}
	// for each caught mon in vector caught_mons, set the corresponding bit in it8
	for (uint16_t mon : caught_mons){
		uint16_t monIndexV8 = mon - 1;
		if (!isFlagBitSet(it8, sym8.getPokemonDataAddress("wPokedexCaught"), monIndexV8)) {
			setFlagBit(it8, sym8.getPokemonDataAddress("wPokedexCaught"), monIndexV8);
			js_info << "Found caught mon " << std::hex << static_cast<int>(mon) << std::endl;
		}
	}

	// clear wPokedexSeen in v8 before patching
	js_info << "Clear w****dexSeen..." << std::endl;
	clearDataBlock(sd, sym8.getPokemonDataAddress("wPokedexSeen"), flag_array(NUM_UNIQUE_POKEMON_V8));

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
					js_info <<  "Dex Seen Mon " << std::hex << static_cast<int>(pokemonIndex) << " converted to " << std::hex << static_cast<int>(pokemonIndexV8) << std::endl;
				}
				// set the bit
				setFlagBit(it8, sym8.getPokemonDataAddress("wPokedexSeen"), pokemonIndexV8);
			}
		}
	}
	// for each seen mon in vector seen_mons, set the corresponding bit in it8
	for (uint16_t mon : seen_mons){
		uint16_t monIndexV8 = mon - 1;
		if (!isFlagBitSet(it8, sym8.getPokemonDataAddress("wPokedexSeen"), monIndexV8)) {
			setFlagBit(it8, sym8.getPokemonDataAddress("wPokedexSeen"), monIndexV8);
			js_info << "Found seen mon " << std::hex << static_cast<int>(mon) << std::endl;
		}
	}

	// Clear wPlayerCaught and wPlayerCaught2
	js_info << "Clear wPlayerCaught..." << std::endl;
	it8.setByte(sym8.getPlayerDataAddress("wPlayerCaught"), 0x00);
	it8.setByte(sym8.getPlayerDataAddress("wPlayerCaught2"), 0x00);
	// check if HO_OH_V8 is in caught_mons, if so set bit 0 in wPlayerCaught
	if (std::find(caught_mons.begin(), caught_mons.end(), HO_OH_V8) != caught_mons.end()) {
		setFlagBit(it8, sym8.getPlayerDataAddress("wPlayerCaught"), 0);
		js_info << "Found caught mon " << std::hex << static_cast<int>(HO_OH_V8) << std::endl;
	}
	// check if LUGIA_V8 is in caught_mons, if so set bit 1 in wPlayerCaught
	if (std::find(caught_mons.begin(), caught_mons.end(), LUGIA_V8) != caught_mons.end()) {
		setFlagBit(it8, sym8.getPlayerDataAddress("wPlayerCaught"), 1);
		js_info << "Found caught mon " << std::hex << static_cast<int>(LUGIA_V8) << std::endl;
	}
	// check if RAIKOU_V8 is in caught_mons, if so set bit 2 in wPlayerCaught
	if (std::find(caught_mons.begin(), caught_mons.end(), RAIKOU_V8) != caught_mons.end()) {
		setFlagBit(it8, sym8.getPlayerDataAddress("wPlayerCaught"), 2);
		js_info << "Found caught mon " << std::hex << static_cast<int>(RAIKOU_V8) << std::endl;
	}
	// check if ENTEI_V8 is in caught_mons, if so set bit 3 in wPlayerCaught
	if (std::find(caught_mons.begin(), caught_mons.end(), ENTEI_V8) != caught_mons.end()) {
		setFlagBit(it8, sym8.getPlayerDataAddress("wPlayerCaught"), 3);
		js_info << "Found caught mon " << std::hex << static_cast<int>(ENTEI_V8) << std::endl;
	}
	// check if SUICUNE_V8 is in caught_mons, if so set bit 4 in wPlayerCaught
	if (std::find(caught_mons.begin(), caught_mons.end(), SUICUNE_V8) != caught_mons.end()) {
		setFlagBit(it8, sym8.getPlayerDataAddress("wPlayerCaught"), 4);
		js_info << "Found caught mon " << std::hex << static_cast<int>(SUICUNE_V8) << std::endl;
	}
	// check if ARTICUNO_V8 is in caught_mons, if so set bit 5 in wPlayerCaught
	if (std::find(caught_mons.begin(), caught_mons.end(), ARTICUNO_V8) != caught_mons.end()) {
		setFlagBit(it8, sym8.getPlayerDataAddress("wPlayerCaught"), 5);
		js_info << "Found caught mon " << std::hex << static_cast<int>(ARTICUNO_V8) << std::endl;
	}
	// check if ZAPDOS_V8 is in caught_mons, if so set bit 6 in wPlayerCaught
	if (std::find(caught_mons.begin(), caught_mons.end(), ZAPDOS_V8) != caught_mons.end()) {
		setFlagBit(it8, sym8.getPlayerDataAddress("wPlayerCaught"), 6);
		js_info << "Found caught mon " << std::hex << static_cast<int>(ZAPDOS_V8) << std::endl;
	}
	// check if MOLTRES_V8 is in caught_mons, if so set bit 7 in wPlayerCaught
	if (std::find(caught_mons.begin(), caught_mons.end(), MOLTRES_V8) != caught_mons.end()) {
		setFlagBit(it8, sym8.getPlayerDataAddress("wPlayerCaught"), 7);
		js_info << "Found caught mon " << std::hex << static_cast<int>(MOLTRES_V8) << std::endl;
	}
	// check if MEW_V8 is in caught_mons, if so set bit 0 in wPlayerCaught2
	if (std::find(caught_mons.begin(), caught_mons.end(), MEW_V8) != caught_mons.end()) {
		setFlagBit(it8, sym8.getPlayerDataAddress("wPlayerCaught2"), 0);
		js_info << "Found caught mon " << std::hex << static_cast<int>(MEW_V8) << std::endl;
	}
	// check if MEWTWO_V8 is in caught_mons, if so set bit 1 in wPlayerCaught2
	if (std::find(caught_mons.begin(), caught_mons.end(), MEWTWO_V8) != caught_mons.end()) {
		setFlagBit(it8, sym8.getPlayerDataAddress("wPlayerCaught2"), 1);
		js_info << "Found caught mon " << std::hex << static_cast<int>(MEWTWO_V8) << std::endl;
	}
	// check if CELEBI_V8 is in caught_mons, if so set bit 2 in wPlayerCaught2
	if (std::find(caught_mons.begin(), caught_mons.end(), CELEBI_V8) != caught_mons.end()) {
		setFlagBit(it8, sym8.getPlayerDataAddress("wPlayerCaught2"), 2);
		js_info << "Found caught mon " << std::hex << static_cast<int>(CELEBI_V8) << std::endl;
	}
	// check if SUDOWOODO_V8 is in caught_mons, if so set bit 3 in wPlayerCaught2
	if (std::find(caught_mons.begin(), caught_mons.end(), SUDOWOODO_V8) != caught_mons.end()) {
		setFlagBit(it8, sym8.getPlayerDataAddress("wPlayerCaught2"), 3);
		js_info << "Found caught mon " << std::hex << static_cast<int>(SUDOWOODO_V8) << std::endl;
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

// Helper to map a map group/number pair
void mapAndWriteMapGroupNumber(SourceDest &sd, uint32_t mapGroupAddr7, uint32_t mapGroupAddr8, uint32_t mapNumberAddr7, uint32_t mapNumberAddr8, const std::string &mapName) {
	uint8_t groupV7 = sd.sourceSave.getByte(mapGroupAddr7);
	uint8_t numberV7 = sd.sourceSave.getByte(mapNumberAddr7);
	js_info << "Patching " << mapName << "..." << std::endl;
	js_info << "V7 Group " << std::hex << static_cast<int>(groupV7) << " V7 Number " << std::hex << static_cast<int>(numberV7) << std::endl;
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

savemon_struct_v8 convertSavemonV7toV8(const savemon_struct_v8& savemon, std::vector<uint16_t>& seen_mons, std::vector<uint16_t>& caught_mons) {
	// input is a savemon_struct_v8 because we convert in place
	savemon_struct_v8 new_savemon;
	uint16_t species_v8 = mapV7PkmnToV8(savemon.species);
	if (species_v8 == INVALID_SPECIES) {
		js_error << "Savemon species " << std::hex << static_cast<int>(savemon.species) << " not found in version 8 mon list." << std::endl;
		return savemon;
	}
	if (savemon.species != species_v8) {
		js_info << "Savemon species " << std::hex << static_cast<int>(savemon.species) << " converted to " << std::hex << static_cast<int>(species_v8) << std::endl;
	}
	new_savemon.setExtSpecies(species_v8);
	uint8_t item = mapV7ItemToV8(savemon.item);
	if (item == 0xFF) {
		js_error << "Savemon item " << std::hex << static_cast<int>(savemon.item) << " not found in version 8 item list." << std::endl;
		new_savemon.item = 0; // NO_ITEM
	}
	else {
		if (savemon.item != item) {
			js_info << "Savemon item " << std::hex << static_cast<int>(savemon.item) << " converted to " << std::hex << static_cast<int>(item) << std::endl;
		}
		new_savemon.item = item;
	}
	std::memcpy(new_savemon.moves, savemon.moves, sizeof(savemon.moves));
	new_savemon.id = savemon.id;
	std::memcpy(new_savemon.exp, savemon.exp, sizeof(savemon.exp));
	std::memcpy(new_savemon.evs, savemon.evs, sizeof(savemon.evs));
	std::memcpy(new_savemon.dvs, savemon.dvs, sizeof(savemon.dvs));
	new_savemon.setShiny(savemon.isShiny());
	new_savemon.setAbility(savemon.getAbility());
	new_savemon.setNature(savemon.getNature());
	new_savemon.setGender(savemon.getGender());
	new_savemon.setEgg(savemon.isEgg());
	if (savemon.getForm() == 0x00) {
		new_savemon.setForm(0x01);
	} else {
		new_savemon.setForm(savemon.getForm());
	}
	uint16_t extspecies_v8;
	if (species_v8 == PIKACHU_V8) {
		// for NUM_MOVES, scan for SURF_V7 and FLY_V7
		for (int j = 0; j < NUM_MOVES; j++) {
			uint8_t move = savemon.moves[j];
			if (move == SURF_V7) {
				new_savemon.setForm(PIKACHU_SURF_FORM_V7);
				extspecies_v8 = mapV7SpeciesFormToV8Extspecies(savemon.species, PIKACHU_SURF_FORM_V7);
				break;
			}
			else if (move == FLY_V7) {
				new_savemon.setForm(PIKACHU_FLY_FORM_V7);
				extspecies_v8 = mapV7SpeciesFormToV8Extspecies(savemon.species, PIKACHU_FLY_FORM_V7);
				break;
			}
		}
	}
	else {
		extspecies_v8 = mapV7SpeciesFormToV8Extspecies(savemon.species, savemon.getForm());
	}
	if (extspecies_v8 != INVALID_SPECIES) {
		seen_mons.push_back(extspecies_v8);
		caught_mons.push_back(extspecies_v8);
	} else {
		seen_mons.push_back(new_savemon.getExtSpecies());
		caught_mons.push_back(new_savemon.getExtSpecies());
	}
	if (species_v8 == MAGIKARP_V8) {
		new_savemon.setForm(mapV7MagikarpFormToV8(savemon.getForm()));
	}
	if (species_v8 == GYARADOS_V8) {
		if (savemon.getForm() == GYARADOS_RED_FORM_V7) {
			new_savemon.setForm(GYARADOS_RED_FORM_V8);
		}
	}
	new_savemon.ppups = savemon.ppups;
	new_savemon.happiness = savemon.happiness;
	new_savemon.pkrus = savemon.pkrus;
	new_savemon.setCaughtTime(savemon.getCaughtTime());
	uint8_t caught_ball_v8 = mapV7ItemToV8(savemon.getCaughtBall());
	if (caught_ball_v8 == 0xFF) {
		js_error << "Savemon ball " << std::hex << static_cast<int>(savemon.getCaughtBall()) << " not found in version 8 item list." << std::endl;
		new_savemon.setCaughtBall(0); // NO_ITEM
	} else {
		if (savemon.getCaughtBall() != caught_ball_v8) {
			js_info << "Savemon ball " << std::hex << static_cast<int>(savemon.getCaughtBall()) << " converted to " << std::hex << static_cast<int>(caught_ball_v8) << std::endl;
		}
		new_savemon.setCaughtBall(caught_ball_v8);
	}
	new_savemon.caughtlevel = savemon.caughtlevel;
	uint8_t caught_location_v8 = mapV7LandmarkToV8(savemon.caughtlocation);
	if (caught_location_v8 == 0xFF) {
		js_error << "Savemon landmark " << std::hex << static_cast<int>(savemon.caughtlocation) << " not found in version 8 landmark list." << std::endl;
		new_savemon.caughtlocation = 0; // SPECIAL_MAP
	} else {
		if (savemon.caughtlocation != caught_location_v8) {
			js_info << "Savemon landmark " << std::hex << static_cast<int>(savemon.caughtlocation) << " converted to " << std::hex << static_cast<int>(caught_location_v8) << std::endl;
		}
		new_savemon.caughtlocation = caught_location_v8;
	}
	new_savemon.level = savemon.level;
	std::memcpy(new_savemon.extra, savemon.extra, sizeof(savemon.extra));
	std::memcpy(new_savemon.nickname, savemon.nickname, sizeof(savemon.nickname));
	std::memcpy(new_savemon.ot, savemon.ot, sizeof(savemon.ot));
	return new_savemon;
}

breedmon_struct_v8 convertBreedmonV7toV8(const breedmon_struct_v8& breedmon, std::vector<uint16_t>& seen_mons, std::vector<uint16_t>& caught_mons) {
	breedmon_struct_v8 new_breedmon;
	uint16_t species_v8 = mapV7PkmnToV8(breedmon.species);
	if (species_v8 == INVALID_SPECIES) {
		js_error << "Breedmon species " << std::hex << static_cast<int>(breedmon.species) << " not found in version 8 mon list." << std::endl;
		return breedmon;
	}
	if (breedmon.species != species_v8) {
		js_info << "Breedmon species " << std::hex << static_cast<int>(breedmon.species) << " converted to " << std::hex << static_cast<int>(species_v8) << std::endl;
	}
	new_breedmon.setExtSpecies(species_v8);
	uint8_t item = mapV7ItemToV8(breedmon.item);
	if (item == 0xFF) {
		js_error << "Breedmon item " << std::hex << static_cast<int>(breedmon.item) << " not found in version 8 item list." << std::endl;
		new_breedmon.item = 0; // NO_ITEM
	}
	else {
		if (breedmon.item != item) {
			js_info << "Breedmon item " << std::hex << static_cast<int>(breedmon.item) << " converted to " << std::hex << static_cast<int>(item) << std::endl;
		}
		new_breedmon.item = item;
	}
	std::memcpy(new_breedmon.moves, breedmon.moves, sizeof(breedmon.moves));
	new_breedmon.id = breedmon.id;
	std::memcpy(new_breedmon.exp, breedmon.exp, sizeof(breedmon.exp));
	std::memcpy(new_breedmon.evs, breedmon.evs, sizeof(breedmon.evs));
	std::memcpy(new_breedmon.dvs, breedmon.dvs, sizeof(breedmon.dvs));
	new_breedmon.setShiny(breedmon.isShiny());
	new_breedmon.setAbility(breedmon.getAbility());
	new_breedmon.setNature(breedmon.getNature());
	new_breedmon.setGender(breedmon.getGender());
	new_breedmon.setEgg(breedmon.isEgg());
	if (breedmon.getForm() == 0x00) {
		new_breedmon.setForm(0x01);
	} else {
		new_breedmon.setForm(breedmon.getForm());
	}
	uint16_t extspecies_v8;
	if (species_v8 == PIKACHU_V8) {
		// for NUM_MOVES, scan for SURF_V7 and FLY_V7
		for (int j = 0; j < NUM_MOVES; j++) {
			uint8_t move = breedmon.moves[j];
			if (move == SURF_V7) {
				new_breedmon.setForm(PIKACHU_SURF_FORM_V7);
				extspecies_v8 = mapV7SpeciesFormToV8Extspecies(breedmon.species, PIKACHU_SURF_FORM_V7);
				break;
			}
			else if (move == FLY_V7) {
				new_breedmon.setForm(PIKACHU_FLY_FORM_V7);
				extspecies_v8 = mapV7SpeciesFormToV8Extspecies(breedmon.species, PIKACHU_FLY_FORM_V7);
				break;
			}
		}
	}
	else {
		extspecies_v8 = mapV7SpeciesFormToV8Extspecies(breedmon.species, breedmon.getForm());
	}
	if (extspecies_v8 != INVALID_SPECIES) {
		seen_mons.push_back(extspecies_v8);
		caught_mons.push_back(extspecies_v8);
	}
	if (species_v8 == MAGIKARP_V8) {
		new_breedmon.setForm(mapV7MagikarpFormToV8(breedmon.getForm()));
	}
	if (species_v8 == GYARADOS_V8) {
		if (breedmon.getForm() == GYARADOS_RED_FORM_V7) {
			new_breedmon.setForm(GYARADOS_RED_FORM_V8);
		}
	}
	memcpy(new_breedmon.pp, breedmon.pp, sizeof(breedmon.pp));
	new_breedmon.happiness = breedmon.happiness;
	new_breedmon.pkrus = breedmon.pkrus;
	new_breedmon.setCaughtTime(breedmon.getCaughtTime());
	uint8_t caught_ball_v8 = mapV7ItemToV8(breedmon.getCaughtBall());
	if (caught_ball_v8 == 0xFF) {
		js_error << "Breedmon ball " << std::hex << static_cast<int>(breedmon.getCaughtBall()) << " not found in version 8 item list." << std::endl;
		new_breedmon.setCaughtBall(0); // NO_ITEM
	}
	else {
		if (breedmon.getCaughtBall() != caught_ball_v8) {
			js_info << "Breedmon ball " << std::hex << static_cast<int>(breedmon.getCaughtBall()) << " converted to " << std::hex << static_cast<int>(caught_ball_v8) << std::endl;
		}
		new_breedmon.setCaughtBall(caught_ball_v8);
	}
	new_breedmon.caughtlevel = breedmon.caughtlevel;
	uint8_t caught_location_v8 = mapV7LandmarkToV8(breedmon.caughtlocation);
	if (caught_location_v8 == 0xFF) {
		js_error << "Breedmon landmark " << std::hex << static_cast<int>(breedmon.caughtlocation) << " not found in version 8 landmark list." << std::endl;
		new_breedmon.caughtlocation = 0; // SPECIAL_MAP
	}
	else {
		if (breedmon.caughtlocation != caught_location_v8) {
			js_info << "Breedmon landmark " << std::hex << static_cast<int>(breedmon.caughtlocation) << " converted to " << std::hex << static_cast<int>(caught_location_v8) << std::endl;
		}
		new_breedmon.caughtlocation = caught_location_v8;
	}
	new_breedmon.level = breedmon.level;
	return new_breedmon;
}

party_struct_v8 convertPartyV7toV8(const party_struct_v8& party, std::vector<uint16_t>& seen_mons, std::vector<uint16_t>& caught_mons) {
	party_struct_v8 new_party;
	new_party.breedmon = convertBreedmonV7toV8(party.breedmon, seen_mons, caught_mons);
	new_party.status = party.status;
	new_party.unused = party.unused;
	new_party.hp = party.hp;
	new_party.maxhp = party.maxhp;
	memcpy(new_party.stats, party.stats, sizeof(party.stats));
	return new_party;
}

hofmon_struct_v8 convertHofmonV7toV8(const hofmon_struct_v8& hofmon, std::vector<uint16_t>& seen_mons, std::vector<uint16_t>& caught_mons) {
	hofmon_struct_v8 new_hofmon;
	uint16_t species_v8 = mapV7PkmnToV8(hofmon.species);
	if (species_v8 == INVALID_SPECIES) {
		js_error << "Hofmon species " << std::hex << static_cast<int>(hofmon.species) << " not found in version 8 mon list." << std::endl;
		return hofmon;
	}
	if (hofmon.species != species_v8) {
		js_info << "Hofmon species " << std::hex << static_cast<int>(hofmon.species) << " converted to " << std::hex << static_cast<int>(species_v8) << std::endl;
	}
	new_hofmon.setExtSpecies(species_v8);
	new_hofmon.id = hofmon.id;
	new_hofmon.setShiny(hofmon.isShiny());
	new_hofmon.setAbility(hofmon.getAbility());
	new_hofmon.setNature(hofmon.getNature());
	new_hofmon.setGender(hofmon.getGender());
	new_hofmon.setEgg(hofmon.isEgg());
	new_hofmon.setForm(hofmon.getForm());
	// Can't do pikachu surf/fly here because we don't have the moves
	uint16_t extspecies_v8 = mapV7SpeciesFormToV8Extspecies(hofmon.species, hofmon.getForm());
	if (extspecies_v8 != INVALID_SPECIES) {
		seen_mons.push_back(extspecies_v8);
		caught_mons.push_back(extspecies_v8);
	}
	if (species_v8 == MAGIKARP_V8) {
		new_hofmon.setForm(mapV7MagikarpFormToV8(hofmon.getForm()));
	}
	if (species_v8 == GYARADOS_V8) {
		if (hofmon.getForm() == GYARADOS_RED_FORM_V7) {
			new_hofmon.setForm(GYARADOS_RED_FORM_V8);
		}
	}


	new_hofmon.level = hofmon.level;
	memcpy(new_hofmon.nickname, hofmon.nickname, sizeof(hofmon.nickname));
	return new_hofmon;
}

roam_struct_v8 convertRoamV7toV8(const roam_struct_v8& roam) {
	roam_struct_v8 new_roam;
	uint16_t species_v8 = mapV7PkmnToV8(roam.species);
	if (species_v8 == INVALID_SPECIES) {
		js_error << "Roam species " << std::hex << static_cast<int>(roam.species) << " not found in version 8 mon list." << std::endl;
		return roam;
	}
	if (roam.species != species_v8) {
		js_info << "Roam species " << std::hex << static_cast<int>(roam.species) << " converted to " << std::hex << static_cast<int>(species_v8) << std::endl;
	}
	new_roam.setExtSpecies(species_v8);
	new_roam.level = roam.level;
	std::tuple <uint8_t, uint8_t> map_v8 = mapv7toV8(roam.map_group, roam.map_number);
	if (std::get<0>(map_v8) == 0 && std::get<1>(map_v8) == 0) {
		new_roam.setMap(std::tuple <uint8_t, uint8_t>(-1, -1));
		// print that the map was not found so we set map group/number to -1
		js_info << "Roam map group " << std::hex << static_cast<int>(roam.map_group) << " number " << std::hex << static_cast<int>(roam.map_number) << " not found in version 8 map list. Setting to -1." << std::endl;
	} else {
		new_roam.setMap(map_v8);
	}
	new_roam.hp = roam.hp;
	memcpy(new_roam.dvs, roam.dvs, sizeof(roam.dvs));
	new_roam.setShiny(roam.isShiny());
	new_roam.setAbility(roam.getAbility());
	new_roam.setNature(roam.getNature());
	new_roam.setGender(roam.getGender());
	new_roam.setEgg(roam.isEgg());
	new_roam.setForm(roam.getForm());
	if (roam.getForm() == 0x00) {
		new_roam.setForm(0x01);
	} else {
		new_roam.setForm(roam.getForm());
	}

	return new_roam;
}

mailmsg_struct_v8 convertMailmsgV7toV8(const mailmsg_struct_v8& mailmsg) {
	mailmsg_struct_v8 new_mailmsg;
	for (int i = 0; i < sizeof(mailmsg.message); i++) {
		new_mailmsg.message[i] = mapV7CharToV8(mailmsg.message[i]);
	}
	new_mailmsg.message_end = mailmsg.message_end;
	memcpy(new_mailmsg.author, mailmsg.author, sizeof(mailmsg.author));
	new_mailmsg.nationality = mailmsg.nationality;
	new_mailmsg.author_id = mailmsg.author_id;
	new_mailmsg.species = mailmsg.species;
	new_mailmsg.type = mailmsg.type;

	return new_mailmsg;
}

}