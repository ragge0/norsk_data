/*	$Id: as.h,v 1.23 2022/11/13 14:09:33 ragge Exp $	*/
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
#include <stdarg.h>
#include <stdint.h>

/*
 * Header for entries stored in the hash table.
 * clnum will have slightly different meaning depending of entry:
 *	- In symbols it will contain the symbol number
 *	- For directives/instructions it contains the class
 *		(== what the arguments look like).
 */
struct hshhdr {
	struct hshhdr *next;
	char *name;	/* Symbol name */
	int num;	/* Symbol number */
};
#define	hname	hhdr.name
#define	hnum	hhdr.num
#define	HDRNAM(x)	{ NULL, x, 0 }

/* Default sizes (in bytes) of the initialize directives.
   May be changed by target */
#define	DIR_BYTE	1
#define	DIR_WORD	2
#define	DIR_INT		4
#define	DIR_LONG	4
#define	DIR_QUAD	8

#include "nd100-defs.h"
#include "nd100.h"

typedef uint16_t abi_relw;	/* XXX aout16 relocation word */

/*
 * Target binary byte layout definitions
 * - If endian is not given it is expected to be run-time changeable.
 * - If MD_WORD_ADDR is unset it is a byte-addressable target.
 * - If MD_WORD_ADDR is is set then an addressable unit is 2 or 4 bytes.
 *	Byte addressing in this case can be either from left or right in
 *	a word, specified by either MD_BYTE_LTOR or MD_BYTE_RTOL.
 *	If not specified it is expected to be run-time changeable.
 */
#ifdef MD_BIG_ENDIAN
#define	md_big_endian 1
#define	md_little_endian 0
#define	md_pdp_endian 0
#elif defined(MD_LITTLE_ENDIAN)
#define	md_big_endian 0
#define	md_little_endian 1
#define	md_pdp_endian 0
#elif defined(MD_PDP_ENDIAN)
#define	md_big_endian 0
#define	md_little_endian 0
#define	md_pdp_endian 1
#else
extern int md_big_endian, md_little_endian, md_pdp_endian;
#endif

#ifndef MD_NBPWD
#define	MD_NBPWD	1
#endif

#if MD_NBPWD > 1
#define	MD_WORD_ADDR
#if defined(MD_BYTE_LTOR)
#define	md_byte_ltor	1
#elif defined(MD_BYTE_RTOL)
#define	md_byte_ltor	0
#else
extern int md_byte_ltor;
#endif
#endif


/* table sizes */
#define	HASHSZ	255	/* hash entries for name */
#define	SYMCHSZ	128	/* symbol table alloc chunk size */
#define STRBKSZ 1024	/* String block size, allocated when needed */
#define	ASBUFSZ	1024	/* Common used buffers in the assembler */
#define	OBUFSZ	1024	/* Output buffer sizes */

#ifndef	DEFAULT_RADIX
#define	DEFAULT_RADIX	10
#endif
/*
 * Temp file, written to during parsing and read after relocations
 * to create the final object file. Only int-size values are stored
 * in the temp file.
 */

/* Base of different parts of symbol table in the temp file words */
/* 0-255 are the same as their ASCII counterpart */
/* 256-511 are definitions used in the scanner */
#define	DIRBASE		512	/* Directives, 512-1023 */
#define	INSNBASE	1024	/* Instructions, 1024-2047 */
#define	SYMBASE		2048	/* Symbols, 2048- */

/*
 * An absolute symbol will have the seg pointer set to NULL.
 */
struct symbol {
	struct hshhdr hhdr;
	mdptr_t val;		/* symbol address, or 0 */
	struct subseg *sub;	/* subseg this symbol belongs to, or NULL */
	int flsdi;		/* Flags & preceding SDIs */
#define	S_SETSDI(sp, s)	(sp->flsdi = ((s) << 2) | (sp->flsdi & 3))
#define	S_SDI(sp)	(sp->flsdi >> 2)
//	int flags;		/* symbol-specific flags */
//	int precsdi;		/* # of preceding SDIs */
};
/* flags */
#define	SYM_DEFINED	1
#define	SYM_GLOBAL	2

