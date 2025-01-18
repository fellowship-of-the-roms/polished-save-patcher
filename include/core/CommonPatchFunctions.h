#ifndef COMMON_PATCH_FUNCTIONS_H
#define COMMON_PATCH_FUNCTIONS_H
#include "SaveBinary.h"
#include "SymbolDatabase.h"
#include "PatcherConstants.h"
#include "Logging.h"

// struct containing source and destination save data and symbol databases
struct SourceDest {
	SaveBinary::Iterator &sourceSave;
	SaveBinary::Iterator &destSave;
	const SymbolDatabase &sourceSym;
	const SymbolDatabase &destSym;
};

// calculateSaveChecksum function
uint16_t calculateSaveChecksum(SaveBinary save, uint32_t start, uint32_t end);

// copy length bytes from source to dest
void copyDataBlock(SourceDest &sd, uint32_t source, uint32_t dest, int length);

// copy a single byte from source to dest
void copyDataByte(SourceDest &sd, uint32_t source, uint32_t dest);

// clear (0x00) length bytes starting at dest
void clearDataBlock(SourceDest &sd, uint32_t dest, int length);

// fill length bytes starting at dest with value
void fillDataBlock(SourceDest &sd, uint32_t dest, int length, uint8_t value);

// assert that the iterator is at the specified address
bool assertAddress(const SaveBinary::Iterator &it, uint32_t address);

// calculate the number of bytes needed to store index_size bits
int flag_array(uint32_t index_size);

// set the specified bit based on bit index in the flag array from the base address
void setFlagBit(SaveBinary::Iterator &it, uint32_t baseAddress, int bitIndex);

// clear the specified bit based on bit index in the flag array from the base address
void clearFlagBit(SaveBinary::Iterator &it, uint32_t baseAddress, int bitIndex);

// check if the specified bit based on bit index in the flag array from the base address is set
bool isFlagBitSet(SaveBinary::Iterator &it, uint32_t baseAddress, int bitIndex);


#endif