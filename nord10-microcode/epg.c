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
 * C implementation of the combinatorical logic for the
 * Nord-10 Entry Point Generator (based on schematics on board 1075).
 * Input is IR (+ old MIR).
 * Output is MPC.
 */

#include "stdio.h"
#include "bits.h"

int sn7464(int a1, int a2, int a3, int a4);
int sn7411(int a1, int a2, int a3);
int sn7410(int a1, int a2, int a3);
int sn7400(int a1, int a2);

int
epg(int ir, int mir)
{
	int ir_n = ~ir;
	int mir_n = ~mir;
	int car = ir;
	int car_n = ~ir;

	int epc;

	int opint_n = 1;
	int int_n = 1;

	int xep0 = sn7400(sn7410(BIT13(ir), BIT11(ir), BIT7(ir)),
		sn7400(BIT7(ir), BIT12(ir_n)));

	int xep1 = sn7464(MKB4(BIT6(ir_n), BIT7(ir_n), BIT11(ir_n), BIT12(ir_n)),
		0,
		MKB3(BIT11(ir), BIT12(ir), BIT13(ir_n)),
		MKB2(BIT8(ir_n), BIT8(ir_n)));

	int d18_1 = sn7411(BIT6(ir_n), BIT11(ir_n), BIT7(ir_n));
	int c18_1 = sn7400(BIT11(ir), BIT12(ir));

	int xep2 = sn7464(MKB4(BIT11(ir), BIT12(ir), BIT13(ir_n), BIT7(ir_n)),
		MKB2(BIT9(ir_n), BIT13(ir)),
		MKB3(d18_1, BIT12(ir_n), BIT12(ir_n)),
		MKB2(BIT9(ir_n), c18_1));

	int xep3 = sn7464(MKB4(BIT11(ir), BIT12(ir), BIT13(ir_n), BIT8(ir_n)),
		MKB2(BIT10(ir_n), BIT13(ir)),
		MKB3(d18_1, BIT12(ir_n), BIT12(ir_n)),
		MKB2(c18_1, BIT10(ir_n)));

	int mar8 = sn7464(MKB4(int_n, int_n, BIT29(mir), BIT8(car_n)),
		MKB2(BIT31(mir_n), int_n),
		MKB3(int_n, BIT29(mir_n), BIT8(mir_n)), MKB2(0, 0));

	int mar9 = sn7464(MKB4(opint_n, opint_n, BIT29(mir), BIT9(car_n)),
		MKB2(BIT31(mir_n), opint_n),
		MKB3(opint_n, BIT29(mir_n), BIT9(mir_n)), MKB2(0, 0));

	int c18_2 = sn7400(BIT11(ir_n), BIT13(ir));

	int mpas0 = sn7464(
		MKB4(BIT14(ir_n), BIT14(ir_n), c18_2, BIT31(mir_n)),
		MKB2(BIT15(ir_n), BIT31(mir_n)),
		MKB3(BIT31(mir_n), BIT12(ir_n), BIT13(ir)),
		MKB2(BIT29(mir), BIT31(mir)));

	int xep5 = sn7411(BIT12(ir), BIT14(ir), BIT14(ir));
	int xep10 = sn7411(BIT6(ir), BIT11(ir_n), BIT12(ir_n));

	if (mpas0 == 0)
		epc = MKB12(0, 0, mar9, mar8,
		    0, 1, BIT15(ir), BIT14(ir),
		    BIT13(ir), BIT12(ir), BIT11(ir), 0);
	else
		epc = MKB12(0, xep10, mar9, mar8,
		    1, BIT13(ir), xep5, BIT11(ir),
		    xep3, xep2, xep1, xep0);

	return epc;
}

int
sn7400(int a, int b)
{
	return !(a && b);
}

int
sn7410(int a1, int a2, int a3)
{
	return !(a1 && a2 && a3);
}

int
sn7411(int a1, int a2, int a3)
{
	return (a1 && a2 && a3);
}

int
sn7464(int a1, int a2, int a3, int a4)
{
	a1 = (a1 == 15);
	a2 = (a2 == 3);
	a3 = (a3 == 7);
	a4 = (a4 == 3);

	return !(a1 || a2 || a3 || a4);
}
