%{
/*	$Id: aslex.l,v 1.7 2022/11/12 16:23:44 ragge Exp $	*/
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

#include "as.h"

int yylex(void);
int yyparse(void);
void cvtdig(int radix);

int lineno;
%}

B			[0-1]
H			[a-fA-F0-9]
L			[a-zA-Z_]
D			[0-9]
MYID			"."?{L}({L}|{D})*

%%

[ \t]			{ ; }

{MYID}:			{	register char *ch = yytext;;
				ch[yyleng-1] = 0; /* remove trailing ':' */
				yylval.hdr = symlookup(ch, SYM_ID);
				return LABEL;
			}
{MYID}			{
				register int n;
				struct hshhdr *h;

				h = symlookup(yytext, SYM_ANY); 
				n =  h->num >= SYMBASE ? IDENT :
				    h->num >= INSNBASE ? INSTR : DIREC;
				yylval.hdr = h;
				return n;
			}

\"(\\.|[^\\"])*\"	{
				yylval.str = &yytext[1];
				yytext[strlen(yytext)-1] = 0;
				return(STRING);
			}

0[xX]{H}+		{ cvtdig(16); return(NUMBER); }
0{D}+			{ cvtdig(8); return(NUMBER); }
0[bB]{B}+		{ cvtdig(2); return(NUMBER); }
{D}+			{ cvtdig(DEFAULT_RADIX); return(NUMBER); }

'(\\.|[^\\'])'?		{	char *c = &yytext[1];
				yylval.val = esccon(&c);
				return(NUMBER);
			}

\n|";"			{ yylval.val = yytext[0]; return(DELIM); }

"<<"|">>"		{ return(yytext[0]); }

#.*$			{ ; }
\/\/.*$			{ ; }

.			{ return yytext[0]; }

%%

int
yywrap(void)
{
	return 1;
}

void
cvtdig(int radix)
{
	char *ch = yytext;
	int n, v;

	if (radix == 16 || radix == 2)
		ch += 2; /* Skip 0x or 0b */
	for (v = 0; *ch; ch++) {
		n = (*ch | 0x20) - '0';	/* upper case and sub 0 */
		if (n > 9)
			n = n - ('a' - '0') + 10;
		v = v * radix + n;
	}
	yylval.val = v;
}
		
unsigned int
esccon(char **sptr)
{
	unsigned int val;
	char *wr = *sptr;
	char c;

	if (*wr++ != '\\') {
		val = wr[-1];
		goto ret;
	}
	switch (c = *wr++) {
	case 'a': val = '\a'; break;
	case 'b': val = '\b'; break;
	case 'f': val = '\f'; break;
	case 'n': val = '\n'; break;
	case 'r': val = '\r'; break;
	case 't': val = '\t'; break;
	case 'v': val = '\v'; break;

	case '\"': val = '\"'; break;
	case '\'': val = '\''; break;
	case '\?': val = '\?'; break;
	case '\\': val = '\\'; break;

	case 'x':
		val = 0;
		for (;;) {
			c = *wr;
			if (c >= '0' && c <= '9')
				c = c - '0';
			else if (c >= 'a' && c <= 'f')
				c = c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				c = c - 'A' + 10;
			else
				break;

			val = (val << 4) + c;
			wr++;
		}
		break;

	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
		val = (c - '0');
		c = *wr;
		if (c >= '0' && c <= '7') {
			wr++;
			val = (val << 3) + (c - '0');
			c = *wr;
			if (c >= '0' && c <= '7') {
				wr++;
				val = (val << 3) + (c - '0');
			}
		}
		break;

	default:
		error("unknown escape sequence \\%c", c);
		val = c;
		break;
	}
ret:	*sptr = wr;
	return val;
}

static int savtok;

union yylval yylval;

void
tok_unget(int t)
{
	savtok = t;
}

/*
 * tok_get() returns either a number that either identifies the token that
 * is just read, or returns the following character.
 * if it is a token (of some sort) then the union yylval is filled in
 * with the values for this token.
 */
int
tok_get()
{
	int n;

	if (savtok) {
		n = savtok;
		savtok = 0;
		return n;
	}
	return yylex();
}

void
tok_acpt(int t)
{
	int u = tok_get();

	if (t == u)
		return;
	error("syntax error");
}

int
tok_input(void)
{
	return input();
}

void
tok_unput(int t)
{
	unput(t);
}

int
tok_peek(void)
{
	int u;

	if (savtok)
		return savtok;

	tok_unget(u = tok_get());
	return u;
}

