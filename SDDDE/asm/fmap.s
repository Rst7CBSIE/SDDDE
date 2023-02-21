	.include "render.s"


	.extern frameCounter
	.text

	.align	4
|
fculling_qhead:
	dc.l	0
	.extern	RDataPool_top

fmap_exit2:
	move.l	fmap_LINK_REGISTER,a0
	jmp	(a0)
	.align	4
fmap:

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
	Profile 14,d0
	lea		fculling_qhead-RFACE.next(pc),a1		|point fout to head pointer
	bra.s	fculling_faces_loop	
	.align	4
fculling_faces_loop_end:
	clr.l	RFACE.next(a1)
	move.l	fculling_qhead(pc),a0
	tst.l	a0
	beq.s	fmap_exit2
	bra.w	fmap_prepare
	.align	4
fculling_accept_face:
	move.l	a0,RFACE.next(a1)
	move.l	a0,a1				| fout=rface
fculling_next_face:
	move.l	RFACE.next(a0),a0
	tst.l	a0
	beq.s	fculling_faces_loop_end
fculling_faces_loop:
	tst.b	RFACE.flagsL(a0)			|check only low 8 bits
	beq.s	fculling_accept_face
	move.l	RFACE.vertex(a0),a6		|
	move.l	RVERTEX.vp(a6),a6		|vlast=rface->vertex->prev
	moveq	#5*3,d5
fculling_planes_loop:
	move.l	a6,a3					|vs2=vlast
	sub.l	a2,a2					|vs1=NULL
	bra.s	fculling_loop
fculling_prep_t1:
	move.w	d1,d7
	move.l	a3,a2					|vs1=vs2
	move.l	RFACE.vertex(a0),a3		|vs2=rfacce->vertex
	move.l	a0,a6					|vlast=(RVERTEX*)rface
fculling_loop:
	move.w	RVERTEX.z(a3),d1
	cmpi.w	#3*3,d5
	bcc.s	fculling_t2_no58
|	.extern section_table_58
|	move.w	section_table_58(pc,d1.w*2),d1
	move.w	d1,d2
	lsr.w	#2,d2
	add.w	d2,d1
	addq.w	#1,d1
	lsr.w	#1,d1
fculling_t2_no58:
	jmp	fculling_t2_0-6(pc,d5*2)
fculling_t2_0:
	sub.w	RVERTEX.y(a3),d1
	bra.s	fculling_t2_ok
fculling_t2_1:
	add.w	RVERTEX.y(a3),d1
	bra.s	fculling_t2_ok
fculling_t2_2:
	sub.w	RVERTEX.x(a3),d1
	bra.s	fculling_t2_ok
fculling_t2_3:
	add.w	RVERTEX.x(a3),d1
	bra.s	fculling_t2_ok
fculling_t2_4:
	subi.w	#51,d1
fculling_t2_ok:
	tst.l	a2
	beq.s	fculling_prep_t1
	move.w	d7,d0            
|
	move.w	d1,d7				|save t2, coz if t2>=0 then ADD_VERTEX(vd,vs2)
	move.w	d0,d2
	eor.w	d1,d2
	bpl.s	no_fculling
| Allocate RVERTEX
	move.l	(a7)+,a4			| Get vertex from pool
	moveq	#5-1,d3
	move.w	d3,RVERTEX.flagH(a4)	| set "no scoords" flag and clear low flags 
	moveq	#RVERTEX.U,d2
	add.l	d2,a2
	add.l	d2,a3
	add.l	d2,a4
	tst.w	d7
	bmi.s	fculling_no_rv
	exg	d0,d1
	exg	a2,a3
fculling_no_rv:
	sub.w	d0,d1
	neg.w	d1				| t2=t1-t2
	swap	d0
	clr.w	d0
|	ext.l	d1
|	add.l	d1,d0
	lsr.l	#1,d0				| t1<<=15
	divu.w	d1,d0				| t1=(t1+(t2>>1))/t2|
