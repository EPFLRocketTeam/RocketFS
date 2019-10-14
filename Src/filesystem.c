/*
 * filesystem.c
 *
 *  Created on: 14 Oct 2019
 *      Author: Arion
 */

#include "io_bridge.h"
#include "filesystem.h"

/*
 * FileSystem structure
 *
 * Each 4KB subsector is called a 'block'.
 *
 * Block 0: RocketFS heuristic magic number
 * Block 1: Master partition
 * Block 2: Recovery partition
 * Block 3: Backup slot 1
 * Block 4: Backup slot 2
 * Block 5: Backup slot 3
 * Block 6: Backup slot 4
 * Block 7: Journal
 *
 * Block 8: Data
 * ...
 * Block 4091: Data
 *
 * Block 4092: Reserved
 * Block 4093: Reserved
 * Block 4094: Reserved (INVASIVE_TEST for flash memory)
 * Block 4095: Reserved (GENTLE_TEST for flash memory)
 */

/*
 * Corruption utility functions
 */

/*
 * Use Gaussian filter on binary magic number to remove impulsional noise.
 * Decompose magic signal in wave form and compare it with the given periodicity
 * (we could use Fourier transforms but that would probably be overkill for embedded systems).
 * Sigma: 1.5
 */

uint16_t __gaussian_kernel[] = { 614, 2447, 3877, 2447, 614 };
uint16_t __gaussian_divider[] = { 13876, 18770, 19998 };

uint8_t __clamp(uint8_t input, uint8_t start, uint8_t end) {
	if(input <= start) {
		return start;
	} else if(input >= end) {
		return end;
	} else {
		return input;
	}
}

uint64_t __generate_periodic(uint8_t period) {
	uint64_t periodic = 0;
	uint64_t period_generator = (-1ULL) >> (64 - period / 2);

	for(uint8_t i = 0; i < 64; i += period) {
		periodic <<= period;
		periodic |= period_generator;
	}

	return periodic;
}

bool __periodic_magic_match(uint8_t period, uint64_t testable_magic) {
	uint8_t i, j;
	uint64_t local_convolution = 0;
	uint64_t hard_coded_magic = __generate_periodic(period);

	uint64_t filtered = 0;

	for(i = 0; i < 64; i++) {
		local_convolution = 0;

		for(j = 0; j < 5; j++) {
			local_convolution += __gaussian_kernel[j] * ((testable_magic >> (i + (j - 2))) & 0b1);
		}

		if(i < 32) {
			filtered |= local_convolution / __gaussian_divider[__clamp(i, 0, 3)];
		} else {
			filtered |= local_convolution / __gaussian_divider[__clamp(64 - i, 0, 3)];
		}
	}

	uint8_t delta = 0;

	filtered ^= hard_coded_magic;

	for(uint8_t i = 0; i < 64; i++) {
		delta += (filtered >> i) & 0b1;
	}

	return delta < CORRUPTION_THRESHOLD;
}

bool __redundant_magic_match(uint64_t magic, uint8_t *input) {

}

/*
 * Utils
 */

__fs_read64(uint64_t address) {
	static uint8_t r64[8];
	fs_read(address, r64, 8);

	uint64_t composition = 0ULL;

	composition |= r64[0] << 56;
	composition |= r64[1] << 48;
	composition |= r64[2] << 40;
	composition |= r64[3] << 32;
	composition |= r64[4] << 24;
	composition |= r64[5] << 16;
	composition |= r64[6] << 8;
	composition |= r64[7];

	return composition;
}


/*
 * FileSystem functions
 */
void rocket_fs_init() {
	uint64_t magic = __fs_read64(0);

	if(__periodic_magic_match(MAGIC_PERIOD, magic)) {
		// Success
	} else {

	}
}

void rocket_fs_format() {
	fs_erase_subsector(0);
}
