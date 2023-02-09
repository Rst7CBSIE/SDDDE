#include "support/gcc8_c_support.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <exec/execbase.h>
#include <graphics/gfxmacros.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include <hardware/cia.h>

#include "render.h"

#define CLEAR_CHUNKY_BY_BLITTER

//For loading SPYRO uncomment and change bm_sz_bits	= 7 in render_utils.s
//#define LOAD_SPYRO

struct ExecBase *SysBase ALIGN4;
volatile struct Custom * const custom=(struct Custom*)0xdff000;
struct DosLibrary *DOSBase ALIGN4;
struct GfxBase *GfxBase ALIGN4;
volatile struct CIA * const ciaa=(struct CIA*)0xBFE001;
volatile struct CIA * const ciab=(struct CIA*)0xBFD000;

typedef unsigned char *va_list;
#define va_start(ap, lastarg) ((ap)=(va_list)(&lastarg+1)) 

void putchar(void);

__attribute__((noinline)) __attribute__((optimize("O1")))
void printf(const char* fmt, ...) 
{
	va_list vl;
	va_start(vl, fmt);
	RawDoFmt((CONST_STRPTR)fmt, vl, putchar, 0);
	void flush_stdout(void);
	flush_stdout();
}

static char CompleteFileName[256];
static char *CFN_name ALIGN4;

void SetDataDir(const char *s)
{
	unsigned long l=strlen(s);
	CFN_name=CompleteFileName+l;
	memcpy(CompleteFileName,s,l);
	printf("Data directory set to %s\n",s);
}

BPTR XOpen(const char *name)
{
	unsigned long l=strlen(name)+1;
	memcpy(CFN_name,name,l);
	return Open(CompleteFileName,MODE_OLDFILE);	
}

//backup
static UWORD SystemInts;
static UWORD SystemDMA;
static UWORD SystemADKCON;
static volatile APTR VBR ALIGN4 =0;
static APTR SystemIrq ALIGN4;
static APTR CiaIrq ALIGN4;
 
struct View *ActiView ALIGN4;

static APTR GetVBR(void) {
	APTR vbr = 0;
	UWORD getvbr[] = { 0x4e7a, 0x0801, 0x4e73 }; // MOVEC.L VBR,D0 RTE

	if (SysBase->AttnFlags & AFF_68010) 
		vbr = (APTR)Supervisor((ULONG (*)())getvbr);

	return vbr;
}

void SetInterruptHandler(APTR interrupt) {
	*(volatile APTR*)(((UBYTE*)VBR)+0x6c) = interrupt;
}

APTR GetInterruptHandler() {
	return *(volatile APTR*)(((UBYTE*)VBR)+0x6c);
}

void SetInterruptHandler2(APTR interrupt) {
	*(volatile APTR*)(((UBYTE*)VBR)+0x68) = interrupt;
}

APTR GetInterruptHandler2() {
	return *(volatile APTR*)(((UBYTE*)VBR)+0x68);
}

//vblank begins at vpos 312 hpos 1 and ends at vpos 25 hpos 1
//vsync begins at line 2 hpos 132 and ends at vpos 5 hpos 18 
void WaitVbl() {
	//debug_start_idle();
	while (1) {
		volatile ULONG vpos=*(volatile ULONG*)0xDFF004;
		vpos&=0x1ff00;
		if (vpos!=(311<<8))
			break;
	}
	while (1) {
		volatile ULONG vpos=*(volatile ULONG*)0xDFF004;
		vpos&=0x1ff00;
		if (vpos==(311<<8))
			break;
	}
	//debug_stop_idle();
}

void WaitLine(USHORT line) {
	while (1) {
		volatile ULONG vpos=*(volatile ULONG*)0xDFF004;
		if(((vpos >> 8) & 511) == line)
			break;
	}
}

inline void WaitBlt() {
	UWORD tst=custom->dmaconr; //*(volatile UWORD*)&custom->dmaconr; //for compatiblity a1000
	(void)tst;
	//while (*(volatile UWORD*)&custom->dmaconr&(1<<14)) {} //blitter busy wait
	while(custom->dmaconr & (1<<14));
}

void TakeSystem() {
	ActiView=GfxBase->ActiView; //store current view
	OwnBlitter();
	WaitBlit();	
	Disable();
	
	//Save current interrupts and DMA settings so we can restore them upon exit. 
	SystemADKCON=custom->adkconr;
	SystemInts=custom->intenar;
	SystemDMA=custom->dmaconr;
	custom->intena=0x7fff;//disable all interrupts
	custom->intreq=0x7fff;//Clear any interrupts that were pending
	
	WaitVbl();
	WaitVbl();
	custom->dmacon=0x7fff;//Clear all DMA channels

	//set all colors black
	for(int a=0;a<32;a++)
		custom->color[a]=0;

	LoadView(0);
	WaitTOF();
	WaitTOF();

	WaitVbl();
	WaitVbl();

	VBR=GetVBR();
	SystemIrq=GetInterruptHandler(); //store interrupt register
	CiaIrq=GetInterruptHandler2();
}

void FreeSystem() { 
	WaitVbl();
	custom->intena=0x7fff;//disable all interrupts
	custom->intreq=0x7fff;//Clear any interrupts that were pending
	WaitBlt();
	custom->dmacon=0x7fff;//Clear all DMA channels

	//restore interrupts
	SetInterruptHandler(SystemIrq);
	SetInterruptHandler2(CiaIrq);

	/*Restore system copper list(s). */
	custom->cop1lc=(ULONG)GfxBase->copinit;
	custom->cop2lc=(ULONG)GfxBase->LOFlist;
	custom->copjmp1=0x7fff; //start coppper

	/*Restore all interrupts and DMA settings. */
	custom->intena=SystemInts|0x8000;
	custom->dmacon=SystemDMA|0x8000;
	custom->adkcon=SystemADKCON|0x8000;

	LoadView(ActiView);
	WaitTOF();
	WaitTOF();
	WaitBlit();	
	DisownBlitter();
	Enable();
}

inline short MouseLeft(){return !((*(volatile UBYTE*)0xbfe001)&64);}	
inline short MouseRight(){return !((*(volatile UWORD*)0xdff016)&(1<<10));}

USHORT* copPlanesL[8]; //Pointers to low cmove to bitplanes pointers

// DEMO - INCBIN
static inline USHORT* copSetPlanes(UBYTE bplPtrStart,USHORT* copListEnd, UBYTE **planes,int numPlanes, USHORT **store_L) {
	for (USHORT i=0;i<numPlanes;i++) {
		ULONG addr=(ULONG)planes[i];
		*copListEnd++=offsetof(struct Custom, bplpt[0]) + (i + bplPtrStart) * sizeof(APTR);
		if (store_L) store_L[i]=copListEnd;
		*copListEnd++=(UWORD)(addr>>16);
		*copListEnd++=offsetof(struct Custom, bplpt[0]) + (i + bplPtrStart) * sizeof(APTR) + 2;
		*copListEnd++=(UWORD)addr;
	}
	return copListEnd;
}

