#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <Windows.h>
#include "render.h"

#include <stdlib.h>

static char dirname[256];
static char* fname_pos;

void SetWorkDir(const char* dir)
{
    strcpy(dirname, dir);
    fname_pos = dirname + strlen(dirname);
}

FILE* xfopen(const char* name, const char* mode)
{
    strcpy(fname_pos, name);
    return fopen(dirname, mode);
}

static FILE* flog;

#define DBG(...) do{if (flog) {fprintf(flog,__VA_ARGS__); fflush(flog);}}while(0)

#define XZOOM (0)
#define YZOOM (0)
#define ZZOOM (0)

uint8_t Texture[65536*4];

FPXYZ FPXYZ_pool[65536];
size_t FPXYZ_pool_top = 0;

uint8_t TFACE_pool[(sizeof(TFACE) + 6 * sizeof(TVERTEX)) * 10000];
size_t TFACE_pool_top = 0;
uint32_t TFACE_ID;

#define MAX_CUBES (10000)

void InitFrustumClipper(void);

void LoadTex1024FromFile(const char* name, uint8_t* D)
{
    FILE* f = xfopen(name, "rb");
    if (!f) return;
#ifdef CMODE256
    fseek(f, 0x48A, SEEK_SET);
    D += 256 * 511;
    for (int i = 0; i < 512; i++)
    {
        fread(D, 1, 256, f);
        D -= 256;
    }
#else
    fseek(f, 0x136, SEEK_SET);
    D += 256 * 1023;
    for (int i = 0; i < 1024; i++)
    {
        fread(D, 1, 256, f);
        D -= 256;
    }
#endif
    fclose(f);
#if 1
    f = xfopen("texture.bin", "wb");
    for (int i = 0; i < 1024 * 256; i++)
    {
        int c;
        c = Texture[i];
        int o;
        o = 0;
        if (c & 0x20) o |= 0x80;
        if (c & 0x10) o |= 0x08;
        if (c & 0x08) o |= 0x40;
        if (c & 0x04) o |= 0x04;
        if (c & 0x02) o |= 0x30;
        if (c & 0x01) o |= 0x03;
        fwrite(&o, 1, 1, f);
    }
    fclose(f);
#endif
}

FIXP32 Scene_max_X;
FIXP32 Scene_min_X;
FIXP32 Scene_max_Y;
FIXP32 Scene_min_Y;
FIXP32 Scene_max_Z;
FIXP32 Scene_min_Z;

//Преаллоцируем буфера
void RenderAllocMemory(void)
{
    //GenerateTexture();
    InitFrustumClipper(); 
    Scene_max_X = Scene_max_Y = Scene_max_Z = INT32_MIN;
    Scene_min_X = Scene_min_Y = Scene_min_Z = INT32_MAX;
    FPXYZ_pool_top = 0;
    TFACE_pool_top = 0;
    TFACE_ID = 0;
    //memset(Texture, 0x0F, 65536);
    int y = 0;
    for (; y < 8; y++)
    {
        memset(Texture + y * 256 + 0, 0x30, 8);
        memset(Texture + y * 256 + 8, 0x2A, 48);
        memset(Texture + y * 256 + 56, 0x3C, 8);
    }
    for (; y < 56; y++)
    {
        memset(Texture + y * 256 + 0, 0x2A, 8);
        memset(Texture + y * 256 + 8, 0x15, 48);
        memset(Texture + y * 256 + 56,0x2A, 8);
    }
    for (; y < 64; y++)
    {
        memset(Texture + y * 256 + 0, 0x0C, 8);
        memset(Texture + y * 256 + 8, 0x2A, 48);
        memset(Texture + y * 256 + 56, 0x03, 8);
    }
}

//Ищем уже существующую точку или добавляем новую
#define MIN_POINT_DIFF (10)

static FPXYZ* FillPoint(FPXYZ* p, FIXP32 X, FIXP32 Y, FIXP32 Z)
{
    if (X < Scene_min_X) Scene_min_X = X;
    if (X > Scene_max_X) Scene_max_X = X;
    if (Y < Scene_min_Y) Scene_min_Y = Y;
    if (Y > Scene_max_Y) Scene_max_Y = Y;
    if (Z < Scene_min_Z) Scene_min_Z = Z;
    if (Z > Scene_max_Z) Scene_max_Z = Z;

    p->flags = 0;
    p->X = X;
    p->Y = Y;
    p->Z = Z;
    p->seq = FrameCounter - 1;
    return p;
}



