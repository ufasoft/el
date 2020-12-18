;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (c) 2012, Intel Corporation
;
; All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are
; met:
;
; * Redistributions of source code must retain the above copyright
;   notice, this list of conditions and the following disclaimer.
;
; * Redistributions in binary form must reproduce the above copyright
;   notice, this list of conditions and the following disclaimer in the
;   documentation and/or other materials provided with the
;   distribution.
;
; * Neither the name of the Intel Corporation nor the names of its
;   contributors may be used to endorse or promote products derived from
;   this software without specific prior written permission.
;
;
; THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY
; EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
; PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL CORPORATION OR
; CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
; EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
; PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
; LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
; NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
; SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;/*######   Copyright (c) 2019      Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
;#                                                                                                                                     #
;# 		See LICENSE for licensing information                                                                                  #
;#####################################################################################################################################*/

; SHA-256 for x64/SSE2

INCLUDE el/x86x64.inc

OPTION_LANGUAGE_C


; COPY_XMM_AND_BSWAP xmm, [mem], byte_flip_mask
; Load xmm with mem and byte swap each dword
COPY_XMM_AND_BSWAP MACRO	a, b, c
	movdqu	a, b
	pshufb	a, c
ENDM

EXTRN g_sha256_k:BYTE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

X0	EQU	xmm4
X1	EQU	xmm5
X2	EQU	xmm6
X3	EQU	xmm7

XTMP0	EQU	xmm0
XTMP1	EQU	xmm1
XTMP2	EQU	xmm2
XTMP3	EQU	xmm3
XTMP4	EQU	xmm8
XFER	EQU	xmm9
XTMP5	EQU	xmm11

SHUF_00BA	EQU	xmm10 ; shuffle xBxA -> 00BA
SHUF_DC00	EQU	xmm12 ; shuffle xDxC -> DC00
BYTE_FLIP_MASK	EQU	xmm13

IFDEF __JWASM__

NUM_BLKS	EQU	rdx	; 3rd arg
CTX	EQU		rsi	; 2nd arg
INP	EQU		rdi	; 1st arg

FULL_SRND	EQU		rdi
SRND	EQU		rdib	; clobbers INP
c	EQU		ecx
d	EQU	 	r8d
e	EQU	 	edx
ELSE
NUM_BLKS	EQU	 r8	; 3rd arg
CTX	EQU		rdx 	; 2nd arg
INP	EQU		rcx 	; 1st arg

FULL_SRND	EQU		rcx
SRND	EQU		cl	; clobbers INP
c	EQU	 	edi
d	EQU		esi
e	EQU	 	r8d

ENDIF
TBL	EQU	rbp
a	EQU	 eax
b	EQU	 ebx

f	EQU	 r9d
g	EQU	 r10d
h	EQU	 r11d

next_bc	EQU	r12d

y0	EQU	 r13d
y1	EQU	 r14d
y2	EQU	 r15d



_INP_END_SIZE	equ 8
_INP_SIZE	equ 8
_XFER_SIZE	equ (4*16 + 8)
IFDEF __JWASM__
_XMM_SAVE_SIZE	equ 0
ELSE
_XMM_SAVE_SIZE	equ 9*16
ENDIF
; STACK_SIZE plus pushes must be an odd multiple of 8
_ALIGN_SIZE	equ 8

_INP_END	equ 0
_INP		equ _INP_END  + _INP_END_SIZE
_XFER		equ _INP      + _INP_SIZE
_XMM_SAVE	equ _XFER     + _XFER_SIZE + _ALIGN_SIZE
STACK_SIZE	equ _XMM_SAVE + _XMM_SAVE_SIZE


; ROTATE_ARGS
; Rotate values of symbols a...h
ROTATE_ARGS MACRO
	TMP_	TEXTEQU	 h
	h	TEXTEQU	g
	g	TEXTEQU	f
	f	TEXTEQU	e
	e	TEXTEQU	d
	d	TEXTEQU	c
	c	TEXTEQU	b
	b	TEXTEQU	a
	a	TEXTEQU	TMP_
ENDM




PROLOG	MACRO
	push	rbx
IFNDEF __JWASM__
	push	rsi
	push	rdi
ENDIF
	push	rbp
	push	r11	; !!! just to alifn
	push	r12
	push	r13
	push	r14
	push	r15

	sub	rsp,STACK_SIZE
IFNDEF __JWASM__
	movdqa	[rsp + _XMM_SAVE + 0*16],xmm6
	movdqa	[rsp + _XMM_SAVE + 1*16],xmm7
	movdqa	[rsp + _XMM_SAVE + 2*16],xmm8
	movdqa	[rsp + _XMM_SAVE + 3*16],xmm9
	movdqa	[rsp + _XMM_SAVE + 4*16],xmm10
	movdqa	[rsp + _XMM_SAVE + 5*16],xmm11
	movdqa	[rsp + _XMM_SAVE + 6*16],xmm12
	movdqa	[rsp + _XMM_SAVE + 7*16],xmm13
	movdqa	[rsp + _XMM_SAVE + 8*16],xmm15
