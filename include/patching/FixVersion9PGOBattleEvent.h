#pragma once
#ifndef FIXVERSION9PGOBATTLEEVENT_H
#define FIXVERSION9PGOBATTLEEVENT_H

#include "core/SaveBinary.h"
#include "core/SymbolDatabase.h"
#include "core/PatcherConstants.h"
#include "core/CommonPatchFunctions.h"


namespace fixVersion9PGOBattleEventNamespace {
	using namespace fixVersion9PGOBattleEventNamespace;
	constexpr int EVENT_BEAT_CANDELA = 0x596;
	constexpr int EVENT_BEAT_BLANCHE = 0x597;
	constexpr int EVENT_BEAT_SPARK = 0x598;
	bool fixVersion9PGOBattleEvent(SaveBinary& oldsave, SaveBinary& patchedsave);

}

#endif