static FPXYZ* FindOrAddPoint(FIXP32 X, FIXP32 Y, FIXP32 Z)
{
    unsigned int i = 0;
    FPXYZ* p = FPXYZ_pool;
    for (; i < FPXYZ_pool_top; p++, i++)
    {
        //if (!(p->flags & XYZF_ALLOCATED)) continue;
        FIXP64 s;
        FIXP32 dx, dy, dz;
        dx = p->X - X;
        dy = p->Y - Y;
        dz = p->Z - Z;
        s = (FIXP64)dx * dx + (FIXP64)dy * dy + (FIXP64)dz * dz;
        if (s < (MIN_POINT_DIFF * MIN_POINT_DIFF))
        {
#if 0
            DBG("Point %f,%f,%f joined with %f,%f,%f!\n",
                (double)x / 256.0, (double)y / 256.0, (double)z / 256.0,
                (double)p->X / 256.0, (double)p->Y / 256.0, (double)p->Z / 256.0);
#endif
            return p;
        }
    }
    FPXYZ_pool_top++;
    FillPoint(p, X, Y, Z);
    return p;
}

static FPXYZ* AddPoint(FIXP32 X, FIXP32 Y, FIXP32 Z)
{
    FPXYZ* p = FPXYZ_pool + FPXYZ_pool_top;
    FPXYZ_pool_top++;
    FillPoint(p, X, Y, Z);
    return p;
}

#if 0
size_t DuplicateFPXYZpool(void)
{
    size_t l = FPXYZ_pool_top;
    memcpy(FPXYZ_pool + l, FPXYZ_pool, l * sizeof(FPXYZ));
    FPXYZ_pool_top += l;
    return l;
}
#endif
TFACE* AllocFace(void)
{
    TFACE* f = (TFACE*)(TFACE_pool + TFACE_pool_top);
    //Init
    f->next = NULL;
    f->T = NULL;
    f->flags = 0;
    f->avg_color = 0x33;
    f->vertexes[0].p = NULL;
    f->id = ++TFACE_ID;
    return f;
}

TVERTEX* InsertNewVertexToFace(TFACE* f, UFIXP16 U, UFIXP16 V, FPXYZ* point)
{
    TVERTEX* v = f->vertexes;
    for (; v->p; v++);
    v[1].p = NULL;
    v->p = point;
    v->U = U;
    v->V = V;
    return v;
}

FIXP32 fast_sqrt64(FIXP64 v)
{
    FIXP32 r = 0;
    FIXP32 rt;
    FIXP32 mask = 0x40000000;
    FIXP64 l;
    do
    {
        rt = r | mask;
        l = rt;
        l *= rt;
        if (v >= l)
        {
            r = rt;
        }
    } while (mask >>= 1);
    return r;
}

//Посчитаем средний цвет грани
static int tile_size = 32;

void ComputeAvgColor(TFACE *face)
{
    int counts[256];
    int max_count;
    int max_color;
    memset(counts, 0, sizeof(counts));

    max_count = 0;
    max_color = 0;

#ifdef TILE32
    for (FIXP32 u = 0; u < 32; u++)
    {
        for (FIXP32 v = 0; v < 32; v++)
        {
            uint8_t c;
            c = face->T[v * 256 + u];
            if (!c) continue;
            counts[c]++;
            if (counts[c] > max_count)
            {
                max_count = counts[c];
                max_color = c;
            }
        }
    }
    face->avg_color = max_color;
#ifdef FADE_TO_BLACK
    max_color = SKY_COLOR;
#endif
    for (FIXP32 u = 0; u < 32; u++)
    {
        for (FIXP32 v = 0; v < 32; v++)
        {
            face->T[v * 256 + u + 0x20000] = max_color;
        }
    }
#else
    TVERTEX* v;
    int n;
    n = 0;
    UFIXP32 U, V;
    UFIXP32 umin,vmin;
    U = V = 0;
    umin = 0xFFFF;
    vmin = 0xFFFF;
    for (v = face->vertexes; v->p; v++)
    {
        if (v->U < umin) umin = v->U;
        if (v->V < vmin) vmin = v->V;
        U += v->U;
        V += v->V;
        n++;
    }
    U /= n;
    V /= n;
    U &= (tile_size * 256) - 1;
    V &= (tile_size * 256) - 1;
    umin >>= 8;
    vmin >>= 8;
    for (FIXP32 u = 0; u < tile_size-2; u++)
    {
        for (FIXP32 v = 0; v < tile_size-2; v++)
        {
            uint8_t c;
            c = face->T[(v + vmin) * 256 + (u + umin)];
            if (!c) continue;
            counts[c]++;
            if (counts[c] > max_count)
            {
                max_count = counts[c];
                max_color = c;
            }
        }
    }
    /*if ((max_color & 0x30)) max_color -= 0x10;
    if ((max_color & 0x0C)) max_color -= 0x04;
    if ((max_color & 0x03)) max_color -= 0x01;*/
    face->avg_color = max_color;
#ifdef FADE_TO_BLACK
    max_color = SKY_COLOR;
#endif
    for (FIXP32 u = 0; u < tile_size; u++)
    {
        for (FIXP32 v = 0; v < tile_size; v++)
        {
            face->T[(v + vmin) * 256 + (u + umin) + 0x20000] = max_color;
        }
    }
#endif
}

