	.include "render.s"

	.text


	.macro	LJMP function
	move.l	#12345678f,\function\()_LINK_REGISTER
	jmp		\function
12345678:
	.endm

|===================================================================================
| Textured faces frustrum culling
|===================================================================================
	.align	4
tculling_qhead:
	dc.l	0
TCULLING_LINK_REGISTER:
	dc.l	0
|
TCULLING:
|a7 - RVERTEX pool
|a0 - rface
|a1 - fout
|a2 - vs1 / &vs1->U
|a3 - vs2 / &vs2->U
|a4 - &newv->U
|a6 - vlast
|d0 - t1
|d1 - t2
|d2 - scratch
|d3 - scratch
|d5 - planes count
|d7 - save t2
|
	Profile 17,d0
|	lea		tculling_qhead-RFACE.next(pc),a1		|point fout to head pointer
	bra.s	tculling_faces_loop	
	.align	4
tculling_faces_loop_end:
	clr.l	RFACE.next(a1)
	move.l	TCULLING_LINK_REGISTER(pc),a0
	jmp	(a0)
|
	.align	4
tculling_accept_face:
	move.l	a0,RFACE.next(a1)
	move.l	a0,a1				| fout=rface
tculling_next_face:
	move.l	RFACE.next(a0),a0
	tst.l	a0
	beq.s	tculling_faces_loop_end
tculling_faces_loop:
	tst.b	RFACE.flagsL(a0)			|check only low 8 bits
	beq.s	tculling_accept_face
	move.l	RFACE.vertex(a0),a6		|
	move.l	RVERTEX.vp(a6),a6		|vlast=rface->vertex->prev
	moveq	#5*3,d5
tculling_planes_loop:
	move.l	a6,a3					|vs2=vlast
	sub.l	a2,a2					|vs1=NULL
	bra.s	tculling_loop
tculling_prep_t1:
	move.w	d1,d7
	move.l	a3,a2					|vs1=vs2
	move.l	RFACE.vertex(a0),a3		|vs2=rfacce->vertex
	move.l	a0,a6					|vlast=(RVERTEX*)rface
tculling_loop:
	move.w	RVERTEX.z(a3),d1
	cmpi.w	#3*3,d5
	bcc.s	tculling_t2_no58
|	move.w	tculling_table_58(pc,d1.w*2),d1
	move.w	d1,d2
	lsr.w	#2,d2
	add.w	d2,d1
	addq.w	#1,d1
	lsr.w	#1,d1
tculling_t2_no58:
	jmp	tculling_t2_0-6(pc,d5*2)
tculling_t2_0:
	sub.w	RVERTEX.y(a3),d1
	bra.s	tculling_t2_ok
tculling_t2_1:
	add.w	RVERTEX.y(a3),d1
	bra.s	tculling_t2_ok
tculling_t2_2:
	sub.w	RVERTEX.x(a3),d1
	bra.s	tculling_t2_ok
tculling_t2_3:
	add.w	RVERTEX.x(a3),d1
	bra.s	tculling_t2_ok
tculling_t2_4:
	subi.w	#51,d1
tculling_t2_ok:
	tst.l	a2
	beq.s	tculling_prep_t1
	move.w	d7,d0            
	move.w	d1,d7				|save t2, coz if t2>=0 then ADD_VERTEX(vd,vs2)
	move.w	d0,d2
	eor.w	d1,d2
	bpl.s	no_tculling
| Allocate RVERTEX
	move.l	(a7)+,a4			| Get vertex from pool
	moveq	#5-1,d3
| ADD_VERTEX(vd,newv)|
	move.l	a4,RVERTEX.vn(a6)	| last->vn=newv	
	move.l	a4,a6				| last=newv
	moveq	#RVERTEX.U,d2
	add.l	d2,a2
	add.l	d2,a3
	add.l	d2,a4
	tst.w	d7
	bmi.s	tculling_no_rv
	exg	d0,d1
	exg	a2,a3
tculling_no_rv:
	sub.w	d0,d1
	neg.w	d1				| t2=t1-t2
	swap	d0
	clr.w	d0
|	ext.l	d1
|	add.l	d1,d0
	lsr.l	#1,d0				| t1<<=15
	divu.w	d1,d0				| t1=(t1+(t2>>1))/t2|
tculling_param_loop:	
	move.w	(a3)+,d1
	move.w	(a2)+,d2
	sub.w	d2,d1				| d = vs2->U - vs1->U|
	muls.w	d0,d1				| d = (d*t+0x4000)>>15|
	add.l	d1,d1
	swap	d1
|	bpl.s	tculling_no_round2
|	addq.w	#1,d1
|tculling_no_round2:
	add.w	d1,d2
	move.w	d2,(a4)+			| newv->U = vs1->U + P[d] - M[d]|
	dbra	d3,tculling_param_loop
	tst.w	d7
	bmi.s	tculling_no_rv2
	exg	a2,a3
tculling_no_rv2:
|	move.w	#0x0001,(a4)			| set "no scoords" flag and clear high flags
|	lea		-RVERTEX.flagH(a3),a3	| restore vs2 base
	moveq	#RVERTEX.flagH,d2
	move.w	d2,(a4)
	sub.l	d2,a3
no_tculling:
	tst.w	d7
	bmi.s	tculling_no_add_vs2
	move.l	a3,RVERTEX.vn(a6)	| last->vn=vs2	
	move.l	a3,a6				| last=vs2
	dc.w	0x0C40				| opcode cmpi.w	#xxxx,d0
tculling_no_add_vs2:
	move.l	a3,-(a7)			| Save vs2 to pool
	move.l	a3,a2				| vs1=vs2
	move.l	RVERTEX.vn(a3),a3
	tst.l	a3
	bne.w	tculling_loop
|
	cmp.l	a6,a0
	beq.s	tculling_skip_face			| if vlast==(RVERTEX*)f - skip face
	clr.l	RVERTEX.vn(a6)				| last->next=NULL
	subq.l	#3,d5
	bne.w	tculling_planes_loop
	move.l	RFACE.vertex(a0),a3
	move.l	a6,RVERTEX.vp(a3)	| rface->vertex->prev=last
	bra.w	tculling_accept_face
tculling_skip_face:
	move.l	a0,-(a7)			| return face to pool
	bra.w	tculling_next_face

|============================================================================================
| Render textured faces
|============================================================================================
	.align	4
TMAP_qhead:
	dc.l	0
TMAP_qtail:
	dc.l	0
TMAP:
|A0 - RFACE*
|A1 - tv
|A2 - bv
|A3 - xmax
|A4 - x
|A5 - flags
|A6
|A7 - wp
|D0 - tx
|D1 - bx
|D2 - Y
|D3 - dY
|D4 - U
|D5 - dU 
|D6 - V
|D7 - dV
	Profile 18,d0
	move.l	a0,a2
| Считаем экранные координаты, если они еще не готовы
| + Ищем самый верхний элемент (и самый нижний)
| + Полностью заполняем prev
	move.l	#_LOGtab+32768,a5
	move.l	#EXPtab+65536,a6
	bra.s	tmap_prep_loop
tmap_scoord_not_ok:
	movem.w	RVERTEX.x(a1),d4-d6
	move.w	(a5,d6.w*2),d6			|log(z)
	move.w	(a5,d4.w*2),d4			|log(x)
	beq.s	tmap_no_clip_xz			|log(x)==0
	sub.w	d6,d4
	bcs.s	tmap_no_clip_xz			|log(x)<log(z)
	or.w	#0xFFFE,d4
tmap_no_clip_xz:
	move.w	(a6,d4.w*2),RVERTEX.sx(a1)		|zoom*exp(log(x)-log(z))+XDISP_FP
	move.w	(a5,d5.w*2),d5			|log(y)
	beq.s	tmap_no_clip_yz
	sub.w	d6,d5
	bcc.s	tmap_clip_yz			|log(y)<log(z)
	cmp.w	#LOG5_8,d5
	bcs.s	tmap_no_clip_yz			| <clip_y_factor
tmap_clip_yz:
	and.w	#0x0001,d5
	or.w	#LOG5_8,d5
tmap_no_clip_yz:
	move.w	(a6,d5.w*2),RVERTEX.sy(a1)
	bra.s	tmap_scoord_ok
|1:
|	move.l	d5,d5
|	bra.s	1b
tmap_prep_loop:
	move.l	a2,-(a7)			|free face
	moveq	#0,d0				|xmax
	moveq	#-1,d1				|xmin
	moveq	#0,d3
	move.l	RFACE.vertex(a2),a1				|take *v
	move.l	RVERTEX.vp(a1),a4	|vlast=prev (этот всегда есть)
tmap_find_loop:
	tst.b	RVERTEX.flagL(a1)
	bne.s	tmap_scoord_not_ok
| Все, экранные координаты готовы
tmap_scoord_ok:
	move.b	RVERTEX.sx(a1),d3
	move.l	a1,-(a7)			|free vertex
	cmp.l	d3,d1
	bcs.s   tmap_not_min
	move.l	d3,d1
	move.l	a1,a3
tmap_not_min:
	move.l	a4,RVERTEX.vp(a1)	|store prev
	cmp.l	d3,d0
	bcc.s	tmap_not_max
	move.l	d3,d0
tmap_not_max:
	move.l	a1,a4				|vlast=v
	move.l	RVERTEX.vn(a1),a1
	tst.l	a1
	bne.s	tmap_find_loop
	move.l	RFACE.vertex(a2),RVERTEX.vn(a4)		| Cycle last vertex to first vertex
	move.l	a3,(a2)+			| Store top vertex as root
	addq.l	#4,a2				| skip T
	move.l	d0,(a2)+			| store xmax
	move.l	(a2),a2				| load next face
	tst.l	a2
	bne.s	tmap_prep_loop
	move.l	a7,RDataPool_top
	move.l	#TMAP_qhead-RFACE.next,TMAP_qtail		| reset queue
