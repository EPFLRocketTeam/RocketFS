/*
 * file.c
 *
 *  Created on: 19 Oct 2019
 *      Author: Arion
 */

#include "file.h"


uint32_t hash_filename(const char* name) {
	uint32_t hash = 13;

	for(uint8_t i = 0; i < 16; i++) {
		hash += 37 * hash + name[i] * 59;
	}

	return hash;
}
