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
 * STATUS, card 1062. 
 */

module status(
	input		clk,
	input [1:0]	CRSEL,
	input [31:0]	MIR,
	input		ACR16,
	input		BCR16,
	input [3:0]	LEV,
	input [7:0]	BSUM7_0,
	input		BSUM15,
	input		SKL,	// Statusklocka
	input		ASH15,
	input		BSH15,
	input		BC15,
	input		ASH0,
	input		BSH0,
	input		BSH6,
	input		AC0,
	input		BZRO,
	input		AZRO,
	input		STSRD,
	input		BAKL,
	input		WSHC,
	input		SHCKL,

	output [15:0]	IB_ut,
	output reg	TERM,
	output [5:0]	SHC,
	output		ASHM,
	output		BSHM,
	output		ASHX,
	output		BSHX,
	output [7:0]	STS,
	output		ACR0,
	output		BCR0
);

	wire [7:0]	DST;		// write data to status reg
	wire		PM, TG, M, Z, C, Q, O, K;

	/*
	 * counter register (for loop instruction).
	 */
	reg [6:0] shcnt;
	always @(posedge clk) begin
		if (WSHC && SHCKL)
			shcnt <= { BSUM7_0[5], BSUM7_0[5:0] };
		else if (~TERM && SHCKL) begin
			if (shcnt[6])
				shcnt <= shcnt + 1;
			else
				shcnt <= shcnt - 1;
		end
	end
	assign SHC = shcnt[5:0];

	/*
	 * Counter termination, uses a MUX.
	 * Will always terminate if counter == 0.
	 */
	wire x_zero = ~|SHC;
	reg x_term;
	always @(*) begin
		casez ({MIR[3:2], BSH15})
		3'b00?: x_term = 0;	// only counter
		3'b010: x_term = C;	// not used
		3'b011: x_term = ~C;	// not used
		3'b100: x_term = 0;	// SH31 check
		3'b101: x_term = 1;	// SH31 check
		3'b11?: x_term = BSH6;	// SH22 (32-bit fp)
		endcase
	end
	assign TERM = x_zero | x_term;

	/*
	 * Logic and mux 19b for TG.	
	 */
	wire TGSEL0 = MIR[8] & MIR[31];
	wire TGSEL1 = MIR[9] & MIR[31];

	/*
	 * Status storage.
	 * Slightly modified using a clocked flipflop instead of a 
	 * latch following the memory cell.
	 */
	(* ram_style = "DISTRIBUTED" *)
	reg [7:0]	rSTS[15:0];

	always @(posedge clk)
		if (SKL)
			rSTS[LEV] <= DST;
	assign STS = rSTS[LEV];

	reg [7:0] x_stff;
	always @(posedge clk) begin
		if (SKL)
			x_stff[7:2] <= STS[7:2];
		if (BAKL)
			x_stff[1:0] <= STS[1:0];
	end
	assign IB_ut = STSRD ? { 8'b0, x_stff } : 16'b0;
	assign PM = x_stff[0], TG = x_stff[1], K = x_stff[2];
	assign Z = x_stff[3], Q = x_stff[4], O = x_stff[5];
	assign C = x_stff[6], M = x_stff[7];
	


	/* alu bit-in logic */
	assign ACR0 = ~CRSEL[0] ? 0 : CRSEL[1] ? 1 : C;
	assign BCR0 = MIR[31] ? ACR16 : ACR0;

	/*
	 * Shift register (in arithmetic) linkage logic.
	 * A bunch of MUXes, converted to postive logic.
	 */
	reg x_19d7, x_19d9, x_16d9;
	wire x_13d4, x_13d7;
	always @(*) begin	// 19D
		case (MIR[14:13])
		2'b00: begin
			x_19d7 = 1'b0;
			x_19d9 = ASH15;
			BSHM = BSH15;
			x_16d9 = 0;
		end
		2'b01: begin
			x_19d7 = BSH15;
			x_19d9 = ASH0;
			BSHM = x_13d4;
			x_16d9 = x_13d7;
		end
		2'b10: begin
			x_19d7 = 1'b0;
			x_19d9 = 1'b0; 
			BSHM = 1'b0;
			x_16d9 = 1'b0;
		end
		2'b11: begin
			x_19d7 = M;
			x_19d9 = M;
			BSHM = M;
			x_16d9 = M;
		end
		endcase
	end
	// 13D
	assign x_13d4 = MIR[12] ? ASH0 : BSH0;
	assign x_13d7 = MIR[12] ? BSH15 : ASH15;
	assign BSHX = MIR[12] ? ASH15 : x_19d7;
	assign ASHM = MIR[12] ? BSH0 : x_19d9;
	reg x_10d5;
	always @(*) begin
		case ({ BC15, BCR16})
		2'b00: x_10d5 = 1'b0;
		2'b01: x_10d5 = ASH0;
		2'b10: x_10d5 = ASH0;
		2'b11: x_10d5 = 1'b1;
		endcase
	end
	assign ASHX = MIR[1] ? x_10d5 : x_16d9;

	/*
	 * Check if conditional is TRUE.
	 */
	// XXX FIXME

endmodule
