; ============================================================================
; NRV2S decompressor in 68000 assembly
; By ross — adapted for Zippy Race Amiga port
;
; Register entry point (called via inline asm wrapper):
;   a0 = src (compressed data)
;   a1 = dst (output buffer)
;
; All registers preserved.
; ============================================================================

	section	.text
	public	_nrv2s_unpack

_nrv2s_unpack:
	movem.l	d0-d5/a0-a4,-(sp)

	moveq	#-$80,d0		; bit buffer init
	moveq	#-1,d2			; m_len init
	moveq	#2,d4			; constant: 2
	moveq	#-2,d5			; stack mask
	moveq	#-1,d3			; last_m_off = -1

	; --- Read stack usage header ---
	move.b	(a0)+,d1		; stack usage byte
	and.b	d1,d5			; align
	lea	(sp),a4
	adda.l	d5,sp			; reserve space on stack
.stk:
	move.b	(a0)+,-(a4)
	addq.b	#1,d1
	bne.b	.stk

	movea.w	#-$d00,a3		; constant for offset threshold

; ------------- DECOMPRESSION -------------

.decompr_literal:
	move.b	(a0)+,(a1)+

.decompr_loop:
	add.b	d0,d0
	bcc.b	.decompr_match
	bne.b	.decompr_literal
	move.b	(a0)+,d0
	addx.b	d0,d0
	bcs.b	.decompr_literal

.decompr_match:
	moveq	#-2,d1
.decompr_gamma_1:
	add.b	d0,d0
	bne.b	.g_1
	move.b	(a0)+,d0
	addx.b	d0,d0
.g_1:
	addx.w	d1,d1

	add.b	d0,d0
	bcc.b	.decompr_gamma_1
	bne.b	.decompr_select
	move.b	(a0)+,d0
	addx.b	d0,d0
	bcc.b	.decompr_gamma_1

.decompr_select:
	addq.w	#3,d1
	beq.b	.decompr_get_mlen
	bpl.b	.decompr_exit_token
	lsl.l	#8,d1
	move.b	(a0)+,d1
	move.l	d1,d3

.decompr_get_mlen:
	add.b	d0,d0
	bne.b	.e_1
	move.b	(a0)+,d0
	addx.b	d0,d0
.e_1:
	addx.w	d2,d2
	add.b	d0,d0
	bne.b	.e_2
	move.b	(a0)+,d0
	addx.b	d0,d0
.e_2:
	addx.w	d2,d2

	lea	(a1,d3.l),a2
	addq.w	#2,d2
	bgt.b	.decompr_gamma_2

.decompr_tiny_mlen:
	move.l	d3,d1
	sub.l	a3,d1
	addx.w	d4,d2

.L_copy2:
	move.b	(a2)+,(a1)+
.L_copy1:
	move.b	(a2)+,(a1)+
	dbra	d2,.L_copy1
.L_rep:
	bra.b	.decompr_loop

.decompr_gamma_2:
	add.b	d0,d0
	bne.b	.g_2
	move.b	(a0)+,d0
	addx.b	d0,d0
.g_2:
	addx.w	d2,d2
	add.b	d0,d0
	bcc.b	.decompr_gamma_2
	bne.b	.decompr_large_mlen
	move.b	(a0)+,d0
	addx.b	d0,d0
	bcc.b	.decompr_gamma_2

.decompr_large_mlen:
	move.b	(a2)+,(a1)+
	move.b	(a2)+,(a1)+
	cmp.l	a3,d3
	bcs.b	.L_copy2
	move.b	(a2)+,(a1)+
	dbra	d2,.L_copy1

.decompr_exit_token:
	lea	(a4),a0
	bclr	d2,d2
	bne.b	.L_rep

	suba.l	d5,sp
	movem.l	(sp)+,d0-d5/a0-a4
	rts