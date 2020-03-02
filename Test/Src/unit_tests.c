/*
 * unit_tests.c
 *
 *  Created on: Oct 14, 2019
 *      Author: pcoo56
 */

#ifdef TESTING

#define TEST_SIZE 819281

#include "emulator.h"
#include "rocket_fs.h"

#include <stdio.h>

void debug(const char* message) {
	printf("%s\n", message);
}

int main() {


	File* file1 = rocket_fs_newfile(&fs, "file1", RAW);

	stream_garbage(&fs, "file1", 1);
	validate_garbage(&fs, "file1");

	rocket_fs_stream(&stream, &fs, file1, APPEND);
	stream.write8(42);
	stream.write((uint8_t*) "THIS TEXT FRAGMENT SHOULD BE APPENDED.", 38);
	stream.close();


	File* file2 = rocket_fs_newfile(&fs, "file2", RAW);

	stream_garbage(&fs, "file2", 2);

	printf("\nFile 1:\n");
	validate_garbage(&fs, "file1");

	printf("\nFile 2:\n");
	validate_garbage(&fs, "file2");

	rocket_fs_unmount(&fs);
	rocket_fs_mount(&fs);

	printf("\nFile 1:\n");
	validate_garbage(&fs, "file1");

	printf("\nFile 2:\n");
	validate_garbage(&fs, "file2");

	rocket_fs_delfile(&fs, file1);

	printf("\nFile 2:\n");
	validate_garbage(&fs, "file2");

	rocket_fs_delfile(&fs, file2);

	emu_deinit();

	return 0;
}

#endif
