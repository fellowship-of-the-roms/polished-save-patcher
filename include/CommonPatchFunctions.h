#ifndef COMMON_PATCH_FUNCTIONS_H
#define COMMON_PATCH_FUNCTIONS_H
#include "SaveBinary.h"
#include "SymbolDatabase.h"
#include "PatcherConstants.h"

struct SourceDest {
	SaveBinary::Iterator &sourceSave;
	SaveBinary::Iterator &destSave;
	const SymbolDatabase &sourceSym;
	const SymbolDatabase &destSym;
};

// calculateChecksum function
uint16_t calculateChecksum(SaveBinary save, uint32_t start, uint32_t end);

void copyDataBlock(SourceDest &sd, uint32_t source, uint32_t dest, int length);

void copyDataByte(SourceDest &sd, uint32_t source, uint32_t dest);

#endif