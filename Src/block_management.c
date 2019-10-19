/*
 * block_management.c
 *
 *  Created on: 17 Oct 2019
 *      Author: Arion
 */

#include "block_management.h"

#define BLOCK_MAGIC_NUMBER 0xC0FFEE42
#define BLOCK_HEADER_SIZE 16
/*
 * 0...3:  Magic number
 * 4...5:  Related file ID
 * 6...7:  Predecessor block ID
 * 8...15: Usage table
 */

void rfs_init_block_management(FileSystem* fs) {
	uint8_t identifier[16];
	Stream stream;
	File* selected_file;

	/*
	 * First pass: Detect all files.
	 */
	for(uint16_t block_id = PROTECTED_BLOCKS; block_id < NUM_BLOCKS; block_id++) {
		uint8_t meta_data = fs->partition_table[block_id];

		if(meta_data > 0) {
			uint32_t address = fs->block_size * block_id;

			init_stream(stream, fs, address, RAW);

			uint32_t magic = stream.read32();
			uint16_t file_id = stream.read16();
			uint16_t predecessor = stream.read16();
			uint64_t usage_table = stream.read64();

			if(magic != BLOCK_MAGIC_NUMBER) {
				fs->log("Warning: Invalid magic number");
			}

			if(predecessor == 0) {
				stream.read(identifier, 16);

				selected_file = &(fs->files[file_id]);

				uint32_t hash = hash_filename(identifier);

				selected_file->first_block = block_id;
				selected_file->filename = identifier;
				selected_file->hash = hash;
				selected_file->used_blocks = 1;
				selected_file->length = __compute_block_length(usage_table);
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

			while(block.successor > 0) {
				selected_file->length += rfs_compute_block_length(block.successor);
				selected_file->used_blocks++;
				selected_file->last_block = block;

				block = fs->data_blocks[block.successor];
			}
		}
	}
}

/*
 * Returns the allocated block ID
 *
 * No block header is written.
 * Only the partition table is modified.
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

			fs->erase_block(fs->block_size * block_id); // Prepare for writing

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

	fs->erase_block(fs->block_size * oldest_block); // Prepare reallocated block for writing

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

uint32_t rfs_compute_block_length(FileSystem* fs, uint16_t block_id) {
	Stream stream;
	uint32_t address = block_id * fs->block_size;

	init_stream(&stream, fs, address + 8, RAW); // Skip the file id and predecessor block id
	uint64_t usage_table = stream.read64();
	stream.close();

	return __compute_block_length(usage_table);
}

uint32_t rfs_get_block_base_address(FileSystem* fs, uint16_t block_id) {
	return block_id * fs->block_size + BLOCK_HEADER_SIZE;
}

void rfs_block_write_header(FileSystem* fs, uint16_t block_id, uint16_t file_id, uint16_t predecessor) {
	Stream stream;
	init_stream(stream, fs, block_id * fs->block_size, RAW);

	stream.write32(BLOCK_MAGIC_NUMBER);
	stream.write16(file_id);
	stream.write16(predecessor);

	stream.close();
}

/*
 * To understand this function, please remember how NOR flash memories work :-)
 *
 *   normalised_end       normalised_begin
 * 		   | 			         |
 * 1111111100000000000000000000000111111111 =
 *
 * 1111111100000000000000000000000000000000 OR
 * 0000000000000000000000000000000111111111
 *
 * 1111111100000000000000000000000000000000 = (~0) << (1  + normalised_end)
 * 0000000000000000000000000000000111111111 = (~0) >> (64 - normalised_begin)
 */
void rfs_block_update_usage_table(FileSystem* fs, uint32_t write_begin, uint32_t write_end) {
	uint16_t block_id = write_begin / fs->block_size;

	uint64_t normalised_begin = (write_begin % fs->block_size) / 64;
	uint64_t normalised_end = (write_end % fs->block_size) / 64;

	uint64_t usage_bit_mask = ((~0) << (1 + normalised_end)) | ((~0) >> (64 - normalised_begin));

	Stream stream;
	init_stream(&stream, fs, block_id * fs->block_size + 8, RAW);

	stream.write64(usage_bit_mask);

	stream.close();
}



/*
 * This only provides forward memory protection.
 * Do not attempt to access memory before the address provided by rfs_block_alloc().
 * Implementing full memory protection would cost memory and is not absolutely necessary.
 * Returns false if the memory access is denied (end of file).
 */
bool rfs_access_memory(FileSystem* fs, uint32_t* address, uint32_t length, AccessType access_type) {
	if(length <= fs->block_size - BLOCK_HEADER_SIZE) {
		uint32_t internal_address = *address % fs->block_size;

		if(internal_address < BLOCK_HEADER_SIZE) {
			*address += BLOCK_HEADER_SIZE - internal_address;
		}

		if(internal_address + length > fs->block_size) {
			uint16_t block_id = *address / fs->block_size;
			uint16_t successor_block = fs->data_blocks[block_id].successor;

			if(successor_block > 0) {
				*address = successor_block * fs->block_size + BLOCK_HEADER_SIZE;
			} else {
				switch(access_type) {
				case READ:
					return false; // End of file
				case WRITE:
					uint8_t file_type = fs->partition_table[block_id] >> 4;
					uint16_t new_block_id = rfs_block_alloc(fs, file_type); // Allocate a new block

					Stream stream;
					init_stream(&stream, fs, new_block_id * fs->block_size + 4, RAW);
					uint16_t file_id = stream.read16();
					stream.close();

					rfs_block_write_header(fs, file_id, block_id);

					File* file = &(fs->files[file_id]);

					file->used_blocks += 1;
					file->last_block = new_block_id;
					// Improve performance and do not compute the exact file length

					break;
				default:
					return false; // Not implemented
				}
			}
		}

		if(access_type == WRITE) {
			rfs_block_update_usage_table(*address, *address + length);
		}

		return true;
	} else {
		return false;
	}
}


/*
 * The relative time ranges from 0 to 16
 */
static rfs_update_relative_time(FileSystem* fs) {
	uint8_t anchor = fs->partition_table[0] & 0xF; // Core block meta is used as a time reference
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
 * Written length is not an actual length but must be regarded as a bit chain.
 * Consider those examples:
 *
 * 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111: Block completely empty
 * 11111111 11111111 11111111 11111111 11111111 11111111 11110000 00000000: Block uses 18.75% of subsector_size
 * 11111111 11111111 00000000 00000000 00000000 00000000 00000000 00000000: Block uses 75% of subsector_size
 * 10000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000: Block uses 98.4% of subsector_size
 *
 */
static uint32_t __compute_block_length(uint64_t usage_table) {
	uint32_t length = 0;

	while(usage_table) {
		length += ~(usage_table & 0b1);
		usage_table >>= 1;
	}

	return length;
}