inline USHORT* copWaitXY(USHORT *copListEnd,USHORT x,USHORT i) {
	*copListEnd++=(i<<8)|(x<<1)|1;	//bit 1 means wait. waits for vertical position x<<8, first raster stop position outside the left 
	*copListEnd++=0xfffe;
	return copListEnd;
}

inline USHORT* copWaitY(USHORT* copListEnd,USHORT i) {
	*copListEnd++=(i<<8)|4|1;	//bit 1 means wait. waits for vertical position x<<8, first raster stop position outside the left 
	*copListEnd++=0xfffe;
	return copListEnd;
}

inline USHORT* copSetColor(USHORT* copListCurrent,USHORT index,USHORT color) {
	*copListCurrent++=offsetof(struct Custom, color[index]);
	*copListCurrent++=color;
	return copListCurrent;
}


// set up a 320x256 lowres display
inline USHORT* screenScanDefault(USHORT* copListEnd) 
{
	const USHORT x=129;
	const USHORT width=320;
	const USHORT height=256;
	const USHORT y=44;
	const USHORT RES=17; //For FMODE=3
	USHORT xstop = x+width;
	USHORT ystop = y+height;
	USHORT fw=(x-RES)/2;

	*copListEnd++ = offsetof(struct Custom, ddfstrt);
	*copListEnd++ = 0x38; //fw;
	*copListEnd++ = offsetof(struct Custom, ddfstop);
	*copListEnd++ = 0xC0; //fw+(((width>>4)-1)<<3);
	*copListEnd++ = offsetof(struct Custom, diwstrt);
	*copListEnd++ = x+(y<<8);
	*copListEnd++ = offsetof(struct Custom, diwstop);
	*copListEnd++ = (xstop-256)+((ystop-256)<<8);
	return copListEnd;
}

ULONG RICOUNTER(void);

UBYTE* planes[6] ALIGN4;

UBYTE* planes2[4] ALIGN4;

USHORT *GenSplitScreen(USHORT *copPtr, UBYTE **planes)
{
	copPtr = copWaitY(copPtr,244);
	*copPtr++ = offsetof(struct Custom, bplcon0);
	*copPtr++ = (0<<10)/*dual pf*/|(1<<9)/*color*/|(0<<11)/*ham*/|((0)<<12)/*num bitplanes*/|(1<<0)/*enable bplcon3*/;
	*copPtr++=offsetof(struct Custom, bpl1mod); //odd planes   1,3,5
	*copPtr++=0;
	*copPtr++ = offsetof(struct Custom, ddfstop);
	*copPtr++ = 0xA0;
	copPtr=copSetPlanes(0,copPtr,planes,4,NULL);
	copPtr=copSetColor(copPtr,1,0xCCC);
	copPtr = copWaitY(copPtr,245);
	*copPtr++ = offsetof(struct Custom, bplcon0);
	*copPtr++ = (0<<10)/*dual pf*/|(1<<9)/*color*/|(0<<11)/*ham*/|((1)<<12)/*num bitplanes*/|(1<<0)/*enable bplcon3*/;
	return copPtr;
}

extern const UBYTE font[2048] ALIGN4;

static void drawchar(UBYTE *d, UBYTE c)
{
	const UBYTE *s=font+c*8;
	d[0*40]=*s++;
	d[1*40]=*s++;
	d[2*40]=*s++;
	d[3*40]=*s++;
	d[4*40]=*s++;
	d[5*40]=*s++;
	d[6*40]=*s++;
	d[7*40]=*s++;
}

static void drawstr(UBYTE *d, const char *s)
{
	UBYTE c;
	while((c=*s++)!=0)
	{
		drawchar(d++,c);
	}
}

PROFILER_DATA profiler_data[256];
PROFILER_DATA *ProfilerWP;
USHORT ProfilerStartScanline;

void StartProfiler(void)
{
	ProfilerWP=profiler_data;
	USHORT t;
	ciab->ciatodhi;
	t=ciab->ciatodmid;
	t<<=8;
	t|=ciab->ciatodlow;
	ProfilerStartScanline=t;
}

static inline void Profile(USHORT stage)
{
	ProfilerWP->stage=stage;
	USHORT t;
	ciab->ciatodhi;
	t=ciab->ciatodmid;
	t<<=8;
	t|=ciab->ciatodlow;
	ProfilerWP->scanline=t-ProfilerStartScanline;
	ProfilerWP++;
}

void ClearChunkyScreen(UBYTE *d);
void ZeroChunkyScreen(UBYTE *d);

//INCBIN(Texture, "textures.bin")

UBYTE *Texture ALIGN4;

UBYTE *MemoryPoolAligned16 ALIGN4;

R_DATA* RDataPool[R_DATA_TOTAL_SZ] ALIGN4;
static R_DATA RDataBuffer[R_DATA_TOTAL_SZ] ALIGN4;

void InitRDataPool(void)
{
    for (int i = 0; i < R_DATA_TOTAL_SZ; i++)
    {
        RDataPool[i] = RDataBuffer + i;
    }
    RDataPool_top=RDataPool;
}

CAMERA MovingCam=
{
#ifdef LOAD_SPYRO
    (0xffffdfaa+0x100)*2,
    (0xfffffa63)*2,
    (0xfffffba4)*2,
	{
		0x4000,0,0,
		0,0x4000,0,
		0,0,0x4000,
	},
#else
#if 1
	2380*2,
	-630*2,
	0,
	{
		0,0,-0x4000,
		0,0x4000,0,
		0,0,0x4000,
	},
#else
	0x00000000,
	0x00000000,
	0x00000000,
	{
		0x4000,0,0,
		0,0x4000,0,
		0,0,0x4000,
	},
#endif
#endif
	0
};


CAMERA RenderCam[2] ALIGN4;

volatile int RenderCamIdx ALIGN4;

UFIXP16 _SQRtab[MAX_SQR - MIN_SQR] ALIGN4;

__attribute__((optimize("Os")))
__attribute__((noinline))
void InitRotations(void)
{
	RenderCam[0]=MovingCam;
    for (int i = MIN_SQR; i < MAX_SQR; i++)
    {
        FIXP32 v;
        v = i;
        v *= i;
        v += 0x8000;
        v >>= 14+2;
        SQRtab[i] = (UFIXP16)v;
    }
}

//__attribute__((optimize("Os")))
static inline void RotateMTX(FIXP16* RM, FIXP16 YAW, FIXP16 PITCH, FIXP16 ROLL)
{
	register volatile FIXP16* _a0 ASM("a0") = RM;
	register volatile FIXP16 _d0 ASM("d0") = YAW;
	register volatile FIXP16 _d1 ASM("d1") = PITCH;
	register volatile FIXP16 _d2 ASM("d2") = ROLL;
	__asm volatile ("jsr RotateMTXasm"
	:"+r"(_a0),"+r"(_d0),"+r"(_d1),"+r"(_d2)
	:
	:"cc","memory","a1","a2","d3","d4","d5","d6","d7");
	_a0=RM;
	__asm volatile ("jsr NormalizeMTXasm"
	:"+r"(_a0)
	:
	:"cc","memory","a1","a2","a3","d0","d1","d2","d3","d4","d5","d6","d7");
}

