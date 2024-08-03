#ifndef PATCHVERSION8TO9_H
#define PATCHVERSION8TO9_H

#include "SaveBinary.h"
#include "SymbolDatabase.h"
#include "PatcherConstants.h"
#include "CommonPatchFunctions.h"
#include <iostream>

namespace {
	constexpr int NUM_KEY_ITEMS_V9 = 0x25;
}

// converts a version 8 key item to a version 8 key item
uint8_t mapV8KeyItemToV9(uint8_t v8);

// bool patchVersion8to9 takes in arguments SaveBinary save7 and SaveBinary save8
bool patchVersion8to9(SaveBinary& save8, SaveBinary& save9);

#endif