//Посчитаем коэффициенты ABCD
//Это и нормаль, и уравнение плоскости для упирания в стенки при движении
//Вообще по хорошим делам надо бы выбрать самые хорошие вектора, с наибольшей точностью, но пока пусть будет так
int ComputeABCD(TFACE* face)
{
    //if (face->id == 5456)
    //{
    //    printf("debug");
    //}
    if (face->T)
        ComputeAvgColor(face);
    FIXP64 MX, MY, MZ;
    int MN;
    MX = 0; MY = 0; MZ = 0; MN = 0;
    for (TVERTEX* v = face->vertexes; v->p; v++)
    {
        MX += v->p->X;
        MY += v->p->Y;
        MZ += v->p->Z;
        MN++;
    }
    face->MX = (FIXP32)(MX / MN);
    face->MY = (FIXP32)(MY / MN);
    face->MZ = (FIXP32)(MZ / MN);
#if 0
    face->R = 0;
    for (TVERTEX* v = face->vertexes; v->p; v++)
    {
        MX = v->p->X;
        MY = v->p->Y;
        MZ = v->p->Z;
        MX -= face->MX;
        MY -= face->MY;
        MZ -= face->MZ;
        FIXP32 R = fast_sqrt64(MX * MX + MY * MY + MZ * MZ);
        if (R > face->R) face->R = R;
    }
#endif
    if (MN < 3)
        return 0;
    MN -= 2;
    TVERTEX* v;
    v = face->vertexes;
    FIXP32 x1, x2, y1, y2, z1, z2;
    FIXP64 bx, by, bz;
    //На самом деле в реальной жизни мы заменим сдвиг на 8 простым взятием одного байта
    FPXYZ* p;
    p = v->p;
    x1 = p->X;
    y1 = p->Y;
    z1 = p->Z;
    bx = 0; by = 0; bz = 0;
    FIXP64 s;
    FIXP32 bl;
    do
    {
        v++;
        if (!v->p) v = face->vertexes;
        p = v->p;
        x2 = p->X;
        y2 = p->Y;
        z2 = p->Z;
        bx += ((FIXP64)y1 * z2 - (FIXP64)z1 * y2) >> 0; //Заодно перейдем в формат 16.16
        by += ((FIXP64)z1 * x2 - (FIXP64)x1 * z2) >> 0;
        bz += ((FIXP64)x1 * y2 - (FIXP64)y1 * x2) >> 0;
        x1 = x2;
        y1 = y2;
        z1 = z2;
    } while (v != face->vertexes);
    s = bx * bx + by * by + bz * bz;
    bl = fast_sqrt64(s); //После корня получим 16.16
    if (bl < 0x80)
    {
        return 0; //Слишком маленькая грань
    }
    bx = (bx << 16) / bl; //16.32/16.16=16.16
    by = (by << 16) / bl;
    bz = (bz << 16) / bl;
    //Считаем свободный коэффициент
    s = bx * face->MX; //16.16*24.8 => 48.24
    s += by * face->MY;
    s += bz * face->MZ;
    s >>= 16; //24.8
    face->D = -(FIXP32)s;
    face->A = (FIXP32)bx;
    face->B = (FIXP32)by;
    face->C = (FIXP32)bz;
    bx -= bx >> 16;
    by -= by >> 16;
    bz -= bz >> 16;
    face->A14 = (FIXP16)(bx >> 2);
    face->B14 = (FIXP16)(by >> 2);
    face->C14 = (FIXP16)(bz >> 2);
    return 1;
}

int CompleteFace(TFACE* f)
{
    if (!ComputeABCD(f)) return 0;
    size_t sz = sizeof(TFACE);
    for (TVERTEX* v = f->vertexes; v->p; v++) sz += sizeof(TVERTEX);
    TFACE_pool_top += sz;
    return 1;
}

void PrintFaceToLog(TFACE* f)
{
    if (!flog) return;
    if (f->T)
    {
        fprintf(flog, "FACE_%d", f->id);
    }
    else
        fprintf(flog, "VFACE_%d", f->id);
    fprintf(flog, "_[%.2f %.2f %.2f %.2f](", f->A / 65536.0, f->B / 65536.0, f->C / 65536.0, f->D / 256.0);
    fprintf(flog, "_[%.3f %.3f %.3f]", -f->MX / 80.0, -f->MZ / 80.0, -f->MY / 80.0);
    /*fprintf(flog, "(");
    for (VERTEX* v = f->v; v; v = v->next)
    {
        //fprintf(flog, "%d", v->point->id);
        //if (v->next) fprintf(flog, ",");
        fprintf(flog, "[%.3f %.3f %.3f]", -v->point->X / 80.0, -v->point->Z / 80.0, -v->point->Y / 80.0);
    }
    fprintf(flog, ")");*/
    fflush(flog);
}

