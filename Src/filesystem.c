/*
 * filesystem.c
 *
 *  Created on: 14 Oct 2019
 *      Author: Arion
 */

#include "filesystem.h"
#include "io_utils.h"

/*
 * FileSystem structure
 *
 * Each 4KB subsector is called a 'block'.
 *
 * Block 0: Miscellaneous
 * 		2KB: RocketFS heuristic magic number
 * 		2KB: Metadata
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

static uint8_t __clamp(uint8_t input, uint8_t start, uint8_t end) {
	if(input <= start) {
		return start;
	} else if(input >= end) {
		return end;
	} else {
		return input;
	}
}

static uint64_t __generate_periodic(uint8_t period) {
	uint64_t periodic = 0;
	uint64_t period_generator = (-1ULL) >> (64 - period / 2);

	for(uint8_t i = 0; i < 64; i += period) {
		periodic <<= period;
		periodic |= period_generator;
	}

	return periodic;
}

static bool __periodic_magic_match(uint8_t period, uint64_t testable_magic) {
	static const uint16_t __gaussian_kernel[] = { 614, 2447, 3877, 2447, 614 };
	static const uint16_t __gaussian_divider[] = { 13876, 18770, 19998 };

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

static void __no_log(const char* _) {}

/*
 * FileSystem functions
 */
void rocket_fs_debug(FileSystem* fs, void (*logger)(const char*)) {
	fs->log = logger;
	fs->debug = true;

	fs->log("FileSystem log initialised.");
}

void rocket_fs_bind(
	FileSystem* fs,
	void (*read)(uint32_t, uint8_t*, uint32_t),
	void (*write)(uint32_t, uint8_t*, uint32_t),
	void (*erase_subsector)(uint32_t),
	void (*erase_sector)(uint32_t)
) {
	fs->read = read;
	fs->write = write;
	fs->erase_subsector = erase_subsector;
	fs->erase_sector = erase_sector;
	fs->io_bound = true;
}

void rocket_fs_device(FileSystem* fs, const char *id, uint32_t capacity, uint32_t sector_size, uint32_t subsector_size) {
	fs->id = id;
	fs->addressable_space = capacity;
	fs->sector_size = sector_size;
	fs->subsector_size = subsector_size;
	fs->device_configured = true;
}

void rocket_fs_mount(FileSystem* fs) {
	if(!fs->debug) {
		fs->log = &__no_log;
	}

	fs->log("Mounting filesystem...");

	uint64_t magic = fs_read64(fs, 0);

	if(__periodic_magic_match(MAGIC_PERIOD, magic)) {

	} else {
		// TODO (a) Check redundant magic number.
		// TODO (b) Write corrupted partition to a free backup slot.
		fs->log("Mounting filesystem for the first time or filesystem corrupted. Formatting...");
		rocket_fs_format(fs);
	}

	fs->log("Filesystem mounted");
}



void rocket_fs_format(FileSystem* fs) {
	fs->erase_subsector(0);    // Misc block
	fs->erase_subsector(4096); // Master partition block

	uint64_t magic = __generate_periodic(MAGIC_PERIOD);

	fs_write64(fs, 0, magic);
}
