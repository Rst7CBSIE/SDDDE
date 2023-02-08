	.include	"render.s"

	.text
	.align	4
	.globl	ClearChunkyScreen
	.type	ClearChunkyScreen, @function
ClearChunkyScreen:
	move.l	4(a7),a0
	move.w	#16000/4-1,d0
	move.l	#0x3C3C3C3C,d1		| R=1 G=1 B=2
1:
	move.l	d1,(a0)+
	dbra	d0,1b

	.align	4
	.globl	ZeroChunkyScreen
	.type	ZeroChunkyScreen, @function
ZeroChunkyScreen:
	move.l	4(a7),a0
	move.w	#16000/4-1,d0
	moveq.l	#0,d1
1:
	move.l	d1,(a0)+
	dbra	d0,1b

	.align	4
	.globl	XtabCorrect_ys
	.type	XtabCorrect_ys, @function
XtabCorrect_ys:	
	.if unsigned_sy
	sub.w	#30*2,a0
	.endif
	rts

	.align	4
	.globl	RICOUNTER
	.type	RICOUNTER, @function
RICOUNTER:
	move.l	#0xbfd800,a0
	move.b	0x200(a0),d0
	swap	d0
	move.b	0x100(a0),d0
	lsl.w	#8,d0
	move.b	(a0),d0
	lsl.l	#8,d0	
	rts

	.align	4
	.globl	PrepareSortFaces
	.type	PrepareSortFaces, @function
PrepareSortFaces:
	lea	sort_q1(pc),a0
	lea	sort_q2(pc),a1
	moveq	#32-1,d0
	moveq	#0,d1
1:
	move.l	d1,(a0)+
	move.l	d1,(a1)+
	dbra	d0,1b
	rts

	.align	4
	.globl	RotateMTXasm
	.type	RotateMTXasm, @function
RotateMTXasm:
|A0 - RM
|D0 - YAW
|D1 - PITCH
|D2 - ROLL
|D3 - t0
|D4 - t1
|A1 - t2
|D5,D6 - scratch
|D7 - counter
	moveq.l	#1,d7
	move.l	#0x8000,a2
RotateMTXloop:
	movem.w	(a0),d3-d4/a1
| RM[0] = t0 + FNORM(FMUL(t1,ROLL) - FMUL(t2,YAW));
	move.w	d4,d5
	muls.w	d2,d5
	move.w	a1,d6
	muls.w	d0,d6
	sub.l	d6,d5
	add.l	d5,d5
	add.l	a2,d5
	swap	d5
	add.w	d3,d5
	move.w	d5,(a0)+
| RM[1] = t1 - FNORM(FMUL(t0,ROLL) + FMUL(t2,PITCH));
	move.w	d3,d5
	muls.w	d2,d5
	move.w	a1,d6
	muls.w	d1,d6
	add.l	d6,d5
	add.l	d5,d5
	add.l	a2,d5
	swap	d5
	move.w	d4,d6			|save t1
	sub.w	d5,d4
	move.w	d4,(a0)+
| RM[2] = t2 + FNORM(FMUL(t0,YAW) + FMUL(t1,PITCH));
	muls.w	d0,d3
	muls.w	d1,d6
	add.l	d3,d6
	add.l	d6,d6
	add.l	a2,d6
	swap	d6
	add.w	a1,d6
	move.w	d6,(a0)+
	dbra	d7,RotateMTXloop
	rts
|
	.align	4
	.globl	NormalizeMTXasm
	.type	NormalizeMTXasm, @function
NormalizeMTXasm:
|A0 - RM
|D0 - RM[0]
|D1 - RM[1]
|D2 - RM[2]
|D3 - RM[3]
|D4 - RM[4]
|D5 - RM[5]
|D6 - scratch
|D7	- scratch
|A1 - scratch
	move.l	#0x8000,a2
	move.l	#0x2000,a3
	movem.w	(a0),d0-d5
|    r = RM[0] * RM[3] + RM[1] * RM[4] + RM[2] * RM[5];
	move.w	d0,d6
	muls.w	d3,d6
	move.w	d1,d7
	muls.w	d4,d7
	add.l	d7,d6
	move.w	d2,d7
	muls.w	d5,d7
	add.l	d7,d6
