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
 * INTERRUPT REGISTERS, card 1008.
 */

module int_regs (
	input			clk,
	input			MCL,
	input			TRR,
	input			TR6,
	input			TR7,
	input			PILKL,
	input [15:0]		IB,

	output [15:0]		IB_ut
);

	/*
	 * PID/PIE registers
	 */
	wire PEKL = TRR & TR7;
	wire PDKL = TRR & TR6;

	reg [15:0]	PID, PIE;
	always @(posedge clk) begin
		if (PEKL)
			PIE <= IB;
		if (PDKL)
			PID <= IB;	// XXX add special handling for 10-15
	end

	/*
	 * Priority encoding and clocking of PIL register.
	 */
	wire [15:0] pidpie3 = PID & PIE;
	wire PK3 = |pidpie3[15:8];
	wire [7:0] pidpie2 = PK3 ? pidpie3[15:8] : pidpie3[7:0];
	wire PK2 = |pidpie2[7:4];
	wire [3:0] pidpie1 = PK2 ? pidpie2[7:4] : pidpie2[3:0];
	wire PK1 = |pidpie1[3:2];
	wire [1:0] pidpie0 = PK1 ? pidpie1[3:2] : pidpie1[1:0];
	wire PK0 = pidpie0[1];

	reg [3:0]	PIL, PVL;
	always @(posedge clk) begin
		if (PILKL) begin
			PVL <= PIL;
			PIL <= { PK3, PK2, PK1, PK0 };
		end
	end

endmodule
