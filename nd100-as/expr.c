/*	$Id: expr.c,v 1.9 2022/11/12 16:23:44 ragge Exp $	*/
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

/*
 * Build a stack of expressions and evaluate directly if possible.
 */
#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "as.h"

#define	UMINUS	7	/* something that is not any other op */

#define	EXPBLK	20

static struct expr eblk[EXPBLK], *epole;

static int expcalc(int op, int l, int r);
static struct expr *addops(void);
static struct expr *bitops(void);
static struct expr *mdvops(void);
static struct expr *termops(void);
static void prexpr(struct expr *e, int n);

#define	ISLEAF(x) ((x) == NUMBER || (x) == IDENT || (x) == '.')
#define	ISUNOP(x) ((x) == UMINUS || (x) == '~')
#define	ISBIOP(x) (!ISUNOP(x) && !(ISLEAF(x)))

static int ctok;
#define	shft() ctok = tok_get()

void
exprinit()
{
	int i;

	/* setup pre-allocated structs */
	for (i = 0; i < EXPBLK; i++) {
		eblk[i].e_left = epole;
		epole = &eblk[i];
	}
}

static struct expr *
buildex(int t, struct expr *e1, struct expr *e2)
{
	struct expr *e;

	if (epole) {
		e = epole;
		epole = e->e_left;
	} else
		e = xcmalloc(sizeof(struct expr));

	e->op = t;
	e->e_left = e1;
	e->e_right = e2;
	return e;
}

static void
efree(struct expr *e)
{
	e->e_left = epole;
	epole = e;
}

/*
 * A pass1 routine.  Fetches an expression from the input stream and 
 * saves it on the temp file.
 */
struct expr *
p1_rdexpr(void)
{
	struct expr *e;

	shft();
	e = addops();
	tok_unget(ctok);
	if (edebug) {
		printf("p1_rdexpr\n");
		prexpr(e, 0);
	}
	return e;
}

struct expr *
addops(void)
{
	struct expr *e;
	int t;

	e = bitops();
	while ((t = ctok) == '+' || ctok == '-' ) {
		shft();
		e = buildex(t, e, bitops());
	}
	return e;
}

struct expr *
bitops(void)
{
	struct expr *e;
	int t;

	e = mdvops();
	while ((t = ctok) == '|' || ctok == '&' || ctok == '^' || ctok == '!') {
		shft();
		e = buildex(t, e, mdvops());
	}
	return e;
}

struct expr *
mdvops(void)
{
	struct expr *e;
	int t;

	e = termops();
	while ((t = ctok) == '*' || ctok == '/' || ctok == '%' ||
	    ctok == '<' || ctok == '>') {
		shft();
		e = buildex(t, e, termops());
	}
	return e;
}

struct expr *
termops(void)
{
	struct symbol *sp;
	struct expr *e = NULL;
	int n, t = ctok;

	switch (t) {
	case '-':
		t = UMINUS;
	case '~':
		shft();
		e = buildex(t, termops(), NULL);
		break;

	case '.':
	case NUMBER:
		n = yylval.val;
		shft();
		e = buildex(t, NULL, NULL);
		e->e_val = n;
		break;

	case IDENT:
		sp = (void *)yylval.hdr;
		shft();
		e = buildex(t, NULL, NULL);
		e->e_sym = sp;
		break;

	case '(': 
		e = p1_rdexpr();
		if (ctok == ')') {
			shft();	/* p1_rdexpr() pushes back last token */
			shft();
			break;
		}
		/* FALLTHROUGH */
	default:
		error("bad terminal '%s'", symname(t));
		e = buildex(NUMBER, NULL, NULL);
	}
	return e;
}

int
expcalc(int op, int l, int r)
{
	int n = 0;

	switch (op) {
	case '+': n = l + r; break;
	case '-': n = l - r; break;
	case '/': n = l / r; break;
	case '*': n = l * r; break;
	case '%': n = l % r; break;
	case '<': n = l << r; break;
	case '>': n = l << r; break;
	case '|': n = l | r; break;
	case '&': n = l & r; break;
	case '~': n = ~l; break;
	case UMINUS: n = -l; break;
	case '!': n = l | ~r; break;
	case '^': n = l ^ r; break;
	default: error("expcalc");
	}
	return n;
}
	
/*
 * Evaluate an expression tree.  Return in struct eval.
 * Parameter "e" is freed after evaluation.
 */