fculling_param_loop:	
	move.w	(a3)+,d1
	move.w	(a2)+,d2
	sub.w	d2,d1				| d = vs2->U - vs1->U|
	muls.w	d0,d1				| d = (d*t+0x4000)>>15|
	add.l	d1,d1
	swap	d1
|	bpl.s	fculling_no_round2
|	addq.w	#1,d1
|fculling_no_round2:
	add.w	d1,d2
	move.w	d2,(a4)+			| newv->U = vs1->U + P[d] - M[d]|
	dbra	d3,fculling_param_loop
	tst.w	d7
	bmi.s	fculling_no_rv2
	exg	a2,a3
fculling_no_rv2:
	moveq	#-RVERTEX.flagH,d2
	add.l	d2,a3				| restore vs2 base
	add.l	d2,a4				| restore newv
| ADD_VERTEX(vd,newv)|
	move.l	a4,RVERTEX.vn(a6)	| last->vn=newv	
	move.l	a4,a6				| last=newv
no_fculling:
	tst.w	d7
	bmi.s	fculling_no_add_vs2
	move.l	a3,RVERTEX.vn(a6)	| last->vn=vs2	
	move.l	a3,a6				| last=vs2
	dc.w	0x0C40				| opcode cmpi.w	#xxxx,d0
fculling_no_add_vs2:
	move.l	a3,-(a7)			| Save vs2 to pool
	move.l	a3,a2				| vs1=vs2
	move.l	RVERTEX.vn(a3),a3
	tst.l	a3
	bne.w	fculling_loop
|
	cmp.l	a6,a0
	beq.s	fculling_skip_face			| if vlast==(RVERTEX*)f - skip face
	clr.l	RVERTEX.vn(a6)				| last->next=NULL
	subq.l	#3,d5
	bne.w	fculling_planes_loop
	move.l	RFACE.vertex(a0),a3
	move.l	a6,RVERTEX.vp(a3)	| rface->vertex->prev=last
	bra.w	fculling_accept_face
fculling_skip_face:
	move.l	a0,-(a7)			| return face to pool
	bra.w	fculling_next_face
	.align	4
fmap_prepare:
|A0 - RFACE*
|A1 - tv
|A2 - bv
|A3 - xmax
|A4 - x
|A5 - color
|A6 - *xtab
|A7 - *D
|D0 - tx
|D1 - bx
|D2 - scratch
|D3 - scratch
|D4 - ty
|D5 - dty 
|D6 - by
|D7 - dby
	Profile 15,d0
	move.l	a0,a2
| Считаем экранные координаты, если они еще не готовы
| + Ищем самый верхний элемент (и самый нижний)
| + Полностью заполняем prev
	move.l	#_LOGtab+32768,a5
	move.l	#EXPtab+65536,a6
	bra.s	fmap_prep_loop
fmap_scoord_not_ok:
	movem.w	RVERTEX.x(a1),d4-d6
	move.w	(a5,d6.w*2),d6			|log(z)
	move.w	(a5,d4.w*2),d4			|log(x)
	beq.s	fmap_no_clip_xz			|log(x)==0
	sub.w	d6,d4
	bcs.s	fmap_no_clip_xz			|log(x)<log(z)
	or.w	#0xFFFE,d4
fmap_no_clip_xz:
	move.w	(a6,d4.w*2),RVERTEX.sx(a1)		|zoom*exp(log(x)-log(z))+XDISP_FP
	move.w	(a5,d5.w*2),d5			|log(y)
	beq.s	fmap_no_clip_yz
	sub.w	d6,d5
	bcc.s	fmap_clip_yz			|log(y)<log(z)
	cmp.w	#LOG5_8,d5
	bcs.s	fmap_no_clip_yz			| <clip_y_factor
fmap_clip_yz:
	and.w	#0x0001,d5
	or.w	#LOG5_8,d5
