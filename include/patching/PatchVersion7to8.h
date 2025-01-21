#ifndef PATCHVERSION7TO8_H
#define PATCHVERSION7TO8_H

#include "core/SaveBinary.h"
#include "core/SymbolDatabase.h"
#include "core/PatcherConstants.h"
#include "core/CommonPatchFunctions.h"

namespace {
	constexpr int NUM_OBJECT_STRUCTS = 13;
	constexpr int OBJECT_PALETTE_V7 = 0x06;
	constexpr int OBJECT_LENGTH_V7 = 0x21;
	constexpr int OBJECT_LENGTH_V8 = 0x22;
	constexpr int NUM_KEY_ITEMS_V7 = 0x1D;
	constexpr int NUM_APRICORNS = 0x07;
	constexpr int NUM_EVENTS = 0x8ff;
	constexpr int NUM_FRUIT_TREES_V7 = 0x23;
	constexpr int NUM_FRUIT_TREES_V8 = 0x2e;
	constexpr int NUM_LANDMARKS_V8 = 0x91;
	constexpr int CONTACT_LIST_SIZE_V7 = 30;
	constexpr int NUM_PHONE_CONTACTS_V8 = 0x25;
	constexpr int NUM_SPAWNS_V7 = 30;
	constexpr int NUM_SPAWNS_V8 = 34;
	constexpr int PARTYMON_STRUCT_LENGTH = 0x30;
	constexpr int PARTY_LENGTH = 6;
	constexpr int PLAYER_NAME_LENGTH = 8;
	constexpr int MON_NAME_LENGTH = 11;
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
	constexpr int NUM_POKEMON_V7 = 0xfe;
	constexpr int MONDB_ENTRIES_V7 = 167;
	constexpr int MONDB_ENTRIES_A_V8 = 167;
	constexpr int MONDB_ENTRIES_B_V8 = 28;
	constexpr int MONDB_ENTRIES_C_V8 = 12;
	constexpr int MONDB_ENTRIES_V8 = MONDB_ENTRIES_A_V8 + MONDB_ENTRIES_B_V8 + MONDB_ENTRIES_C_V8;
	constexpr int MONS_PER_BOX = 20;
	constexpr int MIN_MONDB_SLACK = 10;
	constexpr int NUM_BOXES_V7 = (MONDB_ENTRIES_V7 * 2 - MIN_MONDB_SLACK) / MONS_PER_BOX;
	constexpr int NUM_BOXES_V8 = (MONDB_ENTRIES_V8 * 2 - MIN_MONDB_SLACK) / MONS_PER_BOX;
	constexpr int BOX_NAME_LENGTH = 9;
	constexpr int NEWBOX_SIZE = MONS_PER_BOX + ((MONS_PER_BOX + 7) / 8) + BOX_NAME_LENGTH + 1;
	constexpr int BATTLETOWER_PARTYDATA_SIZE = 6;
	constexpr int NUM_HOF_TEAMS_V8 = 10;
	constexpr int HOF_MON_LENGTH = 1 + 2 + 2 + 1 + (MON_NAME_LENGTH - 1); // species, id, personality, level, nick
	constexpr int HOF_LENGTH = 1 + HOF_MON_LENGTH * PARTY_LENGTH + 1; // win count, party, terminator
	constexpr uint16_t INVALID_SPECIES = -1;
	constexpr uint16_t INVALID_EVENT_FLAG = -1;
	constexpr uint8_t MAGIKARP_V8 = 0x81;
	constexpr uint8_t GYARADOS_V8 = 0x82;
	constexpr uint8_t GYARADOS_RED_FORM_V7 = 0x11;
	constexpr uint8_t GYARADOS_RED_FORM_V8 = 0x15;
	constexpr uint8_t PIKACHU_V8 = 0x19;
	constexpr uint8_t PIKACHU_SURF_FORM_V7 = 0x03;
	constexpr uint8_t PIKACHU_FLY_FORM_V7 = 0x02;
	constexpr uint8_t SURF_V7 = 0x39;
	constexpr uint8_t FLY_V7 = 0x13;
	constexpr int AFFECTION_OPT = 5;
	constexpr int EVS_OPT_CLASSIC = 1;
	constexpr int RESET_INIT_OPTS = 7;
	uint8_t HO_OH_V8 = 0xfa;
	uint8_t LUGIA_V8 = 0xf9;
	uint8_t RAIKOU_V8 = 0xf3;
	uint8_t ENTEI_V8 = 0xf4;
	uint8_t SUICUNE_V8 = 0xf5;
	uint8_t ARTICUNO_V8 = 0x90;
	uint8_t ZAPDOS_V8 = 0x91;
	uint8_t MOLTRES_V8 = 0x92;
	uint8_t MEW_V8 = 0x97;
	uint8_t MEWTWO_V8 = 0x96;
	uint8_t CELEBI_V8 = 0xfb;
	uint8_t SUDOWOODO_V8 = 0xb9;
	constexpr int MAIL_MSG_LENGTH = 0x20;
	constexpr int MAILBOX_CAPACITY = 10;

#pragma pack(push, 1)
	struct breedmon_struct_v8 {
		uint8_t species;
		uint8_t item;
		uint8_t moves[NUM_MOVES];
		uint16_t id;
		uint8_t exp[3];
		uint8_t evs[6];
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

