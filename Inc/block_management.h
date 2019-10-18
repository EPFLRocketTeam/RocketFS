/*
 * block_management.h
 *
 *  Created on: 17 Oct 2019
 *      Author: Arion
 */

#ifndef INC_BLOCK_MANAGEMENT_H_
#define INC_BLOCK_MANAGEMENT_H_

#include <stdint.h>
#include <stdbool.h>

#include "filesystem.h"

/*
 * Since stm32f446 only has 128KB memory, we cannot afford more than 16-bytes data-blocks...
 */
typedef struct DataBlock {
	uint16_t successor;
} DataBlock;


void rfs_init_block_management(FileSystem* fs);
uint16_t rfs_block_alloc(FileSystem* fs, FileType type);
void rfs_block_free(FileSystem* fs, uint16_t block_id);
void rfs_access_memory(FileSystem* fs, uint32_t* address, uint32_t length);


#endif /* INC_BLOCK_MANAGEMENT_H_ */
