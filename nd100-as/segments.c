/*	$Id: segments.c,v 1.10 2022/11/12 16:23:44 ragge Exp $	*/
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
#include <string.h>
#include <stdlib.h>
#include <err.h>

#include "as.h"

/*
 * Three segments are always present, and are therefore preallocated.
 *
 * - Text (seg 0)
 * - Data (seg 1)
 * - Bss (seg 2)
 *
 * Bss do not have data associated with it, hence no subseg.
 */


struct subseg *cursub;		/* current (sub)section in use */
int segcnt;			/* internal segment number */
struct seg **segary;
static int maxseg;

/*
 * Create a segment.
 */
struct seg *
createseg(char *name)
{
	struct seg *seg;
	int segnum = segcnt++;

	if (maxseg <= segnum)
		segary = xcmalloc(sizeof(struct seg *) * (maxseg += 10));

	if (segary[segnum])
		aerror("duplicate segment");
	
	/* create a new segment and subsegment */
	seg = segary[segnum] = xcmalloc(sizeof(struct seg));
	seg->name = xstrdup(name);
	seg->segnum = segnum;

	return seg;
}

static void
setsub(struct seg *seg, int subnum)
{
	struct subseg *sub;

	for (sub = seg->sub; sub; sub = sub->next)
		if (sub->subnum == subnum)
			break;

	if (sub == NULL) {
		/* allocate new subseg */
		sub = xcmalloc(sizeof(struct subseg));
		sub->subnum = subnum;
		sub->segnum = seg->segnum;
		sub->next = seg->sub;
		seg->sub = sub;
	}
	cursub = sub;
	tmpwri(DOTSEGMENT);
	tmpwri(seg->segnum);
	tmpwri(sub->subnum);
//printf("create: %s %d\n", seg->name, sub->dot);
}


/*
 * Change segment to segment "name", subseg "subseg".
 * If segment nonexistent, create one.
 */
int
p1_setseg(char *name, int subnum)
{
	struct seg *seg;
	int sn;

	for (sn = 0; sn < segcnt; sn++) {
		if ((seg = segary[sn]) == NULL)
			continue;
		if (strcmp(seg->name, name) == 0)
			break;
	}

	if (sn == segcnt)
		seg = createseg(name);
	setsub(seg, subnum);
	return seg->segnum;
}

/*
 * Set correct seg/subseg in pass2.
 */
void
p2_setseg(int segnum, int subnum)
{
	struct seg *seg;
	struct subseg *sub;

	if ((seg = getseg(segnum)) == NULL)
		aerror("undefined segment number");

	for (sub = seg->sub; sub; sub = sub->next)
		if (sub->subnum == subnum)
			break;
	if (sub == NULL)
		aerror("undefined subsegment");
	cursub = sub;

	/* set position in output file */
	fseek(ofd,
	    cursub->foff + (cursub->dot-cursub->suboff) * MD_NBPWD, SEEK_SET);
}

void
seginit()
{
	/* Create all default segments */
	createseg(".text");
	createseg(".data");
	createseg(".bss");
}
