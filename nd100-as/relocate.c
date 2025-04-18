/*	$Id: relocate.c,v 1.12 2022/11/12 16:23:44 ragge Exp $	*/
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

#include <stdlib.h>

#include "as.h"

static void sdi_reloc(struct subseg *);

static struct mchunk *relpole;

void
relocate(void)
{
	struct seg *seg;
	struct subseg *sub;
	struct symbol *sp;
	int i;

	/* init reloc data structure as well */
	relpole = init_mchunk(sizeof(struct reloc));

	/*
	 * Resolve SDIs (to get correct symtab offsets).
	 * This is per-sub-segment.
	 */
	for (i = 0; i < segcnt; i++) {
		if ((seg = getseg(i)) == NULL)
			continue;
		for (sub = seg->sub; sub; sub = sub->next) {
			dotsync();
			sdi_reloc(sub);
		}
	}

	/*
	 * Set the final offsets (after SDI calculations) of the (sub)segments.
	 * All segments are set starting at 0, and the subsegs starts after
	 * each other. The binary layout and offsets will then be set
	 * by the binary format routines.
	 *
	 * dot has the pre-SDI size, and after this routine it is reset to
	 * the subseg offset.
	 */
	for (i = 0; i < segcnt; i++) {
		if ((seg = getseg(i)) == NULL)
			continue;
		for (sub = seg->sub; sub; sub = sub->next) {
			/*
			 * set subseg size.  dot is set before SDI
			 * calculations, so add the increment as well.
			 */
			sub->size = ROUNDUP(sub->dot, MD_SEGALIGN);
			if (sub->nsdi)
				sub->size += sub->INCR[sub->nsdi-1];
			sub->dot = sub->suboff = seg->size;
			seg->size += sub->size; /* total size */
		}
	}

	/*
	 * Loop over all symbols and update their offsets due to SDI.
	 */
	for (i = 0; i < nextsym; i++) {
		sp = SYMGET(i);
		/* no update if absolute or no relocations */
		if (sp->sub == NULL || sp->sub->mch == NULL)
			continue;
		sp->val += sp->sub->INCR[S_SDI(sp)];
		if (gdebug)
		printf("symbol %s off %d precsdi %d\n",
		    sp->hhdr.name, (int)sp->val, S_SDI(sp));
	}
}

/*
 * Add a relocation at .
 * Allocate a new reloc entry.  Return its offset.
 */
void
addreloc(struct symbol *sp, int addto, int rtype)
{
	struct reloc *r;

	if (cursub->rmch == NULL)
		cursub->rmch = init_mchunk(sizeof(struct reloc));
	r = new_mchunk(cursub->rmch);

	r->sp = sp;
	r->addr = cdot;
	r->addto = addto;
	r->rtype = rtype;
}

struct sdi_S {
	struct hshhdr *h;	/* instruction */
	struct hshhdr *l;	/* label for this SDI */
	int a;			/* Minimum addr of this SDI */
};

/*
 * low/high is the span (including both) where SDIs affecting this
 * instruction is set.  I.e. "for (i = low; i <= high; i++)..."
 */
struct sdinode {
	int span;	/* calculated span of this instruction */
	int low;
	int high;
};

/*
 * register position of an SDI.
 */
void
reg_sdi(struct hshhdr *h, struct hshhdr *lbl)
{
	struct sdi_S *sdi;

	/* initial alloc */
	if (cursub->mch == NULL) {
		cursub->mch = init_mchunk(sizeof(struct sdi_S));
		(void)new_mchunk(cursub->mch); /* entry 0 unused */
	}
	sdi = new_mchunk(cursub->mch);
	sdi->h = h;
	sdi->a = cdot;
	sdi->l = lbl;
	cursub->nsdi++;
	if (gdebug)
		printf("reg_sdi: n %d . %d lbl %d\n",
		    cursub->nsdi, cdot, lbl->num);
}

/*
 * Process the graph by removing too large values.
 */
static int
loopover(struct subseg *sub, struct sdinode *sdinode)
{
	struct mchunk *mch = sub->mch;
	struct sdi_S *S;
	struct sdinode *sn, *sm;
	int i, j;
	int change = 0;

	for (i = 1; i <= sub->nsdi; i++) {
		sn = &sdinode[i];
		if (sub->LONG[i])	/* Already removed */
			continue;
		/* check if too long jump */
		S = get_mchunk(mch, i);
		if (!toolong(S->h, sn->span))
			continue;
		/* make long jump and remove */
		sub->LONG[i] = longdiff(S->h);
		change = 1;
		if (gdebug)
			printf("sdi_reloc_proc %d: span %d newdiff %d\n",
			    i, sn->span, sub->LONG[i]);
		/* search insn that are affected by me */
		for (j = 1; j <= sub->nsdi; j++) {
			sm = &sdinode[j];
			if (i < sm->low || i > sm->high)
				continue; /* not affected */
			if (j < i)
				sm->span += sub->LONG[i];
			else
				sm->span -= sub->LONG[i];
			if (gdebug)
				printf("sdi_reloc_par %d: adj %d span %d\n",
				    i, j, sm->span);
		}
	}
	return change;
}

/*
 * calculate the SDI offsets.
 */
void
sdi_reloc(struct subseg *sub)
{
	struct sdi_S *S;
	struct sdinode *sn, *sdinode;
	struct symbol *sp;
	int i;
	int numsdi = sub->nsdi+1;

	sub->LONG = xcmalloc(numsdi);
	sdinode = xcmalloc(sizeof(struct sdinode) * numsdi);

	/*
	 * Create the graph. Keep track of the sdi interval where the affected
	 * instructions are located.
	 */
	for (i = SDIBASE; i < numsdi; i++) {
		S = get_mchunk(sub->mch, i);
		sp = (struct symbol *)S->l;
		if ((sp->flsdi & SYM_DEFINED) == 0)
			error("%s not defined", sp->hhdr.name);
		sn = &sdinode[i];
		sn->span = sp->val - S->a;
		sn->low = sn->high = i;
		if (S_SDI(sp) < i)
			sn->low = S_SDI(sp), sn->high--;
		else
			sn->high = S_SDI(sp), sn->low++;
		if (gdebug)
			printf("sdi_reloc %d: span %d low %d high %d\n",
			    i, sn->span, sn->low, sn->high);
	}

	/*
	 * Calculate the SDI instructions.
	 */
	while (loopover(sub, sdinode))
		;
	free(sdinode);

	/*
	 * Calculate offset update for symbol table.
	 */
	sub->INCR = xcmalloc(sizeof(int) * numsdi);
	for (i = 1; i < numsdi; i++)
		sub->INCR[i] = sub->INCR[i-1] + sub->LONG[i];


	if (gdebug)
		for (i = SDIBASE; i < numsdi; i++)
			printf("sdi_final %d: %s, %d\n", i,
			    sub->LONG[i] ? "long" : "short", sdinode[i].span);
}
