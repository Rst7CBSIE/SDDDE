	.include	"render.s"

	.text
	.align	4
	.globl	C2P
	.type	C2P, @function
C2P:
|
| a6 - custom
| a0 - chunky screen
| a1 - c2p buffer
| a2 - planar screen
| a3 - c2p buffer + 40
| a4 - planar screen + 80
| d6 - mask 0F0F0F0F
| d5 - mask 55555555
	move.l	#0x0F0F0F0F,d6
	move.l	#0x55555555,d5
	move.l	#0xDFF000,a6
|
444:
	btst.b	#6,0x002(a6)					| Blitter wait
	bne.s	444b
|
	move.w	#0xFFFF,0x044(a6)				| BLTAFWM
	move.w	#0xFFFF,0x046(a6)				| BLTALWM
| Now we can start stage 0 (by blitter)
	move.w	#0,0x066(a6)					| BLTDMOD=0
	move.l	a1,0x054(a6)					| BLTDPT=c2p buffer
	move.w	#200-2,0x062(a6)				| BLTBMOD=200-2
	move.w	#200-2,0x060(a6)				| BLTCMOD=200-2
	move.l	a0,0x048(a6)					| BLTCPT=chunky screen (A0A1A2A3B0B1B2B3)
	lea		8000(a0),a0
	move.l	a0,0x04C(a6)					| BLTBPT=chunky screen+4000 (C0C1C2C3D0D1D2D3)
	move.w	#0x3333,0x074(a6)				| BLTADAT=0x3333, select B when 1
	move.w	#0x07CA,0x040(a6)				| BLTCON0= USEB USEC USED term=AB + ~AC
	move.w	#0x2000,0x042(a6)				| BLTCON1= BSH=2
	move.w	#(40<<6)+1,0x058(a6)			| run blitter, H=40 W=1
|
	lea		40(a1),a3
	lea		80(a2),a4
| Stage 1
	moveq	#100-1-1,d3
2:
| start next blit
	lea		2-8000(a0),a0
444:
	btst.b	#6,0x002(a6)					| Blitter wait
	bne.s	444b
	move.l	a0,0x048(a6)					| BLTCPT=chunky screen (A0A1A2A3B0B1B2B3)
	lea		8000(a0),a0
	move.l	a0,0x04C(a6)					| BLTBPT=chunky screen+8000 (C0C1C2C3D0D1D2D3)
	move.w	#(40<<6)+1,0x058(a6)			| run blitter, H=40 W=1
| do one line by CPU
	moveq	#10-1,d4
1:
	move.l	(a3)+,d1	| d1 = e7e6e5e4e3e2e1e0 f7f6f5f4f3f2f1f0 g7g6g5g4g3g2g1g0 h7h6h5h4h3h2h1h0
	move.l  d1,d7		| d7 = e7e6e5e4e3e2e1e0 f7f6f5f4f3f2f1f0 g7g6g5g4g3g2g1g0 h7h6h5h4h3h2h1h0
	lsr.l   #4,d7		| d7 = ........e7e6e5e4 e3e2e1e0f7f6f5f4 f3f2f1f0g7g6g5g4 g3g2g1g0h7h6h5h4
	move.l	(a1)+,d0	| d0 = a7a6a5a4a3a2a1a0 b7b6b5b4b3b2b1b0 c7c6c5c4c3c2c1c0 d7d6d5d4d3d2d1d0
	eor.l   d0,d7		| d7 = mask0 ^ mask1   still dirty
	and.l   d6,d7		| d7 = mask
	eor.l   d7,d0		| d0 = a7a6a5a4e7e6e5e4 b7b6b5b4f7f6f5f4 c7c6c5c4g7g6g5g4 d7d6d5d4h7h6h5h4
	move.l	d0,(a2)+
	lsl.l   #4,d7		| d7 = mask << 4
	eor.l	d7,d1		| d1 = a3a2a1a0e3e2e1e0 b3b2b1b0f3f2f1f0 c3c2c1c0g3g2g1g0 d3d2d1d0h3h2h1h0 
	move.l	d1,(a4)+
	dbra	d4,1b
| now one line complete
	lea		80-40(a1),a1
	lea		80-40(a3),a3
	lea		200-40(a2),a2		| next planar line
	lea		200-40(a4),a4
	dbra	d3,2b
