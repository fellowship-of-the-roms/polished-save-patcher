#include "patching/PatchVersion9to10.h"
#include "patching/schemas/V9toV10Schema.h"
#include "core/MigrationEngine.h"
#include "core/CommonPatchFunctions.h"
#include "core/SymbolDatabase.h"
#include "core/Logging.h"
#include "core/SymbolDatabaseContents.h"

namespace patchVersion9to10Namespace {

	bool patchVersion9to10(SaveBinary& save9, SaveBinary& save10) {
		// Copy the old save file to the new save file
		save10 = save9;

		// Create iterators and load symbol databases
		SaveBinary::Iterator it9(save9, 0);
		SaveBinary::Iterator it10(save10, 0);
		SymbolDatabase sym9(version9_sym_data, version9_sym_len);
		SymbolDatabase sym10(version10_sym_data, version10_sym_len);
		SourceDest sd = { it9, it10, sym9, sym10 };

		// --- Common pre-migration validation ---
		if (!validateChecksums(save9, sym9)) return false;
		if (!checkPlayerInPokemonCenter2F(it9, sym9)) return false;

		// --- Version-specific patches ---

		// Invert text speed bits (v10 reverses the encoding: 0=instant↔3=slow)
		js_info << "Patching text speed..." << std::endl;
		uint8_t opt1_byte = it10.getByte(sym10.getOptionsAddress("wOptions1"));
		uint8_t originalBits = opt1_byte & TEXT_DELAY_MASK;
		uint8_t reversedBits = TEXT_DELAY_MASK - originalBits;
		switch (originalBits) {
		case 0x00:
			js_info << "Text speed is set to instant." << std::endl;
			if (reversedBits != 0x03) {
				js_error << "Text speed is set to an invalid value." << std::endl;
			}
			break;
		case 0x01:
			js_info << "Text speed is set to fast." << std::endl;
			if (reversedBits != 0x02) {
				js_error << "Text speed is set to an invalid value." << std::endl;
			}
			break;
		case 0x02:
			js_info << "Text speed is set to medium." << std::endl;
			if (reversedBits != 0x01) {
				js_error << "Text speed is set to an invalid value." << std::endl;
			}
			break;
		case 0x03:
			js_info << "Text speed is set to slow." << std::endl;
			if (reversedBits != 0x00) {
				js_error << "Text speed is set to an invalid value." << std::endl;
			}
			break;
		default:
			js_error << "Text speed is set to an invalid value." << std::endl;
			return false;
		}
		it10.setByte(sym10.getOptionsAddress("wOptions1"), (opt1_byte & ~TEXT_DELAY_MASK) | reversedBits);

		// Clear new NO_EXP_OPT bit and reset initial options
		js_info << "Clearing NO_EXP_OPT in wInitialOptions2..." << std::endl;
		it10.resetBit(sym10.getOptionsAddress("wInitialOptions2"), NO_EXP_OPT);
		js_info << "Resetting Initial Options..." << std::endl;
		it10.setBit(RESET_INIT_OPTS);

		// Fix Magikarp record holder if it's the default Ralph with known length
		it9.seek(sym9.getPokemonDataAddress("wMagikarpRecordHoldersName"));
		bool isRalph = true;
		for (size_t i = 0; i < sizeof(ralphName); i++) {
			if (it9.getByte() != ralphName[i]) {
				isRalph = false;
				break;
			}
			it9.next();
		}
		if (isRalph) {
			js_info << "Magikarp Record Holder's name is Ralph. Checking length..." << std::endl;
			uint16_t magikarpLength = it9.getWordBE(sym9.getPokemonDataAddress("wBestMagikarpLengthMm"));
			if (magikarpLength == 0x0306) {
				js_info << "Magikarp Record Holder's length is 0x" << std::hex << magikarpLength << " patching to 0x042B" << std::endl;
				it10.setWordBE(sym10.getPokemonDataAddress("wBestMagikarpLengthMm"), 0x042B);
			} else {
				js_info << "Magikarp Record Holder's length is 0x" << std::hex << magikarpLength << " not patching." << std::endl;
			}
		} else {
			js_info << "Magikarp Record Holder's name is not Ralph. Not patching." << std::endl;
		}

		// --- Schema-driven event flag remapping ---
		remapEventFlags(it9, it10, sym9, sym10,
			v9toV10Schema::EVENT_FLAG_MAPPINGS,
			v9toV10Schema::NUM_EVENT_FLAG_MAPPINGS,
			v9toV10Schema::NUM_EVENTS, sd);

		// --- Mail message conversion (v9 n-gram encoding → v10 raw chars) ---
		mailmsg_struct_v10 mailmsg;
		js_info << "Fixing sPartyMail..." << std::endl;
		for (int i = 0; i < PARTY_LENGTH; i++) {
			mailmsg = convertMailmsgV9toV10(loadStruct<mailmsg_struct_v10>(it9, sym9.getSRAMAddress("sPartyMail") + i * sizeof(mailmsg_struct_v10)));
			writeStruct<mailmsg_struct_v10>(it10, sym10.getSRAMAddress("sPartyMail") + i * sizeof(mailmsg_struct_v10), mailmsg);
		}
		js_info << "Fixing sPartyMailBackup..." << std::endl;
		for (int i = 0; i < PARTY_LENGTH; i++) {
			mailmsg = convertMailmsgV9toV10(loadStruct<mailmsg_struct_v10>(it9, sym9.getSRAMAddress("sPartyMailBackup") + i * sizeof(mailmsg_struct_v10)));
			writeStruct<mailmsg_struct_v10>(it10, sym10.getSRAMAddress("sPartyMailBackup") + i * sizeof(mailmsg_struct_v10), mailmsg);
		}
		js_info << "Fixing sMailbox..." << std::endl;
		for (int i = 0; i < MAILBOX_CAPACITY; i++) {
			mailmsg = convertMailmsgV9toV10(loadStruct<mailmsg_struct_v10>(it9, sym9.getSRAMAddress("sMailbox") + i * sizeof(mailmsg_struct_v10)));
			writeStruct<mailmsg_struct_v10>(it10, sym10.getSRAMAddress("sMailbox") + i * sizeof(mailmsg_struct_v10), mailmsg);
		}
		js_info << "Fixing sMailboxBackup..." << std::endl;
		for (int i = 0; i < MAILBOX_CAPACITY; i++) {
			mailmsg = convertMailmsgV9toV10(loadStruct<mailmsg_struct_v10>(it9, sym9.getSRAMAddress("sMailboxBackup") + i * sizeof(mailmsg_struct_v10)));
			writeStruct<mailmsg_struct_v10>(it10, sym10.getSRAMAddress("sMailboxBackup") + i * sizeof(mailmsg_struct_v10), mailmsg);
		}

		// --- Common post-migration steps ---
		resetMapScripts(it10, sym10);
		validateAndFixWarp(it10, sym10, v9toV10Schema::WARP_CONFIG);
		finalizeSave(save10, sym10, 0x0A, sd);

		js_info << "Successfully patched version 9 save file to version 10." << std::endl;
		return true;
	}

