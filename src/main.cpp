#include "core/SaveBinary.h"
#include "patching/PatchVersion7to8.h"
#include "patching/PatchVersion8to9.h"
#include "patching/PatchVersion9to10.h"
#include "patching/FixVersion8NoForm.h"
#include "patching/FixVersion9RegisteredKeyItems.h"
#include "patching/FixVersion9PCWarpID.h"
#include "patching/FixVersion9PGOBattleEvent.h"
#include "patching/FixVersion9RoamMap.h"
#include "patching/FixVersion9MagikarpPlainForm.h"
#include "core/PatcherConstants.h"
#include "core/Logging.h"
#include <iostream>
#ifndef CLI_VERSION
#include <emscripten/bind.h>
#endif

#ifndef CLI_VERSION
emscripten::val patch_save_js(const std::string &old_save_path, const std::string &new_save_path, int target_version, int dev_type = 0) {
	// Result object to return to JavaScript
	emscripten::val result = emscripten::val::object();
	bool success=patch_save(old_save_path, new_save_path, target_version, dev_type);
	result.set("success", success);
	return result;
}
#else

#endif

bool patch_save(const std::string &old_save_path, const std::string &new_save_path, int target_version, int dev_type = 0) {
	bool success = true;

	// Load the old save file
	SaveBinary oldSave(old_save_path);
	// copy the old save file to the new save file
	SaveBinary newSave = oldSave;
	// load the save version big endian word

	if (dev_type == 0) {
		uint16_t saveVersion = oldSave.getWordBE(SAVE_VERSION_ABS_ADDRESS);
		if (saveVersion != 0x07 && saveVersion != 0x08 && saveVersion != 0x09) {
			js_error << "Unsupported save version: " << std::hex << saveVersion << std::endl;
			success = false;
		}
		else {
			if (saveVersion == 0x07 && target_version >= 8) {
				if (!patchVersion7to8Namespace::patchVersion7to8(oldSave, newSave)) {
					js_error << "Failed to patch save file from version 7 to 8." << std::endl;
					success = false;
				}
				else {
					js_info << "Patched save file from version 7 to 8." << std::endl;
					saveVersion = 0x08; // Update the save version to 8
					oldSave = newSave; // Update the old save to the new save
				}
			}
			if (saveVersion == 0x08 && target_version >= 9) {
				if (!patchVersion8to9Namespace::patchVersion8to9(oldSave, newSave)) {
					js_error << "Failed to patch save file from version 8 to 9." << std::endl;
					success = false;
				}
				else {
					js_info << "Patched save file from version 8 to 9." << std::endl;
					saveVersion = 0x09; // Update the save version to 9
					oldSave = newSave; // Update the old save to the new save
				}
			}
			if (saveVersion == 0x09 && target_version >= 10) {
				if (!patchVersion9to10Namespace::patchVersion9to10(oldSave, newSave)) {
					js_error << "Failed to patch save file from version 9 to 10." << std::endl;
					success = false;
				}
				else {
					js_info << "Patched save file from version 9 to 10." << std::endl;
				}
			}
		}
	} else {
		js_info << "Running a special one-off patch (dev_type=" << dev_type << ")..." << std::endl;
		switch (dev_type) {
		case 1:
			if (!fixVersion8NoFormNamespace::fixVersion8NoForm(oldSave, newSave)) {
				js_error << "fixVersion9NoForm failed." << std::endl;
				success = false;
			}
			break;
		case 2:
			if (!fixVersion9RegisteredKeyItemsNamespace::fixVersion9RegisteredKeyItems(oldSave, newSave)) {
				js_error << "fixVersion9RegisteredKeyItems failed." << std::endl;
				success = false;
			}
			break;
		case 3:
			if (!fixVersion9PCWarpIDNamespace::fixVersion9PCWarpID(oldSave, newSave)) {
				js_error << "fixVersion9PCWarpID failed." << std::endl;
				success = false;
			}
			break;
		case 4:
			if (!fixVersion9PGOBattleEventNamespace::fixVersion9PGOBattleEvent(oldSave, newSave)) {
				js_error << "fixVersion9PGOBattleEvent failed." << std::endl;
				success = false;
			}
			break;
		case 5:
			if (!fixVersion9RoamMapNamespace::fixVersion9RoamMap(oldSave, newSave)) {
				js_error << "fixVersion9RoamMap failed." << std::endl;
				success = false;
			}
			break;
		case 6:
			if (!fixVersion9MagikarpPlainFormNamespace::fixVersion9MagikarpPlainForm(oldSave, newSave)) {
				js_error << "fixVersion9MagikarpPlainForm failed." << std::endl;
				success = false;
			}
			break;
		default:
			js_error << "Unknown dev_type: " << dev_type << std::endl;
			success = false;
			break;
		}
	}
	if (success) {
		js_info << "Saving file..." << std::endl;
		newSave.save(new_save_path);
		js_info << "File saved successfully!" << std::endl;
	}
	return success;
}

uint16_t get_save_version(const std::string &old_save_path) {
	SaveBinary oldSave(old_save_path);
	return oldSave.getWordBE(SAVE_VERSION_ABS_ADDRESS);
}

#ifndef CLI_VERSION
EMSCRIPTEN_BINDINGS(patch_save_module) {
	emscripten::function("patch_save_js",
		(emscripten::val(*)(const std::string&, const std::string&, int, int)) & patch_save,
		emscripten::allow_raw_pointers()
	);
	emscripten::function("get_save_version", &get_save_version);
}
#endif

int usage(void) {
	printf("usage: polished-save oldsave.sav newsave.sav\n");
	printf("patches oldsave.sav to latest patchversion and saves\n");
	printf("it as newsave.sav\n");
	return 1;
}

int main(int argc, char* argv[]) {
#ifndef CLI_VERSION
	js_info << "Emscripten Save Patcher Version: " << EMSCRIPTEN_PATCHER_VERSION << std::endl;
	js_error << "This program is intended to be run in a browser using Emscripten." << std::endl;
#else
	js_info << "Polished Save Patcher Version: " << EMSCRIPTEN_PATCHER_VERSION << std::endl;
	if(argc < 3) return usage();
	int old_version = get_save_version(argv[1]);
	patch_save(argv[1], argv[2], 10, 0);
#endif
	return 0;
}
