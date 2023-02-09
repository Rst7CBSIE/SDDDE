// SplitMap.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <Windows.h>
#include "render.h"
#include <stdlib.h>

#include "clipper.hpp"

using namespace std;
using namespace ClipperLib;


static char dirname[256];
static char* fname_pos;

void SetWorkDir(const char* dir)
{
    strcpy(dirname, dir);
    fname_pos = dirname + strlen(dirname);
}

FILE* xfopen(const char *name, const char *mode)
{
    strcpy(fname_pos, name);
    return fopen(dirname,mode);
}

//#define DBG(...) do{if (flog) {fprintf(flog,__VA_ARGS__); fflush(flog);}}while(0)
#define DBG(...) printf(__VA_ARGS__)
FPXYZ* points_bank;
FPXYZ* points_free_q = NULL;
static uint32_t points_global_id;
FACE* faces_bank = NULL;
FACE* faces_free_q = NULL;
VERTEX* vertexes_bank;
VERTEX* vertexes_free_q = NULL;
static uint32_t face_global_id;

uint32_t top_p_search;


#define POINTS_FREE_Q_SZ (0x20000)
void FreePoint(FPXYZ* p)
{
    p->flags = 0;
    p->next = points_free_q;
    points_free_q = p;
}

void InitPointsFreeQ(void)
{
    points_bank = (FPXYZ*)malloc(POINTS_FREE_Q_SZ * sizeof(FPXYZ));
    for (int i = POINTS_FREE_Q_SZ - 1; i >= 0; i--) FreePoint(points_bank + i);
}

#define FACES_FREE_Q_SZ (0x80000)

FACES_BY_POINT* faces_by_point = NULL;
FACES_BY_POINT* fbp_free_q = NULL;

FACES_BY_POINT* AllocFBP(void)
{
    FACES_BY_POINT* fbp = fbp_free_q;
    if (!fbp)
    {
        return fbp;
    }
    fbp_free_q = fbp->next;
    fbp->next = NULL;
    return fbp;
}

void FreeFBP(FACES_BY_POINT* fbp)
{
    fbp->face = NULL;
    fbp->v = NULL;
    fbp->next = fbp_free_q;
    fbp_free_q = fbp;
}

void InitFBPQ(void)
{
    faces_by_point = (FACES_BY_POINT*)malloc(FACES_FREE_Q_SZ * sizeof(FACES_BY_POINT));
    for (int i = 0; i < FACES_FREE_Q_SZ; i++) FreeFBP(faces_by_point + i);
}

void FreeFace(FACE* face)
{
    face->next2 = NULL;
    face->v = NULL;
    face->portal_to_cube = NULL;
    face->next = faces_free_q;
    faces_free_q = face;
}

void InitFacesFreeQ(void)
{
    faces_bank = (FACE*)malloc(FACES_FREE_Q_SZ * sizeof(FACE));
    for (int i = FACES_FREE_Q_SZ - 1; i >= 0; i--) FreeFace(faces_bank + i);
}

#define VERTEXES_FREE_Q_SZ (0x80000)

void FreeVertex(VERTEX* v)
{
    v->next = vertexes_free_q;
    vertexes_free_q = v;
}

void InitVertexesFreeQ(void)
{
    vertexes_bank = (VERTEX*)malloc(VERTEXES_FREE_Q_SZ * sizeof(VERTEX));
    for (int i = 0; i < VERTEXES_FREE_Q_SZ; i++) FreeVertex(vertexes_bank + i);
}

#define MIN_POINT_DIFF (0.125)

FPXYZ* AllocatedPoints;

static FPXYZ* FindOrAddPoint(double x, double y, double z)
{
    FPXYZ* p;
    for (p = AllocatedPoints; p; p = p->next)
    {
        //if (!(p->flags & XYZF_ALLOCATED)) continue;
        double s;
        double dx, dy, dz;
        dx = p->X - x;
        dy = p->Y - y;
        dz = p->Z - z;
        s = dx * dx + dy * dy + dz * dz;
        if (s < (MIN_POINT_DIFF * MIN_POINT_DIFF))
        {
#if 0
            DBG("Point %f,%f,%f joined with %f,%f,%f!\n",
                x, y, z,
                p->X, p->Y, p->Z);
#endif
            return p;
        }
    }
    p = points_free_q;
    if (!p)
    {
        return p;
    }
    points_free_q = p->next;
    p->flags = XYZF_ALLOCATED;
    p->next = AllocatedPoints;
    AllocatedPoints = p;
    p->id = ++points_global_id;
    p->X = x;
    p->Y = y;
    p->Z = z;
    p->FbyP = NULL;
    return p;
}

static FPXYZ* FindOrAddPoint2(double x, double y, double z)
{
    FPXYZ* p;
    for (p = AllocatedPoints; p; p = p->next)
    {
        if (!(p->flags & XYZF_ALLOCATED)) continue;
        if (p->id <= top_p_search) continue;
        double s;
        double dx, dy, dz;
        dx = p->X - x;
        dy = p->Y - y;
        dz = p->Z - z;
        s = dx * dx + dy * dy + dz * dz;
        if (s < (MIN_POINT_DIFF * MIN_POINT_DIFF))
        {
            return p;
        }
    }
    p = points_free_q;
    if (!p)
    {
        return p;
    }
    points_free_q = p->next;
    p->flags = XYZF_ALLOCATED;
    p->next = AllocatedPoints;
    AllocatedPoints = p;
    p->id = ++points_global_id;
    p->X = x;
    p->Y = y;
    p->Z = z;
    p->FbyP = NULL;
    return p;
}

#if 0
static void AddToFBP(VERTEX* v)
{
    FACES_BY_POINT* fbp = v->point->FbyP;
    if (fbp)
    {
        //Check present
        do
        {
            if (fbp->v == v) return;
            fbp = fbp->next;
        } while (fbp);
    }
    //Not found
    fbp = AllocFBP();
    fbp->v = v;
    fbp->face = v->face;
    fbp->next = v->point->FbyP;
    v->point->FbyP = fbp;
}

static void RemoveFromFBP(FPXYZ* p, FACE* f)
{
    FACES_BY_POINT* fbp = p->FbyP;
    FACES_BY_POINT* pfbp = NULL;
    if (fbp)
    {
        //Check present
        do
        {
            if (fbp->face == f)
            {
                if (pfbp)
                {
                    pfbp->next = fbp->next;
                }
                else
                {
                    //Remove from top
                    p->FbyP = fbp->next;
                }
                FreeFBP(fbp);
                return;
            }
            pfbp = fbp;
            fbp = fbp->next;
        } while (fbp);
    }
    //Not found
}
#endif

FACE* AllocFace(void)
{
    FACE* face = faces_free_q;
    if (!face)
    {
        return face;
    }
    faces_free_q = face->next;
    //Init
    face->id = ++face_global_id;
    face->next = NULL;
    face->next2 = NULL;
    face->v = NULL;
    face->portal_to_cube = NULL;
    face->flags = 0;
    //face->parent_cube = NULL;
    //face->paired_portal = NULL;
    face->parent_face = NULL;
    face->leaf_cnt = 0;
    return face;
}

VERTEX* AllocVertex()
{
    VERTEX* v = vertexes_free_q;
    if (!v)
    {
        return v;
    }
    vertexes_free_q = v->next;
    //Init
    v->next = NULL;
    v->prev = v;
    //v->point = NULL;
    v->point_id = 0;
    //
    v->face = NULL;
    v->flags = 0;
    return v;
}

VERTEX* CreateVertex(double X, double Y, double Z, double U, double V)
{
    VERTEX* v = AllocVertex();
    v->U = U;
    v->V = V;
    //v->point = FindOrAddPoint(X, Y, Z);
    v->X = X;
    v->Y = Y;
    v->Z = Z;
    return v;
}

//Посчитаем коэффициенты ABCD
//Это и нормаль, и уравнение плоскости для упирания в стенки при движении
//Вообще по хорошим делам надо бы выбрать самые хорошие вектора, с наибольшей точностью, но пока пусть будет так
int ComputeABCD(FACE* face)
{
    double MX, MY, MZ;
    int MN;
    MX = 0; MY = 0; MZ = 0; MN = 0;
    for (VERTEX* v = face->v; v; v = v->next)
    {
        MX += v->X;
        MY += v->Y;
        MZ += v->Z;
        MN++;
    }
    face->MX = MX / MN;
    face->MY = MY / MN;
    face->MZ = MZ / MN;
    face->R = 0;
    for (VERTEX* v = face->v; v; v = v->next)
    {
        MX = v->X;
        MY = v->Y;
        MZ = v->Z;
        MX -= face->MX;
        MY -= face->MY;
        MZ -= face->MZ;
        double R = sqrt(MX * MX + MY * MY + MZ * MZ);
        if (R > face->R) face->R = R;
    }
    face->N = MN;
    if (MN < 3)
        return 0;
    MN -= 2;
    VERTEX* v;
    v = face->v;
    double x1, x2, y1, y2, z1, z2;
    double bx, by, bz;
    //На самом деле в реальной жизни мы заменим сдвиг на 8 простым взятием одного байта
    x1 = v->prev->X;
    y1 = v->prev->Y;
    z1 = v->prev->Z;
    bx = 0; by = 0; bz = 0;
    double s;
    double bl;
    do
    {
        x2 = v->X;
        y2 = v->Y;
        z2 = v->Z;
        bx += y1 * z2 - z1 * y2;
        by += z1 * x2 - x1 * z2;
        bz += x1 * y2 - y1 * x2;
        x1 = x2;
        y1 = y2;
        z1 = z2;
    } while ((v = v->next) != 0);
    s = bx * bx + by * by + bz * bz;
    bl = sqrt(s); //После корня получим 16.16
    if (bl < 0.1)
    {
        return 0; //Слишком маленькая грань
    }
    bx = bx / bl;
    by = by / bl;
    bz = bz / bl;
    //Считаем свободный коэффициент
    s = bx * face->MX; //16.16*24.8 => 48.24
    s += by * face->MY;
    s += bz * face->MZ;
    face->D = -s;
    face->A = bx;
    face->B = by;
    face->C = bz;
    return 1;
}