/*
 * Directive definitions.
 */
struct direc {
	struct hshhdr hhdr;
	int class;
	int type;
};
extern struct direc direc[];
extern int ndirec;
struct sdi_S;
struct mchunk;
#define	SDIBASE	1

#define	cdot	cursub->dot

/*
 * Maintains buffers internal to as.  Lets us allocate a dynamic 
 * amount of buffers.  Used in many places internally in the assembler.
 */
struct mchunk {
	int lpos, spos;
	int chsz;
	int nlinks;
	void **link;
};

/*
 * A segment consists of one or more subsegments.  Each subsegment is
 * handled as its own entity when it comes to local relocations; which
 * may not span multiple (sub)segments.
 * subsegs are identified by their number, given as the argument to the
 * directive when changing segments.
 */
struct subseg {
	struct subseg *next;
	int segnum;	/* parent segment number */

	/* subseg common info */
	int subnum;	/* number of subseg */
	int dot;	/* Current position in subseg */
	int size;	/* Total size of subseg */
	int suboff;	/* subseg offset into current seg */
	long foff;	/* output file offset */
#ifdef MD_WORD_ADDR
	int odd, oddsave;
#endif

	/* SDI pointers */
	struct mchunk *mch; /* storage for SDI structs */
	int nsdi;	/* count of SDIs */
	char *LONG;	/* long SDI instructions */
	int *INCR;	/* Increment for symbol table */

	/* Relocations stored in mchunks */
	struct mchunk *rmch;

#ifdef MD_SUBSEG	/* target-specific data */
	MD_SUBSEG
#endif
};

struct seg {
	struct seg *next;	/* link of segments */
	struct subseg *sub;	/* child subsegments link */
	char *name;		/* segment name */
	int segnum;		/* internal segment number */
	int size;		/* Total size of segment */
};

/* commonly used predefined segments, internal numbers */
#define	SEG_TEXT	0
#define	SEG_DATA	1
#define	SEG_BSS		2

#ifndef MD_SEGALIGN
#define MD_SEGALIGN	0
#endif

/* round x up to 2^y */
#define	ROUNDUP(x, y)	(((x)+((1 << (y))-1))& ~((1 << (y))-1))

/*
 * relocation data structure.
 */
struct reloc {
	struct symbol *sp;	/* Symbol whose value to be written */
	mdptr_t addr;		/* Position where this reloc is located */
	mdptr_t addto;		/* add-to relocated value */
	int rtype;		/* relocation type (MD) */
};

/*
 * Expression stack definition.
 */
struct expr {
	int op;
	union {
		mdptr_t val;
		struct symbol *sym;
		struct expr *left;
#define	e_left u.left
#define	e_val u.val
#define	e_sym u.sym
	} u;
	struct expr *e_right;
};

/*
 * Result of an expression evaluation:
 * - type is UND, ABS, SEG
 * - segn is index into segment array, valid only if type is SEG
 * - val is final value, also containing value from eventual symbol
 * - sp is given symbol if relevant.
 */

enum { EVT_UND = 1, EVT_ABS, EVT_SEG };
struct eval {
	char segn, type;
	mdptr_t val;		/* Symbol value */
	struct symbol *sp;	/* set if symbol */
};

/*
 * Parser definitions.
 */
extern union yylval {
	struct hshhdr *hdr;
	struct direc *dir;
	char *str;
	int val;
	unsigned int uval;
} yylval;

enum { LABEL = 257, IDENT, NUMBER, DELIM, STRING, INSTR, DIREC, LINENO,
	DOTSEGMENT, FILENM, DOTSYNC };