|    r = -r;
	neg.l	d6
|	 r>>=15;
	add.l	d6,d6
	add.l	a2,d6
	swap	d6			|d6=r
	moveq	#14,d7
| RM[0] += RM[3] * r >> 14;
| RM[3] += RM[0] * r >> 14;
	move.w	d0,a1
	muls.w	d6,d0
	add.l	a3,d0
	asr.l	d7,d0
	add.w	d3,d0
	muls.w	d6,d3
	add.l	a3,d3
	asr.l	d7,d3
	add.w	a1,d3			| Now d0 - RM[3], d3 - RM[0]
| RM[1] += RM[4] * r >> 14;
| RM[4] += RM[1] * r >> 14;
	move.w	d1,a1
	muls.w	d6,d1
	add.l	a3,d1
	asr.l	d7,d1
	add.w	d4,d1
	muls.w	d6,d4
	add.l	a3,d4
	asr.l	d7,d4
	add.w	a1,d4			| Now d1 - RM[4], d4 - RM[1]
| RM[2] += RM[5] * r >> 14;
| RM[5] += RM[2] * r >> 14;
	move.w	d2,a1
	muls.w	d6,d2
	add.l	a3,d2
	asr.l	d7,d2
	add.w	d5,d2
	muls.w	d6,d5
	add.l	a3,d5
	asr.l	d7,d5
	add.w	a1,d5			| Now d2 - RM[5], d5 - RM[2]
|D0 - RM[3]
|D1 - RM[4]
|D2 - RM[5]
|D3 - RM[0]
|D4 - RM[1]
|D5 - RM[2]
|    r = RM[0] * RM[0] + RM[1] * RM[1] + RM[2] * RM[2];
	move.w	d3,d6
	muls.w	d3,d6
	move.w	d4,d7
	muls.w	d4,d7
	add.l	d7,d6
	move.w	d5,d7
	muls.w	d5,d7
	add.l	d7,d6
|	 r>>=14;
|    r = 16384 * 3 / 2 - r / 2;
	add.l	d6,d6
	add.l	a2,d6
	swap	d6
	move.w	#16384*3/2,d7
	sub.w	d6,d7
	moveq	#14,d6
|    RM[0] = RM[0] * r >> 14;
	muls.w	d7,d3
	add.l	a3,d3
	asr.l	d6,d3
	move.w	d3,(a0)+
|    RM[1] = RM[1] * r >> 14;
	muls.w	d7,d4
	add.l	a3,d4
	asr.l	d6,d4
	move.w	d4,(a0)+
|    RM[2] = RM[2] * r >> 14;
	muls.w	d7,d5
	add.l	a3,d5
	asr.l	d6,d5
	move.w	d5,(a0)+
|    r = RM[3] * RM[3] + RM[4] * RM[4] + RM[5] * RM[5];
	move.w	d0,d6
	muls.w	d0,d6
	move.w	d1,d7
	muls.w	d1,d7
	add.l	d7,d6
	move.w	d2,d7
	muls.w	d2,d7
	add.l	d7,d6
|	 r>>=14;
|    r = 16384 * 3 / 2 - r / 2;
	add.l	d6,d6
	add.l	a2,d6
	swap	d6
	move.w	#16384*3/2,d7
	sub.w	d6,d7
	moveq	#14,d6
|    RM[3] = RM[3] * r >> 14;
	muls.w	d7,d0
	add.l	a3,d0
	asr.l	d6,d0
	move.w	d0,(a0)+
|    RM[4] = RM[4] * r >> 14;
	muls.w	d7,d1
	add.l	a3,d1
	asr.l	d6,d1
	move.w	d1,(a0)+
|    RM[5] = RM[5] * r >> 14;
	muls.w	d7,d2
	add.l	a3,d2
	asr.l	d6,d2
	move.w	d2,(a0)+
|    RM[6] = (RM[1] * RM[5] - RM[2] * RM[4]) >> 14;
	move.w	d4,d6
	muls.w	d2,d6
	move.w	d5,d7
	muls.w	d1,d7
	sub.l	d7,d6
	moveq	#14,d7
	add.l	a3,d6
	asr.l	d7,d6
	move.w	d6,(a0)+
