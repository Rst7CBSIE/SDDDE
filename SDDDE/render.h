#ifndef RENDER_H
#define RENDER_H

#define ALIGN4 __attribute__ ((aligned (16)))

typedef ULONG UFIXP32;
typedef LONG FIXP32;
typedef USHORT UFIXP16;
typedef SHORT FIXP16;
typedef unsigned long long UFIXP64;
typedef signed long long FIXP64;

typedef struct
{
	FIXP32 X;
	FIXP32 Y;
	FIXP32 Z;
	FIXP16 RMTX[9];
	FIXP16 dummy;
}CAMERA ALIGN4;

typedef struct
{
    SHORT next_index;
    UBYTE axis;
    BYTE split;
}KDTREE_NODE;

typedef struct
{
    ULONG seq;
    FIXP32 X; //World X, 24.8
    FIXP32 Y; //World Y, 24.8
    FIXP32 Z; //World Z, 24.8
    UFIXP16 sx; //Projected and displaced x 8.8
    UFIXP16 sy; //Projected and displaced y 8.8
    FIXP16 x; //Rotated x, 8.8
    FIXP16 y; //Rotated y, 8.8
    FIXP16 z; //Rotated z. 8.8
    USHORT flags;
}FPXYZ;

typedef struct
{
    FPXYZ* p;
    UFIXP16 U;
    UFIXP16 V;
}TVERTEX;

typedef struct TFACE
{
    struct TFACE* next;
    ULONG seq;
    USHORT flags;
    //Коэффициенты плоскости, но в формате 2.14 (как матрица поворота, для умножения через таблицу)
    FIXP16 A14;
    FIXP16 B14;
    FIXP16 C14;
    FIXP32 MX; //Центр грани
    FIXP32 MY;
    FIXP32 MZ;
    UFIXP16 avg_z;
    USHORT avg_color;
    ULONG T; //Указатель на текстуру
    TVERTEX vertexes[]; //Тут сложены все вертексы
}TFACE;

typedef struct
{
    union
    {
        ULONG face_id;
        TFACE *face;
    };
}KDTREE_FACE_NODE;

typedef struct
{
    BYTE K;
    BYTE J;
    BYTE I;
    BYTE dummy;
    ULONG kdtf_offset;
}KDTREE_LEAF_DISK;

typedef struct 
{
    FIXP32* I;
    FIXP32* J;
    FIXP32* K;
    KDTREE_FACE_NODE* FaceList;
}KDTREE_LEAF;


typedef struct
{
    union
    {
        ULONG face_id;
        TFACE *face;
    };
}COLLISION_ITEM;

typedef struct
{
    union
    {
        ULONG offset;
        COLLISION_ITEM *list;
    };
}COLLISION_LEAF;


typedef struct RVERTEX
{
	struct RVERTEX *next;
	struct RVERTEX *prev;
	UFIXP16 sx;
	UFIXP16 sy;
	UFIXP16 U;
	UFIXP16 V;
	FIXP16 x;
	FIXP16 y;
	FIXP16 z;
	USHORT flags;
}RVERTEX;

typedef struct RFACE
{
	RVERTEX *v;
	ULONG T;
	ULONG xmax;	//Computed by tmap, or used as flags
	struct RFACE *next;
}RFACE;

typedef union
{
    RVERTEX v;
    RFACE f;
}R_DATA;

#define R_DATA_TOTAL_SZ (1024)
extern R_DATA* RDataPool[R_DATA_TOTAL_SZ];
extern R_DATA** RDataPool_top;

extern ULONG *TRAPEZOIDS_POINTER;
extern ULONG *SLICES_POINTER;
extern ULONG SLICES_BUFFER[];
extern ULONG TRAPEZOIDS[];

extern USHORT _LOGtab[32768];
extern USHORT EXPtab[65536];
#define LOGtab (_LOGtab+16384)
#define LOG5_8 (0xF528-2)

#define MIN_SQR (-0x8000)
#define MAX_SQR (+0x8000)
extern UFIXP16 _SQRtab[MAX_SQR - MIN_SQR];
#define SQRtab (_SQRtab-MIN_SQR)
#define MUL(A,B) (SQRtab[(A)+(B)]-SQRtab[(A)-(B)])

#define MIN_D_WALK (256/2)

typedef struct
{
    USHORT stage;
    USHORT scanline;
}PROFILER_DATA;

typedef struct
{
    UBYTE B;
    UBYTE G;
    UBYTE R;
    UBYTE dummy;
}RGBPAL;

#endif