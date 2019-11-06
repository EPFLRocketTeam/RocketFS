/*
 * unit_tests.c
 *
 *  Created on: Oct 14, 2019
 *      Author: pcoo56
 */

#ifdef TESTING

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

	for(uint16_t i = 0; i < 12345; i++) {
		stream.write32(i * generator);
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


	File* file1 = rocket_fs_newfile(&fs, "Flight Data", RAW);
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

	rocket_fs_unmount(&fs);
	rocket_fs_mount(&fs);


	emu_deinit();

	return 0;
}

#endif
