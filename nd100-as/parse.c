/*	$Id: asparse.y,v 1.13 2022/11/12 16:23:44 ragge Exp $	*/
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <stdio.h>
#include <err.h>
#include <string.h>

#include "as.h"

void setlbl(struct hshhdr *);
void dotbwlq(int s);

void
parse(void)
{
	int t;

	while ((t = tok_get()) != 0) {
		if (t == DELIM) {
			lninc(yylval.val);
			continue;
		}
		switch (t) {
		case LABEL:
			dotsync();
			setlbl(yylval.hdr);
			continue;

		case DIREC:
			p1_direc((struct direc *)yylval.hdr);
			break;

		case INSTR:
			dotsync();
			tmpwri(yylval.hdr->num);
			p1_instr((struct insn *)yylval.hdr);
			break;

		default:
			serror("expected command, got", t);
		}
		if ((t = tok_get()) != DELIM)
			serror("junk after statement", t);
		lninc(yylval.val);
	}
}

void
setlbl(struct hshhdr *h)
{
	struct symbol *sp = (void *)h;

	if (sp->flsdi & SYM_DEFINED)
		errx(1, "%s redefined", sp->hhdr.name);
	sp->flsdi |= SYM_DEFINED;
	sp->val = cdot;
	sp->sub = cursub;
	S_SETSDI(sp, cursub->nsdi);
	if (sdebug)
		printf("sdebug: %s defined at pos %d\n", h->name, cdot);
}

#ifdef MD_WORD_ADDR
void
dotsync(void)
{
	if (cursub->odd) {
		cursub->dot++;
		cursub->odd = 0;
	}
	tmpwri(DOTSYNC);
}
#endif
