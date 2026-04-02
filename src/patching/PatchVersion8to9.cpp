#include "patching/PatchVersion8to9.h"
#include "patching/schemas/V8toV9Schema.h"
#include "core/MigrationEngine.h"
#include "core/CommonPatchFunctions.h"
#include "core/SymbolDatabase.h"
#include "core/Logging.h"
#include "core/SymbolDatabaseContents.h"

namespace patchVersion8to9Namespace {

	bool patchVersion8to9(SaveBinary& save8, SaveBinary& save9) {
		// Copy the old save file to the new save file
		save9 = save8;

		// Create iterators and load symbol databases
		SaveBinary::Iterator it8(save8, 0);
		SaveBinary::Iterator it9(save9, 0);
		SymbolDatabase sym8(version8_sym_data, version8_sym_len);
		SymbolDatabase sym9(version9_sym_data, version9_sym_len);
		SourceDest sd = { it8, it9, sym8, sym9 };

		// --- Common pre-migration validation ---
		if (!validateChecksums(save8, sym8)) return false;
		if (!checkPlayerInPokemonCenter2F(it8, sym8)) return false;

		// clear unused bytes after wRTC, [wRTC + 4, wRTC + 8)
		js_info << "Clearing 4 unused bytes after wRTC" << std::endl;
		clearDataBlock(sd, sym9.getPlayerDataAddress("wRTC") + 4, 4);

		// clear unused bytes after wTimeOfDayPal, [wTimeOfDayPal + 1, wTimeOfDayPal + 5)
		js_info << "Clearing 4 unused bytes after wTimeOfDayPal" << std::endl;
		clearDataBlock(sd, sym8.getPlayerDataAddress("wTimeOfDayPal") + 1, 4);

		// --- Schema-driven key item remapping ---
		remapKeyItems(it8, it9, sym8, sym9,
			v8toV9Schema::KEY_ITEM_MAPPINGS,
			v8toV9Schema::NUM_KEY_ITEM_MAPPINGS,
			v8toV9Schema::NUM_KEY_ITEMS_V9, sd);

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

		// --- Schema-driven event flag remapping ---
		remapEventFlags(it8, it9, sym8, sym9,
			v8toV9Schema::EVENT_FLAG_MAPPINGS,
			v8toV9Schema::NUM_EVENT_FLAG_MAPPINGS,
			v8toV9Schema::NUM_EVENTS, sd);

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

		// Reset the PGO battle event flags (these must be re-fought in v9)
		js_info << "Resetting PGO battle event flags..." << std::endl;
		js_info << "Clearing flag " << std::hex << v8toV9Schema::EVENT_BEAT_CANDELA << std::endl;
		clearFlagBit(it9, sym9.getPlayerDataAddress("wEventFlags"), v8toV9Schema::EVENT_BEAT_CANDELA);
		js_info << "Clearing flag " << std::hex << v8toV9Schema::EVENT_BEAT_BLANCHE << std::endl;
		clearFlagBit(it9, sym9.getPlayerDataAddress("wEventFlags"), v8toV9Schema::EVENT_BEAT_BLANCHE);
		js_info << "Clearing flag " << std::hex << v8toV9Schema::EVENT_BEAT_SPARK << std::endl;
		clearFlagBit(it9, sym9.getPlayerDataAddress("wEventFlags"), v8toV9Schema::EVENT_BEAT_SPARK);

		// --- Common post-migration steps ---
		resetMapScripts(it9, sym9);
		validateAndFixWarp(it9, sym9, v8toV9Schema::WARP_CONFIG);
		finalizeSave(save9, sym9, 0x09, sd);

		js_info << "Sucessfully patched to 3.1.0 save version 9!" << std::endl;
		return true;

	}

	// Looks up a version 8 event flag index in the schema-driven mapping table.
	uint16_t mapV8EventFlagToV9(uint16_t v8) {
		for (size_t i = 0; i < v8toV9Schema::NUM_EVENT_FLAG_MAPPINGS; i++) {
			if (v8toV9Schema::EVENT_FLAG_MAPPINGS[i].src == v8) {
				return v8toV9Schema::EVENT_FLAG_MAPPINGS[i].dst;
			}
		}
		return INVALID_EVENT_FLAG;
	}

	// Looks up a version 8 key item ID in the schema-driven mapping table.
	uint8_t mapV8KeyItemToV9(uint8_t v8) {
		for (size_t i = 0; i < v8toV9Schema::NUM_KEY_ITEM_MAPPINGS; i++) {
			if (v8toV9Schema::KEY_ITEM_MAPPINGS[i].src == v8) {
				return v8toV9Schema::KEY_ITEM_MAPPINGS[i].dst;
			}
		}
		return 0xFF;
	}

}
