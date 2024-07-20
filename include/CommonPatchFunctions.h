#ifndef COMMON_PATCH_FUNCTIONS_H
#define COMMON_PATCH_FUNCTIONS_H
#include "SaveBinary.h"
#include "SymbolDatabase.h"
#include "PatcherConstants.h"

// calculate_checksum function
uint16_t calculate_checksum(SaveBinary save, uint32_t start, uint32_t end);

#endif