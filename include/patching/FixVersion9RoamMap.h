#pragma once
#ifndef FIXVERSION9ROAMMAP_H
#define FIXVERSION9ROAMMAP_H

#include "core/SaveBinary.h"
#include "core/SymbolDatabase.h"
#include "core/PatcherConstants.h"
#include "core/CommonPatchFunctions.h"

namespace fixVersion9RoamMapNamespace {
	using namespace fixVersion9RoamMapNamespace;
	constexpr uint8_t SHINY_MASK = 0b10000000;
	constexpr uint8_t ABILITY_MASK = 0b01100000;
	constexpr uint8_t NATURE_MASK = 0b00011111;
	constexpr uint8_t GENDER_MASK = 0b10000000;
	constexpr uint8_t EGG_MASK = 0b01000000;
	constexpr uint8_t EXTSPECIES_MASK = 0b00100000;
	constexpr uint8_t FORM_MASK = 0b00011111;
	constexpr uint8_t CAUGHT_GENDER_MASK = 0b10000000;
	constexpr uint8_t CAUGHT_TIME_MASK = 0b01100000;
	constexpr uint8_t CAUGHT_BALL_MASK = 0b00011111;
	bool fixVersion9RoamMap(SaveBinary& oldsave, SaveBinary& patchedsave);

	struct roam_struct_v9 {
		uint8_t species;
		uint8_t level;
		uint8_t map_group;
		uint8_t map_number;

		std::tuple<uint8_t, uint8_t> getMap() const { return std::make_tuple(map_group, map_number); };
		void setMap(std::tuple<uint8_t, uint8_t> map) { std::tie(map_group, map_number) = map; };

		uint8_t hp;
		uint8_t dvs[3];
		uint8_t personality[2];

		bool isShiny() const { return ((personality[0] & SHINY_MASK) >> 7); };
		void setShiny(bool shiny) { personality[0] = shiny ? (personality[0] | SHINY_MASK) : (personality[0] & ~SHINY_MASK); };
		uint8_t getAbility() const { return (personality[0] & ABILITY_MASK) >> 5; };
		void setAbility(uint8_t ability) { personality[0] = (personality[0] & ~ABILITY_MASK) | (ability << 5); };
		uint8_t getNature() const { return personality[0] & NATURE_MASK; };
		void setNature(uint8_t nature) { personality[0] = (personality[0] & ~NATURE_MASK) | nature; };
		bool getGender() const { return (personality[1] & GENDER_MASK) >> 7; };
		void setGender(bool gender) { personality[1] = gender ? (personality[1] | GENDER_MASK) : (personality[1] & ~GENDER_MASK); };
		bool isEgg() const { return (personality[1] & EGG_MASK) >> 6; };
		void setEgg(bool egg) { personality[1] = egg ? (personality[1] | EGG_MASK) : (personality[1] & ~EGG_MASK); };
		// ext species consists of 9 bits, 1 from personality[1] EXTSPECIES_MASK which is the MSB, and 8 from species
		uint16_t getExtSpecies() const { return ((personality[1] & EXTSPECIES_MASK) << 3) | species; };
		void setExtSpecies(uint16_t extspecies) {
			personality[1] = (personality[1] & ~EXTSPECIES_MASK) | ((extspecies & 0x100) >> 3);
			species = extspecies & 0xFF;
		};
		uint8_t getForm() const { return personality[1] & FORM_MASK; }
		void setForm(uint8_t form) { personality[1] = (personality[1] & ~FORM_MASK) | form; }

		uint8_t status;
	};

	roam_struct_v9 fixRoamV9Maps(const roam_struct_v9& roam);
}
#endif