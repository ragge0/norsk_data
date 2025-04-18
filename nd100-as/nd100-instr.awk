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
# sd	HARD	0000010	ROPREG
# sp	HARD	0000020	ROPREG
# sb	HARD	0000030	ROPREG
# sl	HARD	0000040	ROPREG
# sa	HARD	0000050	ROPREG
# st	HARD	0000060	ROPREG
# sx	HARD	0000070	ROPREG
# dd	HARD	0000001	ROPREG
# dp	HARD	0000002	ROPREG
# db	HARD	0000003	ROPREG
# dl	HARD	0000004	ROPREG
# da	HARD	0000005	ROPREG
# dt	HARD	0000006	ROPREG
# dx	HARD	0000007	ROPREG
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
# jmp	HARD	0152000	EA
# jpl	HARD	0152000	EA
## Conditional jump
# jap	HARD	0130000	OFF
# jan	HARD	0130400	OFF
# jaz	HARD	0131000	OFF
# jaf	HARD	0131400	OFF
# jxn	HARD	0133400	OFF
# jxz	HARD	0133000	OFF
# jpc	HARD	0132000	OFF
# jnc	HARD	0132400	OFF
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
