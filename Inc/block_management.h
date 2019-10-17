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

/*
 * Since stm32f446 only has 128KB memory, we cannot afford more than 16-bytes data-blocks...
 */
typedef struct DataBlock {
	char identifier[4];		 // 4 chars
	uint32_t ancestor;	     // 4 bytes (to allow block reallocation)
	uint64_t written_length; // 8 bytes (necessary if we want to append data)
} DataBlock;

#endif /* INC_BLOCK_MANAGEMENT_H_ */
