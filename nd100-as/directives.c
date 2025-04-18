/*	$Id: directives.c,v 1.11 2022/11/12 16:23:44 ragge Exp $	*/
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
#include <stddef.h>

#include "as.h"

static void readidn(struct hshhdr **h, struct expr **ep);
static void wrvalue(struct direc *d);

#ifdef MD_WORD_ADDR
static void incdot(int type);
#else
#define	incdot(x)	cdot += x
#endif

/* classes of directives */
enum { DOTTDB, DOTBWLQ, DOTIDN, DOTGLOBL, DOTFILL, DOTEE,
	DOTALIGN, DOTASCII };

/* dir-specific defines */ 
#define IDN_COMM	0
#define IDN_LCOMM	1
#define IDN_SET		2
#define EE_SPACE	0
#define EE_ORG		1
#define EE_ALIGN	2
#define ASCIZ		1

struct direc direc[] = {
	{ HDRNAM(".text"), DOTTDB, SEG_TEXT },
	{ HDRNAM(".data"), DOTTDB, SEG_DATA },
	{ HDRNAM(".bss"),  DOTTDB, SEG_BSS },

	{ HDRNAM(".byte"), DOTBWLQ, DIR_BYTE },
	{ HDRNAM(".word"), DOTBWLQ, DIR_WORD },
	{ HDRNAM(".int"),  DOTBWLQ, DIR_INT },
	{ HDRNAM(".long"), DOTBWLQ, DIR_LONG },
	{ HDRNAM(".quad"), DOTBWLQ, DIR_QUAD },

	{ HDRNAM(".comm"), DOTIDN, IDN_COMM },
	{ HDRNAM(".lcomm"), DOTIDN, IDN_LCOMM },
	{ HDRNAM(".set"),  DOTIDN, IDN_SET },

	{ HDRNAM(".space"), DOTEE, EE_SPACE },
	{ HDRNAM(".org"), DOTEE, EE_ORG },
	{ HDRNAM(".align"), DOTEE, EE_ALIGN, },

	{ HDRNAM(".globl"), DOTGLOBL, 0 },
	{ HDRNAM(".fill"), DOTFILL, 0 },
	{ HDRNAM(".ascii"), DOTASCII, 0 },
	{ HDRNAM(".asciz"), DOTASCII, ASCIZ },
};
int ndirec = sizeof(direc) / sizeof(direc[0]);

/*
 * Just got a directive from the input stream.
 */
void
p1_direc(struct direc *d)
{
	struct symbol *sp;
	struct hshhdr *h, *h2;
	struct expr *e;
	int n, n2;

	h = &d->hhdr;
	switch (d->class) {
	case DOTBWLQ:
		do {
			tmpwri(h->num);
			e = p1_rdexpr();
			p1_wrexpr(e);
//printf("DOTBWLQ: dot %d\n", cdot);
			incdot(d->type);
//printf("DOTBWLQ2: dot %d\n", cdot);
		} while ((n = tok_get()) == ',');
		tok_unget(n);
		break;

	case DOTASCII:
		for (;;) {
			tok_acpt(STRING);
			dotascii(d->type, yylval.str);
			if (tok_peek() != ',')
				break;
			(void)tok_get();
		}
		break;

	case DOTTDB:
		n = 0;
		if (tok_peek() != DELIM)
			n = absval(p1_rdexpr());
		p1_setseg(h->name, n);
		break;

	case DOTIDN:
		/* .set/.comm/.lcomm */
		readidn(&h2, &e);
		tmpwri(h->num);
		tmpwri(h2->num);
		if (d->type == IDN_SET) {
			sp = (void *)h2;
			if (sp->sub != NULL)
				error("symbol redefined");
			sp->flsdi |= SYM_DEFINED;
			sp->val = absval(expcopy(e));
		}
		p1_wrexpr(e);
		break;

	case DOTEE:
		/* .org/.space */
		tmpwri(h->num);
		n = absval(p1_rdexpr());
		n2 = 0;
		if (tok_peek() == ',') {
			tok_acpt(',');
			n2 = absval(p1_rdexpr());
		}
		tmpwri(n);
		tmpwri(n2);
		if (d->type == EE_ORG) {
			cdot = n;
		} else if (d->type == EE_SPACE) {/* .space */
			cdot += n;
		} else if (d->type == EE_ALIGN) {
			cdot = ROUNDUP(cdot, n);
		}
		break;

	default:
		aerror("%s: unhandled directive", h->name);
	}
}

