/*
 * emulator.c
 *
 *  Created on: Oct 14, 2019
 *      Author: pcoo56
 */

#ifdef TESTING


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "emulator.h"

/*
 * Utils
 */
static void __emu_fatal(const char* message) {
	printf("%s", message);
	printf("Emulator crashed. Insert a breakpoint here for debugging.\n");
	while(1);
}

static void __memand(uint8_t* destination, uint8_t* source, uint32_t length) {
	while(length-- > 0) {
		destination[0] &= source[0];

		destination++;
		source++;
	}
}



/*
 * Memory pointer
 */
static uint8_t* __emu_memory;

/*
 * Implementation
 */
void emu_init() {
	printf("Initialising memory emulator... ");

	__emu_memory = (uint8_t*) malloc(sizeof(uint8_t) * FS_ADDRESSABLE_SPACE);

	for(uint32_t i = 0; i < FS_ADDRESSABLE_SPACE; i++) {
		__emu_memory[i] = i % 256;
	}

	if(!__emu_memory) {
		__emu_fatal("Unable to allocate memory for the emulator");
	}

	printf("done\n");
}

void emu_deinit() {
	free(__emu_memory);
}

void emu_read(uint32_t address, uint8_t* buffer, uint32_t length) {
	if(address + length >= FS_ADDRESSABLE_SPACE) {
		__emu_fatal("Memory access attempt out of addressable space (emu_read)\n");
	}

	memcpy(buffer, __emu_memory + address, length);
}

void emu_write(uint32_t address, uint8_t* buffer, uint32_t length) {
	if(address + length >= FS_ADDRESSABLE_SPACE) {
		__emu_fatal("Memory access attempt out of addressable space (emu_write)\n");
	}

	__memand(__emu_memory + address, buffer, length);
}

void emu_erase_subsector(uint32_t address) {
	if(address >= FS_ADDRESSABLE_SPACE) {
		__emu_fatal("Memory access attempt out of addressable space (emu_erase_subsector)\n");
	}

	memset(__emu_memory + address - address % FS_SUBSECTOR_SIZE, 0xFF, FS_SUBSECTOR_SIZE);
}

void emu_erase_sector(uint32_t address) {
	if(address >= FS_ADDRESSABLE_SPACE) {
		__emu_fatal("Memory access attempt out of addressable space (emu_erase_sector)\n");
	}

	memset(__emu_memory + address - address % FS_SECTOR_SIZE, 0xFF, FS_SECTOR_SIZE);
}

#endif