ENDIF

	shl	NUM_BLKS, 6	; convert to bytes
	add	NUM_BLKS, INP	; pointer to end of data
	mov	[rsp + _INP_END], NUM_BLKS

	;; load initial digest
	mov	a,[4*0 + CTX]
	mov	b,[4*1 + CTX]
	mov	c,[4*2 + CTX]
	mov	d,[4*3 + CTX]
	mov	e,[4*4 + CTX]
	mov	f,[4*5 + CTX]
	mov	g,[4*6 + CTX]
	mov	h,[4*7 + CTX]
ENDM

ADD_TO_HASH	MACRO
	add	a, [4*0 + CTX]
	add	b, [4*1 + CTX]
	add	c, [4*2 + CTX]
	add	d, [4*3 + CTX]
	add	e, [4*4 + CTX]
	add	f, [4*5 + CTX]
	add	g, [4*6 + CTX]
	add	h, [4*7 + CTX]
	mov	[4*0 + CTX], a
	mov	[4*1 + CTX], b
	mov	[4*2 + CTX], c
	mov	[4*3 + CTX], d
	mov	[4*4 + CTX], e
	mov	[4*5 + CTX], f
	mov	[4*6 + CTX], g
	mov	[4*7 + CTX], h
ENDM

EPILOG	MACRO

IFNDEF __JWASM__
	movdqa	xmm6,[rsp + _XMM_SAVE + 0*16]
	movdqa	xmm7,[rsp + _XMM_SAVE + 1*16]
	movdqa	xmm8,[rsp + _XMM_SAVE + 2*16]
	movdqa	xmm9,[rsp + _XMM_SAVE + 3*16]
	movdqa	xmm10,[rsp + _XMM_SAVE + 4*16]
	movdqa	xmm11,[rsp + _XMM_SAVE + 5*16]
	movdqa	xmm12,[rsp + _XMM_SAVE + 6*16]
	movdqa	xmm13,[rsp + _XMM_SAVE + 7*16]
	movdqa	xmm15,[rsp + _XMM_SAVE + 8*16]
ENDIF

	add	rsp, STACK_SIZE

	pop	r15
	pop	r14
	pop	r13
	pop	r12
	pop	r11
	pop	rbp
IFNDEF __JWASM__
	pop	rdi
	pop	rsi
ENDIF
	pop	rbx

	ret
ENDM

DO_2_ROUNDS	MACRO
	rorx	y0, e, (25-11)	; y0 = e >> (25-11)
	rorx	y1, a, (22-13)	; y1 = a >> (22-13)
	xor	y0, e		; y0 = e ^ (e >> (25-11))
	mov	y2, f		; y2 = f
	ror	y0, (11-6)	; y0 = (e >> (11-6)) ^ (e >> (25-6))
	xor	y1, a		; y1 = a ^ (a >> (22-13)
	xor	y2, g		; y2 = f^g
	xor	y0, e		; y0 = e ^ (e >> (11-6)) ^ (e >> (25-6))
	and	y2, e		; y2 = (f^g)&e
	ror	y1, (13-2)	; y1 = (a >> (13-2)) ^ (a >> (22-2))
	xor	y1, a		; y1 = a ^ (a >> (13-2)) ^ (a >> (22-2))
	ror	y0, 6		; y0 = S1 = (e>>6) & (e>>11) ^ (e>>25)
	xor	y2, g		; y2 = CH = ((f^g)&e)^g
	ror	y1, 2		; y1 = S0 = (a>>2) ^ (a>>13) ^ (a>>22)
	add	y2, y0		; y2 = S1 + CH
	add	y2, [rsp + FULL_SRND + _XFER  - 48*4]	; y2 = k + w + S1 + CH
	mov	y0, a		; y0 = a
	add	h, y2		; h = h + S1 + CH + k + w
	mov	next_bc, a		; y2 = a
	or	y0, b		; y0 = a|b
	add	d, h		; d = d + h + S1 + CH + k + w
	and	next_bc, b	; next_bc = a&b
	and	y0, c		; y0 = (a|b)&c
	add	h, y1		; h = h + S1 + CH + k + w + S0
	or	y0, next_bc	; y0 = MAJ = (a|b)&c)|(a&b)
	add	h, y0		; h = h + S1 + CH + k + w + S0 + MAJ
	ROTATE_ARGS

	rorx	y0, e, (25-11)	; y0 = e >> (25-11)
	xor	y0, e		; y0 = e ^ (e >> (25-11))
	mov	y2, f		; y2 = f
	rorx	y1, a, (22-13)	; y1 = a >> (22-13)
	xor	y1, a		; y1 = a ^ (a >> (22-13)
	ror	y0, (11-6)	; y0 = (e >> (11-6)) ^ (e >> (25-6))
	xor	y2, g		; y2 = f^g
	ror	y1, (13-2)	; y1 = (a >> (13-2)) ^ (a >> (22-2))
	xor	y0, e		; y0 = e ^ (e >> (11-6)) ^ (e >> (25-6))
	and	y2, e		; y2 = (f^g)&e
	ror	y0, 6		; y0 = S1 = (e>>6) & (e>>11) ^ (e>>25)
	xor	y1, a		; y1 = a ^ (a >> (13-2)) ^ (a >> (22-2))
	xor	y2, g		; y2 = CH = ((f^g)&e)^g
	add	y2, y0		; y2 = S1 + CH
	add	y2, [rsp + FULL_SRND + _XFER  - 48*4 + 4]	; y2 = k + w + S1 + CH
	ror	y1, 2		; y1 = S0 = (a>>2) ^ (a>>13) ^ (a>>22)
	mov	y0, b		; y0 = b
	add	h, y2		; h = h + S1 + CH + k + w
	or	y0, c		; y0 = b|c
	add	d, h		; d = d + h + S1 + CH + k + w
	and	y0, a		; y0 = (b|c)&a
	add	h, y1		; h = h + S1 + CH + k + w + S0
	or	y0, next_bc	; y0 = MAJ = (b|c)&a)|(b&c)
	add	h, y0		; h = h + S1 + CH + k + w + S0 + MAJ
	ROTATE_ARGS
