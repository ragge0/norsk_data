/*	$Id: nd100s.v,v 1.2 2023/11/27 17:04:04 ragge Exp $	*/
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
 * Nord10/S
 *
 * Top level of the Nord10/S implementation.
 * This is the cpu card crate.
 * Cards are placed in the same order as in a real machine.
 */
`timescale 1ns / 1ps
`default_nettype none

/*
 * The memory address bus is 32 bit, 32 bit wide.
 */

module nord10s (
	input		clk,
	input		n_rst

`ifdef notyet
	output		sdram_clk,
	output		sdram_cke,
	output		sdram_cs,
	output		sdram_ras,
	output		sdram_cas,
	output		sdram_we,
	output		sdram_udqm,
	output		sdram_ldqm,
	output [11:0]	sdram_addr,
	output [1:0]	sdram_ba,
	inout  [15:0]	sdram_data,

	output		buzzer,

	input		tti_rx,
	output		tto_tx
`endif
);
	/*
	 * Backplane connections, named the same as in the 
	 * original documentation.
	 */
	/* THESE ARE CORRECT */
	wire		MCL;
	wire		DBL, ROMDRY, MIRDRY, ACTLOOP, MPCKL, MIRKL;
	wire		MPCLD, ACR16, BCR16, ASH0, ASH15, AZERO, BZERO;
	wire		AC0, AC15, ASHX, ASHM, AAKL, BAKL, ACKL, BCKL;
	wire		ACR0, BCR0, BSH0, BSH15, BC0, BC15, BSHX, BSHM;
	wire		SHKL, BCR16, SKL, BSH6, STSRD, BAKL, WSHC, SHCKL;
	wire		SI, WREN, IODEST, TERM, TRR, TRA, TR6, TR7;
	wire		BMSRC, PILKL, WSCR;
	wire		T3_ic, T3_dec;
	wire [31:0]	ROM, MIR;
	wire [15:0]	MPC, ASUM, BSUM;
	wire [11:0]	MPC;
	wire [7:0]	STS;
	wire [5:0]	SHC;
	wire [3:0]	LEV;
	wire [2:0]	SL;
	wire [1:0]	ASHS, BSHS;

	/* THESE ARE MISSING INITIALIZATION */
	/* different interrupt lines */
	wire		PRIVINT = 0, INT = 0, OPCOM = 0, OPINT = 0;
	wire		ALT = 0, SIR9 = 0, CIKL = 0, HSEL = 0, HX = 0;
	wire		AEN = 0, HKL = 0, LSEL = 0, MOPC = 0;
	wire		RKL = 0, RLD = 0, CPCLR = 0, CPLD = 0, CPKL = 0;
	wire [15:0]	IR = 0;
	wire [3:0]	PIL = 0;

	/* THESE ARE WITHOUT ENDPOINTS */
	wire		T1, T5, ROMRQ;
	wire		ARM;
	wire [3:0]	S;
	wire [1:0]	CRSEL;
	wire [15:0]	AA, BB, R;
