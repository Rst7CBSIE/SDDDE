#define _CRT_SECURE_NO_WARNINGS

#include "render.h"

#include <stdio.h>

int display_wires;

typedef struct
{
    int32_t jumper;
    uint32_t inverse;
}INV_JUMP_ITEM;

INV_JUMP_ITEM InvAndJump_table[256];

static const uint8_t dith_mtx[16] =
{
#if 0
    0,  8,  2,  10,
    12,  4,  14,  6,
    3,  11,  1,  9,
    15,  7,  13,  5
#endif
    0,  10,  2,  8,
    5,  15,  7,  13,
    1,  11,  3,  9,
    4,  14,  6,  12
};

uint32_t dith_masks[16][4];

static R_DATA RDataBuffer[R_DATA_TOTAL_SZ];
R_DATA* RDataPool[R_DATA_TOTAL_SZ];

void InitRender3D(void)
{
    uint32_t s16_count = 1;
    for (unsigned int in = 0; in < 256; in++)
    {
        if (in > 1)
        {
            InvAndJump_table[in].inverse = (uint16_t)((0x8000UL + (in >> 1)) / in);
        }
        else
            InvAndJump_table[in].inverse = 0x7FFF;
        InvAndJump_table[in].jumper = ((-1 - (in & 15)) << 16) + s16_count;
        if ((in & 15) == 15) s16_count++;
    }
    //Инициализируем пул
    for (int i = 0; i < R_DATA_TOTAL_SZ; i++)
    {
        RDataPool[i] = RDataBuffer + i;
    }
    FILE* f;
    f = fopen("distmask.txt", "wb");
    uint8_t m1[16];
    memset(m1, 0, 16);
    for (int c = 0; c < 16; c++)
    {
        m1[dith_mtx[c]] = 0xFF;
        memcpy(dith_masks[c], m1, 16);
        fprintf(f,"  %08X, %08X, %08X, %08X, \n",
            dith_masks[c][0],
            dith_masks[c][1],
            dith_masks[c][2],
            dith_masks[c][3]
            );
    }
    fclose(f);
}

static uint32_t SlicesBuffer[4096+1024];
#define SlicesBuffer_thr (SlicesBuffer+4096)
#define SlicesBuffer_error (SlicesBuffer_thr+900)
uint32_t* Slices_wp;

static uint32_t TrapezoidsBuffer[1024 + 128];
#define TrapezoidsBuffer_thr (TrapezoidsBuffer+1024)
#define TrapezoidsBuffer_error (TrapezoidsBuffer_thr+100)
uint32_t* Trapezoids_wp;

void InitRenderSlices(void)
{
    Trapezoids_wp = TrapezoidsBuffer;
    Slices_wp = SlicesBuffer;
}

//#define MARK_EDGES

static const uint32_t blends[16] =
{
    0x00000,0x00000,
    0x00000,0x00000,

    0x20000,0x00000,
    0x00000,0x00000,

    0x20000,0x00000,
    0x00000,0x20000,

    0x20000,0x20000,
    0x00000,0x20000,
};

uint32_t *RenderSlices(uint32_t* Task)
{
    int32_t j;
    FIXP32 dUdudu = 0, dVdvdv = 0;
    UFIXP32 UUuuuu = 0, VVvvvv = 0;
    uint8_t* D=NULL;
    static uint8_t* T = Texture;

    static const uint32_t* bp = blends;

    uint16_t a;
    uint8_t color=0;

    RDBG("           Task size = %d\n", (int)(Task - SlicesBuffer));
    *Task = 0;
    Task = SlicesBuffer; //Reinit
    for (;;)
    {
        j = *Task++;
        if (j >= 0)
        {
            if (!j) return SlicesBuffer; //Complete
            j -= 2;
            size_t b = (j >> 17) & 3;
            bp = blends + b * 4;
            T = Texture + (j & 0x1FFFF);
            continue;
        }
        uint32_t v;
        v = *Task++;
        VVvvvv = (v << 8) & 0xFFFF00;
        UUuuuu = (v >> 8) & 0xFFFF00;
        dUdudu = *Task++;
        dVdvdv = *Task++;
        D = ChunkScreenR + (*Task++);

        uint8_t* T0;
        uint8_t* T1;

        size_t tb;
        tb = ((size_t)D >> 6) & 2;
        T0 = T + bp[tb++];
        T1 = T + bp[tb];

        v = j & 0xFFFF; //Счетчик
        j >>= 16;

        if (((size_t)D ^ j) & 1)
        {
            uint8_t* t;
            t = T0; T0 = T1; T1 = t;
        }

        switch (j)
        {
#define DO_COL(T) a = (UUuuuu >> 16) & 0xFF; a |= (VVvvvv>>8) & 0xFF00; *D++ = T[a]; UUuuuu += dUdudu; VVvvvv += dVdvdv;
        case -16:
        L1:
            DO_COL(T0);
        case -15:    DO_COL(T1);
        case -14:    DO_COL(T0);
        case -13:    DO_COL(T1);
        case -12:    DO_COL(T0);
        case -11:    DO_COL(T1);
        case -10:    DO_COL(T0);
        case -9:    DO_COL(T1);
        case -8:    DO_COL(T0);
        case -7:    DO_COL(T1);
        case -6:    DO_COL(T0);
        case -5:    DO_COL(T1);
        case -4:    DO_COL(T0);
        case -3:    DO_COL(T1);
        case -2:    DO_COL(T0);
        case -1:    DO_COL(T1);
        }
        if (--v) goto L1;
    }
}

