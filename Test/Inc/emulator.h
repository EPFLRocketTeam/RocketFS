/*
 * emulator.h
 *
 *  Created on: Oct 14, 2019
 *      Author: pcoo56
 */

#ifndef TEST_INC_EMULATOR_H_
#define TEST_INC_EMULATOR_H_


#include "unit_tests.h"

#include <stdint.h>

/*
 * Export emulator functions and constants only in development mode
 */
#ifdef TESTING



/*
 * Emulator functions
 */
void emu_init();
void emu_deinit();
void emu_read(uint32_t address, uint8_t* buffer, uint32_t length);
void emu_write(uint32_t address, uint8_t* buffer, uint32_t length);
void emu_erase_subsector(uint32_t address);
void emu_erase_sector(uint32_t address);



/*
 * Export constants required by io_bridge.h
 */
#define FS_ADDRESSABLE_SPACE (1 << 24) // 16MB (should be extended to 128MB)
#define FS_SECTOR_SIZE       (1 << 19) // 512KB
#define FS_SUBSECTOR_SIZE    (1 << 12) // 4KB



/*
 * Export functions required by io_bridge.h
 */
inline void fs_read(uint32_t a, uint8_t* b, uint32_t c) { emu_read(a, b, c); }
inline void fs_write(uint32_t a, uint8_t* b, uint32_t c) { emu_write(a, b, c); }
inline void fs_erase_subsector(uint32_t a) { emu_erase_subsector(a); }
inline void fs_erase_sector(uint32_t a) { emu_erase_sector(a); }




#endif



#endif /* TEST_INC_EMULATOR_H_ */
