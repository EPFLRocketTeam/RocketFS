   /*
 * stream.h
 *
 *  Created on: 17 Oct 2019
 *      Author: Arion
 */

#ifndef INC_STREAM_H_
#define INC_STREAM_H_

#include <stdint.h>

#include "filesystem.h"



typedef struct Stream {
	void (*close)();

	void     (*read)(uint8_t* buffer, uint32_t length);
	uint8_t  (*read8)();
	uint16_t (*read16)();
	uint32_t (*read32)();
	uint64_t (*read64)();

	void (*write)(uint8_t* buffer, uint32_t length);
	void (*write8)(uint8_t data);
	void (*write16)(uint16_t data);
	void (*write32)(uint32_t data);
	void (*write64)(uint64_t data);
} Stream;

void init_stream(Stream stream, FileSystem* filesystem, uint32_t base_address, FileType type);

static void __close();

static void     raw_read(uint8_t* buffer, uint32_t length);
static uint8_t  raw_read8();
static uint16_t raw_read16();
static uint32_t raw_read32();
static uint64_t raw_read64();

static void raw_write(uint8_t* buffer, uint32_t length);
static void raw_write8(uint8_t data);
static void raw_write16(uint16_t data);
static void raw_write32(uint32_t data);
static void raw_write64(uint64_t data);

#endif /* INC_STREAM_H_ */
