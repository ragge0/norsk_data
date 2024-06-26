#!/usr/bin/awk -f
#
# Copyright (c) 2024 Anders Magnusson (ragge@ludd.ltu.se).
# All rights reserved. 
#       
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:     
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#              
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Awk version of the Nord-10 MICMAC program.
# Compiles a microcode program and generate hex output.
#

BEGIN {
	if (ARGC != 2) {
		print "Usage: micmac <infile>" >> "/dev/stderr";
		exit(1);
	}
	infil = ARGV[1];

	ROMSZ = 4096;

	for (i = 0; i < ROMSZ; i++)
		mem[i] = "00000000000000000000000000000000";

	setup();

	pos = 0;
	while ((getline < infil) > 0) {

		gotcmd = 0;
		for (i = 1; i <= NF; i++) {
			s = tolower($i);
			if (s ~ /^%/)
				break;

			if (i == 2 && s ~ /[0-9a-f]+/)
				continue; 
			if (s ~ /[0-7]+:/) {
				pos = o2n(s);	# .org
			} else if (s ~ /[A-Za-z],$/) {
				lblpos[substr(s, 1, length(s)-1)] = pos;
			} else if (s in cmds) {
				mergecmd(cmds[s], pos);
				gotcmd = 1;
			} else if (s ~ /^[0-7]+/) {
				gotcmd = 1;
				mergecmd(o2bin(s), pos);
			} else {
				gotcmd = 1;
				reloctbl[s] = pos;
			}
		}
		if (gotcmd)
			pos++;
	}
	close(infil);

	print "Read " pos " command lines" >> "/dev/stderr";
	ROMSZ = pos;

	for (i in reloctbl) {
		if (i in lblpos) {
			mergecmd(o2bin(lblpos[i]), reloctbl[i]);
			delete reloctbl[i];
		}
	}

	for (i in reloctbl) {
		print i " missing in label table";
		e = 1;
	}

	if (e) error("errors...");

	for (i = 0; i < ROMSZ; i++) {
		for (j = 1; j < 32; j += 4) {
			s = substr(mem[i], j, 4);
			n = 0;
			if (substr(s, 1, 1) == "1") n = 8;
			if (substr(s, 2, 1) == "1") n += 4;
			if (substr(s, 3, 1) == "1") n += 2;
			if (substr(s, 4, 1) == "1") n += 1;
			printf "%X",n;
		}
		printf "\n";
	}
}


function o2n(n,    r, s, i) {
	r = 0;
	for (i = 1; i <= length(n); i++) {
		s = substr(n, i, 1);
		if (s == "0") r *= 8;
		else if (s == "1") r = r * 8 + 1;
		else if (s == "2") r = r * 8 + 2;
		else if (s == "3") r = r * 8 + 3;
		else if (s == "4") r = r * 8 + 4;
		else if (s == "5") r = r * 8 + 5;
		else if (s == "6") r = r * 8 + 6;
		else if (s == "7") r = r * 8 + 7;
		else break;
	}
	return r;
}

function mergecmd(mnum, p,    i, c1, c2, res) {
	if (length(mnum) != 32)
		error("mergecmd: wrong len: " length(mnum));
	res = "";
	for (i = 1; i <= 32; i++) {
		c1 = substr(mnum, i, 1);
		c2 = substr(mem[p], i, 1);
		if (c1 == "1" && c2 == "1")
			warn("double one pos " i " p " p " mnum " mnum " mem " mem[p]);
		if (c1 == "1")
			c2 = "1";
		res = res c2;
	}
	mem[p] = res;
	if (length(mem[p]) != 32)
		error("mergecmd: wrong len: " length(mem[p]));
}

# Get a number.  Return a 32-bit string of binary digits.
function o2bin(oct,    i, n, o) {
	n = "";
	while (length(oct) > 0) {
		o = substr(oct, 1, 1);
		oct = substr(oct, 2);
		if (o == "0") n = n "000";
		if (o == "1") n = n "001";
		if (o == "2") n = n "010";
		if (o == "3") n = n "011";
		if (o == "4") n = n "100";
		if (o == "5") n = n "101";
		if (o == "6") n = n "110";
		if (o == "7") n = n "111";
	}
	while (length(n) < 32)
		n = "0" n;
	return n;
}

