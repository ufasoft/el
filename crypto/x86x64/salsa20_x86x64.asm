;/*######   Copyright (c) 2014-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
;#                                                                                                                                     #
;# 		See LICENSE for licensing information                                                                                         #
;#####################################################################################################################################*/

; Salsa20 implementation for x86/x64/SSE2

INCLUDE el/x86x64.inc

OPTION_LANGUAGE_C

IF X64
ELSE
.XMM
ENDIF

.CODE

salsa20_round MACRO		a, b, c, d							; contract: b==xmm4
	paddd	xmm4, a
	movdqa	xmm5, xmm4
	pslld	xmm4, 7
	psrld	xmm5, 32-7
	pxor	d, xmm4
	pxor	d, xmm5
	
	movdqa	xmm4, a
	paddd	xmm4, d
	movdqa	xmm5, xmm4
	pslld	xmm4, 9
	psrld	xmm5, 32-9
	pxor	c, xmm4
	pshufd	xmm4, d, 010010011b	; 		; xmm4 = target *1
	pxor	c, xmm5

	paddd	d, c
	movdqa	xmm5, d
	pslld	d, 13
	psrld	xmm5, 32-13
	pxor	b, d
	pxor	b, xmm5
		
	pshufd	d, b, 000111001b
	paddd	b, c
	pshufd	c, c, 001001110b
	movdqa	xmm5, b
	pslld	b, 18
	psrld	xmm5, 32-18
	pxor	a, b
	pxor	a, xmm5
	movdqa	b, xmm4
ENDM


IF X64
IFDEF __JWASM__			; UNIX calling convention
	SalsaCore_SSE2	PROC PUBLIC USES ZCX ZDX, w:QWORD, rounds:QWORD
	mov		zcx, zsi
ELSE
	SalsaCore_SSE2 PROC PUBLIC USES ZCX ZDX ZDI	;;; , w:QWORD, rounds:QWORD
	mov		zdi, zcx		; w
	mov		zcx, zdx		; rounds
ENDIF
ELSE
	SalsaCore_SSE2	PROC PUBLIC USES ECX EDX ESI EDI, w:DWORD, rounds:DWORD
	mov		zdi, w
	mov		zcx, rounds
ENDIF
	movdqa	xmm0, [zdi]
	movdqa	xmm1, [zdi+16]
	movdqa	xmm2, [zdi+32]
	movdqa	xmm3, [zdi+48]
	movdqa	xmm4, xmm1

LAB_SALSA20:
	salsa20_round	xmm0, xmm1, xmm2, xmm3
	dec		ecx
	jnz		LAB_SALSA20

	paddd	xmm0, [zdi]   
	paddd	xmm1, [zdi+16]
	paddd	xmm2, [zdi+32]
	paddd	xmm3, [zdi+48]
	movdqa	[zdi], xmm0
	movdqa	[zdi+16], xmm1
	movdqa	[zdi+32], xmm2
	movdqa	[zdi+48], xmm3
	
	ret
SalsaCore_SSE2 ENDP


CalcSalsa20_8 PROC
	movdqa	[zbx], xmm0
	movdqa	[zbx+16], xmm1
	movdqa	[zbx+32], xmm2
	movdqa	[zbx+48], xmm3
	movdqa	xmm4, xmm1

	salsa20_round	xmm0, xmm1, xmm2, xmm3
	salsa20_round	xmm0, xmm1, xmm2, xmm3
	salsa20_round	xmm0, xmm1, xmm2, xmm3
	salsa20_round	xmm0, xmm1, xmm2, xmm3
	salsa20_round	xmm0, xmm1, xmm2, xmm3
	salsa20_round	xmm0, xmm1, xmm2, xmm3
	salsa20_round	xmm0, xmm1, xmm2, xmm3
	salsa20_round	xmm0, xmm1, xmm2, xmm3


	paddd	xmm0, [zbx]
	paddd	xmm1, [zbx+16]
	paddd	xmm2, [zbx+32]
	paddd	xmm3, [zbx+48]

	movdqa	[zbx], xmm0
	movdqa	[zbx+16], xmm1
	movdqa	[zbx+32], xmm2
	movdqa	[zbx+48], xmm3
	
	ret
CalcSalsa20_8 ENDP

IF X64

CalcSalsa20_8_x64 PROC
	pxor	xmm0, xmm8
	pxor	xmm1, xmm9
	pxor	xmm2, xmm10
	pxor	xmm3, xmm11

	movdqa	xmm12, xmm0
	movdqa	xmm13, xmm1
	movdqa	xmm14, xmm2
	movdqa	xmm15, xmm3

	movdqa	xmm4, xmm1
	salsa20_round	xmm0, xmm1, xmm2, xmm3
	salsa20_round	xmm0, xmm1, xmm2, xmm3
	salsa20_round	xmm0, xmm1, xmm2, xmm3
	salsa20_round	xmm0, xmm1, xmm2, xmm3
	salsa20_round	xmm0, xmm1, xmm2, xmm3
	salsa20_round	xmm0, xmm1, xmm2, xmm3
	salsa20_round	xmm0, xmm1, xmm2, xmm3
	salsa20_round	xmm0, xmm1, xmm2, xmm3

	paddd	xmm0, xmm12
	paddd	xmm1, xmm13
	paddd	xmm2, xmm14
	paddd	xmm3, xmm15


	pxor	xmm8, xmm0
	pxor	xmm9, xmm1
	pxor	xmm10, xmm2
	pxor	xmm11, xmm3

	movdqa	xmm12, xmm8
	movdqa	xmm13, xmm9
	movdqa	xmm14, xmm10
	movdqa	xmm15, xmm11

	movdqa	xmm4, xmm9
	salsa20_round	xmm8, xmm9, xmm10, xmm11
	salsa20_round	xmm8, xmm9, xmm10, xmm11
	salsa20_round	xmm8, xmm9, xmm10, xmm11
	salsa20_round	xmm8, xmm9, xmm10, xmm11
	salsa20_round	xmm8, xmm9, xmm10, xmm11
	salsa20_round	xmm8, xmm9, xmm10, xmm11
	salsa20_round	xmm8, xmm9, xmm10, xmm11
	salsa20_round	xmm8, xmm9, xmm10, xmm11

	paddd	xmm8, xmm12
	paddd	xmm9, xmm13
	paddd	xmm10, xmm14
	paddd	xmm11, xmm15

	ret
CalcSalsa20_8_x64 ENDP

ENDIF						; X64


		END


