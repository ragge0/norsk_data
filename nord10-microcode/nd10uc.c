/*
 * Copyright (c) 2024 Anders Magnusson.
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
 * Trivial implementation of a Nord-10 microcode emulator.
 * Ugly code, in most cases a hack.  Expect 32-bit int.
 * No real support for correct interrupts or interrupt levels.
 * Only written to be able to run INSTRUCTION-B.
 *
 * Reads prom.hex for the 1k microcode.
 * Flags:
 *	-4		reads prom4k.hex instead (commercial microcode)
 *	-t		trace the microcode
 *	-h <file> 	attach a punched tape to device 400
 *	-d <file>	write instruction code execution trace to file
 *	-i <file>	Read microcode commands from file first.
 */


#include <err.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define	M_LALT(x)	(((x)->line >> 0) & 1)
#define	M_ENDID(x)	(((x)->line >> 1) & 1)
#define	M_TERM(x)	(((x)->line >> 2) & 3)
#define	M_LB(x)		(((x)->line >> 4) & 15)
#define	M_TG(x)		(((x)->line >> 8) & 3)
#define	M_LM(x)		(((x)->line >> 10) & 1)
#define	M_LSH32(x)	(((x)->line >> 12) & 1)
#define	M_LSHT(x)	(((x)->line >> 13) & 3)
#define	M_LSHR(x)	(((x)->line >> 15) & 1)
#define	M_LORSHT(x)	(((x)->line >> 18) & 1)
#define	M_LSSAVE(x)	(((x)->line >> 19) & 1)
#define	M_LALUL(x)	(((x)->line >> 20) & 31)
#define	M_LALUM(x)	(((x)->line >> 25) & 31)
#define	M_JCOND(x)	(((x)->line >> 15) & 1)
#define	M_JTC(x)	(((x)->line >> 12) & 7)
#define	M_ADDR(x)	(((x)->line >> 0) & 0xfff)
#define	M_PRIV(x)	(((x)->line >> 28) & 1)
#define	M_CAR(x)	(((x)->line >> 29) & 1)
#define	M_LEVEL(x)	(((x)->line >> 12) & 15)
#define	M_LEVEL_W(x,v)	((x)->line = (((x)->line & ~0xf000) | ((v) << 12)))
#define	M_DIRECT(x)	(((x)->line >> 20) & 1)
#define	M_OP(x)		(((x)->line >> 30) & 3)
#define	M_ALU(x)	(((x)->line >> 25) & 31)
#define	M_ARSEL(x)	(((x)->line >> 24) & 1)
#define	M_CYCLE(x)	(((x)->line >> 21) & 7)
#define	M_CHLEV(x)	(((x)->line >> 20) & 1)
#define	M_SSAVE(x)	(((x)->line >> 19) & 1)
#define	M_ORSPECS(x)	(((x)->line >> 16) & 7)
#define	M_COND(x)	(((x)->line >> 15) & 1)
#define	M_TC(x)		(((x)->line >> 12) & 7)
#define	M_TC_W(x,v)	((x)->line = (((x)->line & ~0x7000) | ((v) << 12)))
#define	M_DEST(x)	(((x)->line >> 8) & 15)
#define	M_DEST_W(x,v)	((x)->line = (((x)->line & ~0xf00) | ((v) << 8)))
#define	M_B(x)		(((x)->line >> 4) & 15)
#define	M_B_W(x,v)	((x)->line = (((x)->line & ~0xf0) | ((v) << 4)))
#define	M_A(x)		(((x)->line >> 0) & 15)
#define	M_A_W(x,v)	((x)->line = (((x)->line & ~0xf) | ((v) << 0)))

union ucent {
	int line;
} rom[4096];

volatile int tflag;
FILE *dfp;

int epg(int, int);
void arith(union ucent *), jump(union ucent *), iblock(union ucent *),
	loop(union ucent *);;
void ioexec(union ucent *), ident(union ucent *);

int mpc, wrtout;

typedef unsigned short Reg;
typedef Reg Rblk[16];
typedef unsigned long long ull;

Rblk A, D, T, X, B, SP, L, S1, S2;
Reg CP, SH[2], Alatch[2];
unsigned char STS[16];

// There is an AC for each arithmetic module.
// It clocks in the latest output from the 74181 ALU.
// But, only for the M or L output unless it is a loop
// microcode instruction, where both are clocked.
// See 1120 ACKL/BCKL and 1101 74174.
Reg AC[2];	// 0 == least, 1 == most.