#pragma comment(lib, "ws2_32.lib")

void DumpWorldForTarget(void)
{
    FILE* f;
    f = xfopen("FPXYZ.bin", "wb");
    for (size_t i = 0; i < FPXYZ_pool_top; i++)
    {
        FPXYZ* p = FPXYZ_pool + i;
        FPXYZ_TARGET o;
        memset(&o, 0, sizeof(o));
        o.X = htonl(p->X * 2);
        o.Y = htonl(p->Y * 2);
        o.Z = htonl(p->Z * 2);
        fwrite(&o, 1, sizeof(o), f);
    }
    fclose(f);
    f = xfopen("FACES.bin", "wb");
    size_t i = 0;
    while (i < TFACE_pool_top)
    {
        TFACE* face = (TFACE*)(TFACE_pool + i);
        i += sizeof(TFACE);
        size_t sz = offsetof(TFACE_TARGET, vertexes);
        TFACE_TARGET o;
        memset(&o, 0, sizeof(o));
        if (face->A14 > 0x3FFF || face->A14 < -0x4000)
        {
            exit(10000);
        }
        if (face->B14 > 0x3FFF || face->B14 < -0x4000)
        {
            exit(10000);
        }
        if (face->C14 > 0x3FFF || face->C14 < -0x4000)
        {
            exit(10000);
        }
        o.A14 = ntohs(face->A14 * 2);
        o.B14 = ntohs(face->B14 * 2);
        o.C14 = ntohs(face->C14 * 2);
        o.MX = ntohl(face->MX * 2);
        o.MY = ntohl(face->MY * 2);
        o.MZ = ntohl(face->MZ * 2);
        o.flags = ntohs(face->flags);
        o.avg_color = ntohs((face->avg_color & 0x3F) << 2);
        o.T = ntohl((((uint32_t)(face->T - Texture)) >> 1) + 0x80000000);
        TVERTEX_TARGET* ov = o.vertexes;
        for (TVERTEX* v = face->vertexes; v->p; v++, ov++)
        {
            i += sizeof(TVERTEX);
            sz += sizeof(TVERTEX_TARGET);
            ov->U = ntohs(v->U);
            ov->V = ntohs(v->V);
            ov->p = ntohl((uint32_t)(v->p - FPXYZ_pool) + 1);
        }
        sz += 4; //Last zero
        fwrite(&o, 1, sz, f);
    }
    fclose(f);
}

FIXP32 TVertex[65536][2];

#define ADDQ(QNAME,ITEM) do{if (QNAME##tail) {QNAME##tail->next=ITEM;}else{QNAME##head=ITEM;}QNAME##tail=ITEM;}while(0)
#define REMQ(QNAME) do{if (!(QNAME##head=QNAME##head->next)) QNAME##tail=NULL;}while(0)

//#define ADDQ2(QNAME,ITEM) do{if (QNAME##tail) {QNAME##tail->next_in_draw_queue=ITEM;}else{QNAME##head=ITEM;}QNAME##tail=ITEM;}while(0)
//#define REMQ2(QNAME) do{if (!(QNAME##head=QNAME##head->next_in_draw_queue)) QNAME##tail=NULL;}while(0)

static FPXYZ* p_by_id[10000];

