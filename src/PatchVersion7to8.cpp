#include "PatchVersion7to8.h"
#include "CommonPatchFunctions.h"
#include "SymbolDatabase.h"
#include "Logging.h"

constexpr int NUM_OBJECT_STRUCTS = 13;
constexpr int OBJECT_PALETTE_V7 = 0x06;
constexpr int OBJECT_PALETTE_V8 = 0x06;
constexpr int OBJECT_PAL_INDEX_V8 = 0x21;
constexpr int OBJECT_LENGTH_V7 = 0x21;
constexpr int OBJECT_LENGTH_V8 = 0x22;
constexpr int NUM_KEY_ITEMS_V7 = 0x1D;
constexpr int NUM_KEY_ITEMS_V8 = 0x24;
constexpr int NUM_APRICORNS = 0x07;
constexpr int NUM_EVENTS = 0x8ff;
constexpr int NUM_FRUIT_TREES_V7 = 0x23;
constexpr int NUM_LANDMARKS_V7 = 0x90;
constexpr int NUM_LANDMARKS_V8 = 0x91;
constexpr int CONTACT_LIST_SIZE_V7 = 30;
constexpr int NUM_PHONE_CONTACTS_V8 = 0x25;
constexpr int NUM_SPAWNS_V7 = 30;
constexpr int NUM_SPAWNS_V8 = 34;
constexpr int PARTYMON_STRUCT_LENGTH = 0x30;
constexpr int PARTY_LENGTH = 6;
constexpr int PLAYER_NAME_LENGTH = 8;
constexpr int MON_NAME_LENGTH = 11;
constexpr uint8_t EXTSPECIES_MASK = 0b00100000;
constexpr uint8_t FORM_MASK = 0b00011111;
constexpr int MON_EXTSPECIES = 0x15;
constexpr int MON_EXTSPECIES_F = 5;
constexpr uint8_t CAUGHT_BALL_MASK = 0b00011111;
constexpr int MON_ITEM = 0x01;
constexpr int MON_FORM = 0x15;
constexpr int MON_CAUGHTBALL = 0x1c;
constexpr int MON_CAUGHTLOCATION = 0x1e;
constexpr int NUM_POKEMON_V7 = 0xfe;
constexpr int MONDB_ENTRIES_V7 = 167;
constexpr int MONDB_ENTRIES_A_V8 = 167;
constexpr int MONDB_ENTRIES_B_V8 = 28;
constexpr int MONDB_ENTRIES_C_V8 = 12;
constexpr int MONDB_ENTRIES_V8 = MONDB_ENTRIES_A_V8 + MONDB_ENTRIES_B_V8 + MONDB_ENTRIES_C_V8;
constexpr int SAVEMON_STRUCT_LENGTH = 0x31;
constexpr int MONS_PER_BOX = 20;
constexpr int MIN_MONDB_SLACK = 10;
constexpr int NUM_BOXES_V7 = (MONDB_ENTRIES_V7 * 2 - MIN_MONDB_SLACK) / MONS_PER_BOX;
constexpr int NUM_BOXES_V8 = (MONDB_ENTRIES_V8 * 2 - MIN_MONDB_SLACK) / MONS_PER_BOX;
constexpr int BOX_NAME_LENGTH = 9;
constexpr int NEWBOX_SIZE = MONS_PER_BOX + ((MONS_PER_BOX + 7) / 8) + BOX_NAME_LENGTH + 1;
constexpr int SAVEMON_EXTSPECIES = 0x15;
constexpr int SAVEMON_ITEM = 0x01;
constexpr int SAVEMON_FORM = 0x15;
constexpr int SAVEMON_CAUGHTBALL = 0x19;
constexpr int SAVEMON_CAUGHTLOCATION = 0x1b;
constexpr int BATTLETOWER_PARTYDATA_SIZE = 6;
constexpr int NUM_HOF_TEAMS_V8 = 10;
constexpr int HOF_MON_LENGTH = 1 + 2 + 2 + 1 + (MON_NAME_LENGTH - 1); // species, id, dvs, level, nick
constexpr int HOF_LENGTH = 1 + HOF_MON_LENGTH * PARTY_LENGTH + 1; // win count, party, terminator

