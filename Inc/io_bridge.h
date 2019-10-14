/*
 * io_bridge.h
 *
 *  Created on: Oct 14, 2019
 *      Author: pcoo56
 */

#ifndef INC_IO_BRIDGE_H_
#define INC_IO_BRIDGE_H_


#include <stdint.h>


#ifndef FS_ADDRESSABLE_SPACE
#error Please define the FS_ADDRESSABLE_SPACE constant
#endif

#ifndef FS_SECTOR_SIZE
#error Please define the SECTOR_SIZE constant
#endif

#ifndef FS_SUBSECTOR_SIZE
#error Please define the SUBSECTOR_SIZE constant
#endif


void fs_read(uint32_t address, uint8_t* buffer, uint32_t length);
void fs_write(uint32_t address, uint8_t* buffer, uint32_t length);
void fs_erase_subsector(uint32_t address);
void fs_erase_sector(uint32_t address);


#endif /* INC_IO_BRIDGE_H_ */