void LoadSpyro(void)
{
    flog = fopen("load.log", "wt");
    SetWorkDir("spyro\\");
#ifdef CMODE256
    LoadTex1024FromFile("Low1.bmp", Texture);
#else
    LoadTex1024FromFile("textures.bmp", Texture);
#endif
    FILE* f = xfopen("world.obj", "rt");
    char str[1024];
    unsigned int top_tvertex = 0;
    unsigned int top_vertex = 0;
    unsigned int id = 0;
    TFACE* fhead = NULL;
    TFACE* ftail = NULL;
    int load_flags = 0;
    int total_v=0;
    int total_vp=0;
    int total_f=0;
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
        if (!strcmp(s,"o"))
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
                TVertex[top_tvertex][0] = (FIXP32)floor(strtod(s, NULL) * 512.0 + 0.5) * 256;
                s = strtok(NULL, "\n\r ");
                if (!s) continue;
                TVertex[top_tvertex][1] = (FIXP32)floor(strtod(s, NULL) * 256.0 + 0.5) * 256;
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
                X *= 80.0;
                Y *= 80.0;
                Z *= 80.0;
                FPXYZ* p = FindOrAddPoint((FIXP32)X, (FIXP32)Y, (FIXP32)Z);
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
        if (!strcmp(s,"f"))
        {
            if (!(load_flags & 1)) continue;
            if (!(load_flags & (2 | 4))) continue;
            unsigned int vid[64], vtid[64];
            unsigned int n = 0;
            FIXP32 min_u = 512 * 0x100;
            FIXP32 min_v = 512 * 0x100;
            FIXP32 U, V;
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
                    U = TVertex[vt][0];
                    V = TVertex[vt][1];
                    DBG("U%05X V%04X, ", U, V);
                    if (U < 0) U = 0;
                    if (V < 0) V = 0;
                    if (U < min_u) min_u = U;
                    if (V < min_v) min_v = V;
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
                TFACE* face = AllocFace();
#ifdef TILE32
                FIXP32 mu = min_u & 0xFFFFE000;
                FIXP32 mv = min_v & 0xFFFFE000;
                face->T = Texture + (mu >> 8) * 0x100 + (mv >> 8);
                for (unsigned int i = 0; i < n; i++)
                {
                    vt = vtid[i];
                    FIXP32 Uu, Vv;
                    U = TVertex[vt][0];
                    Uu = U - mu;
                    V = TVertex[vt][1];
                    Vv = V - mv;
                    if (Vv < 0)
                        Vv = 0;
                    if (Vv > 0x1FFF)
                        Vv = 0x1FFF;
                    if (Uu < 0)
                        Uu = 0;
                    if (Uu > 0x1FFF)
                        Uu = 0x1FFF;
                    if (Uu >= 0x1000)
                        Uu -= 0x80;
                    else
                        Uu += 0x80;
                    if (Vv >= 0x1000)
                        Vv -= 0x80;
                    else
                        Vv += 0x80;
                    InsertNewVertexToFace(face, Vv * 4, Uu * 4, p_by_id[vid[i]]);
                }
#else
                FIXP32 base_y;
                if (min_u >= 0x10000)
                {
                    base_y = 0x10000;
                    face->T = Texture + 0x10000;
                    min_u -= base_y;
                }
                else
                {
                    base_y = 0;
                    face->T = Texture + 0x00000;
                }
                for (unsigned int i = 0; i < n; i++)
                {
                    vt = vtid[i];
                    FIXP32 Uu = TVertex[vt][0] - base_y;
                    FIXP32 Vv = TVertex[vt][1];
                    if (Vv < 0)
                        Vv = 0;
                    if (Vv > 0xFFFF)
                        Vv = 0xFFFF;
                    if (Uu < 0)
                        Uu = 0;
                    if (Uu > 0xFFFF)
                        Uu = 0xFFFF;
                    if ((Uu - min_u) >= 0x1000)
                        Uu -= 0x80;
                    else
                        Uu += 0x80;
                    if ((Vv - min_v) >= 0x1000)
                        Vv -= 0x80;
                    else
                        Vv += 0x80;
                    InsertNewVertexToFace(face, Vv, Uu, p_by_id[vid[i]]);
                }
#endif
                CompleteFace(face);
                ADDQ(f, face);
                total_f++;
                DBG("\n");
            }
            else if (load_flags & 4)
            {
#if 0
                FACE* f1 = AllocFace();
                FACE* f2 = AllocFace();
                for (unsigned int i = 0; i < n; i++)
                {
                    InsertNewVertexToFace(f1, 0, 0, p_by_id[vid[i]]);
                }
                for (unsigned int i = n-1; (int)i >= 0; i--)
                {
                    InsertNewVertexToFace(f2, 0, 0, p_by_id[vid[i]]);
                }
                f1->paired_portal = f2;
                f2->paired_portal = f1;
                ADDQ(f, f1);
                ADDQ(f, f2);
                if (!ComputeABCD(f1)) 
                {
                    DBG("ComputeABCD failed for "); PrintFaceToLog(f1); DBG("\n");
                }
                if (!ComputeABCD(f2))
                {
                    DBG("ComputeABCD failed for "); PrintFaceToLog(f2); DBG("\n");
                }
                //if (f1->id == 5464)
                //{
                //    DBG("*** DEBUG ***\n");
                //}
                UpdateCommonPortalEdges(f1, f2, 1);
                //UpdateCommonPortalEdges(f2, 1);
                total_portals++;
#endif
            }
            continue;
        }
    }
    fclose(f);
    DBG("============================================================\n");
    DBG("total_v=%d total_vp=%d total_f=%d total_fv=%d\n", total_v, total_vp, total_f, total_fv);
    DBG("FPXYZ_pool_top=%zd\n", FPXYZ_pool_top);
    DBG("total_portals=%d\n", total_portals);
    DBG("============================================================\n");
    //Инитим все добавленные грани
    /*for (FACE* face = fhead; face; face = face->next)
    {
        //ComputeABCD(face);
        UpdateCommonEdges(face, 1);
    }*/
    DBG("============================================================\n");
    PlayerX = 0xffffdfaa+0x100;
    PlayerY = 0xfffffa63;
    PlayerZ = 0xfffffba4;
    CamX = PlayerX;
    CamY = PlayerY;
    CamZ = PlayerZ;

    LoadKDtree();
    if (flog) fclose(flog);
    DumpWorldForTarget();
}

