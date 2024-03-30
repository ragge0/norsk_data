/*
 * Copyright (c) 2024 Anders Magnusson.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Given a Nord10 assembly code, prints out its entry point in microcode.
 */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>

int epg(int, int);

int rom[4096];

int
main(int argc, char *argv[])
{
	FILE *fp;
	char hbuf[10];
	int i;

	if (argc != 2)
		errx(1, "usage: %s <ir>", argv[0]);

	if ((fp = fopen("prom.hex", "r")) == NULL)
		err(1, "fopen");
	for (i = 0; i < 1024; i++) {
		if (fgets(hbuf, 10, fp) == NULL)
			err(1, "fgets");
		rom[i] = strtol(hbuf, 0, 16);
	}
	fclose(fp);

	int ir = strtol(argv[1], NULL, 8);

	int rv = epg(ir, 0);

	printf("epg res: %o, rom %08x\n", rv, rom[rv]);

	return 0;
}
