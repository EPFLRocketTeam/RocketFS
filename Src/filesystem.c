/*
 * filesystem.c
 *
 *  Created on: 14 Oct 2019
 *      Author: Arion
 */

#include "filesystem.h"
#include "stream.h"

/*
 * FileSystem structure
 *
 * Each 4KB subsector is called a 'block'.
 *
 * Block 0: Core block
 * 		2KB: RocketFS heuristic magic number
 * 		2KB: Metadata
 * Block 1: Master partition (bit 0...3: FileType, 4...7: relative initialisation time)
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
	if(subsector_size >= NUM_BLOCKS) {
		fs->id = id;
		fs->addressable_space = capacity;
		fs->sector_size = sector_size;
		fs->subsector_size = subsector_size;
		fs->device_configured = true;
		fs->partition_table_modified = false;
	} else {
		fs->log("Fatal: Device's sub-sector granularity is too high. Consider using using a device with higher subsector_size.");
	}
}

void rocket_fs_mount(FileSystem* fs) {
	if(!fs->debug) {
		fs->log = &__no_log;
	}

	fs->log("Mounting filesystem...");

	File core_block = { .identifier = "Core Block", .type = RAW, .length = fs->subsector_size };
	File master_block = { .identifier = "Master Partition Block", .type = RAW, .length = fs->subsector_size };

	Stream stream;
	init_stream(&stream, fs, core_block);

	uint64_t magic = stream.read64();
	stream.close();

	if(__periodic_magic_match(MAGIC_PERIOD, magic)) {
		init_stream(&stream, fs, master_block);

		for(uint32_t i = 0; i < NUM_BLOCKS; i++) {
			fs->partition_table[i] = stream.read8();
		}

		stream.close();

		rfs_populate_blocks(fs); // in block_management.c
	} else {
		// TODO (a) Check redundant magic number.
		// TODO (b) Write corrupted partition to a free backup slot.
		fs->log("Mounting filesystem for the first time or filesystem corrupted. Formatting...");
		rocket_fs_format(fs);
	}

	fs->log("Filesystem mounted");
}



void rocket_fs_format(FileSystem* fs) {
	File core_block = { .identifier = "Core Block", .type = RAW, .length = fs->subsector_size};
	File master_block = { .identifier = "Master Partition Block", .type = RAW, .length = fs->subsector_size};

	fs->erase_subsector(0);    // Core block
	fs->erase_subsector(fs->subsector_size); // Master partition block

	uint64_t magic = __generate_periodic(MAGIC_PERIOD);

	Stream stream;

	init_stream(&stream, fs, core_block);

	stream.write64(magic);

	/*
	 * ... write heuristic magic number and metadata
	 */

	stream.close();

	init_stream(&stream, fs, core_block);

	stream.write8(0b00001111); // Core block (used as internal relative clock)
	stream.write8(0b00001111); // Master partition block
	// stream.write8(0b00001111); // Recovery partition block
	// stream.write8(0b00001111); // Backup partition block 1
	// stream.write8(0b00001111); // Backup partition block 2
	// stream.write8(0b00001111); // Backup partition block 3
	// stream.write8(0b00001111); // Backup partition block 4
	// stream.write8(0b00001111); // Journal block

	stream.close();

	/*
	 * The partition table does not need to be cleared.
	 */
}

/*
 * Flushed the partition table
 */
void rocket_fs_flush(FileSystem* fs) {

}

/*
 * Names at most 16 characters long.
 */
void rocket_fs_newfile(FileSystem* fs, const char* name, FileType type) {
	File file = { .identifier = name, .type = type, .length = fs->subsector_size};
}


Stream rocket_fs_open(FileSystem* fs, const char* name) {
	File file = { .identifier = name, .type = RAW, .length = fs->subsector_size};
	Stream stream;
	init_stream(&stream, fs, &file);
	return stream;
}



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

static uint64_t __signed_shift(int64_t input, int8_t amount) {
	if(amount > 0) {
		return input >> amount;
	} else if(amount < 0) {
		return input << (-amount);
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
	static const uint16_t __gaussian_divider[] = { 3470, 4693, 5000 };

	uint8_t i, j;
	uint64_t weighted = 0;
	uint64_t hard_coded_magic = __generate_periodic(period);

	uint64_t filtered = 0;

	for(i = 0; i < 64; i++) {
		weighted = 0;
		filtered <<= 1;

		for(j = 0; j < 5; j++) {
			weighted += __gaussian_kernel[j] * (__signed_shift(testable_magic, 65 - i - j) & 0b1);
		}

		if(i < 32) {
			filtered |=  weighted / __gaussian_divider[__clamp(i, 0, 2)];
		} else {
			filtered |= weighted / __gaussian_divider[__clamp(64 - i, 0, 2)];
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