uint32_t* TrapezoidsToSlices(uint32_t* in)
{
    static int N = 0;
    if (in < TrapezoidsBuffer)
    {
        return TrapezoidsBuffer;
    }
    if (in == TrapezoidsBuffer) return in; //Пусто
    *in++ = 0x80000000; //Finish
    in = TrapezoidsBuffer;
    uint32_t* wp = Slices_wp;
    uint32_t v;
    v = *in++;
    if (v == 0x80000000)
    {
        return TrapezoidsBuffer;
    }
    v <<= 1;
    do
    {
        *wp++ = v; //Save texture
        FIXP32 xmin;
        xmin = *in++;
        uint32_t D = xmin * 128;
        v = *in++;
        UFIXP32 Yt;
        FIXP32 dYt;
        UFIXP32 Yb;
        FIXP32 dYb;
        UFIXP32 UVt, UVb;
        FIXP32 dUVt, dUVb;
        do
        {
            if (v & 0x8000)
            {
                //Load t
                Yt = *in++;
                dYt = *in++;
                UVt = *in++;
                dUVt = *in++;
                Yt <<= 8;
                dYt >>= 8;
#ifdef TRAPEZOID_ENABLE_ROUNDING
                Yt += dYt;
#endif
                dYt <<= 1;
                FIXP32 m = dUVt;
#ifdef TRAPEZOID_ENABLE_ROUNDING
                m = ((m >> 1) & 0xFFFF7FFF) | (m & 0x8000);
#endif
                if (m & 0x8000)
                {
                    dUVt -= 0x10000;
                }
#ifdef TRAPEZOID_ENABLE_ROUNDING
                UVt += m;
#endif
            }
            if (v & 0x4000)
            {
                //Load b
                Yb = *in++;
                dYb = *in++;
                UVb = *in++;
                dUVb = *in++;
                Yb <<= 8;
                dYb >>= 8;
#ifdef TRAPEZOID_ENABLE_ROUNDING
                Yb += dYb;
#endif
                dYb <<= 1;
                FIXP32 m = dUVb;
#ifdef TRAPEZOID_ENABLE_ROUNDING
                m = ((m >> 1) & 0xFFFF7FFF) | (m & 0x8000);
#endif
                if (m & 0x8000)
                {
                    dUVb -= 0x10000;
                }
#ifdef TRAPEZOID_ENABLE_ROUNDING
                UVb += m;
#endif
            }
            v >>= 16; //Height
            int16_t w;
            w = v;
            do
            {
                UFIXP32 t;
                FIXP32 h;
                t = Yt >> 16;
                h = (Yb >> 16) - t;
                if (h > 0)
                {
                    *wp++ = InvAndJump_table[h - 1].jumper;
                    FIXP32 d = UVb - UVt;
                    FIXP16 dUu, dVv;
                    dUu = d >> 16;
                    dVv = d;
                    FIXP32 in;
                    in = InvAndJump_table[h].inverse;
#ifdef TILE32
                    *wp++ = (UVt >> 2) & 0x3FFF3FFF;
                    *wp++ = ((in * dUu) >> 9) & 0x00FFFFFF;
                    *wp++ = ((in * dVv) >> 9) & 0x00FFFFFF;
#else
                    *wp++ = UVt;
                    *wp++ = ((in * dUu) >> 7) & 0x00FFFFFF;
                    *wp++ = ((in * dVv) >> 7) & 0x00FFFFFF;
#endif
                    *wp++ = D + t;
                    if (wp > SlicesBuffer_error)
                    {
                        exit(2000);
                    }
                    N++;
                }
                D += 128;
                Yt += dYt;
                Yb += dYb;
                UVt += dUVt;
                UVb += dUVb;
                w--;
            } while (w >= 0);
            if (wp > SlicesBuffer_thr)
            {
                RDBG("        RenderSlices %d\n", N);
                N = 0;
                wp = RenderSlices(wp);
            }
            v = *in++;
        } while (!(v & 0x80000000));
        v <<= 1;
    } while (v);
    Slices_wp = wp;
    return TrapezoidsBuffer;
}

