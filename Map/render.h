#ifndef RENDER_H
#define RENDER_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>

#define NO_PLAYER

#define FADE_TO_BLACK

#define TRAPEZOID_ENABLE_ROUNDING

#define ZAVG_SQUARE

#define TSL_Z_THR (0x0480)

//#define TILE32

//#define CMODE256

typedef uint32_t UFIXP32;
typedef int32_t FIXP32;
typedef uint16_t UFIXP16;
typedef int16_t FIXP16;

typedef int64_t FIXP64;
typedef uint64_t UFIXP64;

#define MAX_COLORS 64

#define SCOORD_NOT_VALID (1)
//#define PERSP_COMPENSATE (2)
#define FACE_DIVIDED (2)

#define TSL_FLAG_NEAR (4)
#define TSL_FLAG_DIVIDE (8)

typedef struct
{
    uint32_t seq;
    FIXP32 X; //World X, 24.8
    FIXP32 Y; //World Y, 24.8
    FIXP32 Z; //World Z, 24.8
    FIXP16 x; //Rotated x, 8.8
    FIXP16 y; //Rotated y, 8.8
    FIXP16 z; //Rotated z. 8.8
    UFIXP16 sx; //Projected and displaced x 8.8
    UFIXP16 sy; //Projected and displaced y 8.8
    uint16_t flags;
}FPXYZ;

typedef struct
{
    uint32_t seq;
    FIXP32 X; //World X, 24.8
    FIXP32 Y; //World Y, 24.8
    FIXP32 Z; //World Z, 24.8
    UFIXP16 sx; //Projected and displaced x 8.8
    UFIXP16 sy; //Projected and displaced y 8.8
    FIXP16 x; //Rotated x, 8.8
    FIXP16 y; //Rotated y, 8.8
    FIXP16 z; //Rotated z. 8.8
    uint16_t flags;
}FPXYZ_TARGET;

typedef struct
{
    FPXYZ* p;
    UFIXP16 U;
    UFIXP16 V;
}TVERTEX;

#pragma warning( push )
//#pragma warning( disable : 4200 )
struct TFACE;

typedef struct TFACE
{
    struct TFACE* next;
    uint16_t flags;
    //Коэффициенты плоскости, но в формате 2.14 (как матрица поворота, для умножения через таблицу)
    FIXP16 A14;
    FIXP16 B14;
    FIXP16 C14;
    FIXP32 MX; //Центр грани
    FIXP32 MY;
    FIXP32 MZ;
    FIXP16 avg_z;
    uint16_t avg_color;
    uint8_t* T; //Указатель на текстуру
    FIXP32 A; //Пока пусть полежат, а вообще их походу можно выбросить
    FIXP32 B;
    FIXP32 C;
    FIXP32 D;
    uint32_t id; //Тоже пусть полежит
    uint32_t seq; //Пока пусть лежит, потом может нужно будет выбросить
    TVERTEX vertexes[1]; //Тут сложены все вертексы
}TFACE;

typedef struct
{
    uint32_t p;
    UFIXP16 V;
    UFIXP16 U;
}TVERTEX_TARGET;

typedef struct TFACE_TARGET
{
    uint32_t next;
    uint32_t seq;
    uint16_t flags;
    //Коэффициенты плоскости, но в формате 2.14 (как матрица поворота, для умножения через таблицу)
    FIXP16 A14;
    FIXP16 B14;
    FIXP16 C14;
    FIXP32 MX; //Центр грани
    FIXP32 MY;
    FIXP32 MZ;
    FIXP16 avg_z;
    uint16_t avg_color;
    uint32_t T; //Указатель на текстуру
    TVERTEX_TARGET vertexes[64]; //Тут сложены все вертексы
}TFACE_TARGET;

#pragma warning( pop )

typedef struct RVERTEX
{
    struct RVERTEX* next;
    struct RVERTEX* prev;
    UFIXP16 sx;
    UFIXP16 sy;
    UFIXP16 U;
    UFIXP16 V;
    FIXP16 x;
    FIXP16 y;
    FIXP16 z;
    uint16_t flags;
}RVERTEX;

typedef struct RFACE
{
    RVERTEX* vertex;
    uint32_t T;
    FIXP32 xmax;
    struct RFACE* next;
}RFACE;

extern FIXP32 IMATRIX[256 * 3 * 3];


#if 1
#define YDISP_FP ((64<<8))
#define XDISP_FP ((90<<8))

#define SCREEN_LEFT (10)
#define SCREEN_RIGHT (169)
#define SCREEN_BOTTOM (14)
#define SCREEN_TOP (113)
#else
#define YDISP_FP ((50<<8))
#define XDISP_FP ((80<<8))

#define SCREEN_LEFT (0)
#define SCREEN_RIGHT (159)
#define SCREEN_BOTTOM (0)
#define SCREEN_TOP (99)
#endif

#define SCR_L8 (SCREEN_LEFT<<8)
#define SCR_R8 (((SCREEN_RIGHT+1)<<8)-1)
#define SCR_B8 (SCREEN_BOTTOM<<8)
#define SCR_T8 (((SCREEN_TOP+1)<<8)-1)

#define MIN_Z8 ((1<<8)/10)

#define MAX_DISTANCE (0x3DFFUL)

#ifdef NO_PLAYER
#define MIN_D_WALK (256/2)
#else
#define MIN_D_WALK (256/4)
#endif

#define CamDisp_X (0)
#define CamDisp_Y (-256)
#define CamDisp_Z (-256*3/2)



