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
 * See how the timing signals on Nord10 looks like.
 * Based on the circuits on board 1025.
 */
#include <stdio.h>

int and(int a, int b);
int and3(int a, int b, int c);
int nand(int a, int b);
int nand3(int a, int b, int c);
int or4(int a, int b, int c, int d);
int nor4(int a, int b, int c, int d);
void mktc(int a2, int a7, int a10);
int tc0up(void);
int tc1up(void);
int tc2up(void);
void dblupdate(void);

int TC0, TC0_n, TC1, TC1_n, TC2, TC2_n;
int WT, WT_n, RET3_n, RET5_n;
int T0, T1, T2, T3, T4, T5, T5_n, T1_n;
int MIR[32], MIR_n[32];
int SIR9, CPDEST, DBL, DDBL_n;

int
main(int argc, char *argv[])
{
	int i;
	int x_2a2 = 0, x_2a7 = 0, x_2a10 = 0;

	/* after MCL */
	for (i = 0; i < 32; i++)
		MIR_n[i] = 1;

	for (i = 0; ; i++) {
		printf("%03d: T0=%d T1=%d T2=%d T3=%d T4=%d T5=%d\n", i,
		    T0, T1, T2, T3, T4, T5);
		if (i == 20) break;

		mktc(x_2a2, x_2a7, x_2a10);	// Sets TC[0-2] values.

		// Wait logic.  Not now.
		WT = 0; WT_n = !WT;

		/* Timing pulse updates. No dependencies */
		T0 = and3(TC0_n, TC1_n, TC2_n);
		T1 = and(TC0, TC1_n);
		T2 = and(TC0, TC1);
		T3 = and3(TC0_n, TC1, TC2_n);
		T4 = and(TC1, TC2);
		T5 = and(TC1_n, TC2);
		T5_n = nand(TC1_n, TC2);
		T1_n = nand(TC1_n, TC0);

		// DBL update, depends on MIR + TCx+T1+T5 (circular CPDEST?)
		// Sets DBL + DDBL
		dblupdate();

		/* last input to timing D flipflops, when everything is settled */
		int c_10b11 = tc0up();
		int c_6a8 = tc1up();
		int c_4c8 = tc2up();

		/* FLIPPING! clock time */
		x_2a2 = c_10b11, x_2a7 = c_6a8, x_2a10 = c_4c8;
	}
}

/*
 * 
 */
void
dblupdate()
{
	int x_16c6, AT3_n;
	static int last_T1_n, last_AT3_n;	// Last timing states (for rising edge)
	static int last_14c5_Q, last_14c9_Q;	// last output Q (D ff states)
	int last_14c5_Qn;

	AT3_n = nand3(TC1, TC0_n, TC2_n);

	if (T5_n == 0)				// preset output(s)
		last_14c5_Q = last_14c9_Q = 1;

	last_14c5_Qn = !last_14c5_Q;

	// Update MUX first
	if (last_14c5_Qn == 0) {
		switch ((MIR[23] << 2) | (MIR[22] << 1) | MIR[21]) {
		case 0: case 4: case 6: x_16c6 = 1; break;
		case 1: case 5: case 7: x_16c6 = !SIR9; break;
		case 2: case 3: x_16c6 = !CPDEST; break;
		}
	} else
		x_16c6 = 1;

	if (T5_n) { // no preset
		if (AT3_n && last_AT3_n == 0)
			last_14c9_Q = last_14c5_Q;
		if (T1_n && last_T1_n == 0)
			last_14c5_Q = x_16c6;
	}
	DDBL_n = last_14c9_Q;
	DBL = last_14c5_Q;
	last_T1_n = T1_n;
	last_AT3_n = AT3_n;
}

/*
 * Called when MIRKL goes one.
 */
void
mirupdate()
{
	
}

int
tc0up(void)
{
	int c_10b8 = nand(WT, TC0);
	int c_8a6 = nand3(WT_n, TC1_n, TC2_n);
	return nand(c_10b8, c_8a6);
}

int
tc1up(void)
{
	int a_6a1 = and(T3, RET3_n);
	int a_6a2 = and(TC0, WT_n);
	int a_6a3 = and3(WT_n, T5, RET5_n);
	int a_6a4 = and(WT, TC1);
	return nor4(a_6a1, a_6a2, a_6a3, a_6a4);
}

int
tc2up(void)
{
	int a_4c1 = and3(T3, RET3_n, WT_n);
	int a_4c2 = and(WT, TC2);
	int a_4c3 = and(T5, RET5_n);
	return nor4(a_4c1, a_4c2, a_4c3, T4);
}

void
mktc(int a2, int a7, int a10)
{
	TC0_n = !a2;
	TC0 = !TC0_n;
	TC1 = !a7;
	TC1_n = !TC1;
	TC2 = !a10;
	TC2_n = !TC2;
}

int and(int a, int b) { return a & b; }
int and3(int a, int b, int c) { return a & b & c; }
int nand(int a, int b) { return !and(a, b); }
int nand3(int a, int b, int c) { return !and3(a, b, c); }
int or4(int a, int b, int c, int d) { return a|b|c|d; }
int nor4(int a, int b, int c, int d) { return !or4(a,b,c,d); }
