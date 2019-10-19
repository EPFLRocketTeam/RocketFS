/*
 * rocket_fs.h
 *
 *  Created on: 14 Oct 2019
 *      Author: Arion
 */

#ifndef HEADERS_ROCKET_FS_H_
#define HEADERS_ROCKET_FS_H_

#include "filesystem.h"


#define RFS_VERSION 15102019
#define API_VERSION 20102019


void rocket_fs_debug(FileSystem* fs, void (*logger)(const char*));
void rocket_fs_device(FileSystem* fs, const char *id, uint32_t capacity, uint32_t block_size);

void rocket_fs_bind(
	FileSystem* fs,
	void (*read)(uint32_t, uint8_t*, uint32_t),
	void (*write)(uint32_t, uint8_t*, uint32_t),
	void (*erase_block)(uint32_t)
);

void rocket_fs_mount(FileSystem* fs);
void rocket_fs_format(FileSystem* fs);
void rocket_fs_flush(FileSystem* fs); // Flushes the partition table
void rocket_fs_newfile(FileSystem* fs, const char* name, FileType type);
void rocket_fs_delfile(FileSystem* fs, File* file);
File* rocket_fs_getfile(FileSystem* fs, const char* name);
bool rocket_fs_stream(Stream* stream, FileSystem* fs, File* file, StreamMode mode);


#endif /* HEADERS_ROCKET_FS_H_ */