ENDM



;; void _cdecl Sha256Update_x64_SSE2_BMI2(const void *input_data, uint32_t digest[8], uint64_t num_blks);
Sha256Update_x64_SSE2_BMI2	PROC PUBLIC
	PROLOG

	movdqa	BYTE_FLIP_MASK, OWORD PTR PSHUFFLE_BYTE_FLIP_MASK
	movdqa	SHUF_00BA, OWORD PTR _SHUF_00BA
	movdqa	SHUF_DC00, OWORD PTR _SHUF_DC00

	lea	TBL, g_sha256_k
loop0:

	;; byte swap first 16 dwords
	COPY_XMM_AND_BSWAP	X0, [INP + 0*16], BYTE_FLIP_MASK
	COPY_XMM_AND_BSWAP	X1, [INP + 1*16], BYTE_FLIP_MASK
	COPY_XMM_AND_BSWAP	X2, [INP + 2*16], BYTE_FLIP_MASK
	COPY_XMM_AND_BSWAP	X3, [INP + 3*16], BYTE_FLIP_MASK

	mov	[rsp + _INP], INP

	;; schedule 48 input dwords, by doing 3 rounds of 16 each
	xor	FULL_SRND, FULL_SRND
	movdqa	XFER, X0
	movdqa	XTMP0, X3