fmap_no_clip_yz:
	move.w	(a6,d5.w*2),RVERTEX.sy(a1)
	bra.s	fmap_scoord_ok
fmap_prep_loop:
	move.l	a2,-(a7)			|free face	
	moveq	#0,d0				|xmax
	moveq	#-1,d1				|xmin
	moveq	#0,d3
	move.l	RFACE.vertex(a2),a1				|take *v
	move.l	RVERTEX.vp(a1),a4	|vlast=prev (этот всегда есть)
fmap_find_loop:
	tst.b	RVERTEX.flagL(a1)
	bne.s	fmap_scoord_not_ok
| Все, экранные координаты готовы
fmap_scoord_ok:
	move.b	RVERTEX.sx(a1),d3
	move.l	a1,-(a7)			|free vertex
	cmp.l	d3,d1
	bcs.s   fmap_not_min
	move.l	d3,d1
	move.l	a1,a3
fmap_not_min:
	move.l	a4,RVERTEX.vp(a1)	|store prev
	cmp.l	d3,d0
	bcc.s	fmap_not_max
	move.l	d3,d0
fmap_not_max:
	move.l	a1,a4				|vlast=v
	move.l	RVERTEX.vn(a1),a1
	tst.l	a1
	bne.s	fmap_find_loop
	move.l	RFACE.vertex(a2),RVERTEX.vn(a4)		| Cycle last vertex to first vertex
	move.l	a3,(a2)+			| Store top vertex as root
	move.l	d0,(a2)+			| store xmax
	move.l	(a2),d0				| must be color table
	move.l	dither_color_table(pc,d0.w),(a2)+
|	addq.l	#4,a2				| skip old xmax as flags
	move.l	(a2),a2				| load next face
	tst.l	a2
	bne.s	fmap_prep_loop
	move.l	a7,RDataPool_top
|
	clr.l	d1
	Profile 16,d0
	bra.w	fmap_face_loop

	.align	4
	.globl	dither_color_table
dither_color_table:
	ds.l	64*4

	.align	4
	.set    h_count,255
	
	.macro	inverse8
		.if (h_count > 1)
			dc.w	(0x8000+(h_count>>1))/h_count
		.else
			dc.w	0x7FFF
		.endif
		.set h_count,h_count-1
	.endm
	
Inv_table:
	.rept	256
	inverse8
	.endr
fmap_load_t:
	clr.l	d4
fmap_loop_load_t:
	move.w	RVERTEX.sy(a1),d4		| Грузим начальную точку
	move.w	d0,d2				| w=tx
	cmp.w	a3,d0				| if (tx>=xmax) stop
	bcc.s	fmap_loop_load_t_stop
	move.l	RVERTEX.vp(a1),a1               | tv = tv->pv
	move.b	RVERTEX.sx(a1),d0		| tx = tv->sx >> 8
	cmp.w	a4,d0				| while(tx<x)
	bcs.s	fmap_loop_load_t
fmap_loop_load_t_stop:
	sub.w	d0,d2				| w=tx-w
	move.w	RVERTEX.sy(a1),d5
	sub.w	d4,d5
	muls.w	Inv_table+510(pc,d2.w*2),d5	| w=1/w
	lsl.l	#8,d4
	.if enable_trapezoids_round
	asr.l	#8,d5
	add.l	d5,d4
	add.l	d5,d5
	.else
	asr.l	#7,d5
	.endif
	bra.s	fmap_no_load_t
fmap_load_b:
	clr.l	d6
fmap_loop_load_b:
	move.w	RVERTEX.sy(a2),d6		| Грузим начальную точку
	move.w	d0,d2				| w=tx
	cmp.w	a3,d0				| if (tx>=xmax) stop
	bcc.s	fmap_loop_load_b_stop
	move.l	RVERTEX.vn(a2),a2               | tv = tv->pv
	move.b	RVERTEX.sx(a2),d0		| tx = tv->sx >> 8
	cmp.w	a4,d0				| while(tx<x)
	bcs.s	fmap_loop_load_b
