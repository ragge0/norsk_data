/*	$Id: asarchsubr.c,v 1.17 2022/11/13 14:42:46 ragge Exp $	*/
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
#include <string.h>

#include "as.h"

enum { A_EA = 1, A_NOARG, A_ROP, A_ROPARG, A_ROPREG,
	A_SHFT, A_SHFTARG, A_SHFTR, A_FCONV, A_OFF,
	A_SKP, A_SKPARG, A_IDENT, A_IDARG, A_IOX,
	A_PMRW, A_MOVEW  };

#define OPC(x,y,z)	{ HDRNAM(x), y, z },
struct insn insn[] = {
#include "instr.h"
};
int ninsn = sizeof(insn) / sizeof(insn[0]);

static int
nextch()
{
	int c;

	while ((c = tok_input()) == ' ' || c == '\t')
		;
	return c;
}

static int
getcm(void)
{
	int n;

	if ((n = nextch()) == ',') {
		n = nextch();
		if (n == 'x')
			return 002000;
		if (n == 'b')
			return 000400;
		error("bad arg '%c'", n);
	} else
		tok_unput(n);
	return 0;
}

/*
 * Check for the possible options for memory reference instructions;
 *	- ,X
 *	- I
 *	- ,B
 * These must be in order.
 */
static int
eaopt(void)
{
	int rv, n;

	rv = getcm();
	if ((n = nextch()) == 'i')
		rv |= 001000;
	else
		tok_unput(n);
	rv |= getcm();
	return rv;
}

static int
roparg(void)
{
	struct insn *ar;
	int n, w = 0;

	while ((n = tok_get()) == INSTR) {
		ar = (void *)yylval.hdr;
		if (ar->class != A_ROPARG)
			break;
		w |= ar->opcode;
	}
	tok_unget(n);
	return w;
}
	
static int
ropreg(void)
{
	struct insn *ar;
	int n;

	if ((n = tok_get()) == INSTR) {
		ar = (void *)yylval.hdr;
		if (ar->class == A_ROPREG)
			return ar->opcode;
	}
	tok_unget(n);
	return 0;
}
	
static int
shftarg(void)
{
	struct insn *ar;
	int n, w = 0, negnum = 0;
	short val;

	if ((n = tok_get()) == INSTR) {
		ar = (void *)yylval.hdr;
		if (ar->class == A_SHFTARG) {
			w |= ar->opcode;
			n = tok_get();
		}
	}
	if (n == INSTR) {
		ar = (void *)yylval.hdr;
		if (ar->class == A_SHFTR) {
			negnum = 1;
			n = tok_get();
		}
	}
	tok_unget(n);

	val = absval(p1_rdexpr());
	if (val > 31 || val < -31)
		error("shift count %d", val);
	if (negnum)
		val = -val;
	w |= (val & 077);
	return w;
}

static int
skparg(void)
{
	struct insn *ar;
	int w;

	w = ropreg();
	if (tok_get() == INSTR) {
		ar = (void *)yylval.hdr;
		if (ar->class == A_SKPARG)
			w |= ar->opcode;
	} else
		error("skip: missing conditional");
	w |= ropreg();
	return w;
}

/*
 * Read and parse an instruction from the input stream.
 * Save it on the tempfile.
 */
void
p1_instr(struct insn *ir)
{
	struct insn *ar;
	struct expr *e;
	int w = 0;

	switch (ir->class) {
	case A_EA:
		w = eaopt();
		e = p1_rdexpr();
		break;

	case A_IDENT:
		if (tok_get() != INSTR)
badid:			error("bad ident level");
		ar = (void *)yylval.hdr;
		if (ar->class != A_IDARG)
			goto badid;
		w |= ar->opcode;
		break;
		
	case A_NOARG:
		break;

	case A_ROP:
		w = roparg();
		w |= ropreg();
		w |= ropreg();
		break;

	case A_SHFT:
		w = shftarg();
		break;

	case A_FCONV:
		w = absval(p1_rdexpr()) & 0377;
		break;

	case A_OFF:
		e = p1_rdexpr();
		break;

	case A_SKP:
		w = skparg();
		break;

	case A_IOX:
		w = absval(p1_rdexpr());
		if (w > 03777 || w < 0)
			error("device address out of bounds");
		break;

	case A_MOVEW:
	case A_PMRW:
		w = absval(p1_rdexpr());
		if (w > 7 || w < 0)
			error("argument out of bounds");
		if (ir->class == A_PMRW)
			w <<= 3; /* delta is in bit 3-5 */
		break;

	default:
		error("p1: bad instruction class %d", ir->class);
		w = 0;
	}
		
	if (ir->class != A_NOARG)
		tmpwri(w);
	if (ir->class == A_EA || ir->class == A_OFF)
		p1_wrexpr(e);
	cdot++;
}