R_DATA** tmap_prepare(RFACE* f, R_DATA** pool)
{
    static int N = 0;
    if (!f) return pool;
    RFACE* wf = f;

    RVERTEX* v;
    RVERTEX* t;
    RVERTEX* b;
    FIXP32 xmin;
    FIXP32 xmax;
    do
    {
        xmax = 0;
        xmin = INT32_MAX;
        *--pool = (R_DATA*)wf; //Возвращаем в пул саму грань
        v = wf->vertex;
        RVERTEX* vlast = v->prev;
        do
        {
            *--pool = (R_DATA*)v; //Возвращаем в пул вертекс
            //Апдейтим все наши указатели назад
            v->prev = vlast;
            vlast = v;
            if (v->flags & SCOORD_NOT_VALID)
            {
                FIXP32 x, y, z;
                z = v->z;
                x = v->x;
                y = v->y;
                if (x > 0x3FFF || x < -0x3FFF || y > 0x3FFF || y < -0x3FFF)
                {
                    printf("error");
                }
                if (z < MIN_Z8) z = MIN_Z8;
                uint16_t logx, logy, logz;
                logz = LOGtab[z];
                logx = LOGtab[x];
                if (logx)
                {
                    if (logz <= logx)
                    {
                        logx |= 0xFFFE;
                    }
                    else
                        logx -= logz;
                }
                v->sx = EXPtab[logx];
                logy = LOGtab[y];
                if (logy)
                {
                    if (logz <= logy)
                    {
                    L_yclip:
                        logy &= 0x0001;
                        logy |= LOG5_8;
                    }
                    else
                    {
                        logy -= logz;
                        if (logy > LOG5_8) goto L_yclip;
                    }
                }
                y = EXPtab[logy] + YDISP_FP - XDISP_FP;
                if (y < SCR_B8)
                    y = SCR_B8;
                if (y > SCR_T8)
                    y = SCR_T8;
                v->sy = y;
            }
            FIXP32 x = v->sx >> 8;
            if (x > xmax) xmax = x;
            if (x <= xmin)
            {
                t = v;
                xmin = x;
            }
            b = v;
            v = v->next;
        } 
        while (v);
        if (display_wires)
        {
#ifdef CMODE_PAL32
            int c = 12;
            if (wf->xmax & TSL_FLAG_NEAR)
                c = 15;
            else if (wf->xmax & TSL_FLAG_DIVIDE)
                c = 30;
#else
            int c = 0x30;
            if (wf->xmax & TSL_FLAG_NEAR)
                c = 0x33;
            else if (wf->xmax & TSL_FLAG_DIVIDE)
                c = 0x3C;
#endif
            DrawFaceEdges(wf->vertex, c);
        }
        else
        {
            b->next = wf->vertex; //Зацикливаем списки
        }
        wf->vertex = t;
        wf->xmax = xmax;
    } while ((wf = wf->next) != NULL);
    //Первая часть закончена, теперь мы создаем трапециии
    if (display_wires) return pool;
    uint32_t* wp = Trapezoids_wp;
    do
    {
        t = f->vertex;
        *wp++ = f->T;
        b = t;
        FIXP32 tx, bx, x;
        bx = tx = x = t->sx >> 8;
        xmax = f->xmax;
        *wp++ = x;
        do
        {
            uint32_t* save_flag_p = wp;
            x++;
            wp++;
            uint32_t flags = 0;
            if (tx < x)
            {
                flags |= 0x8000;
                FIXP32 y0, u0, v0;
                FIXP32 w;
                do
                {
                    y0 = t->sy;
                    u0 = t->U;
                    v0 = t->V;
                    w = tx;
                    if (tx >= xmax) break;
                    t = t->prev;
                    tx = t->sx >> 8;
                } while (tx < x);
                w = tx - w;
                FIXP32 y1, u1, v1;
                y1 = t->sy;
                u1 = t->U;
                v1 = t->V;
                w = InvAndJump_table[w].inverse;
                y1 -= y0;
                u1 -= u0;
                v1 -= v0;
                y1 *= w;
                *wp++ = y0;
                *wp++ = y1;
                u1 *= w;
                v1 *= w;
                u1 <<= 1;
                v1 <<= 1;
                *wp++ = (u0 << 16) | v0;
                *wp++ = (u1 & 0xFFFF0000) | ((UFIXP32)v1 >> 16);
            }
            if (bx < x)
            {
                flags |= 0x4000;
                FIXP32 y0, u0, v0;
                FIXP32 w;
                do
                {
                    y0 = b->sy;
                    u0 = b->U;
                    v0 = b->V;
                    w = bx;
                    if (bx >= xmax) break;
                    b = b->next;
                    bx = b->sx >> 8;
                } while (bx < x);
                w = bx - w;
                FIXP32 y1, u1, v1;
                y1 = b->sy;
                u1 = b->U;
                v1 = b->V;
                w = InvAndJump_table[w].inverse;
                y1 -= y0;
                u1 -= u0;
                v1 -= v0;
                y1 *= w;
                *wp++ = y0;
                *wp++ = y1;
                u1 *= w;
                v1 *= w;
                u1 <<= 1;
                v1 <<= 1;
                *wp++ = (u0 << 16) | v0;
                *wp++ = (u1 & 0xFFFF0000) | ((UFIXP32)v1 >> 16);
            }
            if (wp > TrapezoidsBuffer_error)
            {
                exit(3000);
            }
            FIXP32 h = tx;
            if (bx < h) h = bx;
            h -= x;
            if (h < 0) h++;
            x += h;
            if (!flags)
            {
                exit(3001);
            }
            if (h < 0)
            {
                exit(3002);
            }
            *save_flag_p = (h << 16) | flags;
            N++;
            if (x >= xmax) break;
        } while (t != b);
        if (wp > TrapezoidsBuffer_thr)
        {
            RDBG("    TrapezoidsToSlices %d\n", N);
            N = 0;
            wp = TrapezoidsToSlices(wp);
        }
    } while ((f = f->next) != NULL);
    Trapezoids_wp = wp;
    return pool;
}