static inline void RotateWorld(TFACE *face, CAMERA *cam)
{
	register volatile UFIXP16 *_a0 ASM("a0") = SQRtab;
	register volatile TFACE* _a6 ASM("a6") = face;
	register volatile TFACE* _a5 ASM("a5") = NULL;
	register volatile FIXP32 _d4 ASM("d4") = cam->X;
	register volatile FIXP32 _d5 ASM("d5") = cam->Y;
	register volatile FIXP32 _d6 ASM("d6") = cam->Z;
	static ULONG RenderCounter;
	register volatile ULONG _d7 ASM("d7") = ++RenderCounter;
	__asm volatile ("jsr RotateWorld_asm"
	:"+r"(_a5),"+r"(_a6),"+r"(_d4),"+r"(_d5),"+r"(_d6),"+r"(_a0)
	:"r"(_d7)
	:"cc","memory","a1","a2","a3","a4","d0","d1","d2","d3");
}

static inline void PrepareWorldRotation(CAMERA *cam)
{
	register volatile FIXP16* _a0 ASM("a0") = cam->RMTX;
	__asm volatile ("jsr PrepareWorldRotation_asm"
	:"+r"(_a0)
	:
	:"cc","memory","d0","d1","d2","d3","a1");
}

FIXP32 BMATRIX[256 * 3 * 3] ALIGN4;// __attribute__ ((aligned (4)));

static inline void ConstructBMATRIX(FIXP16* m, volatile FIXP32 *cc)
{
    register volatile FIXP32* _a0 ASM("a0") = BMATRIX;
	register volatile FIXP16* _a1 ASM("a1") = m;
	register volatile FIXP32* _a2 ASM("a2") = cc;
	__asm volatile ("jsr ConstructBMATRIX_asm"
	:"+r"(_a0),"+r"(_a1),"+r"(_a2)
	:
	:"cc","memory","a3","a4","d2","d2","d3","d4","d5","d6","d7");
}

FPXYZ *WorldPoints ALIGN4;
ULONG WorldPoints_sz ALIGN4;
UBYTE *WorldFaces ALIGN4;
ULONG WorldFaces_sz ALIGN4;

KDTREE_NODE *kd_tree ALIGN4;
ULONG *pvs_idx ALIGN4;
UBYTE *packed_pvs ALIGN4;

KDTREE_LEAF *kd_tree_leafs ALIGN4;
KDTREE_FACE_NODE* kd_tree_faces ALIGN4;

COLLISION_ITEM *collision_list ALIGN4;
COLLISION_LEAF *collision_index ALIGN4;

static inline USHORT reverse16(USHORT a)
{
	asm("ROR.W #8,%0":"+d"(a)::"cc");
	return a;
}

static inline ULONG reverse32(ULONG a)
{
	asm(
		"ROR.W #8,%0\n"
		"SWAP %0\n"
		"ROR.W #8,%0"
		:"+d"(a)::"cc");
	return a;
}

TFACE *FindFaceById(ULONG id)
{
	ULONG N=0;
    ULONG i = 0;
    while (i < WorldFaces_sz)
    {
		N++;
        TFACE* f = (TFACE*)(WorldFaces + i);
		if (N==id) return f;
        i += sizeof(TFACE);
        for (TVERTEX* v = f->vertexes; v->p; v++) i += sizeof(TVERTEX);
		i+=sizeof(FPXYZ*);
    }
	return NULL;
}