TFACE* PlayerSprite;

#define PLS_SIZE (MIN_D_WALK)

void CreatePlayerSprite(void)
{
    TFACE* face = AllocFace();
    face->T = Texture;
    InsertNewVertexToFace(face, 0x8880, 0x1F80, AddPoint(PLS_SIZE / 2, PLS_SIZE, 0));
    InsertNewVertexToFace(face, 0x8080, 0x1880, AddPoint(PLS_SIZE, PLS_SIZE / 2, 0));
    InsertNewVertexToFace(face, 0x8080, 0x880, AddPoint(PLS_SIZE, -PLS_SIZE / 2, 0));
    InsertNewVertexToFace(face, 0x8880, 0x0080, AddPoint(PLS_SIZE / 2, -PLS_SIZE, 0));
    InsertNewVertexToFace(face, 0x9880, 0x0080, AddPoint(-PLS_SIZE / 2, -PLS_SIZE, 0));
    InsertNewVertexToFace(face, 0x9F80, 0x0880, AddPoint(-PLS_SIZE, -PLS_SIZE / 2, 0));
    InsertNewVertexToFace(face, 0x9F80, 0x1880, AddPoint(-PLS_SIZE, PLS_SIZE / 2, 0));
    InsertNewVertexToFace(face, 0x9880, 0x1F80, AddPoint(-PLS_SIZE / 2, PLS_SIZE, 0));
    CompleteFace(face);
    PlayerSprite = face;
}

TFACE* GetAllFacesFromPool(void)
{
    TFACE* list = NULL;
    size_t i = 0;
    while (i < TFACE_pool_top)
    {
        TFACE* f = (TFACE*)(TFACE_pool + i);
        f->next = list;
        list = f;
        i += sizeof(TFACE);
        for (TVERTEX* v = f->vertexes; v->p; v++) i += sizeof(TVERTEX);
    }
    return list;
}

TFACE* FindFaceById(int id)
{
    size_t i = 0;
    while (i < TFACE_pool_top)
    {
        TFACE* f = (TFACE*)(TFACE_pool + i);
        if (f->id == id) return f;
        i += sizeof(TFACE);
        for (TVERTEX* v = f->vertexes; v->p; v++) i += sizeof(TVERTEX);
    }
    return NULL;
}

TFACE* GetFaceByOffset(uint32_t ofs)
{
    return (TFACE*)(TFACE_pool + ofs);
}

KDTREE_NODE* kd_tree;
KDTREE_FACE_NODE* kd_tree_faces;
KDTREE_LEAF* kd_tree_leafs;

int fsize(FILE* fp)
{
    int prev = ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz = ftell(fp);
    fseek(fp, prev, SEEK_SET); //go back to where we were
    return sz;
}

uint32_t* pvs_index;
uint8_t* packed_pvs;

KDTREE_BIG_LEAF* kd_tree_big_leafs;

