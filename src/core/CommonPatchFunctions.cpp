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

// Calculate the newbox checksum for the given mon
// Reference: https://github.com/Rangi42/polishedcrystal/blob/9bit/docs/newbox_format.md#checksum
uint16_t calculateNewboxChecksum(const SaveBinary& save, uint32_t startAddress) {
	uint16_t checksum = 127;

	// Process bytes 0x00 to 0x1F
	for (int i = 0; i <= 0x1F; ++i) {
		checksum += save.getByte(startAddress + i) * (i + 1);
	}

	// Process bytes 0x20 to 0x30
	for (int i = 0x20; i <= 0x30; ++i) {
		checksum += (save.getByte(startAddress + i) & 0x7F) * (i + 2);
	}

	// Clamp to 2 bytes
	checksum &= 0xFFFF;

	return checksum;
}

// Extract the stored newbox checksum for the given mon
// Reference: https://github.com/Rangi42/polishedcrystal/blob/9bit/docs/newbox_format.md#checksum
uint16_t extractStoredNewboxChecksum(const SaveBinary& save, uint32_t startAddress) {
	uint16_t storedChecksum = 0;

	// Read the most significant bits from 0x20 to 0x30
	for (int i = 0; i <= 0xF; ++i) {
		uint8_t msb = (save.getByte(startAddress + 0x20 + i) & 0x80) >> 7;
		storedChecksum |= (msb << (0xF - i));
	}
	return storedChecksum;
}

// Write the newbox checksum for the given mon
// Reference: https://github.com/Rangi42/polishedcrystal/blob/9bit/docs/newbox_format.md#checksum
void writeNewboxChecksum(SaveBinary& save, uint32_t startAddress) {
	uint16_t checksum = calculateNewboxChecksum(save, startAddress);

	// write the most significant bits from 0x20 to 0x30
	for (int i = 0; i <= 0xF; ++i) {
		uint8_t byte = save.getByte(startAddress + 0x20 + i);
		byte &= 0x7F; // clear the most significant bit
		byte |= ((checksum >> (0xF - i)) & 0x1) << 7; // set the most significant bit
		save.setByte(startAddress + 0x20 + i, byte);
	}
}
