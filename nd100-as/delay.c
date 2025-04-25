/*
 * Copyright (c) 2025 Anders Magnusson (ragge@ludd.ltu.se).
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
 * Save statements whose printout must be delayed until a specific place
 * in the final binary.
 * This routine is supposed to be used for targets that have limited 
 * relative access possibilities (like ARM).
 * This is handled in pass1 so that offset calculations will be correct.
 */

#include <stdio.h>
#include <err.h>
#include <string.h>
#include <stdlib.h>

#include "as.h"

#define SLIST_INIT(h)	\
	{ (h)->q_forw = NULL; (h)->q_last = &(h)->q_forw; }
#define SLIST_SETUP(h) { NULL, &(h)->q_forw }
#define SLIST_ENTRY(t)	struct { struct t *q_forw; }
#define SLIST_HEAD(n,t) struct n { struct t *q_forw, **q_last; }
#define SLIST_INSERT_LAST(h,e,f) {	\
	(e)->f.q_forw = NULL;		\
	*(h)->q_last = (e);		\
	(h)->q_last = &(e)->f.q_forw;	\
}
#define SLIST_FOREACH(v,h,f) \
	for ((v) = (h)->q_forw; (v) != NULL; (v) = (v)->f.q_forw)


int delay_waiting, delay_callme;

static int lblnum;
struct wp {
	struct wp *next;
	int wpnum;
	char wpstr[1];	/* will be allocated as needed */
};
struct wp *wpole, *wlast;

static void
pback(char *c)
{
	int i, n = strlen(c);
	
	for (i = n-1; i >= 0; i--)
		tok_unput(c[i]);
}

/*
 * Read from the input stream until endc is encountered.
 * A label for the new stuff will be put on the input stream.
 */
void
delay_save(int endc)
{
	struct wp *wp;
	char buf[ASBUFSZ+2];
	int i;

	/* fetch everything from input stream */
	for (i = 0; i < ASBUFSZ; i++) {
		if ((buf[i] = tok_input()) == endc)
			break;
	}
	if (i == ASBUFSZ)
		error("delayed string too long");
	buf[i++] = ' ';
	buf[i] = 0;

	wp = xmalloc(sizeof(struct wp) + strlen(buf));
	wp->wpnum = lblnum++;
	wp->next = NULL;
	strcpy(wp->wpstr, buf);

	if (wlast == NULL) {
		wpole = wlast = wp;
	} else {
		wlast->next = wp;
		wlast = wp;
	}

	sprintf(buf, "LA%d ", wp->wpnum);
	pback(buf);
	delay_waiting = 1;
}

/*
 * Push back the newly read data onto the input buffer.
 */
void
delay_reload(void)
{
	struct wp *wp, *owp;
	char buf[ASBUFSZ];

	pback(" ; ");
	for (wp = wpole; wp != NULL; ) {
		owp = wp;
		sprintf(buf, "LA%d: %s ; ", wp->wpnum, wp->wpstr);
		pback(buf);
		wp = wp->next;
		free(owp);
	}
	delay_callme = delay_waiting = 0;
}
