/*
 * unit_tests.c
 *
 *  Created on: Oct 14, 2019
 *      Author: pcoo56
 */

#include "unit_tests.h"
#include "emulator.h"
#include "rocket_fs.h"

#include <stdio.h>

#ifdef TESTING

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


	rocket_fs_newfile(&fs, "test1", RAW);
	rocket_fs_newfile(&fs, "test2", RAW);
	rocket_fs_newfile(&fs, "test3", RAW);
	rocket_fs_newfile(&fs, "test4", RAW);

	stream_garbage(&fs, "test1", 1);



	emu_deinit();

	return 0;
}

#endif