__attribute__((optimize("Os")))
__attribute__((noinline))
int LoadWorld(void)
{
	LONG sz;
	BPTR file;
	printf("Load textures...");
	file=XOpen("texture.bin");
	if (!file)
	{
		printf("No file!\n");
		return 1;
	}
	sz=Read(file,Texture,0x40000);
	Close(file);
	printf("Ok, %ld bytes loaded!\n",sz);
	printf("Load FPXYZ data: ");
	file=XOpen("FPXYZ.bin");
	if (!file)
	{
		printf("No file!\n");
		return 1;
	}
	WorldPoints_sz=Seek(file,0,OFFSET_END);
	WorldPoints_sz=Seek(file,0,OFFSET_BEGINING);
	WorldPoints=AllocVec(WorldPoints_sz,MEMF_ANY);
	if ((ULONG)Read(file,WorldPoints,WorldPoints_sz)!=WorldPoints_sz)
	{
		printf("Read error!\n");
		Close(file);
		return 1;
	}
	Close(file);
	printf("Ok, loaded %ld bytes!\n",WorldPoints_sz);
	printf("Load FACES data: ");
	file=XOpen("FACES.bin");
	if (!file)
	{
		printf("No file!\n");
		return 1;
	}
	WorldFaces_sz=Seek(file,0,OFFSET_END);
	WorldFaces_sz=Seek(file,0,OFFSET_BEGINING);
	WorldFaces=AllocVec(WorldFaces_sz,MEMF_ANY);
	if ((ULONG)Read(file,WorldFaces,WorldFaces_sz)!=WorldFaces_sz)
	{
		printf("Read error!\n");
		Close(file);
		return 1;
	}
	Close(file);
	printf("Ok, loaded %ld bytes!\n",WorldFaces_sz);
	printf("Adjust faces...");
	ULONG N=0;
    ULONG i = 0;
	ULONG tdisp=(ULONG)Texture >> 1;
	TFACE **AllFaces;
	AllFaces=AllocVec(10000*sizeof(TFACE *),MEMF_ANY);
	TFACE **af=AllFaces;
    while (i < WorldFaces_sz)
    {
		N++;
        TFACE* f = (TFACE*)(WorldFaces + i);
		*af++=f;
        i += sizeof(TFACE);
		f->T+=tdisp;
        for (TVERTEX* v = f->vertexes; v->p; v++)
		{
			v->p=WorldPoints+(((ULONG)(v->p))-1);
			i += sizeof(TVERTEX);
		}
		i+=sizeof(FPXYZ*);
    }
	printf("Ok, total %ld faces processed!\n",N);
	printf("Load KDTREE: ");
	file=XOpen("kdtree.bin");
	if (!file)
	{
		printf("No file!\n");
		return 1;
	}
	sz=Seek(file,0,OFFSET_END);
	sz=Seek(file,0,OFFSET_BEGINING);
	kd_tree=AllocVec(sz,MEMF_ANY);
	if (Read(file,kd_tree,sz)!=sz)
	{
		printf("Read error!\n");
		return 1;
	}
	Close(file);
	sz/=sizeof(KDTREE_NODE);
	for(int i=0; i<sz; i++)
	{
		KDTREE_NODE *t=kd_tree+i;
		t->next_index=reverse16(t->next_index);
	}
	printf("Ok!\n");

	printf("Load packed PVS: ");
	file=XOpen("pvs.bin");
	if (!file)
	{
		printf("No file!\n");
		return 1;
	}
	sz=Seek(file,0,OFFSET_END);
	sz=Seek(file,0,OFFSET_BEGINING);
	packed_pvs=AllocVec(sz,MEMF_ANY);
	if (Read(file,packed_pvs,sz)!=sz)
	{
		printf("Read error!\n");
		return 1;
	}
	Close(file);
	printf("Ok!\n");

	printf("Load PVS index: ");
	file=XOpen("pvs_idx.bin");
	if (!file)
	{
		printf("No file!\n");
		return 1;
	}
	sz=Seek(file,0,OFFSET_END);
	sz=Seek(file,0,OFFSET_BEGINING);
	pvs_idx=AllocVec(sz,MEMF_ANY);
	if (Read(file,pvs_idx,sz)!=sz)
	{
		printf("Read error!\n");
		return 1;
	}
	Close(file);
	sz/=sizeof(ULONG);
	for(int i=0; i<sz; i++)
	{
		pvs_idx[i]=reverse32(pvs_idx[i]);
	}
	printf("Ok!\n");

	printf("Load FACELIST: ");
	file=XOpen("faces_in_leafs.bin");
	if (!file)
	{
		printf("No file!\n");
		return 1;
	}
	sz=Seek(file,0,OFFSET_END);
	sz=Seek(file,0,OFFSET_BEGINING);
	kd_tree_faces=AllocVec(sz,MEMF_ANY);
	if (Read(file,kd_tree_faces,sz)!=sz)
	{
		printf("Read error!\n");
		return 1;
	}
	Close(file);
	sz/=sizeof(KDTREE_FACE_NODE);
	for(int i=0; i<sz; i++)
	{
		kd_tree_faces[i].face=AllFaces[reverse32(kd_tree_faces[i].face_id)-1];
	}
	KDTREE_FACE_NODE *end_of_facelist=kd_tree_faces+sz;
	printf("Ok!\n");

	printf("Load FACELIST index: ");
	file=XOpen("fil_idx.bin");
	if (!file)
	{
		printf("No file!\n");
		return 1;
	}
	sz=Seek(file,0,OFFSET_END);
	sz=Seek(file,0,OFFSET_BEGINING);
	sz/=sizeof(KDTREE_LEAF_DISK);
	kd_tree_leafs=AllocVec((sz+1)*sizeof(KDTREE_LEAF),MEMF_ANY);
	KDTREE_LEAF_DISK *ld=(KDTREE_LEAF_DISK*)
		(
			((UBYTE*)kd_tree_leafs)
			+sz*(sizeof(KDTREE_LEAF)-sizeof(KDTREE_LEAF_DISK))
		);
	LONG sz2=sz*sizeof(KDTREE_LEAF_DISK);
	if (Read(file,ld,sz2)!=sz2)
	{
		printf("Read error!\n");
		return 1;
	}
	Close(file);
	for(int i=0; i<sz; i++)
	{
		KDTREE_LEAF *l=kd_tree_leafs+i;
		l->I=BMATRIX+0x180+ld->I*3;
		l->J=BMATRIX+0x480+ld->J*3;
		l->K=BMATRIX+0x780+ld->K*3;
		l->FaceList=kd_tree_faces+reverse32(ld->kdtf_offset)/sizeof(KDTREE_FACE_NODE);
		ld++;
	}
	kd_tree_leafs[sz].FaceList=end_of_facelist;
	printf("Ok!\n");

	printf("Load collision list: ");
	file=XOpen("clist.bin");
	if (!file)
	{
		printf("No file!\n");
		return 1;
	}
	sz=Seek(file,0,OFFSET_END);
	sz=Seek(file,0,OFFSET_BEGINING);
	collision_list=AllocVec(sz,MEMF_ANY);
	if (Read(file,collision_list,sz)!=sz)
	{
		printf("Read error!\n");
		return 1;
	}
	Close(file);
	sz/=sizeof(COLLISION_ITEM);
	for(int i=0; i<sz; i++)
	{
		COLLISION_ITEM *p=collision_list+i;
		ULONG idx;
		idx=reverse32(p->face_id);
		if (idx)
		{
			p->face=AllFaces[idx-1];
		}
	}
	printf("Ok!\n");
	FreeVec(AllFaces);

	printf("Load collision index: ");
	file=XOpen("cindex.bin");
	if (!file)
	{
		printf("No file!\n");
		return 1;
	}
	sz=Seek(file,0,OFFSET_END);
	sz=Seek(file,0,OFFSET_BEGINING);
	collision_index=AllocVec(sz,MEMF_ANY);
	if (Read(file,collision_index,sz)!=sz)
	{
		printf("Read error!\n");
		return 1;
	}
	Close(file);
	sz/=sizeof(COLLISION_LEAF);
	for(int i=0; i<sz; i++)
	{
		COLLISION_LEAF *c=collision_index+i;
		c->list=collision_list + reverse32(c->offset)/sizeof(COLLISION_ITEM*);
	}
	printf("Ok!\n");
	return 0;
}

void FreeWorld(void)
{
	FreeVec(WorldPoints);
	FreeVec(WorldFaces);
	FreeVec(kd_tree);
	FreeVec(pvs_idx);
	FreeVec(packed_pvs);
	FreeVec(kd_tree_faces);
	FreeVec(kd_tree_leafs);
	FreeVec(collision_index);
	FreeVec(collision_list);
}

__attribute__((optimize("Os")))
__attribute__((noinline))
void InitDitherColorTable(void)
{
	extern ULONG dither_color_table;
	ULONG *d=&dither_color_table;
	for(int c=0; c<64; d++,c++)
	{
		ULONG o=0;
		if (c & 0x20) o |= 0x80;
		if (c & 0x10) o |= 0x08;
		if (c & 0x08) o |= 0x40;
		if (c & 0x04) o |= 0x04;
		if (c & 0x02) o |= 0x30;
		if (c & 0x01) o |= 0x03;
		//Screen:
		//c0 c1
		//c2 c3
		//in register - c0 c2 c1 c3
		d[64*0]=(o<<24)|(o<<16)|(o<<8)|o;
		d[64*1]=(o<<16)|(o<<8)|o;
		d[64*2]=(o<<16)|(o<<8);
		d[64*3]=(o<<8);
	}
}

volatile const char *ERROR_TEXT ALIGN4;

__attribute__((noinline))
void ERROR(const char *error_text, USHORT v)
{
	ERROR_TEXT=error_text;
	for(;;)
	{
		custom->color[0]=0;
		custom->color[0]=v;
	}
}

#if 0
static inline void DBG(USHORT color)
{
	custom->color[0]=color;
}
#else
static inline void DBG(USHORT color)
{
}
#endif

TFACE *GetAllFaces(void)
{
    TFACE* list = NULL;
    ULONG i = 0;
    while (i < WorldFaces_sz)
    {
        TFACE* f = (TFACE*)(WorldFaces + i);
        f->next = list;
        list = f;
        i += sizeof(TFACE);
        for (TVERTEX* v = f->vertexes; v->p; v++) i += sizeof(TVERTEX);
		i+=sizeof(FPXYZ*);
    }
    return list;
}