	// Looks up a version 9 event flag index in the schema-driven mapping table.
	// Retained for API compatibility; the main migration uses remapEventFlags() directly.
	uint16_t mapV9EventFlagToV10(uint16_t v9) {
		for (size_t i = 0; i < v9toV10Schema::NUM_EVENT_FLAG_MAPPINGS; i++) {
			if (v9toV10Schema::EVENT_FLAG_MAPPINGS[i].src == v9) {
				return v9toV10Schema::EVENT_FLAG_MAPPINGS[i].dst;
			}
		}
		return INVALID_EVENT_FLAG;
	}

	mailmsg_struct_v10 convertMailmsgV9toV10(const mailmsg_struct_v10& mailmsg) {
		mailmsg_struct_v10 new_mailmsg;
		std::vector<uint8_t> decoded_chars = decodeV9ToChar(mailmsg.message, sizeof(mailmsg.message));
		if (decoded_chars.size() > sizeof(new_mailmsg.message)) {
			js_error << "Decoded mail message is too large to fit in v10 message buffer ("
				<< decoded_chars.size() << " > " << sizeof(new_mailmsg.message) << ")" << std::endl;
		}
		memset(new_mailmsg.message, 0, sizeof(new_mailmsg.message));
		size_t copy_len = std::min(decoded_chars.size(), sizeof(new_mailmsg.message));
		memcpy(new_mailmsg.message, decoded_chars.data(), copy_len);
		new_mailmsg.message_end = mailmsg.message_end;
		memcpy(new_mailmsg.author, mailmsg.author, sizeof(mailmsg.author));
		new_mailmsg.nationality = mailmsg.nationality;
		new_mailmsg.author_id = mailmsg.author_id;
		new_mailmsg.species = mailmsg.species;
		new_mailmsg.type = mailmsg.type;

		return new_mailmsg;
	}

