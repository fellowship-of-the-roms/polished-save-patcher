#include "patching/PatchVersion7to8.h"
#include "patching/schemas/V7toV8Schema.h"
#include <unordered_map>

namespace patchVersion7to8Namespace {

// Looks up a version 7 key item ID in the schema-driven mapping table.
uint8_t mapV7KeyItemToV8(uint8_t v7) {
	for (size_t i = 0; i < v7toV8Schema::NUM_KEY_ITEM_MAPPINGS; i++) {
		if (v7toV8Schema::KEY_ITEM_MAPPINGS[i].src == v7) {
			return v7toV8Schema::KEY_ITEM_MAPPINGS[i].dst;
		}
	}
	return 0xFF;
}

// Looks up a version 7 item ID in the schema-driven mapping table.
uint8_t mapV7ItemToV8(uint8_t v7) {
	for (size_t i = 0; i < v7toV8Schema::NUM_ITEM_MAPPINGS; i++) {
		if (v7toV8Schema::ITEM_MAPPINGS[i].src == v7) {
			return v7toV8Schema::ITEM_MAPPINGS[i].dst;
		}
	}
	return 0xFF;
}

// Looks up a version 7 event flag index in the schema-driven mapping table.
uint16_t mapV7EventFlagToV8(uint16_t v7) {
	for (size_t i = 0; i < v7toV8Schema::NUM_EVENT_FLAG_MAPPINGS; i++) {
		if (v7toV8Schema::EVENT_FLAG_MAPPINGS[i].src == v7) {
			return v7toV8Schema::EVENT_FLAG_MAPPINGS[i].dst;
		}
	}
	return INVALID_EVENT_FLAG;
}

// Looks up a version 7 landmark ID in the schema-driven mapping table.
uint8_t mapV7LandmarkToV8(uint8_t v7) {
	for (size_t i = 0; i < v7toV8Schema::NUM_LANDMARK_MAPPINGS; i++) {
		if (v7toV8Schema::LANDMARK_MAPPINGS[i].src == v7) {
			return v7toV8Schema::LANDMARK_MAPPINGS[i].dst;
		}
	}
	return 0xFF;
}

// Looks up a version 7 spawn ID in the schema-driven mapping table.
uint8_t mapV7SpawnToV8(uint8_t v7) {
	for (size_t i = 0; i < v7toV8Schema::NUM_SPAWN_MAPPINGS; i++) {
		if (v7toV8Schema::SPAWN_MAPPINGS[i].src == v7) {
			return v7toV8Schema::SPAWN_MAPPINGS[i].dst;
		}
	}
	return 0xFF;
}

// Looks up a version 7 Pokemon species in the schema-driven mapping table.
uint16_t mapV7PkmnToV8(uint16_t v7) {
	for (size_t i = 0; i < v7toV8Schema::NUM_POKEMON_MAPPINGS; i++) {
		if (v7toV8Schema::POKEMON_MAPPINGS[i].src == v7) {
			return v7toV8Schema::POKEMON_MAPPINGS[i].dst;
		}
	}
	return INVALID_SPECIES;
}

// Looks up a version 7 map group/number pair in the schema-driven mapping table.
std::tuple<uint8_t, uint8_t> mapv7toV8(uint8_t v7_group, uint8_t v7_map) {
	for (size_t i = 0; i < v7toV8Schema::NUM_MAP_TUPLE_MAPPINGS; i++) {
		if (v7toV8Schema::MAP_TUPLE_MAPPINGS[i].src.group == v7_group &&
		    v7toV8Schema::MAP_TUPLE_MAPPINGS[i].src.map == v7_map) {
			return {v7toV8Schema::MAP_TUPLE_MAPPINGS[i].dst.group,
			        v7toV8Schema::MAP_TUPLE_MAPPINGS[i].dst.map};
		}
	}
	return {v7_group, v7_map};
}

// Looks up a version 7 species+form in the schema-driven mapping table.
uint16_t mapV7SpeciesFormToV8Extspecies(uint16_t species, uint8_t form) {
	for (size_t i = 0; i < v7toV8Schema::NUM_SPECIES_FORM_MAPPINGS; i++) {
		if (v7toV8Schema::SPECIES_FORM_MAPPINGS[i].species == species &&
		    v7toV8Schema::SPECIES_FORM_MAPPINGS[i].form == form) {
			return v7toV8Schema::SPECIES_FORM_MAPPINGS[i].extSpecies;
		}
	}
	return INVALID_SPECIES;
}

// Looks up a version 7 Magikarp form in the schema-driven mapping table.
uint8_t mapV7MagikarpFormToV8(uint8_t v7) {
	for (size_t i = 0; i < v7toV8Schema::NUM_MAGIKARP_FORM_MAPPINGS; i++) {
		if (v7toV8Schema::MAGIKARP_FORM_MAPPINGS[i].src == v7) {
			return v7toV8Schema::MAGIKARP_FORM_MAPPINGS[i].dst;
		}
	}
	return 0xFF;
}

// Looks up a version 7 box theme in the schema-driven mapping table.
uint8_t mapV7ThemeToV8(uint8_t v7) {
	for (size_t i = 0; i < v7toV8Schema::NUM_THEME_MAPPINGS; i++) {
		if (v7toV8Schema::THEME_MAPPINGS[i].src == v7) {
			return v7toV8Schema::THEME_MAPPINGS[i].dst;
		}
	}
	return v7;
}

// Looks up a version 7 character encoding in the schema-driven mapping table.
uint8_t mapV7CharToV8(uint8_t v7) {
	for (size_t i = 0; i < v7toV8Schema::NUM_CHAR_MAPPINGS; i++) {
		if (v7toV8Schema::CHAR_MAPPINGS[i].src == v7) {
			return v7toV8Schema::CHAR_MAPPINGS[i].dst;
		}
	}
	return v7;
}

}
