#include "CommonPatchFunctions.h"

// calculate save checksum
uint16_t calculateSaveChecksum(SaveBinary save, uint32_t start, uint32_t end) {
	uint16_t checksum = 0;
	for (uint32_t i = start; i < end; i++) {
		checksum += save.getByte(i);
	}
	return checksum;
}

// copy length bytes from source to dest
void copyDataBlock(SourceDest &sd, uint32_t source, uint32_t dest, int length) {
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
	sd.destSave.seek(dest);
	for (int i = 0; i < length; i++) {
		sd.destSave.setByte(0);
		sd.destSave.next();
	}
}

// fill length bytes starting at dest with value
void fillDataBlock(SourceDest &sd, uint32_t dest, int length, uint8_t value) {
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