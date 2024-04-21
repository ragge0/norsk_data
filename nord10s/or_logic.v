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
 * OR LOGIC, card 1006. 
 */

module or_logic(
	input			clk,
	input			MCL,
	input [31:0]		ROM,
	input [15:0]		IR,
	input			LSEL,
	input			MOPC,
	input [3:0]		PIL,
	input			MIRKL,
	input			TC1,

	output			BMSRC,
	output [2:0]		SL,
	output [3:0]		LEV,
	output reg [15:0]	MIR15_0
);

	/*
	 * MUX for upper B arithmetic input. (3B)
	 */
	assign SL = MIR15_0[7] ? MIR15_0[6:4] : { 2'b0, TC1 };

	/*
	 * Bitmask selection.
	 */
	assign BMSRC = MIR15_0[7:4] == 4'b1010;




	wire		or0 = (ROM[18:16] == 3'o0);
	wire		or1 = (ROM[18:16] == 3'o1);
	wire		or3 = (ROM[18:16] == 3'o3);
	wire		or4 = (ROM[18:16] == 3'o4);
	wire		or5 = (ROM[18:16] == 3'o5);
	wire		or6 = (ROM[18:16] == 3'o6);
	wire		or7 = (ROM[18:16] == 3'o7);

	wire 		ir0_2 = |IR[2:0];
	wire [3:0]	ir1_210 = ir0_2 ? { 1'b0, IR[2:0] } : 4'b1000;
	wire [3:0]	ir0_2_0 = { 1'b0, IR[2:0] };
	wire [3:0]	ir0_5_3 = { 1'b0, IR[5:3] };
	wire [3:0]	ir20blk = (ROM[30] & IR[2:0] == 3'b010) ? 4'b1110 : ir1_210;

	/* quad bits 15-12 */
	wire [3:0]	m15_12 = (or1|or3) ? IR[6:3] :
		(or0|or5|or6|or7) ? ROM[15:12] :
		ROM[31] ? { IR[5], IR[10:9], ROM[12] } :
		{ ROM[15], IR[10:8] };

	/* quad bits 11-8 */
	wire [3:0]	m11_8 = (or0 | or1 | or4) ? ROM[11:8] :
		(or5 | or6) ? ir0_2_0 :
		(or7) ? ir0_5_3 : ir20blk;

	/* quad bits 7-4 */
	wire 		sel_6_4_A = ~ROM[31] & ROM[18] & ~ROM[15]; // 15B
	wire		sel_6_4_0 = ROM[16] & ROM[18] & IR[6];	// 17B
	wire [2:0]	m6_4 = sel_6_4_0 ? 3'b0 : sel_6_4_A ? IR[2:0] : ROM[6:4];
	wire [3:0]	m7_4 = { ROM[7], m6_4 };

	/* quad bits 3-0 */
	wire [3:0]	m3_0 = or1 ? ir20blk : (or3 & ~ROM[30]) ? ir1_210 :
		((or4|or6) & ~ROM[30] & ~ROM[15]) ? ir0_2_0 :
		(or5|or7) ? ir0_2_0 : ROM[3:0];

	/* MIR 15-0: uses system clock */
	always @(posedge clk)
		if (MCL)
			MIR15_0 <= 0;
		else if (MIRKL)
			MIR15_0 <= { m15_12, m11_8, m7_4, m3_0 };

	/* Level mux */
	assign LEV = ~LSEL & MOPC ? 4'b0 : LSEL ? MIR15_0[15:12] : PIL;

endmodule
