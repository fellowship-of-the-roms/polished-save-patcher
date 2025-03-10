#include "patching/PatchVersion8to9.h"
#include "core/CommonPatchFunctions.h"
#include "core/SymbolDatabase.h"
#include "core/Logging.h"

namespace patchVersion8to9Namespace {

	bool patchVersion8to9(SaveBinary& save8, SaveBinary& save9) {
		// copy the old save file to the new save file
		save9 = save8;

		// create the iterators
		SaveBinary::Iterator it8(save8, 0);
		SaveBinary::Iterator it9(save9, 0);

		// Load the version 8 and version 9 sym files
		SymbolDatabase sym8(VERSION_8_SYMBOL_FILE);
		SymbolDatabase sym9(VERSION_9_SYMBOL_FILE);

		SourceDest sd = { it8, it9, sym8, sym9 };

		// get the checksum word from the version 8 save file
		uint16_t save_checksum = save8.getWord(SAVE_CHECKSUM_ABS_ADDRESS);

		// verify the checksum of the version 8 file matches the calculated checksum
		uint16_t calculated_checksum = calculateSaveChecksum(save8, sym8.getSRAMAddress("sGameData"), sym8.getSRAMAddress("sGameDataEnd"));
		if (save_checksum != calculated_checksum) {
			js_error << "Checksum mismatch! Expected: " << std::hex << calculated_checksum << ", got: " << save_checksum << std::endl;
			return false;
		}

		// check the backup checksum word from the version 8 save file
		uint16_t backup_checksum = save8.getWord(SAVE_BACKUP_CHECKSUM_ABS_ADDRESS);
		uint16_t calculated_backup_checksum = calculateSaveChecksum(save8, sym8.getSRAMAddress("sBackupGameData"), sym8.getSRAMAddress("sBackupGameDataEnd"));
		if (backup_checksum != calculated_backup_checksum) {
			js_error << "Backup checksum mismatch! Expected: " << std::hex << calculated_backup_checksum << ", got: " << backup_checksum << std::endl;
			return false;
		}

		// check if the player is in the PKMN Center 2nd Floor
		uint8_t map_group = it8.getByte(sym8.getMapDataAddress("wMapGroup"));
		it8.next();
		uint8_t map_num = it8.getByte();
		if (map_group != MON_CENTER_2F_GROUP || map_num != MON_CENTER_2F_MAP) {
			js_error << "Player is not in the PKMN Center 2nd Floor. Go to where you heal in game, and head upstairs. Then re-save your game and try again." << std::endl;
			return false;
		}

		// clear unused bytes after wRTC, [wRTC + 4, wRTC + 8)
		js_info << "Clearing 4 unused bytes after wRTC" << std::endl;
		clearDataBlock(sd, sym9.getPlayerDataAddress("wRTC") + 4, 4);

		// clear unused bytes after wTimeOfDayPal, [wTimeOfDayPal + 1, wTimeOfDayPal + 5)
		js_info << "Clearing 4 unused bytes after wTimeOfDayPal" << std::endl;
		clearDataBlock(sd, sym8.getPlayerDataAddress("wTimeOfDayPal") + 1, 4);

		// Clear v9 wKeyItems space
		js_info << "Clearing v9 wKeyItems space..." << std::endl;
		clearDataBlock(sd, sym9.getPlayerDataAddress("wKeyItems"), sym9.getPlayerDataAddress("wKeyItemsEnd") - sym9.getPlayerDataAddress("wKeyItems"));

		// Copy v8 wKeyItems to v9 wKeyItems
		js_info << "Copying v8 wKeyItems to v9 wKeyItems..." << std::endl;
		copyDataBlock(sd, sym8.getPlayerDataAddress("wKeyItems"), sym9.getPlayerDataAddress("wKeyItems"), sym8.getPlayerDataAddress("wKeyItemsEnd") - sym8.getPlayerDataAddress("wKeyItems"));

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
				js_error << "Key Item " << std::hex << key_item_v9 << " not found in version 9 key item list." << std::endl;
			}
			else {
				if (key_item_v8 != key_item_v9) {
					it9.setByte(key_item_v9);
					js_info << "Key Item " << std::hex << static_cast<int>(key_item_v8) << " converted to " << std::hex << static_cast<int>(key_item_v9) << std::endl;
				}
			}
			it8.next();
			it9.next();
		}

		// Copy from [wNumItems, wMooMooBerries - 1)
		js_info << "Copying [wNumItems, wMooMooBerries - 1)" << std::endl;
		copyDataBlock(sd, sym8.getPlayerDataAddress("wNumItems"), sym9.getPlayerDataAddress("wNumItems"), sym8.getPlayerDataAddress("wMooMooBerries") - 1 - sym8.getPlayerDataAddress("wNumItems"));

		// Copy from [wMooMooBerries, wEcruteakHouseSceneID]
		js_info << "Copying [wMooMooBerries, wEcruteakHouseSceneID]" << std::endl;
		copyDataBlock(sd, sym8.getPlayerDataAddress("wMooMooBerries"), sym9.getPlayerDataAddress("wMooMooBerries"), sym8.getPlayerDataAddress("wEcruteakHouseSceneID") + 1 - sym8.getPlayerDataAddress("wMooMooBerries"));

		// Clear wRocketHideoutB4FSceneID
		js_info << "Clearing wRocketHideoutB4FSceneID" << std::endl;
		it9.setByte(sym9.getPlayerDataAddress("wRocketHideoutB4FSceneID"), 0x00);

		// Copy from [wElmsLabSceneID, wEventFlags)
		js_info << "Copying [wElmsLabSceneID, wEventFlags)" << std::endl;
		copyDataBlock(sd, sym8.getPlayerDataAddress("wElmsLabSceneID"), sym9.getPlayerDataAddress("wElmsLabSceneID"), sym8.getPlayerDataAddress("wEventFlags") - sym8.getPlayerDataAddress("wElmsLabSceneID"));

		// Clear wEventFlags
		js_info << "Clearing wEventFlags" << std::endl;
		clearDataBlock(sd, sym9.getPlayerDataAddress("wEventFlags"), sym9.getPlayerDataAddress("wCurBox") - sym9.getPlayerDataAddress("wEventFlags"));

		it9.seek(sym9.getPlayerDataAddress("wEventFlags"));
		// wEventFlags is a flag_array of NUM_EVENTS bits. If v7 bit is set, lookup the bit index in the map and set the corresponding bit in v8
		js_info << "Patching wEventFlags..." << std::endl;
		for (int i = 0; i < NUM_EVENTS; i++) {
			// check if the bit is set
			if (isFlagBitSet(it8, sym8.getPlayerDataAddress("wEventFlags"), i)) {
				// get the event flag index is equal to the bit index
				uint16_t eventFlagIndex = i;
				// map the version 7 event flag to the version 8 event flag
				uint16_t eventFlagIndexV9 = mapV8EventFlagToV9(eventFlagIndex);
				// if the event flag is found set the corresponding bit in it8
				if (eventFlagIndexV9 != INVALID_EVENT_FLAG) {
					// print found event flagv7 and converted event flagv8
					if (eventFlagIndex != eventFlagIndexV9) {
						js_info << "Event Flag " << std::dec << eventFlagIndex << " converted to " << eventFlagIndexV9 << std::endl;
					}
					setFlagBit(it9, sym9.getPlayerDataAddress("wEventFlags"), eventFlagIndexV9);
				}
				else {
					// warn we couldn't find v7 event flag in v8
					js_warning << "Event Flag " << eventFlagIndex << " not found in version 8 event flag list." << std::endl;
				}
			}
		}

		// Copy from [wCurBox, wEmotePal]
		js_info << "Copying [wCurBox, wEmotePal]" << std::endl;
		copyDataBlock(sd, sym8.getPlayerDataAddress("wCurBox"), sym9.getPlayerDataAddress("wCurBox"), sym8.getPlayerDataAddress("wEmotePal") + 1 - sym8.getPlayerDataAddress("wCurBox"));

		// Clear (64 unused bytes + 5 candy bytes) after wEmotePal
		js_info << "Clearing (64 unused bytes + 5 candy bytes) after wEmotePal" << std::endl;
		clearDataBlock(sd, sym9.getPlayerDataAddress("wEmotePal") + 1, 69);

		// Copy from [wWingAmounts, wHiddenGrottoContents)
		js_info << "Copying [wWingAmounts, wHiddenGrottoContents)" << std::endl;
		copyDataBlock(sd, sym8.getPlayerDataAddress("wWingAmounts"), sym9.getPlayerDataAddress("wWingAmounts"), sym8.getPlayerDataAddress("wHiddenGrottoContents") - sym8.getPlayerDataAddress("wWingAmounts"));

		// Clear 19 unused bytes before wHiddenGrottoContents
		js_info << "Clearing 19 unused bytes before wHiddenGrottoContents" << std::endl;
		clearDataBlock(sd, sym9.getPlayerDataAddress("wHiddenGrottoContents") - 19, 19);

		// copy [wHiddenGrottoContents, wPhoneListEnd)
		js_info << "Copying [wHiddenGrottoContents, wPhoneListEnd)" << std::endl;
		copyDataBlock(sd, sym8.getPlayerDataAddress("wHiddenGrottoContents"), sym9.getPlayerDataAddress("wHiddenGrottoContents"), sym8.getPlayerDataAddress("wPhoneListEnd") - sym8.getPlayerDataAddress("wHiddenGrottoContents"));

		// set wPhoneListEnd to 0
		js_info << "Set wPhoneListEnd to 0" << std::endl;
		it9.setByte(sym9.getPlayerDataAddress("wPhoneListEnd"), 0);

		// Copy from [wParkBallsRemaining, wPlayerDataEnd)
		js_info << "Copying [wParkBallsRemaining, wPlayerDataEnd)" << std::endl;
		copyDataBlock(sd, sym8.getPlayerDataAddress("wParkBallsRemaining"), sym9.getPlayerDataAddress("wParkBallsRemaining"), sym8.getPlayerDataAddress("wPlayerDataEnd") - sym8.getPlayerDataAddress("wParkBallsRemaining"));

		// Copy [wCurMapData, wCurMapDataEnd)
		js_info << "Copying [wCurMapData, wCurMapDataEnd)" << std::endl;
		copyDataBlock(sd, sym8.getMapDataAddress("wCurMapData"), sym9.getMapDataAddress("wCurMapData"), sym8.getMapDataAddress("wCurMapDataEnd") - sym8.getMapDataAddress("wCurMapData"));

		// Copy [wPokemonData, wPartyCount]
		js_info << "Copying [wPokemonData, wPartyCount]" << std::endl;
		copyDataBlock(sd, sym8.getPokemonDataAddress("wPokemonData"), sym9.getPokemonDataAddress("wPokemonData"), sym8.getPokemonDataAddress("wPartyCount") + 1 - sym8.getPokemonDataAddress("wPokemonData"));

		// Clear 7 unused bytes after wPartyCount
		js_info << "Clearing 7 unused bytes after wPartyCount" << std::endl;
		clearDataBlock(sd, sym9.getPokemonDataAddress("wPartyCount") + 1, 7);

		// Clear [wPokedexCaught, wUnlockedUnowns)
		js_info << "Clearing [wPokedexCaught, wUnlockedUnowns)" << std::endl;
		clearDataBlock(sd, sym9.getPokemonDataAddress("wPokedexCaught"), sym9.getPokemonDataAddress("wUnlockedUnowns") - sym9.getPokemonDataAddress("wPokedexCaught"));

		// Copy [wPartyMons, wEndPokedexCaught)
		js_info << "Copying [wPartyMons, wEndPokedexCaught)" << std::endl;
		copyDataBlock(sd, sym8.getPokemonDataAddress("wPartyMons"), sym9.getPokemonDataAddress("wPartyMons"), sym8.getPokemonDataAddress("wEndPokedexCaught") - sym8.getPokemonDataAddress("wPartyMons"));

		// Copy [wPokedexSeen, wEndPokedexSeen)
		js_info << "Copying [wPokedexSeen, wEndPokedexSeen)" << std::endl;
		copyDataBlock(sd, sym8.getPokemonDataAddress("wPokedexSeen"), sym9.getPokemonDataAddress("wPokedexSeen"), sym8.getPokemonDataAddress("wEndPokedexSeen") - sym8.getPokemonDataAddress("wPokedexSeen"));

		// Copy wUnlockedUnowns
		js_info << "Copying wUnlockedUnowns" << std::endl;
		it9.setByte(sym9.getPokemonDataAddress("wUnlockedUnowns"), it8.getByte(sym8.getPokemonDataAddress("wUnlockedUnowns")));

		// Clear 2 unused bytes after wUnlockedUnowns
		js_info << "Clearing 2 unused bytes after wUnlockedUnowns" << std::endl;
		clearDataBlock(sd, sym9.getPokemonDataAddress("wUnlockedUnowns") + 1, 2);

		// Copy [wDayCareMan, wBestMagikarpLengthMm)
		js_info << "Copying [wDayCareMan, wBestMagikarpLengthMm)" << std::endl;
		copyDataBlock(sd, sym8.getPokemonDataAddress("wDayCareMan"), sym9.getPokemonDataAddress("wDayCareMan"), sym8.getPokemonDataAddress("wBestMagikarpLengthMm") - sym8.getPokemonDataAddress("wDayCareMan"));

		// Clear 4 unused bytes before wBestMagikarpLengthMm
		js_info << "Clearing 4 unused bytes before wBestMagikarpLengthMm" << std::endl;
		clearDataBlock(sd, sym9.getPokemonDataAddress("wBestMagikarpLengthMm") - 4, 4);

		// Copy [wBestMagikarpLengthMm, wPokemonDataEnd)
		js_info << "Copying [wBestMagikarpLengthMm, wPokemonDataEnd)" << std::endl;
		copyDataBlock(sd, sym8.getPokemonDataAddress("wBestMagikarpLengthMm"), sym9.getPokemonDataAddress("wBestMagikarpLengthMm"), sym8.getPokemonDataAddress("wPokemonDataEnd") - sym8.getPokemonDataAddress("wBestMagikarpLengthMm"));

		// Clear old wNuzlockeLandmarkFlags space
		js_info << "Clearing old wNuzlockeLandmarkFlags space..." << std::endl;
		it8.seek(sym8.getPlayerDataAddress("wHiddenGrottoContents") - 19);
		// Clear wNuzlockeLandmarkFlags
		js_info << "Clear wNuzlockeLandmarkFlags..." << std::endl;
		clearDataBlock(sd, it8.getAddress(), 19);

		// reset the PGO battle event flags
		js_info << "Resetting PGO battle event flags..." << std::endl;
		js_info << "Clearing flag " << std::hex << EVENT_BEAT_CANDELA << std::endl;
		clearFlagBit(it9, sym9.getPlayerDataAddress("wEventFlags"), EVENT_BEAT_CANDELA);
		js_info << "Clearing flag " << std::hex << EVENT_BEAT_BLANCHE << std::endl;
		clearFlagBit(it9, sym9.getPlayerDataAddress("wEventFlags"), EVENT_BEAT_BLANCHE);
		js_info << "Clearing flag " << std::hex << EVENT_BEAT_SPARK << std::endl;
		clearFlagBit(it9, sym9.getPlayerDataAddress("wEventFlags"), EVENT_BEAT_SPARK);

		// copy sGameData to sBackupGameData
	//	js_info <<  "Copying sGameData to sBackupGameData" << std::endl;
	//	for (int i = sym9.getSRAMAddress("sGameData"); i < sym9.getSRAMAddress("sGameDataEnd"); i++) {
	//		it9.setByte(sym9.getSRAMAddress("sBackupGameData") + i, it9.getByte(sym9.getSRAMAddress("sGameData") + i));
	//	}

		// set v9 wCurMapSceneScriptCount and wCurMapCallbackCount to 0
		// set v9 wCurMapSceneScriptPointer word to 0
		// this is done to prevent the game from running any map scripts on load
		js_info << "Set wCurMapSceneScriptCount and wCurMapCallbackCount to 0..." << std::endl;
		it9.seek(sym9.getPlayerDataAddress("wCurMapSceneScriptCount"));
		it9.setByte(0);
		it9.seek(sym9.getPlayerDataAddress("wCurMapCallbackCount"));
		it9.setByte(0);
		js_info << "Set wCurMapSceneScriptPointer to 0..." << std::endl;
		it9.seek(sym9.getPlayerDataAddress("wCurMapSceneScriptPointer"));
		it9.setWord(0);

		uint8_t prev_map_group = it9.getByte(sym9.getMapDataAddress("wBackupMapGroup"));
		uint8_t prev_map_num = it9.getByte(sym9.getMapDataAddress("wBackupMapNumber"));
		// check if the previous map is a valid PC warp ID in the validPCWarpIDs array
		bool valid_prev_map = false;
		for (auto& validPCWarpID : validPCWarpIDs) {
			if (prev_map_group == validPCWarpID.first && prev_map_num == validPCWarpID.second) {
				valid_prev_map = true;
				break;
			}
		}
		if (!valid_prev_map) {
			js_warning << "Player's previous map is not a valid PKMN Center Warp ID! We will reset it to one." << std::endl;
			if (isFlagBitSet(it9, sym9.getPlayerDataAddress("wJohtoBadges"), PLAINBADGE)) {
				js_warning << "Player has the PLAINBADGE, the stairs will now take you to Goldenrod PKMN Center." << std::endl;
				it9.setByte(sym9.getMapDataAddress("wBackupWarpNumber"), 4);
				it9.setByte(sym9.getMapDataAddress("wBackupMapGroup"), GOLDENROD_POKECOM_CENTER_1F.first);
				it9.setByte(sym9.getMapDataAddress("wBackupMapNumber"), GOLDENROD_POKECOM_CENTER_1F.second);
			}
			else {
				js_warning << "Player does not have the PLAINBADGE, the stairs will now warp you to your house." << std::endl;
				it9.setByte(sym9.getMapDataAddress("wBackupWarpNumber"), 3);
				it9.setByte(sym9.getMapDataAddress("wBackupMapGroup"), PLAYERS_HOUSE_1F.first);
				it9.setByte(sym9.getMapDataAddress("wBackupMapNumber"), PLAYERS_HOUSE_1F.second);
			}
		} else {
			js_info << "Player's previous map is a valid PKMN Center warp ID. No need to fix the warp ID." << std::endl;
		}

		// write the new save version number big endian
		js_info << "Writing the new save version number" << std::endl;
		uint16_t new_save_version = 0x09;
		save9.setWordBE(SAVE_VERSION_ABS_ADDRESS, new_save_version);

		// write the new checksums to the version 9 save file
		js_info << "Writing the new checksums" << std::endl;
		uint16_t new_checksum = calculateSaveChecksum(save9, sym9.getSRAMAddress("sGameData"), sym9.getSRAMAddress("sGameDataEnd"));
		save9.setWord(SAVE_CHECKSUM_ABS_ADDRESS, new_checksum);

		// write new backup checksums to the version 9 save file
		uint16_t new_backup_checksum = calculateSaveChecksum(save9, sym9.getSRAMAddress("sBackupGameData"), sym9.getSRAMAddress("sBackupGameDataEnd"));
		save9.setWord(SAVE_BACKUP_CHECKSUM_ABS_ADDRESS, new_backup_checksum);

		js_info << "Sucessfully patched to 3.1.0 save version 9!" << std::endl;
		return true;

	}

	// Converts a version 7 event flag to a version 8 event flag
	uint16_t mapV8EventFlagToV9(uint16_t v8) {
		std::unordered_map<uint16_t, uint16_t> indexMap = {
				{0, 0},  // EVENT_TEMPORARY_UNTIL_MAP_RELOAD_1
				{1, 1},  // EVENT_TEMPORARY_UNTIL_MAP_RELOAD_2
				{2, 2},  // EVENT_TEMPORARY_UNTIL_MAP_RELOAD_3
				{3, 3},  // EVENT_TEMPORARY_UNTIL_MAP_RELOAD_4
				{4, 4},  // EVENT_TEMPORARY_UNTIL_MAP_RELOAD_5
				{5, 5},  // EVENT_TEMPORARY_UNTIL_MAP_RELOAD_6
				{6, 6},  // EVENT_TEMPORARY_UNTIL_MAP_RELOAD_7
				{7, 7},  // EVENT_TEMPORARY_UNTIL_MAP_RELOAD_8
				{8, 8},  // EVENT_GOT_TM31_ROOST
				{9, 9},  // EVENT_GOT_TM69_U_TURN
				{10, 10},  // EVENT_GOT_TM45_ATTRACT
				{11, 11},  // EVENT_GOT_TM30_SHADOW_BALL
				{12, 12},  // EVENT_GOT_TM01_DYNAMICPUNCH
				{13, 13},  // EVENT_GOT_TM23_IRON_TAIL
				{14, 14},  // EVENT_GOT_TM67_AVALANCHE
				{15, 15},  // EVENT_GOT_TM59_DRAGON_PULSE
				{16, 16},  // EVENT_GOT_HM01_CUT
				{17, 17},  // EVENT_GOT_HM02_FLY
				{18, 18},  // EVENT_GOT_HM03_SURF
				{19, 19},  // EVENT_GOT_HM04_STRENGTH
				{20, 20},  // EVENT_GOT_HM05_WHIRLPOOL
				{21, 21},  // EVENT_GOT_HM06_WATERFALL
				{22, 22},  // EVENT_GOT_TM70_FLASH
				{23, 23},  // EVENT_GOT_TM50_ROCK_SMASH
				{24, 24},  // EVENT_GOT_OLD_ROD
				{25, 25},  // EVENT_GOT_GOOD_ROD
				{26, 26},  // EVENT_GOT_SUPER_ROD
				{27, 27},  // EVENT_GOT_A_POKEMON_FROM_ELM
				{28, 28},  // EVENT_GOT_CYNDAQUIL_FROM_ELM
				{29, 29},  // EVENT_GOT_TOTODILE_FROM_ELM
				{30, 30},  // EVENT_GOT_CHIKORITA_FROM_ELM
				{31, 31},  // EVENT_GOT_MYSTERY_EGG_FROM_MR_POKEMON
				{32, 32},  // EVENT_GOT_POKEDEX_FROM_OAK
				{33, 33},  // EVENT_GAVE_MYSTERY_EGG_TO_ELM
				{34, 34},  // EVENT_INTRODUCED_TEALA
				{35, 35},  // EVENT_INTRODUCED_FELICITY
				{36, 36},  // EVENT_SPOKE_TO_LASS_CONNIE
				{37, 37},  // EVENT_SPOKE_TO_GENTLEMAN_PRESTON
				{38, 38},  // EVENT_JASMINE_RETURNED_TO_GYM
				{39, 39},  // EVENT_CLEARED_RADIO_TOWER
				{40, 40},  // EVENT_CLEARED_ROCKET_HIDEOUT
				{41, 41},  // EVENT_GOT_SECRETPOTION_FROM_PHARMACY
				{42, 42},  // EVENT_GOT_SS_TICKET_FROM_ELM
				{43, 43},  // EVENT_USED_THE_CARD_KEY_IN_THE_RADIO_TOWER
				{44, 44},  // EVENT_REFUSED_TO_HELP_LANCE_AT_LAKE_OF_RAGE
				{45, 45},  // EVENT_GOT_MULCH_FROM_ROUTE_30_HOUSE
				{46, 46},  // EVENT_MADE_WHITNEY_CRY
				{47, 47},  // EVENT_HERDED_FARFETCHD
				{48, 48},  // EVENT_FOUGHT_SUDOWOODO
				{49, 49},  // EVENT_CLEARED_SLOWPOKE_WELL
				{50, 50},  // EVENT_REFUSED_TO_TAKE_EGG_FROM_ELMS_AIDE
				{51, 51},  // EVENT_GOT_TOGEPI_EGG_FROM_ELMS_AIDE
				{52, 52},  // EVENT_MADE_UNOWN_APPEAR_IN_RUINS
				{53, 53},  // EVENT_FAST_SHIP_DESTINATION_OLIVINE
				{54, 54},  // EVENT_FAST_SHIP_FIRST_TIME
				{55, 55},  // EVENT_FAST_SHIP_HAS_ARRIVED
				{56, 56},  // EVENT_FAST_SHIP_FOUND_GIRL
				{57, 57},  // EVENT_FAST_SHIP_LAZY_SAILOR
				{58, 58},  // EVENT_FAST_SHIP_INFORMED_ABOUT_LAZY_SAILOR
				{59, 59},  // EVENT_GOT_AMULET_COIN_FROM_LYRA
				{60, 60},  // EVENT_KURT_GAVE_YOU_APRICORN_BOX
				{61, 61},  // EVENT_INITIALIZED_EVENTS
				{62, 62},  // EVENT_JASMINE_EXPLAINED_AMPHYS_SICKNESS
				{63, 63},  // EVENT_LAKE_OF_RAGE_EXPLAINED_WEIRD_MAGIKARP
				{64, 64},  // EVENT_LAKE_OF_RAGE_ASKED_FOR_MAGIKARP
				{65, 65},  // EVENT_LAKE_OF_RAGE_ELIXIR_ON_STANDBY
				{66, 66},  // EVENT_HEALED_MOOMOO
				{67, 67},  // EVENT_GOT_TM62_ACROBATICS_FROM_MOOMOO_FARM
				{68, 68},  // EVENT_TALKED_TO_FARMER_ABOUT_MOOMOO
				{69, 69},  // EVENT_TALKED_TO_MOM_AFTER_MYSTERY_EGG_QUEST
				{70, 70},  // EVENT_LEARNED_TO_CATCH_POKEMON
				{71, 71},  // EVENT_NEVER_LEARNED_TO_CATCH_POKEMON
				{72, 72},  // EVENT_ELM_CALLED_ABOUT_STOLEN_POKEMON
				{73, 73},  // EVENT_BEAT_ELITE_FOUR
				{74, 74},  // EVENT_GOT_SHUCKIE
				{75, 75},  // EVENT_MANIA_TOOK_SHUCKIE_OR_LET_YOU_KEEP_HIM
				{76, 76},  // EVENT_GOT_GBC_SOUNDS_FROM_RADIO_TOWER
				{77, 77},  // EVENT_GOT_PINK_BOW_FROM_MARY
				{78, 78},  // EVENT_USED_BASEMENT_KEY
				{79, 79},  // EVENT_RECEIVED_CARD_KEY
				{80, 80},  // EVENT_LANCE_HEALED_YOU_IN_TEAM_ROCKET_BASE
				{81, 81},  // EVENT_GOT_MYSTIC_WATER_IN_CHERRYGROVE
				{82, 82},  // EVENT_GOT_TM05_ROAR
				{83, 83},  // EVENT_LISTENED_TO_BILL_INTRO
				{84, 84},  // EVENT_GOT_EEVEE
				{85, 85},  // EVENT_GOT_KENYA
				{86, 86},  // EVENT_GAVE_KENYA
				{87, 87},  // EVENT_GOT_HP_UP_FROM_RANDY
				{88, 88},  // EVENT_TOGEPI_HATCHED
				{89, 89},  // EVENT_SHOWED_TOGEPI_TO_ELM
				{90, 90},  // EVENT_GOT_ODD_SOUVENIR_FROM_ELM
				{91, 91},  // EVENT_GOT_EVIOLITE_IN_GOLDENROD
				{92, 92},  // EVENT_GOT_QUICK_CLAW
				{93, 93},  // EVENT_GOT_TM10_HIDDEN_POWER
				{94, 94},  // EVENT_GOT_TM36_SLUDGE_BOMB
				{95, 95},  // EVENT_GOT_ITEMFINDER
				{96, 96},  // EVENT_GOT_BICYCLE
				{97, 97},  // EVENT_GOT_SQUIRTBOTTLE
				{98, 98},  // EVENT_GOT_CHARCOAL_IN_CHARCOAL_KILN
				{99, 99},  // EVENT_DECIDED_TO_HELP_LANCE
				{100, 100},  // EVENT_GOT_TYROGUE_FROM_KIYO
				{101, 101},  // EVENT_MET_FRIEDA_OF_FRIDAY
				{102, 102},  // EVENT_GOT_POISON_BARB_FROM_FRIEDA
				{103, 103},  // EVENT_MET_TUSCANY_OF_TUESDAY
				{104, 104},  // EVENT_GOT_SILK_SCARF_FROM_TUSCANY
				{105, 105},  // EVENT_MET_ARTHUR_OF_THURSDAY
				{106, 106},  // EVENT_GOT_HARD_STONE_FROM_ARTHUR
				{107, 107},  // EVENT_MET_SUNNY_OF_SUNDAY
				{108, 108},  // EVENT_GOT_MAGNET_FROM_SUNNY
				{109, 109},  // EVENT_MET_WESLEY_OF_WEDNESDAY
				{110, 110},  // EVENT_GOT_BLACK_BELT_FROM_WESLEY
				{111, 111},  // EVENT_MET_SANTOS_OF_SATURDAY
				{112, 112},  // EVENT_GOT_SPELL_TAG_FROM_SANTOS
				{113, 113},  // EVENT_MET_MONICA_OF_MONDAY
				{114, 114},  // EVENT_GOT_SHARP_BEAK_FROM_MONICA
				{115, 115},  // EVENT_GOT_POWER_HERB_FROM_KATE
				{116, 116},  // EVENT_GOT_MACHO_BRACE_FROM_GRANDPA_ON_SS_AQUA
				{117, 117},  // EVENT_GOT_BLACKGLASSES_IN_DARK_CAVE
				{118, 118},  // EVENT_GOT_KINGS_ROCK_IN_SLOWPOKE_WELL
				{119, 119},  // EVENT_GOT_TM47_STEEL_WING
				{120, 120},  // EVENT_FIRST_TIME_BANKING_WITH_MOM
				{121, 121},  // EVENT_TOLD_ELM_ABOUT_TOGEPI_OVER_THE_PHONE
				{122, 122},  // EVENT_GOT_CLEAR_BELL
				{123, 123},  // EVENT_GOT_OLD_AMBER
				{124, 124},  // EVENT_GOT_TM54_FALSE_SWIPE
				{125, 125},  // EVENT_RELEASED_THE_BEASTS
				{126, 126},  // EVENT_GOT_MASTER_BALL_FROM_ELM
				{129, 129},  // EVENT_GOT_AIR_BALLOON_FROM_ROUTE_31_LEADER
				{130, 130},  // EVENT_GOT_MIRACLE_SEED_FROM_ROUTE_32_LEADER
				{131, 131},  // EVENT_INTRODUCED_ROUTE_LEADERS
				{132, 132},  // EVENT_GOT_BIG_NUGGET_FROM_ROUTE_34_LEADER
				{133, 133},  // EVENT_GOT_BINDING_BAND_FROM_ROUTE_36_LEADER
				{134, 134},  // EVENT_GOT_PP_MAX_FROM_ROUTE_39_LEADER
				{135, 135},  // EVENT_GOT_PROTECT_PADS_FROM_LIGHTHOUSE_LEADER
				{136, 136},  // EVENT_GOT_FLAME_ORB_FROM_ROUTE_43_LEADER
				{137, 137},  // EVENT_GOT_ROCKY_HELMET_FROM_ROUTE_44_LEADER
				{138, 138},  // EVENT_GOT_FOCUS_SASH_FROM_ROUTE_45_LEADER
				{139, 139},  // EVENT_GOT_CHOICE_SPECS_FROM_ROUTE_27_LEADER
				{140, 140},  // EVENT_LISTENED_TO_DEFENSE_CURL_INTRO
				{141, 141},  // EVENT_LISTENED_TO_EARTH_POWER_INTRO
				{142, 142},  // EVENT_LISTENED_TO_HEADBUTT_INTRO
				{143, 143},  // EVENT_LISTENED_TO_HYPER_VOICE_INTRO
				{144, 144},  // EVENT_LISTENED_TO_ICY_WIND_INTRO
				{145, 145},  // EVENT_LISTENED_TO_KNOCK_OFF_INTRO
				{146, 146},  // EVENT_LISTENED_TO_PAY_DAY_INTRO
				{147, 147},  // EVENT_LISTENED_TO_ROLLOUT_INTRO
				{148, 148},  // EVENT_LISTENED_TO_SEED_BOMB_INTRO
				{149, 149},  // EVENT_LISTENED_TO_SKILL_SWAP_INTRO
				{150, 150},  // EVENT_LISTENED_TO_TRICK_INTRO
				{151, 151},  // EVENT_TIN_TOWER_4F_HIDDEN_MAX_POTION
				{152, 152},  // EVENT_TIN_TOWER_5F_HIDDEN_FULL_RESTORE
				{153, 153},  // EVENT_TIN_TOWER_5F_HIDDEN_CARBOS
				{154, 154},  // EVENT_BURNED_TOWER_1F_HIDDEN_ETHER
				{155, 155},  // EVENT_BURNED_TOWER_1F_HIDDEN_ULTRA_BALL
				{156, 156},  // EVENT_NATIONAL_PARK_HIDDEN_FULL_HEAL
				{157, 157},  // EVENT_OLIVINE_LIGHTHOUSE_5F_HIDDEN_HYPER_POTION
				{158, 158},  // EVENT_TEAM_ROCKET_BASE_B1F_HIDDEN_REVIVE
				{159, 159},  // EVENT_TEAM_ROCKET_BASE_B2F_HIDDEN_FULL_HEAL
				{160, 160},  // EVENT_UNION_CAVE_1F_HIDDEN_GREAT_BALL
				{161, 161},  // EVENT_UNION_CAVE_1F_HIDDEN_BIG_PEARL
				{162, 162},  // EVENT_UNION_CAVE_1F_HIDDEN_PARALYZEHEAL
				{163, 163},  // EVENT_UNION_CAVE_B1F_NORTH_HIDDEN_X_SPEED
				{164, 164},  // EVENT_UNION_CAVE_B1F_NORTH_HIDDEN_REVIVE
				{165, 165},  // EVENT_UNION_CAVE_B1F_SOUTH_DUSK_STONE
				{166, 166},  // EVENT_UNION_CAVE_B1F_SOUTH_SUPER_REPEL
				{167, 167},  // EVENT_UNION_CAVE_B1F_SOUTH_LIGHT_CLAY
				{168, 168},  // EVENT_UNION_CAVE_B1F_SOUTH_HIDDEN_X_SP_DEF
				{169, 169},  // EVENT_UNION_CAVE_B1F_SOUTH_HIDDEN_NUGGET
				{170, 170},  // EVENT_UNION_CAVE_B1F_SOUTH_HIDDEN_FULL_RESTORE
				{171, 171},  // EVENT_UNION_CAVE_B2F_HIDDEN_CALCIUM
				{172, 172},  // EVENT_UNION_CAVE_B2F_HIDDEN_ULTRA_BALL
				{173, 173},  // EVENT_SLOWPOKE_WELL_ENTRANCE_HIDDEN_SUPER_POTION
				{174, 174},  // EVENT_ILEX_FOREST_HIDDEN_ETHER
				{175, 175},  // EVENT_ILEX_FOREST_HIDDEN_SUPER_POTION
				{176, 176},  // EVENT_ILEX_FOREST_HIDDEN_FULL_HEAL
				{177, 177},  // EVENT_ILEX_FOREST_HIDDEN_SILVER_LEAF_1
				{178, 178},  // EVENT_ILEX_FOREST_HIDDEN_SILVER_LEAF_2
				{179, 179},  // EVENT_GOLDENROD_POKECOM_CENTER_1F_HIDDEN_RARE_CANDY
				{180, 180},  // EVENT_WAREHOUSE_ENTRANCE_HIDDEN_PARALYZEHEAL
				{181, 181},  // EVENT_WAREHOUSE_ENTRANCE_HIDDEN_SUPER_POTION
				{182, 182},  // EVENT_WAREHOUSE_ENTRANCE_HIDDEN_ANTIDOTE
				{183, 183},  // EVENT_UNDERGROUND_PATH_SWITCH_ROOM_ENTRANCES_HIDDEN_MAX_POTION
				{184, 184},  // EVENT_UNDERGROUND_PATH_SWITCH_ROOM_ENTRANCES_HIDDEN_REVIVE
				{185, 185},  // EVENT_MOUNT_MORTAR_1F_OUTSIDE_HIDDEN_HYPER_POTION
				{186, 186},  // EVENT_MOUNT_MORTAR_1F_INSIDE_HIDDEN_MAX_REPEL
				{187, 187},  // EVENT_MOUNT_MORTAR_2F_INSIDE_HIDDEN_FULL_RESTORE
				{188, 188},  // EVENT_MOUNT_MORTAR_B1F_HIDDEN_MAX_REVIVE
				{189, 189},  // EVENT_ICE_PATH_B1F_HIDDEN_MAX_POTION
				{190, 190},  // EVENT_ICE_PATH_B2F_MAHOGANY_SIDE_HIDDEN_CARBOS
				{191, 191},  // EVENT_ICE_PATH_B2F_BLACKTHORN_SIDE_HIDDEN_ICE_HEAL
				{192, 192},  // EVENT_WHIRL_ISLAND_B1F_HIDDEN_RARE_CANDY
				{193, 193},  // EVENT_WHIRL_ISLAND_B1F_HIDDEN_ULTRA_BALL
				{194, 194},  // EVENT_WHIRL_ISLAND_B1F_HIDDEN_FULL_RESTORE
				{195, 195},  // EVENT_SILVER_CAVE_ROOM_1_HIDDEN_DIRE_HIT
				{196, 196},  // EVENT_SILVER_CAVE_ROOM_1_HIDDEN_ULTRA_BALL
				{197, 197},  // EVENT_SILVER_CAVE_ROOM_2_HIDDEN_MAX_POTION
				{198, 198},  // EVENT_DARK_CAVE_VIOLET_ENTRANCE_HIDDEN_ELIXIR
				{199, 199},  // EVENT_RUINS_OF_ALPH_OUTSIDE_HIDDEN_RARE_CANDY
				{200, 200},  // EVENT_VICTORY_ROAD_1F_HIDDEN_FULL_HEAL
				{201, 201},  // EVENT_VICTORY_ROAD_2F_HIDDEN_MAX_POTION
				{202, 202},  // EVENT_DRAGONS_DEN_B1F_HIDDEN_REVIVE
				{203, 203},  // EVENT_DRAGONS_DEN_B1F_HIDDEN_MAX_POTION
				{204, 204},  // EVENT_DRAGONS_DEN_B1F_HIDDEN_MAX_ELIXIR
				{205, 205},  // EVENT_ROUTE_28_HIDDEN_RARE_CANDY
				{206, 206},  // EVENT_ROUTE_30_HIDDEN_POTION
				{207, 207},  // EVENT_ROUTE_32_HIDDEN_GREAT_BALL_1
				{208, 208},  // EVENT_ROUTE_32_HIDDEN_SUPER_POTION_1
				{209, 209},  // EVENT_ROUTE_32_HIDDEN_GREAT_BALL_2
				{210, 210},  // EVENT_ROUTE_32_HIDDEN_SUPER_POTION_2
				{211, 211},  // EVENT_ROUTE_32_HIDDEN_GOLD_LEAF
				{212, 212},  // EVENT_ROUTE_32_COAST_HIDDEN_HYPER_POTION
				{213, 213},  // EVENT_ROUTE_32_COAST_HIDDEN_LEVEL_BALL
				{214, 214},  // EVENT_ROUTE_32_COAST_HIDDEN_ELIXIR
				{215, 215},  // EVENT_ROUTE_34_HIDDEN_RARE_CANDY
				{216, 216},  // EVENT_ROUTE_34_HIDDEN_SUPER_POTION
				{217, 217},  // EVENT_ROUTE_35_HIDDEN_NUGGET
				{218, 218},  // EVENT_ROUTE_35_COAST_SOUTH_HIDDEN_STAR_PIECE
				{219, 219},  // EVENT_ROUTE_37_HIDDEN_ETHER
				{220, 220},  // EVENT_ROUTE_39_HIDDEN_NUGGET
				{221, 221},  // EVENT_ROUTE_40_HIDDEN_HYPER_POTION
				{222, 222},  // EVENT_ROUTE_41_HIDDEN_MAX_ETHER
				{223, 223},  // EVENT_ROUTE_42_HIDDEN_MAX_POTION
				{224, 224},  // EVENT_ROUTE_44_HIDDEN_ELIXIR
				{225, 225},  // EVENT_ROUTE_45_HIDDEN_PP_UP
				{226, 226},  // EVENT_NEW_BARK_TOWN_HIDDEN_POTION
				{227, 227},  // EVENT_CHERRYGROVE_CITY_HIDDEN_NUGGET
				{228, 228},  // EVENT_VIOLET_CITY_HIDDEN_POKE_BALL
				{229, 229},  // EVENT_VIOLET_CITY_HIDDEN_HYPER_POTION
				{230, 230},  // EVENT_AZALEA_TOWN_HIDDEN_FULL_HEAL
				{231, 231},  // EVENT_ECRUTEAK_CITY_HIDDEN_HYPER_POTION
				{232, 232},  // EVENT_ECRUTEAK_CITY_HIDDEN_RARE_CANDY
				{233, 233},  // EVENT_ECRUTEAK_CITY_HIDDEN_ULTRA_BALL
				{234, 234},  // EVENT_ECRUTEAK_CITY_HIDDEN_ETHER
				{235, 235},  // EVENT_OLIVINE_CITY_HIDDEN_RARE_CANDY
				{236, 236},  // EVENT_ROUTE_35_COAST_NORTH_HIDDEN_BIG_PEARL
				{237, 237},  // EVENT_ROUTE_35_COAST_NORTH_HIDDEN_SOFT_SAND
				{238, 238},  // EVENT_CIANWOOD_CITY_HIDDEN_REVIVE
				{239, 239},  // EVENT_CIANWOOD_CITY_HIDDEN_MAX_ETHER
				{240, 240},  // EVENT_LAKE_OF_RAGE_HIDDEN_FULL_RESTORE
				{241, 241},  // EVENT_LAKE_OF_RAGE_HIDDEN_RARE_CANDY
				{242, 242},  // EVENT_LAKE_OF_RAGE_HIDDEN_MAX_POTION
				{243, 243},  // EVENT_SILVER_CAVE_OUTSIDE_HIDDEN_FULL_RESTORE
				{244, 244},  // EVENT_CLIFF_CAVE_HIDDEN_ULTRA_BALL
				{245, 245},  // EVENT_CLIFF_EDGE_GATE_HIDDEN_OVAL_STONE
				{246, 246},  // EVENT_ROUTE_47_HIDDEN_PEARL
				{247, 247},  // EVENT_ROUTE_47_HIDDEN_STARDUST
				{248, 248},  // EVENT_GOLDENROD_HARBOR_HIDDEN_REVIVE
				{249, 249},  // EVENT_STORMY_BEACH_HIDDEN_STARDUST
				{250, 250},  // EVENT_MURKY_SWAMP_HIDDEN_MULCH
				{251, 251},  // EVENT_MURKY_SWAMP_HIDDEN_X_SP_DEF
				{252, 252},  // EVENT_MURKY_SWAMP_HIDDEN_BIG_MUSHROOM
				{253, 253},  // EVENT_MURKY_SWAMP_HIDDEN_TINYMUSHROOM
				{254, 254},  // EVENT_YELLOW_FOREST_HIDDEN_BIG_MUSHROOM
				{255, 255},  // EVENT_YELLOW_FOREST_HIDDEN_BALM_MUSHROOM
				{256, 256},  // EVENT_YELLOW_FOREST_HIDDEN_GOLD_LEAF_1
				{257, 257},  // EVENT_YELLOW_FOREST_HIDDEN_GOLD_LEAF_2
				{258, 258},  // EVENT_QUIET_CAVE_B1F_HIDDEN_HYPER_POTION
				{259, 259},  // EVENT_QUIET_CAVE_B2F_HIDDEN_CALCIUM
				{260, 260},  // EVENT_QUIET_CAVE_B3F_HIDDEN_PP_UP
				{261, 261},  // EVENT_QUIET_CAVE_B3F_HIDDEN_MAX_REVIVE
				{262, 262},  // EVENT_GIOVANNIS_CAVE_HIDDEN_BERSERK_GENE
				{263, 263},  // EVENT_SAW_FIRST_HIDDEN_GROTTO
				{264, 264},  // EVENT_CRYS_IN_NAVEL_ROCK
				{266, 266},  // EVENT_BUGGING_KURT_TOO_MUCH
				{267, 267},  // EVENT_TALKED_TO_RUINS_COWARD
				{268, 268},  // EVENT_GOT_DRATINI
				{269, 269},  // EVENT_CAN_GIVE_GS_BALL_TO_KURT
				{270, 270},  // EVENT_GAVE_GS_BALL_TO_KURT
				{271, 271},  // EVENT_FOREST_IS_RESTLESS
				{272, 272},  // EVENT_ANSWERED_DRAGON_MASTER_QUIZ_WRONG
				{273, 273},  // EVENT_GOT_LINKING_CORD_FROM_PROF_OAKS_AIDE
				{274, 274},  // EVENT_GOT_EXP_SHARE_FROM_PROF_OAKS_AIDE
				{275, 275},  // EVENT_GOT_MACHO_BRACE_FROM_PROF_OAKS_AIDE
				{276, 276},  // EVENT_GOT_LUCKY_EGG_FROM_PROF_OAKS_AIDE
				{277, 277},  // EVENT_GOT_TM46_THIEF_FROM_LANCE
				{278, 278},  // EVENT_VALERIE_ECRUTEAK_CITY
				{279, 279},  // EVENT_VALERIE_BELLCHIME_TRAIL
				{280, 280},  // EVENT_LISTENED_TO_VALERIE
				{281, 281},  // EVENT_GOT_TM49_DAZZLINGLEAM_FROM_VALERIE
				{282, 282},  // EVENT_GOT_LURE_BALL_FROM_FRENCHMAN
				{283, 283},  // EVENT_GOT_HONEY_FROM_GOLDENROD
				{284, 284},  // EVENT_GOT_NET_BALL_FROM_GOLDENROD
				{285, 285},  // EVENT_GOT_CHERISH_BALL_FROM_ECRUTEAK
				{286, 286},  // EVENT_GOT_DESTINY_KNOT_FROM_ECRUTEAK
				{287, 287},  // EVENT_GOT_ASSAULT_VEST_FROM_CIANWOOD
				{288, 288},  // EVENT_GOT_FULL_RESTORE_FROM_LIGHTHOUSE
				{289, 289},  // EVENT_GOT_ICY_ROCK_FROM_LORELEI
				{290, 290},  // EVENT_FINAL_BATTLE_WITH_LYRA
				{291, 291},  // EVENT_GOT_NUGGET_FROM_GUY
				{292, 292},  // EVENT_INTRODUCED_TO_CERULEAN_MAN
				{293, 293},  // EVENT_LEARNED_ABOUT_MACHINE_PART
				{294, 294},  // EVENT_TALKED_TO_MR_HYPER
				{295, 295},  // EVENT_MET_MANAGER_AT_POWER_PLANT
				{296, 296},  // EVENT_MET_ROCKET_GRUNT_AT_CERULEAN_GYM
				{297, 297},  // EVENT_MET_REDS_MOM
				{298, 298},  // EVENT_RESTORED_POWER_TO_KANTO
				{299, 299},  // EVENT_GOT_COINS_FROM_GAMBLER_AT_CELADON
				{300, 300},  // EVENT_GOT_SAFE_GOGGLES_FROM_CELADON
				{301, 301},  // EVENT_MET_COPYCAT_FOUND_OUT_ABOUT_LOST_ITEM
				{302, 302},  // EVENT_RETURNED_LOST_ITEM_TO_COPYCAT
				{303, 303},  // EVENT_GOT_PASS_FROM_COPYCAT
				{304, 304},  // EVENT_GOT_LOST_ITEM_FROM_FAN_CLUB
				{305, 305},  // EVENT_LISTENED_TO_FAN_CLUB_PRESIDENT_BUT_BAG_WAS_FULL
				{306, 306},  // EVENT_LISTENED_TO_FAN_CLUB_PRESIDENT
				{307, 307},  // EVENT_TALKED_TO_SEAFOAM_GYM_GUY_ONCE
				{308, 308},  // EVENT_CINNABAR_ROCKS_CLEARED
				{309, 309},  // EVENT_CLEARED_NUGGET_BRIDGE
				{310, 310},  // EVENT_TALKED_TO_WARDENS_GRANDDAUGHTER
				{311, 311},  // EVENT_GOT_TM03_CURSE
				{312, 312},  // EVENT_GOT_TM65_SHADOW_CLAW_FROM_AGATHA
				{313, 313},  // EVENT_GOT_CLEANSE_TAG
				{314, 314},  // EVENT_GOT_TM48_ROCK_SLIDE
				{315, 315},  // EVENT_GOT_TM63_WATER_PULSE
				{316, 316},  // EVENT_GOT_TM57_WILD_CHARGE
				{317, 317},  // EVENT_GOT_TM19_GIGA_DRAIN
				{318, 318},  // EVENT_GOT_TM66_POISON_JAB
				{319, 319},  // EVENT_GOT_TM29_PSYCHIC
				{320, 320},  // EVENT_GOT_TM61_WILL_O_WISP
				{321, 321},  // EVENT_GOT_TM71_STONE_EDGE
				{322, 322},  // EVENT_GOT_UP_GRADE
				{323, 323},  // EVENT_TALKED_TO_OAK_IN_KANTO
				{324, 324},  // EVENT_GOT_BOTTLE_CAP_FROM_VERMILION_GUY
				{325, 325},  // EVENT_GOT_CHERISH_BALL_FROM_SAFFRON
				{326, 326},  // EVENT_GOT_AIR_BALLOON_FROM_SAFFRON
				{327, 327},  // EVENT_GOT_PROTEIN_FROM_SAFFRON_GATE
				{328, 328},  // EVENT_GOT_HP_UP_FROM_CERULEAN
				{329, 329},  // EVENT_GOT_WEAK_POLICY_FROM_VIRIDIAN
				{330, 330},  // EVENT_GOT_ORANGE_TICKET
				{331, 331},  // EVENT_GOT_MYSTICTICKET_FROM_RED
				{332, 332},  // EVENT_VISITED_NAVEL_ROCK
				{333, 333},  // EVENT_VISITED_FARAWAY_ISLAND
				{334, 334},  // EVENT_VERMILION_GYM_SWITCH_1
				{335, 335},  // EVENT_VERMILION_GYM_SWITCH_2
				{336, 336},  // EVENT_SHOWED_SAFFRON_KID_HITMONTOP
				{337, 337},  // EVENT_LISTENED_TO_WESTWOOD_INTRO
				{338, 338},  // EVENT_INTRODUCED_CELADON_FOUR
				{339, 339},  // EVENT_GOT_CHOICE_BAND_FROM_CELADON_FOUR
				{340, 340},  // EVENT_GOT_PERSIM_BERRY_FROM_IMAKUNI
				{341, 341},  // EVENT_GOT_RARE_CANDY_IN_UNIVERSITY
				{342, 342},  // EVENT_GOT_ANTIDOTE_IN_UNIVERSITY
				{343, 343},  // EVENT_GOT_RAGECANDYBAR_IN_UNIVERSITY
				{344, 344},  // EVENT_GOT_LEMONADE_IN_UNIVERSITY
				{345, 345},  // EVENT_GOT_FOCUS_BAND_IN_UNIVERSITY
				{346, 346},  // EVENT_GOT_ABILITY_CAP_IN_UNIVERSITY
				{347, 347},  // EVENT_GOT_PP_MAX_IN_UNIVERSITY
				{348, 348},  // EVENT_GOT_X_SP_ATK_IN_UNIVERSITY
				{349, 349},  // EVENT_PASSED_CELADON_HYPER_TEST
				{350, 350},  // EVENT_GOT_DRAGON_RAGE_MAGIKARP
				{351, 351},  // EVENT_GOT_LUCKY_EGG_FROM_LUCKY_ISLAND
				{352, 352},  // EVENT_LISTENED_TO_AQUA_TAIL_INTRO
				{353, 353},  // EVENT_LISTENED_TO_COUNTER_INTRO
				{354, 354},  // EVENT_LISTENED_TO_DOUBLE_EDGE_INTRO
				{355, 355},  // EVENT_LISTENED_TO_DREAM_EATER_INTRO
				{356, 356},  // EVENT_LISTENED_TO_IRON_HEAD_INTRO
				{357, 357},  // EVENT_LISTENED_TO_SEISMIC_TOSS_INTRO
				{358, 358},  // EVENT_LISTENED_TO_SUCKER_PUNCH_INTRO
				{359, 359},  // EVENT_LISTENED_TO_SWAGGER_INTRO
				{360, 360},  // EVENT_LISTENED_TO_ZEN_HEADBUTT_INTRO
				{361, 361},  // EVENT_LISTENED_TO_IVY_INTRO
				{362, 362},  // EVENT_TOLD_ABOUT_PIKABLU
				{363, 363},  // EVENT_GOT_ODD_SOUVENIR_FROM_PIKABLU_GUY
				{364, 364},  // EVENT_SAW_HAUNTED_ROOM
				{365, 365},  // EVENT_HEARD_LAWRENCES_FINAL_SPEECH
				{366, 366},  // EVENT_HEALED_NIDORINO
				{367, 367},  // EVENT_GOT_MOON_STONE_FROM_IVY
				{368, 368},  // EVENT_LISTENED_TO_BODY_SLAM_INTRO
				{369, 369},  // EVENT_DIGLETTS_CAVE_HIDDEN_MAX_REVIVE
				{370, 370},  // EVENT_DIGLETTS_CAVE_HIDDEN_MAX_REPEL
				{371, 371},  // EVENT_UNDERGROUND_HIDDEN_FULL_RESTORE
				{372, 372},  // EVENT_UNDERGROUND_HIDDEN_X_SP_ATK
				{373, 373},  // EVENT_CERULEAN_CAPE_HIDDEN_BOTTLE_CAP
				{374, 374},  // EVENT_CERULEAN_CAPE_HIDDEN_PEARL_STRING
				{375, 375},  // EVENT_ROCK_TUNNEL_1F_HIDDEN_X_ACCURACY
				{376, 376},  // EVENT_ROCK_TUNNEL_1F_HIDDEN_X_DEFEND
				{377, 377},  // EVENT_ROCK_TUNNEL_2F_HIDDEN_MAX_ETHER
				{378, 378},  // EVENT_ROCK_TUNNEL_B1F_HIDDEN_MAX_POTION
				{379, 379},  // EVENT_OLIVINE_PORT_HIDDEN_PROTEIN
				{380, 380},  // EVENT_VERMILION_PORT_HIDDEN_IRON
				{381, 381},  // EVENT_MOUNT_MOON_1F_HIDDEN_RARE_CANDY
				{382, 382},  // EVENT_MOUNT_MOON_1F_HIDDEN_FULL_RESTORE
				{383, 383},  // EVENT_MOUNT_MOON_B1F_HIDDEN_STAR_PIECE
				{384, 384},  // EVENT_MOUNT_MOON_B1F_HIDDEN_MOON_STONE
				{385, 385},  // EVENT_MOUNT_MOON_B2F_HIDDEN_ETHER
				{386, 386},  // EVENT_MOUNT_MOON_B2F_HIDDEN_STARDUST
				{387, 387},  // EVENT_MOUNT_MOON_B2F_HIDDEN_PP_UP
				{388, 388},  // EVENT_MOUNT_MOON_SQUARE_HIDDEN_MOON_STONE
				{389, 389},  // EVENT_VIRIDIAN_FOREST_HIDDEN_MAX_ETHER
				{390, 390},  // EVENT_VIRIDIAN_FOREST_HIDDEN_FULL_HEAL
				{391, 391},  // EVENT_VIRIDIAN_FOREST_HIDDEN_MULCH
				{392, 392},  // EVENT_VIRIDIAN_FOREST_HIDDEN_BIG_MUSHROOM
				{393, 393},  // EVENT_VIRIDIAN_FOREST_HIDDEN_LEAF_STONE
				{394, 394},  // EVENT_ROUTE_3_HIDDEN_MOON_STONE
				{395, 395},  // EVENT_ROUTE_4_HIDDEN_ULTRA_BALL
				{396, 396},  // EVENT_ROUTE_9_HIDDEN_ETHER
				{397, 397},  // EVENT_ROUTE_9_HIDDEN_SOFT_SAND
				{398, 398},  // EVENT_ROUTE_10_HIDDEN_MAX_ETHER
				{399, 399},  // EVENT_ROUTE_12_HIDDEN_ELIXIR
				{400, 400},  // EVENT_ROUTE_13_HIDDEN_CALCIUM
				{401, 401},  // EVENT_ROUTE_11_HIDDEN_REVIVE
				{402, 402},  // EVENT_ROUTE_16_WEST_HIDDEN_RARE_CANDY
				{403, 403},  // EVENT_ROUTE_17_HIDDEN_MAX_ETHER
				{404, 404},  // EVENT_ROUTE_17_HIDDEN_MAX_ELIXIR
				{405, 405},  // EVENT_ROUTE_19_HIDDEN_REVIVE
				{406, 406},  // EVENT_ROUTE_19_HIDDEN_MAX_REVIVE
				{407, 407},  // EVENT_ROUTE_19_HIDDEN_PEARL
				{408, 408},  // EVENT_ROUTE_19_HIDDEN_BIG_PEARL
				{409, 409},  // EVENT_ROUTE_20_HIDDEN_STARDUST
				{410, 410},  // EVENT_ROUTE_21_HIDDEN_STARDUST_1
				{411, 411},  // EVENT_ROUTE_21_HIDDEN_STARDUST_2
				{412, 412},  // EVENT_ROUTE_24_HIDDEN_MAX_POTION
				{413, 413},  // EVENT_FOUND_BERSERK_GENE_IN_CERULEAN_CITY
				{414, 414},  // EVENT_FOUND_MACHINE_PART_IN_CERULEAN_GYM
				{415, 415},  // EVENT_VERMILION_CITY_HIDDEN_FULL_HEAL
				{416, 416},  // EVENT_CELADON_CITY_HIDDEN_PP_UP
				{417, 417},  // EVENT_CELADON_CHIEF_HOUSE_HIDDEN_DUBIOUS_DISC
				{418, 418},  // EVENT_CINNABAR_ISLAND_HIDDEN_RARE_CANDY
				{419, 419},  // EVENT_SAFARI_ZONE_NORTH_HIDDEN_LUCKY_PUNCH
				{420, 420},  // EVENT_SAFARI_ZONE_WEST_HIDDEN_NUGGET
				{421, 421},  // EVENT_URAGA_CHANNEL_EAST_HIDDEN_NUGGET
				{422, 422},  // EVENT_URAGA_CHANNEL_EAST_HIDDEN_PEARL
				{423, 423},  // EVENT_URAGA_CHANNEL_EAST_HIDDEN_BOTTLE_CAP
				{424, 424},  // EVENT_URAGA_CHANNEL_EAST_HIDDEN_STAR_PIECE
				{425, 425},  // EVENT_URAGA_CHANNEL_WEST_HIDDEN_BIG_PEARL
				{426, 426},  // EVENT_SEAFOAM_ISLANDS_1F_HIDDEN_ESCAPE_ROPE
				{427, 427},  // EVENT_SEAFOAM_ISLANDS_B1F_HIDDEN_ICE_HEAL
				{428, 428},  // EVENT_SEAFOAM_ISLANDS_B2F_HIDDEN_PEARL_1
				{429, 429},  // EVENT_SEAFOAM_ISLANDS_B2F_HIDDEN_PEARL_2
				{430, 430},  // EVENT_SEAFOAM_ISLANDS_B3F_HIDDEN_MAX_REVIVE
				{431, 431},  // EVENT_SEAFOAM_ISLANDS_B3F_HIDDEN_RARE_CANDY
				{432, 432},  // EVENT_CINNABAR_VOLCANO_1F_HIDDEN_FULL_RESTORE
				{433, 433},  // EVENT_CINNABAR_VOLCANO_B1F_HIDDEN_MAX_REVIVE
				{434, 434},  // EVENT_CINNABAR_VOLCANO_B1F_HIDDEN_DIRE_HIT
				{435, 435},  // EVENT_POKEMON_MANSION_1F_HIDDEN_FULL_RESTORE
				{436, 436},  // EVENT_POKEMON_MANSION_1F_HIDDEN_PP_UP
				{437, 437},  // EVENT_POKEMON_MANSION_B1F_HIDDEN_MAX_ELIXIR
				{438, 438},  // EVENT_POKEMON_MANSION_B1F_HIDDEN_RARE_CANDY
				{439, 439},  // EVENT_POKEMON_MANSION_B1F_HIDDEN_BERSERK_GENE
				{440, 440},  // EVENT_CINNABAR_LAB_HIDDEN_BERSERK_GENE
				{441, 441},  // EVENT_CERULEAN_CAVE_1F_HIDDEN_ULTRA_BALL
				{442, 442},  // EVENT_CERULEAN_CAVE_1F_HIDDEN_PP_UP
				{443, 443},  // EVENT_CERULEAN_CAVE_1F_HIDDEN_RARE_CANDY
				{444, 444},  // EVENT_CERULEAN_CAVE_1F_HIDDEN_BERSERK_GENE
				{445, 445},  // EVENT_CERULEAN_CAVE_2F_HIDDEN_PROTEIN
				{446, 446},  // EVENT_CERULEAN_CAVE_2F_HIDDEN_NUGGET
				{447, 447},  // EVENT_CERULEAN_CAVE_2F_HIDDEN_HYPER_POTION
				{448, 448},  // EVENT_CERULEAN_CAVE_B1F_HIDDEN_MAX_REVIVE
				{449, 449},  // EVENT_CERULEAN_CAVE_B1F_HIDDEN_ULTRA_BALL
				{450, 450},  // EVENT_DIM_CAVE_1F_HIDDEN_FULL_HEAL
				{451, 451},  // EVENT_DIM_CAVE_2F_HIDDEN_STARDUST
				{452, 452},  // EVENT_DIM_CAVE_2F_HIDDEN_MOON_STONE
				{453, 453},  // EVENT_DIM_CAVE_3F_HIDDEN_STAR_PIECE
				{454, 454},  // EVENT_DIM_CAVE_3F_HIDDEN_ZINC
				{455, 455},  // EVENT_DIM_CAVE_4F_HIDDEN_CALCIUM
				{456, 456},  // EVENT_DIM_CAVE_4F_HIDDEN_X_ATTACK
				{457, 457},  // EVENT_DIM_CAVE_5F_HIDDEN_X_SP_ATK
				{458, 458},  // EVENT_SCARY_CAVE_1F_HIDDEN_MAX_ELIXIR
				{459, 459},  // EVENT_SCARY_CAVE_1F_HIDDEN_PEARL_STRING
				{460, 460},  // EVENT_SCARY_CAVE_1F_HIDDEN_PEARL
				{461, 461},  // EVENT_NOISY_FOREST_HIDDEN_ULTRA_BALL
				{462, 462},  // EVENT_NOISY_FOREST_HIDDEN_TINYMUSHROOM
				{463, 463},  // EVENT_NOISY_FOREST_HIDDEN_FULL_RESTORE
				{464, 464},  // EVENT_SHAMOUTI_SHRINE_RUINS_HIDDEN_MAX_REVIVE
				{465, 465},  // EVENT_SHAMOUTI_TUNNEL_HIDDEN_LEAF_STONE
				{466, 466},  // EVENT_SHAMOUTI_TUNNEL_HIDDEN_NUGGET
				{467, 467},  // EVENT_WARM_BEACH_HIDDEN_PEARL
				{468, 468},  // EVENT_VALENCIA_PORT_HIDDEN_MAX_POTION
				{469, 469},  // EVENT_GAVE_KURT_RED_APRICORN
				{470, 470},  // EVENT_GAVE_KURT_BLU_APRICORN
				{471, 471},  // EVENT_GAVE_KURT_YLW_APRICORN
				{472, 472},  // EVENT_GAVE_KURT_GRN_APRICORN
				{473, 473},  // EVENT_GAVE_KURT_WHT_APRICORN
				{474, 474},  // EVENT_GAVE_KURT_BLK_APRICORN
				{475, 475},  // EVENT_GAVE_KURT_PNK_APRICORN
				{476, 476},  // EVENT_JACK_ASKED_FOR_PHONE_NUMBER
				{478, 478},  // EVENT_BEVERLY_ASKED_FOR_PHONE_NUMBER
				{480, 480},  // EVENT_HUEY_ASKED_FOR_PHONE_NUMBER
				{482, 482},  // EVENT_GOT_PROTEIN_FROM_HUEY
				{483, 483},  // EVENT_GOT_HP_UP_FROM_JOEY
				{484, 484},  // EVENT_GOT_CARBOS_FROM_VANCE
				{485, 485},  // EVENT_GOT_IRON_FROM_PARRY
				{486, 486},  // EVENT_GOT_CALCIUM_FROM_ERIN
				{487, 487},  // EVENT_KENJI_ON_BREAK
				{488, 488},  // EVENT_GAVEN_ASKED_FOR_PHONE_NUMBER
				{489, 489},  // EVENT_BETH_ASKED_FOR_PHONE_NUMBER
				{490, 490},  // EVENT_JOSE_ASKED_FOR_PHONE_NUMBER
				{491, 491},  // EVENT_REENA_ASKED_FOR_PHONE_NUMBER
				{492, 492},  // EVENT_JOEY_ASKED_FOR_PHONE_NUMBER
				{493, 493},  // EVENT_WADE_ASKED_FOR_PHONE_NUMBER
				{494, 494},  // EVENT_RALPH_ASKED_FOR_PHONE_NUMBER
				{495, 495},  // EVENT_LIZ_ASKED_FOR_PHONE_NUMBER
				{496, 496},  // EVENT_ANTHONY_ASKED_FOR_PHONE_NUMBER
				{497, 497},  // EVENT_TODD_ASKED_FOR_PHONE_NUMBER
				{498, 498},  // EVENT_GINA_ASKED_FOR_PHONE_NUMBER
				{499, 499},  // EVENT_IRWIN_ASKED_FOR_PHONE_NUMBER
				{500, 500},  // EVENT_ARNIE_ASKED_FOR_PHONE_NUMBER
				{501, 501},  // EVENT_ALAN_ASKED_FOR_PHONE_NUMBER
				{502, 502},  // EVENT_DANA_ASKED_FOR_PHONE_NUMBER
				{503, 503},  // EVENT_CHAD_ASKED_FOR_PHONE_NUMBER
				{504, 504},  // EVENT_DEREK_ASKED_FOR_PHONE_NUMBER
				{505, 505},  // EVENT_TULLY_ASKED_FOR_PHONE_NUMBER
				{506, 506},  // EVENT_BRENT_ASKED_FOR_PHONE_NUMBER
				{507, 507},  // EVENT_TIFFANY_ASKED_FOR_PHONE_NUMBER
				{508, 508},  // EVENT_VANCE_ASKED_FOR_PHONE_NUMBER
				{509, 509},  // EVENT_WILTON_ASKED_FOR_PHONE_NUMBER
				{510, 510},  // EVENT_KENJI_ASKED_FOR_PHONE_NUMBER
				{511, 511},  // EVENT_PARRY_ASKED_FOR_PHONE_NUMBER
				{512, 512},  // EVENT_ERIN_ASKED_FOR_PHONE_NUMBER
				{513, 513},  // EVENT_BUENA_OFFERED_HER_PHONE_NUMBER_NO_BLUE_CARD
				{514, 514},  // EVENT_GINA_GAVE_LEAF_STONE
				{515, 515},  // EVENT_ALAN_GAVE_FIRE_STONE
				{516, 516},  // EVENT_DANA_GAVE_THUNDERSTONE
				{517, 517},  // EVENT_TULLY_GAVE_WATER_STONE
				{518, 518},  // EVENT_TIFFANY_GAVE_PINK_BOW
				{519, 519},  // EVENT_SOLVED_HO_OH_PUZZLE
				{520, 520},  // EVENT_SOLVED_KABUTO_PUZZLE
				{521, 521},  // EVENT_SOLVED_OMANYTE_PUZZLE
				{522, 522},  // EVENT_SOLVED_AERODACTYL_PUZZLE
				{523, 523},  // EVENT_DECO_BED_1
				{524, 524},  // EVENT_DECO_BED_2
				{525, 525},  // EVENT_DECO_BED_3
				{526, 526},  // EVENT_DECO_BED_4
				{527, 527},  // EVENT_DECO_CARPET_1
				{528, 528},  // EVENT_DECO_CARPET_2
				{529, 529},  // EVENT_DECO_CARPET_3
				{530, 530},  // EVENT_DECO_CARPET_4
				{531, 531},  // EVENT_DECO_PLANT_1
				{532, 532},  // EVENT_DECO_PLANT_2
				{533, 533},  // EVENT_DECO_PLANT_3
				{534, 534},  // EVENT_DECO_POSTER_5
				{535, 535},  // EVENT_DECO_POSTER_1
				{536, 536},  // EVENT_DECO_POSTER_2
				{537, 537},  // EVENT_DECO_POSTER_3
				{538, 538},  // EVENT_DECO_POSTER_4
				{539, 539},  // EVENT_DECO_SNES
				{540, 540},  // EVENT_DECO_N64
				{541, 541},  // EVENT_DECO_GAMECUBE
				{542, 542},  // EVENT_DECO_WII
				{543, 543},  // EVENT_DECO_PIKACHU_DOLL
				{544, 544},  // EVENT_DECO_RAICHU_DOLL
				{545, 545},  // EVENT_DECO_SURFING_PIKACHU_DOLL
				{546, 546},  // EVENT_DECO_CLEFAIRY_DOLL
				{547, 547},  // EVENT_DECO_JIGGLYPUFF_DOLL
				{548, 548},  // EVENT_DECO_BULBASAUR_DOLL
				{549, 549},  // EVENT_DECO_CHARMANDER_DOLL
				{550, 550},  // EVENT_DECO_SQUIRTLE_DOLL
				{551, 551},  // EVENT_DECO_CHIKORITA_DOLL
				{552, 552},  // EVENT_DECO_CYNDAQUIL_DOLL
				{553, 553},  // EVENT_DECO_TOTODILE_DOLL
				{554, 554},  // EVENT_DECO_POLIWAG_DOLL
				{555, 555},  // EVENT_DECO_MAREEP_DOLL
				{556, 556},  // EVENT_DECO_TOGEPI_DOLL
				{557, 557},  // EVENT_DECO_MAGIKARP_DOLL
				{558, 558},  // EVENT_DECO_ODDISH_DOLL
				{559, 559},  // EVENT_DECO_GENGAR_DOLL
				{560, 560},  // EVENT_DECO_OCTILLERY_DOLL
				{561, 561},  // EVENT_DECO_DITTO_DOLL
				{562, 562},  // EVENT_DECO_VOLTORB_DOLL
				{563, 563},  // EVENT_DECO_ABRA_DOLL
				{564, 564},  // EVENT_DECO_UNOWN_DOLL
				{565, 565},  // EVENT_DECO_GEODUDE_DOLL
				{566, 566},  // EVENT_DECO_PINECO_DOLL
				{567, 567},  // EVENT_DECO_MARILL_DOLL
				{568, 568},  // EVENT_DECO_TEDDIURSA_DOLL
				{569, 569},  // EVENT_DECO_MEOWTH_DOLL
				{570, 570},  // EVENT_DECO_VULPIX_DOLL
				{571, 571},  // EVENT_DECO_GROWLITHE_DOLL
				{572, 572},  // EVENT_DECO_EEVEE_DOLL
				{573, 573},  // EVENT_PLAYERS_ROOM_POSTER
				{574, 574},  // EVENT_DECO_GOLD_TROPHY
				{575, 575},  // EVENT_DECO_SILVER_TROPHY
				{576, 576},  // EVENT_DECO_BIG_SNORLAX_DOLL
				{577, 577},  // EVENT_DECO_BIG_ONIX_DOLL
				{578, 578},  // EVENT_DECO_BIG_LAPRAS_DOLL
				{579, 579},  // EVENT_WARPED_FROM_ROUTE_35_NATIONAL_PARK_GATE
				{580, 580},  // EVENT_SWITCH_1
				{581, 581},  // EVENT_SWITCH_2
				{582, 582},  // EVENT_SWITCH_3
				{583, 583},  // EVENT_EMERGENCY_SWITCH
				{584, 584},  // EVENT_SWITCH_4
				{585, 585},  // EVENT_SWITCH_5
				{586, 586},  // EVENT_SWITCH_6
				{587, 587},  // EVENT_SWITCH_7
				{588, 588},  // EVENT_SWITCH_8
				{589, 589},  // EVENT_SWITCH_9
				{590, 590},  // EVENT_SWITCH_10
				{591, 591},  // EVENT_SWITCH_11
				{592, 592},  // EVENT_SWITCH_12
				{593, 593},  // EVENT_SWITCH_13
				{594, 594},  // EVENT_SWITCH_14
				{595, 595},  // EVENT_UNCOVERED_STAIRCASE_IN_MAHOGANY_MART
				{596, 596},  // EVENT_TURNED_OFF_SECURITY_CAMERAS
				{597, 597},  // EVENT_SECURITY_CAMERA_1
				{598, 598},  // EVENT_SECURITY_CAMERA_2
				{599, 599},  // EVENT_SECURITY_CAMERA_3
				{600, 600},  // EVENT_SECURITY_CAMERA_4
				{601, 601},  // EVENT_SECURITY_CAMERA_5
				{602, 602},  // EVENT_EXPLODING_TRAP_1
				{603, 603},  // EVENT_EXPLODING_TRAP_2
				{604, 604},  // EVENT_EXPLODING_TRAP_3
				{605, 605},  // EVENT_EXPLODING_TRAP_4
				{606, 606},  // EVENT_EXPLODING_TRAP_5
				{607, 607},  // EVENT_EXPLODING_TRAP_6
				{608, 608},  // EVENT_EXPLODING_TRAP_7
				{609, 609},  // EVENT_EXPLODING_TRAP_8
				{610, 610},  // EVENT_EXPLODING_TRAP_9
				{611, 611},  // EVENT_EXPLODING_TRAP_10
				{612, 612},  // EVENT_EXPLODING_TRAP_11
				{613, 613},  // EVENT_EXPLODING_TRAP_12
				{614, 614},  // EVENT_EXPLODING_TRAP_13
				{615, 615},  // EVENT_EXPLODING_TRAP_14
				{616, 616},  // EVENT_EXPLODING_TRAP_15
				{617, 617},  // EVENT_EXPLODING_TRAP_16
				{618, 618},  // EVENT_EXPLODING_TRAP_17
				{619, 619},  // EVENT_EXPLODING_TRAP_18
				{620, 620},  // EVENT_EXPLODING_TRAP_19
				{621, 621},  // EVENT_EXPLODING_TRAP_20
				{622, 622},  // EVENT_EXPLODING_TRAP_21
				{623, 623},  // EVENT_EXPLODING_TRAP_22
				{624, 624},  // EVENT_LEARNED_HAIL_GIOVANNI
				{625, 625},  // EVENT_OPENED_DOOR_TO_ROCKET_HIDEOUT_TRANSMITTER
				{626, 626},  // EVENT_LEARNED_SLOWPOKETAIL
				{627, 627},  // EVENT_LEARNED_RATICATE_TAIL
				{628, 628},  // EVENT_OPENED_DOOR_TO_GIOVANNIS_OFFICE
				{629, 629},  // EVENT_WAREHOUSE_LAYOUT_1
				{630, 630},  // EVENT_WAREHOUSE_LAYOUT_2
				{631, 631},  // EVENT_WAREHOUSE_LAYOUT_3
				{632, 632},  // EVENT_WAREHOUSE_BLOCKED_OFF
				{633, 633},  // EVENT_LEFT_MONS_WITH_CONTEST_OFFICER
				{634, 634},  // EVENT_WILLS_ROOM_ENTRANCE_CLOSED
				{635, 635},  // EVENT_WILLS_ROOM_EXIT_OPEN
				{636, 636},  // EVENT_KOGAS_ROOM_ENTRANCE_CLOSED
				{637, 637},  // EVENT_KOGAS_ROOM_EXIT_OPEN
				{638, 638},  // EVENT_BRUNOS_ROOM_ENTRANCE_CLOSED
				{639, 639},  // EVENT_BRUNOS_ROOM_EXIT_OPEN
				{640, 640},  // EVENT_KARENS_ROOM_ENTRANCE_CLOSED
				{641, 641},  // EVENT_KARENS_ROOM_EXIT_OPEN
				{642, 642},  // EVENT_LANCES_ROOM_ENTRANCE_CLOSED
				{643, 643},  // EVENT_LANCES_ROOM_EXIT_OPEN
				{644, 644},  // EVENT_CONTEST_OFFICER_HAS_PRIZE
				{645, 645},  // EVENT_FOUGHT_HO_OH
				{646, 646},  // EVENT_FOUGHT_LUGIA
				{647, 647},  // EVENT_BEAT_RIVAL_IN_MT_MOON
				{648, 648},  // EVENT_TRADED_RED_SCALE
				{649, 649},  // EVENT_BRED_AN_EGG
				{650, 650},  // EVENT_MET_BILLS_GRANDPA
				{651, 651},  // EVENT_SHOWED_SNUBBULL_TO_BILLS_GRANDPA
				{652, 652},  // EVENT_SHOWED_BELLSPROUT_TO_BILLS_GRANDPA
				{653, 653},  // EVENT_SHOWED_STARYU_TO_BILLS_GRANDPA
				{654, 654},  // EVENT_SHOWED_GROWLITHE_TO_BILLS_GRANDPA
				{655, 655},  // EVENT_SHOWED_PICHU_TO_BILLS_GRANDPA
				{656, 656},  // EVENT_SHOWED_JIGGLYPUFF_TO_BILLS_GRANDPA
				{657, 657},  // EVENT_SHOWED_ODDISH_TO_BILLS_GRANDPA
				{658, 658},  // EVENT_SHOWED_MURKROW_TO_BILLS_GRANDPA
				{659, 659},  // EVENT_SHOWED_TOGEPI_TO_BILLS_GRANDPA
				{660, 660},  // EVENT_GOT_EVERSTONE_FROM_BILLS_GRANDPA
				{661, 661},  // EVENT_GOT_LEAF_STONE_FROM_BILLS_GRANDPA
				{662, 662},  // EVENT_GOT_WATER_STONE_FROM_BILLS_GRANDPA
				{663, 663},  // EVENT_GOT_FIRE_STONE_FROM_BILLS_GRANDPA
				{664, 664},  // EVENT_GOT_THUNDERSTONE_FROM_BILLS_GRANDPA
				{665, 665},  // EVENT_GOT_MOON_STONE_FROM_BILLS_GRANDPA
				{666, 666},  // EVENT_GOT_SUN_STONE_FROM_BILLS_GRANDPA
				{667, 667},  // EVENT_GOT_DUSK_STONE_FROM_BILLS_GRANDPA
				{668, 668},  // EVENT_GOT_SHINY_STONE_FROM_BILLS_GRANDPA
				{669, 669},  // EVENT_LISTENED_TO_INITIAL_RADIO
				{670, 670},  // EVENT_BATTLE_TOWER_INTRO
				{671, 671},  // EVENT_BATTLE_FACTORY_INTRO
				{672, 672},  // EVENT_WALL_OPENED_IN_HO_OH_CHAMBER
				{673, 673},  // EVENT_WALL_OPENED_IN_KABUTO_CHAMBER
				{674, 674},  // EVENT_WALL_OPENED_IN_OMANYTE_CHAMBER
				{675, 675},  // EVENT_WALL_OPENED_IN_AERODACTYL_CHAMBER
				{676, 676},  // EVENT_DOOR_OPENED_IN_RUINS_OF_ALPH
				{677, 677},  // EVENT_BATTLE_TOWER_OPEN
				{678, 678},  // EVENT_BATTLE_TOWER_CLOSED
				{679, 679},  // EVENT_WELCOMING_TO_POKECOM_CENTER
				{680, 680},  // EVENT_WELCOMED_TO_POKECOM_CENTER
				{681, 681},  // EVENT_NURSE_SAW_TRAINER_STAR
				{682, 682},  // EVENT_WADE_HAS_ORAN_BERRY
				{683, 683},  // EVENT_WADE_HAS_PECHA_BERRY
				{684, 684},  // EVENT_WADE_HAS_CHERI_BERRY
				{685, 685},  // EVENT_WADE_HAS_PERSIM_BERRY
				{686, 686},  // EVENT_WILTON_HAS_ULTRA_BALL
				{687, 687},  // EVENT_WILTON_HAS_GREAT_BALL
				{688, 688},  // EVENT_WILTON_HAS_POKE_BALL
				{689, 689},  // EVENT_HOLE_IN_BURNED_TOWER
				{690, 690},  // EVENT_KOJI_ALLOWS_YOU_PASSAGE_TO_TIN_TOWER
				{691, 691},  // EVENT_FOUGHT_SUICUNE
				{692, 692},  // EVENT_GOT_RAINBOW_WING
				{693, 693},  // EVENT_HUEY_PROTEIN
				{694, 694},  // EVENT_JOEY_HP_UP
				{695, 695},  // EVENT_VANCE_CARBOS
				{696, 696},  // EVENT_PARRY_IRON
				{697, 697},  // EVENT_ERIN_CALCIUM
				{698, 698},  // EVENT_BUENA_OFFERED_HER_PHONE_NUMBER
				{699, 699},  // EVENT_MET_BUENA
				{700, 700},  // EVENT_GOT_ODD_EGG
				{701, 701},  // EVENT_GOT_GS_BALL_FROM_POKECOM_CENTER
				{702, 702},  // EVENT_GOT_ARMOR_SUIT
				{703, 703},  // EVENT_TIME_TRAVELING
				{704, 704},  // EVENT_TIME_TRAVEL_FINISHED
				{705, 705},  // EVENT_LYRA_GAVE_AWAY_EGG
				{706, 706},  // EVENT_GOT_LYRAS_EGG
				{707, 707},  // EVENT_GOT_RIVALS_EGG
				{708, 708},  // EVENT_GOT_BULBASAUR_FROM_IVY
				{709, 709},  // EVENT_GOT_CHARMANDER_FROM_IVY
				{710, 710},  // EVENT_GOT_SQUIRTLE_FROM_IVY
				{711, 711},  // EVENT_GOT_A_POKEMON_FROM_IVY
				{712, 712},  // EVENT_GOT_A_POKEMON_FROM_OAK
				{713, 713},  // EVENT_GOT_A_POKEMON_FROM_YELLOW
				{714, 714},  // EVENT_BEAT_FALKNER
				{715, 715},  // EVENT_BEAT_BUGSY
				{716, 716},  // EVENT_BEAT_WHITNEY
				{717, 717},  // EVENT_BEAT_MORTY
				{718, 718},  // EVENT_BEAT_JASMINE
				{719, 719},  // EVENT_BEAT_CHUCK
				{720, 720},  // EVENT_BEAT_PRYCE
				{721, 721},  // EVENT_BEAT_CLAIR
				{722, 722},  // EVENT_BEAT_ELITE_4_WILL
				{723, 723},  // EVENT_BEAT_ELITE_4_KOGA
				{724, 724},  // EVENT_BEAT_ELITE_4_BRUNO
				{725, 725},  // EVENT_BEAT_ELITE_4_KAREN
				{726, 726},  // EVENT_BEAT_CHAMPION_LANCE
				{727, 727},  // EVENT_BEAT_BROCK
				{728, 728},  // EVENT_BEAT_MISTY
				{729, 729},  // EVENT_BEAT_LTSURGE
				{730, 730},  // EVENT_BEAT_ERIKA
				{731, 731},  // EVENT_BEAT_JANINE
				{732, 732},  // EVENT_BEAT_SABRINA
				{733, 733},  // EVENT_BEAT_BLAINE
				{734, 734},  // EVENT_BEAT_BLUE
				{735, 735},  // EVENT_BEAT_RED
				{736, 736},  // EVENT_BEAT_LEAF
				{737, 737},  // EVENT_BEAT_YOUNGSTER_JOEY
				{738, 738},  // EVENT_BEAT_YOUNGSTER_MIKEY
				{739, 739},  // EVENT_BEAT_YOUNGSTER_ALBERT
				{740, 740},  // EVENT_BEAT_YOUNGSTER_GORDON
				{741, 741},  // EVENT_BEAT_YOUNGSTER_WARREN
				{742, 742},  // EVENT_BEAT_YOUNGSTER_JIMMY
				{743, 743},  // EVENT_BEAT_YOUNGSTER_OWEN
				{744, 744},  // EVENT_BEAT_YOUNGSTER_JASON
				{745, 745},  // EVENT_BEAT_YOUNGSTER_JOSH
				{746, 746},  // EVENT_BEAT_YOUNGSTER_REGIS
				{747, 747},  // EVENT_BEAT_YOUNGSTER_ALFIE
				{748, 748},  // EVENT_BEAT_YOUNGSTER_OLIVER
				{749, 749},  // EVENT_BEAT_YOUNGSTER_CHAZ
				{750, 750},  // EVENT_BEAT_YOUNGSTER_TYLER
				{751, 751},  // EVENT_BEAT_BUG_CATCHER_WADE
				{752, 752},  // EVENT_BEAT_BUG_CATCHER_ARNIE
				{753, 753},  // EVENT_BEAT_BUG_CATCHER_DON
				{754, 754},  // EVENT_BEAT_BUG_CATCHER_BENNY
				{755, 755},  // EVENT_BEAT_BUG_CATCHER_AL
				{756, 756},  // EVENT_BEAT_BUG_CATCHER_JOSH
				{757, 757},  // EVENT_BEAT_BUG_CATCHER_KEN
				{758, 758},  // EVENT_BEAT_BUG_CATCHER_WAYNE
				{759, 759},  // EVENT_BEAT_BUG_CATCHER_OSCAR
				{761, 761},  // EVENT_BEAT_BUG_CATCHER_DAVID
				{762, 762},  // EVENT_BEAT_CAMPER_TODD
				{763, 763},  // EVENT_BEAT_CAMPER_ROLAND
				{764, 764},  // EVENT_BEAT_CAMPER_IVAN
				{765, 765},  // EVENT_BEAT_CAMPER_BARRY
				{766, 766},  // EVENT_BEAT_CAMPER_LLOYD
				{767, 767},  // EVENT_BEAT_CAMPER_DEAN
				{768, 768},  // EVENT_BEAT_CAMPER_SID
				{769, 769},  // EVENT_BEAT_CAMPER_TED
				{770, 770},  // EVENT_BEAT_CAMPER_JOHN
				{771, 771},  // EVENT_BEAT_CAMPER_JERRY
				{772, 772},  // EVENT_BEAT_CAMPER_SPENCER
				{773, 773},  // EVENT_BEAT_CAMPER_QUENTIN
				{774, 774},  // EVENT_BEAT_CAMPER_GRANT
				{775, 775},  // EVENT_BEAT_CAMPER_CRAIG
				{776, 776},  // EVENT_BEAT_CAMPER_FELIX
				{777, 777},  // EVENT_BEAT_CAMPER_TANNER
				{778, 778},  // EVENT_BEAT_CAMPER_CLARK
				{779, 779},  // EVENT_BEAT_CAMPER_PEDRO
				{780, 780},  // EVENT_BEAT_CAMPER_AMOS
				{781, 781},  // EVENT_BEAT_PICNICKER_LIZ
				{782, 782},  // EVENT_BEAT_PICNICKER_GINA
				{783, 783},  // EVENT_BEAT_PICNICKER_ERIN
				{784, 784},  // EVENT_BEAT_PICNICKER_TIFFANY
				{785, 785},  // EVENT_BEAT_PICNICKER_KIM
				{786, 786},  // EVENT_BEAT_PICNICKER_CINDY
				{787, 787},  // EVENT_BEAT_PICNICKER_HOPE
				{788, 788},  // EVENT_BEAT_PICNICKER_SHARON
				{789, 789},  // EVENT_BEAT_PICNICKER_DEBRA
				{790, 790},  // EVENT_BEAT_PICNICKER_HEIDI
				{791, 791},  // EVENT_BEAT_PICNICKER_EDNA
				{792, 792},  // EVENT_BEAT_PICNICKER_TANYA
				{793, 793},  // EVENT_BEAT_PICNICKER_LILY
				{794, 794},  // EVENT_BEAT_PICNICKER_GINGER
				{795, 795},  // EVENT_BEAT_PICNICKER_CHEYENNE
				{796, 796},  // EVENT_BEAT_PICNICKER_ADRIAN
				{797, 797},  // EVENT_BEAT_PICNICKER_PIPER
				{798, 798},  // EVENT_BEAT_TWINS_AMY_AND_MAY
				{799, 799},  // EVENT_BEAT_TWINS_ANN_AND_ANNE
				{800, 800},  // EVENT_BEAT_TWINS_JO_AND_ZOE
				{801, 801},  // EVENT_BEAT_TWINS_MEG_AND_PEG
				{802, 802},  // EVENT_BEAT_TWINS_LEA_AND_PIA
				{803, 803},  // EVENT_BEAT_TWINS_DAY_AND_DANI
				{804, 804},  // EVENT_BEAT_TWINS_KAY_AND_TIA
				{805, 805},  // EVENT_BEAT_FISHER_RALPH
				{806, 806},  // EVENT_BEAT_FISHER_TULLY
				{807, 807},  // EVENT_BEAT_FISHER_WILTON
				{808, 808},  // EVENT_BEAT_FISHER_JUSTIN
				{809, 809},  // EVENT_BEAT_FISHER_ARNOLD
				{810, 810},  // EVENT_BEAT_FISHER_KYLE
				{811, 811},  // EVENT_BEAT_FISHER_HENRY
				{812, 812},  // EVENT_BEAT_FISHER_MARVIN
				{813, 813},  // EVENT_BEAT_FISHER_ANDRE
				{814, 814},  // EVENT_BEAT_FISHER_RAYMOND
				{815, 815},  // EVENT_BEAT_FISHER_EDGAR
				{816, 816},  // EVENT_BEAT_FISHER_JONAH
				{817, 817},  // EVENT_BEAT_FISHER_MARTIN
				{818, 818},  // EVENT_BEAT_FISHER_STEPHEN
				{819, 819},  // EVENT_BEAT_FISHER_BARNEY
				{820, 820},  // EVENT_BEAT_FISHER_SCOTT
				{821, 821},  // EVENT_BEAT_FISHER_PATON
				{822, 822},  // EVENT_BEAT_FISHER_KILEY
				{823, 823},  // EVENT_BEAT_FISHER_FRANCIS
				{824, 824},  // EVENT_BEAT_FISHER_LEROY
				{825, 825},  // EVENT_BEAT_FISHER_KYLER
				{826, 826},  // EVENT_BEAT_FISHER_MURPHY
				{827, 827},  // EVENT_BEAT_FISHER_LIAM
				{828, 828},  // EVENT_BEAT_FISHER_GIDEON
				{830, 830},  // EVENT_BEAT_FISHER_HALL
				{831, 831},  // EVENT_BEAT_FISHER_DALLAS
				{832, 832},  // EVENT_BEAT_BIRD_KEEPER_VANCE
				{833, 833},  // EVENT_BEAT_BIRD_KEEPER_JOSE
				{834, 834},  // EVENT_BEAT_BIRD_KEEPER_ROD
				{835, 835},  // EVENT_BEAT_BIRD_KEEPER_ABE
				{836, 836},  // EVENT_BEAT_BIRD_KEEPER_THEO
				{837, 837},  // EVENT_BEAT_BIRD_KEEPER_TOBY
				{838, 838},  // EVENT_BEAT_BIRD_KEEPER_DENIS
				{839, 839},  // EVENT_BEAT_BIRD_KEEPER_HANK
				{840, 840},  // EVENT_BEAT_BIRD_KEEPER_ROY
				{841, 841},  // EVENT_BEAT_BIRD_KEEPER_BORIS
				{842, 842},  // EVENT_BEAT_BIRD_KEEPER_BOB
				{843, 843},  // EVENT_BEAT_BIRD_KEEPER_PETER
				{844, 844},  // EVENT_BEAT_BIRD_KEEPER_PERRY
				{845, 845},  // EVENT_BEAT_BIRD_KEEPER_BRET
				{846, 846},  // EVENT_BEAT_BIRD_KEEPER_MICK
				{847, 847},  // EVENT_BEAT_BIRD_KEEPER_POWELL
				{848, 848},  // EVENT_BEAT_BIRD_KEEPER_TONY
				{849, 849},  // EVENT_BEAT_BIRD_KEEPER_JULIAN
				{850, 850},  // EVENT_BEAT_BIRD_KEEPER_JUSTIN
				{851, 851},  // EVENT_BEAT_BIRD_KEEPER_GAIL
				{852, 852},  // EVENT_BEAT_BIRD_KEEPER_JOSH
				{853, 853},  // EVENT_BEAT_BIRD_KEEPER_BERT
				{854, 854},  // EVENT_BEAT_BIRD_KEEPER_ERNIE
				{855, 855},  // EVENT_BEAT_BIRD_KEEPER_KINSLEY
				{856, 856},  // EVENT_BEAT_BIRD_KEEPER_EASTON
				{857, 857},  // EVENT_BEAT_BIRD_KEEPER_BRYAN
				{858, 858},  // EVENT_BEAT_BIRD_KEEPER_TRENT
				{859, 859},  // EVENT_BEAT_HIKER_ANTHONY
				{860, 860},  // EVENT_BEAT_HIKER_PARRY
				{861, 861},  // EVENT_BEAT_HIKER_RUSSELL
				{862, 862},  // EVENT_BEAT_HIKER_PHILLIP
				{863, 863},  // EVENT_BEAT_HIKER_LEONARD
				{864, 864},  // EVENT_BEAT_HIKER_BENJAMIN
				{865, 865},  // EVENT_BEAT_HIKER_ERIK
				{866, 866},  // EVENT_BEAT_HIKER_MICHAEL
				{867, 867},  // EVENT_BEAT_HIKER_TIMOTHY
				{868, 868},  // EVENT_BEAT_HIKER_BAILEY
				{869, 869},  // EVENT_BEAT_HIKER_TIM
				{870, 870},  // EVENT_BEAT_HIKER_NOLAND
				{871, 871},  // EVENT_BEAT_HIKER_SIDNEY
				{872, 872},  // EVENT_BEAT_HIKER_KENNY
				{873, 873},  // EVENT_BEAT_HIKER_JIM
				{874, 874},  // EVENT_BEAT_HIKER_DANIEL
				{875, 875},  // EVENT_BEAT_HIKER_EDWIN
				{876, 876},  // EVENT_BEAT_HIKER_DEVIN
				{877, 877},  // EVENT_BEAT_HIKER_SEAMUS
				{878, 878},  // EVENT_BEAT_HIKER_TONY
				{879, 879},  // EVENT_BEAT_HIKER_MARCOS
				{880, 880},  // EVENT_BEAT_HIKER_GERARD
				{881, 881},  // EVENT_BEAT_HIKER_DENT
				{882, 882},  // EVENT_BEAT_HIKER_BRUCE
				{883, 883},  // EVENT_BEAT_HIKER_DWIGHT
				{884, 884},  // EVENT_BEAT_HIKER_LESTER
				{885, 885},  // EVENT_BEAT_HIKER_GRADY
				{886, 886},  // EVENT_BEAT_HIKER_STEVE
				{887, 887},  // EVENT_BEAT_HIKER_DERRICK
				{888, 888},  // EVENT_BEAT_HIKER_FLOYD
				{889, 889},  // EVENT_BEAT_ROCKET_GRUNTM_2
				{890, 890},  // EVENT_BEAT_ROCKET_GRUNTM_3
				{891, 891},  // EVENT_BEAT_ROCKET_GRUNTM_4
				{892, 892},  // EVENT_BEAT_ROCKET_GRUNTM_5
				{893, 893},  // EVENT_BEAT_ROCKET_GRUNTM_6
				{894, 894},  // EVENT_BEAT_ROCKET_GRUNTM_7
				{895, 895},  // EVENT_BEAT_ROCKET_GRUNTM_8
				{896, 896},  // EVENT_BEAT_ROCKET_GRUNTM_9
				{897, 897},  // EVENT_BEAT_ROCKET_GRUNTM_10
				{898, 898},  // EVENT_BEAT_ROCKET_GRUNTM_11
				{899, 899},  // EVENT_BEAT_ROCKET_GRUNTM_12
				{900, 900},  // EVENT_BEAT_ROCKET_GRUNTM_13
				{901, 901},  // EVENT_BEAT_ROCKET_GRUNTM_14
				{902, 902},  // EVENT_BEAT_ROCKET_GRUNTM_15
				{903, 903},  // EVENT_BEAT_ROCKET_GRUNTM_16
				{904, 904},  // EVENT_BEAT_ROCKET_GRUNTM_17
				{905, 905},  // EVENT_BEAT_ROCKET_GRUNTM_18
				{906, 906},  // EVENT_BEAT_ROCKET_GRUNTM_19
				{907, 907},  // EVENT_BEAT_ROCKET_GRUNTM_20
				{908, 908},  // EVENT_BEAT_ROCKET_GRUNTM_21
				{909, 909},  // EVENT_BEAT_ROCKET_GRUNTM_22
				{910, 910},  // EVENT_BEAT_ROCKET_GRUNTM_23
				{911, 911},  // EVENT_BEAT_ROCKET_GRUNTM_24
				{912, 912},  // EVENT_BEAT_ROCKET_GRUNTM_25
				{913, 913},  // EVENT_BEAT_ROCKET_GRUNTM_26
				{914, 914},  // EVENT_BEAT_ROCKET_GRUNTM_28
				{915, 915},  // EVENT_BEAT_ROCKET_GRUNTM_29
				{916, 916},  // EVENT_BEAT_ROCKET_GRUNTM_30
				{917, 917},  // EVENT_BEAT_ROCKET_GRUNTM_31
				{918, 918},  // EVENT_BEAT_ROCKET_GRUNTF_1
				{919, 919},  // EVENT_BEAT_ROCKET_GRUNTF_2
				{920, 920},  // EVENT_BEAT_ROCKET_GRUNTF_3
				{921, 921},  // EVENT_BEAT_ROCKET_GRUNTF_4
				{922, 922},  // EVENT_BEAT_ROCKET_GRUNTF_5
				{923, 923},  // EVENT_BEAT_ROCKET_GRUNTF_6
				{924, 924},  // EVENT_BEAT_POKEFANM_DEREK
				{925, 925},  // EVENT_BEAT_POKEFANM_WILLIAM
				{926, 926},  // EVENT_BEAT_POKEFANM_ROBERT
				{927, 927},  // EVENT_BEAT_POKEFANM_JOSHUA
				{928, 928},  // EVENT_BEAT_POKEFANM_CARTER
				{929, 929},  // EVENT_BEAT_POKEFANM_TREVOR
				{930, 930},  // EVENT_BEAT_POKEFANM_BRANDON
				{931, 931},  // EVENT_BEAT_POKEFANM_JEREMY
				{932, 932},  // EVENT_BEAT_POKEFANM_COLIN
				{933, 933},  // EVENT_BEAT_POKEFANM_ALEX
				{934, 934},  // EVENT_BEAT_POKEFANM_REX
				{935, 935},  // EVENT_BEAT_POKEFANM_ALLAN
				{936, 936},  // EVENT_BEAT_POKEFANF_BEVERLY
				{937, 937},  // EVENT_BEAT_POKEFANF_RUTH
				{938, 938},  // EVENT_BEAT_POKEFANF_GEORGIA
				{939, 939},  // EVENT_BEAT_POKEFANF_JAIME
				{940, 940},  // EVENT_BEAT_POKEFANF_BOONE
				{941, 941},  // EVENT_BEAT_POKEFANF_ELEANOR
				{942, 942},  // EVENT_BEAT_OFFICERM_KEITH
				{943, 943},  // EVENT_BEAT_OFFICERM_DIRK
				{944, 944},  // EVENT_BEAT_OFFICERF_JAMIE
				{945, 945},  // EVENT_BEAT_OFFICERF_MARA
				{946, 946},  // EVENT_BEAT_OFFICERF_JENNY
				{947, 947},  // EVENT_BEAT_POKEMANIAC_BRENT
				{948, 948},  // EVENT_BEAT_POKEMANIAC_LARRY
				{949, 949},  // EVENT_BEAT_POKEMANIAC_ANDREW
				{950, 950},  // EVENT_BEAT_POKEMANIAC_CALVIN
				{951, 951},  // EVENT_BEAT_POKEMANIAC_SHANE
				{952, 952},  // EVENT_BEAT_POKEMANIAC_BEN
				{953, 953},  // EVENT_BEAT_POKEMANIAC_RON
				{954, 954},  // EVENT_BEAT_POKEMANIAC_ETHAN
				{955, 955},  // EVENT_BEAT_POKEMANIAC_ISSAC
				{956, 956},  // EVENT_BEAT_POKEMANIAC_DONALD
				{957, 957},  // EVENT_BEAT_POKEMANIAC_ZACH
				{958, 958},  // EVENT_BEAT_POKEMANIAC_MILLER
				{959, 959},  // EVENT_BEAT_POKEMANIAC_AIDAN
				{961, 961},  // EVENT_BEAT_COSPLAYER_CLARA
				{962, 962},  // EVENT_BEAT_COSPLAYER_CHLOE
				{963, 963},  // EVENT_BEAT_COSPLAYER_BROOKE
				{964, 964},  // EVENT_BEAT_COSPLAYER_KUROKO
				{965, 965},  // EVENT_BEAT_SUPER_NERD_STAN
				{966, 966},  // EVENT_BEAT_SUPER_NERD_ERIC
				{967, 967},  // EVENT_BEAT_SUPER_NERD_SAM
				{968, 968},  // EVENT_BEAT_SUPER_NERD_TOM
				{969, 969},  // EVENT_BEAT_SUPER_NERD_PAT
				{970, 970},  // EVENT_BEAT_SUPER_NERD_SHAWN
				{971, 971},  // EVENT_BEAT_SUPER_NERD_TERU
				{972, 972},  // EVENT_BEAT_SUPER_NERD_HUGH
				{973, 973},  // EVENT_BEAT_SUPER_NERD_MARKUS
				{974, 974},  // EVENT_BEAT_SUPER_NERD_CARY
				{975, 975},  // EVENT_BEAT_SUPER_NERD_WALDO
				{976, 976},  // EVENT_BEAT_SUPER_NERD_MERLE
				{977, 977},  // EVENT_BEAT_SUPER_NERD_LUIS
				{978, 978},  // EVENT_BEAT_SUPER_NERD_MIGUEL
				{979, 979},  // EVENT_BEAT_SUPER_NERD_JOVAN
				{980, 980},  // EVENT_BEAT_SUPER_NERD_RORY
				{981, 981},  // EVENT_BEAT_SUPER_NERD_GREGG
				{982, 982},  // EVENT_BEAT_SUPER_NERD_FOOTE
				{983, 983},  // EVENT_BEAT_SUPER_NERD_DAVE
				{984, 984},  // EVENT_BEAT_SUPER_NERD_KOUTA
				{985, 985},  // EVENT_BEAT_LASS_DANA
				{986, 986},  // EVENT_BEAT_LASS_CATHY
				{987, 987},  // EVENT_BEAT_LASS_AMANDA
				{988, 988},  // EVENT_BEAT_LASS_KRISE
				{989, 989},  // EVENT_BEAT_LASS_CONNIE
				{990, 990},  // EVENT_BEAT_LASS_LINDA
				{991, 991},  // EVENT_BEAT_LASS_LAURA
				{992, 992},  // EVENT_BEAT_LASS_SHANNON
				{993, 993},  // EVENT_BEAT_LASS_MICHELLE
				{994, 994},  // EVENT_BEAT_LASS_ELLEN
				{995, 995},  // EVENT_BEAT_LASS_IRIS
				{996, 996},  // EVENT_BEAT_LASS_MIRIAM
				{997, 997},  // EVENT_BEAT_LASS_LAYLA
				{998, 998},  // EVENT_BEAT_LASS_ROSE
				{999, 999},  // EVENT_BEAT_LASS_MEADOW
				{1000, 1000},  // EVENT_BEAT_LASS_JENNIFER
				{1001, 1001},  // EVENT_BEAT_LASS_GINA
				{1002, 1002},  // EVENT_BEAT_LASS_ALICE
				{1003, 1003},  // EVENT_BEAT_LASS_DUPLICA
				{1004, 1004},  // EVENT_BEAT_BEAUTY_VICTORIA
				{1005, 1005},  // EVENT_BEAT_BEAUTY_SAMANTHA
				{1006, 1006},  // EVENT_BEAT_BEAUTY_CASSIE
				{1007, 1007},  // EVENT_BEAT_BEAUTY_JULIA
				{1008, 1008},  // EVENT_BEAT_BEAUTY_VALENCIA
				{1009, 1009},  // EVENT_BEAT_BEAUTY_OLIVIA
				{1010, 1010},  // EVENT_BEAT_BEAUTY_CALLIE
				{1011, 1011},  // EVENT_BEAT_BEAUTY_CASSANDRA
				{1012, 1012},  // EVENT_BEAT_BEAUTY_CHARLOTTE
				{1013, 1013},  // EVENT_BEAT_BEAUTY_BRIDGET
				{1014, 1014},  // EVENT_BEAT_BEAUTY_VERONICA
				{1015, 1015},  // EVENT_BEAT_BEAUTY_NICOLE
				{1016, 1016},  // EVENT_BEAT_BEAUTY_RACHAEL
				{1017, 1017},  // EVENT_BEAT_BEAUTY_IOANA
				{1018, 1018},  // EVENT_BEAT_BUG_MANIAC_LOU
				{1019, 1019},  // EVENT_BEAT_BUG_MANIAC_ROB
				{1020, 1020},  // EVENT_BEAT_BUG_MANIAC_ED
				{1021, 1021},  // EVENT_BEAT_BUG_MANIAC_DOUG
				{1022, 1022},  // EVENT_BEAT_BUG_MANIAC_DANE
				{1023, 1023},  // EVENT_BEAT_BUG_MANIAC_DION
				{1024, 1024},  // EVENT_BEAT_BUG_MANIAC_STACEY
				{1025, 1025},  // EVENT_BEAT_BUG_MANIAC_ELLIS
				{1026, 1026},  // EVENT_BEAT_BUG_MANIAC_ABNER
				{1027, 1027},  // EVENT_BEAT_BUG_MANIAC_KENTA
				{1028, 1028},  // EVENT_BEAT_BUG_MANIAC_ROBBY
				{1029, 1029},  // EVENT_BEAT_BUG_MANIAC_PIERRE
				{1030, 1030},  // EVENT_BEAT_BUG_MANIAC_DYLAN
				{1031, 1031},  // EVENT_BEAT_BUG_MANIAC_KAI
				{1032, 1032},  // EVENT_BEAT_RUIN_MANIAC_JONES
				{1033, 1033},  // EVENT_BEAT_RUIN_MANIAC_LELAND
				{1034, 1034},  // EVENT_BEAT_RUIN_MANIAC_PETRY
				{1035, 1035},  // EVENT_BEAT_RUIN_MANIAC_GLYN
				{1036, 1036},  // EVENT_BEAT_RUIN_MANIAC_SMILTE
				{1037, 1037},  // EVENT_BEAT_FIREBREATHER_OTIS
				{1038, 1038},  // EVENT_BEAT_FIREBREATHER_DICK
				{1039, 1039},  // EVENT_BEAT_FIREBREATHER_NED
				{1040, 1040},  // EVENT_BEAT_FIREBREATHER_BURT
				{1041, 1041},  // EVENT_BEAT_FIREBREATHER_BILL
				{1042, 1042},  // EVENT_BEAT_FIREBREATHER_WALT
				{1043, 1043},  // EVENT_BEAT_FIREBREATHER_RAY
				{1044, 1044},  // EVENT_BEAT_FIREBREATHER_LYLE
				{1045, 1045},  // EVENT_BEAT_FIREBREATHER_JAY
				{1046, 1046},  // EVENT_BEAT_FIREBREATHER_OLEG
				{1047, 1047},  // EVENT_BEAT_FIREBREATHER_TALA
				{1048, 1048},  // EVENT_BEAT_JUGGLER_IRWIN
				{1049, 1049},  // EVENT_BEAT_JUGGLER_FRITZ
				{1050, 1050},  // EVENT_BEAT_JUGGLER_HORTON
				{1051, 1051},  // EVENT_BEAT_SCHOOLBOY_JACK
				{1052, 1052},  // EVENT_BEAT_SCHOOLBOY_ALAN
				{1053, 1053},  // EVENT_BEAT_SCHOOLBOY_CHAD
				{1054, 1054},  // EVENT_BEAT_SCHOOLBOY_KIP
				{1055, 1055},  // EVENT_BEAT_SCHOOLBOY_JOHNNY
				{1056, 1056},  // EVENT_BEAT_SCHOOLBOY_DANNY
				{1057, 1057},  // EVENT_BEAT_SCHOOLBOY_TOMMY
				{1058, 1058},  // EVENT_BEAT_SCHOOLBOY_DUDLEY
				{1059, 1059},  // EVENT_BEAT_SCHOOLBOY_JOE
				{1060, 1060},  // EVENT_BEAT_SCHOOLBOY_BILLY
				{1061, 1061},  // EVENT_BEAT_SCHOOLBOY_NATE
				{1062, 1062},  // EVENT_BEAT_SCHOOLBOY_RICKY
				{1063, 1063},  // EVENT_BEAT_SCHOOLBOY_SHERMAN
				{1064, 1064},  // EVENT_BEAT_SCHOOLBOY_CONNOR
				{1065, 1065},  // EVENT_BEAT_SCHOOLBOY_TORIN
				{1066, 1066},  // EVENT_BEAT_SCHOOLBOY_TRAVIS
				{1067, 1067},  // EVENT_BEAT_SCHOOLGIRL_MOLLY
				{1068, 1068},  // EVENT_BEAT_SCHOOLGIRL_ELIZA
				{1069, 1069},  // EVENT_BEAT_SCHOOLGIRL_FAITH
				{1070, 1070},  // EVENT_BEAT_SCHOOLGIRL_SARAH
				{1071, 1071},  // EVENT_BEAT_SCHOOLGIRL_ISABEL
				{1072, 1072},  // EVENT_BEAT_SCHOOLGIRL_IMOGEN
				{1073, 1073},  // EVENT_BEAT_PSYCHIC_NATHAN
				{1074, 1074},  // EVENT_BEAT_PSYCHIC_FRANKLIN
				{1075, 1075},  // EVENT_BEAT_PSYCHIC_HERMAN
				{1076, 1076},  // EVENT_BEAT_PSYCHIC_FIDEL
				{1077, 1077},  // EVENT_BEAT_PSYCHIC_GREG
				{1078, 1078},  // EVENT_BEAT_PSYCHIC_NORMAN
				{1079, 1079},  // EVENT_BEAT_PSYCHIC_MARK
				{1080, 1080},  // EVENT_BEAT_PSYCHIC_PHIL
				{1081, 1081},  // EVENT_BEAT_PSYCHIC_RICHARD
				{1082, 1082},  // EVENT_BEAT_PSYCHIC_GILBERT
				{1083, 1083},  // EVENT_BEAT_PSYCHIC_JARED
				{1084, 1084},  // EVENT_BEAT_PSYCHIC_RODNEY
				{1085, 1085},  // EVENT_BEAT_PSYCHIC_LEON
				{1086, 1086},  // EVENT_BEAT_PSYCHIC_URI
				{1087, 1087},  // EVENT_BEAT_PSYCHIC_VIRGIL
				{1088, 1088},  // EVENT_BEAT_HEX_MANIAC_TAMARA
				{1089, 1089},  // EVENT_BEAT_HEX_MANIAC_ASHLEY
				{1090, 1090},  // EVENT_BEAT_HEX_MANIAC_AMY
				{1091, 1091},  // EVENT_BEAT_HEX_MANIAC_LUNA
				{1092, 1092},  // EVENT_BEAT_HEX_MANIAC_NATALIE
				{1093, 1093},  // EVENT_BEAT_HEX_MANIAC_VIVIAN
				{1094, 1094},  // EVENT_BEAT_HEX_MANIAC_ESTHER
				{1095, 1095},  // EVENT_BEAT_HEX_MANIAC_MATILDA
				{1096, 1096},  // EVENT_BEAT_HEX_MANIAC_BETHANY
				{1097, 1097},  // EVENT_BEAT_SAGE_CHOW
				{1098, 1098},  // EVENT_BEAT_SAGE_NICO
				{1099, 1099},  // EVENT_BEAT_SAGE_JIN
				{1100, 1100},  // EVENT_BEAT_SAGE_TROY
				{1101, 1101},  // EVENT_BEAT_SAGE_JEFFREY
				{1102, 1102},  // EVENT_BEAT_SAGE_PING
				{1103, 1103},  // EVENT_BEAT_SAGE_EDMOND
				{1104, 1104},  // EVENT_BEAT_SAGE_NEAL
				{1105, 1105},  // EVENT_BEAT_MEDIUM_MARTHA
				{1106, 1106},  // EVENT_BEAT_MEDIUM_GRACE
				{1107, 1107},  // EVENT_BEAT_MEDIUM_REBECCA
				{1108, 1108},  // EVENT_BEAT_MEDIUM_DORIS
				{1109, 1109},  // EVENT_BEAT_KIMONO_GIRL_NAOKO
				{1110, 1110},  // EVENT_BEAT_KIMONO_GIRL_SAYO
				{1111, 1111},  // EVENT_BEAT_KIMONO_GIRL_ZUKI
				{1112, 1112},  // EVENT_BEAT_KIMONO_GIRL_KUNI
				{1113, 1113},  // EVENT_BEAT_KIMONO_GIRL_MIKI
				{1114, 1114},  // EVENT_BEAT_ELDER_LI
				{1115, 1115},  // EVENT_BEAT_ELDER_GAKU
				{1116, 1116},  // EVENT_BEAT_ELDER_MASA
				{1117, 1117},  // EVENT_BEAT_ELDER_KOJI
				{1118, 1118},  // EVENT_BEAT_SR_AND_JR_JO_AND_CATH
				{1119, 1119},  // EVENT_BEAT_SR_AND_JR_IVY_AND_AMY
				{1120, 1120},  // EVENT_BEAT_SR_AND_JR_BEA_AND_MAY
				{1121, 1121},  // EVENT_BEAT_COUPLE_GAIL_AND_ELI
				{1122, 1122},  // EVENT_BEAT_COUPLE_DUFF_AND_EDA
				{1123, 1123},  // EVENT_BEAT_COUPLE_FOX_AND_RAE
				{1124, 1124},  // EVENT_BEAT_COUPLE_MOE_AND_LULU
				{1125, 1125},  // EVENT_BEAT_COUPLE_VIC_AND_TARA
				{1126, 1126},  // EVENT_BEAT_COUPLE_TIM_AND_SUE
				{1127, 1127},  // EVENT_BEAT_COUPLE_JOE_AND_JO
				{1128, 1128},  // EVENT_BEAT_GENTLEMAN_PRESTON
				{1129, 1129},  // EVENT_BEAT_GENTLEMAN_EDWARD
				{1130, 1130},  // EVENT_BEAT_GENTLEMAN_GREGORY
				{1131, 1131},  // EVENT_BEAT_GENTLEMAN_ALFRED
				{1132, 1132},  // EVENT_BEAT_GENTLEMAN_MILTON
				{1133, 1133},  // EVENT_BEAT_GENTLEMAN_CAMUS
				{1134, 1134},  // EVENT_BEAT_GENTLEMAN_GEOFFREY
				{1135, 1135},  // EVENT_BEAT_RICH_BOY_WINSTON
				{1136, 1136},  // EVENT_BEAT_RICH_BOY_GERALD
				{1137, 1137},  // EVENT_BEAT_RICH_BOY_IRVING
				{1138, 1138},  // EVENT_BEAT_LADY_JESSICA
				{1139, 1139},  // EVENT_BEAT_BREEDER_JULIE
				{1140, 1140},  // EVENT_BEAT_BREEDER_THERESA
				{1141, 1141},  // EVENT_BEAT_BREEDER_JODY
				{1142, 1142},  // EVENT_BEAT_BREEDER_CARLENE
				{1143, 1143},  // EVENT_BEAT_BREEDER_SOPHIE
				{1144, 1144},  // EVENT_BEAT_BREEDER_BRENDA
				{1145, 1145},  // EVENT_BEAT_BAKER_CHELSIE
				{1146, 1146},  // EVENT_BEAT_BAKER_SHARYN
				{1147, 1147},  // EVENT_BEAT_BAKER_MARGARET
				{1148, 1148},  // EVENT_BEAT_BAKER_OLGA
				{1149, 1149},  // EVENT_BEAT_COWGIRL_ANNIE
				{1150, 1150},  // EVENT_BEAT_COWGIRL_APRIL
				{1151, 1151},  // EVENT_BEAT_COWGIRL_DANIELA
				{1152, 1152},  // EVENT_BEAT_SAILOR_HUEY
				{1153, 1153},  // EVENT_BEAT_SAILOR_EUGENE
				{1154, 1154},  // EVENT_BEAT_SAILOR_TERRELL
				{1155, 1155},  // EVENT_BEAT_SAILOR_KENT
				{1156, 1156},  // EVENT_BEAT_SAILOR_ERNEST
				{1157, 1157},  // EVENT_BEAT_SAILOR_JEFF
				{1158, 1158},  // EVENT_BEAT_SAILOR_GARRETT
				{1159, 1159},  // EVENT_BEAT_SAILOR_KENNETH
				{1160, 1160},  // EVENT_BEAT_SAILOR_STANLY
				{1161, 1161},  // EVENT_BEAT_SAILOR_HARRY
				{1162, 1162},  // EVENT_BEAT_SAILOR_PARKER
				{1163, 1163},  // EVENT_BEAT_SAILOR_EDDIE
				{1164, 1164},  // EVENT_BEAT_SAILOR_HARVEY
				{1165, 1165},  // EVENT_BEAT_SWIMMERM_HAROLD
				{1166, 1166},  // EVENT_BEAT_SWIMMERM_SIMON
				{1167, 1167},  // EVENT_BEAT_SWIMMERM_RANDALL
				{1168, 1168},  // EVENT_BEAT_SWIMMERM_CHARLIE
				{1169, 1169},  // EVENT_BEAT_SWIMMERM_GEORGE
				{1170, 1170},  // EVENT_BEAT_SWIMMERM_BERKE
				{1171, 1171},  // EVENT_BEAT_SWIMMERM_KIRK
				{1172, 1172},  // EVENT_BEAT_SWIMMERM_MATHEW
				{1173, 1173},  // EVENT_BEAT_SWIMMERM_HAL
				{1174, 1174},  // EVENT_BEAT_SWIMMERM_JEROME
				{1175, 1175},  // EVENT_BEAT_SWIMMERM_TUCKER
				{1176, 1176},  // EVENT_BEAT_SWIMMERM_RICK
				{1177, 1177},  // EVENT_BEAT_SWIMMERM_CAMERON
				{1178, 1178},  // EVENT_BEAT_SWIMMERM_SETH
				{1179, 1179},  // EVENT_BEAT_SWIMMERM_JAMES
				{1180, 1180},  // EVENT_BEAT_SWIMMERM_WALTER
				{1181, 1181},  // EVENT_BEAT_SWIMMERM_LEWIS
				{1182, 1182},  // EVENT_BEAT_SWIMMERM_MICHEL
				{1183, 1183},  // EVENT_BEAT_SWIMMERM_LUCAS
				{1184, 1184},  // EVENT_BEAT_SWIMMERM_FRANK
				{1185, 1185},  // EVENT_BEAT_SWIMMERM_NADAR
				{1186, 1186},  // EVENT_BEAT_SWIMMERM_CONRAD
				{1187, 1187},  // EVENT_BEAT_SWIMMERM_ROMEO
				{1188, 1188},  // EVENT_BEAT_SWIMMERM_MALCOLM
				{1189, 1189},  // EVENT_BEAT_SWIMMERM_ARMAND
				{1190, 1190},  // EVENT_BEAT_SWIMMERM_THOMAS
				{1191, 1191},  // EVENT_BEAT_SWIMMERM_LUIS
				{1192, 1192},  // EVENT_BEAT_SWIMMERM_ELMO
				{1193, 1193},  // EVENT_BEAT_SWIMMERM_DUANE
				{1194, 1194},  // EVENT_BEAT_SWIMMERM_ESTEBAN
				{1195, 1195},  // EVENT_BEAT_SWIMMERM_EZRA
				{1196, 1196},  // EVENT_BEAT_SWIMMERM_ASHE
				{1197, 1197},  // EVENT_BEAT_SWIMMERF_ELAINE
				{1198, 1198},  // EVENT_BEAT_SWIMMERF_PAULA
				{1199, 1199},  // EVENT_BEAT_SWIMMERF_KAYLEE
				{1200, 1200},  // EVENT_BEAT_SWIMMERF_SUSIE
				{1201, 1201},  // EVENT_BEAT_SWIMMERF_DENISE
				{1202, 1202},  // EVENT_BEAT_SWIMMERF_KARA
				{1203, 1203},  // EVENT_BEAT_SWIMMERF_WENDY
				{1204, 1204},  // EVENT_BEAT_SWIMMERF_MARY
				{1205, 1205},  // EVENT_BEAT_SWIMMERF_DAWN
				{1206, 1206},  // EVENT_BEAT_SWIMMERF_NICOLE
				{1207, 1207},  // EVENT_BEAT_SWIMMERF_LORI
				{1208, 1208},  // EVENT_BEAT_SWIMMERF_NIKKI
				{1209, 1209},  // EVENT_BEAT_SWIMMERF_DIANA
				{1210, 1210},  // EVENT_BEAT_SWIMMERF_BRIANA
				{1211, 1211},  // EVENT_BEAT_SWIMMERF_VIOLA
				{1212, 1212},  // EVENT_BEAT_SWIMMERF_KATIE
				{1213, 1213},  // EVENT_BEAT_SWIMMERF_JILL
				{1214, 1214},  // EVENT_BEAT_SWIMMERF_LISA
				{1215, 1215},  // EVENT_BEAT_SWIMMERF_ALISON
				{1216, 1216},  // EVENT_BEAT_SWIMMERF_STEPHANIE
				{1217, 1217},  // EVENT_BEAT_SWIMMERF_CAROLINE
				{1218, 1218},  // EVENT_BEAT_SWIMMERF_NATALIA
				{1219, 1219},  // EVENT_BEAT_SWIMMERF_BARBARA
				{1220, 1220},  // EVENT_BEAT_SWIMMERF_SALLY
				{1221, 1221},  // EVENT_BEAT_SWIMMERF_TARA
				{1222, 1222},  // EVENT_BEAT_SWIMMERF_MAYU
				{1223, 1223},  // EVENT_BEAT_SWIMMERF_LEONA
				{1224, 1224},  // EVENT_BEAT_SWIMMERF_CHELAN
				{1225, 1225},  // EVENT_BEAT_SWIMMERF_KENDRA
				{1226, 1226},  // EVENT_BEAT_SWIMMERF_WODA
				{1227, 1227},  // EVENT_BEAT_SWIMMERF_RACHEL
				{1228, 1228},  // EVENT_BEAT_SWIMMERF_MARINA
				{1229, 1229},  // EVENT_BEAT_BURGLAR_DUNCAN
				{1230, 1230},  // EVENT_BEAT_BURGLAR_ORSON
				{1231, 1231},  // EVENT_BEAT_BURGLAR_COREY
				{1232, 1232},  // EVENT_BEAT_BURGLAR_PETE
				{1233, 1233},  // EVENT_BEAT_BURGLAR_LOUIS
				{1234, 1234},  // EVENT_BEAT_PI_LOOKER
				{1235, 1235},  // EVENT_BEAT_SCIENTIST_LOWELL
				{1236, 1236},  // EVENT_BEAT_SCIENTIST_DENNETT
				{1237, 1237},  // EVENT_BEAT_SCIENTIST_LINDEN
				{1238, 1238},  // EVENT_BEAT_SCIENTIST_OSKAR
				{1239, 1239},  // EVENT_BEAT_SCIENTIST_BRAYDON
				{1240, 1240},  // EVENT_BEAT_SCIENTIST_CARL
				{1241, 1241},  // EVENT_BEAT_SCIENTIST_DEXTER
				{1242, 1242},  // EVENT_BEAT_SCIENTIST_JOSEPH
				{1243, 1243},  // EVENT_BEAT_SCIENTIST_NIGEL
				{1244, 1244},  // EVENT_BEAT_SCIENTIST_PIOTR
				{1245, 1245},  // EVENT_BEAT_ROCKET_SCIENTIST_ROSS
				{1246, 1246},  // EVENT_BEAT_ROCKET_SCIENTIST_MITCH
				{1247, 1247},  // EVENT_BEAT_ROCKET_SCIENTIST_JED
				{1248, 1248},  // EVENT_BEAT_ROCKET_SCIENTIST_MARC
				{1249, 1249},  // EVENT_BEAT_ROCKET_SCIENTIST_RICH
				{1250, 1250},  // EVENT_BEAT_BOARDER_RONALD
				{1251, 1251},  // EVENT_BEAT_BOARDER_BRAD
				{1252, 1252},  // EVENT_BEAT_BOARDER_DOUGLAS
				{1253, 1253},  // EVENT_BEAT_BOARDER_SHAUN
				{1254, 1254},  // EVENT_BEAT_BOARDER_BRYCE
				{1255, 1255},  // EVENT_BEAT_BOARDER_STEFAN
				{1256, 1256},  // EVENT_BEAT_BOARDER_MAX
				{1257, 1257},  // EVENT_BEAT_SKIER_ROXANNE
				{1258, 1258},  // EVENT_BEAT_SKIER_CLARISSA
				{1259, 1259},  // EVENT_BEAT_SKIER_CADY
				{1260, 1260},  // EVENT_BEAT_SKIER_MARIA
				{1261, 1261},  // EVENT_BEAT_SKIER_BECKY
				{1262, 1262},  // EVENT_BEAT_BLACKBELT_KENJI
				{1263, 1263},  // EVENT_BEAT_BLACKBELT_YOSHI
				{1264, 1264},  // EVENT_BEAT_BLACKBELT_LAO
				{1265, 1265},  // EVENT_BEAT_BLACKBELT_NOB
				{1266, 1266},  // EVENT_BEAT_BLACKBELT_LUNG
				{1267, 1267},  // EVENT_BEAT_BLACKBELT_WAI
				{1268, 1268},  // EVENT_BEAT_BLACKBELT_INIGO
				{1269, 1269},  // EVENT_BEAT_BLACKBELT_MANFORD
				{1270, 1270},  // EVENT_BEAT_BLACKBELT_ANDER
				{1271, 1271},  // EVENT_BEAT_BLACKBELT_TAKEO
				{1272, 1272},  // EVENT_BEAT_BATTLE_GIRL_SUBARU
				{1273, 1273},  // EVENT_BEAT_BATTLE_GIRL_DIANE
				{1274, 1274},  // EVENT_BEAT_BATTLE_GIRL_KAGAMI
				{1275, 1275},  // EVENT_BEAT_BATTLE_GIRL_NOZOMI
				{1276, 1276},  // EVENT_BEAT_BATTLE_GIRL_RONDA
				{1277, 1277},  // EVENT_BEAT_BATTLE_GIRL_PADMA
				{1278, 1278},  // EVENT_BEAT_BATTLE_GIRL_EMY
				{1279, 1279},  // EVENT_BEAT_DRAGON_TAMER_PAUL
				{1280, 1280},  // EVENT_BEAT_DRAGON_TAMER_DARIN
				{1281, 1281},  // EVENT_BEAT_DRAGON_TAMER_ADAM
				{1282, 1282},  // EVENT_BEAT_DRAGON_TAMER_ERICK
				{1283, 1283},  // EVENT_BEAT_DRAGON_TAMER_KAZU
				{1284, 1284},  // EVENT_BEAT_DRAGON_TAMER_AEGON
				{1285, 1285},  // EVENT_BEAT_ENGINEER_SMITH
				{1286, 1286},  // EVENT_BEAT_ENGINEER_BERNIE
				{1287, 1287},  // EVENT_BEAT_ENGINEER_CAMDEN
				{1288, 1288},  // EVENT_BEAT_ENGINEER_LANG
				{1289, 1289},  // EVENT_BEAT_ENGINEER_HUGO
				{1290, 1290},  // EVENT_BEAT_ENGINEER_HOWARD
				{1291, 1291},  // EVENT_BEAT_TEACHER_COLETTE
				{1292, 1292},  // EVENT_BEAT_TEACHER_HILLARY
				{1293, 1293},  // EVENT_BEAT_TEACHER_SHIRLEY
				{1294, 1294},  // EVENT_BEAT_TEACHER_KATHRYN
				{1295, 1295},  // EVENT_BEAT_TEACHER_CLARICE
				{1296, 1296},  // EVENT_BEAT_GUITARISTM_CLYDE
				{1297, 1297},  // EVENT_BEAT_GUITARISTM_VINCENT
				{1298, 1298},  // EVENT_BEAT_GUITARISTM_ROGER
				{1299, 1299},  // EVENT_BEAT_GUITARISTM_EZEKIEL
				{1300, 1300},  // EVENT_BEAT_GUITARISTM_BIFF
				{1301, 1301},  // EVENT_BEAT_GUITARISTM_GEDDY
				{1302, 1302},  // EVENT_BEAT_GUITARISTF_JANET
				{1303, 1303},  // EVENT_BEAT_GUITARISTF_MORGAN
				{1304, 1304},  // EVENT_BEAT_GUITARISTF_RITSUKO
				{1305, 1305},  // EVENT_BEAT_GUITARISTF_WANDA
				{1306, 1306},  // EVENT_BEAT_GUITARISTF_JACLYN
				{1307, 1307},  // EVENT_BEAT_BIKER_DWAYNE
				{1308, 1308},  // EVENT_BEAT_BIKER_HARRIS
				{1309, 1309},  // EVENT_BEAT_BIKER_ZEKE
				{1310, 1310},  // EVENT_BEAT_BIKER_CHARLES
				{1311, 1311},  // EVENT_BEAT_BIKER_REILLY
				{1312, 1312},  // EVENT_BEAT_BIKER_JOEL
				{1313, 1313},  // EVENT_BEAT_BIKER_GLENN
				{1314, 1314},  // EVENT_BEAT_BIKER_DALE
				{1315, 1315},  // EVENT_BEAT_BIKER_JACOB
				{1316, 1316},  // EVENT_BEAT_BIKER_AIDEN
				{1317, 1317},  // EVENT_BEAT_BIKER_DAN
				{1318, 1318},  // EVENT_BEAT_BIKER_TEDDY
				{1319, 1319},  // EVENT_BEAT_BIKER_TYRONE
				{1320, 1320},  // EVENT_BEAT_ROUGHNECK_BRIAN
				{1321, 1321},  // EVENT_BEAT_ROUGHNECK_THERON
				{1322, 1322},  // EVENT_BEAT_ROUGHNECK_MARKEY
				{1323, 1323},  // EVENT_BEAT_TAMER_BRETT
				{1324, 1324},  // EVENT_BEAT_TAMER_VINCE
				{1325, 1325},  // EVENT_BEAT_TAMER_OSWALD
				{1326, 1326},  // EVENT_BEAT_TAMER_JORDAN
				{1327, 1327},  // EVENT_BEAT_ARTIST_REINA
				{1328, 1328},  // EVENT_BEAT_ARTIST_ALINA
				{1329, 1329},  // EVENT_BEAT_ARTIST_MARLENE
				{1330, 1330},  // EVENT_BEAT_ARTIST_RIN
				{1331, 1331},  // EVENT_BEAT_AROMA_LADY_DAHLIA
				{1332, 1332},  // EVENT_BEAT_AROMA_LADY_BRYONY
				{1333, 1333},  // EVENT_BEAT_AROMA_LADY_HEATHER
				{1334, 1334},  // EVENT_BEAT_AROMA_LADY_HOLLY
				{1335, 1335},  // EVENT_BEAT_AROMA_LADY_PEONY
				{1336, 1336},  // EVENT_BEAT_SIGHTSEERM_JASKA
				{1337, 1337},  // EVENT_BEAT_SIGHTSEERM_BLAISE
				{1338, 1338},  // EVENT_BEAT_SIGHTSEERM_GARETH
				{1339, 1339},  // EVENT_BEAT_SIGHTSEERM_CHESTER
				{1340, 1340},  // EVENT_BEAT_SIGHTSEERM_HARI
				{1341, 1341},  // EVENT_BEAT_SIGHTSEERF_ROSIE
				{1342, 1342},  // EVENT_BEAT_SIGHTSEERF_KAMILA
				{1343, 1343},  // EVENT_BEAT_SIGHTSEERF_NOELLE
				{1344, 1344},  // EVENT_BEAT_SIGHTSEERF_PILAR
				{1345, 1345},  // EVENT_BEAT_SIGHTSEERF_LENIE
				{1346, 1346},  // EVENT_BEAT_SIGHTSEERS_LI_AND_SU
				{1347, 1347},  // EVENT_BEAT_SIGHTSEERS_CY_AND_VI
				{1348, 1348},  // EVENT_BEAT_COOLTRAINERM_GAVEN
				{1349, 1349},  // EVENT_BEAT_COOLTRAINERM_NICK
				{1350, 1350},  // EVENT_BEAT_COOLTRAINERM_AARON
				{1351, 1351},  // EVENT_BEAT_COOLTRAINERM_CODY
				{1352, 1352},  // EVENT_BEAT_COOLTRAINERM_MIKE
				{1353, 1353},  // EVENT_BEAT_COOLTRAINERM_RYAN
				{1354, 1354},  // EVENT_BEAT_COOLTRAINERM_BLAKE
				{1355, 1355},  // EVENT_BEAT_COOLTRAINERM_ANDY
				{1356, 1356},  // EVENT_BEAT_COOLTRAINERM_SEAN
				{1357, 1357},  // EVENT_BEAT_COOLTRAINERM_KEVIN
				{1358, 1358},  // EVENT_BEAT_COOLTRAINERM_ALLEN
				{1359, 1359},  // EVENT_BEAT_COOLTRAINERM_FRENCH
				{1360, 1360},  // EVENT_BEAT_COOLTRAINERM_HENRI
				{1361, 1361},  // EVENT_BEAT_COOLTRAINERM_CONNOR
				{1362, 1362},  // EVENT_BEAT_COOLTRAINERM_KIERAN
				{1363, 1363},  // EVENT_BEAT_COOLTRAINERM_FINCH
				{1364, 1364},  // EVENT_BEAT_COOLTRAINERM_PETRIE
				{1365, 1365},  // EVENT_BEAT_COOLTRAINERM_COREY
				{1366, 1366},  // EVENT_BEAT_COOLTRAINERM_RAYMOND
				{1367, 1367},  // EVENT_BEAT_COOLTRAINERM_FERGUS
				{1368, 1368},  // EVENT_BEAT_COOLTRAINERF_BETH
				{1369, 1369},  // EVENT_BEAT_COOLTRAINERF_REENA
				{1370, 1370},  // EVENT_BEAT_COOLTRAINERF_GWEN
				{1371, 1371},  // EVENT_BEAT_COOLTRAINERF_LOIS
				{1372, 1372},  // EVENT_BEAT_COOLTRAINERF_FRAN
				{1373, 1373},  // EVENT_BEAT_COOLTRAINERF_LOLA
				{1374, 1374},  // EVENT_BEAT_COOLTRAINERF_KATE
				{1375, 1375},  // EVENT_BEAT_COOLTRAINERF_IRENE
				{1376, 1376},  // EVENT_BEAT_COOLTRAINERF_KELLY
				{1377, 1377},  // EVENT_BEAT_COOLTRAINERF_JOYCE
				{1378, 1378},  // EVENT_BEAT_COOLTRAINERF_MEGAN
				{1379, 1379},  // EVENT_BEAT_COOLTRAINERF_CAROL
				{1380, 1380},  // EVENT_BEAT_COOLTRAINERF_QUINN
				{1381, 1381},  // EVENT_BEAT_COOLTRAINERF_EMMA
				{1382, 1382},  // EVENT_BEAT_COOLTRAINERF_CYBIL
				{1383, 1383},  // EVENT_BEAT_COOLTRAINERF_JENN
				{1384, 1384},  // EVENT_BEAT_COOLTRAINERF_SALMA
				{1385, 1385},  // EVENT_BEAT_COOLTRAINERF_BONITA
				{1386, 1386},  // EVENT_BEAT_COOLTRAINERF_SERA
				{1387, 1387},  // EVENT_BEAT_COOLTRAINERF_NEESHA
				{1388, 1388},  // EVENT_BEAT_COOLTRAINERF_CHIARA
				{1389, 1389},  // EVENT_BEAT_ACE_DUO_ELAN_AND_IDA
				{1390, 1390},  // EVENT_BEAT_ACE_DUO_ARA_AND_BELA
				{1391, 1391},  // EVENT_BEAT_ACE_DUO_THOM_AND_KAE
				{1392, 1392},  // EVENT_BEAT_ACE_DUO_ZAC_AND_JEN
				{1393, 1393},  // EVENT_BEAT_ACE_DUO_JAKE_AND_BRI
				{1394, 1394},  // EVENT_BEAT_ACE_DUO_DAN_AND_CARA
				{1395, 1395},  // EVENT_BEAT_VETERANM_MATT
				{1396, 1396},  // EVENT_BEAT_VETERANM_REMY
				{1397, 1397},  // EVENT_BEAT_VETERANM_BARKHORN
				{1398, 1398},  // EVENT_BEAT_VETERANF_JOANNE
				{1399, 1399},  // EVENT_BEAT_VETERANF_JONET
				{1401, 1401},  // EVENT_BEAT_VETERANF_LITVYAK
				{1402, 1402},  // EVENT_BEAT_PROTON_1
				{1403, 1403},  // EVENT_BEAT_PROTON_2
				{1404, 1404},  // EVENT_BEAT_PETREL_1
				{1405, 1405},  // EVENT_BEAT_PETREL_2
				{1406, 1406},  // EVENT_BEAT_ARCHER_1
				{1407, 1407},  // EVENT_BEAT_ARCHER_2
				{1408, 1408},  // EVENT_BEAT_ARIANA_1
				{1409, 1409},  // EVENT_BEAT_ARIANA_2
				{1410, 1410},  // EVENT_BEAT_PROF_OAK
				{1411, 1411},  // EVENT_BATTLED_PROF_ELM
				{1412, 1412},  // EVENT_BEAT_PROF_IVY
				{1413, 1413},  // EVENT_BEAT_EUSINE
				{1414, 1414},  // EVENT_BEAT_KIYO
				{1415, 1415},  // EVENT_BEAT_PALMER
				{1416, 1416},  // EVENT_BEAT_JESSIE_AND_JAMES
				{1417, 1417},  // EVENT_BEAT_LORELEI
				{1418, 1418},  // EVENT_BEAT_LORELEI_AGAIN
				{1419, 1419},  // EVENT_BEAT_AGATHA
				{1420, 1420},  // EVENT_BEAT_STEVEN
				{1421, 1421},  // EVENT_BEAT_CYNTHIA
				{1422, 1422},  // EVENT_BEAT_CHERYL
				{1423, 1423},  // EVENT_BEAT_RILEY
				{1424, 1424},  // EVENT_BEAT_BUCK
				{1425, 1425},  // EVENT_BEAT_MARLEY
				{1426, 1426},  // EVENT_BEAT_MIRA
				{1427, 1427},  // EVENT_BEAT_ANABEL
				{1428, 1428},  // EVENT_BEAT_DARACH
				{1429, 1429},  // EVENT_BEAT_CAITLIN
				{1430, 1430},  // EVENT_BEAT_CANDELA
				{1431, 1431},  // EVENT_BEAT_BLANCHE
				{1432, 1432},  // EVENT_BEAT_SPARK
				{1433, 1433},  // EVENT_BEAT_FLANNERY
				{1434, 1434},  // EVENT_BEAT_MAYLENE
				{1435, 1435},  // EVENT_BEAT_MARLON
				{1436, 1436},  // EVENT_BEAT_MARLON_AGAIN
				{1437, 1437},  // EVENT_BEAT_VALERIE
				{1438, 1438},  // EVENT_BEAT_KUKUI
				{1439, 1439},  // EVENT_BEAT_PIERS
				{1440, 1440},  // EVENT_BEAT_PIERS_AGAIN
				{1443, 1443},  // EVENT_BEAT_VICTOR
				{1444, 1444},  // EVENT_BEAT_POKEMANIAC_BILL
				{1445, 1445},  // EVENT_BEAT_YELLOW
				{1446, 1446},  // EVENT_BEAT_WALKER
				{1447, 1447},  // EVENT_BEAT_IMAKUNI
				{1448, 1448},  // EVENT_BEAT_LAWRENCE
				{1449, 1449},  // EVENT_BEAT_LAWRENCE_AGAIN
				{1450, 1450},  // EVENT_CYNDAQUIL_POKEBALL_IN_ELMS_LAB
				{1451, 1451},  // EVENT_TOTODILE_POKEBALL_IN_ELMS_LAB
				{1452, 1452},  // EVENT_CHIKORITA_POKEBALL_IN_ELMS_LAB
				{1453, 1453},  // EVENT_VIOLET_CITY_PP_UP
				{1454, 1454},  // EVENT_VIOLET_CITY_RARE_CANDY
				{1455, 1455},  // EVENT_LAKE_OF_RAGE_ELIXIR
				{1456, 1456},  // EVENT_LAKE_OF_RAGE_MAX_REVIVE
				{1457, 1457},  // EVENT_LAKE_OF_RAGE_TM_SUBSTITUTE
				{1458, 1458},  // EVENT_SPROUT_TOWER1F_PARALYZEHEAL
				{1459, 1459},  // EVENT_SPROUT_TOWER2F_X_ACCURACY
				{1460, 1460},  // EVENT_SPROUT_TOWER_3F_POTION
				{1461, 1461},  // EVENT_SPROUT_TOWER_3F_ESCAPE_ROPE
				{1462, 1462},  // EVENT_TIN_TOWER_3F_FULL_HEAL
				{1463, 1463},  // EVENT_TIN_TOWER_4F_ULTRA_BALL
				{1464, 1464},  // EVENT_TIN_TOWER_4F_PP_UP
				{1465, 1465},  // EVENT_TIN_TOWER_4F_ESCAPE_ROPE
				{1466, 1466},  // EVENT_TIN_TOWER_5F_RARE_CANDY
				{1467, 1467},  // EVENT_TIN_TOWER_7F_MAX_REVIVE
				{1468, 1468},  // EVENT_TIN_TOWER_8F_BIG_NUGGET
				{1469, 1469},  // EVENT_TIN_TOWER_8F_MAX_ELIXIR
				{1470, 1470},  // EVENT_TIN_TOWER_8F_FULL_RESTORE
				{1471, 1471},  // EVENT_TEAM_ROCKET_BASE_B3F_ULTRA_BALL
				{1472, 1472},  // EVENT_UNDERGROUND_WAREHOUSE_ULTRA_BALL
				{1473, 1473},  // EVENT_BURNED_TOWER_1F_HP_UP
				{1474, 1474},  // EVENT_BURNED_TOWER_B1F_TM_FLAME_CHARGE
				{1475, 1475},  // EVENT_NATIONAL_PARK_SHINY_STONE
				{1476, 1476},  // EVENT_NATIONAL_PARK_TM_DIG
				{1477, 1477},  // EVENT_UNION_CAVE_1F_GREAT_BALL
				{1478, 1478},  // EVENT_UNION_CAVE_1F_X_ATTACK
				{1479, 1479},  // EVENT_UNION_CAVE_1F_POTION
				{1480, 1480},  // EVENT_UNION_CAVE_1F_AWAKENING
				{1481, 1481},  // EVENT_UNION_CAVE_B1F_NORTH_TM_SWIFT
				{1482, 1482},  // EVENT_UNION_CAVE_B1F_NORTH_X_DEFEND
				{1483, 1483},  // EVENT_UNION_CAVE_B2F_ELIXIR
				{1484, 1484},  // EVENT_UNION_CAVE_B2F_HYPER_POTION
				{1485, 1485},  // EVENT_SLOWPOKE_WELL_B1F_SUPER_POTION
				{1486, 1486},  // EVENT_SLOWPOKE_WELL_B2F_DAMP_ROCK
				{1487, 1487},  // EVENT_OLIVINE_LIGHTHOUSE_3F_ETHER
				{1488, 1488},  // EVENT_OLIVINE_LIGHTHOUSE_5F_RARE_CANDY
				{1489, 1489},  // EVENT_OLIVINE_LIGHTHOUSE_5F_SUPER_REPEL
				{1490, 1490},  // EVENT_OLIVINE_LIGHTHOUSE_5F_TM_ENERGY_BALL
				{1491, 1491},  // EVENT_OLIVINE_LIGHTHOUSE_6F_WIDE_LENS
				{1492, 1492},  // EVENT_ROUTE_41_SILVER_LEAF
				{1493, 1493},  // EVENT_TEAM_ROCKET_BASE_B1F_HYPER_POTION
				{1494, 1494},  // EVENT_TEAM_ROCKET_BASE_B1F_NUGGET
				{1495, 1495},  // EVENT_TEAM_ROCKET_BASE_B1F_GUARD_SPEC
				{1496, 1496},  // EVENT_TEAM_ROCKET_BASE_B2F_HYPER_POTION
				{1497, 1497},  // EVENT_TEAM_ROCKET_BASE_B3F_PROTEIN
				{1498, 1498},  // EVENT_TEAM_ROCKET_BASE_B3F_X_SP_DEF
				{1499, 1499},  // EVENT_TEAM_ROCKET_BASE_B3F_FULL_HEAL
				{1500, 1500},  // EVENT_TEAM_ROCKET_BASE_B3F_ICE_HEAL
				{1501, 1501},  // EVENT_ILEX_FOREST_REVIVE
				{1502, 1502},  // EVENT_WAREHOUSE_ENTRANCE_COIN_CASE
				{1503, 1503},  // EVENT_UNDERGROUND_PATH_SWITCH_ROOM_ENTRANCES_SMOKE_BALL
				{1504, 1504},  // EVENT_UNDERGROUND_PATH_SWITCH_ROOM_ENTRANCES_FULL_HEAL
				{1505, 1505},  // EVENT_GOLDENROD_DEPT_STORE_B1F_ETHER
				{1506, 1506},  // EVENT_GOLDENROD_DEPT_STORE_B1F_METAL_COAT
				{1507, 1507},  // EVENT_GOLDENROD_DEPT_STORE_B1F_BURN_HEAL
				{1508, 1508},  // EVENT_GOLDENROD_DEPT_STORE_B1F_ULTRA_BALL
				{1509, 1509},  // EVENT_UNDERGROUND_WAREHOUSE_MAX_ETHER
				{1510, 1510},  // EVENT_UNDERGROUND_WAREHOUSE_TM_X_SCISSOR
				{1511, 1511},  // EVENT_MOUNT_MORTAR_1F_OUTSIDE_ETHER
				{1512, 1512},  // EVENT_MOUNT_MORTAR_1F_OUTSIDE_REVIVE
				{1513, 1513},  // EVENT_MOUNT_MORTAR_1F_INSIDE_SMOOTH_ROCK
				{1514, 1514},  // EVENT_MOUNT_MORTAR_1F_INSIDE_MAX_REVIVE
				{1515, 1515},  // EVENT_MOUNT_MORTAR_1F_INSIDE_HYPER_POTION
				{1516, 1516},  // EVENT_MOUNT_MORTAR_2F_INSIDE_MAX_POTION
				{1517, 1517},  // EVENT_MOUNT_MORTAR_2F_INSIDE_RARE_CANDY
				{1518, 1518},  // EVENT_MOUNT_MORTAR_2F_INSIDE_TM_AERIAL_ACE
				{1519, 1519},  // EVENT_MOUNT_MORTAR_2F_INSIDE_DRAGON_SCALE
				{1520, 1520},  // EVENT_MOUNT_MORTAR_2F_INSIDE_ELIXIR
				{1521, 1521},  // EVENT_MOUNT_MORTAR_2F_INSIDE_ESCAPE_ROPE
				{1522, 1522},  // EVENT_MOUNT_MORTAR_B1F_HYPER_POTION
				{1523, 1523},  // EVENT_MOUNT_MORTAR_B1F_CARBOS
				{1524, 1524},  // EVENT_ICE_PATH_1F_PP_UP
				{1525, 1525},  // EVENT_ICE_PATH_B1F_IRON
				{1526, 1526},  // EVENT_ICE_PATH_B2F_MAHOGANY_SIDE_ICE_STONE
				{1527, 1527},  // EVENT_ICE_PATH_B2F_MAHOGANY_SIDE_MAX_POTION
				{1528, 1528},  // EVENT_ICE_PATH_B2F_BLACKTHORN_SIDE_NUGGET
				{1529, 1529},  // EVENT_ICE_PATH_B3F_NEVERMELTICE
				{1530, 1530},  // EVENT_WHIRL_ISLAND_NE_ULTRA_BALL
				{1531, 1531},  // EVENT_WHIRL_ISLAND_SW_ULTRA_BALL
				{1532, 1532},  // EVENT_WHIRL_ISLAND_B1F_FULL_RESTORE
				{1533, 1533},  // EVENT_WHIRL_ISLAND_B1F_CARBOS
				{1534, 1534},  // EVENT_WHIRL_ISLAND_B1F_CALCIUM
				{1535, 1535},  // EVENT_WHIRL_ISLAND_B1F_BIG_NUGGET
				{1536, 1536},  // EVENT_WHIRL_ISLAND_B1F_ESCAPE_ROPE
				{1537, 1537},  // EVENT_WHIRL_ISLAND_B2F_FULL_RESTORE
				{1538, 1538},  // EVENT_WHIRL_ISLAND_B2F_MAX_REVIVE
				{1539, 1539},  // EVENT_WHIRL_ISLAND_B2F_MAX_ELIXIR
				{1540, 1540},  // EVENT_SILVER_CAVE_ROOM_1_MAX_ELIXIR
				{1541, 1541},  // EVENT_SILVER_CAVE_ROOM_1_PROTEIN
				{1542, 1542},  // EVENT_SILVER_CAVE_ROOM_1_ESCAPE_ROPE
				{1543, 1543},  // EVENT_SILVER_CAVE_ITEM_ROOMS_MAX_REVIVE
				{1544, 1544},  // EVENT_SILVER_CAVE_ITEM_ROOMS_FULL_RESTORE
				{1545, 1545},  // EVENT_DARK_CAVE_VIOLET_ENTRANCE_POTION
				{1546, 1546},  // EVENT_DARK_CAVE_VIOLET_ENTRANCE_DUSK_STONE
				{1547, 1547},  // EVENT_DARK_CAVE_VIOLET_ENTRANCE_HYPER_POTION
				{1548, 1548},  // EVENT_DARK_CAVE_BLACKTHORN_ENTRANCE_REVIVE
				{1549, 1549},  // EVENT_DARK_CAVE_BLACKTHORN_ENTRANCE_TM_DARK_PULSE
				{1550, 1550},  // EVENT_VICTORY_ROAD_1F_MAX_REVIVE
				{1551, 1551},  // EVENT_VICTORY_ROAD_1F_FULL_HEAL
				{1552, 1552},  // EVENT_VICTORY_ROAD_2F_TM_EARTHQUAKE
				{1553, 1553},  // EVENT_VICTORY_ROAD_2F_FULL_RESTORE
				{1554, 1554},  // EVENT_VICTORY_ROAD_2F_HP_UP
				{1555, 1555},  // EVENT_VICTORY_ROAD_3F_RAZOR_FANG
				{1556, 1556},  // EVENT_DRAGONS_DEN_B1F_DRAGON_FANG
				{1557, 1557},  // EVENT_TOHJO_FALLS_MOON_STONE
				{1558, 1558},  // EVENT_ROUTE_26_MAX_ELIXIR
				{1559, 1559},  // EVENT_ROUTE_26_TM_DRAGON_CLAW
				{1560, 1560},  // EVENT_ROUTE_27_RARE_CANDY
				{1561, 1561},  // EVENT_ROUTE_29_POTION
				{1562, 1562},  // EVENT_ROUTE_31_POTION
				{1563, 1563},  // EVENT_ROUTE_31_POKE_BALL
				{1564, 1564},  // EVENT_ROUTE_32_GREAT_BALL
				{1565, 1565},  // EVENT_ROUTE_32_REPEL
				{1566, 1566},  // EVENT_ROUTE_32_COAST_SOFT_SAND
				{1567, 1567},  // EVENT_ROUTE_32_COAST_WHITE_HERB
				{1568, 1568},  // EVENT_ROUTE_34_COAST_PEARL_STRING
				{1569, 1569},  // EVENT_ROUTE_35_TM_HONE_CLAWS
				{1570, 1570},  // EVENT_ROUTE_35_COAST_SOUTH_BIG_PEARL
				{1571, 1571},  // EVENT_ROUTE_39_TM_BULLDOZE
				{1572, 1572},  // EVENT_ROUTE_42_ULTRA_BALL
				{1573, 1573},  // EVENT_ROUTE_42_SUPER_POTION
				{1574, 1574},  // EVENT_ROUTE_43_MAX_ETHER
				{1575, 1575},  // EVENT_ROUTE_44_MAX_REVIVE
				{1576, 1576},  // EVENT_ROUTE_44_ULTRA_BALL
				{1577, 1577},  // EVENT_ROUTE_45_NUGGET
				{1578, 1578},  // EVENT_ROUTE_45_REVIVE
				{1579, 1579},  // EVENT_ROUTE_45_ELIXIR
				{1580, 1580},  // EVENT_ROUTE_45_MAX_POTION
				{1581, 1581},  // EVENT_ROUTE_46_X_SPEED
				{1582, 1582},  // EVENT_ROUTE_47_REVIVE
				{1583, 1583},  // EVENT_ROUTE_47_MYSTIC_WATER
				{1584, 1584},  // EVENT_ROUTE_47_QUICK_CLAW
				{1585, 1585},  // EVENT_ROUTE_47_MAX_REPEL
				{1586, 1586},  // EVENT_ROUTE_48_NUGGET
				{1587, 1587},  // EVENT_ROUTE_49_WHITE_HERB
				{1588, 1588},  // EVENT_ROUTE_49_CALCIUM
				{1589, 1589},  // EVENT_YELLOW_FOREST_TM_LEECH_LIFE
				{1590, 1590},  // EVENT_YELLOW_FOREST_MIRACLE_SEED
				{1591, 1591},  // EVENT_YELLOW_FOREST_BIG_ROOT
				{1592, 1592},  // EVENT_YELLOW_FOREST_LEMONADE
				{1593, 1593},  // EVENT_QUIET_CAVE_1F_NUGGET
				{1594, 1594},  // EVENT_QUIET_CAVE_1F_TWISTEDSPOON
				{1595, 1595},  // EVENT_QUIET_CAVE_1F_DUSK_STONE
				{1596, 1596},  // EVENT_QUIET_CAVE_1F_DUSK_BALL
				{1597, 1597},  // EVENT_QUIET_CAVE_B1F_BIG_PEARL
				{1598, 1598},  // EVENT_QUIET_CAVE_B1F_ELIXIR
				{1599, 1599},  // EVENT_QUIET_CAVE_B2F_DUSK_BALL
				{1600, 1600},  // EVENT_QUIET_CAVE_B2F_RAZOR_CLAW
				{1601, 1601},  // EVENT_QUIET_CAVE_B2F_SAFE_GOGGLES
				{1602, 1602},  // EVENT_QUIET_CAVE_B3F_TM_FOCUS_BLAST
				{1603, 1603},  // EVENT_CHERRYGROVE_BAY_SHINY_STONE
				{1604, 1604},  // EVENT_GOLDENROD_HARBOR_STAR_PIECE
				{1605, 1605},  // EVENT_STORMY_BEACH_ZINC
				{1606, 1606},  // EVENT_MURKY_SWAMP_FULL_HEAL
				{1607, 1607},  // EVENT_MURKY_SWAMP_BIG_MUSHROOM
				{1608, 1608},  // EVENT_MURKY_SWAMP_TOXIC_ORB
				{1609, 1609},  // EVENT_MURKY_SWAMP_MULCH
				{1610, 1610},  // EVENT_TEACHER_NEW_BARK_TOWN
				{1611, 1611},  // EVENT_LYRA_NEW_BARK_TOWN
				{1612, 1612},  // EVENT_LYRA_ROUTE_29
				{1613, 1613},  // EVENT_LYRA_ROUTE_32
				{1614, 1614},  // EVENT_LYRA_ROUTE_34
				{1615, 1615},  // EVENT_LYRA_DAYCARE
				{1616, 1616},  // EVENT_LYRA_ROUTE_42
				{1617, 1617},  // EVENT_GRANDPA_ROUTE_34
				{1618, 1618},  // EVENT_RIVAL_NEW_BARK_TOWN
				{1619, 1619},  // EVENT_RIVAL_CHERRYGROVE_CITY
				{1620, 1620},  // EVENT_RIVAL_AZALEA_TOWN
				{1621, 1621},  // EVENT_RIVAL_TEAM_ROCKET_BASE
				{1622, 1622},  // EVENT_RIVAL_UNDERGROUND_PATH
				{1623, 1623},  // EVENT_RIVAL_VICTORY_ROAD
				{1624, 1624},  // EVENT_RIVAL_OLIVINE_CITY
				{1625, 1625},  // EVENT_RIVAL_SPROUT_TOWER
				{1626, 1626},  // EVENT_RIVAL_BURNED_TOWER
				{1627, 1627},  // EVENT_RIVAL_DRAGONS_DEN
				{1628, 1628},  // EVENT_PLAYERS_HOUSE_MOM_1
				{1629, 1629},  // EVENT_PLAYERS_HOUSE_MOM_2
				{1630, 1630},  // EVENT_MR_POKEMONS_HOUSE_OAK
				{1631, 1631},  // EVENT_DARK_CAVE_FALKNER
				{1632, 1632},  // EVENT_DARK_CAVE_PIDGEOTTO
				{1633, 1633},  // EVENT_DARK_CAVE_URSARING
				{1634, 1634},  // EVENT_VIOLET_CITY_EARL
				{1635, 1635},  // EVENT_EARLS_ACADEMY_EARL
				{1636, 1636},  // EVENT_VIOLET_GYM_FALKNER
				{1637, 1637},  // EVENT_GOLDENROD_GYM_WHITNEY
				{1638, 1638},  // EVENT_GOLDENROD_CITY_ROCKET_SCOUT
				{1639, 1639},  // EVENT_GOLDENROD_CITY_ROCKET_TAKEOVER
				{1640, 1640},  // EVENT_RADIO_TOWER_ROCKET_TAKEOVER
				{1641, 1641},  // EVENT_GOLDENROD_CITY_CIVILIANS
				{1642, 1642},  // EVENT_RADIO_TOWER_CIVILIANS_AFTER
				{1643, 1643},  // EVENT_RADIO_TOWER_BLACKBELT_BLOCKS_STAIRS
				{1644, 1644},  // EVENT_RADIO_TOWER_DIRECTOR
				{1645, 1645},  // EVENT_RADIO_TOWER_PETREL
				{1646, 1646},  // EVENT_JUDGE_MACHINE_ENGINEER
				{1647, 1647},  // EVENT_OLIVINE_LIGHTHOUSE_JASMINE
				{1648, 1648},  // EVENT_OLIVINE_GYM_JASMINE
				{1649, 1649},  // EVENT_LAKE_OF_RAGE_LANCE
				{1650, 1650},  // EVENT_MAHOGANY_MART_LANCE_AND_DRAGONITE
				{1651, 1651},  // EVENT_TEAM_ROCKET_BASE_B2F_LANCE
				{1652, 1652},  // EVENT_TEAM_ROCKET_BASE_B3F_LANCE_PASSWORDS
				{1653, 1653},  // EVENT_DRAGONS_DEN_CLAIR
				{1654, 1654},  // EVENT_TEAM_ROCKET_BASE_SECURITY_GRUNTS
				{1655, 1655},  // EVENT_TEAM_ROCKET_BASE_POPULATION
				{1656, 1656},  // EVENT_TEAM_ROCKET_BASE_B3F_PETREL
				{1657, 1657},  // EVENT_ROUTE_43_GATE_ROCKETS
				{1658, 1658},  // EVENT_TEAM_ROCKET_BASE_B2F_ARIANA
				{1659, 1659},  // EVENT_TEAM_ROCKET_BASE_B2F_PETREL
				{1660, 1660},  // EVENT_TEAM_ROCKET_BASE_B2F_DRAGONITE
				{1661, 1661},  // EVENT_TEAM_ROCKET_BASE_B2F_ELECTRODE_1
				{1662, 1662},  // EVENT_TEAM_ROCKET_BASE_B2F_ELECTRODE_2
				{1663, 1663},  // EVENT_TEAM_ROCKET_BASE_B2F_ELECTRODE_3
				{1664, 1664},  // EVENT_BLACKTHORN_CITY_DRAGON_TAMER_BLOCKS_GYM
				{1665, 1665},  // EVENT_BLACKTHORN_CITY_DRAGON_TAMER_DOES_NOT_BLOCK_GYM
				{1666, 1666},  // EVENT_DAYCARE_MAN_IN_DAYCARE
				{1667, 1667},  // EVENT_DAYCARE_MAN_ON_ROUTE_34
				{1668, 1668},  // EVENT_DAYCARE_MON_1
				{1669, 1669},  // EVENT_DAYCARE_MON_2
				{1670, 1670},  // EVENT_ILEX_FOREST_FARFETCHD
				{1671, 1671},  // EVENT_ROUTE_34_ILEX_FOREST_GATE_TEACHER_BEHIND_COUNTER
				{1672, 1672},  // EVENT_ROUTE_34_ILEX_FOREST_GATE_LASS
				{1673, 1673},  // EVENT_ROUTE_34_ILEX_FOREST_GATE_TEACHER_IN_WALKWAY
				{1674, 1674},  // EVENT_ILEX_FOREST_LASS
				{1675, 1675},  // EVENT_ILEX_FOREST_LYRA
				{1676, 1676},  // EVENT_ILEX_FOREST_CELEBI
				{1677, 1677},  // EVENT_COPYCAT_1
				{1678, 1678},  // EVENT_COPYCAT_2
				{1679, 1679},  // EVENT_GOLDENROD_SALE_OFF
				{1680, 1680},  // EVENT_GOLDENROD_SALE_ON
				{1681, 1681},  // EVENT_ILEX_FOREST_APPRENTICE
				{1682, 1682},  // EVENT_ILEX_FOREST_CHARCOAL_MASTER
				{1683, 1683},  // EVENT_CHARCOAL_KILN_FARFETCH_D
				{1684, 1684},  // EVENT_CHARCOAL_KILN_APPRENTICE
				{1685, 1685},  // EVENT_CHARCOAL_KILN_BOSS
				{1686, 1686},  // EVENT_ROUTE_36_SUDOWOODO
				{1687, 1687},  // EVENT_AZALEA_TOWN_SLOWPOKES
				{1688, 1688},  // EVENT_AZALEA_TOWN_SLOWPOKETAIL_ROCKET
				{1689, 1689},  // EVENT_SLOWPOKE_WELL_SLOWPOKES
				{1690, 1690},  // EVENT_SLOWPOKE_WELL_ROCKETS
				{1691, 1691},  // EVENT_KURTS_HOUSE_SLOWPOKE
				{1692, 1692},  // EVENT_GUIDE_GENT_IN_HIS_HOUSE
				{1693, 1693},  // EVENT_GUIDE_GENT_VISIBLE_IN_CHERRYGROVE
				{1694, 1694},  // EVENT_ELMS_AIDE_IN_VIOLET_POKEMON_CENTER
				{1695, 1695},  // EVENT_LYRA_IN_HER_ROOM
				{1696, 1696},  // EVENT_ELMS_AIDE_IN_LAB
				{1697, 1697},  // EVENT_LYRA_IN_ELMS_LAB
				{1698, 1698},  // EVENT_COP_IN_ELMS_LAB
				{1699, 1699},  // EVENT_RUINS_OF_ALPH_OUTSIDE_SCIENTIST
				{1700, 1700},  // EVENT_RUINS_OF_ALPH_OUTSIDE_SCIENTIST_CLIMAX
				{1701, 1701},  // EVENT_RUINS_OF_ALPH_RESEARCH_CENTER_SCIENTIST
				{1702, 1702},  // EVENT_RUINS_OF_ALPH_INNER_CHAMBER_TOURISTS
				{1703, 1703},  // EVENT_BOULDER_IN_BLACKTHORN_GYM_1
				{1704, 1704},  // EVENT_BOULDER_IN_BLACKTHORN_GYM_2
				{1705, 1705},  // EVENT_BOULDER_IN_BLACKTHORN_GYM_3
				{1706, 1706},  // EVENT_BOULDER_IN_ICE_PATH_1
				{1707, 1707},  // EVENT_BOULDER_IN_ICE_PATH_2
				{1708, 1708},  // EVENT_BOULDER_IN_ICE_PATH_3
				{1709, 1709},  // EVENT_BOULDER_IN_ICE_PATH_4
				{1710, 1710},  // EVENT_BOULDER_IN_ICE_PATH_1A
				{1711, 1711},  // EVENT_BOULDER_IN_ICE_PATH_2A
				{1712, 1712},  // EVENT_BOULDER_IN_ICE_PATH_3A
				{1713, 1713},  // EVENT_BOULDER_IN_ICE_PATH_4A
				{1714, 1714},  // EVENT_MAGNET_TUNNEL_LODESTONE_1
				{1715, 1715},  // EVENT_MAGNET_TUNNEL_LODESTONE_2
				{1716, 1716},  // EVENT_MAGNET_TUNNEL_LODESTONE_3
				{1717, 1717},  // EVENT_MAGNET_TUNNEL_LODESTONE_4
				{1718, 1718},  // EVENT_NEVER_MET_BILL
				{1719, 1719},  // EVENT_ECRUTEAK_POKE_CENTER_BILL
				{1720, 1720},  // EVENT_ROUTE_30_BATTLE
				{1721, 1721},  // EVENT_ROUTE_30_YOUNGSTER_JOEY
				{1722, 1722},  // EVENT_BUG_CATCHING_CONTESTANT_1A
				{1723, 1723},  // EVENT_BUG_CATCHING_CONTESTANT_2A
				{1724, 1724},  // EVENT_BUG_CATCHING_CONTESTANT_3A
				{1725, 1725},  // EVENT_BUG_CATCHING_CONTESTANT_4A
				{1726, 1726},  // EVENT_BUG_CATCHING_CONTESTANT_5A
				{1727, 1727},  // EVENT_BUG_CATCHING_CONTESTANT_6A
				{1728, 1728},  // EVENT_BUG_CATCHING_CONTESTANT_7A
				{1729, 1729},  // EVENT_BUG_CATCHING_CONTESTANT_8A
				{1730, 1730},  // EVENT_BUG_CATCHING_CONTESTANT_9A
				{1731, 1731},  // EVENT_BUG_CATCHING_CONTESTANT_10A
				{1732, 1732},  // EVENT_BUG_CATCHING_CONTESTANT_1B
				{1733, 1733},  // EVENT_BUG_CATCHING_CONTESTANT_2B
				{1734, 1734},  // EVENT_BUG_CATCHING_CONTESTANT_3B
				{1735, 1735},  // EVENT_BUG_CATCHING_CONTESTANT_4B
				{1736, 1736},  // EVENT_BUG_CATCHING_CONTESTANT_5B
				{1737, 1737},  // EVENT_BUG_CATCHING_CONTESTANT_6B
				{1738, 1738},  // EVENT_BUG_CATCHING_CONTESTANT_7B
				{1739, 1739},  // EVENT_BUG_CATCHING_CONTESTANT_8B
				{1740, 1740},  // EVENT_BUG_CATCHING_CONTESTANT_9B
				{1741, 1741},  // EVENT_BUG_CATCHING_CONTESTANT_10B
				{1742, 1742},  // EVENT_OLIVINE_PORT_SAILOR_AT_GANGWAY
				{1743, 1743},  // EVENT_VERMILION_PORT_SAILOR_AT_GANGWAY
				{1744, 1744},  // EVENT_FAST_SHIP_1F_GENTLEMAN
				{1745, 1745},  // EVENT_FAST_SHIP_CABINS_NNW_NNE_NE_SAILOR
				{1746, 1746},  // EVENT_FAST_SHIP_B1F_SAILOR_LEFT
				{1747, 1747},  // EVENT_FAST_SHIP_B1F_SAILOR_RIGHT
				{1748, 1748},  // EVENT_FAST_SHIP_CABINS_SE_SSE_GENTLEMAN
				{1749, 1749},  // EVENT_FAST_SHIP_CABINS_SE_SSE_CAPTAINS_CABIN_TWIN_1
				{1750, 1750},  // EVENT_FAST_SHIP_CABINS_SE_SSE_CAPTAINS_CABIN_TWIN_2
				{1751, 1751},  // EVENT_FAST_SHIP_MADE_FIRST_TRIP
				{1752, 1752},  // EVENT_ROUTE_35_NATIONAL_PARK_GATE_BUG_MANIAC
				{1753, 1753},  // EVENT_LAKE_OF_RAGE_CIVILIANS
				{1754, 1754},  // EVENT_MAHOGANY_MART_OWNERS
				{1755, 1755},  // EVENT_OLIVINE_PORT_SPRITES_BEFORE_HALL_OF_FAME
				{1756, 1756},  // EVENT_OLIVINE_PORT_SPRITES_AFTER_HALL_OF_FAME
				{1757, 1757},  // EVENT_FAST_SHIP_PASSENGERS_FIRST_TRIP
				{1758, 1758},  // EVENT_FAST_SHIP_PASSENGERS_EASTBOUND
				{1759, 1759},  // EVENT_FAST_SHIP_PASSENGERS_WESTBOUND
				{1760, 1760},  // EVENT_TIN_TOWER_ROOF_HO_OH
				{1761, 1761},  // EVENT_WHIRL_ISLAND_LUGIA_CHAMBER_LUGIA
				{1762, 1762},  // EVENT_KURTS_HOUSE_KURT_1
				{1763, 1763},  // EVENT_KURTS_HOUSE_KURT_2
				{1764, 1764},  // EVENT_SLOWPOKE_WELL_KURT
				{1765, 1765},  // EVENT_PLAYERS_HOUSE_2F_CONSOLE
				{1766, 1766},  // EVENT_PLAYERS_HOUSE_2F_DOLL_1
				{1767, 1767},  // EVENT_PLAYERS_HOUSE_2F_DOLL_2
				{1768, 1768},  // EVENT_PLAYERS_HOUSE_2F_BIG_DOLL
				{1769, 1769},  // EVENT_ROUTE_35_NATIONAL_PARK_GATE_OFFICER_CONTEST_DAY
				{1770, 1770},  // EVENT_ROUTE_35_NATIONAL_PARK_GATE_OFFICER_NOT_CONTEST_DAY
				{1771, 1771},  // EVENT_ROUTE_36_NATIONAL_PARK_GATE_OFFICER_CONTEST_DAY
				{1772, 1772},  // EVENT_ROUTE_36_NATIONAL_PARK_GATE_OFFICER_NOT_CONTEST_DAY
				{1773, 1773},  // EVENT_GOLDENROD_TRAIN_STATION_GENTLEMAN
				{1774, 1774},  // EVENT_BURNED_TOWER_B1F_BEASTS_1
				{1775, 1775},  // EVENT_BURNED_TOWER_B1F_BEASTS_2
				{1776, 1776},  // EVENT_BLACKTHORN_CITY_GRAMPS_BLOCKS_DRAGONS_DEN
				{1777, 1777},  // EVENT_BLACKTHORN_CITY_GRAMPS_NOT_BLOCKING_DRAGONS_DEN
				{1778, 1778},  // EVENT_RUINS_OF_ALPH_KABUTO_CHAMBER_RECEPTIONIST
				{1779, 1779},  // EVENT_LISTENED_TO_OAK_INTRO
				{1780, 1780},  // EVENT_OPENED_MT_SILVER
				{1781, 1781},  // EVENT_LISTENED_TO_YELLOW_INTRO
				{1782, 1782},  // EVENT_FOUGHT_SNORLAX
				{1783, 1783},  // EVENT_LAKE_OF_RAGE_RED_GYARADOS
				{1784, 1784},  // EVENT_WAREHOUSE_ENTRANCE_GRANNY
				{1785, 1785},  // EVENT_WAREHOUSE_ENTRANCE_GRAMPS
				{1786, 1786},  // EVENT_WAREHOUSE_ENTRANCE_OLDER_HAIRCUT_BROTHER
				{1787, 1787},  // EVENT_WAREHOUSE_ENTRANCE_YOUNGER_HAIRCUT_BROTHER
				{1788, 1788},  // EVENT_MAHOGANY_TOWN_POKEFAN_M_BLOCKS_EAST
				{1789, 1789},  // EVENT_MAHOGANY_TOWN_POKEFAN_M_BLOCKS_GYM
				{1790, 1790},  // EVENT_ROUTE_32_FRIEDA_OF_FRIDAY
				{1791, 1791},  // EVENT_ROUTE_29_TUSCANY_OF_TUESDAY
				{1792, 1792},  // EVENT_ROUTE_36_ARTHUR_OF_THURSDAY
				{1793, 1793},  // EVENT_ROUTE_37_SUNNY_OF_SUNDAY
				{1794, 1794},  // EVENT_LAKE_OF_RAGE_WESLEY_OF_WEDNESDAY
				{1795, 1795},  // EVENT_BLACKTHORN_CITY_SANTOS_OF_SATURDAY
				{1796, 1796},  // EVENT_ROUTE_40_MONICA_OF_MONDAY
				{1797, 1797},  // EVENT_LANCES_ROOM_OAK_AND_MARY
				{1798, 1798},  // EVENT_UNION_CAVE_B2F_LAPRAS
				{1799, 1799},  // EVENT_TEAM_ROCKET_DISBANDED
				{1800, 1800},  // EVENT_GOLDENROD_DEPT_STORE_5F_HAPPINESS_EVENT_LADY
				{1801, 1801},  // EVENT_BURNED_TOWER_MORTY
				{1802, 1802},  // EVENT_BURNED_TOWER_1F_EUSINE
				{1803, 1803},  // EVENT_RANG_CLEAR_BELL_1
				{1804, 1804},  // EVENT_RANG_CLEAR_BELL_2
				{1805, 1805},  // EVENT_FLORIA_AT_FLOWER_SHOP
				{1806, 1806},  // EVENT_FLORIA_AT_SUDOWOODO
				{1807, 1807},  // EVENT_GOLDENROD_CITY_MOVE_TUTOR
				{1808, 1808},  // EVENT_RED_FAIRY_BOOK
				{1809, 1809},  // EVENT_BLUE_FAIRY_BOOK
				{1810, 1810},  // EVENT_GREEN_FAIRY_BOOK
				{1811, 1811},  // EVENT_BROWN_FAIRY_BOOK
				{1812, 1812},  // EVENT_VIOLET_FAIRY_BOOK
				{1813, 1813},  // EVENT_PINK_FAIRY_BOOK
				{1814, 1814},  // EVENT_YELLOW_FAIRY_BOOK
				{1815, 1815},  // EVENT_ROUTE_48_JESSIE
				{1816, 1816},  // EVENT_ROUTE_48_JAMES
				{1817, 1817},  // EVENT_ROUTE_48_NURSE
				{1818, 1818},  // EVENT_YELLOW_FOREST_WALKER
				{1819, 1819},  // EVENT_YELLOW_FOREST_SKARMORY
				{1820, 1820},  // EVENT_MYSTRI_UNOWN_W
				{1821, 1821},  // EVENT_MYSTRI_UNOWN_A
				{1822, 1822},  // EVENT_MYSTRI_UNOWN_R
				{1823, 1823},  // EVENT_MYSTRI_UNOWN_P
				{1824, 1824},  // EVENT_MYSTRI_STAGE_CYNTHIA
				{1825, 1825},  // EVENT_MYSTRI_STAGE_EGG
				{1826, 1826},  // EVENT_SINJOH_RUINS_HOUSE_CYNTHIA
				{1827, 1827},  // EVENT_EMBEDDED_TOWER_STEVEN_1
				{1828, 1828},  // EVENT_EMBEDDED_TOWER_STEVEN_2
				{1829, 1829},  // EVENT_GIOVANNIS_CAVE_GIOVANNI
				{1830, 1830},  // EVENT_GIOVANNIS_CAVE_LYRA
				{1831, 1831},  // EVENT_GIOVANNIS_CAVE_CELEBI
				{1832, 1832},  // EVENT_RED_IN_MT_SILVER
				{1833, 1833},  // EVENT_CELADON_UNIVERSITY_POOL_WATER_STONE
				{1834, 1834},  // EVENT_CELADON_UNIVERSITY_LIBRARY_2F_TIMER_BALL
				{1835, 1835},  // EVENT_UNDERGROUND_TM_EXPLOSION
				{1836, 1836},  // EVENT_CERULEAN_CAPE_SHELL_BELL
				{1837, 1837},  // EVENT_CELADON_CITY_MAX_ETHER
				{1838, 1838},  // EVENT_CELADON_HOTEL_ROOM_1_POKE_DOLL
				{1839, 1839},  // EVENT_DIGLETTS_CAVE_RARE_BONE
				{1840, 1840},  // EVENT_ROCK_TUNNEL_1F_ELIXIR
				{1841, 1841},  // EVENT_ROCK_TUNNEL_1F_HP_UP
				{1842, 1842},  // EVENT_ROCK_TUNNEL_B1F_IRON
				{1843, 1843},  // EVENT_ROCK_TUNNEL_B1F_PP_UP
				{1844, 1844},  // EVENT_ROCK_TUNNEL_B1F_REVIVE
				{1845, 1845},  // EVENT_ROCK_TUNNEL_2F_THUNDERSTONE
				{1846, 1846},  // EVENT_ROCK_TUNNEL_2F_TM_THUNDER_WAVE
				{1847, 1847},  // EVENT_ROCK_TUNNEL_2F_ELECTIRIZER
				{1848, 1848},  // EVENT_SOUL_HOUSE_B3F_ESCAPE_ROPE
				{1849, 1849},  // EVENT_ROUTE_2_DIRE_HIT
				{1850, 1850},  // EVENT_ROUTE_2_MAX_POTION
				{1851, 1851},  // EVENT_ROUTE_2_CARBOS
				{1852, 1852},  // EVENT_ROUTE_2_ELIXIR
				{1853, 1853},  // EVENT_ROUTE_3_BIG_ROOT
				{1854, 1854},  // EVENT_ROUTE_4_HP_UP
				{1855, 1855},  // EVENT_ROUTE_7_MENTAL_HERB
				{1856, 1856},  // EVENT_ROUTE_9_MAX_POTION
				{1857, 1857},  // EVENT_ROUTE_10_FULL_RESTORE
				{1858, 1858},  // EVENT_ROUTE_10_TM_VOLT_SWITCH
				{1859, 1859},  // EVENT_ROUTE_11_TM_VENOSHOCK
				{1860, 1860},  // EVENT_ROUTE_12_CALCIUM
				{1861, 1861},  // EVENT_ROUTE_12_NUGGET
				{1862, 1862},  // EVENT_ROUTE_15_PP_UP
				{1863, 1863},  // EVENT_ROUTE_16_WEST_METRONOME
				{1864, 1864},  // EVENT_ROUTE_16_WEST_PP_UP
				{1865, 1865},  // EVENT_ROUTE_16_WEST_MAX_REVIVE
				{1866, 1866},  // EVENT_ROUTE_19_TM_SCALD
				{1867, 1867},  // EVENT_ROUTE_20_BIG_PEARL
				{1868, 1868},  // EVENT_ROUTE_21_STAR_PIECE
				{1869, 1869},  // EVENT_ROUTE_25_PROTEIN
				{1870, 1870},  // EVENT_ROUTE_27_DESTINY_KNOT
				{1871, 1871},  // EVENT_MOUNT_MOON_1F_REVIVE
				{1872, 1872},  // EVENT_MOUNT_MOON_1F_X_ACCURACY
				{1873, 1873},  // EVENT_MOUNT_MOON_1F_CALCIUM
				{1874, 1874},  // EVENT_MOUNT_MOON_B2F_MOON_STONE
				{1875, 1875},  // EVENT_MOUNT_MOON_B2F_DUSK_STONE
				{1876, 1876},  // EVENT_MOUNT_MOON_B2F_SHINY_STONE
				{1877, 1877},  // EVENT_MOUNT_MOON_B2F_BIG_MUSHROOM
				{1878, 1878},  // EVENT_MOUNT_MOON_B2F_HELIX_FOSSIL
				{1879, 1879},  // EVENT_MOUNT_MOON_B2F_DOME_FOSSIL
				{1880, 1880},  // EVENT_ROUTE_9_TM_FLASH_CANNON
				{1881, 1881},  // EVENT_SAFARI_ZONE_HUB_NUGGET
				{1882, 1882},  // EVENT_SAFARI_ZONE_HUB_ULTRA_BALL
				{1883, 1883},  // EVENT_SAFARI_ZONE_EAST_CARBOS
				{1884, 1884},  // EVENT_SAFARI_ZONE_EAST_SILVERPOWDER
				{1885, 1885},  // EVENT_SAFARI_ZONE_EAST_FULL_RESTORE
				{1886, 1886},  // EVENT_SAFARI_ZONE_NORTH_EVIOLITE
				{1887, 1887},  // EVENT_SAFARI_ZONE_NORTH_PROTEIN
				{1888, 1888},  // EVENT_SAFARI_ZONE_WEST_MAX_REVIVE
				{1889, 1889},  // EVENT_URAGA_CHANNEL_EAST_DIVE_BALL
				{1890, 1890},  // EVENT_URAGA_CHANNEL_EAST_EVIOLITE
				{1891, 1891},  // EVENT_SEAFOAM_ISLANDS_B1F_GRIP_CLAW
				{1892, 1892},  // EVENT_SEAFOAM_ISLANDS_B1F_ICE_HEAL
				{1893, 1893},  // EVENT_SEAFOAM_ISLANDS_B2F_WATER_STONE
				{1894, 1894},  // EVENT_SEAFOAM_ISLANDS_B3F_REVIVE
				{1895, 1895},  // EVENT_SEAFOAM_ISLANDS_B3F_BIG_PEARL
				{1896, 1896},  // EVENT_SEAFOAM_ISLANDS_B4F_NEVERMELTICE
				{1897, 1897},  // EVENT_SEAFOAM_ISLANDS_B4F_ULTRA_BALL
				{1898, 1898},  // EVENT_CINNABAR_ISLAND_MAGMARIZER
				{1899, 1899},  // EVENT_CINNABAR_VOLCANO_1F_ESCAPE_ROPE
				{1900, 1900},  // EVENT_CINNABAR_VOLCANO_B1F_FIRE_STONE
				{1901, 1901},  // EVENT_CINNABAR_VOLCANO_B1F_NUGGET
				{1902, 1902},  // EVENT_CINNABAR_VOLCANO_B2F_FLAME_ORB
				{1903, 1903},  // EVENT_POKEMON_MANSION_1F_MOON_STONE
				{1904, 1904},  // EVENT_POKEMON_MANSION_1F_ESCAPE_ROPE
				{1905, 1905},  // EVENT_POKEMON_MANSION_1F_PROTEIN
				{1906, 1906},  // EVENT_POKEMON_MANSION_1F_IRON
				{1907, 1907},  // EVENT_POKEMON_MANSION_B1F_CARBOS
				{1908, 1908},  // EVENT_POKEMON_MANSION_B1F_CALCIUM
				{1909, 1909},  // EVENT_POKEMON_MANSION_B1F_HP_UP
				{1910, 1910},  // EVENT_POKEMON_MANSION_B1F_OLD_SEA_MAP
				{1911, 1911},  // EVENT_CERULEAN_CAVE_1F_BIG_NUGGET
				{1912, 1912},  // EVENT_CERULEAN_CAVE_1F_FULL_RESTORE
				{1913, 1913},  // EVENT_CERULEAN_CAVE_1F_MAX_REVIVE
				{1914, 1914},  // EVENT_CERULEAN_CAVE_2F_FULL_RESTORE
				{1915, 1915},  // EVENT_CERULEAN_CAVE_2F_PP_UP
				{1916, 1916},  // EVENT_CERULEAN_CAVE_2F_ULTRA_BALL
				{1917, 1917},  // EVENT_CERULEAN_CAVE_2F_DUSK_STONE
				{1918, 1918},  // EVENT_CERULEAN_CAVE_B1F_MAX_ELIXIR
				{1919, 1919},  // EVENT_CERULEAN_CAVE_B1F_ULTRA_BALL
				{1920, 1920},  // EVENT_NAVEL_ROCK_SACRED_ASH
				{1921, 1921},  // EVENT_NAVEL_ROCK_MASTER_BALL
				{1922, 1922},  // EVENT_DIM_CAVE_1F_DUSK_BALL
				{1923, 1923},  // EVENT_DIM_CAVE_1F_RARE_BONE
				{1924, 1924},  // EVENT_DIM_CAVE_2F_MAX_REVIVE
				{1925, 1925},  // EVENT_DIM_CAVE_2F_IRON
				{1926, 1926},  // EVENT_DIM_CAVE_2F_LIGHT_CLAY
				{1927, 1927},  // EVENT_DIM_CAVE_2F_TM_FACADE
				{1928, 1928},  // EVENT_DIM_CAVE_3F_METAL_COAT
				{1929, 1929},  // EVENT_DIM_CAVE_3F_ESCAPE_ROPE
				{1930, 1930},  // EVENT_DIM_CAVE_3F_TM_REST
				{1931, 1931},  // EVENT_DIM_CAVE_4F_MAX_ETHER
				{1932, 1932},  // EVENT_DIM_CAVE_4F_NUGGET
				{1933, 1933},  // EVENT_DIM_CAVE_4F_FULL_RESTORE
				{1934, 1934},  // EVENT_DIM_CAVE_5F_RARE_CANDY
				{1935, 1935},  // EVENT_DIM_CAVE_5F_DUSK_STONE
				{1936, 1936},  // EVENT_DIM_CAVE_5F_HYPER_POTION
				{1937, 1937},  // EVENT_SCARY_CAVE_1F_X_SP_DEF
				{1938, 1938},  // EVENT_SCARY_CAVE_1F_DUSK_STONE
				{1939, 1939},  // EVENT_SCARY_CAVE_1F_HYPER_POTION
				{1940, 1940},  // EVENT_SCARY_CAVE_1F_MAX_REPEL
				{1941, 1941},  // EVENT_SCARY_CAVE_1F_REVIVE
				{1942, 1942},  // EVENT_SCARY_CAVE_B1F_BIG_NUGGET
				{1943, 1943},  // EVENT_SCARY_CAVE_B1F_BLACK_SLUDGE
				{1944, 1944},  // EVENT_SCARY_CAVE_SHIPWRECK_RARE_BONE
				{1945, 1945},  // EVENT_LUCKY_ISLAND_LUCKY_EGG
				{1946, 1946},  // EVENT_ROUTE_24_ROCKET
				{1947, 1947},  // EVENT_CERULEAN_GYM_ROCKET
				{1948, 1948},  // EVENT_ROUTE_25_COOLTRAINER_M_BEFORE
				{1949, 1949},  // EVENT_ROUTE_25_COOLTRAINER_M_AFTER
				{1950, 1950},  // EVENT_CERULEAN_CAPE_BOYFRIEND
				{1951, 1951},  // EVENT_TRAINERS_IN_CERULEAN_GYM
				{1952, 1952},  // EVENT_VERMILION_CITY_SNORLAX
				{1953, 1953},  // EVENT_ROUTE_8_SNORLAX
				{1954, 1954},  // EVENT_ROUTE_5_6_POKEFAN_M_BLOCKS_UNDERGROUND_PATH
				{1955, 1955},  // EVENT_SAFFRON_TRAIN_STATION_POPULATION
				{1956, 1956},  // EVENT_LAV_RADIO_TOWER_POPULATION
				{1957, 1957},  // EVENT_LAVENDER_TOWN_FLEEING_YOUNGSTER
				{1958, 1958},  // EVENT_SOUL_HOUSE_MR_FUJI
				{1959, 1959},  // EVENT_ROUTE_8_PROTESTORS
				{1960, 1960},  // EVENT_ROUTE_8_KANTO_POKEMON_FEDERATION
				{1961, 1961},  // EVENT_COPYCATS_HOUSE_2F_DOLL
				{1962, 1962},  // EVENT_VERMILION_FAN_CLUB_DOLL
				{1963, 1963},  // EVENT_ROUTE_19_ROCK
				{1964, 1964},  // EVENT_LISTENED_TO_BLUE_INTRO
				{1965, 1965},  // EVENT_BLUE_IN_CINNABAR
				{1966, 1966},  // EVENT_VIRIDIAN_GYM_BLUE
				{1967, 1967},  // EVENT_SEAFOAM_GYM_GYM_GUY
				{1968, 1968},  // EVENT_MT_MOON_SQUARE_ROCK
				{1969, 1969},  // EVENT_MT_MOON_SQUARE_CLEFAIRY
				{1970, 1970},  // EVENT_MT_MOON_RIVAL
				{1971, 1971},  // EVENT_INDIGO_PLATEAU_POKECENTER_RIVAL
				{1972, 1972},  // EVENT_INDIGO_PLATEAU_POKECENTER_LYRA
				{1973, 1973},  // EVENT_INDIGO_PLATEAU_POKECENTER_YELLOW
				{1974, 1974},  // EVENT_TELEPORT_GUY
				{1975, 1975},  // EVENT_ROCK_TUNNEL_2F_ELECTRODE
				{1976, 1976},  // EVENT_SEAFOAM_ISLANDS_ARTICUNO
				{1977, 1977},  // EVENT_ZAPDOS_GONE
				{1978, 1978},  // EVENT_ROUTE_10_ZAPDOS
				{1979, 1979},  // EVENT_CINNABAR_VOLCANO_MOLTRES
				{1980, 1980},  // EVENT_CERULEAN_CAVE_MEWTWO
				{1981, 1981},  // EVENT_FARAWAY_JUNGLE_MEW
				{1982, 1982},  // EVENT_LAWRENCE_VERMILION_CITY
				{1983, 1983},  // EVENT_LAWRENCE_ROUTE_10
				{1984, 1984},  // EVENT_LAWRENCES_ZAPDOS_ROUTE_10
				{1985, 1985},  // EVENT_LAWRENCE_FINAL_BIRD
				{1986, 1986},  // EVENT_LAWRENCE_FINAL_BIRD_SURF
				{1987, 1987},  // EVENT_LAWRENCE_SHAMOUTI_SHRINE_RUINS
				{1988, 1988},  // EVENT_LAWRENCE_FARAWAY_ISLAND
				{1989, 1989},  // EVENT_ROUTE_22_PAST_LYRA
				{1990, 1990},  // EVENT_ROUTE_22_PAST_CELEBI
				{1991, 1991},  // EVENT_ROUTE_22_PAST_SILVER
				{1992, 1992},  // EVENT_ROUTE_22_PAST_GIOVANNI
				{1993, 1993},  // EVENT_CINNABAR_LAB_CELEBI
				{1994, 1994},  // EVENT_CINNABAR_LAB_CHRIS
				{1995, 1995},  // EVENT_CINNABAR_LAB_KRIS
				{1996, 1996},  // EVENT_CINNABAR_LAB_SCIENTIST1
				{1997, 1997},  // EVENT_CINNABAR_LAB_SCIENTIST2
				{1998, 1998},  // EVENT_CINNABAR_LAB_MEWTWO
				{1999, 1999},  // EVENT_LEAF_IN_NAVEL_ROCK
				{2000, 2000},  // EVENT_CHRIS_IN_NAVEL_ROCK
				{2001, 2001},  // EVENT_KRIS_IN_NAVEL_ROCK
				{2002, 2002},  // EVENT_BOULDER_FELL_IN_DIM_CAVE_2F
				{2003, 2003},  // EVENT_BOULDER_FELL_IN_DIM_CAVE_3F
				{2004, 2004},  // EVENT_BOULDER_FELL_IN_DIM_CAVE_4F
				{2005, 2005},  // EVENT_BOULDER_IN_DIM_CAVE_3F
				{2006, 2006},  // EVENT_BOULDER_IN_DIM_CAVE_4F
				{2007, 2007},  // EVENT_BOULDER_IN_DIM_CAVE_5F
				{2008, 2008},  // EVENT_BOULDER_IN_CINNABAR_VOLCANO_1F_1
				{2009, 2009},  // EVENT_BOULDER_IN_CINNABAR_VOLCANO_1F_2
				{2010, 2010},  // EVENT_BOULDER_IN_CINNABAR_VOLCANO_1F_3
				{2011, 2011},  // EVENT_BOULDER_IN_CINNABAR_VOLCANO_1F_4
				{2012, 2012},  // EVENT_BOULDER_IN_CINNABAR_VOLCANO_B1F
				{2013, 2013},  // EVENT_LUCKY_ISLAND_CIVILIANS
				{2017, 2017},  // EVENT_MURKY_SWAMP_CHERYL
				{2018, 2018},  // EVENT_DIM_CAVE_RILEY
				{2019, 2019},  // EVENT_CINNABAR_VOLCANO_BUCK
				{2020, 2020},  // EVENT_QUIET_CAVE_MARLEY
				{2021, 2021},  // EVENT_SCARY_CAVE_MIRA
				{2022, 2022},  // EVENT_BATTLE_TOWER_OUTSIDE_ANABEL
				{2023, 2023},  // EVENT_BATTLE_TOWER_CHERYL
				{2024, 2024},  // EVENT_BATTLE_TOWER_RILEY
				{2025, 2025},  // EVENT_BATTLE_TOWER_BUCK
				{2026, 2026},  // EVENT_BATTLE_TOWER_MARLEY
				{2027, 2027},  // EVENT_BATTLE_TOWER_MIRA
				{2028, 2028},  // EVENT_BATTLE_TOWER_ANABEL
				{2029, 2029},  // EVENT_BEAUTIFUL_BEACH_FULL_RESTORE
				{2030, 2030},  // EVENT_BEAUTIFUL_BEACH_LUXURY_BALL
				{2031, 2031},  // EVENT_ROCKY_BEACH_FULL_HEAL
				{2032, 2032},  // EVENT_ROCKY_BEACH_PEARL_STRING
				{2033, 2033},  // EVENT_NOISY_FOREST_BALMMUSHROOM
				{2034, 2034},  // EVENT_NOISY_FOREST_MULCH
				{2035, 2035},  // EVENT_NOISY_FOREST_TM_DRAIN_PUNCH
				{2036, 2036},  // EVENT_SHAMOUTI_SHRINE_RUINS_RARE_CANDY
				{2037, 2037},  // EVENT_SHAMOUTI_TUNNEL_SMOOTH_ROCK
				{2038, 2038},  // EVENT_SHAMOUTI_TUNNEL_X_SPEED
				{2039, 2039},  // EVENT_SHAMOUTI_COAST_STAR_PIECE
				{2040, 2040},  // EVENT_FIRE_ISLAND_HEAT_ROCK
				{2041, 2041},  // EVENT_ICE_ISLAND_ICY_ROCK
				{2042, 2042},  // EVENT_LIGHTNING_ISLAND_DAMP_ROCK
				{2043, 2043},  // EVENT_SHAMOUTI_ISLAND_ALOLAN_EXEGGUTOR
				{2044, 2044},  // EVENT_SHAMOUTI_ISLAND_PIKABLU_GUY
				{2045, 2045},  // EVENT_SHAMOUTI_POKE_CENTER_IVY
				{2046, 2046},  // EVENT_NOISY_FOREST_PIKABLU
				{2050, 2050},  // EVENT_KURTS_HOUSE_GRANDDAUGHTER_1
				{2051, 2051},  // EVENT_KURTS_HOUSE_GRANDDAUGHTER_2
				{2052, 2052},  // EVENT_RUINS_OF_ALPH_OUTSIDE_TOURIST_FISHER
				{2053, 2053},  // EVENT_RUINS_OF_ALPH_OUTSIDE_TOURIST_YOUNGSTERS
				{2054, 2054},  // EVENT_DRAGON_SHRINE_CLAIR
				{2055, 2055},  // EVENT_BATTLE_TOWER_BATTLE_ROOM_YOUNGSTER
				{2056, 2056},  // EVENT_PLAYERS_HOUSE_1F_NEIGHBOR
				{2057, 2057},  // EVENT_PLAYERS_NEIGHBORS_HOUSE_NEIGHBOR
				{2058, 2058},  // EVENT_PICKED_UP_SUN_STONE_FROM_HO_OH_ITEM_ROOM
				{2059, 2059},  // EVENT_PICKED_UP_MOON_STONE_FROM_HO_OH_ITEM_ROOM
				{2060, 2060},  // EVENT_PICKED_UP_LIFE_ORB_FROM_HO_OH_ITEM_ROOM
				{2061, 2061},  // EVENT_PICKED_UP_CHARCOAL_FROM_HO_OH_ITEM_ROOM
				{2062, 2062},  // EVENT_PICKED_UP_SITRUS_BERRY_FROM_KABUTO_ITEM_ROOM
				{2063, 2063},  // EVENT_PICKED_UP_LUM_BERRY_FROM_KABUTO_ITEM_ROOM
				{2064, 2064},  // EVENT_PICKED_UP_HEAL_POWDER_FROM_KABUTO_ITEM_ROOM
				{2065, 2065},  // EVENT_PICKED_UP_ENERGYPOWDER_FROM_KABUTO_ITEM_ROOM
				{2066, 2066},  // EVENT_PICKED_UP_PEARL_STRING_FROM_OMANYTE_ITEM_ROOM
				{2067, 2067},  // EVENT_PICKED_UP_BIG_PEARL_FROM_OMANYTE_ITEM_ROOM
				{2068, 2068},  // EVENT_PICKED_UP_STARDUST_FROM_OMANYTE_ITEM_ROOM
				{2069, 2069},  // EVENT_PICKED_UP_STAR_PIECE_FROM_OMANYTE_ITEM_ROOM
				{2070, 2070},  // EVENT_PICKED_UP_RARE_BONE_FROM_AERODACTYL_ITEM_ROOM
				{2071, 2071},  // EVENT_PICKED_UP_ENERGY_ROOT_FROM_AERODACTYL_ITEM_ROOM
				{2072, 2072},  // EVENT_PICKED_UP_REVIVAL_HERB_FROM_AERODACTYL_ITEM_ROOM
				{2073, 2073},  // EVENT_PICKED_UP_SOOTHE_BELL_FROM_AERODACTYL_ITEM_ROOM
				{2074, 2074},  // EVENT_AZALEA_TOWN_KURT
				{2075, 2075},  // EVENT_ILEX_FOREST_KURT
				{2076, 2076},  // EVENT_MOUNT_MORTAR_1F_INSIDE_MAX_POTION
				{2077, 2077},  // EVENT_MOUNT_MORTAR_1F_INSIDE_NUGGET
				{2078, 2078},  // EVENT_ECRUTEAK_GYM_GRAMPS
				{2079, 2079},  // EVENT_ECRUTEAK_CITY_GRAMPS
				{2080, 2080},  // EVENT_EUSINE_IN_BURNED_TOWER
				{2081, 2081},  // EVENT_WISE_TRIOS_ROOM_WISE_TRIO_1
				{2082, 2082},  // EVENT_WISE_TRIOS_ROOM_WISE_TRIO_2
				{2083, 2083},  // EVENT_CIANWOOD_CITY_EUSINE
				{2084, 2084},  // EVENT_SAW_SUICUNE_AT_CIANWOOD_CITY
				{2085, 2085},  // EVENT_SAW_SUICUNE_ON_ROUTE_42
				{2086, 2086},  // EVENT_SAW_SUICUNE_ON_ROUTE_36
				{2087, 2087},  // EVENT_ECRUTEAK_HOUSE_WANDERING_SAGE
				{2088, 2088},  // EVENT_TIN_TOWER_1F_SUICUNE
				{2089, 2089},  // EVENT_TIN_TOWER_1F_ENTEI
				{2090, 2090},  // EVENT_TIN_TOWER_1F_RAIKOU
				{2091, 2091},  // EVENT_TIN_TOWER_1F_EUSINE
				{2092, 2092},  // EVENT_TIN_TOWER_1F_WISE_TRIO_1
				{2093, 2093},  // EVENT_EUSINES_HOUSE_EUSINE
				{2094, 2094},  // EVENT_ROUTE_30_ANTIDOTE
				{2095, 2095},  // EVENT_ILEX_FOREST_X_ATTACK
				{2096, 2096},  // EVENT_ILEX_FOREST_ANTIDOTE
				{2097, 2097},  // EVENT_ILEX_FOREST_MULCH
				{2098, 2098},  // EVENT_ROUTE_34_NUGGET
				{2099, 2099},  // EVENT_ROUTE_44_MAX_REPEL
				{2100, 2100},  // EVENT_ICE_PATH_1F_HEAT_ROCK
				{2101, 2101},  // EVENT_DRAGONS_DEN_B1F_CALCIUM
				{2102, 2102},  // EVENT_DRAGONS_DEN_B1F_MAX_ELIXIR
				{2103, 2103},  // EVENT_SILVER_CAVE_ROOM_1_ULTRA_BALL
				{2104, 2104},  // EVENT_SILVER_CAVE_ROOM_2_CALCIUM
				{2105, 2105},  // EVENT_SILVER_CAVE_ROOM_2_ULTRA_BALL
				{2106, 2106},  // EVENT_SILVER_CAVE_ROOM_2_PP_UP
				{2107, 2107},  // EVENT_TIN_TOWER_1F_WISE_TRIO_2
				{2108, 2108},  // EVENT_TIN_TOWER_6F_MAX_POTION
				{2109, 2109},  // EVENT_TIN_TOWER_9F_HP_UP
				{2110, 2110},  // EVENT_MOUNT_MORTAR_1F_INSIDE_IRON
				{2111, 2111},  // EVENT_MOUNT_MORTAR_1F_INSIDE_ULTRA_BALL
				{2112, 2112},  // EVENT_MOUNT_MORTAR_B1F_PROTECTOR
				{2113, 2113},  // EVENT_MOUNT_MORTAR_B1F_MAX_ETHER
				{2114, 2114},  // EVENT_MOUNT_MORTAR_B1F_PP_UP
				{2115, 2115},  // EVENT_RADIO_TOWER_5F_ZOOM_LENS
				{2116, 2116},  // EVENT_DARK_CAVE_VIOLET_ENTRANCE_DIRE_HIT
				{2117, 2117},  // EVENT_BEAT_ELITE_FOUR_AGAIN
				{2118, 2118},  // EVENT_REMATCH_GYM_LEADER_1
				{2119, 2119},  // EVENT_REMATCH_GYM_LEADER_2
				{2120, 2120},  // EVENT_REMATCH_GYM_LEADER_3
				{2121, 2121},  // EVENT_REMATCH_GYM_LEADER_4
				{2122, 2122},  // EVENT_REMATCH_GYM_LEADER_5
				{2123, 2123},  // EVENT_REMATCH_GYM_LEADER_6
				{2124, 2124},  // EVENT_COPYCAT_3
				{2125, 2125},  // EVENT_CINNABAR_LAB_CRYS
				{2126, 2126},  // EVENT_DO_RUINS_OF_ALPH_CLIMAX
				{2127, 2127},  // EVENT_RUINS_OF_ALPH_CLIMAX_DONE
				{2128, 2128},  // EVENT_MAGNET_TUNNEL_LODESTONE_IN_PIT
				{2129, 2129},  // EVENT_YELLOW_FOREST_ROCKET_TAKEOVER
				{2130, 2130},  // EVENT_CLEARED_YELLOW_FOREST
				{2131, 2131},  // EVENT_GOT_LIGHT_BALL_FROM_YELLOW
				{2132, 2132},  // EVENT_INTRODUCED_LORELEI
				{2133, 2133},  // EVENT_INTRODUCED_FLANNERY
				{2134, 2134},  // EVENT_INTRODUCED_MARLON
				{2135, 2135},  // EVENT_INTRODUCED_KUKUI
				{2136, 2136},  // EVENT_LISTENED_TO_STEVEN_INTRO
				{2137, 2137},  // EVENT_GOT_MUSCLE_BAND_FROM_STEVEN
				{2138, 2138},  // EVENT_LISTENED_TO_CYNTHIA_INTRO
				{2139, 2139},  // EVENT_GOT_WISE_GLASSES_FROM_CYNTHIA
				{2140, 2140},  // EVENT_SPOKE_TO_LADY_JESSICA
				{2141, 2141},  // EVENT_GOT_SILPHSCOPE2_FROM_MR_FUJI
				{2142, 2142},  // EVENT_EXORCISED_LAV_RADIO_TOWER
				{2143, 2143},  // EVENT_GOT_EXPERT_BELT
				{2144, 2144},  // EVENT_ROUTE_2_CUT_TREE_1
				{2145, 2145},  // EVENT_ROUTE_2_CUT_TREE_2
				{2146, 2146},  // EVENT_ROUTE_2_CUT_TREE_3
				{2147, 2147},  // EVENT_ROUTE_2_CUT_TREE_4
				{2148, 2148},  // EVENT_ROUTE_2_CUT_TREE_5
				{2149, 2149},  // EVENT_ROUTE_8_CUT_TREE_1
				{2150, 2150},  // EVENT_ROUTE_8_CUT_TREE_2
				{2151, 2151},  // EVENT_ROUTE_9_CUT_TREE
				{2152, 2152},  // EVENT_ROUTE_10_CUT_TREE_1
				{2153, 2153},  // EVENT_ROUTE_10_CUT_TREE_2
				{2154, 2154},  // EVENT_ROUTE_10_CUT_TREE_3
				{2155, 2155},  // EVENT_ROUTE_10_CUT_TREE_4
				{2156, 2156},  // EVENT_ROUTE_12_CUT_TREE_1
				{2157, 2157},  // EVENT_ROUTE_12_CUT_TREE_2
				{2158, 2158},  // EVENT_ROUTE_13_CUT_TREE
				{2159, 2159},  // EVENT_ROUTE_14_CUT_TREE_1
				{2160, 2160},  // EVENT_ROUTE_14_CUT_TREE_2
				{2161, 2161},  // EVENT_ROUTE_14_CUT_TREE_3
				{2162, 2162},  // EVENT_ROUTE_16_CUT_TREE
				{2163, 2163},  // EVENT_ROUTE_16_WEST_CUT_TREE_1
				{2164, 2164},  // EVENT_ROUTE_16_WEST_CUT_TREE_2
				{2165, 2165},  // EVENT_ROUTE_25_CUT_TREE
				{2166, 2166},  // EVENT_ROUTE_29_CUT_TREE_1
				{2167, 2167},  // EVENT_ROUTE_29_CUT_TREE_2
				{2168, 2168},  // EVENT_ROUTE_30_CUT_TREE
				{2169, 2169},  // EVENT_ROUTE_31_CUT_TREE_1
				{2170, 2170},  // EVENT_ROUTE_31_CUT_TREE_2
				{2171, 2171},  // EVENT_ROUTE_32_CUT_TREE
				{2172, 2172},  // EVENT_ROUTE_35_CUT_TREE
				{2173, 2173},  // EVENT_ROUTE_42_CUT_TREE
				{2174, 2174},  // EVENT_ROUTE_43_CUT_TREE
				{2175, 2175},  // EVENT_CELADON_CITY_CUT_TREE
				{2176, 2176},  // EVENT_FUCHSIA_CITY_CUT_TREE
				{2177, 2177},  // EVENT_VERMILION_CITY_CUT_TREE
				{2178, 2178},  // EVENT_VIOLET_CITY_CUT_TREE
				{2179, 2179},  // EVENT_VIRIDIAN_CITY_CUT_TREE_1
				{2180, 2180},  // EVENT_VIRIDIAN_CITY_CUT_TREE_2
				{2181, 2181},  // EVENT_ILEX_FOREST_CUT_TREE
				{2182, 2182},  // EVENT_LAKE_OF_RAGE_CUT_TREE_1
				{2183, 2183},  // EVENT_LAKE_OF_RAGE_CUT_TREE_2
				{2184, 2184},  // EVENT_LAKE_OF_RAGE_CUT_TREE_3
				{2185, 2185},  // EVENT_LAKE_OF_RAGE_CUT_TREE_4
				{2186, 2186},  // EVENT_LAKE_OF_RAGE_CUT_TREE_5
				{2187, 2187},  // EVENT_SILVER_CAVE_OUTSIDE_CUT_TREE_1
				{2188, 2188},  // EVENT_SILVER_CAVE_OUTSIDE_CUT_TREE_2
				{2189, 2189},  // EVENT_MURKY_SWAMP_CUT_TREE_1
				{2190, 2190},  // EVENT_MURKY_SWAMP_CUT_TREE_2
				{2191, 2191},  // EVENT_NOISY_FOREST_CUT_TREE_1
				{2192, 2192},  // EVENT_NOISY_FOREST_CUT_TREE_2
				{2193, 2193},  // EVENT_ROUTE_49_CUT_TREE_1
				{2194, 2194},  // EVENT_ROUTE_49_CUT_TREE_2
				{2195, 2195},  // EVENT_MAGNET_TUNNEL_EAST_CUT_TREE
				{2196, 2196},  // EVENT_CHERRYGROVE_BAY_CUT_TREE
				{2197, 2197},  // EVENT_MAGNET_TUNNEL_HIDDEN_METAL_POWDER
				{2198, 2198},  // EVENT_BEAT_ENGINEER_GRADEN
				{2199, 2199},  // EVENT_BEAT_ENGINEER_GUSTAV
				{2200, 2200},  // EVENT_BEAT_ENGINEER_NICOLAS
				{2201, 2201},  // EVENT_MAGNET_TUNNEL_TM_GYRO_BALL
				{2203, 2203},  // EVENT_GOT_CLEAR_AMULET_IN_AZALEA
				{2204, 2204},  // EVENT_GOT_ROOM_SERVICE_IN_CELADON_HOTEL
				{2205, 2205},  // EVENT_WAREHOUSE_ENTRANCE_HIDDEN_X_SP_ATK
				{2206, 2206},  // EVENT_INTRODUCED_PIERS
				{2207, 2207},  // EVENT_GOT_THROAT_SPRAY_FROM_PIERS
				{2208, 2208},  // EVENT_LISTENED_TO_ENDURE_INTRO
				{2209, 2209},  // EVENT_BEAT_KIMONO_GIRL_MAKO
				{2210, 2210},  // EVENT_BEAT_KIMONO_GIRL_AMI
				{2211, 2211},  // EVENT_BEAT_KIMONO_GIRL_MINA
				{2212, 2212},  // EVENT_GOT_RARE_CANDY_FROM_KIMONO_GIRL_MAKO
				{2213, 2213},  // EVENT_GOT_PP_MAX_FROM_KIMONO_GIRL_AMI
				{2214, 2214},  // EVENT_GOT_ABILITYPATCH_FROM_KIMONO_GIRL_MINA
				{2215, 2215},  // EVENT_OLIVINE_PORT_GO_GOGGLES
				{2216, 2216},  // EVENT_BEAT_NURSE_BEATRICE
				{2217, 2217},  // EVENT_BEAT_NURSE_KEIKO
				{2218, 2218},  // EVENT_RUGGED_ROAD_SOUTH_IRON_BALL
				{2219, 2219},  // EVENT_RUGGED_ROAD_SOUTH_REVIVE
				{2220, 2220},  // EVENT_RUGGED_ROAD_SOUTH_HIDDEN_IRON
				{2221, 2221},  // EVENT_RUGGED_ROAD_NORTH_X_ATTACK
				{2222, 2222},  // EVENT_RUGGED_ROAD_NORTH_HYPER_POTION
				{2223, 2223},  // EVENT_RUGGED_ROAD_NORTH_HIDDEN_RARE_BONE
				{2224, 2224},  // EVENT_SNOWTOP_MOUNTAIN_INSIDE_ETHER
				{2225, 2225},  // EVENT_SNOWTOP_MOUNTAIN_INSIDE_HEAVY_BOOTS
				{2226, 2226},  // EVENT_SNOWTOP_MOUNTAIN_INSIDE_COVERT_CLOAK
				{2227, 2227},  // EVENT_SNOWTOP_MOUNTAIN_INSIDE_HIDDEN_ZINC
				{2228, 2228},  // EVENT_INTRODUCED_KATY
				{2229, 2229},  // EVENT_BEAT_KATY
				{2230, 2230},  // EVENT_GOT_SWEET_HONEY_FROM_KATY
				{2231, 2231},  // EVENT_BEAT_THORTON
				{2232, 2232},  // EVENT_UNION_CAVE_B2F_LINKING_CORD
				{2233, 2233},  // EVENT_GOT_LOADED_DICE_FROM_GOLDENROD
				{2234, 2234},  // EVENT_LISTENED_TO_BATON_PASS_INTRO
				{2235, 2235},  // EVENT_CAUGHT_HISUIAN_TYPHLOSION
				{2236, 2236},  // EVENT_TALKED_TO_VIOLET_CEMETERY_CARETAKER
				{2237, 2237},  // EVENT_VIOLET_CEMETERY_CARETAKER
		};

		// Return the corresponding version 8 event flag or INVALID_EVENT_FLAG if not found
		return indexMap.find(v8) != indexMap.end() ? indexMap[v8] : INVALID_EVENT_FLAG;
	}

	// converts a version 8 key item to a version 9 key item
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
			{0x1f, 0x21},  // HARSH_LURE
			{0x20, 0x22},  // POTENT_LURE
			{0x21, 0x23},  // MALIGN_LURE
			{0x22, 0x24},  // SHINY_CHARM
			{0x23, 0x25},  // OVAL_CHARM
			{0x24, 0x26},  // CATCH_CHARM
		};

		// return the corresponding version 8 key item or 0xFF if not found
		return indexMap.find(v8) != indexMap.end() ? indexMap[v8] : 0xFF;
	}

}