|	
	Profile 19,d0

	moveq	#0,d2
	move.l	TRAPEZOIDS_POINTER,a7
	bra.s	tmap_face_loop
error_y_ofv1:
	move.l	d2,d2
	bra.s	error_y_ofv1

tmap_face_loop:
	movem.l	(a0)+,a1/a2/a3			| v, T, xmax 
	move.l	a2,(a7)+			| Store *T
	move.l	a1,a2				| tv=bv=min vertex
	moveq	#0,d0
	move.b	RVERTEX.sx(a1),d0		| tx=tv->x
	move.l	d0,(a7)+			| Store x
	move.l	d0,d1				| bx=tx
	move.l	d0,a4				| xmin
tmap_loop:
	move.l	a7,a6				| for later store flags+size
	addq.l	#1,a4				| x++
	addq.l	#4,a7				| reserve for load flags and height
	sub.w	a5,a5                           | flags=0
	cmp.w	a4,d0				| if (tx<x)
	bcc.s	no_load_t
	move.w	#0x8000,a5			| T loaded
loop_load_t:
	move.w	RVERTEX.sy(a1),d2
	move.w	d0,d7				| w=tx
	move.l	RVERTEX.U(a1),d4
	cmp.w	a3,d0				| if (tx>=xmax) stop
	bcc.s	loop_load_t_stop
	move.l	RVERTEX.vp(a1),a1               | tv = tv->pv
	move.b	RVERTEX.sx(a1),d0		| tx = tv->sx >> 8
	cmp.w	a4,d0				| while(tx<x)
	bcs.s	loop_load_t
loop_load_t_stop:
	sub.w	d7,d0				| w=tx-w
	move.w	RVERTEX.sy(a1),d3
|	cmp.l	#0x6400,d2
|	bcc.s	error_y_ofv1
	sub.w	d2,d3
	move.l	RVERTEX.U(a1),d5
	sub.l	d4,d5
	move.l	InvAndJump_table+4(pc,d0.w*8),d6		| w=1/w
	move.l	d2,(a7)+
	muls.w	d6,d3
	move.l	d3,(a7)+
	move.w	d5,d3
	muls.w	d6,d3
	add.l	d3,d3
	move.l	d4,(a7)+
	swap	d5
	muls.w	d6,d5
	add.l	d5,d5
	swap	d3
	move.w	d3,d5
	move.l	d5,(a7)+
	add.w	d7,d0
no_load_t:
	cmp.w	a4,d1				| if (tx<x)
	bcc.s	no_load_b
	add.w	#0x4000,a5			| T loaded
loop_load_b:
	move.w	RVERTEX.sy(a2),d2
	move.w	d1,d7				| w=tx
	move.l	RVERTEX.U(a2),d4
	cmp.w	a3,d1				| if (tx>=xmax) stop
	bcc.s	loop_load_b_stop
	move.l	RVERTEX.vn(a2),a2               | tv = tv->pv
	move.b	RVERTEX.sx(a2),d1		| tx = tv->sx >> 8
	cmp.w	a4,d1				| while(tx<x)
	bcs.s	loop_load_b
loop_load_b_stop:
	sub.w	d7,d1				| w=tx-w
	move.w	RVERTEX.sy(a2),d3
|	cmp.l	#0x6400,d2
|	bcc.s	error_y_ofv2
	sub.w	d2,d3
	move.l	RVERTEX.U(a2),d5
	sub.l	d4,d5
	move.l	InvAndJump_table+4(pc,d1.w*8),d6		| w=1/w
	muls.w	d6,d3
	move.l	d3,(a7)+
	move.w	d5,d3
	muls.w	d6,d3
	move.l	d2,(a7)+
	add.l	d3,d3
	swap	d5
	muls.w	d6,d5
	move.l	d4,(a7)+
	add.l	d5,d5
	swap	d3
	move.w	d3,d5
	move.l	d5,(a7)+
	add.w	d7,d1
no_load_b:
	move.w	d1,d3
	cmp.w	d0,d3
	bcs.s	side_b_min
	move.w	d0,d3
side_b_min:
|d3 = next x
	moveq	#0,d4
	sub.w	a4,d3
	addx.w	d4,d3
	add.w	d3,a4
	swap	d3				| wwww ....
	move.w	a5,d3				| wwww flag
	move.l	d3,(a6)
	cmp.l	a3,a4
	bcc.s	tmap_stop
	cmp.l	a1,a2
	bne.w	tmap_loop
tmap_stop:
	cmp.l	#TRAPEZOIDS_BUFFER_THR,a7
	bcc.s	Trapezoids2slices
return_from_trapezoids_render2:
	move.l	RFACE.vertex(a0),a0
	tst.l	a0
	bne.w	tmap_face_loop
|tmap_end_of_loop:
|	....return to prev level....
	move.l	a7,TRAPEZOIDS_POINTER
	move.l	RDataPool_top(pc),a7
|tmap_exit:
	move.l	TMAP_LINK_REGISTER(pc),a0
	jmp	(a0)
return_from_trapezoids_render:
	moveq	#0,d2
	bra.s	return_from_trapezoids_render2	
error_y_ofv2:
	move.l	d2,d2
	bra.s	error_y_ofv2

	.align	4

TMAP_LINK_REGISTER:
	dc.l	0

	.globl	RDataPool_top
RDataPool_top:
	dc.l	0
|a7 - *out
|a6 - delta UBVB
|a5 - delta UTVT
|a4 - UBVB
|a3 - UTVT
|a2 - delta BY
|a1 - xtab
|a0 - *in
|d7 - delta TY
|d6 - BY
|d5 - TY
|d4 - scratch2
|d3 - inv_height
|d2 - height / scratch
|d1 - width
|d0 - scratch
Trapezoids2slices:
	Profile 20,d0
	move.l	a0,trapezoid_save_a0
	move.l	#0x80000000,(a7)			| End of trapezoids list
	move.l	SLICES_POINTER,a7
	move.l	#TRAPEZOIDS,a0
	move.l	(a0)+,d1
	add.l	d1,d1					| Старший бит тут отвечает за конец
	beq.w	trapezoid_end_of_loop
trapezoid_face_loop:
	move.l	d1,(a7)+				| *wp++=T
	movem.l	(a0)+,d0/d1				| Как раз инициализация movem спрячется под записью
|	lea	Xtab(pc,d0.w*4),a1			| a1=xtab+Xmin*4
	move.l	#0xDEADBEEF,a1
	.globl	Xtab_patch1
Xtab_patch1 = .-4
	lea	(a1,d0.w*4),a1		| xtab+Xmin*4
trapezoid_loop:
	add.w	d1,d1
	bcc.s	trapezoid_no_load_t
	movem.l	(a0)+,d5/d7/a3/a5
	lsl.l	#8,d5
	.if	enable_trapezoids_round
	asr.l	#8,d7
	add.l	d7,d5
	add.l	d7,d7
	move.l	a5,d2
	asr.l	#1,d2
	add.w	d2,d2
	bpl.s	trapezoid_no_corr_t
	add.l	#0xFFFF0000,a5
trapezoid_no_corr_t:
	asr.w	#1,d2
	add.l	d2,a3
	.else
	asr.l	#7,d7
	move.w	a5,d2
	bpl.s	trapezoid_no_corr_t
	add.l	#0xFFFF0000,a5
trapezoid_no_corr_t:	
	.endif
trapezoid_no_load_t:
	add.w	d1,d1
	bcc.s	trapezoid_no_load_b
	movem.l	(a0)+,d4/d6/a4/a6
	lsl.l	#8,d6
	.if enable_trapezoids_round
	asr.l	#8,d4
	add.l	d4,d6
	add.l	d4,d4
	move.l	d4,a2
	move.l	a6,d2
	asr.l	#1,d2
	add.w	d2,d2
	bpl.s	trapezoid_no_corr_b
	add.l	#0xFFFF0000,a6
trapezoid_no_corr_b:
	asr.w	#1,d2
	add.l	d2,a4
	.else
	asr.l	#7,d4
	move.l	d4,a2
	move.w	a6,d2
	bpl.s	trapezoid_no_corr_b
	add.l	#0xFFFF0000,a6
trapezoid_no_corr_b:	
	.endif
trapezoid_no_load_b:
	swap	d1					| width-1 !!!!!!
column_loop:
|	move.l	d5,d0					| t = ty >> 8
|	swap	d0
|	ext.l	d0
	bfextu	d5{0:16},d0
	move.l	d6,d2					| h = (by >> 8) - t
	swap	d2
	sub.w	d0,d2					| h=h-t
	bls.s	nextline				| if (h > 0)
	move.l	InvAndJump_table-8(pc,d2.w*8),(a7)+		| *wp++=jumper and 16T counter
	move.l	a4,d4		|UBVB
	move.l	InvAndJump_table+4(pc,d2.w*8),d3		| d3=inv_tab[h]
	.if		tmap_16bit
	add.l	d0,d0					| 
	add.l	(a1),d0
	move.l	a3,(a7)+				| *wp++==UTVT
	sub.l	a3,d4		|UTVT
	move.l	d4,d2
	muls.w	d3,d2
	add.l	d2,d2
	swap	d2
	swap	d4
	muls.w	d3,d4
	add.l	d4,d4
	move.w	d2,d4
	move.l	d4,(a7)+
	addq.l	#4,a1
	add.l	d7,d5
	add.l	a2,d6
	add.l	a5,a3
	add.l	a6,a4
	move.l	d0,(a7)+				| *wp++ = (*xtab++)+t*2|
	.else
	move.l	a3,(a7)+				| *wp++==UTVT
	sub.l	a3,d4		|UTVT
	move.l	d4,d2
	muls.w	d3,d2
	lsr.l	#7,d2
	move.l	d2,(a7)+
	swap	d4
	muls.w	d3,d4
	add.l	d4,d4
	move.l	d4,(a7)+
	add.l	d0,d0					| 
	add.l	d7,d5
	add.l	a2,d6
	add.l	(a1),d0
	move.l	d0,(a7)+				| *wp++ = (*xtab++)+t*2|
	addq.l	#4,a1
	add.l	a5,a3
	add.l	a6,a4
	.endif