function error(e,   n) {
	printf "Line %o; %s\n", pos, $0 >> "/dev/stderr";
	print e >> "/dev/stderr";
	exit(1);
}

function warn(e,   n) {
	printf "Line %o; %s\n", pos, $0 >> "/dev/stderr";
	print "warning: " e >> "/dev/stderr";
}

function setup() {
	cmds[",car"] = "00100000000000000000000000000000";
	cmds["arl"] = "00000000000000000000000000000000";
	cmds["arm"] = "00000001000000000000000000000000";
	cmds["carl"] = "00000000000000001000000000000000";
	cmds["carm"] = "00000001000000001000000000000000";
	cmds["iarm"] = "01000001000000000000000000000000";
	cmds["chlev"] = "00000000000100000000000000000000";
	cmds["ceatr"] = "00000000001000000000000000000000";
	cmds["cptr"] = "00000000010000000000000000000000";
	cmds["cfc"] = "00000000011000000000000000000000";
	cmds["cwr1"] = "00000000100000000000000000000000";
	cmds["cw"] = "00000000101000000000000000000000";
	cmds["crr1"] = "00000000110000000000000000000000";
	cmds["cr"] = "00000000111000000000000000000000";
	cmds["jmp"] = "10000000000000000000000000000000";
	cmds["cjmp"] = "10000000000000001000000000000000";
	cmds["priv"] = "00010000000000000000000000000000";
	cmds["logl"] = "00100000000000000000000000000000";
	cmds["logm"] = "00100001000000000000000000000000";
	cmds["clogm"] = "00100001000000001000000000000000";
	cmds["ilogm"] = "01100001000000000000000000000000";
	cmds["loop"] = "11000000000000000000000000000000";
	cmds["zero"] = "00000000000000000000000000000000";
	cmds["spos"] = "00000000000000000001000000000000";
	cmds["pos"] = "00000000000000000010000000000000";
	cmds["posm"] = "00000000000000000011000000000000";
	cmds["nzero"] = "00000000000000000100000000000000";
	cmds["sneg"] = "00000000000000000101000000000000";
	cmds["neg"] = "00000000000000000110000000000000";
	cmds["negm"] = "00000000000000000111000000000000";
	cmds["bdirc"] = "00000000000000000000000000000000";
	cmds["andc"] = "00000010000000000000000000000000";
	cmds["orcb"] = "00000100000000000000000000000000";
	cmds["one"] = "00000110000000000000000000000000";
	cmds["orc"] = "00001000000000000000000000000000";
	cmds["adirc"] = "00001010000000000000000000000000";
	cmds["exorc"] = "00001100000000000000000000000000";
	cmds["orca"] = "00001110000000000000000000000000";
	cmds["andcb"] = "00010000000000000000000000000000";
	cmds["exor"] = "00010010000000000000000000000000";
	cmds["adir"] = "00010100000000000000000000000000";
	cmds["or"] = "00010110000000000000000000000000";
	cmds["zro"] = "00011000000000000000000000000000";
	cmds["andca"] = "00011010000000000000000000000000";
	cmds["and"] = "00011100000000000000000000000000";
	cmds["bdir"] = "00011110000000000000000000000000";
	cmds["bm1"] = "00000000000000000000000000000000";
	cmds["bm1a"] = "00000000000000000000000000000000";
	cmds["plusa"] = "00000000000100000000000000000000";
	cmds["plus"] = "00000010000000000000000000000000";
	cmds["plus9"] = "00010010000000000000000000000000";
	cmds["bmam1"] = "00000100000000000000000000000000";
	cmds["bdi"] = "00000110000000000000000000000000";
	cmds["bdia"] = "00000000001100000000000000000000";
	cmds["add1"] = "00011000000000000000000000000000";
	cmds["bmina"] = "00011100000000000000000000000000";
	cmds["bmaa"] = "00000000111000000000000000000000";
	cmds["addc"] = "00001000000000000000000000000000";
	cmds["orin"] = "00000000000000011000000000000000";
	cmds["orin2"] = "00000000000000111000000000000000";
	cmds["orbwo"] = "00000000000000010000000000000000";
	cmds["orbw"] = "00000000000000110000000000000000";
	cmds["orskp"] = "00000000000001000000000000000000";
	cmds["orrop"] = "00000000000001010000000000000000";
	cmds["orsw2"] = "00000000000001100000000000000000";
	cmds["orsw3"] = "00000000000001110000000000000000";
	cmds["a,d"] = "00000000000000000000000000000001";
	cmds["a,p"] = "00000000000000000000000000000010";
	cmds["a,b"] = "00000000000000000000000000000011";
	cmds["a,l"] = "00000000000000000000000000000100";
	cmds["a,a"] = "00000000000000000000000000000101";
	cmds["a,t"] = "00000000000000000000000000000110";
	cmds["a,x"] = "00000000000000000000000000000111";
	cmds["a,s"] = "00000000000000000000000000001000";
	cmds["a,dh"] = "00000000000000000000000000001001";
	cmds["a,io"] = "00000000000000000000000000001010";
	cmds["a,h"] = "00000000000000000000000000001011";
	cmds["a,scr"] = "00000000000000000000000000001100";
	cmds["a,r"] = "00000000000000000000000000001101";
	cmds["a,sp"] = "00000000000000000000000000001110";
	cmds["a,ss"] = "00000000000000000000000000001111";
	cmds["b,pas"] = "00000000000000000000000000000000";
	cmds["b,s"] = "00000000000000000000000000010000";
	cmds["b,opr"] = "00000000000000000000000000100000";
	cmds["b,pgs"] = "00000000000000000000000000110000";
	cmds["b,pvl"] = "00000000000000000000000001000000";
	cmds["b,iic"] = "00000000000000000000000001010000";
	cmds["b,pid"] = "00000000000000000000000001100000";
	cmds["b,pie"] = "00000000000000000000000001110000";
	cmds["b,dpil"] = "00000000000000000000000010010000";
	cmds["b,ald"] = "00000000000000000000000010100000";
	cmds["b,pes"] = "00000000000000000000000010110000";
	cmds["b,mpc"] = "00000000000000000000000011000000";
	cmds["b,pea"] = "00000000000000000000000011010000";
	cmds["b,bio"] = "00000000000000000000000011100000";
	cmds["b,bir3"] = "00000000000000000000000011110000";
	cmds["b,z"] = "00000000000000000000000000000000";
	cmds["b,d"] = "00000000000000000000000000010000";
	cmds["b,p"] = "00000000000000000000000000100000";
	cmds["b,b"] = "00000000000000000000000000110000";
	cmds["b,l"] = "00000000000000000000000001000000";
	cmds["b,a"] = "00000000000000000000000001010000";
	cmds["b,t"] = "00000000000000000000000001100000";
	cmds["b,x"] = "00000000000000000000000001110000";
	cmds["b,sc"] = "00000000000000000000000010000000";
	cmds["b,sh"] = "00000000000000000000000010010000";
	cmds["b,io"] = "00000000000000000000000010100000";
	cmds["b,bm"] = "00000000000000000000000010100000";
	cmds["b,ac"] = "00000000000000000000000010110000";
	cmds["b,hac"] = "00000000000000000000000011000000";
	cmds["b,2ac"] = "00000000000000000000000011010000";
	cmds["b,pac"] = "00000000000000000000000000000000";
	cmds["b,s"] = "00000000000000000000000000010000";
	cmds["b,lmp"] = "00000000000000000000000000100000";
	cmds["b,pcr"] = "00000000000000000000000000110000";
	cmds["b,mis"] = "00000000000000000000000001000000";
	cmds["b,pid"] = "00000000000000000000000001100000";
	cmds["b,pie"] = "00000000000000000000000001110000";
	cmds["b,car"] = "00000000000000000000000010110000";
	cmds["b,ir"] = "00000000000000000000000011000000";
	cmds["b,bio"] = "00000000000000000000000011100000";
	cmds["b,ir3"] = "00000000000000000000000011110000";
	cmds["b,b0"] = "00000000000000000000000010100000";
	cmds["b,b1"] = "00000000000000000001000010100000";
	cmds["b,b2"] = "00000000000000000010000010100000";
	cmds["b,b3"] = "00000000000000000011000010100000";
	cmds["b,b4"] = "00000000000000000100000010100000";
	cmds["b,b5"] = "00000000000000000101000010100000";
	cmds["b,b6"] = "00000000000000000110000010100000";
	cmds["b,b7"] = "00000000000000000111000010100000";
	cmds["b,b10"] = "00000000000000001000000010100000";
	cmds["b,b11"] = "00000000000000001001000010100000";
	cmds["b,b12"] = "00000000000000001010000010100000";
	cmds["b,b13"] = "00000000000000001011000010100000";
	cmds["b,b14"] = "00000000000000001100000010100000";
	cmds["b,b15"] = "00000000000000001101000010100000";
	cmds["b,b16"] = "00000000000000001110000010100000";
	cmds["b,b17"] = "00000000000000001111000010100000";
	cmds["d,d"] = "00000000000000000000000100000000";
	cmds["d,p"] = "00000000000000000000001000000000";
	cmds["d,b"] = "00000000000000000000001100000000";
	cmds["d,l"] = "00000000000000000000010000000000";
	cmds["d,a"] = "00000000000000000000010100000000";
	cmds["d,t"] = "00000000000000000000011000000000";
	cmds["d,x"] = "00000000000000000000011100000000";
	cmds["d,s"] = "00000000000000000000100000000000";
	cmds["d,sh"] = "00000000000000000000100100000000";
	cmds["d,io"] = "00000000000000000000101000000000";
	cmds["d,sc"] = "00000000000000000000101100000000";
	cmds["d,scr"] = "00000000000000000000110000000000";
	cmds["d,sp"] = "00000000000000000000111000000000";
	cmds["d,ss"] = "00000000000000000000111100000000";
	cmds["shr"] = "00000000000000001000000000000000";
	cmds["shro"] = "00000000000000000010000000000000";
	cmds["tsc0"] = "00000000000000000000000000000000";
	cmds["tsh31"] = "00000000000000000000000000001000";
	cmds["tsh22"] = "00000000000000000000000000001100";
	cmds["orsht"] = "00000000000001000000010000000000";
	cmds["sh32"] = "00000000000000000001000000000000";
	cmds["shro"] = "00000000000000000010000000000000";
	cmds["shze"] = "00000000000000000100000000000000";
	cmds["le0"] = "00000000000000000000000000000000";
	cmds["le1"] = "00000000000000000001000000000000";
	cmds["le2"] = "00000000000000000010000000000000";
	cmds["le3"] = "00000000000000000011000000000000";
	cmds["le4"] = "00000000000000000100000000000000";
	cmds["le5"] = "00000000000000000101000000000000";
	cmds["le6"] = "00000000000000000110000000000000";
	cmds["le7"] = "00000000000000000111000000000000";
	cmds["le10"] = "00000000000000001000000000000000";
	cmds["le11"] = "00000000000000001001000000000000";
	cmds["le12"] = "00000000000000001010000000000000";
	cmds["le13"] = "00000000000000001011000000000000";
	cmds["le14"] = "00000000000000001100000000000000";
	cmds["le15"] = "00000000000000001101000000000000";
	cmds["le16"] = "00000000000000001110000000000000";
	cmds["le17"] = "00000000000000001111000000000000";
	cmds["saco"] = "00000000000010000000000000000000";
	cmds["tgsh0"] = "00000000000000000000000100000000";
	cmds["tgac0"] = "00000000000000000000001000000000";
	cmds["tgs0"] = "00000000000000000000001100000000";
	cmds["ash31"] = "00000000000000000000000000000000";
	cmds["ash0"] = "00000000000000000000000000000001";
	cmds["endid"] = "00000000000000000000000000000010";
	cmds["dspl"] = "00000000000100000000000000000000";
}
