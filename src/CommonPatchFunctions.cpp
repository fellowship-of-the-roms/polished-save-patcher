#include "CommonPatchFunctions.h"

uint16_t calculateChecksum(SaveBinary save, uint32_t start, uint32_t end) {
	uint16_t checksum = 0;
	for (uint32_t i = start; i < end; i++) {
		checksum += save.getByte(i);
	}
	return checksum;
}

void copyDataBlock(SourceDest &sd, uint32_t source, uint32_t dest, int length) {
	sd.sourceSave.seek(source);
	sd.destSave.seek(dest);
	sd.destSave.copy(sd.sourceSave, length);
}

void copyDataByte(SourceDest &sd, uint32_t source, uint32_t dest) {
	sd.sourceSave.seek(source);
	sd.destSave.seek(dest);
	sd.destSave.setByte(sd.sourceSave.getByte());
}