/*
 *
 */
void
mach_init(void)
{
}

/*
 * Check if a relative jump is too long for a short instruction.
 * Expect all distances to be inside limits.
 */
int
toolong(struct hshhdr *h, int d)
{
	return 0;
}

/*
 * Get the size for a long-style jxx instruction.
 */
int
longdiff(struct hshhdr *h)
{
	return 1;
}

void
myinsn(int n)
{
}

int
myprint(int *ch)
{
	return 1;
}

int
mywrite(int *ch)
{
	return 0;
}

/*
 * Create the target-specific part of a relocation.
 * Call addreloc() to insert it in the relocation list.
 */
void
md_reloc(struct direc *d, struct eval *ev)
{
	if (d->type != 2)
		error("relocation not word");

	if (ev->type == EVT_UND) {
		addreloc(ev->sp, 0, REL_UNDEXT);
		if (uflag == 0)
			error("symbol '%s' not defined", ev->sp->hname);
	} else {
		if (ev->segn == SEG_TEXT) {
			addreloc(ev->sp, 0, REL_TEXT);
		} else if (ev->segn == SEG_DATA) {
			addreloc(ev->sp, 0, REL_DATA);
		} else if (ev->segn == SEG_BSS) {
			addreloc(ev->sp, 0, REL_BSS);
		} else
			error("bad relocate segment %d", ev->segn);
	}
}

/*
 * Write out an instruction to destination file.
 * The instructions should not have any unresolved symbols here.
 */
void
p2_instr(struct insn *in)
{
	struct eval evv, *ev;
	int val, instr, type;

	ev = &evv;
	instr = in->opcode;
	switch (in->class) {
	case A_OFF:
	case A_EA:
		instr |= tmprd();
		type = expres(ev, p2_rdexpr());
		switch (type) {
		case EVT_UND:
			if (uflag == 0)
				error("symbol %s undefined", ev->sp->hname);
			addreloc(ev->sp, 0, REL_UNDEXT);
			/* FALLTHROUGH */
		case EVT_ABS:
			/* can only be zero page */
			if (ev->val < 0 || ev->val > 0377)
				error("expr value out of bounds");
			val = ev->val;
			break;
		case EVT_SEG:
			/* segment defined */
			val = ev->val - cdot; /* now relative PC */
			if (val < -128 || val > 127)
				error("symbol distance too far");
			if (ev->segn == SEG_TEXT)
				addreloc(ev->sp, 0, REL_TEXT | REL_8);
			else if (ev->segn == SEG_DATA)
				addreloc(ev->sp, 0, REL_DATA | REL_8);
			else if (ev->segn == SEG_BSS)
				addreloc(ev->sp, 0, REL_BSS | REL_8);
			break;
		}
		instr |= (val & 0377);
		break;

	case A_NOARG:
		break;

	case A_MOVEW:
	case A_PMRW:
	case A_IOX:
	case A_IDENT:
	case A_FCONV:
	case A_SHFT:
	case A_ROP:
	case A_SKP:
		instr |= tmprd();
		break;

	

	default:
		aerror("p2: unknown class %d", in->class);
		instr = 0;
	}
	ow2byte(instr);
}

/*
 * Convert to a target-dependent relocate word.
 *
 * Bit 0 (if set) tells it's a PC-relative relocation.
 *
 * Bit 1-3 are segment for relocation, as below:
 * 000	  absolute number
 * 002	  reference to text	segment
 * 004	  reference to initialized data
 * 006	  reference to uninitialized data (bss)
 * 010	  reference to undefined external symbol
 *
 * Bit 4-15 are sequence number of referenced symbol.
 */
int
relwrd(struct reloc *r)
{
	int rv = r->rtype;

	if (r->sp && (rv & REL_UNDEXT))
		rv |= ((r->sp->hhdr.num - SYMBASE) << 4);
	return rv;
}

void
myoptions(char *str)
{
	error("bad -m option '%s'", str);
}