void
p2_direc(struct direc *d)
{
	void (*wrsz)(int);
	struct symbol *sp;
	int n, n2;

	wrsz = MD_NBPWD == 1 ? owbyte : ow2byte;
	switch (d->class) {
	case DOTBWLQ:
		wrvalue(d);
		break;

	case DOTEE:
		n = tmprd();
		n2 = tmprd();
		if (d->type == EE_SPACE) {
			while (n-- > 0)
				(*wrsz)(n2);
		} else if (d->type == EE_ORG) {
			if (n < cdot)
				error(".org goes backwards: %d < %d", n, cdot);
			while (n > cdot)
				(*wrsz)(n2);
		} else
			error("p2 DOTEE");
		break;

	case DOTIDN:
		if (d->class == IDN_SET) {
			sp = (void *)symget(tmprd());
			sp->val = absval(p2_rdexpr());
		} else
			error("p2_direc: DOTIDN");
		break;

	default:
		error("p2_direc: bad class %d", d->class);
	}
}

/*
 * pass2:
 * Write a given value, eventual contained together with a relocation.
 */
void
wrvalue(struct direc *d)
{
	struct eval ev;
	mdptr_t val;
	int t;

	t = expres(&ev, p2_rdexpr());

	/* let target add relocation (and relocation word) if needed */
	if (t != EVT_ABS)
		md_reloc(d, &ev);
	val = ev.val;

	/* Write value to output file */
	switch (d->type) {
	case 1:
		owbyte(val);
		break;

	case 2:
		ow2byte(val);
		break;

	case 4:
		ow4byte(val);
		break;

	default:
		aerror("wrvalue");
	}
}

/*
 * Read an identifier, a ',' and an expression.
 */
static void
readidn(struct hshhdr **h, struct expr **ep)
{
	int t;

	if ((t = tok_get()) == DIREC || t == INSTR) {
		yylval.hdr = symlookup(yylval.hdr->name, SYM_ID);
		t = IDENT;
	}
	*h = yylval.hdr;
	if (t != IDENT)
		error("syntax error");
	tok_acpt(',');
	*ep = p1_rdexpr();
}


#if 0
/*
 * Fill a block of "rep" count of "size" size with "fill" values.
 */
void
dotfill(int rep, int size, int fill)
{
	int o;

	/* sanity */
	for (o = 0; o < 5; o++)
		if (size == tysizes[o])
			break;
	if (o == 5)
		error(".fill bad size %d", fill);
	tmpwri(DOTFILL);
	tmpwri(rep);
	tmpwri(size);
	tmpwri(fill);
	cdot += size * rep;
}
#endif


static int byteno;

static void
wrb(int ch)
{
	tmpwri(byteno);
	tmpwri(NUMBER);
	tmpwri(ch);
	incdot(1);
}

void
dotascii(int t, char *s)
{
	if (byteno == 0)
		byteno = symlookup(".byte", SYM_DIR)->num;

	while (*s)
		wrb(esccon(&s));
	if (t == ASCIZ)
		wrb(0);
}

#ifdef MD_WORD_ADDR
#if MD_NBPWD != 2
#error fix incdot
#endif

void
incdot(int type)
{
	switch (type) {
	case 1: /* byte */
		if (cursub->odd)
			cdot++; 
		cursub->odd ^= 1;
		break;
	case 2: /* word */
		cdot++;
		break;
	case 4: /* long */
		cdot += 2;
		break;
	default:
		error("incdot %d", type);
	}	
}			
#endif