Reg H, R, CAR, IR, PCR;	// 05, 13, 15
int bZ, bO, bS, bC, bCl;

Reg ioreg;
Reg oldCP;
int pil, pid, pie, pvl, iic, iid, iie, SC;
int pgon, inton;

int rtc_doint, rtc_rft, rtc_ctr;
void rtc_int();

unsigned short mem[65536];

volatile int ttistat = 001;
int tti_active;
int sfd;
char *hname;
FILE *ifd, *tfp;
int promsz = 1024;

/* internal interrupt enable register */
#define IIE_MC		0000002 /* Monitor call */
#define IIE_PV		0000004 /* Protect Violation */
#define IIE_PF		0000010 /* Page fault */
#define IIE_II		0000020 /* Illegal instruction */
#define IIE_V		0000040 /* Error indicator */
#define IIE_PI		0000100 /* Privileged instruction */
#define IIE_IOX		0000200 /* IOX error */
#define IIE_PTY		0000400 /* Memory parity error */
#define IIE_MOR		0001000 /* Memory out of range */
#define IIE_POW		0002000 /* Power fail interrupt */


#define SEXT8(x)	((x & 0377) > 127 ? (int)(x & 0377) - 256 : (x & 0377))
#define BIT0(x)		(((x) >> 0) & 1)
#define BIT1(x)		(((x) >> 1) & 1)
#define BIT6(x)		(((x) >> 6) & 1)
#define BIT8(x)		(((x) >> 8) & 1)
#define BIT9(x)		(((x) >> 9) & 1)
#define BIT10(x)	(((x) >> 10) & 1)
#define BIT13(x)	(((x) >> 13) & 1)
#define BIT15(x)	(((x) >> 15) & 1)

#define STS_TG		0000002
#define STS_Q		0000020
#define STS_O		0000040
#define STS_C		0000100
#define STS_M		0000200

static void
sig_io(int signo)
{
	ttistat |= 010;
}


int
main(int argc, char *argv[])
{
	struct termios p, op;
	union ucent *uc;
	FILE *fp;
	char hbuf[10];
	char *prom = "prom.hex";
	int i, ch;

	while ((ch = getopt(argc, argv, "4t:d:h:i:")) != -1) {
		switch (ch) {
		case '4':
			prom = "prom4k.hex";
			promsz = 4096;
			break;

		case 't':
			if ((tfp = fopen(optarg, "w")) == NULL)
				err(1, "fopen t");
	//		tflag = 1;
			break;

		case 'd':
			if ((dfp = fopen(optarg, "w")) == NULL)
				err(1, "fopen d");
			break;

		case 'h': hname = optarg; break;

		case 'i':
			if ((ifd = fopen(optarg, "r")) == NULL)
				err(1, "fopen");
			break;

		default:
			errx(1, "usage: %s [-t] [-h tapename ]", argv[0]);
		}

	}

	if ((fp = fopen(prom, "r")) == NULL)
		err(1, "fopen");
	for (i = 0; i < promsz; i++) {
		if (fgets(hbuf, 10, fp) == NULL)
			err(1, "fgets");
		rom[i].line = strtol(hbuf, 0, 16);
	}
	fclose(fp);

	signal(SIGIO, sig_io);
	if (fcntl(sfd, F_SETFL, O_NONBLOCK|O_ASYNC) < 0)
		err(1, "fcntl");
	tcgetattr(0, &op);
	tcgetattr(0, &p);
	cfmakeraw(&p);
	p.c_lflag |= ISIG;
	tcsetattr(0, TCSANOW, &p);
	ttistat = 0;
	mpc = 1;

	for (;;) {

		uc = &rom[mpc];
		if (tflag)
			fprintf(tfp, "%04o: %08X", mpc, uc->line);
		switch (M_OP(uc)) {
		case 0:
			arith(uc);
			break;

		case 1: // interblock
			iblock(uc);
			break;

		case 2:
			jump(uc);
			if (tflag)
				fprintf(tfp, "\n");
			continue;

		case 3: // LOOP
			loop(uc);
			break;

		default:
			errx(1, "op not implemented: %d line %o", M_OP(uc), mpc);
		}
		mpc++;
		if (tflag)
			fprintf(tfp, "\n");
	}

	return 0;
}

int
pk_calc()
{
	int i, d = pid & pie;

	for (i = 15; i >= 0; i--)
		if (d & (1 << i))
			return i;
	return 0;
}


/*
 * Post an internal interrupt for the given source.
 */