| last line without blitter pass
	moveq	#10-1,d4
444:
	btst.b	#6,0x002(a6)					| Blitter wait
	bne.s	444b
1:
	move.l	(a3)+,d1	| d1 = e7e6e5e4e3e2e1e0 f7f6f5f4f3f2f1f0 g7g6g5g4g3g2g1g0 h7h6h5h4h3h2h1h0
	move.l  d1,d7		| d7 = e7e6e5e4e3e2e1e0 f7f6f5f4f3f2f1f0 g7g6g5g4g3g2g1g0 h7h6h5h4h3h2h1h0
	lsr.l   #4,d7		| d7 = ........e7e6e5e4 e3e2e1e0f7f6f5f4 f3f2f1f0g7g6g5g4 g3g2g1g0h7h6h5h4
	move.l	(a1)+,d0	| d0 = a7a6a5a4a3a2a1a0 b7b6b5b4b3b2b1b0 c7c6c5c4c3c2c1c0 d7d6d5d4d3d2d1d0
	eor.l   d0,d7		| d7 = mask0 ^ mask1   still dirty
	and.l   d6,d7		| d7 = mask
	eor.l   d7,d0		| d0 = a7a6a5a4e7e6e5e4 b7b6b5b4f7f6f5f4 c7c6c5c4g7g6g5g4 d7d6d5d4h7h6h5h4
	move.l	d0,(a2)+
	lsl.l   #4,d7		| d7 = mask << 4
	eor.l	d7,d1		| d1 = a3a2a1a0e3e2e1e0 b3b2b1b0f3f2f1f0 c3c2c1c0g3g2g1g0 d3d2d1d0h3h2h1h0 
	move.l	d1,(a4)+
	dbra	d4,1b
| Stage 2, executed in reverse direction
	lea		40-2(a1),a1	| last word of c2p buffer
	move.l	a1,0x054(a6)					| BLTDPT=c2p buffer
	lea		7800(a0),a0						| last word of chunky screen
	move.l	a0,0x048(a6)					| BLTCPT=chunky screen+8000+7998 (C0C1C2C3D0D1D2D3)
	lea		-8000(a0),a0
	move.l	a0,0x04C(a6)					| BLTBPT=chunky screen+7998 (A0A1A2A3B0B1B2B3)
	move.w	#0xCCCC,0x074(a6)				| BLTADAT=0xCCCC, select B when 1
	move.w	#0x2002,0x042(a6)				| BLTCON1= BSH=2 + DESC
	move.w	#(40<<6)+1,0x058(a6)			| run blitter, H=40 W=1
	lea		-78(a1),a1	| last line of c2p buffer
	lea		40(a1),a3	|
| Stage 3
	moveq	#100-1-1,d3
2:
| start next blit
	lea		8000-2(a0),a0
444:
	btst.b	#6,0x002(a6)					| Blitter wait
	bne.s	444b
	move.l	a0,0x048(a6)					| BLTCPT=chunky screen+8000+7998 (C0C1C2C3D0D1D2D3)
	lea		-8000(a0),a0
	move.l	a0,0x04C(a6)					| BLTBPT=chunky screen+7998 (A0A1A2A3B0B1B2B3)
	move.w	#(40<<6)+1,0x058(a6)			| run blitter, H=40 W=1
| do one line by CPU
	moveq	#10-1,d4
1:
	move.l	(a3)+,d1	| d1 = e7e6e5e4e3e2e1e0 f7f6f5f4f3f2f1f0 g7g6g5g4g3g2g1g0 h7h6h5h4h3h2h1h0
	move.l  d1,d7		| d7 = e7e6e5e4e3e2e1e0 f7f6f5f4f3f2f1f0 g7g6g5g4g3g2g1g0 h7h6h5h4h3h2h1h0
	lsr.l   #4,d7		| d7 = ........e7e6e5e4 e3e2e1e0f7f6f5f4 f3f2f1f0g7g6g5g4 g3g2g1g0h7h6h5h4
	move.l	(a1)+,d0	| d0 = a7a6a5a4a3a2a1a0 b7b6b5b4b3b2b1b0 c7c6c5c4c3c2c1c0 d7d6d5d4d3d2d1d0
	eor.l   d0,d7		| d7 = mask0 ^ mask1   still dirty
	and.l   d6,d7		| d7 = mask
	eor.l   d7,d0		| d0 = a7a6a5a4e7e6e5e4 b7b6b5b4f7f6f5f4 c7c6c5c4g7g6g5g4 d7d6d5d4h7h6h5h4
	move.l	d0,(a2)+
	lsl.l   #4,d7		| d7 = mask << 4
	eor.l	d7,d1		| d1 = a3a2a1a0e3e2e1e0 b3b2b1b0f3f2f1f0 c3c2c1c0g3g2g1g0 d3d2d1d0h3h2h1h0 
	move.l	d1,(a4)+
	dbra	d4,1b
