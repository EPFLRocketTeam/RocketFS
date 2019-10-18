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
	File* selected_file;

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

				selected_file = &(fs->files[file_id]);

				uint32_t hash = __hash_identifier(identifier);

				selected_file->first_block = block_id;
				selected_file->identifier = identifier;
				selected_file->identifier_hash = hash;
				selected_file->used_blocks = 1;
				selected_file->length = __compute_length(usage_table);
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
		selected_file = &(fs->files[file_id]);
		uint32_t first_block = selected_file->first_block;

		if(first_block > 0) {
			DataBlock block = fs->data_blocks[first_block];

			while((block = block.successor) > 0) {
				uint32_t address = block * fs->subsector_size;

				init_stream(stream, fs, address + 8 + 16, RAW); // Skip the file id and predecessor block id
				uint64_t usage_table = stream.read64();
				stream.close();

				selected_file->length += __compute_length(usage_table);
				selected_file->used_blocks++;
				selected_file->last_block = block;
			}
		}
	}
}

/*
 * Returns the allocated block ID
 */
uint16_t rfs_block_alloc(FileSystem* fs, FileType type) {
	uint16_t oldest_block;

	for(uint16_t block_id = PROTECTED_BLOCKS; block_id < NUM_BLOCKS; block_id++) {
		uint8_t* meta = &(fs->partition_table[block_id]);

		if(*meta == 0) {
			fs->total_used_blocks++;
			*meta = (type << 4) | 0b1100;
			fs->partition_table_modified = true;

			rfs_update_relative_time(fs);

			fs->erase_subsector(fs->subsector_size * block_id); // Prepare for writing

			return block_id;
		} else if((*meta) & 0xF < oldest_block){
			oldest_block = block_id;
		}
	}

	/* Device is full! Realloc oldest block. */

	uint8_t* oldest_block_meta = &(fs->partition_table[oldest_block]);

	if(*oldest_block_meta & 0xF > 0) { // Some correction for a better relative time repartition
		rfs_decrease_relative_time(fs);
	}

	*oldest_block_meta = (type << 4) | 0b1100; // Reset the entry in the partition table
	fs->partition_table_modified = true;

	fs->erase_subsector(fs->subsector_size * oldest_block); // Prepare reallocated block for writing

	return oldest_block;
}


void rfs_block_free(FileSystem* fs, uint16_t block_id) {
	if(block_id >= PROTECTED_BLOCKS) {
		fs->partition_table[block_id] = 0;
		fs->partition_table_modified = true;
	} else {
		fs->log("Error: Cannot free a protected block");
	}
}

void rfs_access_memory(FileSystem* fs, uint32_t* address, uint32_t length) {

}


/*
 * The relative time ranges from 0 to 16
 */
static rfs_update_relative_time(FileSystem* fs) {
	uint8_t anchor = fs->partition_table[0] >> 4; // Core block meta is used as a time reference
	uint8_t available_space = 16 - (fs->total_used_blocks * 16UL) / NUM_BLOCKS; // Ranges from 0 to 16

	if(anchor > available_space) {
		rfs_decrease_relative_time(fs);
	}
}

static rfs_decrease_relative_time(FileSystem* fs) {
	for(uint8_t block_id = 0; block_id < NUM_BLOCKS; block_id++) {
		uint8_t* meta = &(fs->partition_table[block_id]);

		if(*meta > 0) {
			(*meta)--;
		}
	}
}

/*
 * Utils
 */
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
