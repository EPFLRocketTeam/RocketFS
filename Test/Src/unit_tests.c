/*
 * unit_tests.c
 *
 *  Created on: Oct 14, 2019
 *      Author: pcoo56
 */

#ifdef TESTING

#define TEST_SIZE 128

#include "emulator.h"
#include "rocket_fs.h"

#include <stdio.h>

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

void validate_garbage(FileSystem* fs, const char* name, uint8_t generator) {
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

		if(*stream.eof) {
			break;
		}

		printf("%d\n", value);
	}

	stream.close();
}

int main() {
	emu_init();

	FileSystem fs = { 0 };
	rocket_fs_debug(&fs, &debug);
	rocket_fs_device(&fs, "emulator", FS_ADDRESSABLE_SPACE, FS_SUBSECTOR_SIZE);
	rocket_fs_bind(&fs, &emu_read, &emu_write, &emu_erase_subsector);
	rocket_fs_mount(&fs);


	File* file1 = rocket_fs_newfile(&fs, "FLIGHT_DATA", RAW);

	stream_garbage(&fs, "FLIGHT_DATA", 1);
	validate_garbage(&fs, "FLIGHT_DATA", 1);

	Stream stream;
	rocket_fs_stream(&stream, &fs, file1, APPEND);
	stream.write8(42);
	stream.write((uint8_t*) "THIS TEXT FRAGMENT SHOULD BE APPENDED.", 38);
	stream.close();


	rocket_fs_unmount(&fs);
	rocket_fs_mount(&fs);


	validate_garbage(&fs, "FLIGHT_DATA", 1);


	emu_deinit();

	return 0;
}

#endif
