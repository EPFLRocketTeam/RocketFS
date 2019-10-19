/*
 * unit_tests.c
 *
 *  Created on: Oct 14, 2019
 *      Author: pcoo56
 */

#include "unit_tests.h"
#include "emulator.h"
#include "filesystem.h"

#include <stdio.h>

#ifdef TESTING

void debug(const char* message) {
	printf("%s\n", message);
}

int main() {
	emu_init();

	FileSystem fs = { 0 };
	rocket_fs_debug(&fs, &debug);
	rocket_fs_device(&fs, "emulator", FS_ADDRESSABLE_SPACE, FS_SUBSECTOR_SIZE);
	rocket_fs_bind(&fs, &emu_read, &emu_write, &emu_erase_subsector);
	rocket_fs_mount(&fs);
	rocket_fs_mount(&fs);

	emu_deinit();

	return 0;
}

#endif