void relocate(void);
unsigned int esccon(char **s);
int acpt(int);
void parse(void);
#ifdef MD_WORD_ADDR
void dotsync(void);
#else
#define	dotsync()
#endif

/* scan.c */
int tok_get(void);
int tok_input(void);
int tok_peek(void);
void tok_unget(int);
void tok_unput(int);
void tok_acpt(int);

/* as.c */
extern FILE *ofd, *mapfd;
extern int tdebug, wdebug, sdebug, edebug, gdebug, odebug, uflag, mapflag;
extern int lineno, pass, infile;
void error(char *s, ...);
void aerror(char *s, ...);
void werror(char *s, ...);
struct mchunk *init_mchunk(int chsz);
void *new_mchunk(struct mchunk *mch);
#define	MKNELEM(mch) (mch->lpos*SYMCHSZ+mch->spos)
void *get_mchunk(struct mchunk *mch, int);
void *xmalloc(int);
void *xcmalloc(int);
void *xstrdup(char *str);
void lninc(int c);

/* symbol.c */
extern struct symbol **symary;
extern int nextsym;
extern struct hshhdr *hsh[];
void syminit(void);
struct hshhdr *symget(int v);
#define	SYMGET(v) &symary[v/SYMCHSZ][v%SYMCHSZ]
struct hshhdr *symlookup(char *s, int t);
#define	SYM_ANY	0
#define	SYM_ID	1
#define	SYM_DIR	2
char *strsave(char *);
char *symname(int);
void serror(char *, int);

/* segments.c */
extern int segcnt;
extern struct seg **segary;
extern struct subseg *cursub;
int p1_setseg(char *name, int subseg);
void p2_setseg(int segnum, int subseg);
#define	getseg(seg) segary[seg]
struct seg *createseg(char *name);
void seginit(void);

/* target code */
void p1_instr(struct insn *ir);
void p2_instr(struct insn *ir);
void wrval(struct direc *, int val, struct symbol *);
extern struct insn insn[];
extern int ninsn;
int relwrd(struct reloc *);
void md_reloc(struct direc *, struct eval *);

extern int tysizes[];
#define	TYSZ(x)	tysizes[x-D_1BYTE]
void myinsn(int);
int myprint(int *ch);
int toolong(struct hshhdr *h, int d);
int longdiff(struct hshhdr *h);
int mywrite(int *ch);
void mywinsn(int);
void my2byte(int, int);
void mach_init(void);
void mydirec(int);
void myoptions(char *str);
int md_gettok(int ch);

/* directives.c */
void p1_direc(struct direc *h);
void p2_direc(struct direc *h);

void dotfill(int rep, int size, int fill);
void dotorg(int fill);
void dotbwlq(int);
void dotascii(int, char *);

/* expr.c  */
struct expr *p1_rdexpr(void);
struct expr *p2_rdexpr(void);
mdptr_t absval(struct expr *);
int expeval(struct expr *);
void p1_wrexpr(struct expr *);
void exprinit(void);
struct expr *expcopy(struct expr *);
int expres(struct eval *ev, struct expr *e);

/* relocate.c */
void addreloc(struct symbol *, int addto, int rtype);
void reg_sdi(struct hshhdr *h, struct hshhdr *);

/* tempfile.c */
int tmprd(void);
int tmpeof(void);
void tmpinit(void);
void tmpwri(int c);	/* write an integer to the temp file */
void tmprew(void);	/* Restart the file pointer */
int tmprdi(void);	/* read next int from the temp file */

/* aoutsubr.c */
void aoutsetup(void);
void aoutwrite(void);
void aoutwrel(void);
void aoutwsym(void);

/* ofile.c */
void owrite(void);
void owbyte(int);
void ow2byte(int);
void ow4byte(long);
void owwsz(int n, int ch);

/* delay.c */
extern int delay_waiting, delay_callme;
void delay_save(int endc);
void delay_reload(void);