|    RM[7] = (RM[2] * RM[3] - RM[0] * RM[5]) >> 14;
	move.w	d5,d6
	muls.w	d0,d6
	move.w	d3,d7
	muls.w	d2,d7
	sub.l	d7,d6
	moveq	#14,d7
	add.l	a3,d6
	asr.l	d7,d6
	move.w	d6,(a0)+
|    RM[8] = (RM[0] * RM[4] - RM[1] * RM[3]) >> 14;
	muls.w	d1,d3
	muls.w	d0,d4
	sub.l	d4,d3
	add.l	a3,d3
	asr.l	d7,d3
	move.w	d3,(a0)+
	rts

	.align	4
	.globl	ConstructBMATRIX_asm
	.type	ConstructBMATRIX_asm, @function
BMD_VAL =	1048576/3

|D0 - scratch
|D1 - coord
|D2:D3 - X and dX	
|D5:D4 - Y and dY
|D7:D6 - Z and dZ
|A0 - *D
|A1 - *matrix
|A2 - *camera_pos
|A3 - scratch
|A4 - counter
bm_sz_bits	=	4
ConstructBMATRIX_asm:
	lea 	(a0,12*(128-(1<<bm_sz_bits))),a0
	lea	12(a2),a4
ConstructBMATRIX_cloop:
	move.l	(a2)+,d1
| X
| dX = m[0];
	move.w	(a1)+,a3
	move.l	a3,d3
|        X = dX >> 1;
	move.l	d3,d2
	asr.l	#1,d2
|        X -= dX << 7;
	move.l	d3,d0
	lsl.l	#bm_sz_bits,d0
	sub.l	d0,d2
|        X -= (dX * cc[j]) >> 9;
	move.l	d3,d0
	muls.w	d1,d0
	asr.l	#2,d0
	asr.l	#8,d0
	sub.l	d0,d2
| Y
	move.w	4(a1),d5
	muls.w	#410,d5
	asr.l	#8,d5
	move.l	d5,d4
	asr.l	#1,d4
	move.l	d5,d0
	lsl.l	#bm_sz_bits,d0
	sub.l	d0,d4
	move.l	d5,d0
	muls.w	d1,d0
	asr.l	#2,d0
	asr.l	#8,d0
	sub.l	d0,d4
| Z
	move.w	10(a1),a3
	move.l	a3,d7
	move.l	d7,d6
	asr.l	#1,d6
	move.l	d7,d0
	lsl.l	#bm_sz_bits,d0
	sub.l	d0,d6
	muls.w	d7,d1
	asr.l	#2,d1
	asr.l	#8,d1
	sub.l	d1,d6
	add.l	#14189 / 3,d6
|
	move.w	#(2<<bm_sz_bits)-1,d1
ConstructBMATRIX_bloop:
	move.l	d6,(a0)+
	add.l	d7,d6
	move.l	d4,d0
	move.l	d2,(a0)+
	add.l	d5,d4
	add.l	d3,d2
	move.l	d0,(a0)+
	dbra	d1,ConstructBMATRIX_bloop
	lea 	(a0,12*(256-(2<<bm_sz_bits))),a0
	cmp.l	a2,a4
	bne.w	ConstructBMATRIX_cloop	
	rts

	.align	4
	.globl	FindKDtreeNode_asm
	.type	FindKDtreeNode_asm, @function
FindKDtreeNode_asm:
	moveq	#9+1,d3
	lsr.l	d3,d0
	lsr.l	d3,d1
	lsr.l	d3,d2
	move.l	a0,a1			| save kd_tree root
	move.w	#0x100,d5
	move.w	#0x200,d6
1:
	move.l	(a0),d3
	move.l	d3,d4
	swap	d4
|31:16 - index
|15:8 - axis
|7:0 - split
	sub.w	d6,d3
	bcs.s	2f
| axis 2 or 3, current 0/1
	sub.w	d5,d3
	bcc.s	4f
| axis 2
	lea	(a1,d4.w*8),a0
	cmp.b	d3,d2
	blt.s	1b
	addq	#4,a0
	bra.s	1b
2:
| axis 0 or 1, current 0xFE../0xFF00
	add.w	d5,d3
	bcs.s	3f
