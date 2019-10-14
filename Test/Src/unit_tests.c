/*
 * unit_tests.c
 *
 *  Created on: Oct 14, 2019
 *      Author: pcoo56
 */

#include "unit_tests.h"
#include "emulator.h"

#ifdef TESTING

int main() {
	emu_init();
	emu_deinit();
	return 0;
}

#endif
