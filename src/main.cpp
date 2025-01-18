#include "core/SaveBinary.h"
#include "core/SymbolDatabase.h"
#include "patching/PatchVersion7to8.h"
#include "patching/PatchVersion8to9.h"
#include "core/PatcherConstants.h"
#include "core/Logging.h"
#include <iostream>
#include <fstream>
#include <streambuf>
#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <vector>

emscripten::val patch_save(const std::string &old_save_path, const std::string &new_save_path, int target_version) {
	// Result object to return to JavaScript
	emscripten::val result = emscripten::val::object();
	bool success = true;

	// Load the old save file
	SaveBinary oldSave(old_save_path);
	// copy the old save file to the new save file
	SaveBinary newSave = oldSave;
	// load the save version big endian word

	uint16_t saveVersion = oldSave.getWordBE(SAVE_VERSION_ABS_ADDRESS);
	if (saveVersion != 0x07 && saveVersion != 0x08) {
		js_error << "Unsupported save version: " << std::hex << saveVersion << std::endl;
		success = false;
	} else {
		if (saveVersion == 0x07 && target_version >= 8) {
			if (!patchVersion7to8(oldSave, newSave)) {
				js_error << "Failed to patch save file from version 7 to 8." << std::endl;
				success = false;
			} else {
				js_info << "Patched save file from version 7 to 8." << std::endl;
				saveVersion = 0x08; // Update the save version to 8
				oldSave = newSave; // Update the old save to the new save
			}
		}
		if (saveVersion == 0x08 && target_version >= 9) {
			if (!patchVersion8to9(oldSave, newSave)) {
				js_error << "Failed to patch save file from version 8 to 9." << std::endl;
				success = false;
			} else {
				js_info << "Patched save file from version 8 to 9." << std::endl;
			}
		}
		if (success) {
			js_info << "Saving file..." << std::endl;
			newSave.save(new_save_path);
			js_info << "File saved successfully!" << std::endl;
		}
	}

	result.set("success", success);
	return result;
}

uint16_t get_save_version(const std::string &old_save_path) {
	SaveBinary oldSave(old_save_path);
	return oldSave.getWordBE(SAVE_VERSION_ABS_ADDRESS);
}

EMSCRIPTEN_BINDINGS(patch_save_module) {
	emscripten::function("patch_save", &patch_save);
	emscripten::function("get_save_version", &get_save_version);
}

int main(int argc, char* argv[]) {
	js_error << "This program is intended to be run in a browser using Emscripten." << std::endl;
	return 1;
}