| now one line complete
	lea		-80-40(a1),a1		| prev line in C2Pbuffer
	lea		-80-40(a3),a3
	lea		-200-40(a2),a2		| prev planar line
	lea		-200-40(a4),a4
	dbra	d3,2b
| last line without blitter pass
	moveq	#10-1,d4
444:
	btst.b	#6,0x002(a6)					| Blitter wait
	bne.s	444b
1:
	move.l	(a3)+,d1	| d1 = e7e6e5e4e3e2e1e0 f7f6f5f4f3f2f1f0 g7g6g5g4g3g2g1g0 h7h6h5h4h3h2h1h0
	move.l  d1,d7		| d7 = e7e6e5e4e3e2e1e0 f7f6f5f4f3f2f1f0 g7g6g5g4g3g2g1g0 h7h6h5h4h3h2h1h0
	lsr.l   #4,d7		| d7 = ........e7e6e5e4 e3e2e1e0f7f6f5f4 f3f2f1f0g7g6g5g4 g3g2g1g0h7h6h5h4
	move.l	(a1)+,d0	| d0 = a7a6a5a4a3a2a1a0 b7b6b5b4b3b2b1b0 c7c6c5c4c3c2c1c0 d7d6d5d4d3d2d1d0
	eor.l   d0,d7		| d7 = mask0 ^ mask1   still dirty
	and.l   d6,d7		| d7 = mask
	eor.l   d7,d0		| d0 = a7a6a5a4e7e6e5e4 b7b6b5b4f7f6f5f4 c7c6c5c4g7g6g5g4 d7d6d5d4h7h6h5h4
	move.l	d0,(a2)+
	lsl.l   #4,d7		| d7 = mask << 4
	eor.l	d7,d1		| d1 = a3a2a1a0e3e2e1e0 b3b2b1b0f3f2f1f0 c3c2c1c0g3g2g1g0 d3d2d1d0h3h2h1h0 
	move.l	d1,(a4)+
	dbra	d4,1b
|
| Stage 4 - convert ABCDEFGH to AACCEEGG and BBDDFFHH
|
| a2 - bpl2
| a4 - bpl0
	lea		40(a2),a2			| a2 = bpl1
|
| Prepare blitter conversion to AACCEEGG
|
	move.w	#200-40,0x066(a6)				| BLTDMOD
	move.l	a4,0x054(a6)					| BLTDPT = bit0
	move.w	#200-40,0x062(a6)				| BLTBMOD
	move.w	#200-40,0x060(a6)				| BLTCMOD
	move.l	a2,0x048(a6)					| BLTCPT=bpl1
	move.l	a2,0x04C(a6)					| BLTBPT=bpl1
	move.w	#0x5555,0x074(a6)				| BLTADAT=0x5555, select B when 1
	move.w	#0x1000,0x042(a6)				| BLTCON1= BSH=1
	move.w	#(100<<6)+20,0x058(a6)			| run blitter, H=100 W=20
|
| Prepare CPU conversion to BBDDFFHH
|
	moveq	#100-1,d4
2:
	moveq	#10-1,d3
	move.l	a2,a3
	move.l	(a2)+,d0
1:
	and.l	d5,d0			| 0B0D0F0H
	move.l	d0,d1
	add.l	d1,d1			| B0D0F0H0
	add.l	d0,d1			| BBDDFFHH
	move.l	(a2)+,d0
	move.l	d1,(a3)+
	dbra	d3,1b
	lea		200-44(a2),a2		| next planar line
	dbra	d4,2b

	rts