VERTEX* InsertNewVertexToFace(FACE* f, double U, double V, FPXYZ *p)
{
    VERTEX* v;
    v = AllocVertex();
    v->face = f;
    v->U = U;
    v->V = V;
    v->X = p->X;
    v->Y = p->Y;
    v->Z = p->Z;
    v->point_id = p->id;
    if (f->v)
    {
        f->v->prev->next = v;
        v->prev = f->v->prev;
        f->v->prev = v;
    }
    else
        f->v = v;
    //AddToFBP(v);
    return v;
}

VERTEX* InsertVertexToFace(FACE* f, VERTEX *v)
{
    v->face = f;
    v->next = NULL;
    if (f->v)
    {
        f->v->prev->next = v;
        v->prev = f->v->prev;
        f->v->prev = v;
    }
    else
        f->v = v;
    //AddToFBP(v);
    return v;
}

void DestructFace(FACE* f)
{
    VERTEX* v;
    while ((v = f->v) != NULL)
    {
        f->v = v->next;
        FreeVertex(v);
    }
    FreeFace(f);
}


void PrintFaceToLog(FACE* f)
{
    if (f->T)
    {
        DBG("FACE_%d", f->id);
    }
    else if (f->portal_to_cube)
    {
        DBG("PORTAL_%d", f->id);
    }
    else
        DBG("VFACE_%d", f->id);
    DBG("[%.6f %.6f %.6f ", -f->MX, -f->MZ, -f->MY);
    DBG("=> %.2f %.2f %.2f %.2f]", f->A, f->B, f->C, f->D);
    /*fprintf(flog, "(");
    for (VERTEX* v = f->v; v; v = v->next)
    {
        //fprintf(flog, "%d", v->point->id);
        //if (v->next) fprintf(flog, ",");
        fprintf(flog, "[%.3f %.3f %.3f]", -v->point->X / 80.0, -v->point->Z / 80.0, -v->point->Y / 80.0);
    }
    fprintf(flog, ")");*/
}

#define NEXTVERTEX(V) (((V)->next)?((V)->next):((V)->face->v))

static FPXYZ* p_by_id[10000];
double TVertex[65536][2];

#define ADDQ(QNAME,ITEM) do{if (QNAME##tail) {QNAME##tail->next=ITEM;}else{QNAME##head=ITEM;}QNAME##tail=ITEM;}while(0)
#define REMQ(QNAME) do{if (!(QNAME##head=QNAME##head->next)) QNAME##tail=NULL;}while(0)

#define ADDQ2(QNAME,ITEM) do{if (QNAME##tail) {QNAME##tail->next_in_draw_queue=ITEM;}else{QNAME##head=ITEM;}QNAME##tail=ITEM;}while(0)
#define REMQ2(QNAME) do{if (!(QNAME##head=QNAME##head->next_in_draw_queue)) QNAME##tail=NULL;}while(0)

unsigned int top_tvertex;

FACE *LoadObj(const char *filename)
{
    FILE* f = xfopen(filename, "rt");
    char str[1024];
    top_tvertex = 0;
    unsigned int top_vertex = 0;
    unsigned int id = 0;
    FACE* fhead = NULL;
    FACE* ftail = NULL;
    int load_flags = 0;
    int total_v = 0;
    int total_vp = 0;
    int total_f = 0;
    int total_fv = 0;
    int total_portals = 0;
    int base_v;
    int base_vt;
    base_v = 0;
    base_vt = 0;
    for (;;)
    {
        if (feof(f)) break;
        char* s = fgets(str, 1024, f);
        if (!s) break;
        s = strtok(s, "\n\r ");
        if (!s) continue;
        if (!strcmp(s, "o"))
        {
            load_flags &= ~1;
            s = strtok(NULL, "\n\r ");
            if (!s) continue;
            if (!strcmp(s, "world"))
            {
                load_flags |= 1;
            }
            continue;
        }
        if (!strcmp(s, "usemtl"))
        {
            load_flags &= ~(2 | 4);
            s = strtok(NULL, "\n\r ");
            if (!s) continue;
            if (!strcmp(s, "Textured"))
            {
                //DBG("====================== Load main geometry =======================\n");
                load_flags |= 2;
            }
            if (!strcmp(s, "Portal"))
            {
                DBG("========================= Load portals ==========================\n");
                load_flags |= 4;
            }
            if (load_flags == 3)
            {
                DBG("====================== Load main geometry =======================\n");
            }
            continue;
        }
        if (!strcmp(s, "vt"))
        {
            if (load_flags & 1)
            {
                s = strtok(NULL, "\n\r ");
                if (!s) continue;
                TVertex[top_tvertex][0] = strtod(s, NULL);
                s = strtok(NULL, "\n\r ");
                if (!s) continue;
                TVertex[top_tvertex][1] = strtod(s, NULL);
                top_tvertex++;
                total_vp++;
            }
            else
            {
                base_vt++;
            }
            continue;
        }
        if (!strcmp(s, "v"))
        {
            if (load_flags & 1)
            {
                double X, Y, Z;
                s = strtok(NULL, "\n\r ");
                if (!s) continue;
                X = -strtod(s, NULL);
                s = strtok(NULL, "\n\r ");
                if (!s) continue;
                Y = -strtod(s, NULL);
                s = strtok(NULL, "\n\r ");
                if (!s) continue;
                Z = strtod(s, NULL);
                FPXYZ* p = FindOrAddPoint(X, Y, Z);
                p_by_id[top_vertex] = p;
                top_vertex++;
                total_v++;
            }
            else
            {
                base_v++;
            }
            continue;
        }
        if (!strcmp(s, "f"))
        {
            if (!(load_flags & 1)) continue;
            if (!(load_flags & (2 | 4))) continue;
            unsigned int vid[64], vtid[64];
            unsigned int n = 0;
            double U, V;
            unsigned int vt;
            s++;
            s = strtok(NULL, "\n\r ");
            if (!s) continue;
            do
            {
                vid[n] = strtoul(s, &s, 10) - 1 - base_v;
                if (load_flags & 2)
                {
                    vtid[n] = vt = strtoul(s + 1, NULL, 10) - 1 - base_vt;
                    U = TVertex[vt][0] * 512;
                    V = TVertex[vt][1] * 256;
                }
                n++;
                s = strtok(NULL, "\n\r ");
            } while (s);
            if (n < 3)
            {
                DBG("Less than 3 nodes!\n");
            }
            total_fv += n;
            if (load_flags & 2)
            {
                FACE* face = AllocFace();
                for (unsigned int i = 0; i < n; i++)
                {
                    vt = vtid[i];
                    U = TVertex[vt][0] * 512;
                    V = TVertex[vt][1] * 256;
                    VERTEX* v;
                    FPXYZ* p;
                    p = p_by_id[vid[i]];
                    v = InsertNewVertexToFace(face, U, V, p);
                    v->TVn = vt;
                }
                //Нам надо заполнить поля common_edge
                ADDQ(f, face);
                ComputeABCD(face);
                //UpdateCommonEdges(face, 0);
                total_f++;
            }
            continue;
        }
    }
    fclose(f);
    DBG("============================================================\n");
    DBG("total_v=%d total_vp=%d total_f=%d total_fv=%d\n", total_v, total_vp, total_f, total_fv);
    DBG("points_global_id=%d\n", points_global_id);
    DBG("total_portals=%d\n", total_portals);
    DBG("============================================================\n");
    return fhead;
}

#define FACEQ_POOL_SZ (0x40000)

FACEQ* faceq_pool;
FACEQ* faceq_pool_top;

void FreeFACEQ(FACEQ* fq)
{
    fq->next = faceq_pool_top;
    faceq_pool_top = fq;
}

FACEQ* AllocFACEQ(void)
{
    FACEQ* fq = faceq_pool_top;
    if (!fq) 
        return fq;
    faceq_pool_top = fq->next;
    fq->next = NULL;
    fq->f = NULL;
    return fq;
}

FACEQ* DuplicateFACEQ(FACEQ* fq0)
{
    FACEQ* fq1 = AllocFACEQ();
    fq1->f = fq0->f;
    return fq1;
}

void InitFACEQ(void)
{
    faceq_pool_top = NULL;
    faceq_pool = (FACEQ*)malloc(FACEQ_POOL_SZ * sizeof(FACEQ));
    for (int i = 0; i < FACEQ_POOL_SZ; i++) FreeFACEQ(faceq_pool + i);
}

#define PVS_POOL_SZ (0x40000)

PVS* pvs_pool;
PVS* pvs_pool_top;

void FreePVS(PVS* pvs)
{
    pvs->next = pvs_pool_top;
    pvs_pool_top = pvs;
}

PVS* AllocPVS(void)
{
    PVS* pvs = pvs_pool_top;
    if (!pvs)
        return pvs;
    pvs_pool_top = pvs->next;
    pvs->next = NULL;
    pvs->f = NULL;
    pvs->c = NULL;
    return pvs;
}

void InitPVS(void)
{
    pvs_pool_top = NULL;
    pvs_pool = (PVS*)malloc(PVS_POOL_SZ * sizeof(PVS));
    for (int i = 0; i < PVS_POOL_SZ; i++) FreePVS(pvs_pool + i);
}

FILE *fout;

static FPXYZ* FindPointById(int id)
{
    FPXYZ* p;
    for (p = AllocatedPoints; p; p = p->next)
    {
        if (!(p->flags & XYZF_ALLOCATED)) continue;
        if (p->id == id) return p;
    }
    return NULL;
}

double minx=1e100;
double miny=1e100;
double minz=1e100;
double maxx=-1e100;
double maxy=-1e100;
double maxz=-1e100;

void ParseMinMax(void)
{
    FPXYZ* p;
    for (p = AllocatedPoints; p; p = p->next)
    {
        if (!(p->flags & XYZF_ALLOCATED)) continue;
        if (p->X < minx) minx = p->X;
        if (p->Y < miny) miny = p->Y;
        if (p->Z < minz) minz = p->Z;
        if (p->X > maxx) maxx = p->X;
        if (p->Y > maxy) maxy = p->Y;
        if (p->Z > maxz) maxz = p->Z;
    }
}

void Predump(void)
{
    fout = xfopen("xxx.obj", "wt");
    fprintf(fout, "mtllib SpyroE2.mtl\n");
    for (unsigned int i = top_p_search + 1; i <= points_global_id; i++)
    {
        FPXYZ* p;
        p = FindPointById(i);
        //fprintf(fout, "#%d\n", i);
        fprintf(fout,"v %.6f %.6f %.6f\n", -p->X, -p->Y, p->Z);
    }
    /*for (int i = 0; i < top_tvertex; i++)
    {
        fprintf(fout, "vt %.6f %.6f\n", TVertex[i][0], TVertex[i][1]);
    }*/
}