int h_dith_rand;
int l_dith_rand;
int dith_rand;

void FinishRenderSlices(void)
{
    RDBG("    TrapezoidsToSlices (cleanup)\n");
    Trapezoids_wp = TrapezoidsToSlices(Trapezoids_wp);
    RDBG("    RenderSlices (cleanup)\n");
    Slices_wp = RenderSlices(Slices_wp); //Оторендерим все слайсы
    //h_dith_rand = rand();
    //l_dith_rand = rand();
    dith_rand = (dith_rand + 1) & 15;
    //h_dith_rand = dith_mtx[dith_rand];
    //l_dith_rand = dith_mtx[dith_rand] >> 2;
}

R_DATA** fmap_prepare(RFACE* f, R_DATA **pool)
{
    if (!f) return pool;
    RFACE* wf = f;
    RVERTEX* v;
    RVERTEX* t;
    RVERTEX* b;
    FIXP32 xmin;
    FIXP32 xmax;
    do
    {
        xmax = 0;
        xmin = INT32_MAX;
        *--pool = (R_DATA*)wf; //Возвращаем в пул саму грань
        v = wf->vertex;
        RVERTEX* vlast = v->prev;
        do
        {
            *--pool = (R_DATA*)v; //Возвращаем в пул вертекс
            //Апдейтим все наши указатели назад
            v->prev = vlast;
            vlast = v;
            if (v->flags & SCOORD_NOT_VALID)
            {
                FIXP32 x, y, z;
                z = v->z;
                x = v->x;
                y = v->y;
                if (x > 0x3FFF || x < -0x3FFF || y > 0x3FFF || y < -0x3FFF)
                {
                    printf("error");
                }
                if (z < MIN_Z8) z = MIN_Z8;
                uint16_t logx, logy, logz;
                logz = LOGtab[z];
                logx = LOGtab[x];
                if (logx)
                {
                    if (logz <= logx)
                    {
                        logx |= 0xFFFE;
                    }
                    else
                        logx -= logz;
                }
                v->sx = EXPtab[logx];
                logy = LOGtab[y];
                if (logy)
                {
                    if (logz <= logy)
                    {
                    L_yclip:
                        logy &= 0x0001;
                        logy |= LOG5_8;
                    }
                    else
                    {
                        logy -= logz;
                        if (logy > LOG5_8) goto L_yclip;
                    }
                }
                y = EXPtab[logy] + YDISP_FP - XDISP_FP;
                if (y < SCR_B8)
                    y = SCR_B8;
                if (y > SCR_T8)
                    y = SCR_T8;
                v->sy = y;
            }
            FIXP32 x = v->sx >> 8;
            if (x > xmax) xmax = x;
            if (x <= xmin)
            {
                t = v;
                xmin = x;
            }
            b = v;
            v = v->next;
        } 
        while (v);
        b->next = wf->vertex; //Зацикливаем списки
        wf->vertex = t;
        wf->xmax = xmax;
    } while ((wf = wf->next) != NULL);
    UFIXP32 Yt;
    FIXP32 dYt;
    UFIXP32 Yb;
    FIXP32 dYb;
    do
    {
        t = f->vertex;
        b = t;
        FIXP32 tx, bx, x;
        bx = tx = x = t->sx >> 8;
        xmax = f->xmax;
        uint8_t *D = ChunkScreenR + x * 128;
        do
        {
            x++;
            if (tx < x)
            {
                FIXP32 y0;
                FIXP32 w;
                do
                {
                    y0 = t->sy;
                    w = tx;
                    if (tx >= xmax) break;
                    t = t->prev;
                    tx = t->sx >> 8;
                } while (tx < x);
                w = tx - w;
                FIXP32 y1;
                y1 = t->sy;
                w = InvAndJump_table[w].inverse;
                y1 -= y0;
                y1 *= w;
		        Yt = y0;
		        dYt = y1;
                Yt <<= 8;
                dYt >>= 8;
#ifdef TRAPEZOID_ENABLE_ROUNDING
                Yt += dYt;
#endif
                dYt <<= 1;
            }
            if (bx < x)
            {
                FIXP32 y0;
                FIXP32 w;
                do
                {
                    y0 = b->sy;
                    w = bx;
                    if (bx >= xmax) break;
                    b = b->next;
                    bx = b->sx >> 8;
                } while (bx < x);
                w = bx - w;
                FIXP32 y1;
                y1 = b->sy;
                w = InvAndJump_table[w].inverse;
                y1 -= y0;
                y1 *= w;
                Yb = y0;
		        dYb = y1;
                Yb <<= 8;
                dYb >>= 8;
#ifdef TRAPEZOID_ENABLE_ROUNDING
                Yb += dYb;
#endif
                dYb <<= 1;
            }
            UFIXP32 t;
            FIXP32 h;
            t = Yt >> 16;
            h = (Yb >> 16) - t;
            if (h > 0)
            {
                uint8_t* d = D + t;
                uint32_t c = f->T;
                uint8_t i = c >> 8;
                c &= 0xFF;
                c |= c << 8;
                c |= c << 16;
                c ^= (SKY_COLOR << 0) | (SKY_COLOR << 8) | (SKY_COLOR << 16) | (SKY_COLOR << 24);
                c &= dith_masks[i][(x + h_dith_rand) & 3];
                c ^= (SKY_COLOR << 0) | (SKY_COLOR << 8) | (SKY_COLOR << 16) | (SKY_COLOR << 24);
                if ((t + l_dith_rand) & 1) c = (c >> 8) | (c << 24);
                if ((t + l_dith_rand) & 2) c = (c >> 16) | (c << 16);
                do
                {
                    *d++ = c;
                    c = (c >> 8) | (c << 24);
                } while (--h);
            }
            D += 128;
            Yt += dYt;
            Yb += dYb;
        } while (x < xmax);
    } while ((f = f->next) != NULL);
    return pool;
}