align 16
loop1:
	paddd	XFER, [TBL + FULL_SRND]
	movdqa	[rsp + _XFER], XFER

		;; compute s0 four at a time and s1 two at a time
		;; compute W[-16] + W[-7] 4 at a time
		; movdqa	XTMP0, X3  ;; already assigned at the ent of loop
	rorx	y0, e, (25-11)	; y0 = e >> (25-11)
		palignr	XTMP0, X2, 4	; XTMP0 = W[-7]
	rorx	y1, a, (22-13)	; y1 = a >> (22-13)
	xor	y0, e		; y0 = e ^ (e >> (25-11))
	mov	y2, f		; y2 = f
	ror	y0, (11-6)	; y0 = (e >> (11-6)) ^ (e >> (25-6))
		movdqa	XTMP1, X1
	xor	y1, a		; y1 = a ^ (a >> (22-13)
	xor	y2, g		; y2 = f^g
		paddd	XTMP0, X0	; XTMP0 = W[-7] + W[-16]
	xor	y0, e		; y0 = e ^ (e >> (11-6)) ^ (e >> (25-6))
	and	y2, e		; y2 = (f^g)&e
	ror	y1, (13-2)	; y1 = (a >> (13-2)) ^ (a >> (22-2))
		;; compute s0
		palignr	XTMP1, X0, 4	; XTMP1 = W[-15]
	xor	y1, a		; y1 = a ^ (a >> (13-2)) ^ (a >> (22-2))
	ror	y0, 6		; y0 = S1 = (e>>6) & (e>>11) ^ (e>>25)
	xor	y2, g		; y2 = CH = ((f^g)&e)^g
		movdqa	XTMP2, XTMP1	; XTMP2 = W[-15]
	ror	y1, 2		; y1 = S0 = (a>>2) ^ (a>>13) ^ (a>>22)
	add	y2, y0		; y2 = S1 + CH
	add	y2, [rsp + _XFER + 0*4]	; y2 = k + w + S1 + CH
		movdqa	XTMP3, XTMP1	; XTMP3 = W[-15]
	mov	y0, a		; y0 = a
	add	h, y2		; h = h + S1 + CH + k + w
	mov	next_bc, a		; y2 = a
		pslld	XTMP1, (32-7)
	or	y0, b		; y0 = a|b
	add	d, h		; d = d + h + S1 + CH + k + w
	and	next_bc, b	; next_bc = a&b
		psrld	XTMP2, 7
	and	y0, c		; y0 = (a|b)&c
	add	h, y1		; h = h + S1 + CH + k + w + S0
		por	XTMP1, XTMP2	; XTMP1 = W[-15] ror 7
	or	y0, next_bc	; y0 = MAJ = (a|b)&c)|(a&b)
	add	h, y0		; h = h + S1 + CH + k + w + S0 + MAJ

	ROTATE_ARGS
		movdqa	XTMP2, XTMP3	; XTMP2 = W[-15]
		movdqa	XTMP4, XTMP3	; XTMP4 = W[-15]
	rorx	y0, e, (25-11)	; y0 = e >> (25-11)
	xor	y0, e		; y0 = e ^ (e >> (25-11))
	mov	y2, f		; y2 = f
	rorx	y1, a, (22-13)	; y1 = a >> (22-13)
		pslld	XTMP3, (32-18)
	xor	y1, a		; y1 = a ^ (a >> (22-13)
	ror	y0, (11-6)	; y0 = (e >> (11-6)) ^ (e >> (25-6))
	xor	y2, g		; y2 = f^g
		psrld	XTMP2, 18
	ror	y1, (13-2)	; y1 = (a >> (13-2)) ^ (a >> (22-2))
	xor	y0, e		; y0 = e ^ (e >> (11-6)) ^ (e >> (25-6))
	and	y2, e		; y2 = (f^g)&e
	ror	y0, 6		; y0 = S1 = (e>>6) & (e>>11) ^ (e>>25)
		pxor	XTMP1, XTMP3
	xor	y1, a		; y1 = a ^ (a >> (13-2)) ^ (a >> (22-2))
	xor	y2, g		; y2 = CH = ((f^g)&e)^g
		psrld	XTMP4, 3	; XTMP4 = W[-15] >> 3
	add	y2, y0		; y2 = S1 + CH
	add	y2, [rsp + _XFER + 1*4]	; y2 = k + w + S1 + CH
	ror	y1, 2		; y1 = S0 = (a>>2) ^ (a>>13) ^ (a>>22)
		pxor	XTMP1, XTMP2	; XTMP1 = W[-15] ror 7 ^ W[-15] ror 18
	mov	y0, b		; y0 = b
	add	h, y2		; h = h + S1 + CH + k + w
		pxor	XTMP1, XTMP4	; XTMP1 = s0
	or	y0, c		; y0 = b|c
	add	d, h		; d = d + h + S1 + CH + k + w
		;; compute low s1
		pshufd	XTMP2, X3, 11111010b	; XTMP2 = W[-2] {BBAA}
	and	y0, a		; y0 = (b|c)&a
	add	h, y1		; h = h + S1 + CH + k + w + S0
		paddd	XTMP0, XTMP1	; XTMP0 = W[-16] + W[-7] + s0
	or	y0, next_bc	; y0 = MAJ = (b|c)&a)|(b&c)
	add	h, y0		; h = h + S1 + CH + k + w + S0 + MAJ

	ROTATE_ARGS
		movdqa	XTMP3, XTMP2	; XTMP3 = W[-2] {BBAA}
	rorx	y0, e, (25-11)	; y0 = e >> (25-11)
		movdqa	XTMP4, XTMP2	; XTMP4 = W[-2] {BBAA}
	xor	y0, e		; y0 = e ^ (e >> (25-11))
	rorx	y1, a, (22-13)	; y1 = a >> (22-13)
	mov	y2, f		; y2 = f
	xor	y1, a		; y1 = a ^ (a >> (22-13)
	ror	y0, (11-6)	; y0 = (e >> (11-6)) ^ (e >> (25-6))
		psrlq	XTMP2, 17	; XTMP2 = W[-2] ror 17 {xBxA}
	xor	y2, g		; y2 = f^g
		psrlq	XTMP3, 19	; XTMP3 = W[-2] ror 19 {xBxA}
	xor	y0, e		; y0 = e ^ (e >> (11-6)) ^ (e >> (25-6))
	and	y2, e		; y2 = (f^g)&e
		psrld	XTMP4, 10	; XTMP4 = W[-2] >> 10 {BBAA}
	ror	y1, (13-2)	; y1 = (a >> (13-2)) ^ (a >> (22-2))
	xor	y1, a		; y1 = a ^ (a >> (13-2)) ^ (a >> (22-2))
	xor	y2, g		; y2 = CH = ((f^g)&e)^g
	ror	y0, 6		; y0 = S1 = (e>>6) & (e>>11) ^ (e>>25)
		pxor	XTMP2, XTMP3
	add	y2, y0		; y2 = S1 + CH
	ror	y1, 2		; y1 = S0 = (a>>2) ^ (a>>13) ^ (a>>22)
	add	y2, [rsp + _XFER + 2*4]	; y2 = k + w + S1 + CH
		pxor	XTMP4, XTMP2	; XTMP4 = s1 {xBxA}
	mov	y0, a		; y0 = a
	add	h, y2		; h = h + S1 + CH + k + w
	mov	next_bc, a		; y2 = a
		pshufb	XTMP4, SHUF_00BA	; XTMP4 = s1 {00BA}
	or	y0, b		; y0 = a|b
	add	d, h		; d = d + h + S1 + CH + k + w
	and	next_bc, b	; next_bc = a&b
		paddd	XTMP0, XTMP4	; XTMP0 = {..., ..., W[1], W[0]}
	and	y0, c		; y0 = (a|b)&c
	add	h, y1		; h = h + S1 + CH + k + w + S0
		;; compute high s1
		pshufd	XTMP2, XTMP0, 01010000b	; XTMP2 = W[-2] {DDCC}
	or	y0, next_bc	; y0 = MAJ = (a|b)&c)|(a&b)
	add	h, y0		; h = h + S1 + CH + k + w + S0 + MAJ

	ROTATE_ARGS
		movdqa	XTMP3, XTMP2	; XTMP3 = W[-2] {DDCC}
	rorx	y0, e, (25-11)	; y0 = e >> (25-11)
		movdqa	X0,    XTMP2	; X0    = W[-2] {DDCC}
	rorx	y1, a, (22-13)	; y1 = a >> (22-13)
	xor	y0, e		; y0 = e ^ (e >> (25-11))
	mov	y2, f		; y2 = f
	ror	y0, (11-6)	; y0 = (e >> (11-6)) ^ (e >> (25-6))
		psrlq	XTMP2, 17	; XTMP2 = W[-2] ror 17 {xDxC}
	xor	y1, a		; y1 = a ^ (a >> (22-13)
	xor	y2, g		; y2 = f^g
		psrlq	XTMP3, 19	; XTMP3 = W[-2] ror 19 {xDxC}
	xor	y0, e		; y0 = e ^ (e >> (11-6)) ^ (e >> (25-6))
	and	y2, e		; y2 = (f^g)&e
	ror	y1, (13-2)	; y1 = (a >> (13-2)) ^ (a >> (22-2))
		psrld	X0,    10	; X0 = W[-2] >> 10 {DDCC}
	xor	y1, a		; y1 = a ^ (a >> (13-2)) ^ (a >> (22-2))
	ror	y0, 6		; y0 = S1 = (e>>6) & (e>>11) ^ (e>>25)
	xor	y2, g		; y2 = CH = ((f^g)&e)^g
		pxor	XTMP2, XTMP3
	ror	y1, 2		; y1 = S0 = (a>>2) ^ (a>>13) ^ (a>>22)
	add	y2, y0		; y2 = S1 + CH
	add	y2, [rsp + _XFER + 3*4]	; y2 = k + w + S1 + CH
		pxor	X0, XTMP2	; X0 = s1 {xDxC}
	mov	y0, b		; y0 = b
	add	h, y2		; h = h + S1 + CH + k + w
		pshufb	X0, SHUF_DC00	; X0 = s1 {DC00}
	or	y0, c		; y0 = b|c
	add	d, h		; d = d + h + S1 + CH + k + w
		paddd	XTMP0, X0	; X0 = {W[3], W[2], W[1], W[0]}
	and	y0, a		; y0 = (b|c)&a
	add	h, y1		; h = h + S1 + CH + k + w + S0
	or	y0, next_bc	; y0 = MAJ = (b|c)&a)|(b&c)
	add	h, y0		; h = h + S1 + CH + k + w + S0 + MAJ

	ROTATE_ARGS
	movdqa	XFER, X1
	xchg	a, e
	movdqa	X1, X2
	xchg	b, f
	movdqa	X2, X3
	xchg	c, g
	movdqa	X3, XTMP0
	xchg	d, h
	movdqa	X0, XFER

	add	SRND, 4*4
	cmp	SRND, 48*4
	jne	loop1

	ROTATE_ARGS
	ROTATE_ARGS
	ROTATE_ARGS
	ROTATE_ARGS

	paddd	X0, [TBL + FULL_SRND + 0*16]
	paddd	X1, [TBL + FULL_SRND + 1*16]
	paddd	X2, [TBL + FULL_SRND + 2*16]
	paddd	X3, [TBL + FULL_SRND + 3*16]
	movdqa	[rsp + _XFER], X0
	movdqa	[rsp + _XFER + 1*16], X1
	movdqa	[rsp + _XFER + 2*16], X2
	movdqa	[rsp + _XFER + 3*16], X3
