/*
 * Test z13 checksum algorithm and verify the results using the
 * Rocksoft Model CRC algorithm. Based on crc32_stress.c from:
 *
 * https://github.com/antonblanchard/crc32-vpmsum
 *
 * Copyright (C) 2015 Anton Blanchard <anton@au.ibm.com>, IBM
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "crcmodel.h"
#include "crc32-s390x.h"

#define MAX_CRC_LENGTH (128*1024)

static unsigned int rocksoft_crc(unsigned int crc, unsigned char *p,
				 unsigned long len)
{
	cm_t cm_t = { 0, };
	int i;

	cm_t.cm_width = 32;	/* Width in bits [8,32]       */
	cm_t.cm_poly  = CRC;	/* The algorithm's polynomial */
	cm_t.cm_init  = crc;	/* Initial checksum value     */
#ifdef REFLECT
	cm_t.cm_refin = TRUE;	/* Reflect input bytes     */
	cm_t.cm_refot = TRUE;	/* Reflect output checksum */
#else
	cm_t.cm_refin = FALSE;
	cm_t.cm_refot = FALSE;
#endif
#ifdef CRC_XOR
	cm_t.cm_init ^= 0xffffffff;	/* XOR the initial checksum */
	cm_t.cm_xorot = 0xffffffff;	/* ...and XOR the result    */
#else
	cm_t.cm_xorot = 0x0;		/* XOR the result with 0 (no-op) */
#endif
	cm_ini(&cm_t);			/* Now initialize the CRC model  */

	/* Process a single byte at a time. */ 
	for (i = 0; i < len; i++)
		cm_nxt(&cm_t, p[i]);

	/* Return the CRC value for the message bytes processed so far. */
	return cm_crc(&cm_t);
}

#define VMX_ALIGN	16
#define VMX_ALIGN_MASK	(VMX_ALIGN-1)

int main(void)
{
	unsigned char *data;
	unsigned int crc = 0, verify = 0, trials = 0;
	unsigned int shortcases[] = { 1, 2, 3, 4, 7, 8, 9, 15, 16, 17 };

	data = memalign(VMX_ALIGN, MAX_CRC_LENGTH+VMX_ALIGN_MASK);
	if (!data) {
		perror("memalign");
		exit(1);
	}

	srandom(1);

	while (1) {
		unsigned int len, offset, orig_crc = crc;
		unsigned long i;

		for (i = 0; i < MAX_CRC_LENGTH; i++)
			data[i] = random() & 0xff;

		/* Make sure we test cases with shorter buffer sizes. */
		if (trials < 10) {
			len = shortcases[trials++];
		} else {
			len = random() % MAX_CRC_LENGTH;
		}
		offset = random() & VMX_ALIGN_MASK;

		crc = CRC_VX_FUNC(crc, data+offset, len);
		verify = rocksoft_crc(verify, data+offset, len);

		printf("Testing offset %08x length %08x\n", offset, len);

		if (crc != verify) {
			fprintf(stderr, "FAILURE: input 0x%08x got 0x%08x expected 0x%08x (len %d)\n", orig_crc, crc, verify, len);
			crc = verify;
		}
	}

	return 0;
}
