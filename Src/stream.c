/*
 * stream.c
 *
 *  Created on: 17 Oct 2019
 *      Author: Arion
 */

#include "stream.h"

#include "block_management.h"


static FileSystem* fs;

static bool file_open = false;

static uint32_t read_address = 0xFFFFFFFFL;
static uint32_t write_address = 0xFFFFFFFFL;


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


bool init_stream(Stream* stream, FileSystem* filesystem, uint32_t base_address, FileType type) {
	if(!file_open) {
		fs = filesystem;
		read_address = base_address;
		write_address = base_address;

		stream->close = &__close;

		switch(type) {
		case RAW:
			stream->read = &raw_read;
			stream->read8 = &raw_read8;
			stream->read16 = &raw_read16;
			stream->read32 = &raw_read32;
			stream->read64 = &raw_read64;

			stream->write = &raw_write;
			stream->write8 = &raw_write8;
			stream->write16 = &raw_write16;
			stream->write32 = &raw_write32;
			stream->write64 = &raw_write64;

			break;
		default:
			fs->log("Error: File type unsupported.");
			return false;
		}

		file_open = true;

		return true;
	} else {
		fs->log("Error: Another file is already open.");
		return false;
	}

}

static void __close() {
	fs = 0;
	read_address = 0xFFFFFFFFL;
	write_address = 0xFFFFFFFFL;
	file_open = false;
}

/* RAW IO FUNCTIONS */
static uint8_t coder[8]; // Used as encoder and decoder

static void raw_read(uint8_t* buffer, uint32_t length) {
	rfs_access_memory(fs, &read_address, length, READ); // Transforms the write address (or fails if end of file) if we are at the end of a readable section

	fs->read(read_address, buffer, length);
	read_address += length;
}

static uint8_t raw_read8() {
	raw_read(coder, 1);
	return coder[0];
}

static uint16_t raw_read16() {
	raw_read(coder, 2);

	uint64_t composition = 0ULL;

	composition |= (uint16_t) coder[1] << 8;
	composition |= (uint16_t) coder[0];

	return composition;
}

static uint32_t raw_read32() {
	raw_read(coder, 4);

	uint64_t composition = 0ULL;

	composition |= (uint64_t) coder[3] << 24;
	composition |= (uint64_t) coder[2] << 16;
	composition |= (uint64_t) coder[1] << 8;
	composition |= (uint64_t) coder[0];

	return composition;
}

static uint64_t raw_read64() {
	raw_read(coder, 8);

	uint64_t composition = 0ULL;

	composition |= (uint64_t) coder[7] << 56;
	composition |= (uint64_t) coder[6] << 48;
	composition |= (uint64_t) coder[5] << 40;
	composition |= (uint64_t) coder[4] << 32;
	composition |= (uint64_t) coder[3] << 24;
	composition |= (uint64_t) coder[2] << 16;
	composition |= (uint64_t) coder[1] << 8;
	composition |= (uint64_t) coder[0];

	return composition;
}

static void raw_write(uint8_t* buffer, uint32_t length) {
	rfs_access_memory(fs, &write_address, length, WRITE); // Transforms the write address (and alloc new blocks if necessary) if we are at the end of a writable section

	fs->write(write_address, buffer, length);
	write_address += length;
}

static void raw_write8(uint8_t data) {
	raw_write(&data, 1);
}

static void raw_write16(uint16_t data) {
	coder[0] = data;
	coder[1] = data >> 8;

	raw_write(coder, 2);
}

static void raw_write32(uint32_t data) {
	coder[0] = data;
	coder[1] = data >> 8;
	coder[2] = data >> 16;
	coder[3] = data >> 24;

	raw_write(coder, 4);
}

static void raw_write64(uint64_t data) {
	coder[0] = data;
	coder[1] = data >> 8;
	coder[2] = data >> 16;
	coder[3] = data >> 24;
	coder[4] = data >> 32;
	coder[5] = data >> 40;
	coder[6] = data >> 48;
	coder[7] = data >> 56;

	raw_write(coder, 8);
}
