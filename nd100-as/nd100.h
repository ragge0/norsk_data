/*	$Id: archdefs.h,v 1.3 2022/11/13 14:09:33 ragge Exp $	*/
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

/* Definitions specific to this target cpu */
#define	MD_NBPWD	2	/* # bytes per dot increment (usually 1) */
#define	MD_WORD_ADDR		/* Arch addresses per word */
#define	MD_MAXPSZ	2	/* # bytes per pointer, also the
				   largest address size */
#define	MD_AOUT16		/* 16-bit a.out format */

#undef DIR_INT
#define	DIR_INT		4

/*
 * Relocations are stored in a word matching the position in data or
 * text segment.
 *	bit 0 set if 8-bit relocation
 *	bit 1-3 is relocation type
 *	bit 4-15 is symbol number.
 */
#define REL_8		001	/* set if 8-bit address (instruction) */
#define REL_ZP		002	/* zero-page address */
#define REL_TEXT	004	/* text address */
#define REL_DATA	006	/* data address */
#define REL_BSS		010	/* bss address */
#define REL_ABS		012	/* absolute address */
#define REL_UNDEXT	014	/* undefined external, have symbol number */
#define REL_BPTR	016	/* byte pointer, address doubled */
#define	RELMSK		016

/* Instruction struct - must be declared here */
struct insn {
	struct hshhdr hhdr;
	int class;
	int opcode;
};