| axis 0
	lea	(a1,d4.w*8),a0
	cmp.b	d3,d0
	blt.s	1b
	addq	#4,a0
	bra.s	1b
| axis 1
3:
	lea	(a1,d4.w*8),a0
	cmp.b	d3,d1
	blt.s	1b
	addq	#4,a0
	bra.s	1b
4:
	rts

	.align	4
	.globl	DecrunchPVS_asm
	.type	DecrunchPVS_asm, @function
DecrunchPVS_asm:
	movem.l	d2-d6/a3-a6,-(a7)
|
| A0 - list 
| A1 - *s
| A2 - *leaf
| A3...A6 - scratch
| D0...D2 - scratch
| D3 - b
| D4 - l
| D5 - Z correction
| D7 - seq
|
	.macro	DecrunchPVSleaf
	movem.l	(a2),a3-a6		| load I,J,K,*facelist and leaf++
	move.l	(a3)+,d0
	add.l	(a4)+,d0
	add.l	(a5)+,d0
	bmi.s	2f
	move.l	(a3)+,d1
	add.l	(a4)+,d1
	add.l	(a5)+,d1
	cmp.l	d1,d0
	bmi.s	2f
	add.l	d0,d1
	bmi.s	2f
	move.l	(a3),d1
	add.l	(a4),d1
	add.l	(a5),d1
	add.l	d5,d0
	cmp.l	d1,d0
	bmi.s	2f
	add.l	d0,d1
	bmi.s	2f
	move.l	28(a2),a5		| Last element in FaceList
3:
	move.l	(a6)+,a3
	cmp.l	TFACE.seq(a3),d7
	beq.s	1f
	move.l	d7,TFACE.seq(a3)
	move.l	a0,TFACE.next(a3)
	move.l	a3,a0
1:
	cmp.l	a6,a5
	bne.s	3b
2:
	.endm
|
	move.l	#(160 * 14189 / 100) - 14189,d5
	moveq	#0,d3
	move.b	(a1)+,d3
	bne.w	DecrunchPVS_loop
	bra.w	DecrunchPVS_end
	.align	4
DecrunchPVS_rle1:
	cmp.b	#0x3F,d3
	bcs.s	DecrunchPVS_rle1_short
	move.w	(a1)+,d3
	ror.w	#8,d3
DecrunchPVS_rle1_short:
	DecrunchPVSleaf
	lea		16(a2),a2
	subq.w	#1,d3
	bne.s	DecrunchPVS_rle1_short
	move.b	(a1)+,d3
	bne.s	DecrunchPVS_loop
	bra.w	DecrunchPVS_end
DecrunchPVS_rle:
	lsr.b	#1,d3
	bcs.s	DecrunchPVS_rle1
	cmp.b	#0x3F,d3
	bcs.s	DecrunchPVS_rle0_short
	move.w	(a1)+,d3
	ror.w	#8,d3
DecrunchPVS_rle0_short:
	add.w	d3,d3
	lea		(a2,d3*8),a2
	moveq	#0,d3
	move.b	(a1)+,d3
DecrunchPVS_loop:
	bpl.s	DecrunchPVS_rle
DecrunchPVS_bmp:
	lsr.b	#1,d3
	bcc.s	DecrunchPVS_bmp1
	DecrunchPVSleaf
DecrunchPVS_bmp1:
	lea		16(a2),a2
	cmp.b	#1,d3
	bne.s	DecrunchPVS_bmp
	move.b	(a1)+,d3
	bne.s	DecrunchPVS_loop
DecrunchPVS_end:
	movem.l	(a7)+,d2-d6/a3-a6
	rts

	.align	4
	.globl	PrepareWorldRotation_asm
	.type	PrepareWorldRotation_asm, @function
PrepareWorldRotation_asm:
	move.w	#0x3FFF,d1
	move.w	#-0x3FFF,d3
	moveq	#9-1,d2
	lea		rmtx_multiplier+2(pc),a1
3:
	move.w	(a0)+,d0
	cmp.w	d1,d0
	ble.s	1f
	move.w	d1,d0
1:
	cmp.w	d3,d0
	bge.s	2f
	move.w	d3,d0
