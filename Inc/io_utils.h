/*
 * io_utils.h
 *
 *  Created on: 15 Oct 2019
 *      Author: Arion
 */

#ifndef IO_UTILS_H_
#define IO_UTILS_H_

#include <stdint.h>

#include "filesystem.h"

uint64_t fs_read64(FileSystem* fs, uint32_t address);
void fs_write64(FileSystem* fs, uint32_t address, uint64_t data);

#endif /* IO_UTILS_H_ */