loop2:
        DO_2_ROUNDS

        ; rotate by 2
        mov	y0, h		; 6 xchg are slower
        mov	y1, g
        mov	h, f
        mov	g, e
        mov	f, d
        mov	e, c
        mov	d, b
        mov	c, a
        mov	b, y0
        mov	a, y1

	add	SRND, 2*4
	jnz	loop2

	ROTATE_ARGS
	ROTATE_ARGS
	ROTATE_ARGS
	ROTATE_ARGS
	ROTATE_ARGS
	ROTATE_ARGS

	ADD_TO_HASH

	mov	INP, [rsp + _INP]
	add	INP, 64
	cmp	INP, [rsp + _INP_END]
	jne	loop0

	EPILOG
Sha256Update_x64_SSE2_BMI2	ENDP




;; void _cdecl Sha256Update_x64_AVX2_BMI2(const void *input_data, uint32_t digest[8], uint64_t num_blks);
Sha256Update_x64_AVX2_BMI2	PROC PUBLIC
	PROLOG

	movdqa	BYTE_FLIP_MASK, OWORD PTR PSHUFFLE_BYTE_FLIP_MASK
	movdqa	SHUF_00BA, OWORD PTR _SHUF_00BA
	movdqa	SHUF_DC00, OWORD PTR _SHUF_DC00

	lea	TBL, g_sha256_k