2:
	add.w	d0,d0
	move.w	d0,(a1)
	addq.l	#4,a1
	neg.w	d0
	move.w	d0,(a1)
	addq.l	#4,a1
	dbra	d2,3b
	rts


	.align	4
	.globl	RotateWorld_asm
	.type	RotateWorld_asm, @function
RotateWorld_asm:
	Profile	11,d0
| a0 - sqr_tab
| d4/d5/d6 - CamX/CamY/CamZ
| a5 - output faces
| a6 - input faces
	.if	enable_distance_test
	.else
| Switch to square table base
| d4 - CamX-sqr_tab
| d5 - CamY-sqr_tab
| d6 - CamZ-sqr_tab
	sub.l	a0,d4
	sub.l	a0,d5
	sub.l	a0,d6
	.endif
	.if		enable_backface_test
NcheckFaceLoop:
| MX,MY,MZ stored as 23.8, multiplied by 2
	move.l	TFACE.next(a6),a4
	movem.l	TFACE.MX(a6),a1/a2/a3			| MX,MY,MZ
	sub.l	d4,a1
	sub.l	d5,a2
	sub.l	d6,a3
	.if		enable_distance_test
| check max distance
	cmp.w	a1,a1
	bne.s	skip_face_stage0
	cmp.w	a2,a2
	bne.s	skip_face_stage0
	cmp.w	a3,a3
	bne.s	skip_face_stage0
| switch to table base
	add.l	a0,a1
	add.l	a0,a2
	add.l	a0,a3
	.endif
.if 1
	movem.w	TFACE.A14(a6),d1/d2/d3
	add.l	d1,a1
	add.l	d2,a2
	add.l	d3,a3
	add.l	d3,d3
	move.w	(a1),d0
	sub.l	d1,a1
	add.w	(a2),d0
	sub.l	d1,a1
	add.w	(a3),d0
	sub.l	d2,a2
	sub.w	(a1),d0
	sub.l	d2,a2
	sub.w	(a2),d0
	sub.l	d3,a3
	sub.w	(a3),d0
.else
|	movem.w	TFACE.A14(a6),d1/d2/d3
	movem.l	TFACE.flags(a6),d1/d3
	move.l	d3,d2
	swap	d2
	ext.l	d3
	add.w	d1,a1
	add.w	d2,a2
	add.w	d3,a3
	add.l	d3,d3
	move.w	(a1),d0
	sub.w	d1,a1
	add.w	(a2),d0
	sub.w	d1,a1
	add.w	(a3),d0
	sub.w	d2,a2
	sub.w	(a1),d0
	sub.w	d2,a2
	sub.w	(a2),d0
	sub.l	d3,a3
	sub.w	(a3),d0
.endif
	bpl.s	skip_face_stage0
	move.l	a5,TFACE.next(a6)			| add face to queue
	move.l	a6,a5
skip_face_stage0:
	move.l	a4,a6
	tst.l	a6
	bne.s	NcheckFaceLoop
	.else
	move.l	a6,a5
	.endif
|=================================================================================================
| Stage 1
|=================================================================================================
	Profile	12,d0
|
|a5 - face
	tst.l	a5
	beq.w	NoUpdateFace_stage2
|a6 - output faces queue
|
	sub.l	a6,a6
| Switch to square table base
| d4 - CamX-sqr_tab
| d5 - CamY-sqr_tab
| d6 - CamZ-sqr_tab
	.if	enable_distance_test
	sub.l	a0,d4
	sub.l	a0,d5
	sub.l	a0,d6
	.endif
UpdateFaceLoop:
|a4 - vertex
	lea	TFACE.vertexes(a5),a4
	move.l	TVERTEX.p(a4),a0
UpdateVertexLoop:
|a0 - point
	cmp.l	FPXYZ.seq(a0),d7
	bne.s	PointCoordCompute
PointCoordOk:
	addq.l	#TVERTEXsz,a4
	move.l	TVERTEX.p(a4),a0
	tst.l	a0
	bne.s	UpdateVertexLoop
	move.l	TFACE.next(a5),d0
	move.l	a6,TFACE.next(a5)	| добавляем грань в очередь
	move.l	a5,a6
	move.l	d0,a5
	tst.l	a5
	bne.s	UpdateFaceLoop
	bra.w	UpdateFace_stage2
