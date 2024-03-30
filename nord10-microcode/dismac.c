/*
 * Copyright (c) 2024 Anders Magnusson
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
 * Very small and simple disassembly program for Nord10 microcode.
 * Input is a file in hex format, output is micmac text.
 */

#include <err.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

void p(char *);
void parsear(int);

#define	BIT0(x)	(((x) >> 0) & 1)
#define	BIT1(x)	(((x) >> 1) & 1)
#define	B12(x)	(((x) >> 12) & 1)
#define	B15(x)	(((x) >> 15) & 1)
#define	B18(x)	(((x) >> 18) & 1)
#define	B19(x)	(((x) >> 19) & 1)
#define	B20(x)	(((x) >> 20) & 1)
#define	B28(x)	(((x) >> 28) & 1)
#define	B29(x)	(((x) >> 29) & 1)
#define	B30(x)	(((x) >> 30) & 1)

char *cond[] = { "ZERO ", "SPOS ", "POS ", "POSM ",
	"NZERO ", "SNEG ", "NEG ", "NEGM " };

char *cycle[] = { "", "CEATR ", "CPTR ", "CFC ", "CWR1 ", "CW ",
	"CRR1 ", "CR " };

char *alu[] = { "BM1 ", "PLUS ", "BMAM1 ", "BDI ", "?alu4 ", "PLUS ADDC ", 
	"BMAM1 ADDC ", "?alu7 ", "?alu8 ", "PLUS9 ", "ADIR ", "?alu11 ", "?alu12 ",
	"PLUS ADD1 ", "BMINA ", "BDI ADD1 ",
	"BDIRC ", "ANDC ", "ORCB ", "ONE ", "ORC ", "ADIRC ", "EXORC ", "ORCA ",
	"ANDCB ", "EXOR ", "ADIR ", "OR ", "ZERO ", "ANDCA ", "AND ", "BDIR " };

char *alu2[] = { "BM1A ", "PLUSA ", "BMAM1 ", "BDIA ", "?alu4 ", "PLUS ADDC ", 
	"BMAM1 ADDC ", "?alu7 ", "?alu8 ", "PLUS9 ", "ADIR ", "?alu11 ", "?alu12 ",
	"PLUS ADD1 ", "BMAA ", "BD1 ADD1 ",
	"BDIRC ", "ANDC ", "ORCB ", "ONE ", "ORC ", "ADIRC ", "EXORC ", "ORCA ",
	"ANDCB ", "EXOR ", "ADIR ", "OR ", "ZERO ", "ANDCA ", "AND ", "BDIR " };

char *orspecs[] = { "", "ORBWO ", "??", "ORBW ", "ORSKP ", "ORROP ",
	"ORSW2 ", "ORSW3 " };

char *orint[] = { "", "ORIN ", "??", "ORIN2 ", "??", "?? ", "?? ", "?? " };

char *aregs[] = { "", "D", "P", "B", "L", "A", "T", "X",
	"S", "DH", "IO", "H", "SCR", "R", "SP", "SS" };

char *bregs[] = { "Z", "D", "P", "B", "L", "A", "T", "X",
	"SC", "SH", "IO", "AC", "HAC", "2AC", "?16", "?17" };

char *dregs[] = { "", "D", "P", "B", "L", "A", "T", "X",
	"S", "SH", "IO", "SC", "SCR", "?15", "SP", "SS" };

char *trr[] = { "PAC", "S", "LMP", "PCR", "MIS", "IIE", "PID", "PIE",
	"?10", "?11", "?12", "CAR", "IR", "?15", "BIO", "IR3" };

char *tra[] = { "PAS", "S", "OPR", "PGS", "PVL", "IIC", "PID", "PIE",
	"?10", "DPIL", "ALD", "PES", "MPC", "PEA", "BIO", "BIR3" };

char *sht[] = { "", "SHRO ", "SHZE ", "??sht2 " };

char *tsc[] = { "TSC0 ", "??tsc ", "TSH31 ", "TSH22 " };

char *tground[] = { "", "TGSH0 ", "TGAC0 ", "TGS0 " };

/*
 * Disassemble microcode file.  Write on stdout.
 */
