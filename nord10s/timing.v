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
 * TIMING, modelled on TIME CONTROL, card 1120.
 *
 * Rewritten to handle a common state machine in an FPGA. Quite different 
 * to how the original timing hardware works.
 */
`define	TMCL	4'd0
`define	TFETCH	4'd1
`define	TDECODE	4'd2
`define	TARITH	4'd3
`define	TIBLOCK	4'd4
`define	TJUMP	4'd5
`define	TLOOP	4'd6
`define	TRR1	4'd7
`define	TAR1	4'd8
`define	TAR2	4'd9
`define	TFAIL	4'd15

module timing (
	input		clk,
	input		MCL,
	input [31:30]	ROM3130,
	input [31:0]	MIR,
	input [3:0]	IR3_0,
	input		PRIVINT,
	input		INT,
	input		OPCOM,
	input		OPINT,
	input		DBL,
	input		ROMDRY,
	input		IODEST,

	output		T3_dec,
	output		T3_ic,
	output		AEN,
	output		TR0,
	output		TR1,
	output		TR2,
	output		TR3,
	output		TR4,
	output		TR5,
	output		TR6,
	output		TR7,
	output		TR8,
	output		TR9,
	output		TR10,
	output		TR11,
	output		TR12,
	output		TR13,
	output		TR14,
	output		TRRSTS,
	output		MPCRD,
	output		STSRD,
	output		TRA,
	output		TRR,
	output		SHKL,
	output		AAKL,
	output		ACKL,
	output		BAKL,
	output		BCKL,
	output		MIRDRY,
	output		ACTLOOP,
	output		MIRKL,
	output		MPCKL,
	output		MPCLD,
	output		ROMRQ,
	output		T1,
	output		T5,
	output reg [3:0] state
);

	/*
	 * Generate outputs based on states.
	 */
	assign MPCKL = (state == `TMCL || state == `TJUMP ||
		state == `TRR1 || state == `TAR2);
	assign MIRKL = (state == `TDECODE);
	assign ROMRQ = (state == `TFETCH);
	assign MPCLD = (state == `TJUMP);
	assign T3_ic = (state == `TAR2);
	assign T3_dec = (state == `TAR2);
	wire intT123 = (state == `TARITH);
	wire intT23 = (state == `TRR1);

	/* Not yet defined signals */
	assign AAKL = 0;	// XXX
	assign BAKL = 0;	// XXX
	assign ACKL = 0;	// XXX
	assign BCKL = 0;	// XXX
	assign SHKL = 0;	// XXX
	assign ACTLOOP = 0;
	assign T1 = 0;
	assign T5 = 0;
	assign MIRDRY = 1;
	wire intT1 = 0;
	wire WRITE = 0;

	initial state = `TMCL;
	always @(posedge clk) begin
		case (state)
		`TMCL: begin
			if (MCL == 0)
				state <= `TFETCH;
		end
		`TFETCH: state <= `TDECODE;
		`TDECODE: begin
			case (ROM3130)
			2'b00: state <= `TARITH;
			2'b01: state <= `TIBLOCK;
			2'b10: state <= `TJUMP;
			2'b11: state <= `TLOOP;
			endcase
		end

		`TARITH: begin
			state <= `TFAIL;
			/* type of arith? */
			if (IODEST)
				state <= `TRR1;
			else if (MIR[3:0] != 4'o12)
				state <= `TAR1;
		end

		`TAR1: begin
			// Set A/B regs here.
			state <= `TAR2;
		end
		`TAR2: begin
			// if (TRUE)
			//	writeback;
			state <= `TFETCH;
		end

		`TRR1: state <= `TFETCH;
		`TIBLOCK: state <= `TFAIL;
		`TJUMP: state <= `TFETCH;
		`TLOOP: state <= `TFAIL;
		`TFAIL: ;
		default: state <= `TMCL;
		endcase
	end

	/*
	 * Decoding of the TR addresses.
	 */
	wire [3:0] trmux;
	assign trmux = (&MIR[7:4] ? IR3_0 : MIR[7:4]);
	assign TR0 = trmux[3:0] == 4'd00;
	assign TR1 = trmux[3:0] == 4'd01;
	assign TR2 = trmux[3:0] == 4'd02;
	assign TR3 = trmux[3:0] == 4'd03;
	assign TR4 = trmux[3:0] == 4'd04;
	assign TR5 = trmux[3:0] == 4'd05;
	assign TR6 = trmux[3:0] == 4'd06;
	assign TR7 = trmux[3:0] == 4'd07;
	assign TR8 = trmux[3:0] == 4'd08;
	assign TR9 = trmux[3:0] == 4'd09;
	assign TR10 = trmux[3:0] == 4'd10;
	assign TR11 = trmux[3:0] == 4'd11;
	assign TR12 = trmux[3:0] == 4'd12;
	assign TR13 = trmux[3:0] == 4'd13;
	assign TR14 = trmux[3:0] == 4'd14;

	/*
	 * TRR/TRA reference
	 */
	assign TRR = IODEST & intT23;
	assign TRA = (MIR[3:0] == 4'o12) & ~MIR[31] & intT23;

	/*
	 * Read signals
	 */
	assign MPCRD = TRA & TR12;
	assign STSRD = TRA & TR1;
	assign TRRSTS = TRR & TR1;

	/*
	 * Most/Least A/AC clocking.
	 */
	assign AAKL = (~MIR[24] & ~MIR[31] & intT1);
	assign BAKL = (MIR[24] & ~MIR[31] & intT1);
	// assign ACKL = 
	// assign BCKL = 
	assign AEN = (WRITE | IODEST) & ~MIR[31] & intT123;

endmodule
