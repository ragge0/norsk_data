/*      $Id: as.h,v 1.23 2022/11/13 14:09:33 ragge Exp $        */
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

#include <err.h>
#include <string.h>
#include <unistd.h>

#include "as.h"

#if defined(MD_AOUT16) || defined(MD_AOUT16_ZREL)
/*
 * a.out definition for the:
 *	Nova computer, using zero-page
 *	PDP-11 computer
 *
 * The output from the assembler is directly an executable unless there
 * are undefined externals.
 *
 * An a.out file header have 8 16-bit words and looks like below.
 * All units are in words (since that is the smallest unit on Nova).
 *
 *	a_magic		 magic number, 0407
 *	a_text		 size of text segment
 *	a_data		 size of initialized data
 *	a_bss		 size of uninitialized data
 *	a_syms		 size of symbol table
 *	a_entry		 entry point
 *	a_zp		 size of zero page (if present)
 *	a_flag		 relocation info stripped (2BSD)
 *
 * The segment layout in the binary looks like this:
 *
 *	+---------------------------------------+
 *	|  A.out header				|
 *	+---------------------------------------+
 *	|  (Zero-page)				|
 *	+---------------------------------------+
 *	|  Text					|
 *	+---------------------------------------+
 *	|  Data					|
 *	+---------------------------------------+
 *	|  Zero-page relocation			|
 *	+---------------------------------------+
 *	|  Text relocation			|
 *	+---------------------------------------+
 *	|  Data relocation			|
 *	+---------------------------------------+
 *	|  Symbol table				|
 *	+---------------------------------------+
 *	|  String table				|
 *	+---------------------------------------+
 */

/* Use byte offsets in the assembler */
#define	AOUTSYMSZ	8	/* size of symbol struct (in bytes) */
#define	AOUTHDRSZ	16	/* size of header (in bytes) */
#define	A_MAGIC1	0407

#define	N_UNDF		0x0
#define	N_ABS		0x1
#define	N_TEXT		0x2
#define	N_DATA		0x3
#define	N_BSS		0x4
#define	N_ZREL		0x5
#define	N_EXT		0x20

static long upsuboff(struct subseg *sub, int segoff, long fpos);

/* in bytes, for outfile offsets */
static long off_text, off_data, off_zp;
static long off_trel, off_drel, off_zrel;
static long off_syms, off_str;
#define tseg segary[SEG_TEXT]
#define dseg segary[SEG_DATA]
#ifdef MD_AOUT16_ZREL
#define zseg segary[SEG_ZREL]
#endif

/*
 * Calculate the in-file offsets for destination.
 */
void
aoutsetup()
{
	long fpos;
	int i, segoff, ocdot;

	tseg = getseg(SEG_TEXT);
	dseg = getseg(SEG_DATA);

#ifdef MD_AOUT16_ZREL
	zseg = getseg(SEG_ZREL);
#define	ZSEGSZ	zseg->size
#else
#define	ZSEGSZ	0
#endif

	/* Calculate binary offsets in the output file */
	off_zp = AOUTHDRSZ;
	off_text = off_zp + ZSEGSZ * MD_NBPWD;
	off_data = off_text + tseg->size * MD_NBPWD;
	off_zrel = off_data + dseg->size * MD_NBPWD;
	off_trel = off_zrel + ZSEGSZ * MD_NBPWD;
	off_drel = off_trel + tseg->size * MD_NBPWD;
	off_syms = off_drel + dseg->size * MD_NBPWD;
	off_str = off_syms + nextsym * AOUTSYMSZ;

	/*
	 * Update all offsets (file, segment) for all sections.
	 * This makes a (possible) executable binary.
	 * The segment order is (zrel,) text, data, bss
	 */
	segoff = 0;	/* initial segment offset */
	fpos = off_zp;	/* initial in-file offset */
#ifdef MD_AOUT16_ZREL
	if (zseg->size)
		fpos = upsuboff(zseg->sub, segoff, fpos);
	segoff += zseg->size;
#endif

	for (i = 0; i < 3; i++) {
		struct seg *seg;

		if ((seg = segary[i]) == NULL)
			continue;

		if (seg->size)
			fpos = upsuboff(seg->sub, segoff, fpos);

		segoff += seg->size;
	}

	/* create the a.out header.  All sizes are known by now */
	fseek(ofd, 0, SEEK_SET);
	ocdot = cdot;
	ow2byte(A_MAGIC1);
	ow2byte(tseg->size);
	ow2byte(dseg->size);
	ow2byte(getseg(SEG_BSS)->size);
	ow2byte(nextsym * AOUTSYMSZ/MD_NBPWD);
	ow2byte(0);
	ow2byte(ZSEGSZ);
	ow2byte(0);
	cdot = ocdot;

	/* update symbol offsets as well */
	for (i = 0; i < nextsym; i++) {
		struct symbol *sp = SYMGET(i);
		if (sp->sub == NULL)
			continue;
		if (sp->sub->segnum == SEG_TEXT ||
		    sp->sub->segnum == SEG_DATA)
			sp->val += ZSEGSZ;
		if (sp->sub->segnum == SEG_DATA)
			sp->val += tseg->size;
	}
}