#define MIN_BOX_SIZE (6.4)

int cube_id;

FACE* flist;

int max_faces_in_cube;

typedef struct
{
    uint16_t next_index;
    uint8_t axis;
    int8_t split;
}KDTREE_NODE;

KDTREE_NODE kd_tree[65536 * 2]; //Потому что мы всегда парами дергаем

uint32_t kd_tree_top;

uint32_t kd_tree_leafs_IJK[65536];

FACEQ* kd_tree_leafs[65536];

uint32_t kd_tree_leafs_top;

KDTREE_NODE* AllocKDTnode(void)
{
    if (kd_tree_top >= 65536 * 2)
    {
        exit(40);
    }
    KDTREE_NODE* kd = kd_tree + kd_tree_top;
    kd_tree_top += 2;
    memset(kd, 0, sizeof(KDTREE_NODE) * 2);
    return kd;
}

uint32_t faces_in_leafs_idx[65536];

void SaveBinaryKDtree(void)
{
    FILE* f = xfopen("kdtree.bin", "wb");
    fwrite(kd_tree, 1, sizeof(KDTREE_NODE) * kd_tree_top, f);
    fclose(f);
    f = xfopen("faces_in_leafs.bin", "wb");
    FILE* f2 = xfopen("fil_idx.bin", "wb");
    for (uint32_t i = 0; i < kd_tree_leafs_top; i++)
    {
        uint32_t id;
        id = ftell(f);
        fwrite(kd_tree_leafs_IJK + i, 1, 4, f2);
        fwrite(&id, 1, 4, f2);
        for (FACEQ* fq = kd_tree_leafs[i]; fq; fq = fq->next)
        {
            FACE* face = fq->f;
            if (face->parent_face) face = face->parent_face;
            if (face->leaf_cnt == 15)
            {
                exit(300);
            }
            face->leaf_idx[face->leaf_cnt++] = i;
            id = face->id;
            fwrite(&id, 1, 4, f);
        }
    }
    fclose(f);
    fclose(f2);
}

VERTEX* ComputeSectionAndCreateVertex(double t1, double t2, VERTEX* vs1, VERTEX* vs2)
{
    t2 = (t1 - t2);
    if (t2)
    {
        t1 /= t2;
    }
    else
        t1 = 1.0; //Хотя это и не ожидаемое поведение, но еще возможны ошибки вычисления

    VERTEX* vd = CreateVertex(
        vs1->X + (vs2->X - vs1->X) * t1,
        vs1->Y + (vs2->Y - vs1->Y) * t1,
        vs1->Z + (vs2->Z - vs1->Z) * t1,
        0,
        0
    );
    return vd;
}

int empty_box_index;

int raw_face_lists_size;

VEC3 KDT_BoxA =
{
    -128 * MIN_BOX_SIZE,
    -128 * MIN_BOX_SIZE,
    -128 * MIN_BOX_SIZE
};

VEC3 KDT_BoxB =
{
    127 * MIN_BOX_SIZE,
    127 * MIN_BOX_SIZE,
    127 * MIN_BOX_SIZE
};

int KDT_IBoxA[3] = { -64,-64,-64 };
int KDT_IBoxB[3] = { 63,63,63 };

void ConstructKDtree_step(KDTREE_NODE *root, FACEQ *froot, int xa, int ya, int za, int xb, int yb, int zb)
{
    //printf("%d %d %d - %d %d %d => ", xa, ya, za, xb, yb, zb);
#if 0
    if (xa == -128 && ya == -11 && za == -128 && xb == 127 && yb == -10 && zb == -26)
    {
        printf("[debug]");
    }
#endif
    uint8_t best_axis;
    int best_split;
    best_axis = 0xFF;
    best_split = 0;
    if (!froot)
    {
        //Нет граней, проверяем размер ячейки
        int max_sz = INT32_MIN;
        int sz;
        sz = xb - xa;
        if (sz > max_sz)
        {
            max_sz = sz;
            best_axis = 0;
            best_split = (xa + xb) / 2;
        }
        sz =yb - ya;
        if (sz > max_sz)
        {
            max_sz = sz;
            best_axis = 1;
            best_split = (ya + yb) / 2;
        }
        sz = zb - za;
        if (sz > max_sz)
        {
            max_sz = sz;
            best_axis = 2;
            best_split = (za + zb) / 2;
        }
        if (max_sz > 8) goto L_split; //Размер большой, надо бы порезать
        //printf("no faces\n");
        root->next_index = 0;
        empty_box_index++;
        root->axis = 0xFE; //Пустой лист
        return;
    }
    if (((xa + 1) == xb) && ((ya + 1) == yb) && ((za + 1) == zb))
    {
        //Бокс минимального размера, просто добавляем в него список граней
        //printf("minimum size\n");
        //printf(".");
        kd_tree_leafs_IJK[kd_tree_leafs_top] =
            ((xa & 0xFF) << 16) | ((ya & 0xFF) << 8) | ((za & 0xFF) << 0);
        kd_tree_leafs[kd_tree_leafs_top] = froot;
        root->axis = 0xFF;
        root->next_index = kd_tree_leafs_top;
        kd_tree_leafs_top++;
        raw_face_lists_size += 2;
        for (; froot; froot = froot->next)
            raw_face_lists_size++;
        return;
    }
    //Бокс можно поделить
    uint32_t bins[256]; //Корзина
    int NA, NB;

    int64_t min_sah;

    min_sah = MAXINT64;


    //Ось X
    memset(bins, 0, sizeof(bins));
    NB = 0;
    for (FACEQ* fq2 = froot; fq2; fq2 = fq2->next)
    {
        FACE* f = fq2->f;
        int i;
        i = (int)floor(f->MX / MIN_BOX_SIZE);
        if (i < -128 || i>127)
        {
            exit(60);
        }
        if (i < xa)
        {
            bins[128 + xa]++;
        }
        else if (i >= xb)
        {
            bins[128 + xb - 1]++;
        }
        else
            bins[128 + i]++;
        NB++;
    }
    //Корзины построены, теперь ищем лучший вариант
    NA = bins[128 + xa]; NB -= NA;
    //NB++;
    //NA++;
    for (int split = xa + 1; split < xb; split++)
    {
        int64_t a1, a2;
        a1 = (split - xa) * (yb - ya) * (zb - za);
        a2 = (xb - split) * (yb - ya) * (zb - za);
        int64_t sah;
        sah = 10 + NA * a1 + NB * a2;
        //printf("X: %.1f %d \n", sah / 100000.0, split);
        if (sah < min_sah)
        {
            min_sah = sah;
            best_axis = 0;
            best_split = split;
        }
        NA += bins[128 + split];
        NB -= bins[128 + split];
    }
    //Ось Y
    memset(bins, 0, sizeof(bins));
    NB = 0;
    for (FACEQ* fq2 = froot; fq2; fq2 = fq2->next)
    {
        FACE* f = fq2->f;
        int i;
        i = (int)floor(f->MY / MIN_BOX_SIZE);
        if (i < -128 || i>127)
        {
            exit(60);
        }
        if (i < ya)
        {
            bins[128 + ya]++;
        }
        else if (i >= yb)
        {
            bins[128 + yb - 1]++;
        }
        else
            bins[128 + i]++;
        NB++;
    }
    //Корзины построены, теперь ищем лучший вариант
    NA = bins[128 + ya]; NB -= NA;
    //NB++;
    //NA++;
    for (int split = ya + 1; split < yb; split++)
    {
        int64_t a1, a2;
        a1 = (xb - xa) * (split - ya) * (zb - za);
        a2 = (xb - xa) * (yb - split) * (zb - za);
        int64_t sah;
        sah = 10 + NA * a1 + NB * a2;
        //printf("X: %.1f %d \n", sah / 100000.0, split);
        if (sah < min_sah)
        {
            min_sah = sah;
            best_axis = 1;
            best_split = split;
        }
        NA += bins[128 + split];
        NB -= bins[128 + split];
    }
    //Ось Z
    memset(bins, 0, sizeof(bins));
    NB = 0;
    for (FACEQ* fq2 = froot; fq2; fq2 = fq2->next)
    {
        FACE* f = fq2->f;
        int i;
        i = (int)floor(f->MZ / MIN_BOX_SIZE);
        if (i < -128 || i>127)
        {
            exit(60);
        }
        if (i < za)
        {
            bins[128 + za]++;
        }
        else if (i >= zb)
        {
            bins[128 + zb - 1]++;
        }
        else
            bins[128 + i]++;
        NB++;
    }
    //Корзины построены, теперь ищем лучший вариант
    NA = bins[128 + za]; NB -= NA;
    //NB++;
    //NA++;
    for (int split = za + 1; split < zb; split++)
    {
        int64_t a1, a2;
        a1 = (xb - xa) * (yb - ya) * (split - za);
        a2 = (xb - xa) * (yb - ya) * (zb - split);
        int64_t sah;
        sah = 10 + NA * a1 + NB * a2;
        //printf("X: %.1f %d \n", sah / 100000.0, split);
        if (sah < min_sah)
        {
            min_sah = sah;
            best_axis = 2;
            best_split = split;
        }
        NA += bins[128 + split];
        NB -= bins[128 + split];
    }
    //Лучшее разбиение выбрано
    //printf("% .1f % d % d\n", min_sah / 1.0, best_axis, best_split);
    //Теперь делим по выбранной оси на две части
L_split:
    FACEQ* fqa;
    FACEQ* fqb;
    fqa = NULL;
    fqb = NULL;
    size_t p;
    switch (best_axis)
    {
    case 0:
        p = offsetof(VERTEX, X);
        break;
    case 1:
        p = offsetof(VERTEX, Y);
        break;
    case 2:
        p = offsetof(VERTEX, Z);
        break;
    default:
        exit(70);
        break;
    }
    double split;
    split = MIN_BOX_SIZE * (double)best_split;
    FACEQ* fqnext;
    for (FACEQ *fq = froot; fq; fq = fqnext)
    {
        fqnext = fq->next;
        FACE* f = fq->f;
        int pos = 0;
        for (VERTEX* v = f->v; v; v = v->next)
        {
            double* C;
            C = (double*)((size_t)v + p);
            if (*C < split)
                pos |= 1;
            else
                pos |= 2;
        }
        switch (pos)
        {
        case 1:
            fq->next = fqa;
            fqa = fq;
            break;
        case 2:
            fq->next = fqb;
            fqb = fq;
            break;
        default:
            //Нужно разделить на две грани
            {
                FACE* face_a = AllocFace();
                FACE* face_b = AllocFace();
                for (VERTEX* v2 = f->v; v2; v2 = v2->next)
                {
                    VERTEX* v1 = v2->prev;
                    double* C;
                    double t1, t2;
                    C = (double*)((size_t)v1 + p);
                    t1 = *C - split;
                    C = (double*)((size_t)v2 + p);
                    t2 = *C - split;
                    if ((t1 < 0.0 && t2 >= 0.0) || (t1 >= 0.0 && t2 < 0.0))
                    {
                        //Нужно создать новый вертекс
                        VERTEX* nv;
                        nv = ComputeSectionAndCreateVertex(t1, t2, v1, v2);
                        InsertVertexToFace(face_a, nv);
                        nv = ComputeSectionAndCreateVertex(t1, t2, v1, v2);
                        InsertVertexToFace(face_b, nv);
                    }
                    if (t2 < 0.0)
                    {
                        VERTEX* nv = CreateVertex(v2->X, v2->Y, v2->Z, 0.0, 0.0);
                        InsertVertexToFace(face_a, nv);
                    }
                    else
                    {
                        VERTEX* nv = CreateVertex(v2->X, v2->Y, v2->Z, 0.0, 0.0);
                        InsertVertexToFace(face_b, nv);
                    }
                }
                //Все, мы разделились на две грани, запоминаем, кто папа
                if (f->parent_face)
                {
                    face_b->parent_face = face_a->parent_face = f->parent_face;
                    //У нас есть родитель, грань, которую мы только что разбили можно убить
                    DestructFace(f);
                }
                else
                    face_b->parent_face = face_a->parent_face = f;
                FreeFACEQ(fq);
                if (ComputeABCD(face_a))
                {
                    fq = AllocFACEQ();
                    fq->f = face_a;
                    fq->next = fqa;
                    fqa = fq;
                }
                else
                    DestructFace(face_a);
                if (ComputeABCD(face_b))
                {
                    fq = AllocFACEQ();
                    fq->f = face_b;
                    fq->next = fqb;
                    fqb = fq;
                }
                else
                    DestructFace(face_b);
            }
            break;
        }
    }
    //Все разделили, создаем новых потомков
    KDTREE_NODE* newnodes;
    newnodes = AllocKDTnode();
    root->axis = best_axis;
    root->split = best_split;
    root->next_index = (newnodes - kd_tree) / 2;
    switch (best_axis)
    {
    case 0:
        ConstructKDtree_step(newnodes + 0, fqa, xa, ya, za, best_split, yb, zb);
        ConstructKDtree_step(newnodes + 1, fqb, best_split, ya, za, xb, yb, zb);
        break;
    case 1:
        ConstructKDtree_step(newnodes + 0, fqa, xa, ya, za, xb, best_split, zb);
        ConstructKDtree_step(newnodes + 1, fqb, xa, best_split, za, xb, yb, zb);
        break;
    case 2:
        ConstructKDtree_step(newnodes + 0, fqa, xa, ya, za, xb, yb, best_split);
        ConstructKDtree_step(newnodes + 1, fqb, xa, ya, best_split, xb, yb, zb);
        break;
    }
}