|======================================================================================
PointCoordCompute:
|X,Y,Z мы храним в формате 23.8, уже *2
	movem.l	FPXYZ.X(a0),a1/a2/a3	| X,Y,Z
	sub.l	d4,a1
	sub.l	d5,a2
	sub.l	d6,a3
|Matrix multiply (self-modified code)
rmtx_multiplier:
	move.w	12345(a1),d0
	sub.w	-12345(a1),d0
	add.w	12345(a2),d0
	sub.w	-12345(a2),d0
	add.w	12345(a3),d0
	sub.w	-12345(a3),d0
	move.w	12345(a1),d1
	sub.w	-12345(a1),d1
	add.w	12345(a2),d1
	sub.w	-12345(a2),d1
	add.w	12345(a3),d1
	sub.w	-12345(a3),d1
	move.w	12345(a1),d2
	sub.w	-12345(a1),d2
	add.w	12345(a2),d2
	sub.w	-12345(a2),d2
	add.w	12345(a3),d2
	sub.w	-12345(a3),d2
	move.w	d2,FPXYZ.z(a0)				| store x,y,z
| Проверяем z на минимально допустимую
	moveq	#0,d3						|flag=0
	cmp.w	#51,d2
	bge.s	min_z8_ok					|d2>=MIN_Z8
	moveq	#0,d2						|z=0
	moveq	#1,d3						|flag=XYZF_SCOORD_NOT_VALID
min_z8_ok:
	move.w	d0,FPXYZ.x(a0)
	move.l	#_LOGtab+32768,a1
	move.l	#EXPtab+65536,a2
	move.w	d1,FPXYZ.y(a0)
	move.w	(a1,d2.w*2),d2				|log(z)
	move.l	d7,FPXYZ.seq(a0)			| update seq
	move.w	(a1,d0.w*2),d0				|log(x)
	beq.s	no_clip_xz					|log(x)==0
	sub.w	d2,d0
	bcs.s	no_clip_xz					|log(x)<log(z)
	moveq	#1,d3						|Flag=XYZF_SCOORD_NOT_VALID	
	or.w	#0xFFFE,d0
no_clip_xz:
	move.w	(a2,d0.w*2),FPXYZ.sx(a0)
	move.w	(a1,d1.w*2),d1				|log(y)
	beq.s	no_clip_yz
	sub.w	d2,d1
	bcc.s	clip_yz						|log(y)<log(z)
	cmp.w	#LOG5_8,d1
	bcs.s	no_clip_yz					| <clip_y_factor
clip_yz:
	moveq	#1,d3						|Flag=XYZF_SCOORD_NOT_VALID	
	and.w	#0x0001,d1
	or.w	#LOG5_8,d1
no_clip_yz:
	move.w	d3,FPXYZ.flags(a0)
	.if unsigned_sy
	move.w	(a2,d1.w*2),FPXYZ.sy(a0)
	.else
	move.w	(a2,d1.w*2),d1				|zoom*exp(log(y)-log(z))+XDISP_FP
	add.w	#YDISP_FP-XDISP_FP,d1		|correct y disp
|	bmi.s	1f
	move.w	d1,FPXYZ.sy(a0)
	.endif
	bra.w	PointCoordOk
1:
	move.l	d1,d1
	bra.s	1b
|======================================================================================
| Stage 2
|======================================================================================
NoUpdateFace_stage2:
	rts
UpdateFace_stage2:
	Profile	13,d0
	move.l	d4,a3
	move.l	d5,a4
	move.l	d6,a5
| a6 - face
	tst.l	a6
	beq.s	NoUpdateFace_stage2
|d1 - llllbbbb bound
|d2 - rrrrtttt bound
	.if unsigned_sy
	move.l	#0x9FFD63FD+0x1E00,d1
	move.l	#0x00020002+0x1E00,d2
	.else
	move.l	#0x9FFD63FD,d1
	move.l	#0x00020002,d2
	.endif
	bra.s	UpdateFaceLoop_stage2
skip_face:
	move.l	TFACE.next(a6),a6
	tst.l	a6
	beq.s	NoUpdateFace_stage2