void
int14(int intr)
{
int xintr = intr;
	iid |= intr;	/* set detect flipflop */
	for (iic = 0; (intr & 1) == 0; iic++, intr >>= 1)
		;
	if (iid & iie) /* if internal int enabled, post priority int */
		pid |= (1 << 14);
}


static void
dprint()
{
//	if (wrtout == 0)
//		return;
	fprintf(dfp, "%06o: IR=%06o STS=%06o D=%06o B=%06o "
	    "L=%06o A=%06o T=%06o X=%06o\n",
	    oldCP, IR, STS[pil] + (pil << 8) + (inton << 15),
	    D[pil], B[pil], L[pil], A[pil], T[pil], X[pil]);
	fprintf(dfp, "N: %d\n", rtc_ctr);
	fflush(dfp);
}


void
ormap(union ucent *uc, union ucent *ucb)
{
	switch (M_ORSPECS(uc)) {
	case 0: return;
	case 1: // ORBWO
	case 3: // ORBW
		M_LEVEL_W(ucb, (CAR >> 3) & 017);
		if (M_OP(uc) == 1) { // interblock
			if ((CAR & 7) == 2)
				ucb->line = (ucb->line & ~15) | 016;
			else if (CAR & 7)
				ucb->line = (ucb->line & ~15) | (CAR & 7);
			else
				ucb->line = (ucb->line & ~15) | 010;
		} else { // arith
			if (CAR & 7)
				ucb->line = (ucb->line & ~15) | (CAR & 7);
			else
				ucb->line = (ucb->line & ~15) | 010;
		}
		if (M_ORSPECS(uc) == 3) {
			M_DEST_W(ucb, M_A(ucb));
			if (M_OP(uc) == 1)
				ucb->line = (ucb->line & ~15) | M_A(uc);
		}
		break;

	case 04: // ORSKP ARM
		if (M_COND(uc) == 0) {
			// set A and B
			M_A_W(ucb, (IR >> 3) & 7);
			M_B_W(ucb, (IR & 7) | (M_B(ucb) & 8));
		}
		M_TC_W(ucb, (IR >> 8) & 7);
		break;

	case 05: // ORROP
		M_A_W(ucb, (CAR >> 3) & 7);
		M_DEST_W(ucb, (CAR & 7));
		M_B_W(ucb, M_B(uc) & 010);
		if (BIT6(CAR) == 0)
			M_B_W(ucb, M_B(ucb) | (CAR & 7));
		break;

	case 06: // ORSW2
		if (M_COND(uc) == 0) {
			M_A_W(ucb, (CAR >> 3) & 7);
			M_B_W(ucb,  (M_B(uc) & 010) | (CAR & 7));
		}
		M_DEST_W(ucb, (CAR & 7));
		break;

	case 07: // ORSW3
		M_A_W(ucb, (CAR >> 3) & 7);
		M_B_W(ucb, (M_B(uc) & 010) | (CAR & 7));
		M_DEST_W(ucb, M_A(ucb));
		break;

	default:
		printf("\n");
		errx(1, "ormap 0%o not implemented: %08X line %o", M_ORSPECS(uc), uc->line, mpc);
	}
}