void ConstructKDtree(void)
{
    //Складываем все грани в список
    printf("Construct kd_tree: ");
    kd_tree_top = 0;
    kd_tree_leafs_top = 0;
    empty_box_index = 0;
    FACEQ *fq = NULL;
    for (FACE* f = flist; f; f = f->next)
    {
        FACEQ* nf = AllocFACEQ();
        nf->f = f;
        nf->next = fq;
        fq = nf;
    }
    //Запускаем создание дерева
    KDT_BoxA[0] = KDT_IBoxA[0] * MIN_BOX_SIZE;
    KDT_BoxA[1] = KDT_IBoxA[1] * MIN_BOX_SIZE;
    KDT_BoxA[2] = KDT_IBoxA[2] * MIN_BOX_SIZE;
    KDT_BoxB[0] = KDT_IBoxB[0] * MIN_BOX_SIZE;
    KDT_BoxB[1] = KDT_IBoxB[1] * MIN_BOX_SIZE;
    KDT_BoxB[2] = KDT_IBoxB[2] * MIN_BOX_SIZE;

    ConstructKDtree_step(AllocKDTnode(), fq, 
        KDT_IBoxA[0], KDT_IBoxA[1], KDT_IBoxA[2],
        KDT_IBoxB[0], KDT_IBoxB[1], KDT_IBoxB[2]
    );
    printf("Ok!\n");
    printf("Total nodes=%d\n", kd_tree_top / 2);
    printf("Total leafs=%d\n", kd_tree_leafs_top);
}

#define SQR(x) ((x)*(x))

int dump_top_v;

void DumpXYZ(int x, int y, int z)
{
    fprintf(fout, "v %.6f %.6f %.6f\n", -MIN_BOX_SIZE*x, -MIN_BOX_SIZE*y, MIN_BOX_SIZE*z);
}

int min_deep = MAXINT32;
int max_deep = MININT32;

int avg_deep;
int avg_n;

uint8_t pvs_bitmap[65536];
uint8_t pvs_banned[65536];
uint8_t pvs_work_bitmap[65536];

//#define SINGLE_PVS

void DumpKDtree_step(KDTREE_NODE* root, int xa, int ya, int za, int xb, int yb, int zb, int level)
{
    int pvs;
    switch (root->axis)
    {
    case 0:
        //X
        DumpKDtree_step(kd_tree + 0 + 2 * root->next_index, xa, ya, za, root->split, yb, zb, level + 1);
        DumpKDtree_step(kd_tree + 1 + 2 * root->next_index, root->split, ya, za, xb, yb, zb, level + 1);
        break;
    case 1:
        //Y
        DumpKDtree_step(kd_tree + 0 + 2 * root->next_index, xa, ya, za, xb, root->split, zb, level + 1);
        DumpKDtree_step(kd_tree + 1 + 2 * root->next_index, xa, root->split, za, xb, yb, zb, level + 1);
        break;
    case 2:
        //Z
        DumpKDtree_step(kd_tree + 0 + 2 * root->next_index, xa, ya, za, xb, yb, root->split, level + 1);
        DumpKDtree_step(kd_tree + 1 + 2 * root->next_index, xa, ya, root->split, xb, yb, zb, level + 1);
        break;
    case 0xFE:
        //Empty
#if 1
        if (level > max_deep) max_deep = level;
        if (level < min_deep) min_deep = level;
        avg_deep += level;
        avg_n++;
        break;
#endif
    case 0xFF:
        //Leaf
        pvs = root->next_index;
#ifdef SINGLE_PVS
        if (!pvs_bitmap[pvs])    break;
#endif
        fprintf(fout, "# [%d]\n", root - kd_tree);
        DumpXYZ(xa, ya, za);
        DumpXYZ(xb, ya, za);
        DumpXYZ(xb, yb, za);
        DumpXYZ(xa, yb, za);
        DumpXYZ(xa, ya, zb);
        DumpXYZ(xb, ya, zb);
        DumpXYZ(xb, yb, zb);
        DumpXYZ(xa, yb, zb);
        //fprintf(fout, "o leaf%d\n", root->next_index);
        //fprintf(fout,"usemtl Portal\n");
        //fprintf(fout,"s off\n");
        fprintf(fout, "f %d %d %d %d\n", dump_top_v + 1, dump_top_v + 2, dump_top_v + 3, dump_top_v + 4);
        fprintf(fout, "f %d %d %d %d\n", dump_top_v + 2, dump_top_v + 1, dump_top_v + 5, dump_top_v + 6);
        fprintf(fout, "f %d %d %d %d\n", dump_top_v + 3, dump_top_v + 2, dump_top_v + 6, dump_top_v + 7);
        fprintf(fout, "f %d %d %d %d\n", dump_top_v + 4, dump_top_v + 3, dump_top_v + 7, dump_top_v + 8);
        fprintf(fout, "f %d %d %d %d\n", dump_top_v + 1, dump_top_v + 4, dump_top_v + 8, dump_top_v + 5);
        fprintf(fout, "f %d %d %d %d\n", dump_top_v + 8, dump_top_v + 7, dump_top_v + 6, dump_top_v + 5);
        dump_top_v += 8;
        if (level > max_deep) max_deep = level;
        if (level < min_deep) min_deep = level;
        avg_deep += level;
        avg_n++;
        break;
    }
}

void DumpKDtree(void)
{
    dump_top_v = 0;
    fout = xfopen("xxx.obj", "wt");
    fprintf(fout, "mtllib SpyroE2.mtl\n");
    fprintf(fout, "o kdtree\n");
    fprintf(fout,"usemtl Portal\n");
    fprintf(fout,"s off\n");
    DumpKDtree_step(kd_tree, 
        KDT_IBoxA[0], KDT_IBoxA[1], KDT_IBoxA[2],
        KDT_IBoxB[0], KDT_IBoxB[1], KDT_IBoxB[2],
        0);
    fclose(fout);
    printf("min_deep=%d max_deep=%d avg_deep=%d\n", min_deep, max_deep, avg_deep / avg_n);
}

