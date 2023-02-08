	.include	"render.s"

	.text
	.align	4
	.globl	SD2leaf
	.type	SD2leaf, @function
|
| d0 - X
| d1 - Y
| d2 - Z
| a0 - kd_tree
| a1 - collision_index
SD2leaf:
	move.w	#0x800,0xDFF180
	movem.l	d0-d6/a2-a6,-(a7)
	move.l	a1,a2
	jsr	FindKDtreeNode_asm
	sub.l	a1,a0				| node-base
	add.l	a0,a2
	move.l	(a2),a6
	moveq	#-1,d7
	move.l	(a6)+,a0
	tst.l	a0
	beq.w	SD2leaf_end
| a0 - TFACE *f
SD2leaf_face_loop:
| a3,a4,a5 - CamX...CamZ
	movem.l	(a7),a3-a5		| load CamX, CamY, CamZ from stack
| a1 - TVERTEX *v
| a2 = FPXYZ *p
	lea	TFACE.vertexes(a0),a1
|	lea ([a0],TFACE.vertexes),a1
	move.l	(a1),a2			| p = v->p;
| a3,a4,a5 - xp,yp,zp
	addq.l	#FPXYZ.X,a2
|    xp = X - p->X;
	sub.l	(a2)+,a3
|    yp = Y - p->Y;
	sub.l	(a2)+,a4
|    zp = Z - p->Z;
	sub.l	(a2),a5
|
| d4,d5,d6 - xa, ya, za
|
SD2leaf_vertex_loop:
	movem.l	-8(a2),d4-d6		| xa = p->X; ya = p->Y; za = p->Z;
	addq.l	#TVERTEXsz,a1			| v++;
	move.l	(a1),a2					| p = v->p;
	tst.l	a2						| if (!p)
	bne.s	1f						| {
	lea		TFACE.vertexes(a0),a1	|   v = f->vertexes;
	move.l	(a1),a2					|   p = v->p;
	sub.l	a1,a1					|   v = NULL;
1:									| }
	addq.l	#FPXYZ.X,a2
	sub.l	(a2)+,d4				| xa -= p->X;
	add.l	d4,a3					| xp+=xa
	sub.l	(a2)+,d5				| ya -= p->Y;
	add.l	d5,a4					| yp+=ya
	sub.l	(a2),d6					| za -= p->Z;
	add.l	d6,a5					| zp+=za
| x = MUL(ya, f->C14) - MUL(za, f->B14);
| y = MUL(za, f->A14) - MUL(xa, f->C14);
| z = MUL(xa, f->B14) - MUL(ya, f->A14);
	movem.l	TFACE.flags(a0),d0/d3		|d0=...A14 d3=B14C14
	move.w	d0,d1
	muls.w	d6,d1					|d1=za*A14
	muls.w	d5,d0					|d0=ya*A14
	move.w	d3,d2
	muls.w	d4,d2					|d2=xa*C14
	sub.l	d2,d1					|d1=y
	swap	d3
	move.w	d3,d2
	muls.w	d4,d2					|d2=xa*B14
	sub.l	d0,d2					|d2=z
	move.w	d3,d0
	muls.w	d6,d0					|d0=za*B14
	swap	d3
	muls.w	d5,d3					|d3=ya*C14
	sub.l	d0,d3					|d3=x
	swap	d1
	swap	d2
	swap	d3
| if ((x * xp + y * yp + z * zp) >= 0)
	move.l	a4,d0
	muls.w	d0,d1
	move.l	a5,d0
	muls.w	d0,d2
	move.l	a3,d0
	muls.w	d0,d3
	add.l	d3,d1
	add.l	d2,d1
	bpl.s	2f					| next vertex
| Test distance to edge
	moveq	#15,d3
| w = xa * xa + ya * ya + za * za;
	move.l	d4,d0
	muls.w	d0,d0
	move.l	d5,d1
	muls.w	d1,d1
	move.l	d6,d2
	muls.w	d2,d2
	add.l	d1,d0
	add.l	d2,d0
|   w >>= 15;
	lsr.l	d3,d0
| if (!w) continue;
	beq.s	1f				| w!=0 always if correct mesh (no points with distance==0)
| h = xp * xa + yp * ya + zp * za;
	move.l	a3,d1
	muls.w	d4,d1	
	move.l	a4,d2
	muls.w	d5,d2
	add.l	d2,d1
	move.l	a5,d2
	muls.w	d6,d2
	add.l	d2,d1
| if (h<0) h=0
	bpl.s	4f
	moveq	#0,d1
4:
| h /= w;
	divu.w	d0,d1
| if (h <= 0x7FFF) {
	bvs.s	1f					| overflow mean h>0xFFFF
	bmi.s	1f					| minus mean h>0x7FFF
| xa = (xa*h)>>15;
| ya = (ya*h)>>15;
| za = (za*h)>>15;
	muls.w	d1,d4
	muls.w	d1,d5
	muls.w	d1,d6
	asr.l	d3,d4
	asr.l	d3,d5
	asr.l	d3,d6				| }
1:
	sub.l	a3,d4				| xa-=xp
	sub.l	a4,d5				| ya-=yp
	sub.l	a5,d6				| za-=yp
| h = xa * xa + ya * ya + za * za;
	muls.w	d4,d4
	muls.w	d5,d5
	muls.w	d6,d6
	add.l	d5,d4
	add.l	d6,d4
| if (h < dmin) dmin = h;
	bra.s	3f
| next vertex
2:
| } while (v);
	tst.l	a1
	bne.w	SD2leaf_vertex_loop
	movem.l	TFACE.flags(a0),d0/d1		|d0=...A14 d1=B14C14
	move.l	a3,d4
	muls.w	d0,d4
	move.l	a5,d0
	muls.w	d1,d0
	add.l	d0,d4
	swap	d1
	move.l	a4,d0
	muls.w	d1,d0
	add.l	d0,d4
	moveq	#15,d3
	asr.l	d3,d4
	muls.w	d4,d4
3:
	cmp.l	d4,d7
	bcs.s	1f
	move.l	d4,d7
1:
	move.l	(a6)+,a0
	tst.l	a0
	bne.w	SD2leaf_face_loop
SD2leaf_end:
	movem.l	(a7)+,d0-d6/a2-a6
	move.w	#0x000,0xDFF180
	rts


	.align	4
	.globl	TestPlayerMove_asm
	.type	TestPlayerMove_asm, @function
TestPlayerMove_asm:
	movem.l	d3-d6,-(a7)
| Rotate dx,dy,dz
	movem.w	CAMERA.RMTX+0*6(a2),d3-d5
	muls.w	d0,d3
	muls.w	d1,d4
	muls.w	d2,d5
	add.l	d4,d3
	add.l	d5,d3
	movem.w	CAMERA.RMTX+1*6(a2),d4-d6
	muls.w	d0,d4
	muls.w	d1,d5
	muls.w	d2,d6
	add.l	d5,d4
	add.l	d6,d4
	movem.w	CAMERA.RMTX+2*6(a2),d5-d7
	muls.w	d0,d5
	muls.w	d1,d6
	muls.w	d2,d7
	add.l	d6,d5
	add.l	d7,d5
|
	bfexts d3{2:16},d0
	bfexts d4{2:16},d1
	bfexts d5{2:16},d2
	add.l	d0,d0
	add.l	d1,d1
	add.l	d2,d2
| Compute X,Y,Z
	add.l	CAMERA.X(a2),d0
	add.l	CAMERA.Y(a2),d1
	add.l	CAMERA.Z(a2),d2
	bsr.w	SD2leaf
	movem.l	(a7)+,d3-d6
	rts
	