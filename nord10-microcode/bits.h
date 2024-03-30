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
 * Bit extraction/creation macros.
 */


#define	BIT0(x)		(((x) >> 0) & 1)
#define	BIT1(x)		(((x) >> 1) & 1)
#define	BIT2(x)		(((x) >> 2) & 1)
#define	BIT3(x)		(((x) >> 3) & 1)
#define	BIT4(x)		(((x) >> 4) & 1)
#define	BIT5(x)		(((x) >> 5) & 1)
#define	BIT6(x)		(((x) >> 6) & 1)
#define	BIT7(x)		(((x) >> 7) & 1)
#define	BIT8(x)		(((x) >> 8) & 1)
#define	BIT9(x)		(((x) >> 9) & 1)
#define	BIT10(x)	(((x) >> 10) & 1)
#define	BIT11(x)	(((x) >> 11) & 1)
#define	BIT12(x)	(((x) >> 12) & 1)
#define	BIT13(x)	(((x) >> 13) & 1)
#define	BIT14(x)	(((x) >> 14) & 1)
#define	BIT15(x)	(((x) >> 15) & 1)
#define	BIT29(x)	(((x) >> 29) & 1)
#define	BIT31(x)	(((x) >> 31) & 1)

#define	MKB4(a,b,c,d)	((a << 3) | (b << 2) | (c << 1) | d)
#define	MKB3(a,b,c)	((a << 2) | (b << 1) | (c))
#define	MKB2(a,b)	((a << 1) | (b))
#define	MKB12(a,b,c,d,e,f,g,h,i,j,k,l)				\
	((a << 11) | (b << 10) | (c << 9) | (d << 8) | 		\
	 (e << 7) | (f << 6) | (g << 5) | (h << 4) | 		\
	 (i << 3) | (j << 2) | (k << 1) | (l << 0))


