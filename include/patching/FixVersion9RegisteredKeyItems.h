#pragma once
#ifndef FIXVERSION9REGISTEREDKEYITEMS_H
#define FIXVERSION9REGISTEREDKEYITEMS_H

#include "core/SaveBinary.h"
#include "core/SymbolDatabase.h"
#include "core/PatcherConstants.h"
#include "core/CommonPatchFunctions.h"

namespace fixVersion9RegisteredKeyItemsNamespace {
	using namespace fixVersion9RegisteredKeyItemsNamespace;
	bool fixVersion9RegisteredKeyItems(SaveBinary& oldsave, SaveBinary& patchedsave);
}
#endif