static void
evtree(struct expr *e, struct eval *ev)
{
	struct eval evl, evr, *e1, *er, *el;

	er = &evr;
	el = &evl;

	if (ISLEAF(e->op) == 0) {
		evtree(e->e_left, el);
		if (ISUNOP(e->op) == 0)
			evtree(e->e_right, er);
	}

	ev->type = ev->segn = 0;
	ev->val = 0;
	ev->sp = NULL;

	switch (e->op) {
	case '.':
		ev->val = cdot;
		ev->type = EVT_SEG;
		ev->segn = cursub->segnum;
		break;

	case NUMBER:
		ev->val = e->e_val;
		ev->type = EVT_ABS;
		break;

	case IDENT:
		ev->sp = e->e_sym;
		ev->type = EVT_UND;
		if (e->e_sym->flsdi & SYM_DEFINED) {
			if (e->e_sym->sub) {
				ev->type = EVT_SEG;
				ev->segn = e->e_sym->sub->segnum;
			} else {
				ev->sp = 0;
				ev->type = EVT_ABS;
			}
			ev->val = e->e_sym->val;
		}
		break;

	case '-':
		/*
		 * "If the first operand is a relocatable segment-type symbol,
		 *  the second operand may be absolute; or the second operand
		 *  may have the same type as the first."
		 */
		if (el->type == EVT_SEG && er->type == EVT_ABS) {
			/*
			 * "If the first operand is a relocatable segment-type
			 *  symbol, the second operand may be absolute;
			 *  in which case the result has the type of
			 *  the first operand"
			 */
			ev->type = EVT_SEG;
			ev->segn = el->segn;
			ev->val = el->val - er->val;
			ev->sp = el->sp;
		} else if (el->type == EVT_SEG && er->type == EVT_SEG &&
		    el->segn == er->segn) {
			/*
			 * "or the second operand may have the same type as
			 *  the first in which case the result is absolute"
			 */
			ev->type = EVT_ABS;
			ev->val = el->val - er->val;
		} else if (el->type == EVT_UND && er->type == EVT_ABS) {
			/*
			 * "If the first operand is external undefined,
			 *  the second must be absolute."
			 */
			ev->type = EVT_UND;
			ev->val = -er->val;
			ev->sp = el->sp;
		} else
			goto def; /* Must be absolute symbols */
		break;

	case '+':
		/*
		 * "If one operand is segment-type relocatable, or is an
		 *  undefined external, the result has the postulated
		 *  type and the other operand must be absolute."
		 */
		/* swap to get ABS ro the right */
		if (el->type == EVT_ABS)
			e1 = el, el = er, er = e1;

		if ((el->type == EVT_UND || el->type == EVT_SEG) && 
		    er->type == EVT_ABS) {
			ev->type = el->type;
			ev->segn = el->segn;
			ev->val = er->val;
			if (el->type == EVT_SEG)
				ev->val += el->val;
		} else
			goto def; /* Must be absolute symbols */
		break;

	default:
		/*
		 * Allow only absolute expressions here.
		 */
		if (ISUNOP(e->op))
			er->type = EVT_ABS;
def:		if (el->type != EVT_ABS || er->type != EVT_ABS)
			error("expression error");
		ev->val = expcalc(e->op, el->val, er->val);
		ev->type = EVT_ABS;
		break;
	}
	efree(e);
}

/*
 * Evaluate an expression tree while saving an (eventual) undefined IDENT.
 */
int
expres(struct eval *ev, struct expr *e)
{
	if (edebug) {
		printf("expres\n");
		prexpr(e, 0);
	}
	evtree(e, ev);
	return ev->type;
}

/*
 * Copy expression tree.
 */
struct expr *
expcopy(struct expr *e)
{
	struct expr *r, *l;

	switch (e->op) {
	case '.':
	case IDENT:
	case NUMBER:
		r = buildex(0, 0, 0);
		*r = *e;
		break;

	case UMINUS:
	case '~':
		r = expcopy(e->e_left);
		r = buildex(e->op, r, 0);
		break;

	default:
		l = expcopy(e->e_left);
		r = expcopy(e->e_right);
		r = buildex(e->op, l, r);
		break;
	}
	return r;
}

/*
 * Evaluate an expression tree and require the result to be an 
 * absolute constant.
 */
mdptr_t
absval(struct expr *e)
{
	struct eval ev;

	evtree(e, &ev);
	if (ev.sp)
		error("expr not absolute");
	return ev.val;
}

void
p1_wrexpr(struct expr *e)
{
	if (e->op == NUMBER) {
		tmpwri(NUMBER);
		tmpwri(e->e_val);
	} else if (e->op == IDENT) {
		tmpwri(IDENT);
		tmpwri(e->e_sym->hhdr.num);
	} else if (e->op == '.') {
		tmpwri(e->op);
	} else {
		tmpwri(e->op);
		p1_wrexpr(e->e_left);
		if (!ISUNOP(e->op))
			p1_wrexpr(e->e_right);
	}
	efree(e);
}

struct expr *
p2_rdexpr(void)
{
	struct expr *e;

	e = buildex(tmprd(), NULL, NULL);
	switch (e->op) {
	case '.':
		break;
	case NUMBER:
		e->e_val = tmprd();
		break;
	case IDENT:
		e->e_sym = (void *)symget(tmprd());
		break;
	default:
		e->e_left = p2_rdexpr();
		if (!ISUNOP(e->op))
			e->e_right = p2_rdexpr();
		break;
	}
	return e;
}

void
prexpr(struct expr *e, int n)
{
	int nn = n;

	while (n-- > 0)
		printf("  ");
	printf("%p) ", e);
	if (e->op == NUMBER) {
		printf("NUMBER %d\n", e->e_val);
		return;
	} else if (e->op == '.') {
		printf(". %d\n", cdot);
		return;
	} else if (e->op == IDENT) {
		printf("IDENT %s\n", e->e_sym->hname);
		return;
	} else if (e->op == UMINUS) {
		printf("U-");
	} else
		printf("%c", e->op);
	printf("\n");
	prexpr(e->e_left, nn+1);
	if (!ISUNOP(e->op))
		prexpr(e->e_right, nn+1);
}
