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
	movzx	rax, byte ptr [rdx + c*8]
	movzx	r10, byte ptr [rdx + (c+1)*8]
	movlpd	xmm8, qword ptr [rbx+rax*8]	
	movhpd	xmm8, qword ptr [rbx+r10*8]
	pxor	colsXMM, xmm8
ENDM


IF X64	
IFDEF __JWASM__			; UNIX calling convention
	Groestl512_x86x64Sse2	PROC PUBLIC USES ZBX ZSI ZDI R14 ; , x:QWORD	; x - just for generating frame   rdi=state, rsi= data
	TODO!!!
ELSE
	Groestl512_x86x64Sse2	PROC PUBLIC USES ZBX ZSI ZDI R12 R13 R14 R15; , pfnAddRoundConstant:QWORD, u:QWORD, shiftTable8:QWORD
	mov		rsi, rcx				; pfnAddRoundConstant
	mov		rdi, rdx				; u
	mov		r12, r8					; shiftTable8
ENDIF
	mov	r14, g_groestl_T_table
ELSE
	Groestl512_x86x64Sse2	PROC PUBLIC USES EBX ECX EDX ESI EDI, pfnAddRoundConstant:DWORD, u:DWORD, shiftTable8:DWORD
	TODO!!!
ENDIF
	push	zbp
	mov		zbp, zsp
	sub		rsp, 8*3

	mov		r13, 0			; round
LAB_ROUND:

	mov		rcx, rdi
	mov		rdx, r13
	mov		r8, 16
	call	rsi					; AddRoundConstant

	movdqa	xmm0, [rdi]
	movdqa	xmm1, [rdi+1*16] 
	movdqa	xmm2, [rdi+2*16] 
	movdqa	xmm3, [rdi+3*16] 
	movdqa	xmm4, [rdi+4*16] 
	movdqa	xmm5, [rdi+5*16] 
	movdqa	xmm6, [rdi+6*16] 
	movdqa	xmm7, [rdi+7*16] 

	movdqa	[128+rdi], xmm0
	movdqa	[128+rdi+1*16], xmm1
	movdqa	[128+rdi+2*16], xmm2
	movdqa	[128+rdi+3*16], xmm3
	movdqa	[128+rdi+4*16], xmm4
	movdqa	[128+rdi+5*16], xmm5
	movdqa	[128+rdi+6*16], xmm6
	movdqa	[128+rdi+7*16], xmm7

	pxor	xmm0, xmm0
	pxor	xmm1, xmm1
	pxor	xmm2, xmm2
	pxor	xmm3, xmm3
	pxor	xmm4, xmm4
	pxor	xmm5, xmm5
	pxor	xmm6, xmm6
	pxor	xmm7, xmm7

	mov		rcx, 8
LAB_ROW:
	mov		rbx, rcx
	shl		rbx, 11
	lea		rbx, [r14 + rbx - 256*8]
	movzx	rdx, byte ptr [r12+rcx-1]		; offset*8
	lea		rdx, [rdx + rcx - 1]
	add		rdx, rdi

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

	movdqa	[rdi], xmm0
	movdqa	[rdi+1*16], xmm1
	movdqa	[rdi+2*16], xmm2
	movdqa	[rdi+3*16], xmm3
	movdqa	[rdi+4*16], xmm4
	movdqa	[rdi+5*16], xmm5
	movdqa	[rdi+6*16], xmm6
	movdqa	[rdi+7*16], xmm7

	inc		r13
	cmp		r13, 14
	jb		LAB_ROUND

	leave
	ret
Groestl512_x86x64Sse2 ENDP


		END