int PackPVS(uint8_t* out, uint8_t* in, int len)
{
    uint8_t* base = out;
    int N = 0;
    int v;
    while (len>0)
    {
        N++;
        v = *in++;
        //printf("%d", v);
        len--;
        if (v == *in)
        {
            //Пока значение постоянно, увеличиваем счетчик
            continue;
        }
        //Значение будет новое, пробуем сохранить
        if (N > 7)
        {
            //Пока у нас больше 63х одинаковых бит, выльем все наружу
#if 1
            if (N > 62)
            {
                //Два байта размера
                if (v)
                {
                    *out++ = 0x7F;
                    //printf("[%02X]", 0x7F);
                }
                else
                {
                    *out++ = 0x7E;
                    //printf("[%02X]", 0x7E);
                }
                *out++ = N & 0xFF;
                *out++ = (N >> 8) & 0xFF;
            }
            else
#else
            while (N > 63)
            {
                if (v)
                {
                    *out++ = 0x7F;
                    //printf("[%02X]", 0x7F);
                }
                else
                {
                    *out++ = 0x7E;
                    //printf("[%02X]", 0x7E);
                }
                N -= 63;
            }
#endif
            {
                N <<= 1;
                if (v) N |= 1;
                *out++ = N;
                //printf("[%02X]", N);
            }
        }
        else
        {
            //Мало бит, войдет в септет
            int b = 0x80;
            int mask = 0x01;
            do
            {
                if (v) b |= mask;
                mask <<= 1;
            } while (--N);
            while (mask != 0x80)
            {
                //Заполняем остаток септета
                v = *in++;
                len--;
                //printf("%d", v);
                if (v) b |= mask;
                mask <<= 1;
            }
            *out++ = b;
            //printf("[%02X]", b);
        }
        N = 0;
    }
    *out++ = 0;
    return out - base;
}

uint32_t pvs_indexes[65536];
uint32_t pvs_top_index = 0;
uint32_t pvs_offset;

double RMTX[9];

#define FM_PI 3.14159265358979323846


void PrepareRotMatrix(double YAW, double PITCH, double ROLL)
{
    //Уже суммарный вариант, но оси не те
    double Sx, Sy, Sz, Cx, Cy, Cz;
    Sx = sin(FM_PI * 2.0f * PITCH);
    Cx = cos(FM_PI * 2.0f * PITCH);
    Sy = sin(FM_PI * 2.0f * YAW);
    Cy = cos(FM_PI * 2.0f * YAW);
    Sz = sin(FM_PI * 2.0f * ROLL);
    Cz = cos(FM_PI * 2.0f * ROLL);
    RMTX[0] = Cy * Cz;
    RMTX[1] = -Cy * Sz;
    RMTX[2] = Sy;
    RMTX[3] = Cz * Sx * Sy + Cx * Sz;
    RMTX[4] = Cx * Cz - Sx * Sy * Sz;
    RMTX[5] = -Cy * Sx;
    RMTX[6] = -Cx * Cz * Sy + Sx * Sz;
    RMTX[7] = Cz * Sx + Cx * Sy * Sz;
    RMTX[8] = Cx * Cy;
}

int SceneOut[256][256];

#define MUL(A,B) ((A)*(B))

#define SCOORD_NOT_VALID (1)
#define MIN_Z8 (0.32)

#define SQR(A) ((A)*(A))

void RenderFace(RVERTEX* v, int color)
{
    RVERTEX* tv;
    RVERTEX* bv;
    tv = v;
    bv = tv->next;
    double xmin;
    double xmax;
    xmin = tv->sx;
    xmax = xmin;
    do
    {
        double x;
        x = bv->sx;
        if (x > xmax) xmax = x;
        if (x < xmin)
        {
            xmin = x;
            tv = bv;
        }
    } while ((bv = bv->next) != NULL);
    //Теперь vt указывают на самую верхнюю грань
    v->prev->next = v;
    double ty;
    double by;
    double tdy;
    double bdy;
    int* D;
    int X;
    X = (int)floor(xmin);
    bv = tv;
    //Предзагрузка
    double tx, bx;
    tx = xmin;
    bx = xmin;
    double x;
    x = floor(xmin);
    xmax = floor(xmax);
    ty = tv->sy;
    by = bv->sy; //Загрузим на всякий случай
    tdy = 0;
    bdy = 0;
    do
    {
        if (X > 255) break;
        D = SceneOut[X++];
        x += 1.0;
        double w;
        if (tx < x)
        {
            do
            {
                w = tx; ty = tv->sy;
                if (tx >= xmax) break; //Никуда дальше не идем, если дошли до края
                tv = tv->prev;
                tx = tv->sx;
            } while (tx < x);
            w = floor(tx) - floor(w);
            tdy = tv->sy - ty;
            if (w)
            {
                tdy /= w;
            }
        }
        if (bx < x)
        {
            do
            {
                w = bx; by = bv->sy;
                if (bx >= xmax) break;
                bv = bv->next;
                bx = bv->sx;
            } while (bx < x);
            w = floor(bx) - floor(w);
            bdy = bv->sy - by;
            if (w)
            {
                bdy /= w;
            }
        }
        int t, b;
        int h;
        t = (int)floor(ty);
        if (t < 0) t = 0; if (t > 255) t = 255;
        b = (int)floor(by);
        if (b < 0) b = 0; if (b > 255) b = 255;
        if (t > b)
        {
            h = t; t = b; b = h;
            //return;
            //color = 0xFFFF;
        }
        h = b - t;
        if (X > 0)
        {
            while (h >= 0)
            {
                if (t > 255)
                {
                    exit(1000);
                }
                D[t++] = color;
                h--;
            }
        }
        ty += tdy;
        by += bdy;
    } while (x <= xmax);
}

//Буфера для обрезания
#define MAX_VERTEXES (128)
static RVERTEX vertexes[MAX_VERTEXES];

#define ADD_VERTEX(ROOT,_V) \
do \
{ RVERTEX *V=(_V);\
    V->next=NULL; \
    if (ROOT) \
    { \
        (V->prev = ROOT->prev)->next = V; \
    } \
    else \
    { \
        ROOT = V; \
    } \
    ROOT->prev = V; \
}while(0)

void ComputeSection(double t1, double t2, RVERTEX* vd, RVERTEX* vs1, RVERTEX* vs2)
{
    t2 = (t1 - t2);
    if (t2)
    {
        t1 /= t2;
        if (t1 >= 1.0)
        {
            t1 = 1.0;
        }
        if (t1 < 0.0)
        {
            t1 = 0.0;
        }
    }
    else
        t1 = 1.0; //Хотя это и не ожидаемое поведение, но еще возможны ошибки вычисления
    vd->x = vs1->x + (vs2->x - vs1->x) * t1;
    vd->y = vs1->y + (vs2->y - vs1->y) * t1;
    vd->z = vs1->z + (vs2->z - vs1->z) * t1;
    vd->flags = SCOORD_NOT_VALID; //Установим флаг невалидности sx/sy
}

#define PROCESS_CLIP_PLANE() do{ \
    if (t1 >= 0 && t2 >= 0) \
    { \
        /*Обе точки внутри*/ \
        ADD_VERTEX(vd,vs2); \
    } \
    else if (t1 >= 0 && t2 < 0) \
    { \
        /*Вышли*/ \
        RVERTEX *newv; newv=vpool++; ComputeSection(t1, t2, newv, vs1, vs2); \
        ADD_VERTEX(vd,newv); \
    } \
    else if (t1 < 0 && t2 >= 0) \
    { \
        /*Зашли*/ \
        RVERTEX *newv; newv=vpool++; ComputeSection(t2, t1, newv, vs2, vs1); \
        ADD_VERTEX(vd,newv); \
        ADD_VERTEX(vd,vs2); \
    } \
    if (vpool>=vertexes+MAX_VERTEXES) \
    { \
        exit(-100); \
    } \
    vs1 = vs2; \
}while(0)

#define MIN_Z_CLIP (0.64)

//Синагога с обрезанием
//Все генерится в xvertexes1
RVERTEX* ClipFace(RVERTEX* vs2, RVERTEX* vpool)
{
    if (!vs2 || !vs2->next || !vs2->next->next) return NULL;
    RVERTEX* vs1;
    RVERTEX* vnext;
    RVERTEX* vd;
    double t1, t2;

    //Setup and run stage z
    vd = NULL; vs1 = vs2->prev;
    do
    {
        vnext = vs2->next;
        t1 = vs1->z - MIN_Z_CLIP;
        t2 = vs2->z - MIN_Z_CLIP;
        PROCESS_CLIP_PLANE();
    } while ((vs2 = vnext) != NULL);
    vs2 = vd;
    if (!vs2 || !vs2->next || !vs2->next->next) return NULL;
    //Setup and run stage z+x
    vd = NULL; vs1 = vs2->prev;
    do
    {
        vnext = vs2->next;
        t1 = vs1->z + vs1->x;
        t2 = vs2->z + vs2->x;
        PROCESS_CLIP_PLANE();
    } while ((vs2 = vnext) != NULL);
    vs2 = vd;
    if (!vs2 || !vs2->next || !vs2->next->next) return NULL;
    //
    //Setup and run stage z-x
    vd = NULL; vs1 = vs2->prev;
    do
    {
        vnext = vs2->next;
        t1 = vs1->z - vs1->x;
        t2 = vs2->z - vs2->x;
        PROCESS_CLIP_PLANE();
    } while ((vs2 = vnext) != NULL);
    vs2 = vd;
    if (!vs2 || !vs2->next || !vs2->next->next) return NULL;
    //
    //Setup and run stage z+y
    vd = NULL; vs1 = vs2->prev;
    do
    {
        vnext = vs2->next;
        t1 = vs1->z + vs1->y;
        t2 = vs2->z + vs2->y;
        PROCESS_CLIP_PLANE();
    } while ((vs2 = vnext) != NULL);
    vs2 = vd;
    if (!vs2 || !vs2->next || !vs2->next->next) return NULL;
    //
    //Setup and run stage z-y
    vd = NULL; vs1 = vs2->prev;
    do
    {
        vnext = vs2->next;
        t1 = vs1->z - vs1->y;
        t2 = vs2->z - vs2->y;
        PROCESS_CLIP_PLANE();
    } while ((vs2 = vnext) != NULL);
    vs2 = vd;
    if (!vs2 || !vs2->next || !vs2->next->next) return NULL;
    return vs2;
}

