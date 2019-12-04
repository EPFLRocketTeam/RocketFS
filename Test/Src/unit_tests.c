/*
 * unit_tests.c
 *
 *  Created on: Oct 14, 2019
 *      Author: pcoo56
 */

#ifdef TESTING

#define TEST_SIZE 1100000

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
		stream.write32(i * generator);
	}

	stream.close();
}

void validate_garbage(FileSystem* fs, const char* name, uint8_t generator) {
   File* file = rocket_fs_getfile(fs, name);
   uint32_t value;

   Stream stream;
   rocket_fs_stream(&stream, fs, file, OVERWRITE);

   uint32_t success = 0;

   for(uint32_t i = 0; i < TEST_SIZE; i++) {
      value = stream.read32();

      if(value == i * generator) {
         success++;
      }
   }

   printf("Validation successfull at %.2f%% for test %s\n", 100.0f * success / TEST_SIZE, name);

   stream.close();
}

int main() {
	emu_init();

	FileSystem fs = { 0 };
	rocket_fs_debug(&fs, &debug);
	rocket_fs_device(&fs, "emulator", FS_ADDRESSABLE_SPACE, FS_SUBSECTOR_SIZE);
	rocket_fs_bind(&fs, &emu_read, &emu_write, &emu_erase_subsector);
	rocket_fs_mount(&fs);


	File* file1 = rocket_fs_newfile(&fs, "test1", RAW);
	rocket_fs_newfile(&fs, "test2", RAW);
	rocket_fs_newfile(&fs, "test3", RAW);
	File* file4 = rocket_fs_newfile(&fs, "test4", RAW);

	stream_garbage(&fs, "test1", 1);
	stream_garbage(&fs, "test2", 2);

	rocket_fs_delfile(&fs, file1);

	stream_garbage(&fs, "test3", 3);

	rocket_fs_newfile(&fs, "test1", RAW);

	stream_garbage(&fs, "test1", 5);
	stream_garbage(&fs, "test4", 4);

	Stream stream;
	rocket_fs_stream(&stream, &fs, file4, APPEND);
	stream.write((uint8_t*) "THIS TEXT FRAGMENT SHOULD BE APPENDED.", 38);
	stream.close();

   validate_garbage(&fs, "test1", 5);
   validate_garbage(&fs, "test2", 2);
   validate_garbage(&fs, "test3", 3);
   validate_garbage(&fs, "test4", 4);

	rocket_fs_unmount(&fs);
	rocket_fs_mount(&fs);

	validate_garbage(&fs, "test1", 5);
   validate_garbage(&fs, "test2", 2);
   validate_garbage(&fs, "test3", 3);
   validate_garbage(&fs, "test4", 4);


	emu_deinit();

	return 0;
}

#endif
