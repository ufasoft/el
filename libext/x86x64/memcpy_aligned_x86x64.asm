;/*######   Copyright (c) 2014-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
;#                                                                                                                                     #
;# 		See LICENSE for licensing information                                                                                         #
;#####################################################################################################################################*/

; Memcpy implementations

INCLUDE el/x86x64.inc

IF X64
ELSE
.XMM
ENDIF


EXTRN g_bHasSse2:PTR BYTE

IF X64
IFDEF __JWASM__			; UNIX calling convention
	MemcpyAligned32	PROC PUBLIC USES ZSI ZDI
	mov		rax, rdi
	mov		rcx, rdx
ELSE
	MemcpyAligned32	PROC PUBLIC USES ZSI ZDI
	mov		rax, rcx
	mov		rsi, rdx
	mov		rcx, r8
ENDIF
ELSE
	MemcpyAligned32	PROC PUBLIC USES ZSI ZDI, d:DWORD, s:DWORD, sz:DWORD
	mov		zsi, s
	mov		zcx, sz
	mov		zax, d
ENDIF
	mov		zdi, zax
	cmp		zcx, 8
	jb		$MovsImp
	prefetchnta [zsi]
	cmp		g_bHasSse2, 0
	je		$MovsImp
	add		zcx, 63
   	shr		zcx, 6
$Loop:
	prefetchnta [zsi+64*8]
	movdqa	xmm0, [zsi]
	movdqa	xmm1, [zsi+16]
	movdqa	xmm2, [zsi+16*2]
	movdqa	xmm3, [zsi+16*3]
	movntdq	[zdi],		xmm0		;!!! was movdqa
	movntdq	[zdi+16],	xmm1 
	movntdq	[zdi+16*2],	xmm2 
	movntdq	[zdi+16*3],	xmm3 
	add		zsi, 64
	add		zdi, 64
	loop	$Loop
	ret
$MovsImp:
IF X64
	add		zcx, 7
	shr		zcx, 3
	rep		movsq
ELSE
	add		zcx, 3
	shr		zcx, 2
	rep		movsd
ENDIF
$Ret:
	ret
MemcpyAligned32	ENDP


		END




