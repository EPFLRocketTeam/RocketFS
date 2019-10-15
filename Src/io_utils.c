/*
 * io_utils.c
 *
 *  Created on: 15 Oct 2019
 *      Author: Arion
 */

#include <stdint.h>

#include "io_utils.h"
#include "filesystem.h"

uint64_t fs_read64(FileSystem* fs, uint32_t address) {
	static uint8_t r64[8];

	fs->read(address, r64, 8);

	uint64_t composition = 0ULL;

	composition |= (uint64_t) r64[0] << 56;
	composition |= (uint64_t) r64[1] << 48;
	composition |= (uint64_t) r64[2] << 40;
	composition |= (uint64_t) r64[3] << 32;
	composition |= (uint64_t) r64[4] << 24;
	composition |= (uint64_t) r64[5] << 16;
	composition |= (uint64_t) r64[6] << 8;
	composition |= (uint64_t) r64[7];

	return composition;
}

void fs_write64(FileSystem* fs, uint32_t address, uint64_t data) {
	static uint8_t w64[8];

	w64[0] = data;
	w64[1] = data >> 8;
	w64[2] = data >> 16;
	w64[3] = data >> 24;
	w64[4] = data >> 32;
	w64[5] = data >> 40;
	w64[6] = data >> 48;
	w64[7] = data >> 56;

	fs->write(address, w64, 8);
}