void RenderScene3(FACE* face, RVERTEX* vroot, RVERTEX* vpool)
{
    RVERTEX* v;
#if 1
    if (face->flags & SCOORD_NOT_VALID)
    {
        //Там настолько пиздец, что надо в синагогу, обрезать все
        vroot = ClipFace(vroot, vpool);
        if (!vroot) return;
        //Compute sx,sy,iz
        v = vroot;
        do
        {
            if (v->flags & SCOORD_NOT_VALID)
            {
                double x, y, z;
                z = v->z;
                x = v->x;
                y = v->y;
                //if (z < MIN_Z8) z = MIN_Z8;
                v->sx = floor(127.5 * x / z + 127.5);
                v->sy = floor(127.5 * y / z + 127.5);
            }
        } while ((v = v->next) != NULL);
    }
#else
    if (face->flags & SCOORD_NOT_VALID) return;
#endif
    double x1, x2, y1, y2;
    v = vroot;
    double area;
    area = 0;
    //На самом деле в реальной жизни мы заменим сдвиг на 8 простым взятием одного байта
    x1 = v->prev->sx;// >> 8;
    y1 = v->prev->sy;// >> 8;
    do
    {
        x2 = v->sx;// >> 8;
        y2 = v->sy;// >> 8;
        area -= x1 * y2;
        area += x2 * y1;
        x1 = x2;
        y1 = y2;
    } while ((v = v->next) != NULL);
    if (area > 0.0)
    //if (1)
    {
        if (face->parent_face)
        {
            RenderFace(vroot, face->parent_face - faces_bank);
        }
        else
        {
            RenderFace(vroot, face - faces_bank);
        }
    }
    else
    {
        RenderFace(vroot, -2);
    }
}

#define VERTEX2RVERTEX() \
    v = vpool++; \
    v->x = tv->x; \
    v->y = tv->y; \
    v->z = tv->z; \
    if (!(v->flags = tv->flags & 1)) \
    { \
        v->sx = tv->sx; \
        v->sy = tv->sy; \
    } \
    ADD_VERTEX(vroot, v)



void RenderScene2(FACE* face, VEC3 *pos)
{
    FACE* fnext;
    RVERTEX* vroot;
    RVERTEX* v;
    RVERTEX* vpool;
    VERTEX* tv;

    for (; face; face = fnext)
    {
        fnext = face->next2;
        switch (face->N)
        {
        case 3:
        default:
            //Преобразуем в список RVERTEX
            vpool = vertexes;
            vroot = NULL;
            for (tv = face->v; tv; tv = tv->next)
            {
                VERTEX2RVERTEX();
            }
            RenderScene3(face, vroot, vpool);
            break;
        case 4:
            //Triangle 1
            vpool = vertexes;
            vroot = NULL;
            tv = face->v; //0
            VERTEX2RVERTEX();
            tv = tv->next; //1
            VERTEX2RVERTEX();
            tv = tv->next->next; //3
            VERTEX2RVERTEX();
            RenderScene3(face, vroot, vpool);
            //Triangle 2
            vpool = vertexes;
            vroot = NULL;
            VERTEX2RVERTEX(); //3
            tv = face->v->next; //1
            VERTEX2RVERTEX();
            tv = tv->next; //2
            VERTEX2RVERTEX();
            RenderScene3(face, vroot, vpool);
        }
    }
}

void ImgPostProcess(void)
{
#if 0
    for (int y = 1; y < 255; y++)
    {
        for (int x = 0; x < 256; x++)
        {
            if (SceneOut[x][y] >= 0 &&
                SceneOut[x][y - 1] < 0 &&
                SceneOut[x][y + 1] < 0)
                SceneOut[x][y] = -1;
        }
    }
    for (int y = 1; y < 255; y++)
    {
        for (int x = 0; x < 256; x++)
        {
            if (SceneOut[y][x] >= 0 &&
                SceneOut[y - 1][x] < 0 &&
                SceneOut[y + 1][x] < 0)
                SceneOut[y][x] = -1;
        }
    }
#endif
}

static inline double ComputeDistanceFromCamToMidpoint(FACE* face, VEC3 *pos)
{
    return sqrt(SQR(face->MX - (*pos)[0]) + SQR(face->MY - (*pos)[1]) + SQR(face->MZ - (*pos)[2]));
}

FACE* triangled_list;

void TriangulateList(void)
{
    FACE* list = NULL;
    FACE* fnext;
    FACE* face;
    face = flist;
    //Проворачиваем все вершины
    for (; face; face = fnext)
    {
        fnext = face->next;
        if (face->N != 4)
        {
            face->next_tri = list;
            list = face;
        }
        else
        {
            FACE* face_a = AllocFace();
            FACE* face_b = AllocFace();
            face_a->parent_face = face;
            face_b->parent_face = face;
            VERTEX* v, * sv;
            sv = face->v;
            v = CreateVertex(sv->X, sv->Y, sv->Z, 0, 0);
            InsertVertexToFace(face_a, v);
            sv = sv->next;
            v = CreateVertex(sv->X, sv->Y, sv->Z, 0, 0);
            InsertVertexToFace(face_a, v);
            sv = sv->next->next;
            v = CreateVertex(sv->X, sv->Y, sv->Z, 0, 0);
            InsertVertexToFace(face_a, v);
            v = CreateVertex(sv->X, sv->Y, sv->Z, 0, 0);
            InsertVertexToFace(face_b, v);
            sv = face->v->next;
            v = CreateVertex(sv->X, sv->Y, sv->Z, 0, 0);
            InsertVertexToFace(face_b, v);
            sv = sv->next;
            v = CreateVertex(sv->X, sv->Y, sv->Z, 0, 0);
            InsertVertexToFace(face_b, v);
            if (!ComputeABCD(face_a))
            {
                exit(1000);
            }
            if (!ComputeABCD(face_b))
            {
                exit(1000);
            }
            face_a->next_tri = list;
            list = face_a;
            face_b->next_tri = list;
            list = face_b;
        }
    }
    triangled_list = list;
}

void RenderScene(VEC3* pos)
{
    memset(SceneOut, 0xFF, sizeof(SceneOut));
    FACE* list = NULL;
    FACE* fnext;
    FACE* face;
    face = triangled_list;// flist;
    //Проворачиваем все вершины
    for (; face; face = fnext)
    {
        fnext = face->next_tri;
        double X, Y, Z, x, y, z;
        face->flags = 0;
        //Грань пока подходит, пробуем обновить все вершины
        double max_z;
        max_z = 0;
        for (VERTEX* v = face->v; v; v=v->next)
        {
            int f;
            f = 0;
            //Расчет новых координат из-за поворотов и движения
            X = v->X - (*pos)[0];
            Y = v->Y - (*pos)[1];
            Z = v->Z - (*pos)[2];
            x = MUL(X, RMTX[0]) + MUL(Y, RMTX[3]) + MUL(Z, RMTX[6]);
            y = MUL(X, RMTX[1]) + MUL(Y, RMTX[4]) + MUL(Z, RMTX[7]);
            z = MUL(X, RMTX[2]) + MUL(Y, RMTX[5]) + MUL(Z, RMTX[8]);
            v->x = x;
            v->y = y;
            v->z = z;
            if (z > max_z) max_z = z;
            if (z < MIN_Z8)
            {
                z = 0; //Чтобы проверка по пирамиде выдала нужные результаты (-max,0,+max)
                f = SCOORD_NOT_VALID; //Точно ошибка экранных координат
            }
            else
            {
                if (x > z || -x > z)
                {
                    f = SCOORD_NOT_VALID;
                }
                v->sx = floor(127.5 * x / z + 127.5);
                if (y > z || -y > z)
                {
                    f = SCOORD_NOT_VALID;
                }
                v->sy = floor(127.5 * y / z + 127.5);
            }
            v->flags = f;
            face->flags |= f;
        }
        if (max_z > 0.0)
        {
            face->next2 = list;
            list = face;
        }
    }
    //Все вершины провернуты, сортируем
    FACE* q1[256];
    FACE* q2[256];
    FACE** qf;
    face = list;
    memset(q1, 0, sizeof(q1));
    memset(q2, 0, sizeof(q2));
    //Первый этап: расчет среднего и заполнение букета
    for (; face; face = fnext)
    {
        fnext = face->next2;
        double fz;
        fz = ComputeDistanceFromCamToMidpoint(face, pos);
        int32_t z;
        z = (int32_t)(fz * 80.0);
        //if (z > 0x2EFF)
        if (z > 0x1FFF)
        {
#if 0
            FACE* xf;
            xf = face; if (xf->parent_face) xf = xf->parent_face;
            for (int i = 0; i < xf->leaf_cnt; i++)
                pvs_banned[xf->leaf_idx[i]] = 1;
#endif
            continue;
        }
        face->avg_z = z;
        qf = q1 + (z & 0xFF);
        face->next2 = *qf;
        *qf = face;
    }
    for (int i = 0; i < 256; i++)
    {
        face = q1[i];
        if (face)
        {
            do
            {
                fnext = face->next2;
                qf = q2 + ((face->avg_z >> 8) & 0xFF);
                face->next2 = *qf;
                *qf = face;
            } while ((face = fnext) != NULL);
        }
    }
    //Все, теперь можно просто собрать всех и вся.
    for (int i = 255; i >= 0; i--)
    {
        RenderScene2(q2[i], pos);
    }
    ImgPostProcess();
}