static inline KDTREE_NODE* FindKDtreeNode(FIXP32 X, FIXP32 Y, FIXP32 Z)
{
	register volatile KDTREE_NODE* _a0 ASM("a0") = kd_tree;
	register volatile FIXP32 _d0 ASM("d0") = X;
	register volatile FIXP32 _d1 ASM("d1") = Y;
	register volatile FIXP32 _d2 ASM("d2") = Z;
	__asm volatile ("jsr FindKDtreeNode_asm"
	:"+r"(_a0),"+r"(_d0),"+r"(_d1),"+r"(_d2)
	:
	:"cc","d3","d4","d5","a1");
	return (KDTREE_NODE*)_a0;
}

static inline TFACE* DecrunchPVS(KDTREE_NODE *node)
{
	static ULONG stored_seq;
	static ULONG last_idx;
    ULONG idx;
    idx = pvs_idx[node - kd_tree];
    if (idx == 0xFFFFFFFF)
		idx=last_idx;// return NULL;
	last_idx=idx;
	//return DecrunchPVScaller(packed_pvs + idx, kd_tree_leafs, ++stored_seq);
    register volatile UBYTE* _a1 ASM("a1") = packed_pvs + idx;
    register volatile KDTREE_LEAF* _a2 ASM("a2")= kd_tree_leafs;
    register volatile TFACE* _a0 ASM("a0")=NULL;
	register volatile ULONG _d7 ASM("d7") = ++stored_seq;
	__asm volatile ("jsr DecrunchPVS_asm"
		:"+r"(_a0),"+r"(_a2),"+r"(_a1)
		:"r"(_d7)
		:"cc","memory"
	);
	return (TFACE*)_a0;	
}

volatile UBYTE ClearChunkyScreenState;

void SortFaces(void);
void PrepareSortFaces(void);
void RenderF(void);
void RenderT(void);

__attribute__((optimize("Os")))
__attribute__((noinline))
void RenderWorld(CAMERA *cam)
{
	TFACE *face;
	Profile(2);
	DBG(0x00F);
	ConstructBMATRIX(cam->RMTX,&cam->X);
	Profile(6);
	PrepareSortFaces();
	Profile(3);
	DBG(0xF00);
	#if 0
	face=GetAllFaces();
	#else
	KDTREE_NODE *node;
	node=FindKDtreeNode(cam->X,cam->Y,cam->Z);
	DBG(0xF0F);
	Profile(4);
	face=DecrunchPVS(node);
	#endif
	DBG(0x000);
	if (!face) return; //No world!!!
	DBG(0x0F0);
	Profile(5);
	RotateWorld(face,cam);
	DBG(0x0FF);
	Profile(7);
	SortFaces();
	DBG(0x000);
	Profile(8);
	//PrepareAllRenderSequence();
	Profile(9);
	RenderF();
	if (RDataPool_top!=RDataPool)
	{
		ERROR("Not all rfaces/rvertexes free!",0xF00);
	}
	Profile(10);
	RenderT();
	//Check for errors
	if (RDataPool_top!=RDataPool)
	{
		ERROR("Not all rfaces/rvertexes free!",0x0F0);
	}
	#if 0
	if (TRAPEZOIDS_POINTER!=TRAPEZOIDS)
	{
		ERROR("Not all trapezoids renderd!");
	}
	if (SLICES_POINTER!=SLICES_BUFFER)
	{
		ERROR("Not all slices rendered!");
	}
	#endif
L_end:
}

UBYTE *ChunkFrames[2] ALIGN4;

typedef struct
{
	UBYTE *plane0;
	UBYTE *plane1;
}BitmapBuf;

BitmapBuf BitmapBufs[2] ALIGN4;

BitmapBuf *BitmapFrames[2] ALIGN4 =
{
	BitmapBufs+0,
	BitmapBufs+1
};

UBYTE *Xtab[160*2] ALIGN4;

UBYTE **Xtabs[2] ALIGN4=
{
	Xtab+0,
	Xtab+160
};

static inline UBYTE *XtabCorrect_ysC(UBYTE *s)
{
	register volatile UBYTE *_a0 ASM("a0") = s;
	__asm volatile("jsr XtabCorrect_ys":"+r"(_a0)::"cc");
	return (UBYTE*)_a0;
}

__attribute__((optimize("Os")))
__attribute__((noinline))
void InitXtab(int N)
{
	UBYTE **p=Xtabs[N];
	UBYTE *s = ChunkFrames[N];
	s=XtabCorrect_ysC(s);
	USHORT i;
	i=40;
	do
	{
		*p++=s;
		*p++=s+8000;
		*p++=s+1;
		*p++=s+8001;
		s+=200;
	} 
	while (--i);
}

USHORT *copper2[2] ALIGN4;

__attribute__((optimize("Os")))
__attribute__((noinline))
void CreateBlitterCList(int N)
{
	USHORT *cp=copper2[N];
	//Prepare blitter
	//*cp++=offsetof(struct Custom,color);
	//*cp++=0x888;
	*cp++=offsetof(struct Custom,bltamod);
	*cp++=0;
	*cp++=offsetof(struct Custom,bltbmod);
	*cp++=0;
	*cp++=offsetof(struct Custom,bltdmod);
	*cp++=80-2;
	*cp++=offsetof(struct Custom,bltafwm);
	*cp++=0xFFFF;
	*cp++=offsetof(struct Custom,bltalwm);
	*cp++=0xFFFF;
	*cp++=offsetof(struct Custom,bltcdat);
	*cp++=0x0F0F;
	//Init MSB bitplane
	*cp++=offsetof(struct Custom,bltcon0);
	*cp++=SRCA | SRCB | DEST | (0<<ASHIFTSHIFT) | (0x50 | 0x88); //(A & ~C) | (B & C)
	*cp++=offsetof(struct Custom,bltcon1);
	*cp++=(4<<BSHIFTSHIFT); 
	ULONG a;
	a=(ULONG)ChunkFrames[N];
	*cp++=offsetof(struct Custom,bltapt);
	*cp++=a>>16;
	*cp++=offsetof(struct Custom,bltapt)+2;
	*cp++=a;
	a+=8000;
	*cp++=offsetof(struct Custom,bltbpt);
	*cp++=a>>16;
	*cp++=offsetof(struct Custom,bltbpt)+2;
	*cp++=a;
	a=(ULONG)(BitmapBufs[N].plane1);
	//Setup 40 blits
	USHORT n;
	n=40;
	do
	{
		*cp++=offsetof(struct Custom,bltdpt);
		*cp++=a>>16;
		*cp++=offsetof(struct Custom,bltdpt)+2;
		*cp++=a;
		*cp++=offsetof(struct Custom,bltsize);
		*cp++=(100<<6)|1; //w=1 h=100
		*cp++=0x0001; //CWAIT only blitter
		*cp++=0x0000;
		a+=2;
	} while(--n);
	//Init LSB bitplane
	*cp++=offsetof(struct Custom,bltcon0);
	*cp++=SRCA | SRCB | DEST | (4<<ASHIFTSHIFT) | (0x50 | 0x88); //(A & ~C) | (B & C)
	*cp++=offsetof(struct Custom,bltcon1);
	*cp++=(0<<BSHIFTSHIFT) | BLITREVERSE;
	a=(ULONG)ChunkFrames[N];
	a+=7998;
	*cp++=offsetof(struct Custom,bltapt);
	*cp++=a>>16;
	*cp++=offsetof(struct Custom,bltapt)+2;
	*cp++=a;
	a+=8000;
	*cp++=offsetof(struct Custom,bltbpt);
	*cp++=a>>16;
	*cp++=offsetof(struct Custom,bltbpt)+2;
	*cp++=a;
	a=(ULONG)(BitmapBufs[N].plane0);
	a+=7998;
	n=40;
	do
	{		
		*cp++=offsetof(struct Custom,bltdpt);
		*cp++=a>>16;
		*cp++=offsetof(struct Custom,bltdpt)+2;
		*cp++=a;
		*cp++=offsetof(struct Custom,bltsize);
		*cp++=(100<<6)|1; //w=1 h=100
		*cp++=0x0001; //CWAIT only blitter
		*cp++=0x0000;
		a-=2;
	} while (--n);
	//Rise interrupt
	//*cp++=offsetof(struct Custom,color);
	//*cp++=0x008;
	*cp++=offsetof(struct Custom,intreq);
	*cp++=INTF_SETCLR | INTF_COPER;
	cp = GenSplitScreen(cp, planes2);
	*cp++=0xffff;
	*cp++=0xfffe; // end copper list
}