	std::vector<uint8_t> decodeV9ToChar(const uint8_t* data, size_t length) {
		std::unordered_map<uint8_t, std::vector<uint8_t>> v9NgramMap = {
			{0x09, {0xA4, 0x7F}},
			{0x0A, {0x7F, 0xB3}},
			{0x0B, {0xAE, 0xB4}},
			{0x0C, {0xA8, 0xAD}},
			{0x0D, {0xB3, 0xA7}},
			{0x0E, {0xA7, 0xA4}},
			{0x0F, {0xB3, 0x7F}},
			{0x10, {0xA4, 0xB1}},
			{0x11, {0xAE, 0xAD}},
			{0x12, {0xB1, 0xA4}},
			{0x13, {0xB2, 0x7F}},
			{0x14, {0xA0, 0xB3}},
			{0x15, {0xA0, 0xAD}},
			{0x16, {0xB3, 0xAE}},
			{0x17, {0xA7, 0xA0}},
			{0x18, {0xAD, 0xA6}},
			{0x19, {0xA8, 0xB3}},
			{0x1A, {0xA8, 0xB2}},
			{0x1B, {0xA4, 0xA0}},
			{0x1C, {0xB5, 0xA4}},
			{0x1D, {0xA0, 0xB1}},
			{0x1E, {0xB2, 0xB3}},
			{0x1F, {0xAB, 0xA4}},
			{0x20, {0xAE, 0xB1}},
			{0x21, {0xB3, 0xA4}},
			{0x22, {0xA0, 0xB2}},
			{0x23, {0xB8, 0xAE}},
			{0x24, {0xB8, 0x7F}},
			{0x25, {0xB1, 0x7F}},
			{0x26, {0x7F, 0xA1}},
			{0x27, {0xA4, 0xAD}},
			{0x28, {0xAC, 0xA4}},
			{0x29, {0xA4, 0x7F, 0xB3}},
			{0x2A, {0x9D, 0x7F}},
			{0x2B, {0xA4, 0xB2}},
			{0x2C, {0xA4, 0x7F, 0xB8, 0xAE, 0xB4}},
			{0x2D, {0xB2, 0xA4}},
			{0x2E, {0xAD, 0xA4}},
			{0x2F, {0x7F, 0xA7}},
			{0x30, {0x88, 0x7F}},
			{0x31, {0xAE, 0xB4, 0xB1}},
			{0x32, {0x98, 0xAE, 0xB4}},
			{0x33, {0xAD, 0xA3}},
			{0x34, {0xAE, 0xB6}},
			{0x35, {0x7F, 0xA2}},
			{0x36, {0x7F, 0xB6, 0xA0}},
			{0x37, {0xAE, 0xAC, 0xA4}},
			{0x38, {0xA0, 0xB1, 0xA4}},
			{0x39, {0x93, 0xA7, 0xA4}},
			{0x3A, {0xB3, 0xC0, 0xB2}},
			{0x3B, {0xB4, 0xB3}},
			{0x3C, {0xAD, 0xB3}},
			{0x3D, {0xB3, 0xA7, 0xA4}},
			{0x3E, {0xB8, 0xAE, 0xB4}},
			{0x3F, {0xA8, 0xAD, 0xA6}},
			{0x40, {0xA7, 0xA0, 0xB3}},
			{0x41, {0xA0, 0xAD, 0xA3}},
			{0x42, {0xA5, 0xAE, 0xB1}},
			{0x43, {0xA0, 0xAB, 0xAB}},
			{0x44, {0xA7, 0xA4, 0xB1, 0xA4}},
			{0x45, {0xB3, 0xA7, 0xA0, 0xB3}},
			{0x46, {0xA7, 0xA0, 0xB5, 0xA4}},
			{0x47, {0xB1, 0xA0, 0xA8, 0xAD}},
			{0x48, {0xB3, 0xA7, 0xA8, 0xB2}},
			{0x49, {0xA8, 0xA6, 0xA7, 0xB3}},
			{0x4A, {0xB6, 0xA8, 0xB3, 0xA7}},
			{0x4B, {0xAE, 0xB4, 0xAB, 0xA3}},
			{0x4C, {0xA0, 0xB3, 0xB3, 0xAB, 0xA4}},
		};

		std::vector<uint8_t> decoded;
		decoded.reserve(length);

		// 3) Walk each byte, decode or pass-through
		for (size_t i = 0; i < length; i++)
		{
			uint8_t b = data[i];

			// If b is in our n-gram map, expand it
			auto it = v9NgramMap.find(b);
			if (it != v9NgramMap.end()) {
				const std::vector<uint8_t>& expansion = it->second;
				decoded.insert(decoded.end(), expansion.begin(), expansion.end());
			}
			else {
				// Not in 0x09..0x51 (or it�s an unlisted code).
				// => treat as a single "character code"
				decoded.push_back(b);
				if (b == 0x52 || b == 0x53) {
					// <DONE> or @ character found, stop decoding
					break;
				}
			}
		}
		return decoded;
	}

}
