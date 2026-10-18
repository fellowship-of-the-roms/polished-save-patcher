#include "patching/PatchVersion10to11.h"
#include "patching/Version11Maps.h"
#include "core/CommonPatchFunctions.h"
#include "core/SymbolDatabaseContents.h"
#include <utility>

namespace patchVersion10to11Namespace {
namespace {

// Polished Crystal v3.2.3 -> v3.3.0. See docs/v3.3.0-save-audit.md for
// provenance and the distinction between species/form bytes and dex indexes.
constexpr uint8_t ARBOK = 24;
constexpr uint8_t FORM_MASK = 0x1f;
constexpr uint8_t EXTSPECIES_MASK = 0x20;
constexpr unsigned PARTY_LENGTH = 6;
constexpr unsigned PARTY_MON_LENGTH = 48;
constexpr unsigned SAVEMON_LENGTH = 49;
constexpr unsigned FORM_OFFSET = 21;
constexpr unsigned DEX_INSERT_BIT = 0x140 - 1; // new Orange Arbok, indexes are species - 1
constexpr unsigned OLD_DEX_BITS = 0x189;
constexpr unsigned SPAWN_INSERT_BIT = 15;
constexpr unsigned OLD_SPAWN_BITS = 34;
constexpr unsigned FIRST_NEW_EVENT = 2259;
constexpr unsigned LAST_NEW_EVENT = 2286;
constexpr unsigned FIRST_SWITCH_EVENT = 606;
constexpr unsigned LAST_SWITCH_EVENT = 620;
constexpr unsigned EMERGENCY_SWITCH_EVENT = 609;
constexpr unsigned EMERGENCY_DOOR_EVENT = 620;
constexpr unsigned BETA_IN_NAVEL_ROCK_EVENT = 2259;
constexpr unsigned CINNABAR_LAB_BETA_EVENT = 2261;
constexpr unsigned FIREBREATHER_ASHES_EVENT = 2270;

void clear(SaveBinary& save, uint32_t start, uint32_t end) {
	for (auto address = start; address < end; ++address) save.setByte(address, 0);
}

bool flag(const SaveBinary& save, uint32_t address, unsigned bit) {
	return (save.getByte(address + bit / 8) & (1u << (bit % 8))) != 0;
}

void flag(SaveBinary& save, uint32_t address, unsigned bit, bool value) {
	const auto byte = address + bit / 8;
	const auto mask = 1u << (bit % 8);
	save.setByte(byte, (save.getByte(byte) & ~mask) | (value ? mask : 0));
}

void insertFlag(const SaveBinary& source, SaveBinary& dest, uint32_t address,
	unsigned insert, unsigned oldBits, unsigned bytes) {
	clear(dest, address, address + bytes);
	for (unsigned bit = 0; bit < oldBits; ++bit) {
		flag(dest, address, bit < insert ? bit : bit + 1, flag(source, address, bit));
	}
}

bool patchArbok(SaveBinary& save, uint32_t speciesAddress, uint32_t formAddress) {
	const auto form = save.getByte(formAddress);
	const auto value = form & FORM_MASK;
	if (save.getByte(speciesAddress) != ARBOK || (form & EXTSPECIES_MASK) || value < 3 || value > 5) {
		return false;
	}
	save.setByte(formAddress, (form & ~FORM_MASK) | (value + 1));
	return true;
}

void patchMap(SaveBinary& save, uint32_t address) {
	const auto group = save.getByte(address);
	const auto number = save.getByte(address + 1);
	for (const auto& map : mapChanges) {
		if (group == map.oldGroup && number == map.oldNumber) {
			save.setByte(address, map.newGroup);
			save.setByte(address + 1, map.newNumber);
			return; // Apply exactly once, including changes whose output is another old ID.
		}
	}
}

void patchBoxRegion(SaveBinary& save, uint32_t start, uint32_t end) {
	for (auto address = start; address < end; address += SAVEMON_LENGTH) {
		// Unallocated database slots may contain garbage; do not legitimize a Bad Egg.
		if (calculateNewboxChecksum(save, address) != extractStoredNewboxChecksum(save, address)
			|| (save.getByte(address + SAVEMON_LENGTH - 1) & 0x80)) continue;
		if (patchArbok(save, address, address + FORM_OFFSET)) writeNewboxChecksum(save, address);
	}
}

void patchFacilityParty(SaveBinary& save, uint32_t address) {
	// PR #1566 inserts Bloodmoon Ursaluna into tiers B and A. Each saved
	// Pokemon is a (tier, zero-based set index) pair, not a species/form pair.
	for (unsigned i = 0; i < 3; ++i, address += 2) {
		const auto tier = save.getByte(address);
		const auto index = save.getByte(address + 1);
		if ((tier == 2 && index >= 101 && index < 143)
			|| (tier == 3 && index >= 51 && index < 88)) {
			save.setByte(address + 1, index + 1);
		}
		// Leave uninitialized (-1) and out-of-range input pairs unchanged.
	}
}

} // namespace

bool patchVersion10to11(SaveBinary& save10, SaveBinary& save11) {
	SymbolDatabase sym(version11_sym_data, version11_sym_len);
	const auto player = [&sym](const char* name) { return sym.getPlayerDataAddress(name); };
	const auto pokemon = [&sym](const char* name) { return sym.getPokemonDataAddress(name); };
	const auto map = [&sym](const char* name) { return sym.getMapDataAddress(name); };
	const auto sram = [&sym](const char* name) { return sym.getSRAMAddress(name); };

	if (save10.getWordBE(SAVE_VERSION_ABS_ADDRESS) != 10) {
		js_error << "The v3.3.0 migration requires a version 10 save." << std::endl;
		return false;
	}
	if (save10.getWord(sram("sChecksum")) != calculateSaveChecksum(save10, sram("sGameData"), sram("sGameDataEnd"))
		|| save10.getWord(sram("sBackupChecksum")) != calculateSaveChecksum(save10, sram("sBackupGameData"), sram("sBackupGameDataEnd"))) {
		js_error << "Primary or backup checksum mismatch. Re-save in v3.2.3 before patching." << std::endl;
		return false;
	}
	if (save10.getByte(sram("sWritingBackup")) == 1) {
		js_error << "The save was interrupted while writing its backup. Re-save in v3.2.3 before patching." << std::endl;
		return false;
	}
	if (save10.getByte(map("wMapGroup")) != MON_CENTER_2F_GROUP
		|| save10.getByte(map("wMapNumber")) != MON_CENTER_2F_MAP) {
		js_error << "Save upstairs in a Pokemon Center near the stairs before patching to v3.3.0." << std::endl;
		return false;
	}
	if (save10.getByte(pokemon("wPartyCount")) > PARTY_LENGTH) {
		js_error << "Invalid party count." << std::endl;
		return false;
	}

	save11 = save10;
	js_info << "Migrating v3.2.3 save data to v3.3.0 (save version 11)..." << std::endl;
	for (const auto name : {"wPokedexCaught", "wPokedexSeen"}) {
		insertFlag(save10, save11, pokemon(name), DEX_INSERT_BIT, OLD_DEX_BITS, 50);
	}
	insertFlag(save10, save11, map("wVisitedSpawns"), SPAWN_INSERT_BIT, OLD_SPAWN_BITS, 5);
	// An existing Indigo Plateau Fly point proves the League approach was reached.
	flag(save11, map("wVisitedSpawns"), SPAWN_INSERT_BIT, flag(save10, map("wVisitedSpawns"), 14));

	js_info << "Updating Arbok forms in the party, Day Care, boxes, and Hall of Fame..." << std::endl;
	for (unsigned i = 0; i < PARTY_LENGTH; ++i) {
		const auto address = pokemon("wPartyMon1") + i * PARTY_MON_LENGTH;
		patchArbok(save11, address, address + FORM_OFFSET);
	}
	for (const auto name : {"wBreedMon1", "wBreedMon2", "wLevelUpMon", "wContestMon"}) {
		patchArbok(save11, pokemon(name), pokemon(name) + FORM_OFFSET);
	}
	for (const auto name : {"wRoamMon1", "wRoamMon2", "wRoamMon3"}) {
		patchArbok(save11, pokemon(name), pokemon(name) + 9);
		const auto species = save10.getByte(pokemon(name));
		if (species >= 243 && species <= 245) { // Raikou, Entei, Suicune
			// PR #1301 starts reading this previously uninitialized byte, including
			// its extended-species bit. All three legendary roamers use PLAIN_FORM.
			save11.setByte(pokemon(name) + 9, 1);
		}
	}
	for (auto address = player("wHiddenGrottoContents"); address < player("wLastMapYCoord"); address += 3) {
		if (save11.getByte(address) == 1) patchArbok(save11, address + 1, address + 2); // GROTTO_POKEMON
	}
	for (const auto& region : {
		std::pair<const char*, const char*>{"sBoxMons1AMons", "sBoxMons1AEnd"},
		{"sBoxMons1BMons", "sBoxMons1BEnd"}, {"sBoxMons1CMons", "sBoxMons1CEnd"},
		{"sBoxMons2AMons", "sBoxMons2AEnd"}, {"sBoxMons2BMons", "sBoxMons2BEnd"},
		{"sBoxMons2CMons", "sBoxMons2CEnd"}}) {
		patchBoxRegion(save11, sram(region.first), sram(region.second));
	}
	for (auto team = sram("sHallOfFame"); team < sram("sHallOfFameEnd"); team += 98) {
		for (unsigned i = 0; i < PARTY_LENGTH; ++i) {
			const auto mon = team + 1 + i * 16;
			patchArbok(save11, mon, mon + 4);
		}
	}

	js_info << "Updating map references and the Pokemon Center return warp..." << std::endl;
	for (const auto name : {"wDigMapGroup", "wBackupMapGroup", "wLastSpawnMapGroup"}) patchMap(save11, map(name));
	patchMap(save11, player("wYanmaMapGroup"));
	for (const auto name : {"wDunsparceMapGroup", "wRoamMon1MapGroup", "wRoamMon2MapGroup", "wRoamMon3MapGroup"}) {
		patchMap(save11, pokemon(name));
	}
	const CenterWarp* center = nullptr;
	for (const auto& candidate : centerWarps) {
		if (candidate.group == save11.getByte(map("wBackupMapGroup"))
			&& candidate.number == save11.getByte(map("wBackupMapNumber"))) center = &candidate;
	}
	if (!center) {
		// Older patch chains sometimes returned to the player's house. A known
		// center also avoids using stale warp indexes from removed/split maps.
		const bool hasPlainBadge = (save11.getByte(player("wJohtoBadges")) & 0x04) != 0;
		js_warning << "Invalid upstairs return map; redirecting the stairs to "
			<< (hasPlainBadge ? "Goldenrod" : "Cherrygrove") << " Pokemon Center." << std::endl;
		for (const auto& candidate : centerWarps) {
			if (candidate.group == (hasPlainBadge ? 11 : 26)
				&& candidate.number == (hasPlainBadge ? 24 : 6)) center = &candidate;
		}
	}
	if (!center) return false;
	save11.setByte(map("wBackupMapGroup"), center->group);
	save11.setByte(map("wBackupMapNumber"), center->number);
	save11.setByte(map("wBackupWarpNumber"), center->warp);
	save11.setByte(map("wDigMapGroup"), center->outsideGroup);
	save11.setByte(map("wDigMapNumber"), center->outsideNumber);
	save11.setByte(map("wDigWarpNumber"), center->outsideWarp);

	// Existing named events retain their numeric IDs (including Dick -> Cyd).
	// Newly allocated events must not inherit unused bits from an old save.
	const auto events = player("wEventFlags");
	for (unsigned bit = FIRST_NEW_EVENT; bit <= LAST_NEW_EVENT; ++bit) flag(save11, events, bit, false);
	flag(save11, events, BETA_IN_NAVEL_ROCK_EVENT, true);
	flag(save11, events, FIREBREATHER_ASHES_EVENT, true);
	// The new avatar's time-travel double is never visible for an old avatar.
	flag(save11, events, CINNABAR_LAB_BETA_EVENT, true);
	for (unsigned bit = FIRST_SWITCH_EVENT; bit <= LAST_SWITCH_EVENT; ++bit) flag(save11, events, bit, false);
	flag(save11, events, EMERGENCY_DOOR_EVENT, flag(save10, events, EMERGENCY_SWITCH_EVENT));
	// The old switch-position byte is now follower synchronization state.
	save11.setByte(player("wFollowInSync"), 0);
	const auto opponent = save10.getByte(player("wDailyTrainerHouseOpponent"));
	if (opponent == 4 || opponent == 5) save11.setByte(player("wDailyTrainerHouseOpponent"), opponent + 1);

	// Previously unused bytes now hold persistent weather/palette state.
	clear(save11, player("wPaletteSwapAddress"), player("wTimeOfDayPalFlags"));
	clear(save11, player("wOvercastRandomDay"), player("wCandyAmounts"));
	save11.setByte(player("wOvercastRandomDay"), 0xff); // generate weather on first load
	for (const auto name : {"sOptions3", "sBackupOptions3"}) {
		save11.setByte(sram(name), save11.getByte(sram(name)) & ~0x06); // default nickname prompt
	}
	const auto textboxFlags = sym.getOptionsAddress("wTextboxFlags") - sram("sOptions");
	for (const auto name : {"sOptions", "sBackupOptions"}) {
		const auto address = sram(name) + textboxFlags;
		// PR #1390 uses a formerly unused saved bit for temporary menu text.
		save11.setByte(address, save11.getByte(address) & ~0x20); // NO_TEXT_PAUSE_F
	}

	// Continue reloads map headers but keeps saved objects and their script
	// pointers. Remove the NPC cache until the player exits and re-enters.
	clear(save11, player("wObject1Struct"), player("wObjectStructsEnd"));
	clear(save11, player("wMap1Object"), player("wMapObjectsEnd"));
	clear(save11, player("wObjectMasks"), player("wVariableSprites"));
	save11.setByte(player("wPlayerStepType"), 0); // STEP_TYPE_RESET
	save11.setByte(player("wPlayerState"), 0); // walking indoors
	save11.setByte(player("wPlayerFlags") + 1, save11.getByte(player("wPlayerFlags") + 1) & ~0x98);
	save11.setByte(player("wPlayerPalette"), save11.getByte(player("wPlayerPalette")) & ~0x10);
	save11.setByte(player("wObjectFollow_Leader"), 0xff);
	save11.setByte(player("wObjectFollow_Follower"), 0xff);
	save11.setByte(player("wFollowerMovementQueueLength"), 0);
	save11.setWord(player("wCurMapSceneScriptPointer"), 0);
	save11.setByte(player("wPokecenter2FSceneID"), 0);
	for (auto address = player("wVariableSprites"); address < player("wStatusFlags3"); ++address) {
		if (save11.getByte(address) == 0xef) save11.setByte(address, 0xee); // SPRITE_MON_ICON
	}
	// In normal Tower mode this SRAM union contains player party selections,
	// not rental set IDs. Trainer table indexes, streaks, and rewards are stable.
	if (save10.getByte(sram("sBattleTowerChallengeState")) & 0x08) { // rental mode
		patchFacilityParty(save11, sram("sBT_MonParty"));
	}
	for (const auto name : {"sBT_OTMonParty1", "sBT_OTMonParty2", "sBT_OTMonParty3"}) {
		patchFacilityParty(save11, sram(name));
	}

	// As in earlier migrations, make the backup agree with the migrated primary.
	const auto start = sram("sGameData");
	const auto end = sram("sGameDataEnd");
	const auto backup = sram("sBackupGameData");
	for (auto i = 0u; i < end - start; ++i) save11.setByte(backup + i, save11.getByte(start + i));
	save11.setWord(sram("sChecksum"), calculateSaveChecksum(save11, start, end));
	save11.setWord(sram("sBackupChecksum"), calculateSaveChecksum(save11, backup, sram("sBackupGameDataEnd")));
	save11.setWordBE(SAVE_VERSION_ABS_ADDRESS, 11);
	js_info << "Migrated to v3.3.0. Exit the Pokemon Center upstairs before interacting with anything." << std::endl;
	return true;
}

} // namespace patchVersion10to11Namespace
