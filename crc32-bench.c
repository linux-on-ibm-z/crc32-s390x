/*
 * Perform a large number of checksums on random data of given
 * size, for timing purpose. Based on crc32_bench.c from:
 *
 * https://github.com/antonblanchard/crc32-vpmsum
 *
 * Copyright (C) 2015 Anton Blanchard <anton@au.ibm.com>, IBM
 *
 */

#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include "crc32-s390x.h"

int main(int argc, char *argv[])
{
	unsigned long length, iterations;
	unsigned char *data;
	unsigned long i;
	unsigned int crc = 0;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s length iterations\n", argv[0]);
		exit(1);
	}  

	length = strtoul(argv[1], NULL, 0);
	iterations = strtoul(argv[2], NULL, 0);

	data = memalign(getpagesize(), length);

	srandom(1);
	for (i = 0; i < length; i++)
		data[i] = random() & 0xff;

	for (i = 0; i < iterations; i++)
		crc = CRC_FUNC(crc, data, length);

	printf("CRC: %08x\n", crc);

	return 0;
}
