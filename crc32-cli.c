/*
 * A simple command-line program to exercise the hardware-
 * accelerated CRC-32 algorithms.
 *
 * Copyright IBM Corp. 2016
 * Author(s): Chris Zou <chriszou@ca.ibm.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "crc32-s390x.h"

int main(int argc, char *argv[]) {
	unsigned long length;
	unsigned long i;
	unsigned int initial_value;
	unsigned char *data;
	unsigned int crcle, crcbe, crccbe, crccle;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s initial_value length\n", argv[0]);
		exit(1);
	}

	initial_value = strtoul(argv[1], NULL, 0);
	length = strtoul(argv[2], NULL, 0);

	data = memalign(16, length);
	if (!data) {
		perror("memalign");
		exit(1);
	}

	srandom(1);
	for (i = 0; i < length; i++)
		data[i] = random() & 0xff;

	crcbe = crc32_be_vx(initial_value, data, length);
	crcle = crc32_le_vx(initial_value, data, length);
	crccbe = crc32c_be_vx(initial_value, data, length);
	crccle = crc32c_le_vx(initial_value, data, length);

	printf("IEEE big-endian CRC: %08x\n", crcbe);
	printf("IEEE little-endian CRC: %08x\n", crcle);
	printf("Castagnoli big-endian CRC: %08x\n", crccbe);
	printf("Castagnoli little-endian CRC: %08x\n", crccle);

	return 0;
}
