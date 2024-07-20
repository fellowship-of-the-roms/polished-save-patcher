#include "CommonPatchFunctions.h"

uint16_t calculate_checksum(SaveBinary save, uint32_t start, uint32_t end) {
	uint16_t checksum = 0;
	for (uint32_t i = start; i < end; i++) {
		checksum += save.getByte(i);
	}
	return checksum;
}