wire [3:0] state;

	/* HELPER WIRES */
	wire [15:0]	MIR15_0; // from OR LOGIC
	wire [31:19]	MIR31_19;
	assign MIR = { MIR31_19, 3'b0, MIR15_0 };

`ifndef notdef
	/* printout for debugging */
	always @(posedge clk)
		if (state > 2)
		$fdisplay(32'h80000002,
    "state=%d MPC=%o ROM=%x MIR=%x SUM %o WREN %d WSCR %d\r",
		state, MPC, ROM, MIR, BSUM, WREN, WSCR);
`endif

	/* "OR" for IB */
	wire [15:0]	IB, IB_mic, IB_regs, IB_status, IB_intregs;
	assign IB = IB_mic | IB_regs | IB_status | IB_intregs;


	/*
	 * Initial reset (MCL) logic.
	 * Keep MCL high first four clock pulses.
	 * Make extern reset signal clocked.
	 */
	reg [1:0]	mclcnt;
	reg		mclrst;
	initial mclcnt = 2'b10;
	always @(posedge clk) begin
		mclcnt <= { 1'b0, mclcnt[1] };
		mclrst <= ~n_rst;
	end
	wire MCL = |mclcnt | mclrst;


	/* card #1, Status logic, 1062 */ 
	status status (.clk(clk),
		.CRSEL(CRSEL), .MIR(MIR), .ACR16(ACR16), .BCR16(BCR16),
		.LEV(LEV), .BSUM7_0(BSUM[7:0]), .BSUM15(BSUM[15]), .SKL(SKL), 
		.ASH0(ASH0), .ASH15(ASH15), .AZRO(AZERO), .BZRO(BZERO),
		.AC0(AC0), .BSH0(BSH0), .BSH6(BSH6), .STSRD(STSRD),
		.BSH15(BSH15), .BC15(BC15), .BAKL(BAKL), .WSHC(WSHC),
		.SHCKL(SHCKL),

		.TERM(TERM),
		.SHC(SHC), .ASHX(ASHX), .ASHM(ASHM), .IB_ut(IB_status),
		.BSHM(BSHM), .BSHX(BSHX), .STS(STS), .ACR0(ACR0), .BCR0(BCR0));

	/* card #2, Arithmetic B, 1001 */
	arithmetic ar_b(.clk(clk),
		.C(BCR0), .AA(AA), .BB(BB), .ACKL(BCKL), .AKL(BAKL),
		.SHC(SHC), .SHS(BSHS), .SHKL(SHKL), .SHM(BSHM),
		.SHX(BSHX), .SL(SL), .S(S), .M(ARM), .BC15(AC15),
		.MIR15_12(MIR[15:12]),

		.AC15(BC15), .AC0(BC0), .ZERO(BZERO), .SH6(BSH6),
		.SH15(BSH15), .SH0(BSH0), .CO(BCR16), .SUM(BSUM));

	/* card #3, Arithmetic A, 1001 */
	arithmetic ar_a(.clk(clk),
		.C(ACR0), .AA(AA), .BB(BB), .ACKL(ACKL), .AKL(AAKL),
		.SHC(SHC), .SHS(ASHS), .SHKL(SHKL), .SHM(ASHM),
		.SHX(ASHX), .SL(SL), .S(S), .M(ARM), .BC15(BC0),

		.MIR15_12(MIR[15:12]),
		.AC15(AC15), .AC0(AC0), .ZERO(AZERO),
		.SH15(ASH15), .SH0(ASH0), .CO(ACR16), .SUM(ASUM));

	/* card #4-7, Registers, 1002 */
	regs regs(.clk(clk),
		.ASUM(ASUM), .BSUM(BSUM), .LEV(LEV), .MIR6_0(MIR[6:0]),
		.ASEL(MIR[24]), .AEN(AEN), .HKL(HKL), .HSEL(HSEL), .HX(HX),
		.IB_in(IB), .RLD(RLD), .RKL(RKL), .CPCLR(CPCLR),
		.STS(STS), .CPLD(CPLD), .CPKL(CPKL), 

		.WSCR(WSCR), .R(R), .IB_ut(IB_regs), .AA(AA), .BB(BB));

	/* card #8, Decoding, 1007 */
	decoding decoding(.clk(clk), .MCL(MCL),
		.ALT(ALT), .SIR9(SIR9), .T1(T1), .T5(T5),
		.MIRKL(MIRKL), .ROM(ROM), .ASHS(ASHS), .BSHS(BSHS),
		.MIR11_8(MIR[11:8]), .BMSRC(BMSRC), .T3(T3_dec),

		.SI(SI), .WSCR(WSCR),
		.MIR31_19(MIR31_19), .SKL(SKL), .IODEST(IODEST), .WREN(WREN),
		.DBL(DBL), .ARM(ARM), .CRSEL(CRSEL), .S(S));

	/* card #9, Or logic, 1006 */
	or_logic or_logic(.clk(clk), .MCL(MCL),
		.ROM(ROM), .IR(IR), .MOPC(MOPC), .PIL(PIL), .LSEL(LSEL),
		.MIRKL(MIRKL),

		.BMSRC(BMSRC), .SL(SL), .LEV(LEV), .MIR15_0(MIR15_0));

	/* card #10, Microcode PROM 4k, 1167 */
	prom4k prom4k(.clk(clk),
		.ROMRQ(ROMRQ), .MPC(MPC),

		.ROM(ROM), .ROMDRY(ROMDRY));

	/* card #11, EPG calculation and microcode PC, 1075 */
	microaddressing microaddressing(.clk(clk), .MCL(MCL),
		.MPCLD(MPCLD), .MPCKL(MPCKL), .CIKL(CIKL), .IB_in(IB),
		.MIR(MIR), .STS0(STS[0]),

		.IB_ut(IB_mic), .IR(IR), .MPC(MPC));

	/* card #12, Timing logic (based on 1120) */
	timing timing(.clk(clk), .MCL(MCL),
	    .PRIVINT(PRIVINT), .INT(INT), .OPCOM(OPCOM), .OPINT(OPINT),
	    .ROM3130(ROM[31:30]), .MIR(MIR), .DBL(DBL), .ROMDRY(ROMDRY),
	    .ACTLOOP(ACTLOOP), .IODEST(IODEST),

	    .TR6(TR6), .TR7(TR7), .T3_ic(T3_ic), .T3_dec(T3_dec),
	    .ACKL(ACKL), .BCKL(BCKL), .SHKL(SHKL), .STSRD(STSRD), .TRR(TRR),
	    .MIRKL(MIRKL), .MPCLD(MPCLD), .AAKL(AAKL), .BAKL(BAKL),
	    .ROMRQ(ROMRQ), .T1(T1), .T5(T5), .MPCKL(MPCKL), .MIRDRY(MIRDRY),
	    .state(state));

	/* card #15, Interrupt control */
	int_ctrl int_ctrl(.clk(clk), .MCL(MCL),
		.SI(SI), .T3(T3_ic),

		.PILKL(PILKL));


	/* card #16, Interrupt registers */
	int_regs int_regs(.clk(clk), .MCL(MCL),
		.TRR(TRR), .TR6(TR6), .TR7(TR7), .IB(IB), .PILKL(PILKL),

		.IB_ut(IB_intregs));


`ifdef notyet

	/*
	 * Ram 16MB (8MW).  23 bits addr (22-0) * 16 bits data.
	 */
	sdram sdram(
		.clk(clk),
		.rst(rst),

		.sdram_clk(sdram_clk),
		.sdram_cke(sdram_cke),
		.sdram_cs(sdram_cs),
		.sdram_ras(sdram_ras),
		.sdram_cas(sdram_cas),
		.sdram_we(sdram_we),
		.sdram_udqm(sdram_udqm),
		.sdram_ldqm(sdram_ldqm),
		.sdram_addr(sdram_addr),
		.sdram_ba(sdram_ba),
		.sdram_data(sdram_data),

		.ADR_I(cpu_addr),		// i
		.WE_I(cpu_write),		// i
		.DAT_I(cpu_wdata),		// i
		.STB_I(cpu_stb), 		// i
		.DAT_O(cpu_rdata),		// o
		.ACK_O(cpu_ack)			// o
	);

	/*
	 * Terminal 1 input.
	 */
	term_in term_in_1 (
		.clk(clk),
		.rst(rst),

		.io_adr(io_adr),
		.io_stb(io_stb),
		.io_ack(t1r_io_ack),
		.io_identi(t1r_identi),
		.io_idento(t1r_idento),
		.io_intrq(t1r_intrq12),

		.io_dati(io_dato),
		.io_dato(t1r_io_dati),

		.rxdata(con_rxdata),
		.rxack(con_rxack),
		.rxrdy(con_rxrdy),

		.tti_rx(tti_rx)
	);

	/*
	 * Terminal 1 output.
	 */
	term_out term_out_1 (
		.clk(clk),
		.rst(rst),

		.io_adr(io_adr),
		.io_stb(io_stb),
		.io_ack(t1t_io_ack),
		.io_identi(t1t_identi),
		.io_idento(t1t_idento),
		.io_intrq(t1t_intrq10),

		.io_dati(io_dato),
		.io_dato(t1t_io_dati),

		.txdata(con_txdata),
		.txsend(con_txsend),
		.txrdy(con_txrdy),

		.tto_tx(tto_tx)
	);

	rtc
//`ifdef VERILATOR
//		#(.COUNT(20000))
//`endif

	 rtc_1 (
		.clk(clk),
		.rst(rst),

		.io_adr(io_adr),
		.io_stb(io_stb),
		.io_ack(rtc_io_ack),

		.io_identi(rtc_identi),
		.io_idento(rtc_idento),
		.io_intrq(rtc_intrq13),
		.io_idack(rtc_idack),
		.io_idcode(rtc_idcode),

		.io_dati(io_dato),
		.io_dato(rtc_io_dati)

	);
`endif
endmodule