long
upsuboff(struct subseg *sub, int segoff, long fpos)
{
	for (; sub; sub = sub->next) {
		sub->dot += segoff;
		sub->suboff += segoff;
		sub->foff = fpos;
		fpos += sub->size * MD_NBPWD;
	}
	return fpos;
}

/*
 * Old BSD relocation word, stored as an image of the corresponding segment.
 * For no relocation the word is zero.
 *
 * This format only exists on 16-bit machines.
 */
static void
wrtreloc(struct subseg *sub)
{
	struct reloc *r;
	int i, rw;
	int nextmch;

	nextmch = 0;
	r = sub->rmch ? get_mchunk(sub->rmch, nextmch++) : 0;
	for (i = 0; i < sub->size; i++) {
		rw = 0;
		if (r && r->addr == i) {
			rw = relwrd(r);
			r = get_mchunk(sub->rmch, nextmch++);
		}
		ow2byte(rw);
	}
}

/*
 * Write out relocations.
 * Relocs are stored in the order that they were found (their underlying
 * data was written out to the final file).
 */
void
aoutwrel()
{
	struct subseg *sub;

	fseek(ofd, off_zrel, SEEK_SET);
#ifdef MD_AOUT16_ZREL
	for (sub = zseg->sub; sub; sub = sub->next)
		wrtreloc(sub);
#endif
	for (sub = tseg->sub; sub; sub = sub->next)
		wrtreloc(sub);
	for (sub = dseg->sub; sub; sub = sub->next)
		wrtreloc(sub);
}

/*
 * Write out symbols and strings.
 */
void
aoutwsym()
{
	char strbuf[OBUFSZ];
	struct symbol *sp;
	long strboff;
	int i, ws, strpos;
	char *strp;

	strboff = off_str;
	strpos = 4;

	for (i = 0; i < nextsym; i++) {
		sp = SYMGET(i);
		ow4byte(strpos);
		ws = N_EXT;	/* Mark as undefined externals */
		if (sp->flsdi & SYM_DEFINED) {
			ws = N_ABS;
			if (sp->sub) {
#ifdef MD_AOUT16_ZREL
				if (sp->sub->segnum == SEG_ZREL)
					ws = N_ZREL;
				else
#endif
				    if (sp->sub->segnum == SEG_TEXT) {
					ws = N_TEXT;
				} else if (sp->sub->segnum == SEG_DATA) {
					ws = N_DATA;
				} else if (sp->sub->segnum == SEG_BSS) {
					ws = N_BSS;
				}
			}
			if (sp->flsdi & SYM_GLOBAL)
				ws |= N_EXT;
		}
		ow2byte(ws);
		ow2byte(sp->val);

		/* save symbol name */
		strp = sp->hhdr.name;
		do {
			strbuf[strpos++] = *strp;
			if (strpos == OBUFSZ) {
				long spos = ftell(ofd);
				fseek(ofd, strboff, SEEK_SET);
				fwrite(strbuf, OBUFSZ, 1, ofd);
				strpos = 0;
				strboff += OBUFSZ;
				fseek(ofd, spos, SEEK_SET);
			}
		} while (*strp++);
	}
	if (strpos&1)
		strbuf[strpos++] = 0;
	fseek(ofd, strboff, SEEK_SET);
	fwrite(strbuf, strpos, 1, ofd);
	fseek(ofd, off_str, SEEK_SET);
	ow4byte(strboff+strpos-off_str);
}
#endif /* MD_AOUT16 */
