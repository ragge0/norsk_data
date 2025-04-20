/*	$Id: ofile.c,v 1.7 2022/11/13 14:09:33 ragge Exp $	*/
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
#include <stdlib.h>
#include <unistd.h>

#include "as.h"

#define	ODBG(x)	if (odebug) printf x

/*
 * Write the object file.
 */
void
owrite()
{
	extern char **argvp, *cinfile;
	struct seg *seg;
	struct subseg *sub;
	int i, ch, nsz;

	p2_setseg(SEG_TEXT, 0);	/* start from beginning */

	/*
	 * Call ABI-specific routines to setup offsets in the output 
	 * object file.
	 */
	aoutsetup();
	/*
	 * Write the object data itself.  Mostly target- and ABI-independent.
	 */
	while ((ch = tmprd()) != EOF || !tmpeof()) {
		if (ch > INSNBASE+ninsn)
			aerror("tempfile sync error");
		if (ch >= INSNBASE)
			p2_instr(&insn[ch-INSNBASE]);
		else if (ch >= DIRBASE)
			p2_direc(&direc[ch-DIRBASE]);
		else switch (ch) {
		case DOTSEGMENT:
			i = tmprd();
			p2_setseg(i, tmprd());
			break;

		case LINENO:
			lineno = tmprd();
			if (mapflag)
				fprintf(mapfd, "%s:%d -> %06o\n",
				    cinfile, lineno, cdot);
			break;

		case FILENM:
			i = tmprd();
			cinfile = i < 0 ? "<stdin>" : argvp[i];
			break;
#ifdef MD_WORD_ADDR
		case DOTSYNC:
			while (cursub->odd)
				owbyte(0);
			break;
#endif

		default:
			aerror("owrite: %d", ch);
		}

	}

	/*
	 * Consistency check of dot calculation.
	 */
	for (i = 0; i < segcnt; i++) {
		if ((seg = getseg(i)) == NULL)
			continue;
		for (sub = seg->sub; sub; sub = sub->next) {
			cursub = sub;
#ifdef MD_WORD_ADDR
			/* write out remaining words */
			while (sub->odd)
				owbyte(0);
#endif
			/* segment aligning by writing out bytes */
			nsz = ROUNDUP(sub->dot, MD_SEGALIGN);
			while (nsz > sub->dot)
				owbyte(0);
			nsz -= sub->suboff;
			if (sub->size != nsz)
				aerror("pass size error: p1=%d != p2=%d",
				    sub->size, nsz);
		}
	}
	aoutwrel();	/* Write out relocation information */
	aoutwsym();	/* Write out symbols and strings */
}


#ifdef MD_WORD_ADDR
void
owbyte(int ch)
{
	ODBG(("write 8bit (%d): 0x%x (0%o)  pos %lo\n",
	    cdot, ch & 0xff, ch & 0xff, ftell(ofd)));

	cursub->oddsave |= (ch & 0377) << 8 * (md_byte_ltor ^ cursub->odd);
	if (cursub->odd) {
		ow2byte(cursub->oddsave);
		cursub->oddsave = 0;
	}
	cursub->odd ^= 1;
}

#else
void
owbyte(int ch)
{
	ODBG(("write 8bit (%d): 0x%x (0%o)  pos %lo\n",
	    cdot, ch & 0xff, ch & 0xff, ftell(ofd)));
	fputc(ch, ofd);
	cdot = cdot + 1;
}
#endif

void
ow2byte(int ch)
{
	uint16_t n = ch;

	ODBG(("write 16bit (%d): 0x%x (0%o)  pos %lo\n",
		    cdot, ch & 0xffff, ch & 0xffff, ftell(ofd)));
#ifdef MD_BIG_ENDIAN
	n = (n << 8) | (n >> 8);
#endif
	fputc(n, ofd);
	fputc(n >> 8, ofd);
	cdot += 2/MD_NBPWD;
}

void
ow4byte(long ch)
{
	uint32_t n = ch;

	ODBG(("write 32 (%d): 0x%lx (0%lo) pos %lo\n",
	    cdot, ch, ch, ftell(ofd)));
#ifdef MD_BIG_ENDIAN
	n = (n << 24) | ((n & 0xff00) << 8) |
	    ((n >> 8) & 0xff00) | ((n >> 24) & 0xff);
#else
#ifdef MD_PDP_ENDIAN
	n = ((n >> 16) & 0xff) | ((n >> 24) & 0xff00) |
	    ((n & 0xff) << 16) | ((n & 0xff00) << 24);
#endif
#endif
	fputc(n, ofd);
	fputc(n >>= 8, ofd);
	fputc(n >>= 8, ofd);
	fputc(n >>= 8, ofd);
	cdot += 4/MD_NBPWD;
}

