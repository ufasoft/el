;/*######   Copyright (c) 2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
;#                                                                                                                                     #
;# 		See LICENSE for licensing information                                                                                         #
;#####################################################################################################################################*/

; Groestl Hash for x86/x64/SSE2

INCLUDE el/x86x64.inc

OPTION_LANGUAGE_C

IF X64
ELSE
.XMM
ENDIF


EXTRN g_groestl_T_table:PTR QWORD


.CODE

groestl_2cols MACRO		colsXMM, c
	mov	al, byte ptr [rdi + rdx + c*8]
	mov	bl, byte ptr [rdi + rdx + (c+1)*8]
	movlpd	xmm8, qword ptr [r10+rax*8]	
	movhpd	xmm8, qword ptr [r10+rbx*8]
	pxor	colsXMM, xmm8
ENDM

IF X64	
IFDEF __JWASM__			; UNIX calling convention
	Groestl512_x86x64Sse2	PROC PUBLIC USES ZBX ZSI ZDI R14 ; , roundConstants:QWORD, u:QWORD	; just for generating frame  
	TODO!!!
ELSE
	Groestl512_x86x64Sse2	PROC PUBLIC USES ZBX ZSI ZDI R12 R13 R14 R15; , roundConstants:QWORD, u:QWORD, shiftTable8:QWORD
	mov		rsi, rcx				; roundConstants
	mov		rdi, rdx				; u
	mov		r12, r8					; shiftTable8
ENDIF
ELSE
	Groestl512_x86x64Sse2	PROC PUBLIC USES EBX ECX EDX ESI EDI, roundConstants:DWORD, u:DWORD, shiftTable8:DWORD
	TODO!!!
ENDIF
	mov		r14, g_groestl_T_table
	push	zbp
	mov		zbp, zsp
	sub		rsp, 8*3

	movdqa	xmm0, [zdi+0*16]
	movdqa	xmm1, [zdi+1*16]  
	movdqa	xmm2, [zdi+2*16]  
	movdqa	xmm3, [zdi+3*16]  
	movdqa	xmm4, [zdi+4*16]  
	movdqa	xmm5, [zdi+5*16] 
	movdqa	xmm6, [zdi+6*16] 
	movdqa	xmm7, [zdi+7*16] 

	pxor	xmm0, [zsi+0*16]		; AddRoundConstant for round #0
	pxor	xmm1, [zsi+1*16]
	pxor	xmm2, [zsi+2*16]
	pxor	xmm3, [zsi+3*16]
	pxor	xmm4, [zsi+4*16]
	pxor	xmm5, [zsi+5*16]
	pxor	xmm6, [zsi+6*16]
	pxor	xmm7, [zsi+7*16]
	add		zsi, 8*16

	mov		r13, 0					; round
LAB_ROUND:
	movdqa	[zdi+0*16], xmm0
	movdqa	[zdi+1*16], xmm1
	movdqa	[zdi+2*16], xmm2
	movdqa	[zdi+3*16], xmm3
	movdqa	[zdi+4*16], xmm4
	movdqa	[zdi+5*16], xmm5
	movdqa	[zdi+6*16], xmm6
	movdqa	[zdi+7*16], xmm7

	movdqa	[zdi+128+0*16], xmm0	; second copy of State
	movdqa	[zdi+128+1*16], xmm1
	movdqa	[zdi+128+2*16], xmm2
	movdqa	[zdi+128+3*16], xmm3
	movdqa	[zdi+128+4*16], xmm4
	movdqa	[zdi+128+5*16], xmm5
	movdqa	[zdi+128+6*16], xmm6
	movdqa	[rdi+128+7*16], xmm7

	movdqa	xmm0, [zsi+0*16]		; AddRoundConstant for rounds #1..15,  #15 contains zeros
	movdqa	xmm1, [zsi+1*16]
	movdqa	xmm2, [zsi+2*16]
	movdqa	xmm3, [zsi+3*16]
	movdqa	xmm4, [zsi+4*16]
	movdqa	xmm5, [zsi+5*16]
	movdqa	xmm6, [zsi+6*16]
	movdqa	xmm7, [zsi+7*16]
	add		zsi, 8*16

	mov		rcx, 8
	lea		r10, [r14 + 8*256*8]
	xor		rax, rax
	xor		rbx, rbx
	xor		rdx, rdx
LAB_ROW:
	sub		r10, 256*8
	mov		dl, byte ptr [r12+rcx-1]		; offset*8 + row

	groestl_2cols	xmm0, 0
	groestl_2cols	xmm1, 2
	groestl_2cols	xmm2, 4
	groestl_2cols	xmm3, 6
	groestl_2cols	xmm4, 8
	groestl_2cols	xmm5, 10
	groestl_2cols	xmm6, 12
	groestl_2cols	xmm7, 14

	dec		rcx
	jnz		LAB_ROW

	inc		r13
	cmp		r13, 14
	jb		LAB_ROUND

	movdqa	[zdi+0*16], xmm0
	movdqa	[zdi+1*16], xmm1
	movdqa	[zdi+2*16], xmm2
	movdqa	[zdi+3*16], xmm3
	movdqa	[zdi+4*16], xmm4
	movdqa	[zdi+5*16], xmm5
	movdqa	[zdi+6*16], xmm6
	movdqa	[zdi+7*16], xmm7

	leave
	ret
Groestl512_x86x64Sse2 ENDP


		END




