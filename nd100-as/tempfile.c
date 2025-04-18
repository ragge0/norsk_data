/*	$Id: tempfile.c,v 1.11 2022/11/12 16:23:44 ragge Exp $	*/
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


#include <string.h>
#include <stdio.h>
#include <err.h>

#include "as.h"

/*
 * The temp file consists of integers.  First int tells which type follows,
 * following ints are args to the first.
 *
 * Basic defined integers:
 *	- <directive index> into directive table, followed by args, see below
 *	- <symtab index> into the symtab entry
 *	- <instruction index> + insn-dependent arguments.
 *
 * Specific directive args:
 *	- DOTSEGMENT + segment index (number).
 *	- DOTBYTE/DOTWORD/... + expression.
 *
 * Expressons written in prefix notation, left-right.
 */


static void prtmpfile(void);


static FILE *tfp;

void
tmpinit(void)
{
	if ((tfp = tmpfile()) == NULL)
		err(1, "tmpfile");
}

void
tmpwri(int w)
{
	if (wdebug)
		printf("tmpwri: %d\n", w);
	if (pass != 1)
		aerror("tmpwrite not in pass1");

	if (putw(w, tfp))
		err(1, "putw");
}

void
tmprew(void)
{
	if (wdebug)
		printf("rewind\n");
	rewind(tfp);
	if (tdebug) {
		prtmpfile();
		if (wdebug)
			printf("rewind\n");
		rewind(tfp);
	}
}

int
tmprd(void)
{
	int w;

	w = getw(tfp);
	if (wdebug)
		printf("tmprd: %d\n", w);
	return w;
}

int
tmpeof(void)
{
	return feof(tfp);
}

void
prtmpfile(void)
{
#if 0
	extern struct seg **seglink;
	struct symbol *sp;
	int ch;

	for (;;) {
		if ((ch = tmprd()) == EOF && tmpeof())
			break;
		if (myprint(&ch))
			continue;
		switch (ch) {
		case DOTSEGMENT:
			tmprd();
			tmprd();
			printf("%s\n", "foo");
			break;
		case D_1BYTE:
		case D_2BYTE:
		case D_4BYTE:
			printf("	.%s ", ch == D_1BYTE ? "byte" :
			    ch == D_2BYTE ? "word" : "long");
			prexpr(0);
			printf("\n");
			break;

		case DOTFILL:
			printf(".fill %d,", tmprd());
			printf("%d,", tmprd());
			printf("%d\n", tmprd());
			break;

		case D_LCOMM:
		case D_COMM:
			printf(".%scomm	", ch == D_LCOMM ? "l" : "");
			sp = (void *)symget(tmprd());
			printf("%s,", sp->hhdr.name);
			prexpr(0);
			printf("\n");
			break;
		case DOTALIGN:
		case DOTSPACE:
			printf("	.%s ",
			    ch == DOTSPACE ? "space" : "align");
			printf("%d,", tmprd());
			printf("%d\n", tmprd());
			break;
		case DOTSET:
			sp = (void *)symget(tmprd());
			printf("	.set %s,", sp->hhdr.name);
			prexpr(0);
			printf("\n");
			break;
		default:
			if (ch >= SYMBASE) {
				sp = (void *)symget(ch);
				if (sp->flags & SYM_GLOBAL)
					printf(".globl\t%s\n", sp->hhdr.name);
			//	if (sp->hhdr.name[0] == '~' &&
			//	    sp->hhdr.name[1] == '~')
			//		printf("\n");
				printf("%s:", sp->hhdr.name);
				if (sp->hhdr.name[0] == '~' &&
				    sp->hhdr.name[1] == '~')
					printf("\n");
				else if (sp->hhdr.name[0] != 'L')
					printf("\n");
			} else if (ch >= INSNBASE)
				myinsn(ch);
			else
				printf("UNKNOWN CODE %d\n", ch);
		}
	}
#endif
}

#if 0
/*
 * Print an expression in a readable format.
 */
void
prexpr(int lvl)
{
	int ch;

	if ((ch = tmprd()) == NUMBER) {
		printf("%d", tmprd());
	} else if (ch == IDENT) {
		struct symbol *sp = (void *)symget(tmprd());
		printf("%s", sp->hhdr.name);
	} else {
		if (lvl)
			printf("(");
		if (ch != '~' && ch != UMINUS)
			prexpr(lvl+1);
		printf("%c", ch);
		prexpr(lvl+1);
		if (lvl)
			printf(")");
	}
}
#endif