void LoadKDtree()
{
    FILE* f;
    int sz;
    DBG("Load kd_tree...");
    f = xfopen("kdtree.bin", "rb");
    sz = fsize(f);
    kd_tree = (KDTREE_NODE*)malloc(sz);
    if (!kd_tree) exit(100);
    fread(kd_tree, 1, sz, f);
    fclose(f);
    DBG("Ok!\n");
    DBG("Load kd_tree_leafs...");
    f = xfopen("fil_idx.bin", "rb");
    sz = fsize(f);
    kd_tree_leafs = (KDTREE_LEAF*)malloc(sz + sizeof(KDTREE_LEAF));
    if (!kd_tree_leafs) exit(100);
    fread(kd_tree_leafs, 1, sz, f);
    int top_leafs = sz / sizeof(KDTREE_LEAF);
    fclose(f);
    DBG("Ok!\n");
    DBG("Load kd_tree_faces...");
    f = xfopen("faces_in_leafs.bin", "rb");
    sz = fsize(f);
    kd_tree_leafs[top_leafs].kdtf_offset = sz; //Патчим последний, чтобы не вылетел за границы (хотя надо бы в генераторе учесть)
    kd_tree_faces = (KDTREE_FACE_NODE*)malloc(sz);
    if (!kd_tree_faces) exit(100);
    fread(kd_tree_faces, 1, sz, f);
    fclose(f);
    sz /= sizeof(KDTREE_FACE_NODE);
    for (int i = 0; i < sz; i++)
    {
        TFACE* face;
        uint32_t id;
        id = kd_tree_faces[i].face_id;
        face = FindFaceById(id);
        if (face)
        {
            kd_tree_faces[i].face_offset = (uint32_t)((uint8_t*)face - TFACE_pool);
        }
        else
        {
            DBG("Face %d not found!\n", id);
        }
    }
    DBG("Ok!\n");
    DBG("Load packed pvs...");
    f = xfopen("pvs.bin", "rb");
    sz = fsize(f);
    packed_pvs = (uint8_t*)malloc(sz);
    fread(packed_pvs, 1, sz, f);
    fclose(f);
    DBG("Ok!\n");
    DBG("Load pvs index...");
    f = xfopen("pvs_idx.bin", "rb");
    sz = fsize(f);
    pvs_index = (uint32_t*)malloc(sz);
    fread(pvs_index, 1, sz, f);
    fclose(f);
    DBG("Ok!\n");
    DBG("Prepare big leafs...");
    kd_tree_big_leafs = (KDTREE_BIG_LEAF*)malloc((top_leafs + 1) * sizeof(KDTREE_BIG_LEAF));
    for (int i = 0; i <= top_leafs; i++)
    {
        KDTREE_BIG_LEAF* b = kd_tree_big_leafs + i;
        KDTREE_LEAF* l = kd_tree_leafs + i;
        int v;
        v = l->I; if (v < -127) v = -127;
        b->I = BMATRIX + 0x180 + v * 3;
        v = l->J; if (v < -127) v = -127;
        b->J = BMATRIX + 0x480 + v * 3;
        v = l->K; if (v < -127) v = -127;
        b->K = BMATRIX + 0x780 + v * 3;
        b->FaceList = kd_tree_faces + l->kdtf_offset / sizeof(KDTREE_FACE_NODE);
    }
    DBG("Ok!\n");
    DBG("Load collision index...");
    f = xfopen("cindex.bin", "rb");
    if (!f) 
        exit(20000);
    sz = fsize(f);
    collision_index = (uint32_t*)malloc(sz);
    fread(collision_index, 1, sz,f);
    fclose(f);
    DBG("Ok!\n");
    DBG("Load collision list...");
    f = xfopen("clist.bin", "rb");
    if (!f)
        exit(20000);
    sz = fsize(f);
    collision_list = (uint32_t*)malloc(sz);
    fread(collision_list, 1, sz, f);
    fclose(f);
    sz /= 4;
    DBG(" Convert %d indexes to offsets...", sz);
    for (int i = 0; i < sz; i++)
    {
        TFACE* face;
        uint32_t id;
        id = collision_list[i];
        if (!id) continue;
        face = FindFaceById(id);
        if (face)
        {
            collision_list[i] = (uint32_t)((uint8_t*)face - TFACE_pool) + 1;
        }
        else
        {
            collision_list[i] = 0;
            DBG("Face %d not found!\n", id);
        }
    }
    DBG("Ok!\n");
}

