##	$Id: instr.awk,v 1.1 2022/11/12 16:23:44 ragge Exp $
##
##	Instruction definitions for nd100.
##
##	Generate instruction definitions for different utilities.
##	It is modeled after the script that were supplied with 4.2BSD.
##
##	THIS FILE IS BOTH AN AWK SCRIPT AND THE DATA
##
##	Usage is as follows;
##	cat instr.awk | awk -v flavor=AS -f instr.awk > instr.h
##	cat instr.awk | awk -v flavor=ADB -f instr.awk > instr.h
##
##
##	Field	Description
##
##	$2	Instruction name
##
##	$3	Type (HARD, SOFT, REG, DIREC)
##
##	$4	Opcode/number/...
##
##	$5	Instruction class (ACE,SSDD,...)
##
## Code that prints out the instructions.
##
{
	if (NF == 0 || $1 != "#")
		next;
	if (flavor == "AS" ) {
		pre="";
		if ($3 == "SOFT")
			pre="B_";
		if ($3 == "HARD" || $3 == "DIREC")
			pre="A_";
		if ($3 == "SKIP")
			$5 = $3;
		if ($3 == "DEV")
			$5 = $3;
		printf "OPC(\"%s\",%s%s,%s)\n",$2,pre,$5,$4;
	}
	if (flavor == "ADB" ) {
		pre="";
		if ($3 == "SOFT")
			next;
		if ($3 == "HARD")
			pre="A_";
		if ($3 == "DEV")
			next;
		if ($3 == "SKIP")
			next;
		if ($3 == "DIREC") {
			next;
		}
		printf "OPC(\"%s\",%s%s,%s)\n",$2,pre,$5,$4;
	}
}
## Store instructions
# stz	HARD	0000000	EA
# sta	HARD	0004000	EA
# stt	HARD	0010000	EA
# stx	HARD	0014000	EA
# min	HARD	0040000	EA
## Load instructions
# lda	HARD	0044000	EA
# ldt	HARD	0050000	EA
# ldx	HARD	0054000	EA
## Arithmetical/Logical instructions
# add	HARD	0060000	EA
# sub	HARD	0064000	EA
# and	HARD	0070000	EA
# ora	HARD	0074000	EA
# mpy	HARD	0120000	EA
## Double word instructions
# std	HARD	0020000	EA
# ldd	HARD	0024000	EA
## Floating instructions
# stf	HARD	0030000	EA
# ldf	HARD	0034000	EA
# fad	HARD	0100000	EA
# fsb	HARD	0104000	EA
# fmu	HARD	0110000	EA
# fdv	HARD	0114000	EA
## Byte instructions
# sbyt	HARD	0142600	NOARG
# lbyt	HARD	0142200	NOARG
## Register operations
# radd	HARD	0146000	ROP
# rsub	HARD	0146600	ROP
# copy	HARD	0146100	ROP
# swap	HARD	0144000	ROP
# rand	HARD	0144400	ROP
# rexp	HARD	0145000	ROP
# rora	HARD	0145400	ROP
## ROP arguments
# ad1	HARD	0000400	ROPARG
# adc	HARD	0001000	ROPARG
# cld	HARD	0000100	ROPARG
# cm1	HARD	0000200	ROPARG
## ROP registers
# sd	HARD	0000010	ROPSREG
# sp	HARD	0000020	ROPSREG
# sb	HARD	0000030	ROPSREG
# sl	HARD	0000040	ROPSREG
# sa	HARD	0000050	ROPSREG
# st	HARD	0000060	ROPSREG
# sx	HARD	0000070	ROPSREG
# dd	HARD	0000001	ROPDREG
# dp	HARD	0000002	ROPDREG
# db	HARD	0000003	ROPDREG
# dl	HARD	0000004	ROPDREG
# da	HARD	0000005	ROPDREG
# dt	HARD	0000006	ROPDREG
# dx	HARD	0000007	ROPDREG
## Combined rop insn
# exit	HARD	0146142	NOARG
# rclr	HARD	0146100	ROP
# rinc	HARD	0146400	ROP
# rdcr	HARD	0146200	ROP
## Extended ROP
# rmpy	HARD	0141200	ROP
# rdiv	HARD	0141600	ROP
# mix3	HARD	0143200	NOARG
# exr	HARD	0140600	ROP
## Shift instructions
# sht	HARD	0154000	SHFT
# shd	HARD	0154200	SHFT
# sha	HARD	0154400	SHFT
# sad	HARD	0154600	SHFT
# rot	HARD	0001000	SHFTARG
# zin	HARD	0002000	SHFTARG
# lin	HARD	0003000	SHFTARG
# shr	HARD	0000000	SHFTR
## Float conv
# nlz	HARD	0151400	FCONV
# dnz	HARD	0152000	FCONV
## Jump
# jmp	HARD	0124000	EA
# jpl	HARD	0134000	EA
## Conditional jump
# jap	HARD	0130000	OFF
# jan	HARD	0130400	OFF
# jaz	HARD	0131000	OFF
# jaf	HARD	0131400	OFF
# jxn	HARD	0133400	OFF
# jxz	HARD	0133000	OFF
# jpc	HARD	0132000	OFF
# jnc	HARD	0132400	OFF
## Bit instructions
# bskp	HARD	0175000	BSKP
# bset	HARD	0174000	BSKP
# zro	HARD	0000000	BSKPARG
# one	HARD	0000200	BSKPARG
# bcm	HARD	0000400	BSKPARG
# bac	HARD	0000600	BSKPARG
## One-bit AC instructions
# bsta	HARD	0176200	OBA
# bstc	HARD	0176000	OBA
# blda	HARD	0176600	OBA
# bldc	HARD	0176400	OBA
# banc	HARD	0177200	OBA
# borc	HARD	0177400	OBA
# band	HARD	0177200	OBA
# bora	HARD	0177600	OBA
## skip
# skp	HARD	0140000	SKP
# eql	HARD	0000000	SKPARG
# geq	HARD	0000400	SKPARG
# gre	HARD	0001000	SKPARG
# mgre	HARD	0001400	SKPARG
# ueq	HARD	0002000	SKPARG
# lss	HARD	0002400	SKPARG
# lst	HARD	0003000	SKPARG
# mlst	HARD	0003400	SKPARG
## Argument instructions
# saa	HARD	0170400	OFF
# aaa	HARD	0172400	OFF
# sax	HARD	0171400	OFF
# aax	HARD	0173400	OFF
# sat	HARD	0171000	OFF
# aat	HARD	0173000	OFF
# sab	HARD	0170000	OFF
# aab	HARD	0172000	OFF
## Privileged instructions
# iof	HARD	0150401	NOARG
# ion	HARD	0150402	NOARG
# piof	HARD	0150405	NOARG
# pion	HARD	0150412	NOARG
# pof	HARD	0150404	NOARG
# pon	HARD	0150410	NOARG
# lwcs	HARD	0143500	NOARG
# wait	HARD	0151000	NOARG
# ident	HARD	0143600	IDENT
# iox	HARD	0164000	IOX
# ioxt	HARD	0150415	NOARG
# tra	HARD	0150000	TRARG
# trr	HARD	0150100	TRARG
# mcl	HARD	0150200	TRARG
# mst	HARD	0150300	TRARG
# lrb	HARD	0152600	NOARG
# srb	HARD	0152402	NOARG
# irw	HARD	0153400	IRARG
# irr	HARD	0153600	IRARG
# rex	HARD	0150407	NOARG
# sex	HARD	0150406	NOARG
# exam	HARD	0150416	NOARG
# depo	HARD	0150417	NOARG
# opcom	HARD	0150400	NOARG
##
# pans	HARD	000000	TRREG
# panc	HARD	000000	TRREG
# sts	HARD	000001	TRREG
# opr	HARD	000002	TRREG
# lmp	HARD	000002	TRREG
# psr	HARD	000003	TRREG
# pcr	HARD	000003	TRREG
# pvl	HARD	000004	TRREG
# iic	HARD	000005	TRREG
# iie	HARD	000005	TRREG
# pid	HARD	000006	TRREG
# pie	HARD	000007	TRREG
# csr	HARD	000010	TRREG
# ccl	HARD	000010	TRREG
# actl	HARD	000011	TRREG
# lcil	HARD	000011	TRREG
# ald	HARD	000012	TRREG
# ucil	HARD	000012	TRREG
# pes	HARD	000013	TRREG
# cilp	HARD	000013	TRREG
# pgc	HARD	000014	TRREG
# pea	HARD	000015	TRREG
# eccr	HARD	000015	TRREG
# cs	HARD	000017	TRREG
##
# pl10	HARD	0000004	IDARG
# pl11	HARD	0000011	IDARG
# pl12	HARD	0000022	IDARG
# pl13	HARD	0000043	IDARG
## Extended byte operations
# bfill	HARD	0140130	NOARG
# movb	HARD	0140131	NOARG
# movbf	HARD	0140132	NOARG
## Physical memory read/write
# ldatx	HARD	0143300	PMRW
# ldxtx	HARD	0143301	PMRW
# lddtx	HARD	0143302	PMRW
# ldbtx	HARD	0143303	PMRW
# statx	HARD	0143304	PMRW
# stztx	HARD	0143305	PMRW
# stdtx	HARD	0143306	PMRW
## Decimal instructions
# addd	HARD	0140120	NOARG
# subd	HARD	0140121	NOARG
# comd	HARD	0140122	NOARG
# pack	HARD	0140124	NOARG
# upack	HARD	0140125	NOARG
# shde	HARD	0140126	NOARG
## Stack instructions
# init	HARD	0140134	NOARG
# entr	HARD	0140135	NOARG
# leave	HARD	0140136	NOARG
# eleav	HARD	0140137	NOARG
## Other instructions
# movew	HARD	0143100	MOVEW
# tset	HARD	0140123	NOARG
# rdus	HARD	0140127	NOARG
## Sintran III Segment Change Instructions
# setpt	HARD	0140300	NOARG
# clept	HARD	0140301	NOARG
# clnreent	HARD	0140302	NOARG
# chreentpages	HARD	0140303	NOARG
# clepu	HARD	0140304	NOARG