volatile USHORT* copJ2_position ALIGN4; //Poiters to trig copjmp2

__attribute__((optimize("Os")))
__attribute__((noinline))
void StartC2P(void)
{
	while(*copJ2_position!=0x01FE);
#ifdef CLEAR_CHUNKY_BY_BLITTER
	while(!ClearChunkyScreenState);
	Profile(26);
	(void)(custom->dmaconr);
	while(custom->dmaconr & (1<<14));
	ClearChunkyScreenState=0;
#endif
	*copJ2_position=offsetof(struct Custom, copjmp2);
	//Можно обменять буфера для рендера
	UBYTE **xt;
	xt=Xtabs[0];
	Xtabs[0]=Xtabs[1];
	Xtabs[1]=xt;
	UBYTE *xc;
	xc=ChunkFrames[0];
	ChunkFrames[0]=ChunkFrames[1];
	ChunkFrames[1]=xc;
}

volatile short frameCounter = 0;
volatile short C2Pstate = 0;

volatile UBYTE key_forward;
volatile UBYTE key_back;
volatile UBYTE key_left;
volatile UBYTE key_right;

static inline FIXP16 FMUL(FIXP16 a, FIXP16 b)
{
	__asm volatile (
		"muls.w %1,%0\n"
		"lsl.l #2,%0\n"
		"swap %0"
		:"+d"(a)
		:"d"(b)
		:"cc"
	);
	return a;
}

static inline UFIXP32 SignedDistance2leaf(FIXP32 X, FIXP32 Y, FIXP32 Z)
{
	register volatile FIXP32 _d0 ASM("d0") = X;
	register volatile FIXP32 _d1 ASM("d1") = Y;
	register volatile FIXP32 _d2 ASM("d2") = Z;
	register volatile UFIXP32 _d7 ASM("d7");
	register volatile KDTREE_NODE *_a0 ASM("a0") = kd_tree;
	register volatile COLLISION_LEAF *_a1 ASM("a1") = collision_index;
	__asm volatile(
		"jsr SD2leaf"
		:"=r"(_d7),"+r"(_a0),"+r"(_a1)
		:"r"(_d0),"r"(_d1),"r"(_d2)
		:"cc"
	);
	return (UFIXP32)_d7;
}

static inline void TestPlayerMove(CAMERA *mc, FIXP32 x, FIXP32 y, FIXP32 z)
{
	register volatile FIXP32 _d0 ASM("d0") = x;
	register volatile FIXP32 _d1 ASM("d1") = y;
	register volatile FIXP32 _d2 ASM("d2") = z;
	register volatile KDTREE_NODE *_a0 ASM("a0") = kd_tree;
	register volatile COLLISION_LEAF *_a1 ASM("a1") = collision_index;
	register volatile CAMERA *_a2 ASM("a2") = mc;
	//register volatile UFIXP16 *_a3 ASM("a3") = SQRtab;
	register volatile UFIXP32 _d7 ASM("d7");
	__asm volatile(
		"jsr TestPlayerMove_asm"
		:"+r"(_a0),"+r"(_a1),"=r"(_d7),"+r"(_d0),"+r"(_d1),"+r"(_d2)
		:"r"(_a2)
		:"cc"
	);
	if (_d7 >= MIN_D_WALK*MIN_D_WALK*4)
	{
		_a2->X=_d0;
		_a2->Y=_d1;
		_a2->Z=_d2;
	}
}

static inline FIXP32 RDIV2(FIXP32 v)
{
	FIXP32 zero=0;
	__asm volatile(
		"asr.l #2,%0\n"
		"addx.l %1,%0\n"
		"add.l	%0,%0"
		:"+d"(v)
		:"d"(zero)
		:"cc"
	);
	return v;
}

static BYTE old_mx;
static BYTE old_my;

__attribute__((optimize("Os")))
static __attribute__((interrupt)) void interruptHandler() 
{
	UWORD ireq=custom->intreqr;
	if (ireq & INTF_COPER)
	{
		custom->intreq=INTF_COPER;
		*copJ2_position=0x01FE; //Nop
		//Мы находимся в безопасном месте для смены видимого буфера
		//Т.е. изменение cop2lc
		USHORT *cl;
		cl=copper2[1];
		copper2[1]=copper2[0];
		copper2[0]=cl;
		custom->cop2lc=(ULONG)cl;
		//И так же не забываем обновить указатели отображения
		BitmapBuf *b;
		b=BitmapFrames[0];
		BitmapFrames[0]=BitmapFrames[1];
		BitmapFrames[1]=b;
		ULONG a0=(ULONG)b->plane0;
		ULONG a1=(ULONG)b->plane1;
		copPlanesL[0][0]=a0>>16;
		copPlanesL[0][2]=a0;
		copPlanesL[1][0]=a1>>16;
		copPlanesL[1][2]=a1;
		copPlanesL[2][0]=a0>>16;
		copPlanesL[2][2]=a0;
		copPlanesL[3][0]=a1>>16;
		copPlanesL[3][2]=a1;
		//custom->color[0]=0x000; //Complete
#ifdef CLEAR_CHUNKY_BY_BLITTER
		//Start clear chunky screen by blitter
		ClearChunkyScreenState=1; //Ready for wait blitter
		custom->bltdpt=ChunkFrames[1];
		custom->bltdmod=0;
		custom->bltcon1=0;
		custom->bltcon0=DEST;
		custom->bltsize=(200<<6)|40; //w=80 h=100
#endif
	}
	if (ireq & INTF_VERTB)
	{
		custom->intreq=INTF_VERTB;
		// DEMO - increment frameCounter
		frameCounter++;
		USHORT mxy=custom->joy0dat;
		BYTE mx=mxy;
		mx-=old_mx;
		old_mx+=mx;
		BYTE my=mxy>>8;
		my-=old_my;
		old_my+=my;
		FIXP16 yaw=mx;
		FIXP16 pitch=my;
		FIXP16 r = MovingCam.RMTX[3];
    	r >>= 4;
    	if (r > 0x100) r = 0x100; else if (r < -0x100) r = -0x100;
		RotateMTX(MovingCam.RMTX,yaw*32,pitch*32,-r);
		FIXP32 dz=0;
		FIXP32 dy=0;
		FIXP32 dx=0;
		if (key_forward) dz+=0x10;
		if (key_back) dz-=0x10;
		if (key_left) dx-=0x10;
		if (key_right) dx+=0x10;
		CAMERA *mc=&MovingCam;
		TestPlayerMove(mc,dx,dy,dz);
		CAMERA *rc=RenderCam+RenderCamIdx;
		rc->X=mc->X;
		rc->Y=mc->Y;
		rc->Z=mc->Z;
		rc->RMTX[0]=mc->RMTX[0];
		rc->RMTX[1]=mc->RMTX[3];
		rc->RMTX[2]=mc->RMTX[6];
		rc->RMTX[3]=mc->RMTX[1];
		rc->RMTX[4]=mc->RMTX[4];
		rc->RMTX[5]=mc->RMTX[7];
		rc->RMTX[6]=mc->RMTX[2];
		rc->RMTX[7]=mc->RMTX[5];
		rc->RMTX[8]=mc->RMTX[8];
	}
}