nextline1:
	dbra	d1,column_loop
	cmp.l	#SLICES_BUFFER_THR,a7
	bcc.s	RenderSlices_X
return_from_slice_render_X:
	move.l	(a0)+,d1
	bpl.w	trapezoid_loop
	add.l	d1,d1
	bne.w	trapezoid_face_loop
	bra.s	trapezoid_end_of_loop
nextline:
	addq.l	#4,a1
	add.l	d7,d5
	add.l	a2,d6
	add.l	a5,a3
	add.l	a6,a4
	bra.s	nextline1
trapezoid_end_of_loop:
	move.l	a7,SLICES_POINTER
|	....return to prev level....
	move.l	#TRAPEZOIDS,a7
	move.l	trapezoid_save_a0(pc),a0
	Profile 21,d0
	move.l	TRAPEZOIDS_LINK_REGISTER,a1
	jmp	(a1)
RenderSlices_X:
	movem.l	d5-d6/a1-a2,trapezoid_save_regs
	bra.w	RenderSlices
return_from_slice_render:
	movem.l	trapezoid_save_regs,d5-d6/a1-a2
	bra.s	return_from_slice_render_X
	.align	4
trapezoid_save_regs:
	ds.l	4
	.align	4
trapezoid_save_a0:
	dc.l	0
	.align	4
	.globl	InvAndJump_table
InvAndJump_table:
	.set	s16_count,0
	.set    h_count,0
	
	.macro	inverse8
		.if (h_count > 1)
			dc.l	(0x8000+(h_count>>1))/h_count
		.else
			dc.l	0x7FFF
		.endif
		.set h_count,h_count+1
	.endm

	.macro	do_and_count lbl
	dc.l	((\lbl) & 0xFFFF) + ((0xFFFF-s16_count) << 16)
	.endm

	.rept	16

	do_and_count	do01T
	inverse8
	do_and_count	do02T
	inverse8
	do_and_count	do03T
	inverse8
	do_and_count	do04T
	inverse8
	do_and_count	do05T
	inverse8
	do_and_count	do06T
	inverse8
	do_and_count	do07T
	inverse8
	do_and_count	do08T
	inverse8
	do_and_count	do09T
	inverse8
	do_and_count	do10T
	inverse8
	do_and_count	do11T
	inverse8
	do_and_count	do12T
	inverse8
	do_and_count	do13T
	inverse8
	do_and_count	do14T
	inverse8
	do_and_count	do15T
	inverse8
	do_and_count	do16T
	inverse8

	.set	s16_count,s16_count+1
	.endr
|	.globl	Xtab
|Xtab:
|	ds.l	160
	
|a0 - saved
|a7 - stop addr in SLICE_BUFFER 
|
| slice cmd
| 0 - stop
| >0 - set TextureAddr
| <0 - set all params and jump to do1T...do16T (high 16 bit) and do16T (low 16 bit count)
| 	dc.l	UUuuVVvv, __dVdvdv, dUdudu__, D	
|
| d0-d6,a1,a2 - 
	.text
	.align	4
RenderSlices:
	Profile 22,d0
	move.l	SLICE_TEXTURE_ADDR,d4		| Restore saved TextureAddr
	clr.l	(a7)+
	move.l	#SLICES_BUFFER,a1
	moveq	#1,d5
	swap	d5
	bra.w	dotexels
RenderSlicesEnd:
	move.l	d4,SLICE_TEXTURE_ADDR
	move.l	#SLICES_BUFFER,a7
	Profile 23,d0
	move.l	SLICE_LINK_REGISTER,a1
	jmp	(a1)

	.macro	map_texel
		move.w	d1,d4
		move.b	d0,d4
		move.l	d4,a2
		move.b	(a2),(a7)+
		.if tmap_16bit
		add.l	d2,d1
		addx.b	d3,d0
		.else
		addx.l	d2,d1
		addx.l	d3,d0
		.endif
	.endm

	.if	rearrange_tmap_innerloop
	.macro	map_texel_a
		move.w	d1,d4
		move.b	d0,d4
		move.l	d4,a2
			.if tmap_16bit
		add.l	d2,d1
		move.b	(a2),(a7)+
		addx.b	d3,d0
		.else
		addx.l	d2,d1
		move.b	(a2),(a7)+
		addx.l	d3,d0
		.endif
	.endm
	.macro	map_texel_b
		move.w	d1,d4
		move.b	d0,d4
		move.l	d4,a2
		.if tmap_16bit
		add.l	d2,d1
		addx.b	d3,d0
		.else
		addx.l	d2,d1
		addx.l	d3,d0
		.endif
		move.b	(a2),(a7)+
	.endm
	.macro	map_texel_c
		move.w	d1,d4
		move.b	d0,d4
		move.l	d4,a2
		move.b	(a2),(a7)+
		.if tmap_16bit
		add.l	d2,d1
		addx.b	d3,d0
		.else
		addx.l	d2,d1
		addx.l	d3,d0
		.endif
	.endm

	.else
	.macro	map_texel_a
	 map_texel
	.endm
	.macro	map_texel_b
	 map_texel
	.endm
	.macro	map_texel_c
	 map_texel
	.endm
	.endif
	.align	4
do16texels:
		.if tmap_16bit
		add.l	d2,d1
		addx.b	d3,d0
		.else
		addx.l	d2,d1
		addx.l	d3,d0
		.endif
| 16
do16T	=	.-dotexels
	map_texel_a
| 15
do15T	=	.-dotexels
	map_texel_b
| 14
do14T	=	.-dotexels
	map_texel_c
| 13
do13T	=	.-dotexels
	map_texel_a
| 12
do12T	=	.-dotexels
	map_texel_b
| 11
do11T	=	.-dotexels
	map_texel_c
| 10
do10T	=	.-dotexels
	map_texel_a
| 9
do09T	=	.-dotexels
	map_texel_b
| 8
do08T	=	.-dotexels
	map_texel_c
| 7
do07T	=	.-dotexels
	map_texel_a
| 6
do06T	=	.-dotexels
	map_texel_b
| 5
do05T	=	.-dotexels
	map_texel_c
| 4
do04T	=	.-dotexels
	map_texel_a
| 3
do03T	=	.-dotexels
	map_texel_b
| 2
do02T	=	.-dotexels
	map_texel_c
| 1
do01T	=	.-dotexels
|	map_texel_a
	move.w	d1,d4
	move.b	d0,d4
	move.l	d4,a2
	move.b	(a2),(a7)+
|
	add.l	d5,d6
	bcc.w	do16texels
dotexels:
	move.l	(a1)+,d6
	bpl.s	LoadTextureAddr		| >=0 - new texture or end
	.if		tmap_16bit
| Load all parameters
	move.l	(a1)+,d1			| UUuuVVvv
	move.l	d1,d0
	move.l	(a1)+,d2			| dUdudVdv
	move.l	d2,d3
	move.l	(a1)+,a7			| *D
| UUuuVVvv => ______VV
	lsr.w	#8,d0				| ______VV
| UUuuVVvv => vv__UUuu
	lsl.w	#8,d1				| UUuuvv__
	swap	d1					| vv__UUuu 
| dUdudVdv => ______dV
	lsr.w	#8,d3				| ______dV
| dUdudVdv => dv__dUdu
	lsl.w	#8,d2				| dUdudv__
	swap	d2					| dv__dUdu
	.else
| Load all parameters
	move.l	(a1)+,d1			| UUuuVVvv
	moveq	#0,d0
	move.l	(a1)+,d2			| __dVdvdv
| UUuuVVvv => ______VV
	move.w	d1,d0			| ____VVvv
	move.l	(a1)+,d3		| __dVdvdv
	lsr.w	#8,d0			| ______VV
| UUuuVVvv => vv__UUuu
	lsl.w	#8,d1			| UUuuvv__
	swap	d1			| vv__UUuu 
|
	swap	d2			| dvdv__dV
	swap	d3			| du__dUdu
	eor.w	d2,d3
	move.l	(a1)+,a7		| *D
	eor.w	d3,d2			| dvdvdUdu
	eor.w	d2,d3			| du____dV
|	sub.w	d4,d4			| clear carry flag
	.endif
	jmp	dotexels(pc,d6.w)
LoadTextureAddr:
	beq.w	RenderSlicesEnd
	move.l	d6,d4
	bra.s	dotexels
|
| ...end of textured render...
|


|=======================================================================================
| X-splitter
|=======================================================================================
	.align	4
XSPLIT_LINK_REGISTER:
	dc.l	0
XSPLIT:
	Profile 35,d0
	move.l	TMAP_qtail(pc),a1