#define AMISS(x) if (uc->x) { printf("\n");	\
	errx(1, "arith " #x " 0%o not implemented: %08X line %o", uc->x, uc->line, mpc); }

char *anames[] = { "ZZ", " D", " P", " B", " L", " A", " T", " X",
	" S", "DH", "XX", " H", "SR", " R", "SP", "SS" };

static int
areg(union ucent *uc, int regno, int lvl)
{
	int rv;

	switch (regno) {
	case 000: rv = 0; break;
	case 001: rv = D[lvl]; break;
	case 002: rv = CP; break;
	case 003: rv = B[lvl]; break;
	case 004: rv = L[lvl]; break;
	case 005: rv = A[lvl]; break;
	case 006: rv = T[lvl]; break;
	case 007: rv = X[lvl]; break;

	case 010: rv = STS[lvl]; break;
	case 011: rv = SEXT8(H); break;
	case 012: rv = 0; break;	// unused
	case 013: rv = H; break;
	case 014: rv = S1[lvl]; break;
	case 015: rv = R; break;
	case 016: rv = SP[lvl]; break;	// SP
	case 017: rv = S2[lvl]; break;	// Scratch II
	}
	if (tflag)
		fprintf(tfp, " %s%02o(A)=%06o", anames[regno], lvl, rv);

	Alatch[M_ARSEL(uc)] = rv;
	return rv;
}

static int
breg(union ucent *uc, int lvl)
{
	int rv;

	switch (M_B(uc)) {
	case 000: rv = 0; break;
	case 001: rv = D[lvl]; break;
	case 002: rv = CP; break;		// Current P
	case 003: rv = B[lvl]; break;
	case 004: rv = L[lvl]; break;
	case 005: rv = A[lvl]; break;
	case 006: rv = T[lvl]; break;
	case 007: rv = X[lvl]; break;

	case 011: rv = SH[M_ARSEL(uc)]; break;		// Shift reg
	case 012:	// special case 2
		rv = (1 << ((uc->line >> 12) & 017));
		break;
	case 013: rv = AC[M_ARSEL(uc)]; break;			// AC
	case 014:
		rv = AC[M_ARSEL(uc)] >> 1;
		if (M_ARSEL(uc) == 0 && (AC[1] & 1))
			rv |= 0100000;	// ACs connected in HAC
		else if (M_ARSEL(uc))
			rv |= (bC << 15);
		break;						// 1/2 AC
	case 015:
		rv = AC[M_ARSEL(uc)] << 1;
		if (M_ARSEL(uc))		// ACs connected in 2AC
			rv |= (AC[0] >> 15);
		break;						// 2*AC

	default:
		if (M_B(uc)) { printf("\n");	\
		errx(1, "arith b 0%o not implemented: %08X line %o", M_B(uc), uc->line, mpc); }
	}
	if (tflag)
		fprintf(tfp, " B=%06o", rv);
	return rv; // XXX
}

/* transfer to H reg, special 3 */
static void
tra(union ucent *uc)
{
	switch (M_B(uc)) {
	case 001: H = STS[pil]; break;	// status reg
	case 003: H = 0; break;		// pgs
	case 004: H = pvl;		// PVL
	case 005:
		H = iic;		// iic
		iic = 0;
		break;

	case 006: H = pid; break;	// pid
	case 007: H = pie; break;	// pie
	case 010: H = 0; break;		// cache status (0 == no cache)
	case 011: H = (1 << pil); break;// dpil XXX
	case 012: H = 0300; break;	// ALD
	case 013: H = 0; break;		// pes
	case 014: H = mpc+1; break;	// MPC to H
	case 015: H = 0; break;		// pea

	case 016: H = ioreg; break;	// IO to H

	default:
		if (M_B(uc)) { printf("\n");	\
		errx(1, "arith b 0%o not implemented: %08X line %o", M_B(uc), uc->line, mpc); }
	}
}

static void
trr(union ucent *uc, int aval)
{
	switch (M_B(uc)) {
	case 000: /* printf(" PAC=%06o", aval); */ break;
	case 001: STS[pil] = aval; break;
	case 002: /* printf(" LMP=%06o", aval); */ break;
	case 003: PCR = aval; break;
	case 004:
		// Setting of bit 0-3 flips paging/interrupt (RS latch)
		if (aval & 04) pgon = 0;
		else if (aval & 010) pgon = 1;
		if (aval & 01) inton = 0;
		else if (aval & 02) inton = 1;
		if (aval & 020) {
		// Setting bit 4 sets MCALL D-ff (1058 1D), which in turn sets 
		//  interrupt request ff (1058 13D)
		// Clearing bit 4 clears the MCALL ff, but the interrupt request remains.
		// Slightly different logic ic used here
			int14(IIE_MC);
		}
		break;

	case 005: iie = aval; break;

	case 006: pid = aval; break;
	case 007: pie = aval; break;

	case 013: CAR = aval; break;
	case 014:
		H = CAR = IR = aval;
		if (dfp)
			dprint();
		mpc = epg(IR, 0)-1; // writing to IR resets MPC
		break;

	case 015: break; // XXX unused???

	case 016:
		ioreg = aval;
		if ((CAR & 0174000) == 0164000)
			ioexec(uc);
		else if ((CAR & 0177700) == 0143600)
			ident(uc);
		else printf("ioreg! IR %06o\r\n", IR);
		break;

	default:
		printf("\n");
		errx(1, "trr 0%o not implemented: %08X line %o", M_B(uc), uc->line, mpc);
	}
}

char *dnames[] = { "ZZ", " D", " P", " B", " L", " A", " T", " X",
	" S", "SH", "XX", "SC", "SR", "YY", "SP", "SS" };

static void
setdreg(union ucent *uc, int dval, int lvl)
{
	if (tflag)
		fprintf(tfp, ": D=%06o in %s%02o", dval, dnames[M_DEST(uc)], lvl);
	switch (M_DEST(uc)) {
	case 001: D[lvl] = dval; break;		// D
	case 002: CP = dval; break;		// Current P
	case 003: B[lvl] = dval; break;		// B
	case 004: L[lvl] = dval; break;		// L
	case 005: A[lvl] = dval; break;		// A
	case 006: T[lvl] = dval; break;		// T
	case 007: X[lvl] = dval; break;		// X
	case 010: STS[lvl] = dval; break;	// Status
	case 011: SH[M_ARSEL(uc)] = dval; break;		// Shift reg
	case 013:				// Shift counter
		SC = dval & 077;
		if (SC > 037) SC |= (0xffffffff << 6);
		break;
	case 014: S1[lvl] = dval; break;	// Scratch I
	case 016: SP[lvl] = dval; break;	// Saved P
	case 017: S2[lvl] = dval; break;	// Scratch II

	default:
		if (M_DEST(uc)) { printf("\n");	\
		errx(1, "arith dest 0%o not implemented: %08X line %o", M_DEST(uc), uc->line, mpc); }
	}
}

int
ckcond(union ucent *uc)
{
	int n;

	if (M_JCOND(uc) == 0)
		return 1; // no test condition
	switch (M_JTC(uc) & 03) {
	case 0: /* eql */ n = bZ; break;
	case 1: /* geq */ n = bS == 0; break;
	case 2: /* gre */ n = (bS ^ bO) == 0; break;
	case 3: /* mgre */ n = bC; break;
	}
	if (M_JTC(uc) & 04)
		n = !n;
	return n;
}

int
alu(union ucent *uc, Reg aval, Reg bval, int most)
{
	int c_o, dval;
	int c = (STS[pil] & STS_C) != 0;
	Reg negA = ~aval;

	if (most && M_OP(uc) == 3)
		c = bCl; // combined

	switch (M_ALU(uc)) {
	case 000: dval = bval - 1; break;		// BM1
	case 001: dval = aval + bval; break;		// PLUS XXX
	case 002: dval = bval + negA; break;		// BMAM1 (B-A-1)
	case 003: dval = bval; break;			// BDI

	case 005: dval = bval + aval + c; break;	// PLUS ADDC
	case 006: dval = bval + negA + c; break;	// BMAM1 ADDC (B-A-1)+C

	case 011: dval = aval + bval; break;		// PLUS

	case 015: dval = aval + bval + 1; break;	// PLUS + 1
	case 016: dval = bval + negA + 1; break;	// BMINA (B-A)
	case 017: dval = bval + 1; break;		// BDI ADD1
	case 020: dval = ~bval; break;			// BDIRC

	case 022: dval = aval | ~bval; break;		// ORCB

	case 024: dval = ~(aval|bval); break;		// ORC
	case 025: dval = ~aval; break;			// ADIRC

	case 030: dval = aval & ~bval; break;		// ANCB
	case 031: dval = aval ^ bval; break;		// EXOR
	case 032: dval = aval; break;			// ADIR
	case 033: dval = aval | bval; break;		// AND

	case 035: dval = ~aval & bval; break;		// ANDCA
	case 036: dval = aval & bval; break;		// AND
	case 037: dval = bval; break;			// BDIR

	default:
		{ printf("\n"); \
		errx(1, "alu 0%o not implemented: %08X line %o", M_ALU(uc), uc->line, mpc); }
	}

	if (most) {
		if (M_ALU(uc) == 016 || M_ALU(uc) == 002 || M_ALU(uc) == 006)
			aval = negA;
		bZ = ((dval & 0177777) == 0);
		bO = (!BIT15(bval ^ aval) && BIT15(bval ^ dval));
		bS = BIT15(dval);
		bC = dval > 0177777;
	} else
		bCl = dval > 0177777;

	if (M_SSAVE(uc)) {
		STS[pil] &= ~(STS_C|STS_Q);
		if (bC) STS[pil] |= STS_C;
		if (bO) STS[pil] |= (STS_O|STS_Q);
	}

	return dval & 0177777;
}

int
calcea(void)
{
	int ea;

	if ((IR & 0174000) == 0130000) {
		ea = SEXT8(IR) + oldCP;
	} else {
		ea = BIT8(IR) ? B[pil] : oldCP;
		if (BIT10(IR) & !BIT9(IR) & !BIT8(IR))
			ea = 0;
		ea += SEXT8(IR);
		if (BIT9(IR))
			ea = mem[ea & 0177777];
		if (BIT10(IR))
			ea += X[pil];
	}
	return ea & 0177777;
}

void
cycles(union ucent *uc, int aval)
{
	int n;

	if (M_CYCLE(uc) == 0)
		return;
	if (tflag)
		fprintf(tfp, " cycle in adr %o ", mem[052744]);

	switch (M_CYCLE(uc)) {
	case 01:				// CEATR
		R = calcea();
		break;

	case 02: R = CP; break;			// CPTR

	case 03:				// CFC
		// 1) check if pending interrupts.
		// 2) Fetch instrction
		// 3) Ensure mpc is within memory space
		n = pk_calc();
		if (inton && pil != n) {
			mpc = 0400 - 1;
		} else {
			H = CAR = IR = mem[CP];
if (IR == 0140134) tflag = 1;
			oldCP = CP++;
			if (dfp)
				dprint();
			if (inton == 0 && (IR & 0177400) == 0151000)
				mpc = -1; // stop
			else
				mpc = epg(IR, 0)-1;		
			// mpc will be incremented before next micro insn
			if (mpc > promsz-1) { // Illegal instruction
				int14(IIE_II);
				if (inton && pil != pk_calc())
					mpc = 0400 - 1;
			}
		if (rtc_ctr-- == 0) {
			rtc_int();
			rtc_ctr = 10000;
		}
		}
		break;

	// Write cycle: A goes to IB which is written to memory.
	case 04:
		mem[++R] = aval;
		break;				// CWR1
	case 05:				// CW
		R = calcea();
		mem[R] = aval;
		break;

	case 06: H = mem[++R]; break;		// CRR1
	case 07:
		R = calcea();
		H = mem[R];
		break;				// CR

	default: ;
	}
	if (tflag)
		fprintf(tfp, " cycle %o R=%o adr %o ", M_CYCLE(uc), R, mem[052744]);
}

void
arith(union ucent *uc)
{
	int aval, bval;
	int dval;
	int arsel = M_ARSEL(uc);
	int true;
	union ucent ucs, *ucb = &ucs;

	int spec1 = M_DEST(uc) == 012;
	int spec2 = M_B(uc) == 012;
	int spec3 = M_A(uc) == 012;

	*ucb = *uc;
	ormap(uc, ucb);
	aval = areg(ucb, M_A(ucb), pil);

	if (spec1) {
		if (M_B(ucb) == 017)		// BIR3
			M_B_W(ucb, IR & 15);
		trr(ucb, aval);
		return;
	} else if (spec2) {
		bval = breg(ucb, pil);	// set bit in bval
	} else if (spec3) {
		if (M_B(ucb) == 017)
			M_B_W(ucb, IR & 15);
		tra(ucb);
		return;
	} else {
		bval = breg(ucb, pil);
	}

	true = 1;
	if (M_B(ucb) != 012)
		true = ckcond(ucb);

	if (true)
		dval = alu(ucb, aval, bval, M_ARSEL(ucb));

	if (true) {
		AC[M_ARSEL(ucb)] = dval;
		setdreg(ucb, dval, pil);
	}

	if (M_CHLEV(ucb) && inton) {
		// Change level.  Update pil/pvl.
		pvl = pil;
		pil = pk_calc();
	}

	cycles(ucb, aval);
}


void
iblock(union ucent *uc)
{
	int bval, dval;
	union ucent ucs, *ucb = &ucs;

	int arsel = M_ARSEL(uc);
	int spec1 = M_DEST(uc) == 012;
	int spec2 = M_B(uc) == 012;
	int spec3 = M_A(uc) == 012;

	*ucb = *uc;
	ormap(uc, ucb);
	int slvl = M_DIRECT(uc) ? pil : M_LEVEL(ucb);
	int dlvl = M_DIRECT(uc) ? M_LEVEL(ucb) : pil;


	int aval = areg(ucb, M_A(ucb), slvl);
	if (spec1 == 0) // special case 1
		bval = breg(ucb, pil);

	dval = alu(ucb, aval, bval, M_ARSEL(ucb));

	if (spec1) {
		if (M_DEST(uc)) {
			printf("\n");
			errx(1, "iblock dest 0%o not implemented: %08X line %o", M_DEST(uc), uc->line, mpc);
		}
	} else {
		switch (M_ORSPECS(ucb)) {
		case 1: // only read
		case 0: setdreg(ucb, dval, dlvl); break; // no or

		case 3: setdreg(ucb, dval, dlvl); break;

		default:
		if (M_ORSPECS(ucb)) {
			printf("\n");
			errx(1, "iblock orspecs 0%o not implemented: %08X line %o", M_ORSPECS(ucb), uc->line, mpc);
		}
		}
	}

	cycles(ucb, aval);

	if (M_SSAVE(ucb)) {
		printf("\n");
		errx(1, "iblock ssave 0%o not implemented: %08X line %o", M_SSAVE(ucb), uc->line, mpc);
	}
}

#define EJUMP(x) if (uc->x) { printf("\n");	\
	errx(1, "jump " #x " 0%o not implemented: %08X line %o", uc->x, uc->line, mpc); }

void
jump(union ucent *uc)
{
	int true;

	if (M_PRIV(uc)) {
		if (pgon) {
			if (M_PRIV(uc)) { printf("\n");	\
	errx(1, "jump priv 0%o not implemented: %08X line %o", M_PRIV(uc), uc->line, mpc); }
		}
	}
	true = ckcond(uc);
	if (true)
		mpc = M_CAR(uc) ? CAR : M_ADDR(uc);
	else
		mpc++;
	if (tflag)
		fprintf(tfp, " %sJMP to %o", M_JCOND(uc) ? "C" : "", mpc);
}

/*
 * Loop instruction ALU ops reads from B, does something and saves in AC.
 */
void	
loop(union ucent *uc)
{
	union ucent ucb;
	int m, xbit, shright;
	int bvm, bvl, aclbit = 0;

	shright = M_LORSHT(uc) ? (IR & 040) : M_LSHR(uc);

	// 1 == rotational, 2 == zero input
	int shtyp = M_LORSHT(uc) ? (IR >> 9) & 3 : M_LSHT(uc);

	for (;;) {
		if (M_TERM(uc) == 0 && SC == 0)
			break;
		if (M_TERM(uc) == 2 && (SC == 0 || (SH[1] & 0100000)))
			break;
		if (M_TERM(uc) == 3 && (SC == 0 || (SH[1] & 0000100)))
			break;

		unsigned int ACL, ALL;

		ALL = (Alatch[1] << 16) | Alatch[0];
		ACL = (AC[1] << 16) | AC[0];

		switch (M_LB(uc)) {
		case 0: ACL = 0; break;
		case 013: break;
		case 014: 
			ACL >>= 1;
			ACL |= (aclbit << 31);
			// XXX input fr}n ALU???
			break;

		case 015:
			ACL <<= 1;
			// XXX
			break;

		default: 
			errx(1, "unspec B reg %02o", M_LB(uc));
		}

		int alucmd = M_LALUM(uc);
		if ((M_LALT(uc) && BIT0(SH[0])) ||
		    ((M_LALT(uc) == 0) && BIT15(SH[1])))
			alucmd = M_LALUL(uc);

		int acsign = BIT15(AC[1]); // before calculations
		ull ACLlong;
		unsigned int acinv = ~ALL;
		Reg ACold;
		switch (alucmd) {
		case 00: ACLlong = ACL-1; break;	// BM1
		case 01: ACLlong = (ull)ACL + (ull)ALL; break;// PLUS
		case 03: ACLlong = ACL;	break;			// BD1
		case 016:
			ACLlong = (ull)ACL + (ull)acinv + 1;
			break;
		default:
			errx(1, "bad cmd %02o", alucmd);
		}

		AC[1] = ACLlong >> 16;
		AC[0] = ACLlong;
		aclbit = ACLlong > 0xffffffffULL;
		bZ = AC[1] == 0;
		bC = aclbit;
		bS = BIT15(AC[1]);
		bO = BIT15(alucmd == 016 ? acinv : ALL) ^ BIT15(ACold);
		bO = ((bO == 0) && BIT15(ACold ^ ACLlong));
		if (M_LSSAVE(uc)) {
			STS[pil] &= ~(STS_C|STS_Q);
			if (bC) STS[pil] |= STS_C;
			if (bO) STS[pil] |= (STS_O|STS_Q);
		}

		switch (M_TG(uc)) {
		case 0: break;
		case 1: if (SH[0] & 1) STS[pil] |= STS_TG; break;
		case 2: if (AC[0] & 1) STS[pil] |= STS_TG; break;
		case 3: if ((AC[0] | AC[1]) == 0) STS[pil] |= STS_TG; break;
		}


		if (shright == 0) { // shift left
			m = (SH[1] & 0100000) != 0;
			/* defined at 1062 lower left */
			if (M_ENDID(uc)) {	// shift left input
				if (acsign) {
					xbit = bC ? 1 : BIT0(SH[0]);
				} else
					xbit = bC ? BIT0(SH[0]) : 0;
			} else {
				switch (shtyp) {
				case 2: case 0: xbit = 0; break;
				case 1: xbit = m; break;
				case 3: xbit = ((STS[pil] & STS_M) != 0); break;
				}
			}
			if (M_LSH32(uc)) { // Combined shift
				SH[1] = (SH[1] << 1) | BIT15(SH[0]);
				SH[0] = (SH[0] << 1) | xbit;
			} else
				SH[1] = (SH[1] << 1) | xbit;
		} else {
			m = M_LSH32(uc) ? SH[0] & 1 : SH[1] & 1;
			switch (shtyp) {
			case 0: xbit = (SH[1] & 0100000) != 0; break;
			case 1: xbit = m; break;
			case 2: xbit = 0; break;
			case 3: xbit = ((STS[pil] & STS_M) != 0); break;
			}
			if (M_LSH32(uc)) {
				SH[0] = (SH[0] >> 1) | (SH[1] << 15);
				SH[1] = (SH[1] >> 1) | (xbit << 15);
			} else
				SH[1] = (SH[1] >> 1) | (xbit << 15);
		}
		if (M_LM(uc)) {
			STS[pil] &= ~STS_M;
			if (m) STS[pil] |= STS_M;
		}

		if (SC < 0)
			SC++;
		else
			SC--;
	}
}

int ttostat = 010;
FILE *ptrfp;
unsigned char ptr_char;
int ptr_intr;

static int incnt;

void
ioexec(union ucent *uc)
{
	int i;
	char inchar;

	if (tflag)
		fprintf(tfp, " iox %o", CAR & 03777);
	switch (CAR & 03777) {
	case 0011: // clear counter
		rtc_ctr = 10000; // something
		break;

	case 0013: // Set RTC status
		if (BIT0(ioreg))  rtc_doint = 1;
		if (BIT13(ioreg)) rtc_rft = 0;
		break;

	case 0300:
		if (ifd) {
			if ((i = fgetc(ifd)) == EOF) {
				fclose(ifd);
				ifd = NULL;
			} else {
				if (i == 10) i = 13;
				ioreg = (unsigned char)i;
				break;
			}
		}

		if ((i = read(sfd, &inchar, 1)) < 0)
			return;
		ttistat &= ~010;
		ioreg = inchar;
		break;

	case 0302: ioreg = ttistat; break;	// Read status
	case 0303: break;			// Set tti/tto param

	case 0305:
		inchar = ioreg;
		printf("%c", inchar); fflush(stdout);
		break;

	case 0306: ioreg = ttostat; break;	// read status

	case 0307: break;			//

	case 0400: // read char
		ioreg = ptr_char;
		break;

	case 0402: // read ptr status
		ioreg = ptr_intr;
		if (ptrfp) {
			if ((i = fgetc(ptrfp)) == EOF) {
				fclose(ptrfp);
				ptrfp = NULL;
			} else {
				ptr_char = i;
				ioreg |= 010; // Ready for transfer
			}
		}
		break;

	case 0403: // set ptr status
		ptr_intr = ioreg & 1;
		if (ioreg & 4) {  // activate
			if (hname) {
				if (ptrfp == NULL)
					if ((ptrfp = fopen(hname, "r")) == NULL)
						err(1, "fopen(hname");
			}
		}
		break;

	default:
		fprintf(stderr, "ioexec %o: CAR %o ioreg %o addr %o\r\n", mpc, CAR, ioreg, CAR & 03777);
		int14(IIE_IOX);
	//	printf("\n"); errx(1, "ioexec %o: CAR %o ioreg %o\n", mpc, CAR, ioreg);
	}
}

void
ident(union ucent *uc)
{
	int wrtioreg = 0;

	switch (IR & 03) {
	case 00: break;
	case 01: break;
	case 02: break;
	case 03:
		if (rtc_rft)
			wrtioreg = 1;
		break;
	}
	if (wrtioreg) {
		ioreg = wrtioreg;
	} else
		int14(IIE_IOX);
}

/*
 * rtc counter just reached 0.
 */
void
rtc_int()
{
	rtc_rft = 1;
	if (rtc_doint)
		pid |= (1 << 13);
}