fmap_loop_load_b_stop:
	sub.w	d0,d2				| w=tx-w
	move.w	RVERTEX.sy(a2),d7
	sub.w	d6,d7
	muls.w	Inv_table+510(pc,d2.w*2),d7	| w=1/w
	lsl.l	#8,d6
	.if enable_trapezoids_round
	asr.l	#8,d7
	add.l	d7,d6
	add.l	d7,d7
	.else
	asr.l	#7,d7
	.endif
	bra.s	fmap_no_load_b
fmap_face_loop:
	movem.l	(a0)+,a1/a3/a5			| v, xmax, color 
	move.l	a1,a2				| tv=bv=min vertex
	moveq	#0,d0
	move.b	RVERTEX.sx(a1),d0		| tx=tv->x
	move.l	#0xDEADBEEF,a6
	.globl	Xtab_patch2
Xtab_patch2 = .-4
	lea	(a6,d0.w*4),a6		| xtab+Xmin*4
	move.l	d0,a4				| xmin
	move.w	d0,d2				| bx=tx
	swap	d0
	move.w	d2,d0
fmap_loop:
	addq.l	#1,a4				| x++
	cmp.w	a4,d0				| if (tx<x)
	bcs.s	fmap_load_t
fmap_no_load_t:
	swap	d0
	cmp.w	a4,d0				| if (tx<x)
	bcs.s	fmap_load_b
fmap_no_load_b:
	move.l	(a6)+,a7				| load *D
	move.l	a6,d2
	move.l	a5,d1
	btst	#2,d2
	beq.s	fmap_no_x_swap
	swap	d1
fmap_no_x_swap:
	swap	d0
	move.l	d4,d2
	swap	d2
	move.l	d6,d3					| h = (by >> 8) - t
	swap	d3
	sub.w	d2,d3					| h=h-t
	bls.s	fmap_nextline				| if (h > 0)
	subq.w	#1,d3
	btst	#0,d2
	bne.s	fmap_no_y_swap
	ror.w	#8,d1
fmap_no_y_swap:
	add.w	d2,d2
	add.w	d2,a7					| D+=t*2
	move.w	d1,d2
	lsr.w	#8,d2
fmap_inner_loop:
	move.b	d2,(a7)+
	dbra	d3,fmap_inner_loop1
	bra.s	fmap_nextline
fmap_inner_loop1:
	move.b	d1,(a7)+
	dbra	d3,fmap_inner_loop
fmap_nextline:
	add.l	d5,d4
	add.l	d7,d6
	cmp.l	a3,a4
	bcs.s	fmap_loop
fmap_stop:
	move.l	(a0),a0
	tst.l	a0
	bne.s	fmap_face_loop
|fmap_end_of_loop:
|	....return to prev level....
	move.l	RDataPool_top,a7
|fmap_exit:
	move.l	fmap_LINK_REGISTER,a0
	jmp	(a0)
	.align	4
fmap_LINK_REGISTER:
	ds.l	1

|=======================================================================================
|
| Filled render
|
|=======================================================================================

	.align	4
	.globl	RenderF
	.type	RenderF,@function
	.extern sort_q2

RenderF_qhead:
	dc.l	0
|d4 - dith
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
RenderF:
	movem.l	d2-d7/a2-a6,-(a7)
	move.l	#RenderF_ret_from_fmap,fmap_LINK_REGISTER
	move.l	a7,RenderF_save_a7
	move.l	RDataPool_top,a7
	lea		RenderF_qhead-RFACE.next(pc),a1		|point ftail to head pointer
	moveq	#32-TRender_thr-1,d7