UpdateFaceLoop_stage2:
	moveq	#0,d3
	moveq.l	#-1,d4				|LLLLBBBB=0xFFFFFFFF
	moveq.l	#0,d5				|RRRRTTTT=0x00000000
	moveq.l	#-1,d6				|max_z=-1
|d3 - flag
|d4 - LLLLBBBB
|d5 - RRRRTTTT
|d6 - ZZZZFFFF, Z - max_z, F - flags
	lea	TFACE.vertexes(a6),a1
	move.l	TVERTEX.p(a1),a0
UpdateVertexLoop_stage2:
	move.l	FPXYZ.z(a0),d0				| load z and flags
|Test max_z
	cmp.l	d0,d6
	bge.s	no_max_z
	move.l	d0,d6
no_max_z:
	or.w	d0,d3					| Accumulate flags
|
	move.l	FPXYZ.sx(a0),d0				| load sx,sy
|d0 - XXXXYYYY
|Compute box
	cmp.w	d0,d4			| BBBB-YYYY
	bcs.s	box_b_ok
	move.w	d0,d4
box_b_ok:
	cmp.w	d0,d5			| TTTT-YYYY
	bcc.s	box_t_ok
	move.w	d0,d5
box_t_ok:
	move.w	d4,d0			| d0=XXXXBBBB
	cmp.l	d0,d4			| LLLLBBBB-XXXXBBBB
	bcs.s	box_l_ok
	move.l	d0,d4
box_l_ok:
	move.w	d5,d0			| d0=XXXXTTTT
	cmp.l	d0,d5			| RRRRTTTT-XXXXTTTT
	bcc.s	box_r_ok
	move.l	d0,d5
box_r_ok:
| Next vertex
	addq.l	#TVERTEXsz,a1
	move.l	TVERTEX.p(a1),a0
	tst.l	a0
	bne.s	UpdateVertexLoop_stage2
	move.w	d3,TFACE.flags(a6)			| store flags to face
| if (max_z<0) drop
	tst.l	d6
	bmi.s	skip_face
| Test screen bounds
	cmp.w	d4,d1					| bbbb-BBBB
	bcs.s	skip_face				| exit if BBBB>bbbb
	cmp.l	d4,d1					| llllbbbb - LLLLBBBB, BBBB<=bbbb
	bcs.s	skip_face				| exit if LLLL>llll
	cmp.w	d2,d5					| TTTT-tttt
	bcs.s	skip_face				| exit if TTTT<tttt
	cmp.l	d2,d5					| RRRRTTTT - rrrrtttt, TTTT>=tttt
	bcs.s	skip_face				| exit if RRRR<rrrr
| Compute z_avg and place in sort_q1
	movem.l	TFACE.MX(a6),a0/a1/a2			| MX,MY,MZ
	sub.l	a3,a0
	move.w	(a0),d0
	sub.l	a4,a1
	add.w	(a1),d0
	sub.l	a5,a2
	add.w	(a2),d0
	move.w	d0,TFACE.avg_z(a6)
	andi.w	#0x1F,d0
	lea	sort_q1(pc,d0.w*4),a0
	move.l	TFACE.next(a6),a1
	move.l	(a0),TFACE.next(a6)
	move.l	a6,(a0)
	move.l	a1,a6
	tst.l	a6
	bne.w	UpdateFaceLoop_stage2
	rts
	.align	4
	.globl	sort_q1
sort_q1:
	ds.l	128
	.align	4
	.globl	SortFaces
	.type	SortFaces, @function
SortFaces:
	moveq	#127,d0
	lea	sort_q1(pc),a0
	movem.l	a2,-(a7)
1:
	move.l	(a0)+,d1
	beq.s	2f
3:
	move.l	d1,a1
	move.w	TFACE.avg_z(a1),d1		|avg_z
	lsr.w	#5,d1
	lea	sort_q2(pc,d1.w*4),a2
	move.l	TFACE.next(a1),d1		|tnext
	move.l	(a2),TFACE.next(a1)
	move.l	a1,(a2)
	tst.l	d1
	bne.s	3b
2:
	dbra	d0,1b
	movem.l	(a7)+,a2
	rts
	.align	4
	.globl	sort_q2
sort_q2:
	ds.l	128







