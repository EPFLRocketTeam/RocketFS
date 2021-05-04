/*
 * unit_tests.c
 *
 *  Created on: Oct 14, 2019
 *      Author: pcoo56
 */

#ifdef DEBUG

#define TEST_SIZE 819281

#include "emulator.h"
#include "rocket_fs.h"

#include <stdio.h>
#include <cstring>

void debug(const char* message) {
	printf("%s\n", message);
}

void stream_garbage(FileSystem* fs, const char* name, uint8_t generator) {
 	File* file = rocket_fs_getfile(fs, name);

 	Stream stream;
 	rocket_fs_stream(&stream, fs, file, OVERWRITE);

 	for(uint32_t i = 0; i < TEST_SIZE; i++) {
 		uint64_t value = i * generator % 256;

 		if(i % 2 == 0) {
 			stream.write64(value);
 		} else if(i % 3 == 0) {
 			stream.write32(value);
 		} else if(i % 5 == 0) {
 			stream.write16(value);
 		} else {
 			stream.write8(value);
 		}
 	}

 	stream.close();
 }

 void validate_garbage(FileSystem* fs, const char* name) {
 	File* file = rocket_fs_getfile(fs, name);
 	uint64_t value;

 	Stream stream;
 	rocket_fs_stream(&stream, fs, file, OVERWRITE);

 	for(uint32_t i = 0; i < TEST_SIZE + 64; i++) {
 		if(i % 2 == 0) {
 		   value = stream.read64();
 		} else if(i % 3 == 0) {
 		   value = stream.read32();
 		} else if(i % 5 == 0) {
 			value = stream.read16();
 		} else {
 			value = stream.read8();
 		}

 		if(stream.eof) {
 			break;
 		}

 		//printf("%lld\n", value);
 	}

 	stream.close();
 }

 void wr(uint32_t address, uint8_t* buffer, uint32_t length) {
 	printf("Write at %d with length %d with %c\n", address, length, *buffer);
 	emu_write(address, buffer, length);
 }

 int main() {
 	emu_init();

 	FileSystem fs = { 0 };
	Stream stream;

 	rocket_fs_debug(&fs, &debug);
 	rocket_fs_device(&fs, "emulator", FS_ADDRESSABLE_SPACE, FS_SUBSECTOR_SIZE);
 	rocket_fs_bind(&fs, &emu_read, &emu_write, &emu_erase_subsector);
 	rocket_fs_mount(&fs);


	File* file1 = rocket_fs_newfile(&fs, "file1", RAW);

	printf("===== Testing append mode =====\n");
	stream_garbage(&fs, "file1", 1);
	validate_garbage(&fs, "file1");
	rocket_fs_stream(&stream, &fs, file1, APPEND);
	stream.write8(42);
	stream.write((uint8_t*) "THIS TEXT FRAGMENT SHOULD BE APPENDED.", 38);
	stream.close();

	printf("===== Testing concurrent streams =====\n");
	rocket_fs_stream(&stream, &fs, file1, OVERWRITE);
	fs.partition_table_modified = true;
	stream.write8(42);
	rocket_fs_flush(&fs); // Check if concurrent operations possible
	stream.write16(666);
	stream.close();

	printf("===== Testing read/write =====\n");
	File* file3 = rocket_fs_newfile(&fs, "file3", RAW);
	rocket_fs_stream(&stream, &fs, file3, OVERWRITE);
	const char* text = "THIS TEXT FRAGMENT SHOULD BE WRITTEN AND READ BACK.";
	uint8_t buffer[51];
	stream.write((uint8_t*) text, 51);
	stream.read(buffer, 51);
	printf("Comparison: %s\n", text);


	stream.close();

	printf("===== Testing multiple files =====\n");
	File* file2 = rocket_fs_newfile(&fs, "file2", RAW);
	stream_garbage(&fs, "file2", 2);
	validate_garbage(&fs, "file1");
	validate_garbage(&fs, "file2");

	printf("===== Testing filesystem remounting =====\n");
	rocket_fs_unmount(&fs);
	rocket_fs_mount(&fs);
	validate_garbage(&fs, "file1");
	validate_garbage(&fs, "file2");

	printf("===== Testing file deletion =====\n");
	rocket_fs_delfile(&fs, file1);
	validate_garbage(&fs, "file2");
	rocket_fs_delfile(&fs, file2);

	emu_deinit();

	return 0;
}

#endif