//3D part
void RotateMTX(FIXP32 *m, FIXP32 YAW, FIXP32 PITCH, FIXP32 ROLL);
void NormalizeMTX(FIXP32* m);
extern FIXP32 RMTX[9];
extern FIXP32 RMTXp[9];
//extern float RotMatrix[9];
//extern float RotMatrixP[9];
//void UpdateRMTX(void);
//void UpdateRMTXp(void);
void UpdateIMTX(void);

//============= rendinit.cpp ======================
void RenderAllocMemory(void);
int ComputeABCD(TFACE* face);
extern uint8_t Texture[65536*4];

void LoadSpyro(void);
void LoadTR(void);
void LoadOmni(void);

FIXP32 fast_sqrt64(FIXP64 v);

TFACE* GetAllFacesFromPool(void);

TFACE* GetFaceByOffset(uint32_t ofs);

extern FIXP32 Scene_max_X;
extern FIXP32 Scene_min_X;
extern FIXP32 Scene_max_Y;
extern FIXP32 Scene_min_Y;
extern FIXP32 Scene_max_Z;
extern FIXP32 Scene_min_Z;

#define SKY_COLOR2 (22UL)
//#define SKY_COLOR (0x2BUL)
#define SKY_COLOR (0)

extern TFACE* PlayerSprite;
void CreatePlayerSprite(void);

//============= render2d.cpp =======================

//2D part
void InitRender2D(void);
void C2P(uint32_t* d);
void DrawLine(FIXP32 x1, FIXP32 y1, FIXP32 x2, FIXP32 y2, uint16_t color);
extern uint8_t ChunkScreenR[128 * 180];

//============== render3d.cpp ======================

extern int display_wires;

void InitRender3D(void);

void FinishRenderSlices(void);
void InitRenderSlices(void);

typedef union
{
    RVERTEX v;
    RFACE f;
}R_DATA;

#define R_DATA_TOTAL_SZ (512)

extern R_DATA* RDataPool[R_DATA_TOTAL_SZ];

#define R_DATA_THR_error (RDataPool+R_DATA_TOTAL_SZ)
//#define R_DATA_THR_force_draw (RDataPool+64)
#define R_DATA_THR_faces (RDataPool+192)
//#define R_DATA_THR_clip (R_DATA_THR_faces+128)
#define R_DATA_THR_max_faces (280)

R_DATA** tmap_prepare(RFACE* f, R_DATA** pool);
R_DATA** fmap_prepare(RFACE* f, R_DATA **pool);

//============== render.cpp ========================

extern FILE* f_render_log;

#define RDBG(...) do{if (f_render_log) fprintf(f_render_log,__VA_ARGS__);}while(0)

extern uint32_t FrameCounter;

void InitFrustumClipper(void);
void ProcessWorld(void);

void DrawFaceLines(RVERTEX* v, uint8_t color);

extern uint16_t _LOGtab[32768];
#define LOGtab (_LOGtab+16384)
extern uint16_t EXPtab[65536];
#define LOG5_8 (0xF528-2)

#define MIN_SQR (-0x8000)
#define MAX_SQR (+0x8000)
extern UFIXP16 _SQRtab[MAX_SQR - MIN_SQR];
#define SQRtab (_SQRtab-MIN_SQR)

extern FIXP32 CamX;
extern FIXP32 CamY;
extern FIXP32 CamZ;

extern FIXP32 PlayerX;
extern FIXP32 PlayerY;
extern FIXP32 PlayerZ;

#define Player_G (3)
#define Player_J (-40)

extern FIXP32 PSpeedY;

#define MUL(A,B) (SQRtab[(A)+(B)]-SQRtab[(A)-(B)])

extern uint8_t DumpRequest;

void DrawFaceEdges(RVERTEX* _v, uint8_t color);

//================================= rendermove.cpp =======================================

//Печать отладочной информации
void DebugCEdistances(char *str);
//Перемещаем камеру
void MovePlayer(FIXP32 dX, FIXP32 dY, FIXP32 dZ);

//=========================================================================================
extern int test_mode;
extern int test_mode2;

extern int traveled_cubes;
extern int prepared_cubes;
extern int prepared_points;
extern int prepared_vertexes;
extern int prepared_faces_stage1;
extern int prepared_faces_stage2;
extern int prepared_faces_stage3;
extern int processed_faces;
extern int send_to_render_faces;
extern int dropped_in_render_faces;

//=============================== kd-tree ======================================
typedef struct
{
    uint16_t next_index;
    uint8_t axis;
    int8_t split;
}KDTREE_NODE;

typedef struct
{
    union
    {
        uint32_t face_id;
        uint32_t face_offset;
    };
}KDTREE_FACE_NODE;

typedef struct
{
    int8_t K;
    int8_t J;
    int8_t I;
    uint8_t dummy;
    uint32_t kdtf_offset;
}KDTREE_LEAF;

typedef struct
{
    FIXP32* I;
    FIXP32* J;
    FIXP32* K;
    KDTREE_FACE_NODE* FaceList;
}KDTREE_BIG_LEAF;

extern FIXP32 BMATRIX[256 * 3 * 3];
extern KDTREE_BIG_LEAF* kd_tree_big_leafs;

extern KDTREE_NODE* kd_tree;
extern KDTREE_FACE_NODE* kd_tree_faces;
extern KDTREE_LEAF *kd_tree_leafs;

extern uint32_t* pvs_index;
extern uint8_t* packed_pvs;

extern uint32_t* collision_index;
extern uint32_t* collision_list;

void LoadKDtree(void);

KDTREE_NODE* FindKDtreeNode(FIXP32 X, FIXP32 Y, FIXP32 Z);

#endif