volatile UBYTE raw_key;

//__attribute__((optimize("Os")))
static __attribute__((interrupt)) void interruptHandler2() 
{
	UBYTE ir=ciaa->ciaicr;
	custom->intreq=INTF_PORTS;
	custom->intreq=INTF_PORTS;
	if (ir&1)
	{
		//TA
		ciaa->ciacra=0;
	}
	if (ir&2)
	{
		//TB
	}
	if (ir&8)
	{
		//SP
		UBYTE code=ciaa->ciasdr;
		ciaa->ciacra=0x59; //Start ACK
		code=~code;
		asm("ror.b #1,%0":"+d"(code)::"cc");
		raw_key=code;
		switch(code)
		{
		case 0x11: key_forward=1; break;
		case 0x91: key_forward=0; break;
		case 0x21: key_back=1; break;
		case 0xA1: key_back=0; break;
		case 0x20: key_left=1; break;
		case 0xA0: key_left=0; break;
		case 0x22: key_right=1; break;
		case 0xA2: key_right=0; break;
		}
	}
}


__attribute__((optimize("Os")))
__attribute__((noinline))
void PatchXtab_addrs(void)
{
	extern UBYTE **Xtab_patch1;
	extern UBYTE **Xtab_patch2;
	Xtab_patch1=Xtabs[0];
	Xtab_patch2=Xtabs[0];
}


char *i2a2(char *s, ULONG n)
{
  ULONG i=2;
  s+=i; 
  *s=0;
  do
  {
    ULONG b=n%10;
	*--s=b+'0';
	n/=10;
  }
  while (--i);
}

int main()
{
	ULONG render_time=0;
	SysBase = *((struct ExecBase**)4UL);
	//custom = (struct Custom*)0xdff000;

	// We will use the graphics library only to locate and restore the system copper list once we are through.
	GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR)"graphics.library",0);
	if (!GfxBase)
		Exit(0);

	// used for printing
	DOSBase = (struct DosLibrary*)OpenLibrary((CONST_STRPTR)"dos.library", 0);
	if (!DOSBase)
		Exit(0);

	printf("****************************\n");
	printf("*** CBSIE on air again! ****\n");
	printf("* It's Good To Be The King *\n");
	printf("****************************\n");
#ifdef LOAD_SPYRO
	SetDataDir("spyro/");
#else
	SetDataDir("omni/");
#endif
	printf("Allocate memory...");
	MemoryPoolAligned16=(UBYTE*)AllocVec(65536+0x40000,MEMF_ANY);
	if (!MemoryPoolAligned16)
	{
		printf("No enouth memory!\n");
		Exit(0);
	}
	Texture=(UBYTE*)(((ULONG)MemoryPoolAligned16 + 0xFFFF) & 0xFFFF0000);
	printf("Addr=%08lx, Rounded=%08lx, Ok\n",(ULONG)MemoryPoolAligned16,(ULONG)Texture);

	if (LoadWorld()) goto L_abort;
	printf("Init coppered blitter...\n");

	ChunkFrames[0]=(UBYTE*)AllocVec(16000,MEMF_CHIP);
	ChunkFrames[1]=(UBYTE*)AllocVec(16000,MEMF_CHIP);
	ZeroChunkyScreen(ChunkFrames[0]);
	ZeroChunkyScreen(ChunkFrames[1]);

	copper2[0]=(USHORT*)AllocVec(2048, MEMF_CHIP);
	copper2[1]=(USHORT*)AllocVec(2048, MEMF_CHIP);

	// set bitplane pointers
	struct BitMap *bitmap;
	bitmap = AllocBitMap(640,100,6,BMF_DISPLAYABLE|BMF_CLEAR,NULL);
	//memset(bitmap->Planes[0],0,80*100);
	//memset(bitmap->Planes[1],0,80*100);
	//memset(bitmap->Planes[2],0,80*100);
	//memset(bitmap->Planes[3],0,80*100);
	BitmapFrames[0]->plane0=bitmap->Planes[0];
	BitmapFrames[0]->plane1=bitmap->Planes[1];
	BitmapFrames[1]->plane0=bitmap->Planes[2];
	BitmapFrames[1]->plane1=bitmap->Planes[3];

	struct BitMap *bitmap2;
	bitmap2 = AllocBitMap(320,56,4,BMF_DISPLAYABLE|BMF_CLEAR,NULL);

	planes[0]=BitmapFrames[0]->plane0;
	planes[1]=BitmapFrames[0]->plane1;
	planes[2]=BitmapFrames[0]->plane0;
	planes[3]=BitmapFrames[0]->plane1;
	planes[4]=(UBYTE*)(bitmap->Planes[4]);
	planes[5]=(UBYTE*)(bitmap->Planes[5]);
	memset(planes[4],0x77,80*100);
	memset(planes[5],0xCC,80*100);
	planes2[0]=(UBYTE*)(bitmap2->Planes[0]);
	planes2[1]=(UBYTE*)(bitmap2->Planes[1]);
	planes2[2]=(UBYTE*)(bitmap2->Planes[2]);
	planes2[3]=(UBYTE*)(bitmap2->Planes[3]);
	//memset(planes2[0],0x80,40);
	//memset(planes2[0]+80,0x80,40);

	CreateBlitterCList(0);
	CreateBlitterCList(1);

	USHORT* copper1 = (USHORT*)AllocVec(1024, MEMF_CHIP);
	USHORT* copPtr = copper1;

	copPtr = screenScanDefault(copPtr);
	//enable bitplanes	
	*copPtr++ = offsetof(struct Custom, bplcon0);
	*copPtr++ = (0<<10)/*dual pf*/
	|(1<<9)		/*color*/
	|(1<<11)	/*ham*/
	|((6)<<12)	/*num bitplanes*/
	|(1<<0)		/*enable bplcon3*/
	|(1<<15)	/*hires*/
	;
	*copPtr++ = offsetof(struct Custom, bplcon1);	//scrolling
	*copPtr++ = 0;
	*copPtr++ = offsetof(struct Custom, bplcon2);	//playfied priority
	*copPtr++ = 1<<6;//0x24;			//Sprites have priority over playfields
	*copPtr++ = offsetof(struct Custom, bplcon3);
	*copPtr++ = 0x0C00;
	*copPtr++ = offsetof(struct Custom, fmode);
	*copPtr++ = 0x4003; //64 bit bandwidth + bscan

	const USHORT lineSize=320/8;

	//set bitplane modulo
	*copPtr++=offsetof(struct Custom, bpl1mod); //odd planes   1,3,5
	*copPtr++=0-80;
	*copPtr++=offsetof(struct Custom, bpl2mod); //even  planes 2,4,6
	*copPtr++=0;
	copPtr = copSetPlanes(0, copPtr, planes, 6, copPlanesL);

    copJ2_position=copPtr;
	*copPtr++=0x01FE;	//Nop move
	*copPtr++=0x7FFF;

	copPtr = GenSplitScreen(copPtr, planes2);

	*copPtr++=0xffff;
	*copPtr++=0xfffe; // end copper list

	printf("Precalc...");	
	InitRotations();
	InitXtab(0);
	InitXtab(1);
	InitRDataPool();
	InitDitherColorTable();
	printf("Ok\n");
	printf("Start now, use ESC for exit...");
	Delay(100);
	TakeSystem();
	WaitVbl();