int
main(int argc, char *argv[])
{
	char buf[20];
	unsigned int l, n, lno;
	FILE *fp;

	if (argc != 2)
		errx(1, "usage: %s <micfile>", argv[0]);

	if ((fp = fopen(argv[1], "r")) == NULL)
		err(1, "fopen");
	lno = 0;
	while (fgets(buf, sizeof buf, fp) != NULL) {
		l = strtol(buf, NULL, 16);
		printf("%04o: %08X ", lno++, l);
		n = l >> 30;
		if ((n & 2) == 0) {
			int iscond = (n == 0 && B15(l) &&
			    (((l >> 4) & 017) != 012));
			int isint = n & 1;
			int ismost = l & 0x01000000;
			int isspeca = ((l >> 0) & 15) == 012;
			int isspecb = ((l >> 4) & 15) == 012;
			int isspecd = ((l >> 8) & 15) == 012;

			if (isint) p("I");
			else if (iscond) p("C");
			p(l & 0x20000000 ? "LOG" : "AR");
			p(ismost ? "M " : "L ");
			p(cycle[(l >> 21) & 7]);
			p(alu[(l >> 25) & 31]);
			if (B20(l)) p(isint ? "DSPL " : "CHLEV ");
			if (B19(l)) p("SACO ");
			if (isint) {
				if ((l >> 16) & 7) { // OR logic
					if (B15(l))
						p(orint[(l >> 16) & 7]);
					else
						p(orspecs[(l >> 16) & 7]);
				} else
					printf("LE%o ", (l >> 12) & 15);
			} else {
				int bm = (l >> 16) & 7;

				p(orspecs[bm]);
				if (bm == 1 || bm == 3) p("B,BM ");
			}
			if (iscond) p(cond[(l >> 12) & 7]);
			if (isspecd) { // case 1
				if (l & 15) printf("A,%s ", aregs[l & 15]);
				printf("B,%s ", trr[(l >> 4) & 15]);
				p("D,IO ");
			} else if (isspecb) { // case 2
				// bits
				if (l & 15) printf("A,%s ", aregs[l & 15]);
				if (((l >> 16) & 7) == 0)
					printf("B,B%o ", (l >> 12) & 15);
				if ((l >> 8) & 15)
					printf("D,%s ", dregs[(l >> 8) & 15]);
			} else if (isspeca) { // case 3
				// B op transferred to H reg
				p("A,IO ");
				printf("B,%s ", tra[(l >> 4) & 15]);
			} else {
				if (l & 15) printf("A,%s ", aregs[l & 15]);
		//		if ((l >> 4) & 15)
					printf("B,%s ", bregs[(l >> 4) & 15]);
				if ((l >> 8) & 15)
					printf("D,%s ", dregs[(l >> 8) & 15]);
			}
		} else if ((n & 1) == 0) { // JUMP
			if (B15(l)) p("C");
			p("JMP ");
			if (B29(l)) p(",CAR ");
			if (B28(l)) p("PRIV ");
			if (B15(l)) p(cond[(l >> 12) & 7]);
			if (l & 0xfff) printf("%04o ", l & 0xfff);
		} else { // LOOP
			p("LOOP ");
			p(alu[(l >> 25) & 31]);
			p(alu2[(l >> 20) & 31]);
			if (B19(l)) p("SACO ");
			if (B18(l)) p("ORSHT ");
			if (B15(l)) p("SHR ");
			p(sht[(l >> 13) & 3]);
			if (B12(l)) p("SH32 ");
			p(tground[(l >> 8) & 3]);
			switch ((l >> 4) & 15) {
			case 0: break;
			case 013: p("B,AC "); break;
			case 014: p("B,HAC "); break;
			case 015: p("B,2AC "); break;
			default: p("LOOPAC?? "); break;
			}
			p(tsc[(l >> 2) & 3]);
			if (BIT1(l)) p("ENDID ");
			if (BIT0(l)) p("ASH0 ");
		}
		p("\n");
	}
	fclose(fp);
}

void
p(char *s)
{
	printf("%s", s);
}

void
parsear(int l)
{
}
