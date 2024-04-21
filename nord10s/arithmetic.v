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
`timescale 1ns / 1ps
`default_nettype none
/*
 * AMIN ARITHMETIC, card 1001.
 */

module arithmetic (
	input			clk,
//	input			MCL,
	input			C,
	input [15:0]		AA,
	input [15:0]		BB,
	input			ACKL,
	input			AKL,
	input [5:0]		SHC,
	input [1:0]		SHS,
	input			SHKL,
	input			SHM,
	input			SHX,
	input [2:0]		SL,
	input [3:0]		S,
	input			M,
	input			BC15,
	input			BC0,
	input [15:12]		MIR15_12,

	output			SH6,
	output			AC15,
	output			AC0,
	output			ZERO,
	output			SH15,
	output			SH0,
	output 			CO,
	output [15:0]		SUM
);

	reg [15:0]	A, B, AC, BH, SH;

	/*
	 * Shift register.
	 */
	always @(posedge clk) begin
		if (SHKL) begin
			if (&SHS)
				SH <= SUM;
			else if (SHS[0])
				SH <= { SH[14:0], SHX };
			else if (SHS[1])
				SH <= { SHM, SH[15:1] };
		end
	end
	assign SH15 = SH[15];
	assign SH6 = SH[6];
	assign SH0 = SH[0];

	/*
	 * Bitmask (microcode instruction).
	 */
	always @(*) begin
		case (MIR15_12)
		4'd00: BH = 16'o000001;
		4'd01: BH = 16'o000002;
		4'd02: BH = 16'o000004;
		4'd03: BH = 16'o000010;
		4'd04: BH = 16'o000020;
		4'd05: BH = 16'o000040;
		4'd06: BH = 16'o000100;
		4'd07: BH = 16'o000200;
		4'd08: BH = 16'o000400;
		4'd09: BH = 16'o001000;
		4'd10: BH = 16'o002000;
		4'd11: BH = 16'o004000;
		4'd12: BH = 16'o010000;
		4'd13: BH = 16'o020000;
		4'd14: BH = 16'o040000;
		4'd15: BH = 16'o100000;
		endcase
	end

	/*
	 * A storage.
	 */
	always @(posedge clk)
		if (AKL)
			A <= AA;

	/*
	 * B input selection.
	 */
	always @(*) begin
		case (SL)
		3'o0: B = { {10{SHC[5]}}, SHC };
		3'o1: B = SH;
		3'o2: B = BH;
		3'o3: B = AC;
		3'o4: B = { BC0, AC[15:1] };
		3'o5: B = { AC[14:0], BC15 };
		3'o6: B = 0;	// XXX  loop?
		3'o7: B = BB;
		endcase
	end

	/*
	 * 14181 emulation.  A terrible amount of calculations.
	 * Note that the arithmetic part is slightly changed.
	 */
	wire [15:0]	invB = ~B, invA = ~A, mone = 16'o177777;
	reg [15:0]	xSUM;
	reg		xCO;
	always @(*) begin
		case ({ M, S })
		5'o00: { xCO, xSUM } = { 1'b0, B } + { 1'b0, mone} ;
		5'o01: { xCO, xSUM } = { 1'b0, B } + { 1'b0, A} ;
		5'o02: { xCO, xSUM } = { 1'b0, B } + { 1'b0, invA} ;
		5'o03: { xCO, xSUM } = { 1'b0, B } + { 17'b0 } ;
		5'o04: { xCO, xSUM } = { 1'b0, B } + { 1'b0, mone } + { 16'b0, C};
		5'o05: { xCO, xSUM } = { 1'b0, B } + { 1'b0, A } + { 16'b0, C};
		5'o06: { xCO, xSUM } = { 1'b0, B } + { 1'b0, invA } + { 16'b0, C};
		5'o07: { xCO, xSUM } = { 1'b0, B } + { 17'b0 } + { 16'b0, C};
		5'o10: { xCO, xSUM } = { 1'b0, B } + { 1'b0, mone} ;// undef
		5'o11: { xCO, xSUM } = { 1'b0, B } + { 1'b0, A} ;   // undef
		5'o12: { xCO, xSUM } = { 1'b0, B } + { 1'b0, invA} ;// undef
		5'o13: { xCO, xSUM } = { 1'b0, B } + { 17'b0 } ;   // undef
		5'o14: { xCO, xSUM } = { 1'b0, B } + { 1'b0, mone } + 1;
		5'o15: { xCO, xSUM } = { 1'b0, B } + { 1'b0, A } + 1;
		5'o16: { xCO, xSUM } = { 1'b0, B } + { 1'b0, invA } + 1;
		5'o17: { xCO, xSUM } = { 1'b0, B } + { 17'b0 } + 1;
		5'o20: xSUM = invB;
		5'o21: xSUM = ~(A & B);
		5'o22: xSUM = invB | A;
		5'o23: xSUM = 1;
		5'o24: xSUM = ~(A | B);
		5'o25: xSUM = invA;
		5'o26: xSUM = ~(A ^ B);
		5'o27: xSUM = A | invB;
		5'o30: xSUM = invB & A;
		5'o31: xSUM = A ^ B;
		5'o32: xSUM = A;
		5'o33: xSUM = A | B; 
		5'o34: xSUM = 0;
		5'o35: xSUM = B & invA;
		5'o36: xSUM = B & A;
		5'o37: xSUM = B;
		endcase
	end
	assign ZERO = ~|SUM;
	assign CO = xCO;
	assign SUM = xSUM;

	/*
	 * AC storage.
	 */
	always @(posedge clk)
		if (ACKL)
			AC <= SUM;
	assign AC15 = AC[15];
	assign AC0 = AC[0];

endmodule