		uint8_t pp[NUM_MOVES];
		uint8_t happiness;
		uint8_t pkrus;
		uint8_t caughtdata;

		uint8_t getCaughtTime() const { return (caughtdata & CAUGHT_TIME_MASK) >> 5; };
		void setCaughtTime(uint8_t time) { caughtdata = (caughtdata & ~CAUGHT_TIME_MASK) | (time << 5); };
		uint8_t getCaughtBall() const { return caughtdata & CAUGHT_BALL_MASK; };
		void setCaughtBall(uint8_t ball) { caughtdata = (caughtdata & ~CAUGHT_BALL_MASK) | ball; };

		uint8_t caughtlevel;
		uint8_t caughtlocation;
		uint8_t level;
	};

	struct party_struct_v8 {
		breedmon_struct_v8 breedmon;
		uint8_t status;
		uint8_t unused;
		uint16_t hp;
		uint16_t maxhp;
		uint16_t stats[5];
	};

	struct savemon_struct_v8 {
		uint8_t species;
		uint8_t item;
		uint8_t moves[NUM_MOVES];
		uint16_t id;
		uint8_t exp[3];
		uint8_t evs[6];
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

		uint8_t ppups;
		uint8_t happiness;
		uint8_t pkrus;
		uint8_t caughtdata;

		uint8_t getCaughtTime() const { return (caughtdata & CAUGHT_TIME_MASK) >> 5; };
		void setCaughtTime(uint8_t time) { caughtdata = (caughtdata & ~CAUGHT_TIME_MASK) | (time << 5); };
		uint8_t getCaughtBall() const { return caughtdata & CAUGHT_BALL_MASK; };
		void setCaughtBall(uint8_t ball) { caughtdata = (caughtdata & ~CAUGHT_BALL_MASK) | ball; };

		uint8_t caughtlevel;
		uint8_t caughtlocation;
		uint8_t level;
		uint8_t extra[3];
		uint8_t nickname[MON_NAME_LENGTH - 1];
		uint8_t ot[PLAYER_NAME_LENGTH - 1];
	};

	struct hofmon_struct_v8 {
		uint8_t species;
		uint16_t id;
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

		uint8_t level;
		uint8_t nickname[MON_NAME_LENGTH - 1];
	};

	struct roam_struct_v8 {
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

