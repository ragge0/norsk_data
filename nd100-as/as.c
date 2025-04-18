/*	$Id: as.c,v 1.12 2022/11/12 16:23:44 ragge Exp $	*/
/*
 * Copyright (c) 2022 Anders Magnusson (ragge@ludd.ltu.se).
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

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "as.h"

static void usage(void);
static void setfn(int);

char *ofile, *cinfile;
char **argvp;
FILE *ofd;
int errval, pass;
int tdebug, wdebug, sdebug, edebug, gdebug, odebug, uflag;
extern int lineno;

#ifndef md_big_endian
int md_big_endian, md_little_endian, md_pdp_endian;
#endif
#ifndef md_byte_ltor
int md_byte_ltor;
#endif

int
main(int argc, char *argv[])
{
	int ch;

	argvp = argv;

	while ((ch = getopt(argc, argv, "um:o:D:")) != -1) {
		switch (ch) {
		case 'D': /* debug */
			for (ch = 0; optarg[ch]; ch++) {
				if (optarg[ch] == 't')
					tdebug++; /* print out tempfile */
				if (optarg[ch] == 's')
					sdebug++; /* symbols */
				if (optarg[ch] == 'e')
					edebug++; /* expr */
				if (optarg[ch] == 'w')
					wdebug++; /* write tempfile */
				if (optarg[ch] == 'g')
					gdebug++; /* SDI graph */
				if (optarg[ch] == 'o')
					odebug++; /* obuf debug */
			}
			break;

		case 'u':
			uflag++;
			break;

		case 'o':
			if (ofile)
				errx(1, "multiple -o");
			ofile = optarg;
			break;

		case 'm': /* Target/dependent */
			myoptions(optarg);
			break;

		default:
			usage();
		}
	}
	if (ofile == NULL)
		ofile = "a.out";

	pass = 1;
	tmpinit();
	syminit();
	exprinit();
	seginit();
	mach_init();	/* target-specific setup */

	p1_setseg(".text", 0);	/* Initial segment */

	if ((ofd = fopen(ofile, "w")) == NULL)
		err(1, "fopen %s", ofile);

	argc -= optind;
	argv += optind;

	if (argc == 0) {
		setfn(-1);
		lninc('\n');
		parse();
	} else while (argc > 0) {
		setfn(argv - argvp);
		if (freopen(cinfile, "r", stdin) == NULL)
			err(1, "open %s", cinfile);
		argc--;
		argv++;
		lineno = 0;
		lninc('\n');
		parse();
	}

	relocate();
	tmprew();

	pass = 2;
	owrite();

	fclose(ofd);
	if (errval)
		unlink(ofile);
	return errval;
}

static void
cmerror(char *t, char *str, va_list ap)
{
	fprintf(stderr, "%s: %s, line %d: ", t, cinfile, lineno);
	vfprintf(stderr, str, ap);
	fprintf(stderr, "\n");
	fflush(stderr);
}

void
werror(char *str, ...)
{
	va_list ap;

	va_start(ap, str);
	cmerror("warning", str, ap);
	va_end(ap);
}

void
error(char *str, ...)
{
	va_list ap;
	static int ecount;

	va_start(ap, str);
	cmerror("error", str, ap);
	va_end(ap);

	errval = 1;
	if (ecount++ > 10)
		exit(1);
}

void
aerror(char *str, ...)
{
	va_list ap;

	va_start(ap, str);
	/* ignore internal errors that can be side effects */
	if (errval == 0)
		cmerror("internal error", str, ap);
	va_end(ap);
	exit(1);
}

void
usage(void)
{
	fprintf(stderr, "Usage: %s [-o ofile] <infile>\n", argvp[0]);
	exit(1);
}

/*
 * init new chunk structure.
 */
struct mchunk *
init_mchunk(int chsz)
{
	struct mchunk *mch;

	mch = xmalloc(sizeof(struct mchunk));
	mch->chsz = chsz;
	mch->link = xcmalloc(sizeof(void **) * (mch->nlinks = 10));
	mch->link[0] = xcmalloc(mch->chsz * SYMCHSZ);
	mch->lpos = mch->spos = 0;
	return mch;
}

/*
 * Return next free chunk.
 */
void *
new_mchunk(struct mchunk *mch)
{
	if (mch->spos == SYMCHSZ) {
		if (++mch->lpos == mch->nlinks) { /* Expand pointers */
			if ((mch->link = realloc(mch->link,
			    (mch->nlinks += 10))) == NULL)
				err(1, "realloc");
		}
		mch->spos = 0;
		mch->link[mch->lpos] = xcmalloc(mch->chsz * SYMCHSZ);
	}
	return &mch->link[mch->lpos][mch->chsz * mch->spos++];
}

void *
get_mchunk(struct mchunk *mch, int ch)
{
	if (MKNELEM(mch) < ch)
		error("get_mchunk %d > %d", MKNELEM(mch), ch);
	return &mch->link[ch / SYMCHSZ][(ch % SYMCHSZ) * mch->chsz];
}

void *
xstrdup(char *str)
{
	void *rv;

	if ((rv = strdup(str)) == NULL)
                error("out of memory!");
	return rv;
}

void *
xmalloc(int sz)
{
	void *rv;

	if ((rv = malloc(sz)) == NULL)
                error("out of memory!");
	return rv;
}

void *
xcmalloc(int sz)
{
	return memset(xmalloc(sz), 0, sz);
}

void
lninc(int c)
{
        if (c != '\n')
                return;

        lineno++;
        tmpwri(LINENO);
        tmpwri(lineno);
}

void
setfn(int idx)
{
	cinfile = idx < 0 ? "<stdin>" : argvp[idx];
	tmpwri(FILENM);
	tmpwri(idx);
}
