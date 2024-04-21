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
 * DECODING, card 1007.
 */

module decoding(
	input			clk,
	input			MCL,
	input [31:0]		ROM,
	input			ALT,
	input			SIR9,
	input			T1,
	input			T5,
	input			MIRKL,
	input [11:8]		MIR11_8,
	input			MIR15,
	input			ACTLOOP,
	input			TRRSTS,
	input			BMSRC,
	input			T3,

	output			SI,
	output			SKL,
	output			WREN,
	output			WSTS,
	output			WD,
	output			WB,
	output			WL,
	output			WA,
	output			WT,
	output			WX,
	output			IODEST,
	output			WSHC,
	output			WSCR,
	output			DEST15,
	output			WSP,
	output			WSSTS,
	output [1:0]		ASHS,
	output [1:0]		BSHS,
	output reg [31:19]	MIR31_19,
	output			DBL,
	output			ARM,
	output [1:0]		CRSEL,
	output [3:0]		S
);

	/* NORMAL WIRES */
	wire		SHDEST, STSDEST;

	/* NEGATIVE WIRES TO SIMPLIFY LOGIC */
	wire		T1_n = ~T1, T5_n = ~T5;
	wire		DBL_n;
	assign DBL = ~DBL_n;

	/* UNASSIGNED WIRE */
	wire		CPDEST = 0;
	wire		AT3 = 0;
	wire		TRUE = 1;
	wire		DDBL = 0;

	/*
	 * Control for shift registers.
	 */
	wire x_6c12, x_6c8, x_4c3, x_4c6;
	assign x_6c12 = ~(MIR31_19[24] & WREN & SHDEST);
	assign x_6c8 = ~(~MIR31_19[24] & WREN & SHDEST);
	assign x_4c3 = ~(MIR15 & ACTLOOP);
	assign x_4c6 = ~(~MIR15 & ACTLOOP);
	assign BSHS[1] = ~(x_6c12 & x_4c6);
	assign ASHS[0] = ~(x_4c3 & x_6c8);
	assign BSHS[0] = ~(x_6c12 & x_4c3);
	assign ASHS[1] = ~(x_6c8 & x_4c6);



	wire		x_16c6_n, x_14c_q, x_14c_q_n;
	reg		x_16c6;
	always @(*) begin		// MUX 16C
		casez (MIR31_19[23:21])
		3'b000: x_16c6 = 0;
		3'b001: x_16c6 = SIR9;
		3'b01?: x_16c6 = CPDEST;
		3'b1?0: x_16c6 = 0;
		3'b1?1: x_16c6 = SIR9;
		endcase
	end
	assign x_16c6_n = x_14c_q_n ? 1 : ~x_16c6;

	/* flipflop 14C */
	reg		x_14c_qx;
	always @(posedge T1_n)
		x_14c_qx <= T5_n == 0 ? 1 : x_16c6_n;
	assign x_14c_q = T5_n == 0 ? 1 : x_14c_qx;
	assign x_14c_q_n = ~x_14c_q;

	assign DBL_n = x_14c_q;

	/* slight change to drawing due to different ALU design */
	assign S = ALT ? MIR31_19[23:20] : MIR31_19[28:25];	// 4B
	assign ARM = ALT ? MIR31_19[24] : MIR31_19[29];		// 16B
	assign CRSEL = S[3:2];

	/*
	 * Flipflops ROM -> MIR
	 */
	always @(posedge clk) begin
		if (MCL)
			MIR31_19 <= 0;
		else if (MIRKL)
			MIR31_19 <= ROM[31:19];
	end

	/*
	 * Demux for destination regs.
	 */
	assign WREN = (TRUE | BMSRC | MIR31_19[30]) &
		~MIR31_19[31] & ~DDBL & T3;
	assign WD = MIR11_8 == 4'd01 ? WREN : 0;
	assign CPDEST = MIR11_8 == 4'd02 ? WREN : 0;
	assign WB = MIR11_8 == 4'd03 ? WREN : 0;
	assign WL = MIR11_8 == 4'd04 ? WREN : 0;
	assign WA = MIR11_8 == 4'd05 ? WREN : 0;
	assign WT = MIR11_8 == 4'd06 ? WREN : 0;
	assign WX = MIR11_8 == 4'd07 ? WREN : 0;
	assign STSDEST = MIR11_8 == 4'd08 ? WREN : 0;
	assign SHDEST = MIR11_8 == 4'd09 ? WREN : 0;
	assign IODEST = (MIR11_8 == 4'd10) ? WREN : 0;
	assign WSHC = MIR11_8 == 4'd11 ? WREN : 0;
	assign WSCR = MIR11_8 == 4'd12 ? WREN : 0;
	assign DEST15 = MIR11_8 == 4'd13 ? WREN : 0;
	assign WSP = MIR11_8 == 4'd14 ? WREN : 0;
	assign WSSTS = MIR11_8 == 4'd15 ? WREN : 0;
	assign WSTS = TRRSTS | STSDEST;			// 18A11

	/*
	 * status clock.
	 */
	assign SKL = (AT3 & ACTLOOP) |
		(MIR31_19[19] & WREN & AT3) | (WSTS & AT3);

	/*
	 * Level change? Set if no change.
	 */
	assign SI = MIR31_19[30] | ~MIR31_19[20];

endmodule