	struct mailmsg_struct_v8 {
		uint8_t message[MAIL_MSG_LENGTH];
		uint8_t message_end;
		uint8_t author[PLAYER_NAME_LENGTH];
		uint16_t nationality;
		uint16_t author_id;
		uint8_t species;
		uint8_t type;
	};

#pragma pack(pop)

}

// converts a version 7 key item to a version 8 key item
uint8_t mapV7KeyItemToV8(uint8_t v7);

// converts a version 7 item to a version 8 item
uint8_t mapV7ItemToV8(uint8_t v7);

// converts a version 7 event flag to a version 8 event flag
uint16_t mapV7EventFlagToV8(uint16_t v7);

// converts a version 7 landmark to a version 8 landmark
uint8_t mapV7LandmarkToV8(uint8_t v7);

// converts a version 7 spawn to a version 8 spawn
uint8_t mapV7SpawnToV8(uint8_t v7);

// converts a version 7 Pokémon index to a version 8 Pokémon index
uint16_t mapV7PkmnToV8(uint16_t v7);

// struct for hashing tuples
struct TupleHash {
	template <typename T>
	std::size_t operator()(const T& tuple) const {
		auto [first, second] = tuple;
		return std::hash<unsigned char>()(first) ^ std::hash<unsigned char>()(second);
	}
};

// converts a version 7 (uint8_t group, uint8_t map) tuple to a version 8 (uint8_t group, uint8_t map) tuple
std::tuple<uint8_t, uint8_t> mapv7toV8(uint8_t v7_group, uint8_t v7_map);

// Converts a version 7 (uint16_t species, uint8_t form) tuple to a version 8 unint16_t extspecies
uint16_t mapV7SpeciesFormToV8Extspecies(uint16_t species, uint8_t form);

// converts a version 7 magikarp form to a version 8 magikarp form
uint8_t mapV7MagikarpFormToV8(uint8_t v7);

// converts a version 7 theme to a version 8 theme
uint8_t mapV7ThemeToV8(uint8_t v7);

// converts a version 7 char to a version 8 char
uint8_t mapV7CharToV8(uint8_t v7);

// Calculate the newbox checksum for the given mon
uint16_t calculateNewboxChecksum(const SaveBinary& save, uint32_t startAddress);

// Extract the stored newbox checksum for the given mon
uint16_t extractStoredNewboxChecksum(const SaveBinary& save, uint32_t startAddress);

// Write the newbox checksum for the given mon
void writeNewboxChecksum(SaveBinary& save, uint32_t startAddress);

// Writes the default box name for the given box number
void writeDefaultBoxName(SaveBinary::Iterator& it, int boxNum);

// Migrate the newbox box data from version 7 to version 8
void migrateBoxData(SourceDest &sd, const std::string &prefix);

// a helper function to convert item lists
void convertItemList(SourceDest &sd, uint32_t numItemsAddr7, uint32_t itemsAddr7, uint32_t numItemsAddr8, uint32_t itemsAddr8, const std::string &itemListName);

// Helper to map a map group/number pair
void mapAndWriteMapGroupNumber(SourceDest &sd, uint32_t mapGroupAddr7, uint32_t mapGroupAddr8, uint32_t mapNumberAddr7, uint32_t mapNumberAddr8, const std::string &mapName);

// bool patchVersion7to8 takes in arguments SaveBinary save7 and SaveBinary save8
bool patchVersion7to8(SaveBinary& save7, SaveBinary& save8);

savemon_struct_v8 convertSavemonV7toV8(const savemon_struct_v8& savemon, std::vector<uint16_t> &seen_mons, std::vector<uint16_t> &caught_mons);

breedmon_struct_v8 convertBreedmonV7toV8(const breedmon_struct_v8& breedmon, std::vector<uint16_t>& seen_mons, std::vector<uint16_t>& caught_mons);

party_struct_v8 convertPartyV7toV8(const party_struct_v8& party, std::vector<uint16_t>& seen_mons, std::vector<uint16_t>& caught_mons);

hofmon_struct_v8 convertHofmonV7toV8(const hofmon_struct_v8& hofmon, std::vector<uint16_t>& seen_mons, std::vector<uint16_t>& caught_mons);

roam_struct_v8 convertRoamV7toV8(const roam_struct_v8& roam);

mailmsg_struct_v8 convertMailmsgV7toV8(const mailmsg_struct_v8& mailmsg);

#endif
