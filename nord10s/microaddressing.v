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
 * MICROADDRESSING, card 1075.
 *
 * Signal notes:
 *	CJARG - Set for CJP or ARG classes of instructions.
 *	SIR9 -  Set if CJARG=0 && I=1 (Indirect but not CJP)
 *	RIR9 -  Set if IR9=1 || CJARG=1 (Ind or CJP)
 */

module microaddressing (
	input		clk,
	input		MCL,
	input		MPCLD,
	input		MPCKL,
	input		CIKL,
	input [15:0]	IB_in,
	input [31:0]	MIR,
	input		DBL,
	input		STS0,

	output		CJARG,
	output		BREN,
	output		REL,
	output		SIR9,
	output [15:0]	IB_ut,
	output reg [15:0] IR,
	output reg [11:0] MPC
);

	/* THESE ARE MISSING INITIALIZATION */
	wire		OPINT = 0, INT = 0;

	reg [11:0]	mpcmux;
	wire		MAR9, MAR8, XEP5, XEP3, XEP2, XEP1, XEP0, MPAS0;
	wire		XEP10;
	wire [11:0]	CAR = IR[11:0];

	assign CJARG = IR[15] & IR[13] & IR[12] & ~IR[11];
	assign SIR9 = ~CJARG & IR[9];
	wire RIR9 = CJARG | IR[9];

	assign BREN = (IR[8] | RIR9 | ~IR[10]) & ~DBL;
	assign REL =  (RIR9 | ~IR[10]) & (CJARG | ~IR[8]);


	/* CAR or IR */
	always @(posedge CIKL)
		IR <= IB_in;

	/*
	 * FPGA change: Clock on system clock instead.
	 */
	always @(posedge clk) begin
		if (MCL)
			MPC <= 0;
		else if (MPCLD && MPCKL)
			MPC <= mpcmux;
		else if (MPCKL)
			MPC <= MPC + 1;
	end

	always @(*)
		case ({MIR[31], MPAS0 })
		2'b00: mpcmux = { 2'b0, MAR9, MAR8, 2'b01, IR[15:11], 1'b0 };
		2'b01: mpcmux = { 1'b0, XEP10, MAR9, MAR8, 1'b1, IR[13],
			XEP5, IR[11], XEP3, XEP2, XEP1, XEP0 };
		2'b10: mpcmux = { CAR[11:10], MAR9, MAR8, CAR[7:0] };
		2'b11: mpcmux = { MIR[11:10], MAR9, MAR8, MIR[7:0] };
		endcase

	// 15A
	assign MPAS0 = ~((~IR[14] & ~(~IR[11] & IR[13]) & ~MIR[31]) |
		(~IR[15] & ~MIR[31]) |
		(~MIR[31] & ~IR[12] & IR[13]) |
		(MIR[29] & MIR[31]));

	// 18A
	assign MAR9 = ~((~OPINT & MIR[29] & ~CAR[9]) |
		(~MIR[31] & ~OPINT) |
		(~OPINT & ~MIR[29] & ~MIR[9]));

	// 18B
	assign MAR8 = ~((~INT & MIR[29] & ~CAR[8]) |
		(~MIR[31] & ~INT) |
		(~INT & ~MIR[29] & ~MIR[8]));

	wire ir6711 = (~IR[6] & ~IR[11] & ~IR[7]);
	wire ir1112 = ~(IR[11] & IR[12]);
	// 8C
	assign XEP3 = ~((IR[11] & IR[12] & ~IR[13] & ~IR[8]) |
		(~IR[10] & IR[13]) |
		(ir6711 & ~IR[12]) |
		(ir1112 & ~IR[10]));

	// 8D
	assign XEP2 = ~((IR[11] & IR[12] & ~IR[13] & ~IR[7]) |
		(~IR[9] & IR[13]) |
		(ir6711 & ~IR[12]) |
		(~IR[9] & ir1112));

	// 5D
	assign XEP1 = ~((~IR[6] & ~IR[7] & ~IR[11] & ~IR[12]) |
		(IR[11] & IR[12] & ~IR[13]) | (~IR[8]));

	// 18D
	assign XEP5 = (IR[12] & IR[14]);

	// 18D
	assign XEP10 = (IR[6] & ~IR[11] & ~IR[12]);

	// 18C
	assign XEP0 = ~(~(IR[13] & IR[11] & IR[7]) & ~(IR[7] & ~IR[12]));

endmodule