loop0:

	;; byte swap first 16 dwords
	COPY_XMM_AND_BSWAP	X0, [INP + 0*16], BYTE_FLIP_MASK
	COPY_XMM_AND_BSWAP	X1, [INP + 1*16], BYTE_FLIP_MASK
	COPY_XMM_AND_BSWAP	X2, [INP + 2*16], BYTE_FLIP_MASK
	COPY_XMM_AND_BSWAP	X3, [INP + 3*16], BYTE_FLIP_MASK

	mov	[rsp + _INP], INP

	;; schedule 48 input dwords, by doing 3 rounds of 16 each
	xor	FULL_SRND, FULL_SRND
align 16
loop1:
	vpaddd	XFER, X0, [TBL + FULL_SRND]
	vmovdqa	OWORD PTR [rsp + _XFER], XFER

		;; compute s0 four at a time and s1 two at a time
		;; compute W[-16] + W[-7] 4 at a time
		; movdqa	XTMP0, X3  ;; already assigned at the ent of loop
	rorx	y0, e, (25-11)	; y0 = e >> (25-11)
		vpalignr	XTMP0, X3, X2, 4	; XTMP0 = W[-7]
	rorx	y1, a, (22-13)	; y1 = a >> (22-13)
	xor	y0, e		; y0 = e ^ (e >> (25-11))
	mov	y2, f		; y2 = f
	ror	y0, (11-6)	; y0 = (e >> (11-6)) ^ (e >> (25-6))
	xor	y1, a		; y1 = a ^ (a >> (22-13)
	xor	y2, g		; y2 = f^g
		vpaddd	XTMP0, XTMP0, X0	; XTMP0 = W[-7] + W[-16]
	xor	y0, e		; y0 = e ^ (e >> (11-6)) ^ (e >> (25-6))
	and	y2, e		; y2 = (f^g)&e
	ror	y1, (13-2)	; y1 = (a >> (13-2)) ^ (a >> (22-2))
		;; compute s0
		vpalignr	XTMP1, X1, X0, 4	; XTMP1 = W[-15]
	xor	y1, a		; y1 = a ^ (a >> (13-2)) ^ (a >> (22-2))
	ror	y0, 6		; y0 = S1 = (e>>6) & (e>>11) ^ (e>>25)
	xor	y2, g		; y2 = CH = ((f^g)&e)^g
	ror	y1, 2		; y1 = S0 = (a>>2) ^ (a>>13) ^ (a>>22)
	add	y2, y0		; y2 = S1 + CH
	add	y2, [rsp + _XFER + 0*4]	; y2 = k + w + S1 + CH
	mov	y0, a		; y0 = a
	add	h, y2		; h = h + S1 + CH + k + w
	mov	next_bc, a		; y2 = a
		vpsrld	XTMP2, XTMP1, 7
	or	y0, b		; y0 = a|b
	add	d, h		; d = d + h + S1 + CH + k + w
	and	next_bc, b	; next_bc = a&b
		vpslld	XTMP3, XTMP1, (32-7)
	and	y0, c		; y0 = (a|b)&c
	add	h, y1		; h = h + S1 + CH + k + w + S0
		vpor	XTMP3, XTMP3, XTMP2	; XTMP1 = W[-15] MY_ROR 7
	or	y0, next_bc	; y0 = MAJ = (a|b)&c)|(a&b)
	add	h, y0		; h = h + S1 + CH + k + w + S0 + MAJ

	ROTATE_ARGS
	rorx	y0, e, (25-11)	; y0 = e >> (25-11)
	xor	y0, e		; y0 = e ^ (e >> (25-11))
	mov	y2, f		; y2 = f
	rorx	y1, a, (22-13)	; y1 = a >> (22-13)
	xor	y1, a		; y1 = a ^ (a >> (22-13)
	ror	y0, (11-6)	; y0 = (e >> (11-6)) ^ (e >> (25-6))
	xor	y2, g		; y2 = f^g
		vpsrld	XTMP2, XTMP1, 18
	ror	y1, (13-2)	; y1 = (a >> (13-2)) ^ (a >> (22-2))
	xor	y0, e		; y0 = e ^ (e >> (11-6)) ^ (e >> (25-6))
	and	y2, e		; y2 = (f^g)&e
	ror	y0, 6		; y0 = S1 = (e>>6) & (e>>11) ^ (e>>25)
		vpsrld	XTMP4, XTMP1, 3	; XTMP4 = W[-15] >> 3
	xor	y1, a		; y1 = a ^ (a >> (13-2)) ^ (a >> (22-2))
	xor	y2, g		; y2 = CH = ((f^g)&e)^g
		vpslld	XTMP1, XTMP1, (32-18)
	add	y2, y0		; y2 = S1 + CH
	add	y2, [rsp + _XFER + 1*4]	; y2 = k + w + S1 + CH
	ror	y1, 2		; y1 = S0 = (a>>2) ^ (a>>13) ^ (a>>22)
		vpxor	XTMP3, XTMP3, XTMP1
	mov	y0, b		; y0 = b
	add	h, y2		; h = h + S1 + CH + k + w
		vpxor	XTMP3, XTMP3, XTMP2	; XTMP1 = W[-15] MY_ROR 7 ^ W[-15] MY_ROR 18
	or	y0, c		; y0 = b|c
	add	d, h		; d = d + h + S1 + CH + k + w
		;; compute low s1
		vpshufd	XTMP2, X3, 11111010b	; XTMP2 = W[-2] {BBAA}
	and	y0, a		; y0 = (b|c)&a
		vpxor	XTMP1, XTMP3, XTMP4	; XTMP1 = s0
	add	h, y1		; h = h + S1 + CH + k + w + S0

	or	y0, next_bc	; y0 = MAJ = (b|c)&a)|(b&c)
		vpsrld	XTMP4, XTMP2, 10	; XTMP4 = W[-2] >> 10 {BBAA}
	add	h, y0		; h = h + S1 + CH + k + w + S0 + MAJ

	ROTATE_ARGS
	rorx	y0, e, (25-11)	; y0 = e >> (25-11)
	xor	y0, e		; y0 = e ^ (e >> (25-11))
	rorx	y1, a, (22-13)	; y1 = a >> (22-13)
	mov	y2, f		; y2 = f
	xor	y1, a		; y1 = a ^ (a >> (22-13)
	ror	y0, (11-6)	; y0 = (e >> (11-6)) ^ (e >> (25-6))
		vpaddd	XTMP0, XTMP0, XTMP1	; XTMP0 = W[-16] + W[-7] + s0
	xor	y2, g		; y2 = f^g
		vpsrlq	XTMP3, XTMP2, 19	; XTMP3 = W[-2] MY_ROR 19 {xBxA}
	xor	y0, e		; y0 = e ^ (e >> (11-6)) ^ (e >> (25-6))
	and	y2, e		; y2 = (f^g)&e
		vpsrlq	XTMP2, XTMP2, 17	; XTMP2 = W[-2] MY_ROR 17 {xBxA}
	ror	y1, (13-2)	; y1 = (a >> (13-2)) ^ (a >> (22-2))
	xor	y1, a		; y1 = a ^ (a >> (13-2)) ^ (a >> (22-2))
	xor	y2, g		; y2 = CH = ((f^g)&e)^g
	ror	y0, 6		; y0 = S1 = (e>>6) & (e>>11) ^ (e>>25)
		vpxor	XTMP2, XTMP2, XTMP3
	add	y2, y0		; y2 = S1 + CH
	ror	y1, 2		; y1 = S0 = (a>>2) ^ (a>>13) ^ (a>>22)
	add	y2, [rsp + _XFER + 2*4]	; y2 = k + w + S1 + CH
		vpxor	XTMP4, XTMP4, XTMP2	; XTMP4 = s1 {xBxA}
	mov	y0, a		; y0 = a
	add	h, y2		; h = h + S1 + CH + k + w
	mov	next_bc, a		; y2 = a
		vpshufb	XTMP4, XTMP4, SHUF_00BA	; XTMP4 = s1 {00BA}
	or	y0, b		; y0 = a|b
	add	d, h		; d = d + h + S1 + CH + k + w
	and	next_bc, b	; next_bc = a&b
		vpaddd	XTMP0, XTMP0, XTMP4	; XTMP0 = {..., ..., W[1], W[0]}
	and	y0, c		; y0 = (a|b)&c
	add	h, y1		; h = h + S1 + CH + k + w + S0
		;; compute high s1
		vpshufd	XTMP2, XTMP0, 01010000b	; XTMP2 = W[-2] {DDCC}
	or	y0, next_bc	; y0 = MAJ = (a|b)&c)|(a&b)
	add	h, y0		; h = h + S1 + CH + k + w + S0 + MAJ

	ROTATE_ARGS
	rorx	y0, e, (25-11)	; y0 = e >> (25-11)
	rorx	y1, a, (22-13)	; y1 = a >> (22-13)
	xor	y0, e		; y0 = e ^ (e >> (25-11))
	mov	y2, f		; y2 = f
	ror	y0, (11-6)	; y0 = (e >> (11-6)) ^ (e >> (25-6))
		vpsrld	XTMP5, XTMP2,   10	; XTMP5 = W[-2] >> 10 {DDCC}
	xor	y1, a		; y1 = a ^ (a >> (22-13)
	xor	y2, g		; y2 = f^g
		vpsrlq	XTMP3, XTMP2, 19	; XTMP3 = W[-2] MY_ROR 19 {xDxC}
	xor	y0, e		; y0 = e ^ (e >> (11-6)) ^ (e >> (25-6))
	and	y2, e		; y2 = (f^g)&e
	ror	y1, (13-2)	; y1 = (a >> (13-2)) ^ (a >> (22-2))
		vpsrlq	XTMP2, XTMP2, 17	; XTMP2 = W[-2] MY_ROR 17 {xDxC}
	xor	y1, a		; y1 = a ^ (a >> (13-2)) ^ (a >> (22-2))
	ror	y0, 6		; y0 = S1 = (e>>6) & (e>>11) ^ (e>>25)
	xor	y2, g		; y2 = CH = ((f^g)&e)^g
		vpxor	XTMP2, XTMP2, XTMP3
	ror	y1, 2		; y1 = S0 = (a>>2) ^ (a>>13) ^ (a>>22)
	add	y2, y0		; y2 = S1 + CH
		vpxor	XTMP5, XTMP5, XTMP2	; XTMP5 = s1 {xDxC}
	add	y2, [rsp + _XFER + 3*4]	; y2 = k + w + S1 + CH
	mov	y0, b		; y0 = b
	add	h, y2		; h = h + S1 + CH + k + w
		vpshufb	XTMP5, XTMP5, SHUF_DC00	; XTMP5 = s1 {DC00}
	or	y0, c		; y0 = b|c
	add	d, h		; d = d + h + S1 + CH + k + w
	and	y0, a		; y0 = (b|c)&a
	add	h, y1		; h = h + S1 + CH + k + w + S0
	or	y0, next_bc	; y0 = MAJ = (b|c)&a)|(b&c)
		vpaddd	XTMP0, XTMP5, XTMP0	; X0 = {W[3], W[2], W[1], W[0]}
	add	h, y0		; h = h + S1 + CH + k + w + S0 + MAJ

	ROTATE_ARGS
	vmovdqa	X0, X1
	xchg	a, e
	vmovdqa	X1, X2
	xchg	b, f
	vmovdqa	X2, X3
	xchg	c, g
	vmovdqa	X3, XTMP0
	xchg	d, h

	add	SRND, 16
	jnz	loop1

	ROTATE_ARGS
	ROTATE_ARGS
	ROTATE_ARGS
	ROTATE_ARGS


	ADD_TO_HASH

	mov	INP, [rsp + _INP]
	add	INP, 64
	cmp	INP, [rsp + _INP_END]
	jne	loop0

	EPILOG
Sha256Update_x64_AVX2_BMI2	ENDP


ALIGN	16

PSHUFFLE_BYTE_FLIP_MASK DQ	0405060700010203h, 0c0d0e0f08090a0bh

; shuffle xBxA -> 00BA
_SHUF_00BA	DQ	0b0a090803020100h, 0FFFFFFFFFFFFFFFFh

; shuffle xDxC -> DC00
_SHUF_DC00	DQ	0FFFFFFFFFFFFFFFFh, 00b0a090803020100h









END