void DumpBMP(void)
{
    static uint8_t hdr[] =
    {
        0x42, 0x4D, 0x36, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    FILE* f;
    f = xfopen("test.bmp", "wb");
    fwrite(hdr, 1, sizeof(hdr), f);
    for (int y = 255; y >= 0; y--)
    {
        for (int x = 0; x < 256; x++)
        {
            int v;
            v = SceneOut[x][y];
            int color;
            color = ((v & 0xF00) << (8+4)) | ((v & 0x0F0) << (8)) | ((v & 0x0F) << (4));
            fwrite(&color, 1, 3, f);
        }
    }
    fclose(f);
}

int SceneOutToPVS(void)
{
    //Чекаем статистику
    static int counts[65536];
    memset(counts, 0, sizeof(counts));
    for (int y = 0; y < 256; y++)
    {
        for (int x = 0; x < 256; x++)
        {
            int v = SceneOut[x][y];
            if (v >= 0)
            {
                FACE* face = faces_bank + v;
                for (int i = 0; i < face->leaf_cnt; i++)
                    counts[face->leaf_idx[i]]++;
            }
        }
    }
    int sky_n=0;
    int back_n=0;
    int front_n = 0;
    for (int y = 0; y < 256; y++)
    {
        for (int x = 0; x < 256; x++)
        {
            int v;
            v = SceneOut[x][y];
            if (v == -1)
            {
                sky_n++;
            }
            if (v == -2)
            {
                back_n++;
            }
            if (v >= 0)
            {
                front_n++;
            }
        }
    }
    if (back_n * 4 > front_n) return 1;
    for (int y = 0; y < 256; y++)
    {
        for (int x = 0; x < 256; x++)
        {
            int v;
            v = SceneOut[x][y];
            if (v >= 0)
            {
                FACE* face = faces_bank + v;
                for (int i = 0; i < face->leaf_cnt; i++)
                {
                    int j = face->leaf_idx[i];
                    if (counts[j] > 2)
                        pvs_work_bitmap[j] = 1;
                }
            }
        }
    }
    return 0;
}

void DoPVSpos(VEC3* pos)
{
    printf("Trace %.2f %.2f %.2f...", (*pos)[0], (*pos)[1], (*pos)[2]);
    //printf(".");
    for (FACE* f = flist; f; f = f->next)
    {
        f->side_hit = 0;
        f->seq = 0;
    }
    memset(pvs_work_bitmap, 0, sizeof(pvs_work_bitmap));
    PrepareRotMatrix(0.0, 0.0, 0.0);
    RenderScene(pos);
    //if (SceneOutToPVS()) goto L_skip;
    SceneOutToPVS();
    PrepareRotMatrix(0.25, 0.0, 0.0);
    RenderScene(pos);
    //if (SceneOutToPVS()) goto L_skip;
    SceneOutToPVS();
    PrepareRotMatrix(0.5, 0.0, 0.0);
    RenderScene(pos);
    //if (SceneOutToPVS()) goto L_skip;
    SceneOutToPVS();
    PrepareRotMatrix(0.75, 0.0, 0.0);
    RenderScene(pos);
    //if (SceneOutToPVS()) goto L_skip;
    SceneOutToPVS();
    PrepareRotMatrix(0.0, 0.25, 0.0);
    RenderScene(pos);
    //if (SceneOutToPVS()) goto L_skip;
    SceneOutToPVS();
    PrepareRotMatrix(0.0, -0.25, 0.0);
    RenderScene(pos);
    //if (SceneOutToPVS()) goto L_skip;
    SceneOutToPVS();    
    for (int i = 0; i < sizeof(pvs_bitmap); i++)
    {
        pvs_bitmap[i] |= pvs_work_bitmap[i];
    }
//L_skip:
    ;
}

void DoPVS(KDTREE_NODE* root, int xa, int ya, int za, int xb, int yb, int zb, int level)
{
    switch (root->axis)
    {
    case 0:
        //X
        DoPVS(kd_tree + 0 + 2 * root->next_index, xa, ya, za, root->split, yb, zb, level + 1);
        DoPVS(kd_tree + 1 + 2 * root->next_index, root->split, ya, za, xb, yb, zb, level + 1);
        break;
    case 1:
        //Y
        DoPVS(kd_tree + 0 + 2 * root->next_index, xa, ya, za, xb, root->split, zb, level + 1);
        DoPVS(kd_tree + 1 + 2 * root->next_index, xa, root->split, za, xb, yb, zb, level + 1);
        break;
    case 2:
        //Z
        DoPVS(kd_tree + 0 + 2 * root->next_index, xa, ya, za, xb, yb, root->split, level + 1);
        DoPVS(kd_tree + 1 + 2 * root->next_index, xa, ya, root->split, xb, yb, zb, level + 1);
        break;
    case 0xFE:
        //Empty
    case 0xFF:
        uint32_t index;
        index = root - kd_tree;
        printf("Leaf %d (size %d %d %d): ", index, xb - xa, yb - ya, zb - za);
        if (index > pvs_top_index) pvs_top_index = index;
        memset(pvs_bitmap, 0, sizeof(pvs_bitmap));
        //if (root->axis == 0xFF)
        //    pvs_bitmap[root->next_index] = 1; //Обязательно включаем в список текущий лист
        memset(pvs_banned, 0, sizeof(pvs_banned));
        VEC3 pos;
        //Центр
        pos[0] = ((double)xb + (double)xa) * (MIN_BOX_SIZE / 2.0);
        pos[1] = ((double)yb + (double)ya) * (MIN_BOX_SIZE / 2.0);
        pos[2] = ((double)zb + (double)za) * (MIN_BOX_SIZE / 2.0);
        DoPVSpos(&pos);

        //Грани по X
        pos[0] = ((double)xa) * MIN_BOX_SIZE;
        pos[1] = ((double)yb + (double)ya) * (MIN_BOX_SIZE / 2.0);
        pos[2] = ((double)zb + (double)za) * (MIN_BOX_SIZE / 2.0);
        DoPVSpos(&pos);
        pos[0] = ((double)xb) * MIN_BOX_SIZE;
        pos[1] = ((double)yb + (double)ya) * (MIN_BOX_SIZE / 2.0);
        pos[2] = ((double)zb + (double)za) * (MIN_BOX_SIZE / 2.0);
        DoPVSpos(&pos);

        //Грани по Y
        pos[0] = ((double)xb + (double)xa) * (MIN_BOX_SIZE / 2.0);
        pos[1] = ((double)ya) * MIN_BOX_SIZE;
        pos[2] = ((double)zb + (double)za) * (MIN_BOX_SIZE / 2.0);
        DoPVSpos(&pos);
        pos[0] = ((double)xb + (double)xa) * (MIN_BOX_SIZE / 2.0);
        pos[1] = ((double)yb) * MIN_BOX_SIZE;
        pos[2] = ((double)zb + (double)za) * (MIN_BOX_SIZE / 2.0);
        DoPVSpos(&pos);

        //Грани по Z
        pos[0] = ((double)xb + (double)xa) * (MIN_BOX_SIZE / 2.0);
        pos[1] = ((double)yb + (double)ya) * (MIN_BOX_SIZE / 2.0);
        pos[2] = ((double)za) * MIN_BOX_SIZE;
        DoPVSpos(&pos);
        pos[0] = ((double)xb + (double)xa) * (MIN_BOX_SIZE / 2.0);
        pos[1] = ((double)yb + (double)ya) * (MIN_BOX_SIZE / 2.0);
        pos[2] = ((double)zb) * MIN_BOX_SIZE;
        DoPVSpos(&pos);

        //Вершины
        pos[0] = ((double)xa) * MIN_BOX_SIZE;
        pos[1] = ((double)ya) * MIN_BOX_SIZE;
        pos[2] = ((double)za) * MIN_BOX_SIZE;
        DoPVSpos(&pos);
        pos[0] = ((double)xb) * MIN_BOX_SIZE;
        pos[1] = ((double)ya) * MIN_BOX_SIZE;
        pos[2] = ((double)za) * MIN_BOX_SIZE;
        DoPVSpos(&pos);
        pos[0] = ((double)xa) * MIN_BOX_SIZE;
        pos[1] = ((double)yb) * MIN_BOX_SIZE;
        pos[2] = ((double)za) * MIN_BOX_SIZE;
        DoPVSpos(&pos);
        pos[0] = ((double)xb) * MIN_BOX_SIZE;
        pos[1] = ((double)yb) * MIN_BOX_SIZE;
        pos[2] = ((double)za) * MIN_BOX_SIZE;
        DoPVSpos(&pos);
        pos[0] = ((double)xa) * MIN_BOX_SIZE;
        pos[1] = ((double)ya) * MIN_BOX_SIZE;
        pos[2] = ((double)zb) * MIN_BOX_SIZE;
        DoPVSpos(&pos);
        pos[0] = ((double)xb) * MIN_BOX_SIZE;
        pos[1] = ((double)ya) * MIN_BOX_SIZE;
        pos[2] = ((double)zb) * MIN_BOX_SIZE;
        DoPVSpos(&pos);
        pos[0] = ((double)xa) * MIN_BOX_SIZE;
        pos[1] = ((double)yb) * MIN_BOX_SIZE;
        pos[2] = ((double)zb) * MIN_BOX_SIZE;
        DoPVSpos(&pos);
        pos[0] = ((double)xb) * MIN_BOX_SIZE;
        pos[1] = ((double)yb) * MIN_BOX_SIZE;
        pos[2] = ((double)zb) * MIN_BOX_SIZE;
        DoPVSpos(&pos);
        printf("Compute NL / NF...");
        int NL;
        int NF;
        int NLF;
        NL = 0;
        NF = 0;
        NLF = 0;
        uint32_t seq = 1;
        int last_pvs = -1;

        for (int pvs = 0; pvs < 65536; pvs++)
        {
            if (pvs_banned[pvs])
                pvs_bitmap[pvs] = 0;
            if (pvs_bitmap[pvs])
            {
                //printf("[%d]", pvs);
                NL++;
                for (FACEQ* fq = kd_tree_leafs[pvs]; fq; fq = fq->next)
                {
                    FACE* f = fq->f;
                    if (f->parent_face) f = f->parent_face;
                    NLF++;
                    if (f->seq != seq)
                    {
                        f->seq = seq;
                        if (f->side_hit > 0)
                        {
                            NF++;
                        }
                    }
                }
                last_pvs = pvs;
            }
        }
        printf(" - %d leafs with %d faces, %d unique faces\n", NL, NLF, NF);
        if (last_pvs != -1)
        {
            static uint8_t packed_pvs[10000];
            int ppvs_len;
            ppvs_len = PackPVS(packed_pvs, pvs_bitmap, last_pvs + 1);
            printf("Unpacked pvs size = %d, packed = %d\n", (last_pvs + 7) >> 3, ppvs_len);
#if 0
            printf("pvs bitmap:");
            for (int i=0; i< last_pvs; i++)
            {
                printf("%01X", pvs_bitmap[i]);
            }
            printf("\n");
#endif
#if 0
            printf("packed pvs: ");
            for (int i = 0; i < ppvs_len; i++)
            {
                int v;
                v = packed_pvs[i];
                printf(" %02X", v);
            }
            printf("\n");
#endif
            FILE* fpvs;
            fpvs = xfopen("pvs.bin", "ab");
            pvs_indexes[index] = pvs_offset;
            pvs_offset += ppvs_len;
            fwrite(packed_pvs, 1, ppvs_len, fpvs);
            fclose(fpvs);
        }
        break;
    }
}

FACEQ* base_mesh;

uint32_t* collision_index;

FILE* collision_list_file;

FACEQ* SplitCollision(FACEQ* froot, double split, int reverse, int axis)
{
    FACEQ* ofq;
    ofq = NULL;
    size_t p;
    switch (axis)
    {
    case 0:
        p = offsetof(VERTEX, X);
        break;
    case 1:
        p = offsetof(VERTEX, Y);
        break;
    case 2:
        p = offsetof(VERTEX, Z);
        break;
    default:
        exit(70);
        break;
    }
    FACEQ* fqnext;
    for (FACEQ* fq = froot; fq; fq = fqnext)
    {
        fqnext = fq->next;
        FACE* f = fq->f;
        int pos = 0;
        for (VERTEX* v = f->v; v; v = v->next)
        {
            double* C;
            C = (double*)((size_t)v + p);
            double t;
            t = *C - split;
            if (reverse)
            {
                t = -t;
            }
            if (t < 0)
                pos |= 1;
            else
                pos |= 2;
        }
        switch (pos)
        {
        case 2:
            if (f->parent_face)
            {
                //Отбрасываем непригодившееся
                DestructFace(f);
                FreeFACEQ(fq);
            }
            break;
        default:
            //Нужно разделить на две грани
            {
                FACE* face_a = AllocFace();
                for (VERTEX* v2 = f->v; v2; v2 = v2->next)
                {
                    VERTEX* v1 = v2->prev;
                    double* C;
                    double t1, t2;
                    C = (double*)((size_t)v1 + p);
                    t1 = *C - split;
                    if (reverse)
                    {
                        t1 = -t1;
                    }
                    C = (double*)((size_t)v2 + p);
                    t2 = *C - split;
                    if (reverse)
                    {
                        t2 = -t2;
                    }
                    if ((t1 < 0.0 && t2 >= 0.0) || (t1 >= 0.0 && t2 < 0.0))
                    {
                        //Нужно создать новый вертекс
                        VERTEX* nv;
                        nv = ComputeSectionAndCreateVertex(t1, t2, v1, v2);
                        InsertVertexToFace(face_a, nv);
                    }
                    if (t2 < 0.0)
                    {
                        VERTEX* nv = CreateVertex(v2->X, v2->Y, v2->Z, 0.0, 0.0);
                        InsertVertexToFace(face_a, nv);
                    }
                }
                //Все, мы разделились на две грани, запоминаем, кто папа
                if (f->parent_face)
                {
                    face_a->parent_face = f->parent_face;
                    //У нас есть родитель, грань, которую мы только что разбили можно убить
                    DestructFace(f);
                    //И элемент списка с ней тоже
                    FreeFACEQ(fq);
                }
                else
                    face_a->parent_face = f;
                if (ComputeABCD(face_a))
                {
                    FACEQ* fq;
                    fq = AllocFACEQ();
                    fq->f = face_a;
                    fq->next = ofq;
                    ofq = fq;
                }
                else
                    DestructFace(face_a);
            }
            break;
        }
    }
    return ofq;
}

void DestructFQ(FACEQ* fq)
{
    FACEQ* fqnext;
    for (; fq; fq = fqnext)
    {
        fqnext = fq->next;
        FACE* f = fq->f;
        DestructFace(f);
        FreeFACEQ(fq);
    }
}

void DoCollision(KDTREE_NODE* root, int xa, int ya, int za, int xb, int yb, int zb, int level)
{
    switch (root->axis)
    {
    case 0:
        //X
        DoCollision(kd_tree + 0 + 2 * root->next_index, xa, ya, za, root->split, yb, zb, level + 1);
        DoCollision(kd_tree + 1 + 2 * root->next_index, root->split, ya, za, xb, yb, zb, level + 1);
        break;
    case 1:
        //Y
        DoCollision(kd_tree + 0 + 2 * root->next_index, xa, ya, za, xb, root->split, zb, level + 1);
        DoCollision(kd_tree + 1 + 2 * root->next_index, xa, root->split, za, xb, yb, zb, level + 1);
        break;
    case 2:
        //Z
        DoCollision(kd_tree + 0 + 2 * root->next_index, xa, ya, za, xb, yb, root->split, level + 1);
        DoCollision(kd_tree + 1 + 2 * root->next_index, xa, ya, root->split, xb, yb, zb, level + 1);
        break;
    case 0xFE:
        //Empty
    case 0xFF:
        uint32_t index;
        index = root - kd_tree;
        printf("Leaf %05d (size %d %d %d) F=%d: ", index, xb - xa, yb - ya, zb - za, root->axis-0xFE);
        FACEQ* fq = base_mesh;
#define MIN_D_WALK (1.6)
        //Грани по X
        fq = SplitCollision(fq, ((double)xa) * MIN_BOX_SIZE - MIN_D_WALK, 1, 0);
        fq = SplitCollision(fq, ((double)xb) * MIN_BOX_SIZE + MIN_D_WALK, 0, 0);
        //Грани по Y
        fq = SplitCollision(fq, ((double)ya) * MIN_BOX_SIZE - MIN_D_WALK, 1, 1);
        fq = SplitCollision(fq, ((double)yb) * MIN_BOX_SIZE + MIN_D_WALK, 0, 1);
        //Грани по Z
        fq = SplitCollision(fq, ((double)za) * MIN_BOX_SIZE - MIN_D_WALK, 1, 2);
        fq = SplitCollision(fq, ((double)zb) * MIN_BOX_SIZE + MIN_D_WALK, 0, 2);
        if (fq)
        {
            collision_index[index] = ftell(collision_list_file);
            FACEQ* fqnext;
            for (; fq; fq = fqnext)
            {
                fqnext = fq->next;
                FACE* f = fq->f;
                uint32_t id;
                if (f->parent_face)
                    id = f->parent_face->id;
                else
                    id = f->id;
                printf("[%d]", id);
                fwrite(&id, 1, 4, collision_list_file);
                if (f->parent_face)
                {
                    DestructFace(f);
                    FreeFACEQ(fq);
                }
            }
            index = 0;
            fwrite(&index, 1, 4, collision_list_file);
        }
        else
        {
            printf("Empty, write zero index...");
        }
        printf("\n");
    }
}


void ComputeCollisionsList(void)
{
    printf("=== Compute collision list ===\n");
    collision_index = (uint32_t*)malloc(4 * kd_tree_top);
    memset(collision_index, 0, 4 * kd_tree_top);
    collision_list_file = xfopen("clist.bin","wb");
    uint32_t zero = 0;
    fwrite(&zero, 1, 4, collision_list_file); //Все со смещением 0 будут иметь пустой список
    DoCollision(kd_tree,
        KDT_IBoxA[0], KDT_IBoxA[1], KDT_IBoxA[2],
        KDT_IBoxB[0], KDT_IBoxB[1], KDT_IBoxB[2],
        0);

    FILE* f;
    f = xfopen("cindex.bin", "wb");
    fwrite(collision_index, 1, 4 * kd_tree_top, f);
    fclose(f);
    fclose(collision_list_file);
    printf("=== collision list complete! ===\n");
}


int main()
{
    InitPointsFreeQ();
    InitFacesFreeQ();
    InitVertexesFreeQ();
    InitFBPQ();
    InitFACEQ();
    InitPVS();
    //SetWorkDir("D:\\ZX\\Map\\spyro\\");
    SetWorkDir("D:\\ZX\\Map\\omni\\");
    flist = LoadObj("world.obj");
    ParseMinMax();
    DBG("minx=%.6f maxx=%.6f\n", minx, maxx);
    KDT_IBoxA[0] = (int)floor(minx / MIN_BOX_SIZE);
    KDT_IBoxB[0] = (int)floor(maxx / MIN_BOX_SIZE) + 1;
    DBG("IBoxA[0]=%d, IBoxB[0]=%d\n", KDT_IBoxA[0], KDT_IBoxB[0]);
    DBG("miny=%.6f maxy=%.6f\n", miny, maxy);
    KDT_IBoxA[1] = (int)floor(miny / MIN_BOX_SIZE);
    KDT_IBoxB[1] = (int)floor(maxy / MIN_BOX_SIZE) + 1;
    DBG("IBoxA[1]=%d, IBoxB[1]=%d\n", KDT_IBoxA[1], KDT_IBoxB[1]);
    DBG("minz=%.6f maxz=%.6f\n", minz, maxz);
    KDT_IBoxA[2] = (int)floor(minz / MIN_BOX_SIZE);
    KDT_IBoxB[2] = (int)floor(maxz / MIN_BOX_SIZE) + 1;
    DBG("IBoxA[2]=%d, IBoxB[2]=%d\n", KDT_IBoxA[2], KDT_IBoxB[2]);
    //exit(0);
    TVertex[top_tvertex][0] = 0.0;
    TVertex[top_tvertex][1] = 0.0;
    //fcubes = GenerateCube2(fcubes, minx + step / 2.0, miny + step / 2.0, minz + step / 2.0);
    top_p_search = points_global_id;
    printf("Generate base mesh...\n");
    FACEQ* fq = NULL;
    for (FACE* f = flist; f; f = f->next)
    {
        FACEQ* nf = AllocFACEQ();
        nf->f = f;
        nf->next = fq;
        fq = nf;
    }
    base_mesh = fq;
    ConstructKDtree();
    printf("raw_face_lists_size=%d\n", raw_face_lists_size);
    SaveBinaryKDtree();
    ComputeCollisionsList();
    TriangulateList();
#ifdef SINGLE_PVS
    VEC3 pos;
    //Центр
#if 1
    pos[0] = 0;
    pos[1] = 0;
    pos[2] = 0;
#endif
#if 0
    pos[0] = -97.22;
    pos[1] = -10.68;
    pos[2] = 69.78;
#endif
#if 0
    pos[0] = -100;
    pos[1] = -25;
    pos[2] = -16;
#endif
    memset(pvs_bitmap, 0, sizeof(pvs_bitmap));
    DoPVSpos(&pos);
    DumpKDtree();
#else
    //fout = fopen("pvs.log", "wt");
    memset(pvs_indexes, 0xFF, sizeof(pvs_indexes));
    FILE* fpvs;
    fpvs = xfopen("pvs.bin", "wb");
    fclose(fpvs);
    DoPVS(kd_tree,
        KDT_IBoxA[0], KDT_IBoxA[1], KDT_IBoxA[2],
        KDT_IBoxB[0], KDT_IBoxB[1], KDT_IBoxB[2],
        0);
    FILE* fpvs_index;
    fpvs_index = xfopen("pvs_idx.bin", "wb");
    fwrite(pvs_indexes, 4, pvs_top_index + 1, fpvs_index);
    fclose(fpvs_index);
#endif
}