//==============================================================================
void LoadOmni(void)
{
    flog = fopen("load.log", "wt");
    SetWorkDir("omni\\");
    LoadTex1024FromFile("textures.bmp", Texture);
    FILE* f = xfopen("world.obj", "rt");
    char str[1024];
    unsigned int top_tvertex = 0;
    unsigned int top_vertex = 0;
    unsigned int id = 0;
    TFACE* fhead = NULL;
    TFACE* ftail = NULL;
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
                TVertex[top_tvertex][0] = (FIXP32)floor(strtod(s, NULL) * 512.0 + 0.5) * 256;
                s = strtok(NULL, "\n\r ");
                if (!s) continue;
                TVertex[top_tvertex][1] = (FIXP32)floor(strtod(s, NULL) * 256.0 + 0.5) * 256;
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
                X *= 80.0*1.0;
                Y *= 80.0*1.0;
                Z *= 80.0*1.0;
                FPXYZ* p = FindOrAddPoint((FIXP32)X, (FIXP32)Y, (FIXP32)Z);
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
            FIXP32 min_u = 512 * 0x100;
            FIXP32 min_v = 512 * 0x100;
            FIXP32 U, V;
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
                    U = TVertex[vt][0];
                    V = TVertex[vt][1];
                    //DBG("U%05X V%04X, ", U, V);
                    if (U < 0) U = 0;
                    if (V < 0) V = 0;
                    if (U < min_u) min_u = U;
                    if (V < min_v) min_v = V;
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
                TFACE* face = AllocFace();
#ifdef TILE32
                FIXP32 mu = min_u & 0xFFFFE000;
                FIXP32 mv = min_v & 0xFFFFE000;
                face->T = Texture + (mu >> 8) * 0x100 + (mv >> 8);
                for (unsigned int i = 0; i < n; i++)
                {
                    vt = vtid[i];
                    FIXP32 Uu, Vv;
                    U = TVertex[vt][0];
                    Uu = U - mu;
                    V = TVertex[vt][1];
                    Vv = V - mv;
                    if (Vv < 0)
                        Vv = 0;
                    if (Vv > 0x1FFF)
                        Vv = 0x1FFF;
                    if (Uu < 0)
                        Uu = 0;
                    if (Uu > 0x1FFF)
                        Uu = 0x1FFF;
#if 0
                    if (Uu >= 0x1000)
                        Uu -= 0x80;
                    else
                        Uu += 0x80;
                    if (Vv >= 0x1000)
                        Vv -= 0x80;
                    else
                        Vv += 0x80;
#endif
                    if (mu & 0x2000)
                    {
                        if (Uu >= 0x1000)
                            Uu -= 0x80;
                    }
                    else
                    {
                        if (Uu <= 0x1000)
                            Uu += 0x80;
                    }
                    if (mv & 0x2000)
                    {
                        if (Vv >= 0x1000)
                            Vv -= 0x80;
                    }
                    else
                    {
                        if (Vv <= 0x1000)
                            Vv += 0x80;
                    }


                    InsertNewVertexToFace(face, Vv * 4, Uu * 4, p_by_id[vid[i]]);
                }
#else
                FIXP32 base_y;
                if (min_u >= 0x10000)
                {
                    base_y = 0x10000;
                    face->T = Texture + 0x10000;
                    min_u -= base_y;
                }
                else
                {
                    base_y = 0;
                    face->T = Texture + 0x00000;
                }
                FIXP32 mu = min_u & 0xFFFFE000;
                FIXP32 mv = min_v & 0xFFFFE000;
                for (unsigned int i = 0; i < n; i++)
                {
                    vt = vtid[i];
                    FIXP32 Uu = TVertex[vt][0] - base_y;
                    FIXP32 Vv = TVertex[vt][1];
                    if (Vv < 0)
                        Vv = 0;
                    if (Vv > 0xFFFF)
                        Vv = 0xFFFF;
                    if (Uu < 0)
                        Uu = 0;
                    if (Uu > 0xFFFF)
                        Uu = 0xFFFF;
#if 1
                    if ((Uu - min_u) >= 0x1000)
                        Uu -= 0x80;
                    else
                        Uu += 0x80;
                    if ((Vv - min_v) >= 0x1000)
                        Vv -= 0x80;
                    else
                        Vv += 0x80;
#else
                    if (mu & 0x2000)
                    {
                        if ((Uu - min_u) >= 0x1000)
                            Uu -= 0x80;
                    }
                    else
                    {
                        if ((Uu - min_u) <= 0x1000)
                            Uu += 0x80;
                    }
                    if (mv & 0x2000)
                    {
                        if ((Vv - min_v) >= 0x1000)
                            Vv -= 0x80;
                    }
                    else
                    {
                        if ((Vv - min_v) <= 0x1000)
                            Vv += 0x80;
                    }
#endif
                    InsertNewVertexToFace(face, Vv, Uu, p_by_id[vid[i]]);
                }
#endif
                CompleteFace(face);
                ADDQ(f, face);
                total_f++;
                //DBG("\n");
            }
            continue;
        }
    }
    fclose(f);
    DBG("============================================================\n");
    DBG("total_v=%d total_vp=%d total_f=%d total_fv=%d\n", total_v, total_vp, total_f, total_fv);
    DBG("FPXYZ_pool_top=%zd\n", FPXYZ_pool_top);
    DBG("total_portals=%d\n", total_portals);
    DBG("============================================================\n");
    DBG("X=%08X/%08X\n", Scene_min_X, Scene_max_X);
    DBG("Y=%08X/%08X\n", Scene_min_Y, Scene_max_Y);
    DBG("Z=%08X/%08X\n", Scene_min_Z, Scene_max_Z);
    //Инитим все добавленные грани
    /*for (FACE* face = fhead; face; face = face->next)
    {
        //ComputeABCD(face);
        UpdateCommonEdges(face, 1);
    }*/
    DBG("============================================================\n");
#if 0
    PlayerX = 0;
    PlayerY = 0;
    PlayerZ = 0;
#else
    PlayerX = 2380;
    PlayerY = -630;
    PlayerZ = 0;
#endif
    CamX = PlayerX;
    CamY = PlayerY;
    CamZ = PlayerZ;
    LoadKDtree();
    if (flog) fclose(flog);
    DumpWorldForTarget();
}
