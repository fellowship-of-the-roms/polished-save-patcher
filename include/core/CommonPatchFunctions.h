#ifndef COMMON_PATCH_FUNCTIONS_H
#define COMMON_PATCH_FUNCTIONS_H
#include <cstring>
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

// Calculate the newbox checksum for the given mon
uint16_t calculateNewboxChecksum(const SaveBinary& save, uint32_t startAddress);

// Extract the stored newbox checksum for the given mon
uint16_t extractStoredNewboxChecksum(const SaveBinary& save, uint32_t startAddress);

// Write the newbox checksum for the given mon
void writeNewboxChecksum(SaveBinary& save, uint32_t startAddress);

template <typename T>
T loadStruct(SaveBinary::Iterator& it, uint32_t address) {
	T data;
	it.seek(address);

	// Access the raw memory of the struct
	uint8_t* structPtr = reinterpret_cast<uint8_t*>(&data);

	// Copy data byte-by-byte into the struct
	for (int i = 0; i < sizeof(T); i++) {
		structPtr[i] = it.getByte();
		it.next();
	}

	return data;
}

template <typename T>
void writeStruct(SaveBinary::Iterator& it, uint32_t address, const T& data) {
	it.seek(address);

	// Access the raw memory of the struct
	const uint8_t* structPtr = reinterpret_cast<const uint8_t*>(&data);

	// Write data byte-by-byte
	for (size_t i = 0; i < sizeof(T); i++) {
		it.setByte(structPtr[i]);
		it.next();
	}
}

#endif
