/*
 * block_management.c
 *
 *  Created on: 17 Oct 2019
 *      Author: Arion
 */

#include "block_management.h"
#include "stream.h"

void rfs_init_block_management(FileSystem* fs) {
	uint8_t identifier[16];
	Stream stream;

	/*
	 * First pass: Detect all files.
	 */
	for(uint16_t block_id = 0; block_id < NUM_BLOCKS; block_id++) {
		uint8_t meta_data = fs->partition_table[block_id];

		if(meta_data > 0) {
			uint32_t address = fs->subsector_size * block_id;

			init_stream(stream, fs, address, RAW);

			uint8_t file_id = stream.read8();
			uint16_t predecessor = stream.read16();
			uint64_t usage_table = stream.read64();

			if(predecessor == 0) {
				stream.read(identifier, 16);

				uint32_t hash = __hash_identifier(identifier);

				fs->files[file_id].first_block = block_id;
				fs->files[file_id].identifier = identifier;
				fs->files[file_id].identifier_hash = hash;
				fs->files[file_id].used_blocks = 1;
				fs->files[file_id].length = __compute_length(usage_table);
			} else {
				fs->data_blocks[predecessor].successor = block_id;
			}

			stream.close();
		}
	}

	/*
	 * Second pass: Resolve all block links and compute storage statistics.
	 * Warning: This function traverses all blocks and does not check for cyclicity.
	 * This may cause infinite loops in extreme data corruption cases. TODO: Check cyclicity.
	 */
	for(uint8_t file_id = 0; file_id < NUM_FILES; file_id++) {
		uint32_t first_block = fs->files[file_id].first_block;

		if(first_block > 0) {
			DataBlock block = fs->data_blocks[first_block];

			while((block = block.successor) > 0) {
				uint32_t address = block * fs->subsector_size;

				init_stream(stream, fs, address + 8 + 16, RAW); // Skip the file id and predecessor block id
				uint64_t usage_table = stream.read64();
				stream.close();

				fs->files[file_id].length += __compute_length(usage_table);
				fs->files[file_id].used_blocks++;
				fs->files[file_id].last_block = block;
			}
		}
	}
}

void rfs_block_alloc(FileSystem* fs) {

}

void rfs_block_free(FileSystem* fs) {

}


static uint32_t __hash_identifier(uint8_t* identifier) {
	uint32_t hash = 13;

	for(uint8_t i = 0; i < 16; i++) {
		hash += 37 * hash + identifier[i] * 59;
	}

	return hash;
}

/*
 * Written length is not an actual length but must be regarded as a bit chain.
 * Consider those examples:
 *
 * 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111: Block completely empty
 * 11111111 11111111 11111111 11111111 11111111 11111111 11110000 00000000: Block uses 18.75% of subsector_size
 * 11111111 11111111 00000000 00000000 00000000 00000000 00000000 00000000: Block uses 75% of subsector_size
 * 10000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000: Block uses 98.4% of subsector_size
 *
 */
static uint32_t __compute_length(uint64_t usage_table) {
	uint32_t length = 0;

	while(usage_table) {
		length += usage_table & 0b1;
		usage_table >>= 1;
	}

	return length;
}