RenderF_q_loop:
	move.l	#sort_q2+TRender_thr*4,a2
	move.l	(a2,d7.w*4),a2
	tst.l	a2
	beq.s	RenderF_q_empty
	move.w	RenderF_dith+TRender_thr*2(pc,d7.w*2),d4
RenderF_f_loop:
	move.l	(a7)+,a3
	move.w	TFACE.flags(a2),d0
	swap	d0
	move.w	TFACE.avg_c(a2),d0
	or.w	d4,d0
	move.l	d0,RFACE.flagsH(a3)
	move.l	a3,a0					| vlast=(RVERTEX*)f
	lea	TFACE.vertexes(a2),a4		| tv=face->vertexes
	move.l	(a4)+,a5				| 
RenderF_v_loop:
	move.l	(a7)+,a6				| alloc RVERTEX
	move.l	FPXYZ.z(a5),RVERTEX.z(a6)	| copy z, flags
	move.l	FPXYZ.x(a5),RVERTEX.x(a6)	| copy x, y
	move.l	FPXYZ.sx(a5),RVERTEX.sx(a6)	| copy sx, sy
|	move.l	(a4)+,RVERTEX.U(a6)		| copy U,V
	addq.l	#4,a4					| skip U,V
	move.l	a6,RVERTEX.vn(a0)		| vlast->next=v
	move.l	a6,a0					| vlast = v
	move.l	(a4)+,a5				| while((p=tv->p)!=NULL)
	tst.l	a5
	bne.s	RenderF_v_loop
	clr.l	RVERTEX.vn(a0)			| vlast->next=NULL
	move.l	RFACE.vertex(a3),a6
	move.l	a0,RVERTEX.vp(a6)		| f->vertex->prev=vlast
| add rface to queue
	move.l	a3,RFACE.next(a1)		| ftail->next=f
	move.l	a3,a1					| ftail=f
| Test output queue overflow
	cmp.l	#R_DATA_THR_faces,a7
	bcc.s	RenderF_X
|
2:
	move.l	TFACE.next(a2),a2
	tst.l	a2
	bne.s	RenderF_f_loop
RenderF_q_empty:
	dbra	d7,RenderF_q_loop
| Complete, run cleanup
	clr.l	RFACE.next(a1)						|ftail->next=NULL
	Profile 24,d0
	move.l	#RenderF_ret_from_fmap2,fmap_LINK_REGISTER
	move.l	RenderF_qhead(pc),a0
	tst.l	a0
	bne.w	fmap	
RenderF_ret_from_fmap2:
	move.l	a7,RDataPool_top
	move.l	RenderF_save_a7,a7
	movem.l	(a7)+,d2-d7/a2-a6
	rts
|
RenderF_X:
	clr.l	RFACE.next(a1)						|ftail->next=NULL
	movem.l	d4/d7/a2,RenderF_regsave
	move.l	RenderF_qhead(pc),a0
	tst.l	a0
	bne.w	fmap
RenderF_ret_from_fmap:
	Profile 9,d0
	movem.l	RenderF_regsave(pc),d4/d7/a2
	lea		RenderF_qhead-RFACE.next(pc),a1		|point ftail to head pointer
	bra.w	2b
|
	.align	4
RenderF_regsave:
	ds.l	3
RenderF_save_a7:
	ds.l	1
RenderF_dith:
	dc.w	0x0000,0x0000,0x0000,0x0000	| 0...3
	dc.w	0x0000,0x0000,0x0000,0x0000	| 4...7
	dc.w	0x0000,0x0000,0x0000,0x0000	| 8...11
	dc.w	0x0100,0x0100,0x0100,0x0100	| 12...15
	dc.w	0x0200,0x0200,0x0200,0x0200	| 16...19
	dc.w	0x0300,0x0300,0x0300,0x0300	| 20...23
	dc.w	0x0300,0x0300,0x0300,0x0300	| 24...27
	dc.w	0x0300,0x0300,0x0300,0x0300	| 28...31
