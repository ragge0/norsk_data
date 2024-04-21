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
 * REGISTERS, card 1002. 
 * All four of the cards are implemented in the same module.
 * All registers are stored in LUT RAM, so quite some LUTs are used here.
 */

module regs(
	input			clk,

	input [15:0]		ASUM,
	input [15:0]		BSUM,		// input from the ALUs.
	input			ASEL,		// MIR24 to select ALU
	input [3:0]		LEV,		// level used for writing
	input [6:0]		MIR6_0,		// select output from regs
	input [15:0]		IB_in,		// shared bus
	input			AEN,		// A to IB
	input			HKL,		// H clock
	input			HSEL,		// H mux
	input			HX,		// ...
	input			RLD,		// Load R
	input			RKL,		// Clock R
	input			CPCLR,		// 
	input			CPLD,		// 
	input			CPKL,		// 
	input [7:0]		STS,		// 
	input			WSCR,

	output reg [15:0]	R,
	output [15:0]		IB_ut,		// shared bus
	output reg [15:0]	AA,
	output reg [15:0]	BB
);

	wire [15:0]	SUM;
	reg [15:0]	CP;

	wire [15:0]	h;

	(* ram_style = "DISTRIBUTED" *)
	reg [15:0]	D[15:0], B[15:0], L[15:0], A[15:0], T[15:0],
		X[15:0], RSP[15:0], rSTS[15:0], rSCR[15:0];

	/* write regs checking */
	always @(posedge clk) begin
		if (WSCR)
			rSCR[LEV] <= SUM;
	end

	/* wires for all regs */
	wire [15:0]	rd = D[LEV];
	wire [15:0]	rb = B[LEV];
	wire [15:0]	rl = L[LEV];
	wire [15:0]	ra = A[LEV];
	wire [15:0]	rt = T[LEV];
	wire [15:0]	rx = X[LEV];
	wire [15:0]	sp = RSP[LEV];
	wire [15:0]	ssts = rSTS[LEV];
	wire [15:0]	scr = rSCR[LEV];

	/* Giant A mux */
	always @(*) begin
		case (MIR6_0[3:0])
		4'd00: AA = 16'd0;
		4'd01: AA = rd;
		4'd02: AA = CP;
		4'd03: AA = rb;
		4'd04: AA = rl;
		4'd05: AA = ra;
		4'd06: AA = rt;
		4'd07: AA = rx;
		4'd08: AA = { 8'b0, STS };
		4'd09: AA = h;
		4'd10: AA = 16'd0;	// 1010, I/O
		4'd11: AA = h;
		4'd12: AA = scr;
		4'd13: AA = R;
		4'd14: AA = sp;
		4'd15: AA = ssts;
		endcase
	end

	/* B mux */
	always @(*) begin
		case (MIR6_0[6:4])
		3'd00: BB = 16'd0;
		3'd01: BB = rd;
		3'd02: BB = CP;
		3'd03: BB = rb;
		3'd04: BB = rl;
		3'd05: BB = ra;
		3'd06: BB = rt;
		3'd07: BB = rx;
		endcase
	end

	/* Clock (instead of latch) for H */
	reg [15:0]	H;
	always @(posedge clk)	// 3A
		if (HKL)
			H <= IB_in;
	wire [15:0]	hline = HKL ? IB_in : H;

	assign 		h = HSEL ? hline : {16{HX}};

	/*
	 * CP reg.
	 * FPGA change: Clock on system clock instead.
	 */
	always @(posedge clk) begin
		if (CPCLR)
			CP <= 0;
		else if (CPLD && CPKL)
			CP <= SUM;
		else if (CPKL)
			CP <= CP + 1;
	end

	/*
	 * R reg.
	 * FPGA change: Clock on system clock instead.
	 */
	always @(posedge clk) begin
		if (CPCLR)
			R <= 0;
		else if (RLD && RKL)
			R <= 0;	// XXX
		else if (RKL)
			R <= R + 1;
	end



	assign IB_ut = AEN ? AA : 16'b0;	// driver 5A

	/* A/B card output select */
	assign SUM = ASEL ? BSUM : ASUM;

endmodule
