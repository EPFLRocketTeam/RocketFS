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

#define RFS_VERSION 15102019
#define API_VERSION 20102019

/*
 * FS-specific defines
 */
#define CORRUPTION_THRESHOLD 8
#define MAGIC_PERIOD 7
#define BACKUP_MAGIC 0xC0FFEE


/*
 * API-specific defines
 */

typedef struct FileSystem {
	bool device_configured;
	bool io_bound;
	bool mounted;
	bool debug;

	const char *id;
	uint32_t addressable_space;
	uint32_t sector_size;
	uint32_t subsector_size;

	void (*read)(uint32_t address, uint8_t* buffer, uint32_t length);
	void (*write)(uint32_t address, uint8_t* buffer, uint32_t length);
	void (*erase_subsector)(uint32_t address);
	void (*erase_sector)(uint32_t address);

	void (*log)(const char*);
} FileSystem;

typedef enum FileType { RAW, ECC, CRC, LOW_REDUNDANCE, HIGH_REDUNDANCE, FOURIER_REDUNDANCE } FileType;

typedef struct File {
	FileType type;
	uint64_t length;
	const char* identifier;
} File;

typedef struct Stream {
	FileType type;
	uint64_t length;
	const char* identifier;
} Stream;


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
void rocket_fs_newfile(FileSystem* fs, const char* name, FileType type);
Stream rocket_fs_open(FileSystem* fs, const char* file);
Stream rocket_fs_close();

#endif /* INC_FILESYSTEM_H_ */
