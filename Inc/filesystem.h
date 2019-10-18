/*
 * filesystem.h
 *
 *  Created on: 14 Oct 2019
 *      Author: Arion
 */

#ifndef INC_FILESYSTEM_H_
#define INC_FILESYSTEM_H_

#include <stdbool.h>
#include <stdint.h>

#include "block_management.h"
#include "stream.h"

#define RFS_VERSION 15102019
#define API_VERSION 20102019

/*
 * FS-specific defines
 */
#define CORRUPTION_THRESHOLD 4
#define MAGIC_PERIOD 7
#define BACKUP_MAGIC 0xC0FFEE
#define NUM_BLOCKS 4096
#define NUM_FILES 64


/*
 * API-specific defines
 */

typedef enum FileType { EMPTY, RAW, ECC, CRC, LOW_REDUNDANCE, HIGH_REDUNDANCE, FOURIER_REDUNDANCE } FileType;

// 32 bytes
typedef struct File {
	const char identifier[16];
	uint32_t identifier_hash;
	uint16_t first_block;
	uint16_t last_block;
	uint32_t length;
	uint16_t used_blocks;
	uint16_t reserved;
} File;


typedef struct FileSystem {
	bool device_configured;
	bool io_bound;
	bool mounted;
	bool debug;

	const char *id;
	uint32_t addressable_space;
	uint32_t sector_size;
	uint32_t subsector_size;

	uint32_t total_used_blocks;
	uint8_t partition_table[NUM_BLOCKS];
	bool partition_table_modified;
	DataBlock data_blocks[NUM_BLOCKS];
	File files[NUM_FILES];

	void (*read)(uint32_t address, uint8_t* buffer, uint32_t length);
	void (*write)(uint32_t address, uint8_t* buffer, uint32_t length);
	void (*erase_subsector)(uint32_t address);
	void (*erase_sector)(uint32_t address);

	void (*log)(const char*);
} FileSystem;

void rocket_fs_debug(FileSystem* fs, void (*logger)(const char*));
void rocket_fs_device(FileSystem* fs, const char *id, uint32_t capacity, uint32_t sector_size, uint32_t subsector_size);

void rocket_fs_bind(
	FileSystem* fs,
	void (*read)(uint32_t, uint8_t*, uint32_t),
	void (*write)(uint32_t, uint8_t*, uint32_t),
	void (*erase_subsector)(uint32_t),
	void (*erase_sector)(uint32_t)
);

void rocket_fs_mount(FileSystem* fs);
void rocket_fs_format(FileSystem* fs);
void rocket_fs_flush(FileSystem* fs); // Flushed the partition table
void rocket_fs_newfile(FileSystem* fs, const char* name, FileType type);
Stream rocket_fs_open(FileSystem* fs, const char* file);

/* STATIC FUNCTIONS */
static uint8_t __clamp(uint8_t input, uint8_t start, uint8_t end);
static uint64_t __signed_shift(int64_t input, int8_t amount);
static uint64_t __generate_periodic(uint8_t period);
static bool __periodic_magic_match(uint8_t period, uint64_t testable_magic);
static void __no_log(const char* _);



#endif /* INC_FILESYSTEM_H_ */