|R_DATA** AddVertexesByX(RFACE** fout, RFACE* f, R_DATA** pool)
|{
|    RDBG("\t\tAddVertexesByX...\n");
AddVertexesByX:
|    do
|    {
|        RVERTEX* vs2 = f->vertex;
|        RVERTEX* vs1 = vs2->prev;
|        RVERTEX* vlast = (RVERTEX*)f;
|        RVERTEX* vd;
|        FIXP16 t1, t2;
|        //goto L_no_add;
	move.l	RFACE.vertex(a0),a3
	move.l	RVERTEX.vp(a3),a2
	move.l	a0,a5
AddVertexesByX_vloop:
|        do
|        {
|            if (vs1->flags & vs2->flags & FACE_DIVIDED) goto L_next;
	move.w	RVERTEX.flagH(a2),d2
	and.w	RVERTEX.flagH(a3),d2
	bmi.w	9f
|            RVERTEX* stack;
|            stack = NULL;
	sub.l	a6,a6
|            FIXP16 save_t2;
|            FIXP16 save_t1;
|            save_t1 = (vs1->x + vs1->z) * 4;
|            save_t2 = (vs2->x + vs2->z) * 4;
	move.w	RVERTEX.x(a2),d6
	move.w	RVERTEX.x(a3),d7
	move.w	RVERTEX.z(a2),d4
	move.w	RVERTEX.z(a3),d5
	add.w	d4,d6
	add.w	d5,d7
	lsl.w	#2,d6
	lsl.w	#2,d7
|            int count = 3 + 1 + 3;
|	moveq	#3+1+3-1,d4
AddVertexesByX_sloop:
|            do
|            {
|                save_t1 -= vs1->z;
|                save_t2 -= vs2->z;
|	sub.w	RVERTEX.z(a2),d6
|	sub.w	RVERTEX.z(a3),d7
	sub.w	d4,d6
	ble.s	1f
	sub.w	d5,d7
6:
|                t2 = save_t2;
|                t1 = save_t1;
|                if ((t1 ^ t2) >= 0) continue; //Нет сечения
	move.w	d6,d2
	eor.w	d7,d2
	bpl.s	AddVertexesByX_sloop
|                //Знак поменялся, нам надо посчитать точку
	move.w	d6,d0
	move.w	d7,d1
|                vd = &(*pool++)->v;
|                vd->flags = SCOORD_NOT_VALID; //Установим флаг невалидности sx/sy
|                if (save_t2 >= 0)
|                {
|                    //Меняем
|                    FIXP16 t; t = t1; t1 = t2; t2 = t;
|                    RVERTEX* v; v = vs1; vs1 = vs2; vs2 = v;
|                }
|                UFIXP16* p1, * p2, * p;
|                p1 = &vs1->U;
|                p2 = &vs2->U;
|                p = &vd->U;
|                int pcount = 5;
|                FIXP32 t;
|                t = t1;
|                t2 = t1 - t2;
|                t <<= 15;
|                t = t / t2;
|                do
|                {
|                    FIXP16 b, d;
|                    b = *p1++;
|                    d = *p2++;
|                    d -= b;
|                    *p++ = b + ((t * d /* + 0x4000*/) >> 15);
|                } while (--pcount);
	move.l	(a7)+,a4
	moveq	#5-1,d3
|	move.w	d3,RVERTEX.flagH(a4)
	tst.w	d7
	bmi.s	3f
	exg	d0,d1
	exg	a2,a3
3:
	moveq	#RVERTEX.U,d2
	add.l	d2,a2
	add.l	d2,a3
	add.l	d2,a4
	sub.w	d0,d1
	neg.w	d1				| t2=t1-t2
	swap	d0
	clr.w	d0
	lsr.l	#1,d0				| t1<<=15
	divu.w	d1,d0				| t1=(t1+(t2>>1))/t2|
4:
	move.w	(a3)+,d1
	move.w	(a2)+,d2
	sub.w	d2,d1				| d = vs2->U - vs1->U|
	muls.w	d0,d1				| d = (d*t+0x4000)>>15|
	add.l	d1,d1
	swap	d1
	add.w	d1,d2
	move.w	d2,(a4)+			| vd1->U = vs1->U + P[d] - M[d]|
	dbra	d3,4b
	move.w	d3,(a4)
|                if (save_t2 >= 0)
|                {
|                    //Меняем назад
|                    RVERTEX* v; v = vs1; vs1 = vs2; vs2 = v;
|                    vlast->next = vd;
|                    vlast = vd;
|                }
|                else
|                {
|                    vd->next = stack;
|                    stack = vd;
|                }
	moveq	#-RVERTEX.flagH,d2
	add.l	d2,a4				| restore vd1
	add.l	d2,a3				| restore vs2 base
	add.l	d2,a2				| restore vs1 base
	tst.w	d7
	bmi.s	5f
	exg	a2,a3
	move.l	a4,RVERTEX.vn(a5)
	move.l	a4,a5
	bra.s	AddVertexesByX_sloop
5:
	move.l	a6,RVERTEX.vn(a4)
	move.l	a4,a6
	bra.s	AddVertexesByX_sloop
1:
	sub.w	d5,d7
	bgt.s	6b
|            } while (--count);
2:
|            //Move from stack
|            while (stack)
|            {
|                vlast->next = stack;
|                vlast = stack;
|                stack = stack->next;
|            }
	tst.l	a6
	beq.s	7f
8:
	move.l	a6,RVERTEX.vn(a5)
	move.l	a6,a5
	move.l	RVERTEX.vn(a6),a6
	tst.l	a6
	bne.s	8b
7:
|        L_next:
9:
|            vlast->next = vs2;
|            vlast = vs2;
|            vs1 = vs2;
|        } while ((vs2 = vs2->next) != NULL);
	move.l	a3,RVERTEX.vn(a5)
	move.l	a3,a5
	move.l	a3,a2
	move.l	RVERTEX.vn(a3),a3
	tst.l	a3
	bne.w	AddVertexesByX_vloop
|
|        vlast->next = NULL;
|        f->vertex->prev = vlast;
	clr.l	RVERTEX.vn(a5)
	move.l	RFACE.vertex(a0),a4
	move.l	a5,RVERTEX.vp(a4)	| rface->vertex->prev=last
|        RDBG("\t\t\tsplitted...\n");
|        send_to_render_faces++;
	move.l	a0,RFACE.next(a1)
	move.l	a0,a1
|    } while ((f = f->next) != NULL);
	move.l	RFACE.next(a0),a0
	tst.l	a0
	bne.w	AddVertexesByX
|    TMAP_qtail = fout;
|    if (pool > R_DATA_THR_near_faces)
|    {
|        RDBG("*** AddVertexesByX R_DATA_THR_near_faces cleanup (%d)\n", N);
|        pool = RenderTMAPQ(pool);
|        N = 0;    
|    }
	move.l	a1,TMAP_qtail
	cmp.l	#R_DATA_THR_near_faces,a7
	bcc.s	1f
2:
	move.l	XSPLIT_LINK_REGISTER(pc),a0
	jmp		(a0)
1:
	clr.l	RFACE.next(a1)
	move.l	TMAP_qhead(pc),a0
	tst.l	a0
	beq.s	2b
	LJMP	TMAP
	bra.s	2b

|=======================================================================================
| Y-splitter
|=======================================================================================
	.align	4
YSPLIT_LINK_REGISTER:
	dc.l	0
ysplit_qhead:
	dc.l	0
YSPLIT:
	Profile 34,d0
|
|R_DATA** SplitFaceByY(RFACE** fout, RFACE* face_a, R_DATA** pool)
|{
|    RFACE* ysplit_qhead;
|    RFACE* fq = INIT_RFACEQ(ysplit_qhead);
	lea		ysplit_qhead-RFACE.next(pc),a1		|point local fout to head pointer
SplitFaceByY:
|    RDBG("\t\tSplitFaceByY...\n");
	move.l	RFACE.vertex(a0),a4					|loop as in section procedure
	move.l	RVERTEX.vp(a4),a5
	moveq	#4-1,d4			| K
|    for (int K = 0; K <= 3; K++)
|    {
SplitFaceByY_floop:
	move.l	a1,d5			| save fout
|        RVERTEX* vs2;
|        RVERTEX* vs1;
|        vs2 = face_a->vertex;
|        vs1 = vs2->prev;
|        RFACE* face_b;
|        face_b = &(*pool++)->f;
|        face_b->T = face_a->T;
|        face_b->xmax = face_a->xmax;
|        RVERTEX* vlast_a = (RVERTEX*)face_a; //Last vertex in face_a
|        RVERTEX* vlast_b = (RVERTEX*)face_b; //Last vertex in face_b
	move.l	(a7)+,a6
	move.l	RFACE.T(a0),RFACE.T(a6)
|	move.l	RFACE.flagsH(a0),RFACE.flagsH(a2)
	move.l	a6,d6			| save face_b
|	move.l	RFACE.vertex(a0),a4
|	move.l	RVERTEX.vp(a4),a3
	move.l	a5,a3
	sub.l	a2,a2
	move.l	a0,a5
	bra.s	SplitFaceByY_vloop
SplitFaceByY_vloop_prep:
	move.w	d1,d7
	move.l	a3,a2
	move.l	a4,a3
|        do
|        {
SplitFaceByY_vloop:
|d0 - t1
|d1 - t2
|d2 - scratch
|d3 - scratch
|            FIXP16 t1, t2;
|            t1 = vs1->z;
|            t2 = vs2->z;
|            switch (K)
|            {
|            case 0: t1 = t1 * 3; t2 = t2 * 3; break;
|            case 1: t1 = t1 * 1; t2 = t2 * 1; break;
|            case 2: t1 = -t1 * 1; t2 = -t2 * 1; break;
|            case 3: t1 = -t1 * 3; t2 = -t2 * 3; break;
|            }
|	move.w	RVERTEX.z(a3),d1
	movem.w	RVERTEX.y(a3),d0-d1
	move.w	d1,d2
	jmp	SplitFaceByY_j0(pc,d4*4)
SplitFaceByY_j0:
	lsl.w	#1,d2			|*2
	add.w	d2,d1			|*3
SplitFaceByY_j1:
	neg.w	d1			|-
	bra.s	SplitFaceByY_j	
SplitFaceByY_j2:
	bra.s	SplitFaceByY_j	
	nop
SplitFaceByY_j3:
	lsl.w	#1,d2			|*2
	add.w	d2,d1			|*3
SplitFaceByY_j:
|            t1 -= vs1->y * 8;
|            t2 -= vs2->y * 8;
|	move.w	RVERTEX.y(a3),d0
	lsl.w	#3,d0
	sub.w	d0,d1
	tst.l	a2
	beq.s	SplitFaceByY_vloop_prep
	move.w	d7,d0
|            FIXP16 save_t2;
|            save_t2 = t2;
|            if ((t1 ^ t2) < 0)
|            {
|                //Sign changed, compute two new vertexes
|                RVERTEX* vd1, * vd2;
|                vd1 = &(*pool++)->v;
|                vd2 = &(*pool++)->v;
	move.w	d1,d7				|save t2, coz if t2>=0 then ADD_VERTEX(vd,vs2)
	move.w	d0,d2
	eor.w	d1,d2
	bpl.s	2f
	move.l	(a7)+,a1
	move.l	(a7)+,a4
|                int pcount = 5;
	moveq	#5-1,d3
	move.l	a4,RVERTEX.vn(a6)
	move.l	a4,a6
|                if (save_t2 >= 0)
|                {
|                    //Swap vs1/vs2
|                    FIXP16 t; t = t1; t1 = t2; t2 = t;
|                    RVERTEX* v; v = vs1; vs1 = vs2; vs2 = v;
|                }
|                UFIXP16* p1, * p2, * pd1, * pd2;
|                p1 = &vs1->U;
|                p2 = &vs2->U;
|                pd1 = &vd1->U;
|                pd2 = &vd2->U;
|                FIXP32 t;
|                t = t1;
|                t2 = t1 - t2;
|                t <<= 15;
|                //t = (t + (t2 >> 1)) / t2;
|                t = t / t2;
|                do
|                {
|                    FIXP16 b, d;
|                    b = *p1++;
|                    d = *p2++;
|                    d -= b;
|                    b = b + ((t * d /* + 0x4000*/) >> 15);
|                    *pd1++ = b;
|                    *pd2++ = b;
|                } while (--pcount);
|                if (save_t2 >= 0)
|                {
|                    //Swap back
|                    vs2 = vs1;
|                }
|                vlast_a->next = vd1;
|                vlast_a = vd1;
|                vlast_b->next = vd2;
|                vlast_b = vd2;
|            }
|            if (save_t2 >= 0)
|            {
|                //Store to face_a
|                vlast_a->next = vs2;
|                vlast_a = vs2;
|            }
|            else
|            {
|                //Store to face_b
|                vlast_b->next = vs2;
|                vlast_b = vs2;
|            }
|            vs1 = vs2;
|        } while ((vs2 = vs2->next) != NULL);
	tst.w	d7
	bmi.s	3f
	exg	d0,d1
	exg	a2,a3
3:
	move.l	a1,RVERTEX.vn(a5)
	move.l	a1,a5
	moveq	#RVERTEX.U,d2
	add.l	d2,a1
	add.l	d2,a2
	add.l	d2,a3
	add.l	d2,a4
	sub.w	d0,d1
	neg.w	d1				| t2=t1-t2
	swap	d0
	clr.w	d0
	lsr.l	#1,d0				| t1<<=15
	divu.w	d1,d0				| t1=(t1+(t2>>1))/t2|
4:
	move.w	(a3)+,d1
	move.w	(a2)+,d2
	sub.w	d2,d1				| d = vs2->U - vs1->U|
	muls.w	d0,d1				| d = (d*t+0x4000)>>15|
	add.l	d1,d1
	swap	d1
	add.w	d1,d2
	move.w	d2,(a4)+			| vd1->U = vs1->U + P[d] - M[d]|
	move.w	d2,(a1)+			| vd2->U = vs1->U + P[d] - M[d]|
	dbra	d3,4b
|                if (vs1->flags & vs2->flags & FACE_DIVIDED)
|                {
|                    vd1->flags = SCOORD_NOT_VALID | FACE_DIVIDED;
|                    vd2->flags = SCOORD_NOT_VALID | FACE_DIVIDED;
|                }
|                else
|                {
|                    vd1->flags = SCOORD_NOT_VALID;
|                    vd2->flags = SCOORD_NOT_VALID;
|                }
	move.w	(a2),d2
	and.w	(a3),d2
	move.b	d3,d2
	move.w	d2,(a1)
	move.w	d2,(a4)	
	tst.w	d7
	bmi.s	5f
	exg	a2,a3
5:
	lea		-RVERTEX.flagH(a3),a3		| restore vs2 base
2:
	tst.w	d7
	bmi.s	6f	
	move.l	a3,RVERTEX.vn(a5)
	move.l	a3,a5
	bra.s	7f
6:
	move.l	a3,RVERTEX.vn(a6)
	move.l	a3,a6
7:
	move.l	a3,a2
	move.l	RVERTEX.vn(a3),a3
	tst.l	a3
	bne.w	SplitFaceByY_vloop
|
	move.l	d5,a1				| restore fout
	move.l	d6,a2				| restore face_b
|        if (vlast_b != (RVERTEX*)face_b)
|        {
|            vlast_b->next = NULL;
|            face_b->vertex->prev = vlast_b;
|            RDBG("\t\t\tsplitted...\n");
|            fq->next = face_b;
|            fq = face_b;
|        }
|        else
|        {
|            //Грань а пуста
|            *--pool = (R_DATA*)face_b; //Return to pool
|        }
	cmp.l	a6,a2
	beq.s	SplitFaceByY_face_b_empty
	clr.l	RVERTEX.vn(a6)
	move.l	RFACE.vertex(a2),a3
	move.l	a6,RVERTEX.vp(a3)
	move.l	a2,RFACE.next(a1)
	move.l	a2,a1						| fout=rface
	dc.w	0x0C40				| opcode cmpi.w	#xxxx,d0
SplitFaceByY_face_b_empty:	
	move.l	a2,-(a7)
|        if (vlast_a != (RVERTEX*)face_a)
|        {
|            vlast_a->next = NULL;
|            face_a->vertex->prev = vlast_a;
|        }
|        else
|        {
|            *--pool = (R_DATA*)face_a; //Return face to pool
|            goto L1;
|        }
	cmp.l	a5,a0
	beq.s	SplitFaceByY_face_a_empty
	clr.l	RVERTEX.vn(a5)
	move.l	RFACE.vertex(a0),a4
|
	dbra	d4,SplitFaceByY_floop
|
	move.l	a5,RVERTEX.vp(a4)	| rface->vertex->prev=last
|    }
|    //Send rest of face to queue
|    fq->next = face_a;
|    fq = face_a;
	move.l	a0,RFACE.next(a1)
	move.l	a0,a1						| fout=rface
	dc.w	0x0C40				| opcode cmpi.w	#xxxx,d0
|L1:
SplitFaceByY_face_a_empty:
	move.l	a0,-(a7)
| Test pool underflow
|	cmp.l	#R_DATA_THR_faces,a7
|	bcc.s	2f
|
	move.l	RFACE.next(a0),a0
3:
	tst.l	a0
	bne.w	SplitFaceByY
|    fq->next = NULL;
	clr.l	RFACE.next(a1)
	move.l	ysplit_qhead(pc),a0
	tst.l	a0
	beq.s	1f
	LJMP	XSPLIT
1:
	move.l	YSPLIT_LINK_REGISTER(pc),a0
	jmp		(a0)
| Pool underflow
|2:
|	move.l	RFACE.next(a0),ysplit_save_next
|	clr.l	RFACE.next(a1)
|	move.l	ysplit_qhead(pc),a0
|	tst.l	a0
|	beq.s	1f
|	LJMP	XSPLIT
|1:
|	Profile	36,d0
|	lea		ysplit_qhead-RFACE.next(pc),a1		|point local fout to head pointer
|	move.l	ysplit_save_next(pc),a0
|	bra.w	3b
|
|	.align	4
|ysplit_save_next:
|	dc.l	0


|=======================================================================================
|
| Top level textured render
|
|=======================================================================================

	.align	4
	.globl	RenderT
	.type	RenderT,@function
	.extern sort_q2
|TFACE *
far_qhead:
	dc.l	0
mid_qhead:
	dc.l	0
near_qhead:
	dc.l	0
|RFACE *
RenderT_qhead:
	dc.l	0
NF_qhead:
	dc.l	0
NF_qtail:
	dc.l	0
|
RenderT:
	move.l	#TRAPEZOIDS,TRAPEZOIDS_POINTER
	move.l	#return_from_trapezoids_render,TRAPEZOIDS_LINK_REGISTER
	move.l	#SLICES_BUFFER,SLICES_POINTER
	move.l	#return_from_slice_render,SLICE_LINK_REGISTER

	movem.l	d2-d7/a2-a6,-(a7)
|	move.l	#RenderT_ret_from_tmap,TMAP_LINK_REGISTER
	move.l	a7,RenderT_save_a7
	move.l	RDataPool_top,a7
	lea		far_qhead-TFACE.next(pc),a1
	lea		mid_qhead-TFACE.next(pc),a2
	lea		near_qhead-TFACE.next(pc),a3
	moveq	#TRender_thr-1,d7
	move.w	#TSL_z_thr,d3			| tesselation z threshold
RenderT_makeq3_zloop:
	move.l	#sort_q2,a0
	move.l	(a0,d7.w*4),a0
	tst.l	a0
	beq.s	RenderT_makeq3_nextz
RenderT_makeq3_floop:
	lea	TFACE.vertexes(a0),a4		| tv=face->vertexes
	move.l	(a4)+,a5				| 
	moveq	#0,d1					| max_z
	move.w	d3,d2					| min_z
RenderT_makeq3_vloop:
	move.w	FPXYZ.z(a5),d0
	cmp.w	d0,d1
	bgt.s	1f
	move.w	d0,d1
1:
	cmp.w	d0,d2
	blt.s	1f
	move.w	d0,d2
1:
	addq.l	#4,a4
	move.l	(a4)+,a5				| 
	tst.l	a5
	bne.s	RenderT_makeq3_vloop
| Test max/min and place to required queue
	.if 1
	cmp.w	d3,d2
	bge.s	1f
	cmp.w	d3,d1
	bge.s	3f
	move.l	a0,TFACE.next(a3)		| place in near_q
	move.l	a0,a3
	bra.s	2f
3:
	move.l	a0,TFACE.next(a2)		| place in mid_q
	move.l	a0,a2
	bra.s	2f
1:
	.endif
	move.l	a0,TFACE.next(a1)		| place in far_q
	move.l	a0,a1
2:
	move.l	TFACE.next(a0),a0
	tst.l	a0
	bne.s	RenderT_makeq3_floop
RenderT_makeq3_nextz:
	dbra	d7,RenderT_makeq3_zloop
|
	clr.l	TFACE.next(a1)
	clr.l	TFACE.next(a2)
	clr.l	TFACE.next(a3)
|    TMAP_qtail = INIT_RFACEQ(TMAP_qhead);
	move.l	#TMAP_qhead-RFACE.next,TMAP_qtail
	move.l	#NF_qhead-RFACE.next,NF_qtail
|
	Profile 27,d0
|
|d5 - vroot
|d7 - counter
|a0 - vlast
|a1 - ftail
|a2 - face
|a3 - f
|a4 - tv
|a5 - p
|a6 - v
|a7 - pool
|
	lea		RenderT_qhead-RFACE.next(pc),a1		|point ftail to head pointer
	move.l	far_qhead(pc),a2
	tst.l	a2
	beq.w	RenderT_no_far_faces
RenderT_far_floop:
	move.l	(a7)+,a3
	move.l	TFACE.T(a2),RFACE.T(a3)
	move.l	TFACE.flags(a2),RFACE.flagsH(a3)
	move.l	a3,a0					| vlast=(RVERTEX*)f
	lea	TFACE.vertexes(a2),a4		| tv=face->vertexes
	move.l	(a4)+,a5				| 
RenderT_far_vloop:
	move.l	(a7)+,a6				| alloc RVERTEX
	move.l	FPXYZ.z(a5),RVERTEX.z(a6)	| copy z, flags
	move.l	FPXYZ.x(a5),RVERTEX.x(a6)	| copy x, y
	move.l	FPXYZ.sx(a5),RVERTEX.sx(a6)	| copy sx, sy
	move.l	(a4)+,RVERTEX.U(a6)		| copy U,V
	move.l	a6,RVERTEX.vn(a0)		| vlast->next=v
	move.l	a6,a0					| vlast = v
	move.l	(a4)+,a5				| while((p=tv->p)!=NULL)
	tst.l	a5
	bne.s	RenderT_far_vloop
	clr.l	RVERTEX.vn(a0)			| vlast->next=NULL
	move.l	RFACE.vertex(a3),a6
	move.l	a0,RVERTEX.vp(a6)		| f->vertex->prev=vlast
|	move.l	a0,([a3],RVERTEX.vp)
| add rface to queue
	move.l	a3,RFACE.next(a1)		| ftail->next=f
	move.l	a3,a1					| ftail=f
| Test output queue overflow
	cmp.l	#R_DATA_THR_faces,a7
	bcc.s	RenderT_no_pool_for_far_faces
2:
	move.l	TFACE.next(a2),a2
	tst.l	a2
	bne.s	RenderT_far_floop
	bra.s	RenderT_no_far_faces
RenderT_no_pool_for_far_faces:
	clr.l	RFACE.next(a1)						|ftail->next=NULL
	move.l	a2,RenderT_regsave
	move.l	RenderT_qhead(pc),a0
	move.l	TMAP_qtail(pc),a1					|point fout to head pointer
	LJMP	TCULLING
	move.l	a1,TMAP_qtail	
	move.l	TMAP_qhead(pc),a0
	tst.l	a0
	beq.s	1f	
	LJMP	TMAP
1:
	move.l	RenderT_regsave(pc),a2
	lea		RenderT_qhead-RFACE.next(pc),a1		|point ftail to head pointer
	Profile 27,d0
	bra.w	2b
RenderT_no_far_faces:
	clr.l	RFACE.next(a1)						|ftail->next=NULL
| Cleanup far faces
	Profile 28,d0
	move.l	RenderT_qhead(pc),a0
	tst.l	a0
	beq.s	1f
	move.l	TMAP_qtail(pc),a1		|point fout to head pointer
	LJMP	TCULLING
	move.l	a1,TMAP_qtail
1:
|========================================================================================
| Now prepare all mid-faces
	Profile 29,d0
	lea		RenderT_qhead-RFACE.next(pc),a1		|point ftail to head pointer
	move.l	mid_qhead(pc),a2
	tst.l	a2
	beq.w	RenderT_no_mid_faces
RenderT_mid_floop:
	move.l	(a7)+,a3
	move.l	TFACE.T(a2),RFACE.T(a3)
	move.l	TFACE.flags(a2),RFACE.flagsH(a3)
	move.l	a3,a0					| vlast=(RVERTEX*)f
	lea	TFACE.vertexes(a2),a4		| tv=face->vertexes
	move.l	(a4)+,a5				| 
RenderT_mid_vloop:
	move.l	(a7)+,a6				| alloc RVERTEX
	move.l	FPXYZ.z(a5),RVERTEX.z(a6)	| copy z, flags
	move.l	FPXYZ.x(a5),RVERTEX.x(a6)	| copy x, y
	move.l	FPXYZ.sx(a5),RVERTEX.sx(a6)	| copy sx, sy
	move.l	(a4)+,RVERTEX.U(a6)		| copy U,V
	move.l	a6,RVERTEX.vn(a0)		| vlast->next=v
	move.l	a6,a0					| vlast = v
	move.l	(a4)+,a5				| while((p=tv->p)!=NULL)
	tst.l	a5
	bne.s	RenderT_mid_vloop
	clr.l	RVERTEX.vn(a0)			| vlast->next=NULL
	move.l	RFACE.vertex(a3),a6
	move.l	a0,RVERTEX.vp(a6)		| f->vertex->prev=vlast
|	move.l	a0,([a3],RVERTEX.vp)
| add rface to queue
	move.l	a3,RFACE.next(a1)		| ftail->next=f
	move.l	a3,a1					| ftail=f
| Test output queue overflow
	move.l	TFACE.next(a2),a2
	tst.l	a2
	bne.s	RenderT_mid_floop
|========================================================================================
| Now split all mid-faces by Z
	clr.l	RFACE.next(a1)						|ftail->next=NULL
	move.l	RenderT_qhead(pc),a0
	lea		tculling_qhead-RFACE.next(pc),a1		|point fout to head pointer
	LJMP	TCULLING	
	move.l	tculling_qhead(pc),a0
	tst.l	a0
	beq.w	RenderT_no_mid_faces
	move.l	TMAP_qtail,a1							|far faces stored in TMAP queue
	move.l	NF_qtail,a4								|near faces stored in NF queue
|R_DATA** SplitFaceByZ(RFACE** fout, RFACE* face_a, R_DATA** pool)
|{
| d4 - save fout_b
| d5 - save fout_a
| d6 - save face_b
| d7 - save_t2
	Profile 30,d0
SplitFaceByZ:
	move.l	a1,d5			| save fout_a
	move.l	a4,d4			| save fout_b
|a0 - face_a
|a1 - vd2
|a2 - vs1 / &vs1->U / face_b
|a3 - vs2 / &vs2->U
|a4 - vd1
|a5 - vlast_a
|a6 - vlast_b
|    RFACE* face_b;
|    face_b = &(*pool++)->f;
	move.l	(a7)+,a6
|    face_b->T = face_a->T;
|    face_b->xmax = face_a->xmax;
	move.l	RFACE.T(a0),RFACE.T(a6)
|	move.l	RFACE.flagsH(a0),RFACE.flagsH(a2)
	move.l	a6,d6			| save face_b
|    RVERTEX* vs2;
|    RVERTEX* vs1;
|    vs2 = face_a->vertex;
|    vs1 = vs2->prev;
	move.l	RFACE.vertex(a0),a3
	move.l	RVERTEX.vp(a3),a2
|
|    RVERTEX* vlast_a = (RVERTEX*)face_a; //Последняя вершина грани a
|    RVERTEX* vlast_b = (RVERTEX*)face_b; //Последняя вершина грани b
	move.l	a0,a5
|    do
|    {
SplitFaceByZ_vloop:
|d0 - t1
|d1 - t2
|d2 - scratch
|d3 - scratch
|        FIXP16 t1, t2;
|        t1 = vs1->z;
|        t2 = vs2->z;
	move.w	RVERTEX.z(a2),d0
	move.w	RVERTEX.z(a3),d1
|        t1 -= 0x480;
|        t2 -= 0x480;
	move.w	#TSL_z_thr,d2
	sub.w	d2,d0
	sub.w	d2,d1
|        FIXP16 save_t2;
|        save_t2 = t2;
	move.w	d1,d7				|save t2, coz if t2>=0 then ADD_VERTEX(vd,vs2)
	move.w	d0,d2
|        if ((t1 ^ t2) < 0)
|        {
	eor.w	d1,d2
	bpl.s	2f
|            //Знак поменялся, нам надо посчитать точку и добавить ее в обе грани
|            RVERTEX* vd1, * vd2;
|            vd1 = &(*pool++)->v;
|            vd2 = &(*pool++)->v;
	move.l	(a7)+,a1
	move.l	(a7)+,a4
|            int pcount = 5;
	moveq	#5-1,d3	
|            vlast_a->next = vd1;
	move.l	a4,RVERTEX.vn(a6)
|            vlast_a = vd1;
	move.l	a4,a6
|            if (save_t2 >= 0)
|            {
|                //Меняем
|                FIXP16 t; t = t1; t1 = t2; t2 = t;
|                RVERTEX* v; v = vs1; vs1 = vs2; vs2 = v;
|            }
	tst.w	d7
	bmi.s	3f
	exg	d0,d1
	exg	a2,a3
3:
|            vlast_b->next = vd2;
	move.l	a1,RVERTEX.vn(a5)
|            vlast_b = vd2;
	move.l	a1,a5
|            UFIXP16* p1, * p2, * pd1, *pd2;
|            p1 = &vs1->U;
|            p2 = &vs2->U;
|            pd1 = &vd1->U;
|            pd2 = &vd2->U;
	moveq	#RVERTEX.U,d2
	add.l	d2,a1
	add.l	d2,a2
	add.l	d2,a3
	add.l	d2,a4
|            FIXP32 t;
|            t = t1;
|            t2 = t1 - t2;
|            t <<= 15;
|            //t = (t + (t2 >> 1)) / t2;
|            t = t / t2;
	sub.w	d0,d1
	neg.w	d1				| t2=t1-t2
	swap	d0
	clr.w	d0
	lsr.l	#1,d0				| t1<<=15
	divu.w	d1,d0				| t1=(t1+(t2>>1))/t2|
|            do
|            {
4:
|                FIXP16 b, d;
|                b = *p1++;
|                d = *p2++;
	move.w	(a3)+,d1
	move.w	(a2)+,d2
|                d -= b;
	sub.w	d2,d1				| d = vs2->U - vs1->U|
|                b = b + ((t * d /* + 0x4000*/) >> 15);
	muls.w	d0,d1				| d = (d*t+0x4000)>>15|
	add.l	d1,d1
	swap	d1
	add.w	d1,d2
|                *pd1++ = b;
	move.w	d2,(a4)+			| vd1->U = vs1->U + P[d] - M[d]|
|                *pd2++ = b;
	move.w	d2,(a1)+			| vd2->U = vs1->U + P[d] - M[d]|
|            } while (--pcount);
	dbra	d3,4b
|            vd1->flags = SCOORD_NOT_VALID | FACE_DIVIDED; //Установим флаг невалидности sx/sy
|            vd2->flags = SCOORD_NOT_VALID | FACE_DIVIDED; //Установим флаг невалидности sx/sy
	move.w	d3,(a1)
	move.w	d3,(a4)
|            if (save_t2 >= 0)
|            {
	tst.w	d7
	bmi.s	5f
|                //Меняем назад
|                vs2 = vs1;
	exg	a2,a3
5:
|            }
	lea		-RVERTEX.flagH(a3),a3		| restore vs2 base
|        }
2:
|        if (save_t2 >= 0)
	tst.w	d7
	bmi.s	6f	
|        {
|            //Отправляем в грань a
|            vlast_a->next = vs2;
	move.l	a3,RVERTEX.vn(a5)
|            vlast_a = vs2;
	move.l	a3,a5
|        }
	bra.s	7f
|        else
|        {
6:
|            //Отправляем в грань b
|            vlast_b->next = vs2;
	move.l	a3,RVERTEX.vn(a6)
|            vlast_b = vs2;
	move.l	a3,a6
|        }
7:
|        vs1 = vs2;
	move.l	a3,a2
|    } while ((vs2 = vs2->next) != NULL);
	move.l	RVERTEX.vn(a3),a3
	tst.l	a3
	bne.w	SplitFaceByZ_vloop
|    //Все, мы все прошли, теперь тест, не надо ли вернуть пустые грани в пул
	move.l	d5,a1				| restore fout_a
	move.l	d4,a4				| restore fout_b
	move.l	d6,a2				| restore face_b
|    if (vlast_a != (RVERTEX*)face_a)
|    {
	cmp.l	a5,a0
	beq.s	SplitFaceByZ_face_a_empty
|        //Set right next/prev only for first and last vertexes
|        vlast_a->next = NULL;
	clr.l	RVERTEX.vn(a5)
|        face_a->vertex->prev = vlast_a;
	move.l	RFACE.vertex(a0),a3
	move.l	a5,RVERTEX.vp(a3)	| rface->vertex->prev=last
|            //Save to far queue
|            fout_a->next = face_a;
|            fout_a = face_a;
	move.l	a0,RFACE.next(a1)
	move.l	a0,a1
|    else
|    {
	dc.w	0x0C40				| opcode cmpi.w	#xxxx,d0
SplitFaceByZ_face_a_empty:	
|        //Грань а пуста
|        *--pool = (R_DATA*)face_a; //Возвращаем в пул грань
	move.l	a0,-(a7)
|    }
|    if (vlast_b != (RVERTEX*)face_b)
|    {
	cmp.l	a6,a2
	beq.s	SplitFaceByZ_face_b_empty
|        vlast_b->next = NULL;
	clr.l	RVERTEX.vn(a6)
|        face_b->vertex->prev = vlast_b;
	move.l	RFACE.vertex(a2),a3
	move.l	a6,RVERTEX.vp(a3)
|            //Save to near queue
|            fout_b->next = face_b;
|            fout_b = face_b;
	move.l	a2,RFACE.next(a4)
	move.l	a2,a4
|    }
|    else
|    {
	dc.w	0x0C40				| opcode cmpi.w	#xxxx,d0
SplitFaceByZ_face_b_empty:	
|        //Грань а пуста
|        *--pool = (R_DATA*)face_b; //Возвращаем в пул грань
	move.l	a2,-(a7)
|    }
| do next face
	move.l	RFACE.next(a0),a0
	tst.l	a0
	bne.w	SplitFaceByZ
	clr.l	RFACE.next(a1)
	clr.l	RFACE.next(a4)
	move.l	a4,NF_qtail
| All mid-faces splitted by Z
|========================================================================================
| Now add vertexes by Y to far faces
	move.l	TMAP_qtail(pc),a0		| take prev tail as head of faces list for Yadd
	move.l	RFACE.next(a0),a0
	move.l	a1,TMAP_qtail			| save real tail
	tst.l	a0
	beq.w	RenderT_no_far_faces_after_z_split
|R_DATA** AddVertexesByY(RFACE** fout, RFACE* f, R_DATA** pool)
|{
	Profile 31,d0
AddVertexesByY:
|    RVERTEX* vlast = f->vertex->prev;
	move.l	RFACE.vertex(a0),a5		|
	move.l	RVERTEX.vp(a5),a5		|vlast=rface->vertex->prev
	moveq	#4-1,d4			| K
AddVertexesByY_floop:
|    for (int K = 0; K <= 3; K++)
|    {
|        RVERTEX* vs2; 
|        RVERTEX* vs1;
|        vs1 = NULL;
|        vs2 = vlast;
|        vlast = (RVERTEX*)f;
	sub.l	a2,a2
	move.l	a5,a3
	move.l	a0,a5
	bra.s	AddVertexesByY_vloop
AddVertexesByY_vloop_prep:
	move.w	d1,d7
	move.l	a3,a2					|vs1=vs2
	move.l	RFACE.vertex(a0),a3		|vs2=rfacce->vertex
AddVertexesByY_vloop:
|        FIXP16 t1, t2;
|        FIXP16 save_t2;
|        do
|        {
|            t2 = vs2->z;
|            switch (K)
|            {
|            case 0: t2 = t2 * 3; break;
|            case 1: t2 = t2 * 1; break;
|            case 2: t2 = -t2 * 1; break;
|            case 3: t2 = -t2 * 3; break;
|            }
|            t2 -= vs2->y * 8;
|            t1 = save_t2
|            save_t2 = t2;
	move.w	RVERTEX.z(a3),d1
	move.w	d1,d0
	jmp	AddVertexesByY_j0(pc,d4*4)
AddVertexesByY_j0:
	lsl.w	#1,d0			|*2
	add.w	d0,d1			|*3
AddVertexesByY_j1:
	neg.w	d1			|-
	bra.s	AddVertexesByY_j	
AddVertexesByY_j2:
	bra.s	AddVertexesByY_j	
	nop
AddVertexesByY_j3:
	lsl.w	#1,d0			|*2
	add.w	d0,d1			|*3
AddVertexesByY_j:
	move.w	RVERTEX.y(a3),d0
	lsl.w	#3,d0
	sub.w	d0,d1
	tst.l	a2
	beq.s	AddVertexesByY_vloop_prep
	move.w	d7,d0
	move.w	d1,d7				|save t2, coz if t2>=0 then ADD_VERTEX(vd,vs2)
|            if (!(vs1->flags & vs2->flags & FACE_DIVIDED)) goto L_next;
	move.w	RVERTEX.flagH(a2),d2
	and.w	RVERTEX.flagH(a3),d2
	bpl.s	2f
|            if ((t1 ^ t2) < 0)
|            {
|                //Знак поменялся, нам надо посчитать точку
|                RVERTEX* vd;
|                vd = &(*pool++)->v;
|                vd->flags = SCOORD_NOT_VALID | FACE_DIVIDED; //Установим флаг невалидности sx/sy
|                if (save_t2 >= 0)
|                {
|                    //Меняем
|                    FIXP16 t; t = t1; t1 = t2; t2 = t;
|                    RVERTEX* v; v = vs1; vs1 = vs2; vs2 = v;
|                }
|                UFIXP16* p1, * p2, * p;
|                p1 = &vs1->U;
|                p2 = &vs2->U;
|                p = &vd->U;
|                int pcount = 5;
|                FIXP32 t;
|                t = t1;
|                t2 = t1 - t2;
|                t <<= 15;
|                t = t / t2;
|                do
|                {
|                    FIXP16 b, d;
|                    b = *p1++;
|                    d = *p2++;
|                    d -= b;
|                    *p++ = b + ((t * d /* + 0x4000*/) >> 15);
|                } while (--pcount);
|                if (save_t2 >= 0)
|                {
|                    //Меняем назад
|                    vs2 = vs1;
|                }
|                vlast->next = vd;
|                vlast = vd;
|            }
	move.w	d0,d2
	eor.w	d1,d2
	bpl.s	2f
	move.l	(a7)+,a4
	moveq	#5-1,d3
	move.l	a4,RVERTEX.vn(a5)
	move.l	a4,a5
	tst.w	d7
	bmi.s	3f
	exg	d0,d1
	exg	a2,a3
3:
	moveq	#RVERTEX.U,d2
	add.l	d2,a2
	add.l	d2,a3
	add.l	d2,a4
	sub.w	d0,d1
	neg.w	d1				| t2=t1-t2
	swap	d0
	clr.w	d0
	lsr.l	#1,d0				| t1<<=15
	divu.w	d1,d0				| t1=(t1+(t2>>1))/t2|
4:
	move.w	(a3)+,d1
	move.w	(a2)+,d2
	sub.w	d2,d1				| d = vs2->U - vs1->U|
	muls.w	d0,d1				| d = (d*t+0x4000)>>15|
	add.l	d1,d1
	swap	d1
	add.w	d1,d2
	move.w	d2,(a4)+			| vd1->U = vs1->U + P[d] - M[d]|
	dbra	d3,4b
	move.w	d3,(a4)				| set flags
	tst.w	d7
	bmi.s	5f
	exg	a2,a3
5:
	lea		-RVERTEX.flagH(a3),a3				| restore vs2 base
2:
|        L_next:
|            vlast->next = vs2;
|            vlast = vs2;
|            vs1 = vs2;
|        } while ((vs2 = vs2->next) != NULL);
|        vlast->next = NULL;
|    }
	move.l	a3,RVERTEX.vn(a5)
	move.l	a3,a5
	move.l	a3,a2
	move.l	RVERTEX.vn(a3),a3
	tst.l	a3
	bne.w	AddVertexesByY_vloop
	clr.l	RVERTEX.vn(a5)
	dbra	d4,AddVertexesByY_floop
|    f->vertex->prev = vlast;
|    vlast->next = NULL;
	move.l	RFACE.vertex(a0),a4
	move.l	a5,RVERTEX.vp(a4)	| rface->vertex->prev=last
|    send_to_render_faces++;
| next face
	move.l	RFACE.next(a0),a0
	tst.l	a0
	bne.w	AddVertexesByY
|========================================================================================
| Now we can render all rest of far faces
	cmp.l	#R_DATA_THR_near_faces,a7
	bcs.s	1f
	Profile 32,d0
	move.l	TMAP_qhead(pc),a0
	tst.l	a0
	beq.s	1f
	LJMP	TMAP
1:
RenderT_no_far_faces_after_z_split:
|
|	Profile 37,d0
|	move.l	NF_qhead(pc),a0
|	tst.l	a0
|	beq.s	1f
|	LJMP	YSPLIT
|1:
|
RenderT_no_mid_faces:
| Process near faces
	Profile 33,d0
|
	lea		RenderT_qhead-RFACE.next(pc),a1		|point ftail to head pointer
	move.l	near_qhead(pc),a2
	tst.l	a2
	beq.w	RenderT_no_near_faces
RenderT_near_floop:
	move.l	(a7)+,a3
	move.l	TFACE.T(a2),RFACE.T(a3)
	move.l	TFACE.flags(a2),RFACE.flagsH(a3)
	move.l	a3,a0					| vlast=(RVERTEX*)f
	lea	TFACE.vertexes(a2),a4		| tv=face->vertexes
	move.l	(a4)+,a5				| 
RenderT_near_vloop:
	move.l	(a7)+,a6				| alloc RVERTEX
	move.l	FPXYZ.z(a5),RVERTEX.z(a6)	| copy z, flags
	move.l	FPXYZ.x(a5),RVERTEX.x(a6)	| copy x, y
	move.l	FPXYZ.sx(a5),RVERTEX.sx(a6)	| copy sx, sy
	move.l	(a4)+,RVERTEX.U(a6)		| copy U,V
	move.l	a6,RVERTEX.vn(a0)		| vlast->next=v
	move.l	a6,a0					| vlast = v
	move.l	(a4)+,a5				| while((p=tv->p)!=NULL)
	tst.l	a5
	bne.s	RenderT_near_vloop
	clr.l	RVERTEX.vn(a0)			| vlast->next=NULL
	move.l	RFACE.vertex(a3),a6
	move.l	a0,RVERTEX.vp(a6)		| f->vertex->prev=vlast
|	move.l	a0,([a3],RVERTEX.vp)
| add rface to queue
	move.l	a3,RFACE.next(a1)		| ftail->next=f
	move.l	a3,a1					| ftail=f
| Test output queue overflow
	cmp.l	#R_DATA_THR_near_faces,a7
	bcc.s	RenderT_no_pool_for_near_faces
2:
	move.l	TFACE.next(a2),a2
	tst.l	a2
	bne.s	RenderT_near_floop
	bra.w	RenderT_no_near_faces
RenderT_no_pool_for_near_faces:
	clr.l	RFACE.next(a1)						|ftail->next=NULL
	move.l	a2,RenderT_regsave
	move.l	RenderT_qhead(pc),a0
	move.l	NF_qtail,a1
	LJMP	TCULLING	
	move.l	NF_qhead(pc),a0
	tst.l	a0
	beq.s	1f
	LJMP	YSPLIT
1:
	move.l	#NF_qhead-RFACE.next,NF_qtail
	move.l	RenderT_regsave(pc),a2
	lea		RenderT_qhead-RFACE.next(pc),a1		|point ftail to head pointer
	Profile 33,d0
	bra.w	2b
RenderT_no_near_faces:
	clr.l	RFACE.next(a1)						|ftail->next=NULL
| Cleanup near faces
	Profile 38,d0
	move.l	RenderT_qhead(pc),a0
	tst.l	a0
	beq.s	1f
	move.l	NF_qtail(pc),a1
	LJMP	TCULLING
	move.l	a1,NF_qtail
1:
	move.l	NF_qtail(pc),a0
	clr.l	RFACE.next(a0)
	move.l	NF_qhead(pc),a0
	tst.l	a0
	beq.s	1f	
	LJMP	YSPLIT
1:
	move.l	TMAP_qtail(pc),a0
	clr.l	RFACE.next(a0)
	move.l	TMAP_qhead(pc),a0
	tst.l	a0
	beq.s	1f
	LJMP	TMAP
1:
|========================================================================================
| All TMAP complete, run trapezoids and slices cleanup
	Profile 25,d0
	move.l	a7,RDataPool_top
| Cleanup trapezoids
	move.l	#CRenderTrapezoids_end,TRAPEZOIDS_LINK_REGISTER
	move.l	TRAPEZOIDS_POINTER,a7
	bra.l	Trapezoids2slices
CRenderTrapezoids_end:
	move.l	a7,TRAPEZOIDS_POINTER
| Cleanup slices:
	move.l	SLICES_POINTER,a7
	move.l	#CRenderSlices_end,SLICE_LINK_REGISTER
	bra.l	RenderSlices
CRenderSlices_end:
	move.l	a7,SLICES_POINTER
	move.l	RenderT_save_a7(pc),a7
	movem.l	(a7)+,d2-d7/a2-a6
	rts
|
	.align	4
RenderT_regsave:
	dc.l	2
RenderT_save_a7:
	dc.l	1

	.bss
	.align	4
TRAPEZOIDS_LINK_REGISTER:
	ds.l	1
SLICE_LINK_REGISTER:
	ds.l	1
	.globl	TRAPEZOIDS_POINTER
TRAPEZOIDS_POINTER:
	ds.l	1
	.globl	SLICES_POINTER
SLICES_POINTER:
	ds.l	1
SLICE_TEXTURE_ADDR:
	ds.l	1
	.globl	SLICES_BUFFER
SLICES_BUFFER:
	ds.l	4096
	.globl	SLICES_BUFFER_THR
SLICES_BUFFER_THR:
	ds.l	1024				| max 100 lines (*5 long) + texture + stop
	.globl	TRAPEZOIDS
TRAPEZOIDS:
	ds.l	1024
TRAPEZOIDS_BUFFER_THR:
	ds.l	128