#ifdef CLEAR_CHUNKY_BY_BLITTER
	ClearChunkyScreenState=1; //No wait for first time
#else
	ClearChunkyScreenState=2; //Disable blitter ready check
#endif
	USHORT mxy=custom->joy0dat;
	old_mx=mxy;
	old_my=mxy>>8;

	custom->cop2lc=(ULONG)copper2[0];
	custom->copcon=0x0002;

	WaitVbl();
	custom->cop1lc = (ULONG)copper1;
	custom->dmacon = DMAF_BLITTER;//disable blitter dma for copjmp bug
	custom->copjmp1 = 0x7fff; //start coppper
	custom->dmacon = DMAF_SETCLR | /*DMAF_BLITHOG | */DMAF_MASTER | DMAF_RASTER | DMAF_COPPER | DMAF_BLITTER;

	// Go!
	SetInterruptHandler((APTR)interruptHandler);
	SetInterruptHandler2((APTR)interruptHandler2);
	ciaa->ciacra=0;
	ciaa->ciatalo=142;
	ciaa->ciatahi=0;
	ciaa->ciaicr=0x7F;
	ciaa->ciaicr; //reset all flags
	ciaa->ciaicr=0x89; //Enable SP and TA
	custom->intena = INTF_SETCLR | INTF_INTEN | INTF_VERTB | INTF_COPER | INTF_PORTS;
	custom->intreq=(1<<INTB_VERTB);//reset vbl req
	do
	{
	//Set 1 for accurate profiling
	//Set 0 for async engine run (better for play)
#if 0
		short fc;
		fc=frameCounter;
		while(fc==frameCounter);
		frameCounter=0;
#endif
		StartProfiler();
		Profile(0);
		ULONG _render_time;
		_render_time=RICOUNTER();
		int updated_cam_idx=RenderCamIdx;
		CAMERA *cam=RenderCam+updated_cam_idx;
		RenderCamIdx=updated_cam_idx^1;
		PatchXtab_addrs();
		PrepareWorldRotation(cam);
		CacheClearU();
		//custom->color[0]=0x600;
		//ClearChunkyScreen(ChunkFrames[0]);
	#ifdef CLEAR_CHUNKY_BY_BLITTER
	#else
		Profile(1);
		ZeroChunkyScreen(ChunkFrames[0]);
	#endif
		RenderWorld(cam);
		Profile(253);
		StartC2P();
		Profile(254);
		_render_time=RICOUNTER()-_render_time;
		render_time-=render_time>>2;
		render_time+=_render_time>>2;
		//drawchar(planes2[0],'R');
		char s[20];
		ULONG rt=render_time>>8;
		s[0]=rt/312+'0';
		s[1]='.';
		i2a2(s+2,100*(rt%312)/312);
		drawstr(planes2[0],s);
		Profile(255);
	}
	while(raw_key!=0x45);
	//while(!MouseLeft());
	// END
	FreeSystem();
L_abort:
	printf("Exit!\n");
	FreeBitMap(bitmap);
	FreeBitMap(bitmap2);
	FreeVec(MemoryPoolAligned16);
	FreeVec(copper1);
	FreeVec(copper2[0]);
	FreeVec(copper2[1]);
	FreeVec(ChunkFrames[0]);
	FreeVec(ChunkFrames[1]);
	render_time>>=8;
	printf("render_time=%ld.%02ld\n",render_time/312,100*(render_time%312)/312);
	printf("Mission complete!\n");
	int prev_sl=0;
	for(PROFILER_DATA *p=profiler_data; p!=ProfilerWP; p++)
	{
		const char *s;
		s="???";
		switch(p->stage)
		{
		case 0: s="Init"; break;
		case 1: s="Clear chunkyscreen"; break;
		case 2: s="Construct BMATRIX"; break;
		case 3: s="Find player node"; break;
		case 4: s="Decrunch PVS"; break;
		case 5: s="Rotate world"; break;
		case 6: s="Prepare sort"; break;
		case 7: s="Do sort"; break;
		case 8: s="Prepare render"; break;
		case 9: s="Filled render"; break;
		case 10: s="Textured render"; break;
		case 11: s="Backface culling"; break;
		case 12: s="RMTX multiply"; break;
		case 13: s="Box check"; break;
		case 14: s="fmap culling"; break;
		case 15: s="fmap scoords"; break;
		case 16: s="fmap faceloop"; break;
		case 17: s="tmap section"; break;
		case 18: s="tmap scoords"; break;
		case 19: s="tmap faceloop"; break;
		case 20: s="trapezoids begin"; break;
		case 21: s="trapezoids end"; break;
		case 22: s="slices begin"; break;
		case 23: s="slices end"; break;
		case 24: s="Filled render cleanup"; break;
		case 25: s="Textured render cleanup"; break;
		case 26: s="Wait for blitter"; break;
		case 27: s="Render far faces"; break;
		case 28: s="Tesselation"; break;
		case 253: s="StartC2P"; break;
		case 254: s="Debug"; break;
		case 255: s="Complete"; p[1].scanline=p->scanline; break;
		}
		printf("Scanline %04ld, duration %04ld: %s\n",p->scanline,p[1].scanline-p->scanline,s);
	}
	FreeWorld();
	CloseLibrary((struct Library*)DOSBase);
	CloseLibrary((struct Library*)GfxBase);
}
