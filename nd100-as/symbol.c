/*	$Id: symbol.c,v 1.8 2022/11/12 16:23:44 ragge Exp $	*/
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include "as.h"

/*
 * Symbols are stored sequentially in blocks of SYMCHSZ symbols.
 */
struct symbol **symary;
int maxsymp, nextsym;
static struct symbol *next_symbol(void);

/* string block, allocated when more needed */
static char sblk0[STRBKSZ];		/* have one block preallocated */
static char *strblk = sblk0;
static int sblkp;			/* next free position */

/* Symbol hash table */
struct hshhdr *hsh[HASHSZ];
static int hshcalc(char *s);

static void
symins(struct hshhdr *h, char *name, int num)
{
	int n = hshcalc(name);
	h->num = num;
	h->next = hsh[n];
	hsh[n] = h;
}

void
syminit(void)
{
	struct hshhdr *h;
	int i;

	for (i = 0; i < ndirec; i++) {
		h = &direc[i].hhdr;
		symins(h, h->name, DIRBASE+i);
	}

	for (i = 0; i < ninsn; i++) {
		h = &insn[i].hhdr;
		symins(h, h->name, INSNBASE+i);
	}
}

/*
 * Return a symbol based on the internal number.
 */
struct hshhdr *
symget(int v)
{
	if (v < DIRBASE || v >= nextsym+SYMBASE)
		return NULL;
	if (v < INSNBASE)
		return &direc[v-DIRBASE].hhdr;
	if (v < SYMBASE)
		return &insn[v-INSNBASE].hhdr;
	v -= SYMBASE;
	return &symary[v/SYMCHSZ][v%SYMCHSZ].hhdr;
}

static char *nmapper[] = {
	"LABEL", "IDENT", "NUMBER", "DELIM", "STRING", "INSTR", "DIREC",
	    "LINENO", "DOTSEGMENT", "FILENM", "DOTSYNC"
};

/*
 * Return a symbol name based on its internal number.
 */
char *
symname(int n)
{
	static char ss[15];
	struct hshhdr *h;
	char *c;

	c = NULL;
	if (n < 256) {
		ss[0] = n;
		c = ss;
	} else if (n < DIRBASE) {
		if (n > LABEL &&
		    n < LABEL + sizeof(nmapper)/sizeof(nmapper[0]))
			c = nmapper[n - LABEL];
	} else {
		if ((h = symget(n)) != NULL)
			c = h->name;
	}
	if (c == NULL)
		sprintf(ss, "%d", n);
	return c;
}

void
serror(char *u, int t)
{
	char *c, *n = symname(t);

	c = NULL;
	if (t < DIRBASE && t > 255) {
		switch (t) {
		case STRING:
			c = yylval.str;
			break;
		default:
			break;
		}
	}
	error("%s: '%s'%s%s", u, n, c ? ": " : "", c ? c : "");
}

/*
 * Search for a symbol.  If not found, create a new entry and return.
 */
struct hshhdr *
symlookup(char *s, int type)
{
	struct hshhdr *h;
	struct symbol *sp;

	if (sdebug > 1)
		printf("symlookup: %s,%d\n", s,type);
	for (h = hsh[hshcalc(s)]; h; h = h->next) {
		if (*h->name == *s && strcmp(h->name, s) == 0) {
			if (type == SYM_ANY ||
			    (type == SYM_ID && h->num >= SYMBASE) ||
			    (type == SYM_DIR && h->num >= DIRBASE &&
			     h->num < SYMBASE))
				return h;
		}
	}
	sp = next_symbol();
	sp->hname = s = strsave(s);
	symins(&sp->hhdr, s, nextsym++ + SYMBASE);
	if (sdebug)
		printf("Adding symbol %s with num %d\n", s, sp->hhdr.num);
	return &sp->hhdr;
}

static struct symbol *
next_symbol()
{
	int symp = nextsym/SYMCHSZ;
	int symn = nextsym%SYMCHSZ;

	if (symp == maxsymp) {
		struct symbol **osymary = symary;
		symary = xcmalloc((maxsymp += 10) * sizeof(struct symbol *));
		memcpy(symary, osymary, symp * sizeof(struct symbol *));
	}
	if (symary[symp] == NULL)
		symary[symp] = xmalloc(sizeof(struct symbol) * SYMCHSZ);
	return &symary[symp][symn];
}

static int
hshcalc(char *s)
{
	int j;

	for (j = 0; *s; s++)
		j += *s;
	return j % HASHSZ;
}

/*
 * Save (permanently) a string in the string block.
 */
char *
strsave(char *s)
{
	int l;

	if ((l = strlen(s)+1) + sblkp >= STRBKSZ)
		strblk = xmalloc(STRBKSZ), sblkp = 0;
	sblkp += l;
	return memcpy(&strblk[sblkp-l], s, l);
}