bool patchVersion7to8(SaveBinary& save7, SaveBinary& save8) {
	// copy the old save file to the new save file
	save8 = save7;

	// create the iterators
	SaveBinary::Iterator it7(save7, 0);
	SaveBinary::Iterator it8(save8, 0);

	// Load the version 7 and version 8 sym files
	SymbolDatabase sym7(VERSION_7_SYMBOL_FILE);
	SymbolDatabase sym8(VERSION_8_SYMBOL_FILE);

	// get the checksum word from the version 7 save file
	uint16_t save_checksum = save7.getWord(SAVE_CHECKSUM_ABS_ADDRESS);

	// verify the checksum of the version 7 file matches the calculated checksum
	// calculate the checksum from lookup symbol name "sGameData" to "sGameDataEnd"
	uint16_t calculated_checksum = calculate_checksum(save7, sym7.getSRAMAddress("sGameData"), sym7.getSRAMAddress("sGameDataEnd"));
	if (save_checksum != calculated_checksum) {
		js_error <<  "Checksum mismatch! Expected: " << std::hex << calculated_checksum << ", got: " << save_checksum << std::endl;
		return false;
	}

	// check the backup checksum word from the version 7 save file
	uint16_t backup_checksum = save7.getWord(SAVE_BACKUP_CHECKSUM_ABS_ADDRESS);
	// verify the backup checksum of the version 7 file matches the calculated checksum
	// calculate the checksum from lookup symbol name "sBackupGameData" to "sBackupGameDataEnd"
	uint16_t calculated_backup_checksum = calculate_checksum(save7, sym7.getSRAMAddress("sBackupGameData"), sym7.getSRAMAddress("sBackupGameDataEnd"));
	if (backup_checksum != calculated_backup_checksum) {
		js_error <<  "Backup checksum mismatch! Expected: " << std::hex << calculated_backup_checksum << ", got: " << backup_checksum << std::endl;
		return false;
	}

	// check if the player is inside a pokemon center
	it7.seek(sym7.getMapDataAddress("wMapGroup"));
	uint8_t map_group = it7.getByte();
	it7.next();
	uint8_t map_num = it7.getByte();
	if (!inPokecenter(map_group, map_num)) {
		js_error <<  "Player is not in a pokemon center. Please save in a pokemon center and try again." << std::endl;
		return false;
	}

	// for n, 1, NUM_BOXES_V8 + 1
	js_info <<  "Clearing v8 sNewBox#..." << std::endl;
	for (int n = 1; n < NUM_BOXES_V8 + 1; n++) {
		// clear the newbox
		it8.seek(sym8.getSRAMAddress("sNewBox" + std::to_string(n)));
		for (int i = 0; i < NEWBOX_SIZE; i++) {
			it8.setByte(0x00);
			it8.next();
		}
	}

	// copy the version 7 boxes to the version 8 boxes
	for (int n = 1; n < NUM_BOXES_V7 + 1; n++) {
		// copy the newbox
		it7.seek(sym7.getSRAMAddress("sNewBox" + std::to_string(n)));
		it8.seek(sym8.getSRAMAddress("sNewBox" + std::to_string(n)));
		for (int i = 0; i < NEWBOX_SIZE; i++) {
			it8.setByte(it7.getByte());
			it7.next();
			it8.next();
		}
	}

	// Write default box names from NUM_BOXES_V7 + 1 to NUM_BOXES_V8
	js_info <<  "Writing default box names..." << std::endl;
	for (int n = NUM_BOXES_V7 + 1; n < NUM_BOXES_V8 + 1; n++) {
		it8.seek(sym8.getSRAMAddress("sNewBox" + std::to_string(n) + "Name"));
		js_info <<  "Writing default box name for sNewBox" << n << "..." << std::endl;
		it8.setByte(0x7f); // ' '
		it8.next();
		it8.setByte(0x7f); // ' '
		it8.next();
		it8.setByte(0x81); // B
		it8.next();
		it8.setByte(0xae); // o
		it8.next();
		it8.setByte(0xb7); // x
		it8.next();
		it8.setByte(0x7f); // ' '
		it8.next();
		// write 10s place
		it8.setByte(0xe0 + (n / 10));
		it8.next();
		// write 1s place
		it8.setByte(0xe0 + (n % 10));
	}

	// convert pc box themes
	js_info <<  "Converting PC box themes..." << std::endl;
	for (int n = 1; n < NUM_BOXES_V8 + 1; n++) {
		it8.seek(sym8.getSRAMAddress("sNewBox" + std::to_string(n) + "Theme"));
		uint8_t theme = it8.getByte();
		uint8_t theme_v8 = mapv7ThemeToV8(theme);
		if (theme != theme_v8) {
			js_info <<  "Theme " << std::hex << static_cast<int>(theme) << " converted to " << std::hex << static_cast<int>(theme_v8) << std::endl;
			it8.setByte(theme_v8);
		}
	}

	// clear the v8 sBackupNewBox space
	js_info <<  "Clearing v8 sBackupNewBox#..." << std::endl;
	for (int n = 1; n < NUM_BOXES_V8 + 1; n++) {
		// clear the backup newbox
		it8.seek(sym8.getSRAMAddress("sBackupNewBox" + std::to_string(n)));
		for (int i = 0; i < NEWBOX_SIZE; i++) {
			it8.setByte(0x00);
			it8.next();
		}
	}

	// copy the version 7 backup boxes to the version 8 backup boxes
	for (int n = 1; n < NUM_BOXES_V7 + 1; n++) {
		// copy the backup newbox
		it7.seek(sym7.getSRAMAddress("sBackupNewBox" + std::to_string(n)));
		it8.seek(sym8.getSRAMAddress("sBackupNewBox" + std::to_string(n)));
		for (int i = 0; i < NEWBOX_SIZE; i++) {
			it8.setByte(it7.getByte());
			it7.next();
			it8.next();
		}
	}

	// Write default backup box names from NUM_BOXES_V7 + 1 to NUM_BOXES_V8
	js_info <<  "Writing default backup box names..." << std::endl;
	for (int n = NUM_BOXES_V7 + 1; n < NUM_BOXES_V8 + 1; n++) {
		it8.seek(sym8.getSRAMAddress("sBackupNewBox" + std::to_string(n) + "Name"));
		js_info <<  "Writing default backup box name for sBackupNewBox" << n << "..." << std::endl;
		it8.setByte(0x7f); // ' '
		it8.next();
		it8.setByte(0x7f); // ' '
		it8.next();
		it8.setByte(0x81); // B
		it8.next();
		it8.setByte(0xae); // o
		it8.next();
		it8.setByte(0xb7); // x
		it8.next();
		it8.setByte(0x7f); // ' '
		it8.next();
		// write 10s place
		it8.setByte(0xe0 + (n / 10));
		it8.next();
		// write 1s place
		it8.setByte(0xe0 + (n % 10));
	}

	// convert backup pc box themes
	js_info <<  "Converting backup PC box themes..." << std::endl;
	for (int n = 1; n < NUM_BOXES_V8 + 1; n++) {
		it8.seek(sym8.getSRAMAddress("sBackupNewBox" + std::to_string(n) + "Theme"));
		uint8_t theme = it8.getByte();
		uint8_t theme_v8 = mapv7ThemeToV8(theme);
		if (theme != theme_v8) {
			js_info <<  "Theme " << std::hex << static_cast<int>(theme) << " converted to " << std::hex << static_cast<int>(theme_v8) << std::endl;
			it8.setByte(theme_v8);
		}
	}

	// copy sBoxMons1 to sBoxMons1A
	js_info <<  "Copying from sBoxMons1 to sBoxMons1A..." << std::endl;
	it7.seek(sym7.getSRAMAddress("sBoxMons1"));
	it8.seek(sym8.getSRAMAddress("sBoxMons1A"));
	it8.copy(it7, MONDB_ENTRIES_A_V8 * SAVEMON_STRUCT_LENGTH);

	// clear it8 sBoxMons1B
	js_info <<  "Clearing sBoxMons1B..." << std::endl;
	it8.seek(sym8.getSRAMAddress("sBoxMons1B"));
	for (int i = 0; i < MONDB_ENTRIES_B_V8; i++) {
		for (int j = 0; j < SAVEMON_STRUCT_LENGTH; j++) {
			it8.setByte(0x00);
			it8.next();
		}
	}

	// clear it8 sBoxMons1C
	js_info <<  "Clearing sBoxMons1C..." << std::endl;
	it8.seek(sym8.getSRAMAddress("sBoxMons1C"));
	for (int i = 0; i < MONDB_ENTRIES_C_V8; i++) {
		for (int j = 0; j < SAVEMON_STRUCT_LENGTH; j++) {
			it8.setByte(0x00);
			it8.next();
		}
	}

	// copy sBoxMons2 to SBoxMons2A
	js_info <<  "Copying from sBoxMons2 to sBoxMons2A..." << std::endl;
	it7.seek(sym7.getSRAMAddress("sBoxMons2"));
	it8.seek(sym8.getSRAMAddress("sBoxMons2A"));
	it8.copy(it7, MONDB_ENTRIES_A_V8 * SAVEMON_STRUCT_LENGTH);

	// clear it8 sBoxMons2B
	js_info <<  "Clearing sBoxMons2B..." << std::endl;
	it8.seek(sym8.getSRAMAddress("sBoxMons2B"));
	for (int i = 0; i < MONDB_ENTRIES_B_V8; i++) {
		for (int j = 0; j < SAVEMON_STRUCT_LENGTH; j++) {
			it8.setByte(0x00);
			it8.next();
		}
	}

	// clear it8 sBoxMons2C
	js_info <<  "Clearing sBoxMons2C..." << std::endl;
	it8.seek(sym8.getSRAMAddress("sBoxMons2C"));
	for (int i = 0; i < MONDB_ENTRIES_C_V8; i++) {
		for (int j = 0; j < SAVEMON_STRUCT_LENGTH; j++) {
			it8.setByte(0x00);
			it8.next();
		}
	}

	// Patching sBoxMons1A if checksums match
	js_info <<  "Checking sBoxMons1A checksums..." << std::endl;
	for (int i = 0; i < MONDB_ENTRIES_A_V8; i++) {
		it8.seek(sym8.getSRAMAddress("sBoxMons1A") + i * SAVEMON_STRUCT_LENGTH);
		uint16_t calc_checksum = calculateNewboxChecksum(save8, sym8.getSRAMAddress("sBoxMons1A") + i * SAVEMON_STRUCT_LENGTH);
		uint16_t cur_checksum = extractStoredNewboxChecksum(save8, sym8.getSRAMAddress("sBoxMons1A") + i * SAVEMON_STRUCT_LENGTH);
		if (calc_checksum == cur_checksum) {
			// patching
			uint16_t species = it8.getByte();
			it8.next();
			uint8_t item = it8.getByte();
			it8.seek(sym8.getSRAMAddress("sBoxMons1A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_CAUGHTLOCATION);
			uint8_t caught_location = it8.getByte();
			it8.seek(sym8.getSRAMAddress("sBoxMons1A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_CAUGHTBALL);
			uint8_t caught_ball = it8.getByte() & CAUGHT_BALL_MASK;
			// convert species
			uint16_t species_v8 = mapv7PkmntoV8(species);
			if (species_v8 == 0xFFFF) {
				js_error <<  "Species " << std::hex << species << " not found in version 8 pokemon list." << std::endl;
				continue;
			} else {
				if (species != species_v8) {
					js_info <<  "Species " << std::hex << species << " converted to " << std::hex << species_v8 << std::endl;
				}
				it8.seek(sym8.getSRAMAddress("sBoxMons1A") + i * SAVEMON_STRUCT_LENGTH);
				it8.setByte(species_v8 & 0xFF);
				it8.seek(sym8.getSRAMAddress("sBoxMons1A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_EXTSPECIES);
				uint8_t personality = it8.getByte();
				personality &= ~EXTSPECIES_MASK;
				personality |= (species_v8 >> 8) << MON_EXTSPECIES_F;
				uint8_t form = personality & FORM_MASK;
				if (species_v8 == 0x81) { // if species is Magikarp
					form = mapv7MagikarpFormToV8(form);
					personality &= ~FORM_MASK;
					personality |= form;
				}
				if (species_v8 == 0x82) { // if species is Gyarados
					if (form == 0x11){
						form = 0x15;
						personality &= ~FORM_MASK;
						personality |= form;
					}
				}
				it8.setByte(personality);
			}
			// convert item
			uint8_t item_v8 = mapv7ItemtoV8(item);
			if (item_v8 == 0xFF) {
				js_error <<  "Item " << std::hex << static_cast<int>(item) << " not found in version 8 item list." << std::endl;
			} else {
				if (item != item_v8) {
					js_info <<  "Item " << std::hex << static_cast<int>(item) << " converted to " << std::hex << static_cast<int>(item_v8) << std::endl;
				}
				it8.seek(sym8.getSRAMAddress("sBoxMons1A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_ITEM);
				it8.setByte(item_v8);
			}
			// convert caught location
			uint8_t caught_location_v8 = mapv7LandmarktoV8(caught_location);
			if (caught_location_v8 == 0xFF) {
				js_error <<  "Landmark " << std::hex << static_cast<int>(caught_location) << " not found in version 8 landmark list." << std::endl;
			} else {
				if (caught_location != caught_location_v8) {
					js_info <<  "Landmark " << std::hex << static_cast<int>(caught_location) << " converted to " << std::hex << static_cast<int>(caught_location_v8) << std::endl;
				}
				it8.seek(sym8.getSRAMAddress("sBoxMons1A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_CAUGHTLOCATION);
				it8.setByte(caught_location_v8);
			}
			// convert caught ball
			uint8_t caught_ball_v8 = mapv7ItemtoV8(caught_ball);
			if (caught_ball_v8 == 0xFF) {
				js_error <<  "Ball " << std::hex << static_cast<int>(caught_ball) << " not found in version 8 item list." << std::endl;
			} else {
				if (caught_ball != caught_ball_v8) {
					js_info <<  "Ball " << std::hex << static_cast<int>(caught_ball) << " converted to " << std::hex << static_cast<int>(caught_ball_v8) << std::endl;
				}
				it8.seek(sym8.getSRAMAddress("sBoxMons1A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_CAUGHTBALL);
				uint8_t caught_ball_byte = it8.getByte();
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
			// patching
			uint16_t species = it8.getByte();
			it8.next();
			uint8_t item = it8.getByte();
			it8.seek(sym8.getSRAMAddress("sBoxMons2A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_CAUGHTLOCATION);
			uint8_t caught_location = it8.getByte();
			it8.seek(sym8.getSRAMAddress("sBoxMons2A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_CAUGHTBALL);
			uint8_t caught_ball = it8.getByte() & CAUGHT_BALL_MASK;
			// convert species
			uint16_t species_v8 = mapv7PkmntoV8(species);
			if (species_v8 == 0xFFFF) {
				js_error <<  "Species " << std::hex << species << " not found in version 8 pokemon list." << std::endl;
				continue;
			} else {
				if (species != species_v8) {
					js_info <<  "Species " << std::hex << species << " converted to " << std::hex << species_v8 << std::endl;
				}
				it8.seek(sym8.getSRAMAddress("sBoxMons2A") + i * SAVEMON_STRUCT_LENGTH);
				it8.setByte(species_v8 & 0xFF);
				it8.seek(sym8.getSRAMAddress("sBoxMons2A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_EXTSPECIES);
				uint8_t personality = it8.getByte();
				personality &= ~EXTSPECIES_MASK;
				personality |= (species_v8 >> 8) << MON_EXTSPECIES_F;
				uint8_t form = personality & FORM_MASK;
				if (species_v8 == 0x81) { // if species is Magikarp
					form = mapv7MagikarpFormToV8(form);
					personality &= ~FORM_MASK;
					personality |= form;
				}
				if (species_v8 == 0x82) { // if species is Gyarados
					if (form == 0x11){
						form = 0x15;
						personality &= ~FORM_MASK;
						personality |= form;
					}
				}
				it8.setByte(personality);
			}
			// convert item
			uint8_t item_v8 = mapv7ItemtoV8(item);
			if (item_v8 == 0xFF) {
				js_error <<  "Item " << std::hex << static_cast<int>(item) << " not found in version 8 item list." << std::endl;
			} else {
				if (item != item_v8) {
					js_info <<  "Item " << std::hex << static_cast<int>(item) << " converted to " << std::hex << static_cast<int>(item_v8) << std::endl;
				}
				it8.seek(sym8.getSRAMAddress("sBoxMons2A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_ITEM);
				it8.setByte(item_v8);
			}
			// convert caught location
			uint8_t caught_location_v8 = mapv7LandmarktoV8(caught_location);
			if (caught_location_v8 == 0xFF) {
				js_error <<  "Landmark " << std::hex << static_cast<int>(caught_location) << " not found in version 8 landmark list." << std::endl;
			} else {
				if (caught_location != caught_location_v8) {
					js_info <<  "Landmark " << std::hex << static_cast<int>(caught_location) << " converted to " << std::hex << static_cast<int>(caught_location_v8) << std::endl;
				}
				it8.seek(sym8.getSRAMAddress("sBoxMons2A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_CAUGHTLOCATION);
				it8.setByte(caught_location_v8);
			}
			// convert caught ball
			uint8_t caught_ball_v8 = mapv7ItemtoV8(caught_ball);
			if (caught_ball_v8 == 0xFF) {
				js_error <<  "Ball " << std::hex << static_cast<int>(caught_ball) << " not found in version 8 item list." << std::endl;
			} else {
				if (caught_ball != caught_ball_v8) {
					js_info <<  "Ball " << std::hex << static_cast<int>(caught_ball) << " converted to " << std::hex << static_cast<int>(caught_ball_v8) << std::endl;
				}
				it8.seek(sym8.getSRAMAddress("sBoxMons2A") + i * SAVEMON_STRUCT_LENGTH + SAVEMON_CAUGHTBALL);
				uint8_t caught_ball_byte = it8.getByte();
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
	it7.seek(sym7.getSRAMAddress("sLinkBattleResults"));
	it8.seek(sym8.getSRAMAddress("sLinkBattleResults"));
	it8.copy(it7, sym7.getSRAMAddress("sLinkBattleStatsEnd") - sym7.getSRAMAddress("sLinkBattleResults"));

	// copy from sBattleTowerChallengeState to (sBT_OTMonParty3 + BATTLETOWER_PARTYDATA_SIZE + 1)
	js_info <<  "Copying from sBattleTowerChallengeState to sBT_OTMonParty3..." << std::endl;
	it7.seek(sym7.getSRAMAddress("sBattleTowerChallengeState"));
	it8.seek(sym8.getSRAMAddress("sBattleTowerChallengeState"));
	it8.copy(it7, sym7.getSRAMAddress("sBT_OTMonParty3") + BATTLETOWER_PARTYDATA_SIZE + 1 - sym7.getSRAMAddress("sBattleTowerChallengeState"));

	// copy from sPartyMail to sSaveVersion
	js_info <<  "Copying from sPartyMail to sSaveVersion..." << std::endl;
	it7.seek(sym7.getSRAMAddress("sPartyMail"));
	it8.seek(sym8.getSRAMAddress("sPartyMail"));
	it8.copy(it7, sym7.getSRAMAddress("sSaveVersion") - sym7.getSRAMAddress("sPartyMail"));

	// copy sUpgradeStep to sWritingBackup
	js_info <<  "Copying from sUpgradeStep to sWritingBackup..." << std::endl;
	it7.seek(sym7.getSRAMAddress("sUpgradeStep"));
	it8.seek(sym8.getSRAMAddress("sUpgradeStep"));
	it8.copy(it7, sym7.getSRAMAddress("sWritingBackup") + 1 - sym7.getSRAMAddress("sUpgradeStep"));

	// copy sRTCStatusFlags to sLuckyIDNumber
	js_info <<  "Copying from sRTCStatusFlags to sLuckyIDNumber..." << std::endl;
	it7.seek(sym7.getSRAMAddress("sRTCStatusFlags"));
	it8.seek(sym8.getSRAMAddress("sRTCStatusFlags"));
	it8.copy(it7, sym7.getSRAMAddress("sLuckyIDNumber") + 2 - sym7.getSRAMAddress("sRTCStatusFlags"));

	// copy sOptions to sGameData
	js_info <<  "Copying from sOptions to sGameData..." << std::endl;
	it7.seek(sym7.getSRAMAddress("sOptions"));
	it8.seek(sym8.getSRAMAddress("sOptions"));
	it8.copy(it7, sym7.getSRAMAddress("sGameData") - sym7.getSRAMAddress("sOptions"));

	js_info <<  "Copying from wPlayerData to wObjectStructs..." << std::endl;
	// copy bytes from wPlayerData to wObjectStructs - 1 from version 7 to version 8
	it7.seek(sym7.getPlayerDataAddress("wPlayerData"));
	it8.seek(sym8.getPlayerDataAddress("wPlayerData"));
	it8.copy(it7, sym7.getPlayerDataAddress("wObjectStructs") - sym7.getPlayerDataAddress("wPlayerData"));
	
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

	js_info <<  "Copying from wObjectStructsEnd to wBattleFactorySwapCount..." << std::endl;

	// copy bytes from wObjectStructsEnd to wBattleFactorySwapCount
	it7.seek(sym7.getPlayerDataAddress("wObjectStructsEnd"));
	it8.seek(sym8.getPlayerDataAddress("wObjectStructsEnd"));
	it8.copy(it7, sym7.getPlayerDataAddress("wBattleFactorySwapCount") + 1 - sym7.getPlayerDataAddress("wObjectStructsEnd"));

	js_info <<  "Copying from wMapObjects to wEnteredMapFromContinue..." << std::endl;

	// seek to wMapObjects
	it7.seek(sym7.getPlayerDataAddress("wMapObjects"));
	it8.seek(sym8.getPlayerDataAddress("wMapObjects"));
	// copy bytes to wEnteredMapFromContinue
	it8.copy(it7, sym7.getPlayerDataAddress("wEnteredMapFromContinue") - sym7.getPlayerDataAddress("wMapObjects"));
	
	js_info <<  "Copy wEnteredMapFromContinue" << std::endl;
	// copy it7 wEnteredMapFromContinue to it8 wEnteredMapFromContinue
	// assert that the address is correct
	if (it7.getAddress() != sym7.getPlayerDataAddress("wEnteredMapFromContinue")) {
		js_error <<  "Unexpected address for wEnteredMapFromContinue in version 7 save file: " << std::hex << it7.getAddress() << std::endl;
	}
	it8.seek(sym8.getPlayerDataAddress("wEnteredMapFromContinue"));
	it8.setByte(it7.getByte());
	js_info <<  "Copy wStatusFlags3" << std::endl;
	// copy it7 wStatusFlags3 to it8 wStatusFlags3
	it7.seek(sym7.getPlayerDataAddress("wStatusFlags3"));
	it8.seek(sym8.getPlayerDataAddress("wStatusFlags3"));
	it8.setByte(it7.getByte());

	js_info <<  "Copying from wTimeOfDayPal to wTMsHMsEnd..." << std::endl;
	// copy from it7 wTimeOfDayPal to it7 wTMsHMsEnd
	it7.seek(sym7.getPlayerDataAddress("wTimeOfDayPal"));
	it8.seek(sym8.getPlayerDataAddress("wTimeOfDayPal"));
	it8.copy(it7, sym7.getPlayerDataAddress("wTMsHMsEnd") - sym7.getPlayerDataAddress("wTimeOfDayPal"));

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
			uint8_t keyItemIndexV8 = mapv7KeyItemtoV8(keyItemIndex);
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
		uint8_t itemIDV8 = mapv7ItemtoV8(itemIDV7);
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
	it8.seek(sym8.getPlayerDataAddress("wNumItems"));
	it8.setByte(numItemsv8);

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
		uint8_t itemIDV8 = mapv7ItemtoV8(itemIDV7);
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
	it8.seek(sym8.getPlayerDataAddress("wNumMedicine"));
	it8.setByte(numMedicinev8);

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
		uint8_t itemIDV8 = mapv7ItemtoV8(itemIDV7);
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
	it8.seek(sym8.getPlayerDataAddress("wNumBalls"));
	it8.setByte(numBallsV8);

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
		uint8_t itemIDV8 = mapv7ItemtoV8(itemIDV7);
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
	it8.seek(sym8.getPlayerDataAddress("wNumBerries"));
	it8.setByte(numBerriesv8);

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
		uint8_t itemIDV8 = mapv7ItemtoV8(itemIDV7);
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
	it8.seek(sym8.getPlayerDataAddress("wNumPCItems"));
	it8.setByte(numPCItemsV8);

	js_info <<  "Copy wApricorns..." << std::endl;
	it7.seek(sym7.getPlayerDataAddress("wApricorns"));
	it8.seek(sym8.getPlayerDataAddress("wApricorns"));
	it8.copy(it7, NUM_APRICORNS);

	js_info <<  "Copy from wPokegearFlags to wAlways0SceneID..." << std::endl;
	it8.copy(it7, sym7.getPlayerDataAddress("wAlways0SceneID") - sym7.getPlayerDataAddress("wPokegearFlags"));

	js_info <<  "copy from wAlways0SceneID to wEcruteakHouseSceneID + 1..." << std::endl;
	it8.copy(it7, sym7.getPlayerDataAddress("wEcruteakHouseSceneID") + 1 - sym7.getPlayerDataAddress("wAlways0SceneID"));

	// clear wEcruteakPokecenter1FSceneID as it is no longer used
	js_info <<  "Clear wEcruteakPokecenter1FSceneID..." << std::endl;
	it8.setByte(0x00);
	it7.next();
	it8.next();

	// copy from wElmsLabSceneID to wEventFlags
	js_info <<  "Copy from wElmsLabSceneID to wEventFlags..." << std::endl;
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
			uint16_t eventFlagIndexV8 = mapv7EventFlagtoV8(eventFlagIndex);
			// if the event flag is found set the corresponding bit in it8
			if (eventFlagIndexV8 != 0xFFFF) {
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
	it7.seek(sym7.getPlayerDataAddress("wCurBox"));
	it8.seek(sym8.getPlayerDataAddress("wCurBox"));
	it8.setByte(it7.getByte());

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
	js_info <<  "Copy from wCelebiEvent to wCurMapCallbacksPointer + 2..." << std::endl;
	it7.seek(sym7.getPlayerDataAddress("wCelebiEvent"));
	it8.seek(sym8.getPlayerDataAddress("wCelebiEvent"));
	it8.copy(it7, sym7.getPlayerDataAddress("wCurMapCallbacksPointer") + 2 - sym7.getPlayerDataAddress("wCelebiEvent"));

	// copy from wDecoBed to wFruitTreeFlags
	js_info <<  "Copy from wDecoBed to wFruitTreeFlags..." << std::endl;
	it7.seek(sym7.getPlayerDataAddress("wDecoBed"));
	it8.seek(sym8.getPlayerDataAddress("wDecoBed"));
	it8.copy(it7, sym7.getPlayerDataAddress("wFruitTreeFlags") - sym7.getPlayerDataAddress("wDecoBed"));

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
			uint8_t landmarkFlagIndexV8 = mapv7LandmarktoV8(landmarkFlagIndex);
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
	it7.seek(sym7.getPlayerDataAddress("wLuckyNumberDayBuffer"));
	it8.seek(sym8.getPlayerDataAddress("wLuckyNumberDayBuffer"));
	it8.copy(it7, sym7.getPlayerDataAddress("wPhoneList") - sym7.getPlayerDataAddress("wLuckyNumberDayBuffer"));

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
	it7.seek(sym7.getPlayerDataAddress("wParkBallsRemaining"));
	it8.seek(sym8.getPlayerDataAddress("wParkBallsRemaining"));
	it8.copy(it7, sym7.getPlayerDataAddress("wPlayerDataEnd") - sym7.getPlayerDataAddress("wParkBallsRemaining"));

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
			uint8_t spawnIndexV8 = mapv7SpawntoV8(spawnIndex);
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
	it7.seek(sym7.getMapDataAddress("wDigWarpNumber"));
	it8.seek(sym8.getMapDataAddress("wDigWarpNumber"));
	it8.copy(it7, sym7.getMapDataAddress("wCurMapDataEnd") - sym7.getMapDataAddress("wDigWarpNumber"));

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
	it7.seek(sym7.getPokemonDataAddress("wPartyCount"));
	it8.seek(sym8.getPokemonDataAddress("wPartyCount"));
	it8.setByte(it7.getByte());

	// copy wPartyMons PARTYMON_STRUCT_LENGTH * PARTY_LENGTH
	js_info <<  "Copy wPartyMons..." << std::endl;
	it7.seek(sym7.getPokemonDataAddress("wPartyMons"));
	it8.seek(sym8.getPokemonDataAddress("wPartyMons"));
	it8.copy(it7, PARTYMON_STRUCT_LENGTH * PARTY_LENGTH);

	// fix the party mon species
	js_info <<  "Fix party mon species..." << std::endl;
	it8.seek(sym8.getPokemonDataAddress("wPartyMons"));
	for (int i = 0; i < PARTY_LENGTH; i++) {
		it8.seek(sym8.getPokemonDataAddress("wPartyMons") + i * PARTYMON_STRUCT_LENGTH);
		uint16_t species = it8.getByte();
		if (species == 0x00) {
			continue;
		}
		uint16_t speciesV8 = mapv7PkmntoV8(species);
		// warn if the species was not found
		if (speciesV8 == 0xFFFF) {
			js_error <<  "Species " << std::hex << species << " not found in version 8 species list." << std::endl;
			continue;
		}
		// print found speciesv7 and converted speciesv8
		if (species != speciesV8){
			js_info <<  "Species " << std::hex << species << " converted to " << std::hex << speciesV8 << std::endl;
		}
		// write the lower 8 bits of the species
		it8.setByte(speciesV8 & 0xFF);
		it8.seek(sym8.getPokemonDataAddress("wPartyMons") + i * PARTYMON_STRUCT_LENGTH + MON_EXTSPECIES);
		// get the 9th bit of the species
		uint8_t extSpecies = speciesV8 >> 8;
		extSpecies = extSpecies << MON_EXTSPECIES_F;
		uint8_t currentExtSpecies = it8.getByte();
		currentExtSpecies &= ~EXTSPECIES_MASK;
		currentExtSpecies |= extSpecies;
		uint8_t form = currentExtSpecies & FORM_MASK;
		if (speciesV8 == 0x81){
			form = mapv7MagikarpFormToV8(form);
			currentExtSpecies &= ~FORM_MASK;
			currentExtSpecies |= form;
		}
		if (speciesV8 == 0x82){
			if (form == 0x11){
				form = 0x15;
				currentExtSpecies &= ~FORM_MASK;
				currentExtSpecies |= form;
			}
		}
		it8.setByte(currentExtSpecies);
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
		uint8_t itemV8 = mapv7ItemtoV8(item);
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
		it8.seek(sym8.getPokemonDataAddress("wPartyMons") + i * PARTYMON_STRUCT_LENGTH + MON_CAUGHTBALL);
		uint8_t caughtBall = it8.getByte() & CAUGHT_BALL_MASK;
		uint8_t caughtBallV8 = mapv7ItemtoV8(caughtBall);
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
		uint8_t caughtLoc = it8.getByte();
		uint8_t caughtLocV8 = mapv7LandmarktoV8(caughtLoc);
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
	it7.seek(sym7.getPokemonDataAddress("wPartyMonOTs"));
	it8.seek(sym8.getPokemonDataAddress("wPartyMonOTs"));
	for (int i = 0; i < PARTY_LENGTH; i++) {
		it8.copy(it7, PLAYER_NAME_LENGTH + 3);
	}

	// copy wPartyMonNicknames PARTY_LENGTH * MON_NAME_LENGTH
	js_info <<  "Copy wPartyMonNicknames..." << std::endl;
	it7.seek(sym7.getPokemonDataAddress("wPartyMonNicknames"));
	it8.seek(sym8.getPokemonDataAddress("wPartyMonNicknames"));
	it8.copy(it7, MON_NAME_LENGTH * PARTY_LENGTH);

	// TODO: convert forms

	// wPokedexCaught is a flag_array of NUM_POKEMON_V7 bits. If v7 bit is set, lookup the bit index in the map and set the corresponding bit in v8
	js_info <<  "Patching wPokedexCaught..." << std::endl;
	it7.seek(sym7.getPokemonDataAddress("wPokedexCaught"));
	it8.seek(sym8.getPokemonDataAddress("wPokedexCaught"));
	for (int i = 0; i < NUM_POKEMON_V7; i++) {
		// check if the bit is set
		if (it7.getByte() & (1 << (i % 8))) {
			// get the pokemon index is equal to the bit index
			uint16_t pokemonIndex = i + 1;
			// map the version 7 pokemon to the version 8 pokemon
			uint16_t pokemonIndexV8 = mapv7PkmntoV8(pokemonIndex) - 1;
			// if the pokemon is found set the corresponding bit in it8
			if (pokemonIndexV8 != 0xFFFF) {
				// print found pokemonv7 and converted pokemonv8
				if (pokemonIndex != pokemonIndexV8 + 1){
					js_info <<  "Pokemon " << std::hex << static_cast<int>(pokemonIndex) << " converted to " << std::hex << static_cast<int>(pokemonIndexV8) << std::endl;
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

	// wPokedexSeen is a flag_array of NUM_POKEMON_V7 bits. If v7 bit is set, lookup the bit index in the map and set the corresponding bit in v8
	js_info <<  "Patching wPokedexSeen..." << std::endl;
	it7.seek(sym7.getPokemonDataAddress("wPokedexSeen"));
	it8.seek(sym8.getPokemonDataAddress("wPokedexSeen"));
	for (int i = 0; i < NUM_POKEMON_V7; i++) {
		// check if the bit is set
		if (it7.getByte() & (1 << (i % 8))) {
			// get the pokemon index is equal to the bit index
			uint16_t pokemonIndex = i + 1;
			// map the version 7 pokemon to the version 8 pokemon
			uint16_t pokemonIndexV8 = mapv7PkmntoV8(pokemonIndex) - 1;
			// if the pokemon is found set the corresponding bit in it8
			if (pokemonIndexV8 != 0xFFFF) {
				// print found pokemonv7 and converted pokemonv8
				if (pokemonIndex != pokemonIndexV8 + 1){
					js_info <<  "Pokemon " << std::hex << static_cast<int>(pokemonIndex) << " converted to " << std::hex << static_cast<int>(pokemonIndexV8) << std::endl;
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

	// copy wUnlockedUnowns
	js_info <<  "Copy wUnlockedUnowns..." << std::endl;
	it7.seek(sym7.getPokemonDataAddress("wUnlockedUnowns"));
	it8.seek(sym8.getPokemonDataAddress("wUnlockedUnowns"));
	it8.setByte(it7.getByte());

	// copy wDayCareMan to wBugContestSecondPartySpecies - 54
	js_info <<  "Copy wDayCareMan to wBugContestSecondPartySpecies - 54..." << std::endl;
	it7.seek(sym7.getPokemonDataAddress("wDayCareMan"));
	it8.seek(sym8.getPokemonDataAddress("wDayCareMan"));
	it8.copy(it7, sym7.getPokemonDataAddress("wBugContestSecondPartySpecies") - 54 - sym7.getPokemonDataAddress("wDayCareMan"));

	// fix wBreedMon1Species and wBreedMon1ExtSpecies
	js_info <<  "Fix wBreedMon1Species..." << std::endl;
	it8.seek(sym8.getPokemonDataAddress("wBreedMon1Species"));
	uint16_t species = it8.getByte();
	if (species != 0x00) {
		uint16_t speciesV8 = mapv7PkmntoV8(species);
		// warn if the species was not found
		if (speciesV8 == 0xFFFF) {
			js_error <<  "Species " << std::hex << species << " not found in version 8 species list." << std::endl;
		}
		// print found speciesv7 and converted speciesv8
		if (species != speciesV8){
			js_info <<  "Species " << std::hex << species << " converted to " << std::hex << speciesV8 << std::endl;
		}
		// write the lower 8 bits of the species
		it8.setByte(speciesV8 & 0xFF);
		it8.seek(sym8.getPokemonDataAddress("wBreedMon1ExtSpecies"));;
		// get the 9th bit of the species
		uint8_t extSpecies = speciesV8 >> 8;
		extSpecies = extSpecies << MON_EXTSPECIES_F;
		uint8_t currentExtSpecies = it8.getByte();
		currentExtSpecies &= ~EXTSPECIES_MASK;
		currentExtSpecies |= extSpecies;
		uint8_t form = currentExtSpecies & FORM_MASK;
		if (speciesV8 == 0x81){
			form = mapv7MagikarpFormToV8(form);
			currentExtSpecies &= ~FORM_MASK;
			currentExtSpecies |= form;
		}
		if (speciesV8 == 0x82){
			if (form == 0x11){
				form = 0x15;
				currentExtSpecies &= ~FORM_MASK;
				currentExtSpecies |= form;
			}
		}
		it8.setByte(currentExtSpecies);
	}

	// fix wBreedMon1Item
	js_info <<  "Fix wBreedMon1Item..." << std::endl;
	it8.seek(sym8.getPokemonDataAddress("wBreedMon1Item"));
	uint8_t item = it8.getByte();
	if (item != 0x00) {
		uint8_t itemV8 = mapv7ItemtoV8(item);
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
	it8.seek(sym8.getPokemonDataAddress("wBreedMon1CaughtBall"));
	uint8_t caughtBall = it8.getByte() & CAUGHT_BALL_MASK;
	uint8_t caughtBallV8 = mapv7ItemtoV8(caughtBall);
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
	it8.seek(sym8.getPokemonDataAddress("wBreedMon1CaughtLocation"));
	uint8_t caughtLoc = it8.getByte();
	uint8_t caughtLocV8 = mapv7LandmarktoV8(caughtLoc);
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
	it8.seek(sym8.getPokemonDataAddress("wBreedMon2Species"));
	species = it8.getByte();
	if (species != 0x00) {
		uint16_t speciesV8 = mapv7PkmntoV8(species);
		// warn if the species was not found
		if (speciesV8 == 0xFFFF) {
			js_error <<  "Species " << std::hex << species << " not found in version 8 species list." << std::endl;
		}
		// print found speciesv7 and converted speciesv8
		if (species != speciesV8){
			js_info <<  "Species " << std::hex << species << " converted to " << std::hex << speciesV8 << std::endl;
		}
		// write the lower 8 bits of the species
		it8.setByte(speciesV8 & 0xFF);
		it8.seek(sym8.getPokemonDataAddress("wBreedMon2ExtSpecies"));;
		// get the 9th bit of the species
		uint8_t extSpecies = speciesV8 >> 8;
		extSpecies = extSpecies << MON_EXTSPECIES_F;
		uint8_t currentExtSpecies = it8.getByte();
		currentExtSpecies &= ~EXTSPECIES_MASK;
		currentExtSpecies |= extSpecies;
		uint8_t form = currentExtSpecies & FORM_MASK;
		if (speciesV8 == 0x81){
			form = mapv7MagikarpFormToV8(form);
			currentExtSpecies &= ~FORM_MASK;
			currentExtSpecies |= form;
		}
		if (speciesV8 == 0x82){
			if (form == 0x11){
				form = 0x15;
				currentExtSpecies &= ~FORM_MASK;
				currentExtSpecies |= form;
			}
		}
		it8.setByte(currentExtSpecies);
	}

	// fix wBreedMon2Item
	js_info <<  "Fix wBreedMon2Item..." << std::endl;
	it8.seek(sym8.getPokemonDataAddress("wBreedMon2Item"));
	item = it8.getByte();
	if (item != 0x00) {
		uint8_t itemV8 = mapv7ItemtoV8(item);
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
	it8.seek(sym8.getPokemonDataAddress("wBreedMon2CaughtBall"));
	caughtBall = it8.getByte() & CAUGHT_BALL_MASK;
	caughtBallV8 = mapv7ItemtoV8(caughtBall);
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
	it8.seek(sym8.getPokemonDataAddress("wBreedMon2CaughtLocation"));
	caughtLoc = it8.getByte();
	caughtLocV8 = mapv7LandmarktoV8(caughtLoc);
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
	it7.seek(sym7.getPokemonDataAddress("wBugContestSecondPartySpecies"));
	it8.seek(sym8.getPokemonDataAddress("wBugContestBackupPartyCount"));
	it8.setByte(it7.getByte());

	// copy from wContestMon to wPokemonDataEnd
	js_info <<  "Copy from wContestMon to wPokemonDataEnd..." << std::endl;
	it7.seek(sym7.getPokemonDataAddress("wContestMon"));
	it8.seek(sym8.getPokemonDataAddress("wContestMon"));
	it8.copy(it7, sym7.getPokemonDataAddress("wPokemonDataEnd") - sym7.getPokemonDataAddress("wContestMon"));

	// fix wContestMonSpecies and wContestMonExtSpecies
	js_info <<  "Fix wContestMonSpecies..." << std::endl;
	it8.seek(sym8.getPokemonDataAddress("wContestMonSpecies"));
	species = it8.getByte();
	if (species != 0x00) {
		uint16_t speciesV8 = mapv7PkmntoV8(species);
		// warn if the species was not found
		if (speciesV8 == 0xFFFF) {
			js_error <<  "Species " << std::hex << species << " not found in version 8 species list." << std::endl;
		}
		// print found speciesv7 and converted speciesv8
		if (species != speciesV8){
			js_info <<  "Species " << std::hex << species << " converted to " << std::hex << speciesV8 << std::endl;
		}
		// write the lower 8 bits of the species
		it8.setByte(speciesV8 & 0xFF);
		it8.seek(sym8.getPokemonDataAddress("wContestMonExtSpecies"));;
		// get the 9th bit of the species
		uint8_t extSpecies = speciesV8 >> 8;
		extSpecies = extSpecies << MON_EXTSPECIES_F;
		uint8_t currentExtSpecies = it8.getByte();
		currentExtSpecies &= ~EXTSPECIES_MASK;
		currentExtSpecies |= extSpecies;
		uint8_t form = currentExtSpecies & FORM_MASK;
		if (speciesV8 == 0x81){
			form = mapv7MagikarpFormToV8(form);
			currentExtSpecies &= ~FORM_MASK;
			currentExtSpecies |= form;
		}
		if (speciesV8 == 0x82){
			if (form == 0x11){
				form = 0x15;
				currentExtSpecies &= ~FORM_MASK;
				currentExtSpecies |= form;
			}
		}
		it8.setByte(currentExtSpecies);
	}

	// fix wContestMonItem
	js_info <<  "Fix wContestMonItem..." << std::endl;
	it8.seek(sym8.getPokemonDataAddress("wContestMonItem"));
	item = it8.getByte();
	if (item != 0x00) {
		uint8_t itemV8 = mapv7ItemtoV8(item);
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
	it8.seek(sym8.getPokemonDataAddress("wContestMonCaughtBall"));
	caughtBall = it8.getByte() & CAUGHT_BALL_MASK;
	caughtBallV8 = mapv7ItemtoV8(caughtBall);
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
	it8.seek(sym8.getPokemonDataAddress("wContestMonCaughtLocation"));
	caughtLoc = it8.getByte();
	caughtLocV8 = mapv7LandmarktoV8(caughtLoc);
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
	js_info <<  "Map wDunsparceMapGroup and wDunsparceMapNumber..." << std::endl;
	it7.seek(sym7.getPokemonDataAddress("wDunsparceMapGroup"));
	it8.seek(sym8.getPokemonDataAddress("wDunsparceMapGroup"));
	uint8_t dunsparceMapGroup = it7.getByte();
	it7.next();
	uint8_t dunsparceMapNumber = it7.getByte();
	// create tuple to store the map group and map number
	std::tuple<uint8_t, uint8_t> dunsparceMap = mapv7toV8(dunsparceMapGroup, dunsparceMapNumber);
	// print found dunsparce mapv7 and converted dunsparce mapv8
	if (dunsparceMapGroup != std::get<0>(dunsparceMap) || dunsparceMapNumber != std::get<1>(dunsparceMap)){
		js_info <<  "Dunsparce Map " << std::hex << static_cast<int>(dunsparceMapGroup) << " " << std::hex << static_cast<int>(dunsparceMapNumber) << " converted to " << std::hex << static_cast<int>(std::get<0>(dunsparceMap)) << " " << std::hex << static_cast<int>(std::get<1>(dunsparceMap)) << std::endl;
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
	it7.seek(sym7.getSRAMAddress("sCheckValue2"));
	it8.seek(sym8.getSRAMAddress("sCheckValue2"));
	it8.setByte(it7.getByte());

	// copy it8 Save to it8 Backup Save
	js_info <<  "Copy Save to Backup Save..." << std::endl;
	for (int i = 0; i < sym8.getSRAMAddress("sCheckValue2") + 1 - sym8.getSRAMAddress("sOptions"); i++) {
		save8.setByte(sym8.getSRAMAddress("sBackupOptions") + i, save8.getByte(sym8.getSRAMAddress("sOptions") + i));
	}

	// TODO: determine how we want to truncate NUM_HOF_TEAMS from 30 to 10.
	// copy from sHallOfFame to sHallOfFameEnd
	js_info <<  "Copy from sHallOfFame to sHallOfFameEnd..." << std::endl;
	it7.seek(sym7.getSRAMAddress("sHallOfFame"));
	it8.seek(sym8.getSRAMAddress("sHallOfFame"));
	it8.copy(it7, sym8.getSRAMAddress("sHallOfFameEnd") - sym8.getSRAMAddress("sHallOfFame")); // only copy as many as v8 can hold.

	// fix the hall of fame mon species
	js_info <<  "Fix hall of fame mon species..." << std::endl;
	for (int i = 0; i < NUM_HOF_TEAMS_V8; i++) {
		for (int j = 0; j < PARTY_LENGTH; j++){
			it8.seek(sym8.getSRAMAddress("sHallOfFame01Mon1") + i * HOF_LENGTH);
			it8.seek(it8.getAddress() + j * HOF_MON_LENGTH);
			uint16_t species = it8.getByte();
			if (species == 0x00) {
				continue;
			}
			uint16_t speciesV8 = mapv7PkmntoV8(species);
			// warn if the species was not found
			if (speciesV8 == 0xFFFF) {
				js_error <<  "Species " << std::hex << species << " not found in version 8 species list." << std::endl;
				continue;
			}
			// print found speciesv7 and converted speciesv8
			if (species != speciesV8){
				js_info <<  "Species " << std::hex << species << " converted to " << std::hex << speciesV8 << std::endl;
			}
			// write the lower 8 bits of the species
			it8.setByte(speciesV8 & 0xFF);
			it8.seek(it8.getAddress() + 4);
			// get the 9th bit of the species
			uint8_t extSpecies = speciesV8 >> 8;
			extSpecies = extSpecies << MON_EXTSPECIES_F;
			uint8_t currentExtSpecies = it8.getByte();
			currentExtSpecies &= ~EXTSPECIES_MASK;
			currentExtSpecies |= extSpecies;
			uint8_t form = currentExtSpecies & FORM_MASK;
			if (speciesV8 == 0x81){
				form = mapv7MagikarpFormToV8(form);
				currentExtSpecies &= ~FORM_MASK;
				currentExtSpecies |= form;
			}
			if (speciesV8 == 0x82){
				if (form == 0x11){
					form = 0x15;
					currentExtSpecies &= ~FORM_MASK;
					currentExtSpecies |= form;
				}
			}
			it8.setByte(currentExtSpecies);
		}
	}

	// write the new save version number big endian
	js_info <<  "Write new save version number..." << std::endl;
	uint16_t new_save_version = 0x08;
	save8.setWordBE(SAVE_VERSION_ABS_ADDRESS, new_save_version);

	// write new checksums to the version 8 save file
	js_info <<  "Write new checksums..." << std::endl;
	uint16_t new_checksum = calculate_checksum(save8, sym8.getSRAMAddress("sGameData"), sym8.getSRAMAddress("sGameDataEnd"));
	save8.setWord(SAVE_CHECKSUM_ABS_ADDRESS, new_checksum);

	// write new backup checksums to the version 8 save file
	js_info <<  "Write new backup checksums..." << std::endl;
	uint16_t new_backup_checksum = calculate_checksum(save8, sym8.getSRAMAddress("sBackupGameData"), sym8.getSRAMAddress("sBackupGameDataEnd"));
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
