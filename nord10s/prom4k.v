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
 * PROM 4K, card 1167.
 */

module prom4k (
	input			clk,
	input			ROMRQ,
	input [11:0]		MPC,

	output reg [31:0]	ROM,
	output 			ROMDRY
);

	reg [31:0]	uc_prom4k [4095:0];
	initial $readmemh("prom4k.hex", uc_prom4k, 0, 4095);

	/* Block ram is registered, hence clocking. */
	always @(posedge clk)
		if (ROMRQ)
			ROM <= uc_prom4k[MPC];

	/*
	 * Set ready one clock pulse after reading.
	 */
	always @(posedge clk)
		ROMDRY <= ROMRQ;

endmodule
