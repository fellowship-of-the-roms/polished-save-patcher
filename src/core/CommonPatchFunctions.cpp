#include "core/CommonPatchFunctions.h"

// calculate save checksum
uint16_t calculateSaveChecksum(SaveBinary save, uint32_t start, uint32_t end) {
	if (start >= end) {
		js_error << "Invalid start and end addresses for calculateSaveChecksum:  " << start << " " << end << std::endl;
		return 0;
	}
	uint16_t checksum = 0;
	for (uint32_t i = start; i < end; i++) {
		checksum += save.getByte(i);
	}
	return checksum;
}

// copy length bytes from source to dest
void copyDataBlock(SourceDest &sd, uint32_t source, uint32_t dest, int length) {
	if (length <= 0) {
		js_error << "Invalid length for copyDataBlock:  " << length << std::endl;
		return;
	}
	sd.sourceSave.seek(source);
	sd.destSave.seek(dest);
	sd.destSave.copy(sd.sourceSave, length);
}

// copy a single byte from source to dest
void copyDataByte(SourceDest &sd, uint32_t source, uint32_t dest) {
	sd.sourceSave.seek(source);
	sd.destSave.seek(dest);
	sd.destSave.setByte(sd.sourceSave.getByte());
}

// clear (0x00) length bytes starting at dest
void clearDataBlock(SourceDest &sd, uint32_t dest, int length) {
	if (length <= 0) {
		js_error << "Invalid length for clearDataBlock:  " << length << std::endl;
		return;
	}
	sd.destSave.seek(dest);
	for (int i = 0; i < length; i++) {
		sd.destSave.setByte(0);
		sd.destSave.next();
	}
}

// fill length bytes starting at dest with value
void fillDataBlock(SourceDest &sd, uint32_t dest, int length, uint8_t value) {
	if (length <= 0) {
		js_error << "Invalid length for fillDataBlock:  " << length << std::endl;
		return;
	}
	sd.destSave.seek(dest);
	for (int i = 0; i < length; i++) {
		sd.destSave.setByte(value);
		sd.destSave.next();
	}
}

// assert that the iterator is at the specified address
bool assertAddress(const SaveBinary::Iterator &it, uint32_t address) {
	if (it.getAddress() != address) {
		js_error << "Address mismatch: Expected " << std::hex << address << " but got " << it.getAddress() << std::endl;
		return false;
	}
	return true;
}

// calculate the number of bytes needed to store index_size bits
int flag_array(uint32_t index_size) {
	if (index_size <= 0) {
		js_error << "Invalid index_size for flag_array:  " << index_size << std::endl;
		return 0;
	}
	return (index_size + 7) / 8;
}

// set the specified bit based on bit index in the flag array from the base address
void setFlagBit(SaveBinary::Iterator &it, uint32_t baseAddress, int bitIndex) {
	if (bitIndex < 0) {
		js_error << "Invalid bitIndex for setFlagBit:  " << bitIndex << std::endl;
		return;
	}
	uint32_t byteAddress = baseAddress + (bitIndex / 8);
	uint8_t mask = 1 << (bitIndex % 8);
	uint8_t byte = it.getByte(byteAddress);
	byte |= mask;
	it.setByte(byteAddress, byte);
}


// clear the specified bit based on bit index in the flag array from the base address
void clearFlagBit(SaveBinary::Iterator &it, uint32_t baseAddress, int bitIndex) {
	if (bitIndex < 0) {
		js_error << "Invalid bitIndex for clearFlagBit:  " << bitIndex << std::endl;
		return;
	}
	uint32_t byteAddress = baseAddress + (bitIndex / 8);
	uint8_t mask = ~(1 << (bitIndex % 8));
	uint8_t byte = it.getByte(byteAddress);
	byte &= mask;
	it.setByte(byteAddress, byte);
}


// check if the specified bit based on bit index in the flag array from the base address is set
bool isFlagBitSet(SaveBinary::Iterator &it, uint32_t baseAddress, int bitIndex) {
	if (bitIndex < 0) {
		js_error << "Invalid bitIndex for isFlagBitSet:  " << bitIndex << std::endl;
		return false;
	}
	uint32_t byteAddress = baseAddress + (bitIndex / 8);
	uint8_t mask = 1 << (bitIndex % 8);
	return (it.getByte(byteAddress) & mask) != 0;
}
