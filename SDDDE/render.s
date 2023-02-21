LOG5_8 	=	(0xF528-2)
YDISP_FP 	=	((50<<8))
XDISP_FP 	=	((80<<8))

	.extern	_LOGtab
	.extern	EXPtab
|
CAMERA.X	=	0
CAMERA.Y	=	4
CAMERA.Z	=	8
CAMERA.RMTX	=	12
|
RVERTEX.vn	=	0
RVERTEX.vp	=	4
RVERTEX.sx	=	8
RVERTEX.sy	=	10
RVERTEX.U	=	12
RVERTEX.V	=	14
RVERTEX.x	=	16
RVERTEX.y	=	18
RVERTEX.z	=	20
RVERTEX.flagH	=	22
RVERTEX.flagL	=	23
RVERTEX_sz	=	24
|
RFACE.vertex	=	0
RFACE.T		=	4
RFACE.xmax	=	8
RFACE.flagsH	=	8
RFACE.flagsL	=	9
RFACE.next	=	12

	.extern ProfilerStartScanline
	.extern ProfilerWP
	.macro	Profile stage,reg
	move.w	#\stage,([ProfilerWP])
	move.b	0xbfda00,\reg
	move.b	0xbfd900,\reg
	lsl.w	#8,\reg
	move.b	0xbfd800,\reg
	sub.w	ProfilerStartScanline,\reg
	move.w	\reg,([ProfilerWP],2)
	addq.l	#4,ProfilerWP
	.endm

TFACE.next	=	0
TFACE.seq	=	4
TFACE.flags	=	8
TFACE.A14	=	10
TFACE.B14	=	12
TFACE.C14	=	14
TFACE.MX	=	16
TFACE.MY	=	20
TFACE.MZ	=	24
TFACE.avg_z	=	28
TFACE.avg_c	=	30
TFACE.T		=	32
TFACE.vertexes	= 36

TVERTEX.p	=	0
TVERTEX.U	=	4
TVERTEX.V	=	6
TVERTEXsz	=	8

FPXYZ.seq	=	0
FPXYZ.X		=	4
FPXYZ.Y		=	8
FPXYZ.Z		=	12
FPXYZ.sx	=	16
FPXYZ.sy	=	18
FPXYZ.x		=	20
FPXYZ.y		=	22
FPXYZ.z		=	24
FPXYZ.flags	=	26

enable_distance_test	=	0
enable_backface_test	=	1

enable_trapezoids_round	=	0

	.extern	RDataPool
R_DATA_THR_faces		=	RDataPool+670*4
R_DATA_THR_near_faces	=	RDataPool+105*4

TRender_thr		=	8

TSL_z_thr		=	0x480	| Z threshold for tesselation

rearrange_tmap_innerloop	=	0

tmap_16bit		=		1
