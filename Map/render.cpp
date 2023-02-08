#define _CRT_SECURE_NO_WARNINGS
#include "render.h"
#include <assert.h>

int traveled_cubes;
int prepared_cubes;
int prepared_points;
int prepared_vertexes;
int prepared_faces_stage1;
int prepared_faces_stage2;
int prepared_faces_stage3;
int processed_faces;
int send_to_render_faces;
int dropped_in_render_faces;

uint16_t _LOGtab[32768];
uint16_t EXPtab[65536];

uint16_t inv16[16384];

void InitInv16tab(void)
{
    for (int i = 0; i < 16384; i++)
    {
        if (i < 2)
            inv16[i] = 0x7FFF;
        else
            inv16[i] = (uint16_t)((0x8000UL + (i >> 1)) / i);
    }
}

//#define DEFFERED_RENDER

//������ ��� ���������
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


TFACE *Qfaces; //���� ������� � ��������� ������

uint32_t FrameCounter;

void InitLogExpTables(void)
{
    //�������������� ������� ����������
    for (int i = 1; i < 0x4000; i++)
    {
        int m, e;
        m = i;
        e = 15;
        while (!(m & 0x8000))
        {
            m <<= 1;
            e--;
        }
        m = (int)(2048.0 * log2((double)m / 32768.0) + 0.5);
        if (m > 2047)
            m = 2047;
        m &= 0x7FF; //������� ��� � ��� ����� ����
        m <<= 1;
        LOGtab[i] = (e << 12) | m;
        LOGtab[-i] = (e << 12) | m | 1;
    }
    LOGtab[1] |= 2; //��������� ������� ��� � 1, ����� ���������� �� ����
    //������ ������� ����������
    FILE* f;
    f = fopen("logtab.txt", "wt");
    for (int i = 0; i < 0x8000; i++)
    {
        fprintf(f,"0x%04X, ", _LOGtab[i] & 0xFFFF);
        if ((i & 15) == 15) fprintf(f, "\n");
    }
    fclose(f);
    //�������������� ������� ���������
    for (int i = 0; i < 0x10000; i += 2)
    {
        int m, e;
        int r;
        r = i;
        if (r >= 0x1000)
        {
            //��� ������������� �������, ���� ���������������
            r -= 0x10000;
        }
        m = ((r >> 1) & 0x7FF);
        e = r >> 12;
        if (e > 14)
        {
            //Overflow!!!
            EXPtab[i] = 0x7FFF;
        }
        else
        {
            int v;
            double k;
            k = 32768.0 * 80.0 * 256.0;
            while (e < 15)
            {
                k /= 2.0;
                e++;
            }
            v = (int)(k * pow(2.0, (double)m / 2048.0) + 0.5);
            int v2;
            v2 = XDISP_FP + v;
            if (v2 > SCR_R8) v2 = SCR_R8;
            EXPtab[i] = v2 & 0xFFFF;
            v2 = XDISP_FP - v;
            if (v2 < SCR_L8) v2 = SCR_L8;// -1;
            EXPtab[i + 1] = v2 & 0xFFFF;
        }
    }
    EXPtab[0] = XDISP_FP; //���� � ��� 0 - �� ����� ������
    EXPtab[1] = SCR_L8;// -1; //���� �������������� ������
    //��� ����, ����� ������ ���� ������ ���������
    EXPtab[0xFFFE] = SCR_R8;
    EXPtab[0xFFFF] = SCR_L8;// -1;
    f = fopen("exptab.txt", "wt");
    for (int i = 0; i < 0x10000; i++)
    {
        fprintf(f, "0x%04X, ", EXPtab[i] & 0xFFFF);
        if ((i & 15) == 15) fprintf(f, "\n");
    }
    fclose(f);
}


UFIXP16 _SQRtab[MAX_SQR - MIN_SQR];

void InitSqrTable(void)
{
    for (int i = MIN_SQR; i < MAX_SQR; i++)
    {
        FIXP64 v;
        v = i;
        v *= i;
        v += 0x8000;
        v >>= 14+2; //sin/cos bit width
        //v >>= 2; //4ab
        SQRtab[i] = (uint16_t)v;
    }
}


void InitFrustumClipper(void)
{
    InitLogExpTables();
    InitSqrTable();
}

#define MIN_Z_CLIP ((1<<8)/5)

void DrawFaceLines(RVERTEX* _v, uint8_t color)
{
    //Draw
    RVERTEX* v = _v;
    FIXP32 sx, sy, sn;
    sx = 0;
    sy = 0;
    sn = 0;
    RVERTEX* vp = v->prev;
    do
    {
        //RVERTEX* vp = v->prev;
        sx += v->sx;
        sy += v->sy;
        sn++;
        DrawLine(vp->sx, vp->sy, v->sx, v->sy, color);
        vp = v;
    } while ((v = v->next) != NULL);
    sx /= sn;
    sy /= sn;
    v = _v;
    do
    {
        DrawLine(v->sx, v->sy, sx, sy, 0x28/*color*/);
    } while ((v = v->next) != NULL);
}

void DrawFaceEdges(RVERTEX* _v, uint8_t color)
{
    //Draw
    RVERTEX* v = _v;
    FIXP32 sx, sy, sn;
    sx = 0;
    sy = 0;
    sn = 0;
    do
    {
        RVERTEX* vp = v->prev;
        sx += v->sx;
        sy += v->sy;
        sn++;
        DrawLine(vp->sx, vp->sy, v->sx, v->sy, color);
    } while ((v = v->next) != NULL);
#if 1
    sx /= sn;
    sy /= sn;
    v = _v;
    do
    {
        DrawLine(v->sx, v->sy, sx, sy, 0x28/*color*/);
    } while ((v = v->next) != NULL);
#endif
}

#define CLIP_WITH_MUL

FILE* dump_file;
uint8_t DumpRequest;

#define INIT_RFACEQ(HEAD) ((RFACE*)((uint8_t*)&HEAD - offsetof(RFACE, next)))

R_DATA** AddVertexesByX(RFACE** fout, RFACE* f, R_DATA** pool)
{
    RDBG("\t\tAddVertexesByX...\n");
    do
    {
        RVERTEX* vs2 = f->vertex;
        RVERTEX* vs1 = vs2->prev;
        RVERTEX* vlast = (RVERTEX*)f;
        RVERTEX* vd;
        FIXP16 t1, t2;
        //goto L_no_add;
        do
        {
            if (vs1->flags & vs2->flags & FACE_DIVIDED) goto L_next;
            RVERTEX* stack;
            stack = NULL;
            FIXP16 save_t2;
            FIXP16 save_t1;
            save_t1 = (vs1->x + vs1->z) * 4;
            save_t2 = (vs2->x + vs2->z) * 4;
            int count = 3 + 1 + 3;
            do
            {
                save_t1 -= vs1->z;
                save_t2 -= vs2->z;
                t2 = save_t2;
                t1 = save_t1;
                if ((t1 ^ t2) >= 0) continue; //��� �������
                //���� ���������, ��� ���� ��������� �����
                vd = &(*pool++)->v;
                vd->flags = SCOORD_NOT_VALID; //��������� ���� ������������ sx/sy
                if (save_t2 >= 0)
                {
                    //������
                    FIXP16 t; t = t1; t1 = t2; t2 = t;
                    RVERTEX* v; v = vs1; vs1 = vs2; vs2 = v;
                }
                UFIXP16* p1, * p2, * p;
                p1 = &vs1->U;
                p2 = &vs2->U;
                p = &vd->U;
                int pcount = 5;
                FIXP32 t;
                t = t1;
                t2 = t1 - t2;
                t <<= 15;
                t = t / t2;
                do
                {
                    FIXP16 b, d;
                    b = *p1++;
                    d = *p2++;
                    d -= b;
                    *p++ = b + ((t * d /* + 0x4000*/) >> 15);
                } while (--pcount);
                if (save_t2 >= 0)
                {
                    //������ �����
                    RVERTEX* v; v = vs1; vs1 = vs2; vs2 = v;
                    vlast->next = vd;
                    vlast = vd;
                }
                else
                {
                    vd->next = stack;
                    stack = vd;
                }
            } while (--count);
            //Move from stack
            while (stack)
            {
                vlast->next = stack;
                vlast = stack;
                stack = stack->next;
            }
        L_next:
            vlast->next = vs2;
            vlast = vs2;
            vs1 = vs2;
        } while ((vs2 = vs2->next) != NULL);
        //��������� ���������� next/prev ������ ��� ������� � ���������� ��������, ��� ��� ����� ����� � ��������
        f->vertex->prev = vlast;
        vlast->next = NULL;
        //��������� ����� �� �����
    //L_no_add:
        RDBG("\t\t\tsplitted...\n");
        send_to_render_faces++;
        (*fout)->next = f;
        *fout = f;
    } while ((f = f->next) != NULL);
    return pool;
}

R_DATA** AddVertexesByY(RFACE** fout, RFACE* f, R_DATA** pool)
{
    RDBG("\t\tAddVertexesByY...\n");
    //goto L_no_add;
#if 0
    RVERTEX* vs2 = f->vertex;
    RVERTEX* vs1 = vs2->prev;
    RVERTEX* vlast = (RVERTEX*)f;
    RVERTEX* vd;
    FIXP16 t1, t2;
    do
    {
        if (!(vs1->flags & vs2->flags & FACE_DIVIDED)) goto L_next;
        RVERTEX* stack;
        stack = NULL;
        FIXP16 save_t2;
        FIXP16 save_t1;
        save_t1 = vs1->z * 5 - vs1->y * 8;
        save_t2 = vs2->z * 5 - vs2->y * 8;
        int count = 4;
        do
        {
            save_t1 -= vs1->z * 2;
            save_t2 -= vs2->z * 2;
            t2 = save_t2;
            t1 = save_t1;
            if ((t1 ^ t2) >= 0) continue; //��� �������
            //���� ���������, ��� ���� ��������� �����
            vd = &(*pool++)->v;
            vd->flags = SCOORD_NOT_VALID; //��������� ���� ������������ sx/sy
            if (save_t2 >= 0)
            {
                //������
                FIXP16 t; t = t1; t1 = t2; t2 = t;
                RVERTEX* v; v = vs1; vs1 = vs2; vs2 = v;
            }
            UFIXP16* p1, * p2, * p;
            p1 = &vs1->U;
            p2 = &vs2->U;
            p = &vd->U;
            int pcount = 5;
            FIXP32 t;
            t = t1;
            t2 = t1 - t2;
            t <<= 15;
            t = t / t2;
            do
            {
                FIXP16 b, d;
                b = *p1++;
                d = *p2++;
                d -= b;
                *p++ = b + ((t * d /* + 0x4000*/) >> 15);
            } while (--pcount);
            if (save_t2 >= 0)
            {
                //������ �����
                RVERTEX* v; v = vs1; vs1 = vs2; vs2 = v;
                vlast->next = vd;
                vlast = vd;
            }
            else
            {
                vd->next = stack;
                stack = vd;
            }
        } while (--count);
        //Move from stack
        while (stack)
        {
            vlast->next = stack;
            vlast = stack;
            stack = stack->next;
        }
    L_next:
        vlast->next = vs2;
        vlast = vs2;
        vs1 = vs2;
    } while ((vs2 = vs2->next) != NULL);
#else
    RVERTEX* vlast = f->vertex->prev;
    for (int K = 0; K <= 3; K++)
    {
        RVERTEX* vs2; 
        RVERTEX* vs1;
        vs1 = vlast;
        vs2 = f->vertex;
        vlast = (RVERTEX*)f;
        do
        {
            if (!(vs1->flags & vs2->flags & FACE_DIVIDED)) goto L_next;
            FIXP16 t1, t2;
            t1 = vs1->z;
            t2 = vs2->z;
            switch (K)
            {
            case 0: t1 = t1 * 3; t2 = t2 * 3; break;
            case 1: t1 = t1 * 1; t2 = t2 * 1; break;
            case 2: t1 = -t1 * 1; t2 = -t2 * 1; break;
            case 3: t1 = -t1 * 3; t2 = -t2 * 3; break;
            }
            t1 -= vs1->y * 8;
            t2 -= vs2->y * 8;
            FIXP16 save_t2;
            save_t2 = t2;
            if ((t1 ^ t2) < 0)
            {
                //���� ���������, ��� ���� ��������� �����
                RVERTEX* vd;
                vd = &(*pool++)->v;
                vd->flags = SCOORD_NOT_VALID | FACE_DIVIDED; //��������� ���� ������������ sx/sy
                if (save_t2 >= 0)
                {
                    //������
                    FIXP16 t; t = t1; t1 = t2; t2 = t;
                    RVERTEX* v; v = vs1; vs1 = vs2; vs2 = v;
                }
                UFIXP16* p1, * p2, * p;
                p1 = &vs1->U;
                p2 = &vs2->U;
                p = &vd->U;
                int pcount = 5;
                FIXP32 t;
                t = t1;
                t2 = t1 - t2;
                t <<= 15;
                t = t / t2;
                do
                {
                    FIXP16 b, d;
                    b = *p1++;
                    d = *p2++;
                    d -= b;
                    *p++ = b + ((t * d /* + 0x4000*/) >> 15);
                } while (--pcount);
                if (save_t2 >= 0)
                {
                    //������ �����
                    vs2 = vs1;
                }
                vlast->next = vd;
                vlast = vd;
            }
        L_next:
            vlast->next = vs2;
            vlast = vs2;
            vs1 = vs2;
        } while ((vs2 = vs2->next) != NULL);
        vlast->next = NULL;
    }
#endif
    //��������� ���������� next/prev ������ ��� ������� � ���������� ��������, ��� ��� ����� ����� � ��������
    f->vertex->prev = vlast;
    vlast->next = NULL;
    //��������� ����� �� �����
//L_no_add:
    send_to_render_faces++;
    (*fout)->next = f;
    *fout = f;
    return pool;
}

R_DATA** SplitFaceByY(RFACE** fout, RFACE* face_a, R_DATA** pool)
{
    RDBG("\t\tSplitFaceByY...\n");
    RFACE* ysplit_qhead;
    RFACE* fq = INIT_RFACEQ(ysplit_qhead);
    for (int K = 0; K <= 3; K++)
    {
        RVERTEX* vs2;
        RVERTEX* vs1;
        vs2 = face_a->vertex;
        vs1 = vs2->prev;
        RFACE* face_b;
        face_b = &(*pool++)->f;
        face_b->T = face_a->T;
        face_b->xmax = face_a->xmax;
        RVERTEX* vlast_a = (RVERTEX*)face_a; //��������� ������� ����� a
        RVERTEX* vlast_b = (RVERTEX*)face_b; //��������� ������� ����� b
        do
        {
            FIXP16 t1, t2;
            t1 = vs1->z;
            t2 = vs2->z;
            switch (K)
            {
            case 0: t1 = t1 * 3; t2 = t2 * 3; break;
            case 1: t1 = t1 * 1; t2 = t2 * 1; break;
            case 2: t1 = -t1 * 1; t2 = -t2 * 1; break;
            case 3: t1 = -t1 * 3; t2 = -t2 * 3; break;
            }
            t1 -= vs1->y * 8;
            t2 -= vs2->y * 8;
            FIXP16 save_t2;
            save_t2 = t2;
            if ((t1 ^ t2) < 0)
            {
                //���� ���������, ��� ���� ��������� ����� � �������� �� � ��� �����
                RVERTEX* vd1, * vd2;
                vd1 = &(*pool++)->v;
                vd2 = &(*pool++)->v;
                if (vs1->flags & vs2->flags & FACE_DIVIDED)
                {
                    vd1->flags = SCOORD_NOT_VALID | FACE_DIVIDED; //��������� ���� ������������ sx/sy
                    vd2->flags = SCOORD_NOT_VALID | FACE_DIVIDED; //��������� ���� ������������ sx/sy
                }
                else
                {
                    vd1->flags = SCOORD_NOT_VALID; //��������� ���� ������������ sx/sy
                    vd2->flags = SCOORD_NOT_VALID; //��������� ���� ������������ sx/sy
                }
                if (save_t2 >= 0)
                {
                    //������
                    FIXP16 t; t = t1; t1 = t2; t2 = t;
                    RVERTEX* v; v = vs1; vs1 = vs2; vs2 = v;
                }
                UFIXP16* p1, * p2, * pd1, * pd2;
                p1 = &vs1->U;
                p2 = &vs2->U;
                pd1 = &vd1->U;
                pd2 = &vd2->U;
                int pcount = 5;
                FIXP32 t;
                t = t1;
                t2 = t1 - t2;
                t <<= 15;
                //t = (t + (t2 >> 1)) / t2;
                t = t / t2;
                do
                {
                    FIXP16 b, d;
                    b = *p1++;
                    d = *p2++;
                    d -= b;
                    b = b + ((t * d /* + 0x4000*/) >> 15);
                    *pd1++ = b;
                    *pd2++ = b;
                } while (--pcount);
                if (save_t2 >= 0)
                {
                    //������ �����
                    vs2 = vs1;
                }
                vlast_a->next = vd1;
                vlast_a = vd1;
                vlast_b->next = vd2;
                vlast_b = vd2;
            }
            if (save_t2 >= 0)
            {
                //���������� � ����� a
                vlast_a->next = vs2;
                vlast_a = vs2;
            }
            else
            {
                //���������� � ����� b
                vlast_b->next = vs2;
                vlast_b = vs2;
            }
            vs1 = vs2;
        } while ((vs2 = vs2->next) != NULL);
        //���, �� ��� ������, ������ ����, �� ���� �� ������� ������ ����� � ���
        if (vlast_b != (RVERTEX*)face_b)
        {
            vlast_b->next = NULL;
            //��������� ���������� next/prev ������ ��� ������� � ���������� ��������, ��� ��� ����� ����� � ��������
            face_b->vertex->prev = vlast_b;
            //pool = AddVertexesByX(fout, face_b, pool);
            RDBG("\t\t\tsplitted...\n");
            fq->next = face_b;
            fq = face_b;
        }
        else
        {
            //����� � �����
            *--pool = (R_DATA*)face_b; //���������� � ��� �����
        }
        if (vlast_a != (RVERTEX*)face_a)
        {
            //��������� ���������� next/prev ������ ��� ������� � ���������� ��������, ��� ��� ����� ����� � ��������
            vlast_a->next = NULL;
            face_a->vertex->prev = vlast_a;
        }
        else
        {
            //����� � �����, � ������ ��� ������ ������ ������
            *--pool = (R_DATA*)face_a; //���������� � ��� �����
            goto L1;
        }
    }
    //��������� ������� ����� �� �����
    fq->next = face_a;
    fq = face_a;
L1:
    fq->next = NULL;
    if (ysplit_qhead)
    {
        pool = AddVertexesByX(fout, ysplit_qhead, pool);
    }
    return pool;
}

R_DATA** SplitFaceByZ(RFACE** fout, RFACE* face_a, R_DATA** pool)
{
    RDBG("\tSplitFaceByZ...\n");
    RVERTEX* vs2;
    RVERTEX* vs1;
    vs2 = face_a->vertex;
    vs1 = vs2->prev;
    RFACE* face_b;
    face_b = &(*pool++)->f;
    face_b->T = face_a->T;
    face_b->xmax = face_a->xmax;
    RVERTEX* vlast_a = (RVERTEX*)face_a; //��������� ������� ����� a
    RVERTEX* vlast_b = (RVERTEX*)face_b; //��������� ������� ����� b
    do
    {
        FIXP16 t1, t2;
        t1 = vs1->z;
        t2 = vs2->z;
        t1 -= TSL_Z_THR;
        t2 -= TSL_Z_THR;
        FIXP16 save_t2;
        save_t2 = t2;
        if ((t1 ^ t2) < 0)
        {
            //���� ���������, ��� ���� ��������� ����� � �������� �� � ��� �����
            RVERTEX* vd1, * vd2;
            vd1 = &(*pool++)->v;
            vd2 = &(*pool++)->v;
            vd1->flags = SCOORD_NOT_VALID | FACE_DIVIDED; //��������� ���� ������������ sx/sy
            vd2->flags = SCOORD_NOT_VALID | FACE_DIVIDED; //��������� ���� ������������ sx/sy
            if (save_t2 >= 0)
            {
                //������
                FIXP16 t; t = t1; t1 = t2; t2 = t;
                RVERTEX* v; v = vs1; vs1 = vs2; vs2 = v;
            }
            UFIXP16* p1, * p2, * pd1, *pd2;
            p1 = &vs1->U;
            p2 = &vs2->U;
            pd1 = &vd1->U;
            pd2 = &vd2->U;
            int pcount = 5;
            FIXP32 t;
            t = t1;
            t2 = t1 - t2;
            t <<= 15;
            //t = (t + (t2 >> 1)) / t2;
            t = t / t2;
            do
            {
                FIXP16 b, d;
                b = *p1++;
                d = *p2++;
                d -= b;
                b = b + ((t * d /* + 0x4000*/) >> 15);
                *pd1++ = b;
                *pd2++ = b;
            } while (--pcount);
            if (save_t2 >= 0)
            {
                //������ �����
                vs2 = vs1;
            }
            vlast_a->next = vd1;
            vlast_a = vd1;
            vlast_b->next = vd2;
            vlast_b = vd2;
        }
        if (save_t2 >= 0)
        {
            //���������� � ����� b
            vlast_a->next = vs2;
            vlast_a = vs2;
        }
        else
        {
            //���������� � ����� b
            vlast_b->next = vs2;
            vlast_b = vs2;
        }
        vs1 = vs2;
    } while ((vs2 = vs2->next) != NULL);
    //���, �� ��� ������, ������ ����, �� ���� �� ������� ������ ����� � ���
    if (vlast_a != (RVERTEX*)face_a)
    {
        //��������� ���������� next/prev ������ ��� ������� � ���������� ��������, ��� ��� ����� ����� � ��������
        vlast_a->next = NULL;
        face_a->vertex->prev = vlast_a;
        if (vlast_b != (RVERTEX*)face_b)
        {
            pool = AddVertexesByY(fout, face_a, pool);
        }
        else
        {
            *--pool = (R_DATA*)face_b; //���������� � ��� �����
            RDBG("\t\tNo tesselation...\n");
            send_to_render_faces++;
            (*fout)->next = face_a;
            *fout = face_a;
            return pool;
        }
    }
    else
    {
        //����� � �����
        *--pool = (R_DATA*)face_a; //���������� � ��� �����
    }
    if (vlast_b != (RVERTEX*)face_b)
    {
        //��������� ���������� next/prev ������ ��� ������� � ���������� ��������, ��� ��� ����� ����� � ��������
        vlast_b->next = NULL;
        face_b->vertex->prev = vlast_b;
        //� ��� ���-�� ���� � ������� ����, ����� �� �� Y
        pool = SplitFaceByY(fout, face_b, pool);
    }
    else
    {
        //����� � �����
        *--pool = (R_DATA*)face_b; //���������� � ��� �����
    }
    return pool;
}

int max_pool_used;

R_DATA** tmap(RFACE* f, R_DATA** pool)
{
    static_assert(offsetof(RVERTEX, next) == 0, "Incorrect offset of RVERTEX.next");
    static_assert(offsetof(RFACE, vertex) == 0, "Incorrect offset of RFACE.vertex");
    RFACE* section_qhead;
    int N = 0;
    RFACE* fout;
    fout = INIT_RFACEQ(section_qhead);
    do
    {
        if (!(f->xmax & SCOORD_NOT_VALID)) goto L_accept;
        RVERTEX* vlast = f->vertex->prev;
        //������ ��������
        int stage = 5;
        do
        {
            RVERTEX* vs1;
            RVERTEX* vs2;
            RVERTEX* vd;
            FIXP16 t1, t2;
            vs1 = vlast;
            vs2 = f->vertex;
            //������� ���, ����� vlast->next �������� �� f->vertex
            vlast = (RVERTEX*)f;
            do
            {
                t1 = vs1->z;
                t2 = vs2->z;
                switch (stage)
                {
                case 5:
                    t1 -= MIN_Z_CLIP;
                    t2 -= MIN_Z_CLIP;
                    break;
                case 4:
                    t1 += vs1->x;
                    t2 += vs2->x;
                    break;
                case 3:
                    t1 -= vs1->x;
                    t2 -= vs2->x;
                    break;
                case 2:
                    t1 = (t1 + (t1 >> 2) + 1) >> 1;
                    t2 = (t2 + (t2 >> 2) + 1) >> 1;
                    t1 += vs1->y;
                    t2 += vs2->y;
                    break;
                default:
                    t1 = (t1 + (t1 >> 2) + 1) >> 1;
                    t2 = (t2 + (t2 >> 2) + 1) >> 1;
                    t1 -= vs1->y;
                    t2 -= vs2->y;
                    break;
                }
                FIXP16 save_t2;
                save_t2 = t2;
                if ((t1 ^ t2) < 0)
                {
                    //���� ���������, ��� ���� ��������� �����
                    vd = &(*pool++)->v;
                    vd->flags = SCOORD_NOT_VALID; //��������� ���� ������������ sx/sy
                    if (save_t2 >= 0)
                    {
                        //������
                        FIXP16 t; t = t1; t1 = t2; t2 = t;
                        RVERTEX* v; v = vs1; vs1 = vs2; vs2 = v;
                    }
                    UFIXP16* p1, * p2, * p;
                    p1 = &vs1->U;
                    p2 = &vs2->U;
                    p = &vd->U;
                    int pcount = 5;
                    FIXP32 t;
                    t = t1;
                    t2 = t1 - t2;
#ifdef CLIP_WITH_MUL
                    t <<= 15;
                    //t = (t + (t2 >> 1)) / t2;
                    t = t / t2;
                    do
                    {
                        FIXP16 b, d;
                        b = *p1++;
                        d = *p2++;
                        d -= b;
                        *p++ = b + ((t * d /* + 0x4000*/) >> 15);
                    } while (--pcount);
#else
                    t <<= 14;
                    //t = (t + (t2 >> 1)) / t2;
                    t = t / t2;
                    UFIXP16* P = SQRtab + t;
                    UFIXP16* M = SQRtab - t;
                    do
                    {
                        FIXP16 b, d;
                        b = *p1++;
                        d = *p2++;
                        d -= b;
                        *p++ = b + P[d] - M[d];
                    } while (--pcount);
#endif
                    if (save_t2 >= 0)
                    {
                        //������ �����
                        vs2 = vs1;
                    }
                    vlast->next = vd;
                    vlast = vd;
                }
                if (save_t2 >= 0)
                {
                    vlast->next = vs2;
                    vlast = vs2;
                }
                else
                {
                    *--pool = (R_DATA*)vs2; //���������� � ��� �������
                }
                vs1 = vs2;
            } while ((vs2 = vs2->next) != NULL);
            if (vlast == (RVERTEX*)f)
            {
                dropped_in_render_faces++;
                *--pool = (R_DATA*)f; //���������� � ��� �����
                goto L_discard;
            }
            vlast->next = NULL;
        } while (--stage);
        f->vertex->prev = vlast; //�������� ��� ��������
    L_accept:
#if 0
        if (f->xmax & TSL_FLAG_NEAR)
        {
            RDBG("\tNear face...\n");
            pool = SplitFaceByY(&fout, f, pool);
        }
        else if (f->xmax & TSL_FLAG_DIVIDE)
        {
            pool = SplitFaceByZ(&fout, f, pool);
        }
        else
        {
            RDBG("\tNo tesselation...\n");
            send_to_render_faces++;
            if (!fout)
            {
                CScoordThead = f;
            }
            else
            {
                fout->next = f;
            }
            fout = f;
            N++;
        }
#endif
        fout->next = f;
        fout = f;
        N++;
    L_discard:
        ;
    } while ((f = f->next) != NULL);
    RDBG("%d faces culled\n", N);
    fout->next = NULL;
    if (!section_qhead)
    {
        RDBG("Empty after culling!\n");
        return pool;
    }
    //������� �������� ���� �� ������ ����������
    //��� �����, ����� ���������� ��� �������������� �������/�����
    RFACE* fnext;
    fnext = NULL;
    f = section_qhead;
    N = 0;
    do
    {
        if (f->xmax & (TSL_FLAG_NEAR | TSL_FLAG_DIVIDE))
            break;
        fnext = f;
        N++;
    } while ((f = f->next) != NULL);
    //������ f � ��� ��������� �� ������ ����� ��� ����������
    //fnext - �� ��������� �����, ������� ����� ������������� ���������� �����
    if (fnext)
    {
        fnext->next = NULL;
        RDBG("%d far faces send to tmap_prepare (pool=%d)\n", N, (int)(pool - RDataPool));
        pool = tmap_prepare(section_qhead, pool);
        RDBG("Far faces rendered (pool=%d)\n", (int)(pool - RDataPool));
    }
    if (!f)
    {
        RDBG("No faces for tesselation\n");
        return pool;
    }
    //� � f � ��� ��� �������� ������ ��������� �����
    fout = INIT_RFACEQ(section_qhead);
    N = 0;
    do
    {
        N++;
        fnext = f->next;
        if (f->xmax & TSL_FLAG_NEAR)
        {
            RDBG("\tNear face...\n");
            pool = SplitFaceByY(&fout, f, pool);
            //fout->next = f;
            //fout = f;
        }
        else if (f->xmax & TSL_FLAG_DIVIDE)
        {
            RDBG("\tSplit face...\n");
            pool = SplitFaceByZ(&fout, f, pool);
            //fout->next = f;
            //fout = f;
        }
        else
        {
            RDBG("\tNo tesselation...\n");
            fout->next = f;
            fout = f;
        }
        int i;
        i = (int)(pool - RDataPool);
        if (i > max_pool_used)
            max_pool_used = i;
        if (max_pool_used >= R_DATA_THR_max_faces)
        {
            RDBG("*********** TOO SMALL POOL ******************\n");
            max_pool_used = 0;
        }
        if (pool >= /*R_DATA_THR_force_draw*/R_DATA_THR_faces)
        {
            RDBG("Send %d faces to tmap_prepare (pool=%d)\n", N, (int)(pool - RDataPool));
            if (f_render_log) fflush(f_render_log);
            fout->next = NULL;
            pool = tmap_prepare(section_qhead, pool);
            fout = INIT_RFACEQ(section_qhead);
            N = 0;
        }
    } while ((f = fnext) != NULL);
    fout->next = NULL;
    if (section_qhead)
    {
        RDBG("Send(2) %d faces to tmap_prepare (pool=%d)\n", N, (int)(pool - RDataPool));
        if (f_render_log) fflush(f_render_log);
        pool = tmap_prepare(section_qhead, pool);
    }
    return pool;
}

//�������� ��� �� ����� ������
R_DATA** RenderT(TFACE **Q, R_DATA** pool, int Z)
{
    max_pool_used = 0;
    RFACE* RenderT_qhead;
    RFACE* fout;
    int N = 0;
    fout = INIT_RFACEQ(RenderT_qhead);
    do
    {
        Z--;
        TFACE* face;
        face = Q[Z];
        for (; face; face = face->next)
        {
            RFACE* f;
            RVERTEX* vlast;
            RVERTEX* v;
            //������� RFACE
            f = &(*pool++)->f;
            f->T = (((uint32_t)(face->T - Texture) /* + (blend << 17)*/) >> 1) + 0x80000001;
            f->xmax = face->flags;
            if (dump_file)
            {
                fprintf(dump_file, "f\t%08x\t%d\t%04X\n", (uint32_t)(face->T - Texture), f->xmax, face->avg_color);
            }
            //����������� � ������ RVERTEX
            vlast = (RVERTEX*)f;
            FIXP16 min_z, max_z;
            min_z = 0x7FFF;
            max_z = 0x0000;
            for (TVERTEX* tv = face->vertexes; tv->p; tv++)
            {
                v = &(*pool++)->v;
                v->U = tv->U;
                v->V = tv->V;
                FPXYZ* p;
                p = tv->p;
                v->x = p->x;
                v->y = p->y;
                FIXP16 z;
                z = p->z;
                v->z = z;
                if (z > max_z) max_z = z;
                if (z < min_z) min_z = z;
                v->flags = p->flags & 1;
                v->sx = p->sx;
                v->sy = p->sy;
                if (dump_file)
                {
                    fprintf(dump_file, "v\t%04X\t%04X\t%04X\t%04X\t%04X\t%04X\t%04X\t%d\n",
                        v->sx,
                        v->sy,
                        (UFIXP16)v->x,
                        (UFIXP16)v->y,
                        (UFIXP16)v->z,
                        v->U,
                        v->V,
                        v->flags
                    );
                }
                vlast->next = v;
                vlast = v;
            }
            if (min_z < TSL_Z_THR)
            {
                if (max_z < TSL_Z_THR)
                    f->xmax |= TSL_FLAG_NEAR;
                else
                    f->xmax |= TSL_FLAG_DIVIDE;
            }
            //��������� ���������� next/prev ������ ��� ������� � ���������� ��������, ��� ��� ����� ����� � ��������
            vlast->next = NULL;
            f->vertex->prev = vlast;
            N++;
            fout->next = f;
            fout = f;
            if (pool >= R_DATA_THR_faces)
            {
                RDBG("Send %d faces to tmap\n", N);
                fout->next = NULL;
                pool = tmap(RenderT_qhead, pool);
                fout = INIT_RFACEQ(RenderT_qhead);
                N = 0;
            }
        }
    } while (Z);
    fout->next = NULL;
    if (RenderT_qhead)
    {
        RDBG("Send %d faces to tmap (cleanup)\n", N);
        pool = tmap(RenderT_qhead, pool);
    }
    FinishRenderSlices();
    return pool;
}

R_DATA** fmap(RFACE* f, R_DATA** pool)
{
    static_assert(offsetof(RVERTEX, next) == 0, "Incorrect offset of RVERTEX.next");
    static_assert(offsetof(RFACE, vertex) == 0, "Incorrect offset of RFACE.vertex");
    int N = 0;
    RFACE* fculling_qhead;
    RFACE* fout = INIT_RFACEQ(fculling_qhead);
    do
    {
        if (!(f->xmax & SCOORD_NOT_VALID)) goto L_accept;
        RVERTEX* vlast = f->vertex->prev;
        //������ ��������
        FIXP16 t1, t2;
        int stage = 5;
        do
        {
            RVERTEX* vs1;
            RVERTEX* vs2;
            RVERTEX* vd;
            vs1 = vlast;
            vs2 = f->vertex;
            //������� ���, ����� vlast->next �������� �� f->vertex
            vlast = (RVERTEX*)f;
            do
            {
                t1 = vs1->z;
                t2 = vs2->z;
                switch (stage)
                {
                case 5:
                    t1 -= MIN_Z_CLIP;
                    t2 -= MIN_Z_CLIP;
                    break;
                case 4:
                    t1 += vs1->x;
                    t2 += vs2->x;
                    break;
                case 3:
                    t1 -= vs1->x;
                    t2 -= vs2->x;
                    break;
                case 2:
                    t1 = (t1 + (t1 >> 2) + 1) >> 1;
                    t2 = (t2 + (t2 >> 2) + 1) >> 1;
                    t1 += vs1->y;
                    t2 += vs2->y;
                    break;
                default:
                    t1 = (t1 + (t1 >> 2) + 1) >> 1;
                    t2 = (t2 + (t2 >> 2) + 1) >> 1;
                    t1 -= vs1->y;
                    t2 -= vs2->y;
                    break;
                }
                FIXP16 save_t2;
                save_t2 = t2;
                if ((t1 ^ t2) < 0)
                {
                    //���� ���������, ��� ���� ��������� �����
                    vd = &(*pool++)->v;
                    vd->flags = SCOORD_NOT_VALID; //��������� ���� ������������ sx/sy
                    if (save_t2 >= 0)
                    {
                        //������
                        FIXP16 t; t = t1; t1 = t2; t2 = t;
                        RVERTEX* v; v = vs1; vs1 = vs2; vs2 = v;
                    }
                    FIXP16* p1, * p2, * p;
                    p1 = &vs1->x;
                    p2 = &vs2->x;
                    p = &vd->x;
                    int pcount = 3;
                    FIXP32 t;
                    t = t1;
                    t2 = t1 - t2;
#ifdef CLIP_WITH_MUL
                    t <<= 15;
                    //t = (t + (t2 >> 1)) / t2;
                    t = t / t2;
                    do
                    {
                        FIXP16 b, d;
                        b = *p1++;
                        d = *p2++;
                        d -= b;
                        *p++ = b + ((t * d /* + 0x4000*/) >> 15);
                    } while (--pcount);
#else
                    t <<= 14;
                    //t = (t + (t2 >> 1)) / t2;
                    t = t / t2;
                    UFIXP16* P = SQRtab + t;
                    UFIXP16* M = SQRtab - t;
                    do
                    {
                        FIXP16 b, d;
                        b = *p1++;
                        d = *p2++;
                        d -= b;
                        *p++ = b + P[d] - M[d];
                    } while (--pcount);
#endif
                    if (save_t2 >= 0)
                    {
                        //������ �����
                        vs2 = vs1;
                    }
                    vlast->next = vd;
                    vlast = vd;
                }
                if (save_t2 >= 0)
                {
                    vlast->next = vs2;
                    vlast = vs2;
                }
                else
                {
                    *--pool = (R_DATA*)vs2; //���������� � ��� �������
                }
                vs1 = vs2;
            } while ((vs2 = vs2->next) != NULL);
            if (vlast==(RVERTEX*)f)
            {
                dropped_in_render_faces++;
                *--pool = (R_DATA*)f; //���������� � ��� �����
                goto L_discard;
            }
#if 0
            if (vroot == vlast || vroot->next == vlast)
            {
                dropped_in_render_faces++;
                goto L_discard;
            }
#endif
            vlast->next = NULL;
        } while (--stage);
        f->vertex->prev = vlast; //�������� ��� ��������
    L_accept:
        send_to_render_faces++;
        fout->next = f;
        fout = f;
        N++;
    L_discard:
        ;
    } while ((f = f->next) != NULL);
    //���������� ��� � ����� ������
    fout->next = NULL;
    if (fculling_qhead)
    {
        RDBG("Send(2) %d faces to fmap_prepare (pool=%d)\n", N, (int)(pool - RDataPool));
        N = 0;
        pool = fmap_prepare(fculling_qhead, pool);
    }
    return pool;
}

#define SORTQ_SZ (32)

//�������� ��� �� ����� ������ (����������)
R_DATA** RenderF(TFACE **Q, R_DATA** pool, int Zmin)
//R_DATA** DrawWorldQueueF(TFACE* face, R_DATA** pool, uint32_t i)
{
    RFACE* RenderF_qhead;
    RFACE* fout = INIT_RFACEQ(RenderF_qhead);
    int Z;
    Z = SORTQ_SZ;
    int N = 0;
    do
    {
        Z--;
        int blend;
        blend = 4;
        if (Z >= 20) blend = 12;
        else if (Z >= 16) blend = 8;
        else if (Z >= 12) blend = 4;
        RDBG("RenderF %d blend=%d\n", Z, blend);
        TFACE* face;
        face = Q[Z];
        blend ^= 15;
        for (; face; face = face->next)
        {
            RFACE* f;
            RVERTEX* vlast;
            RVERTEX* v;
            //������� RFACE
            f = &(*pool++)->f;
            f->T = face->avg_color | (blend << 8);
            f->xmax = face->flags; //�������� ����� � ���� xmax
            if (dump_file)
            {
                fprintf(dump_file, "f\t%08x\t%d\t%04X\n", 0, f->xmax, face->avg_color);
            }
            //����������� � ������ RVERTEX
            vlast = (RVERTEX*)f;
            for (TVERTEX* tv = face->vertexes; tv->p; tv++)
            {
                v = &(*pool++)->v;
                FPXYZ* p;
                p = tv->p;
                v->x = p->x;
                v->y = p->y;
                v->z = p->z;
                if (!(v->flags = p->flags & 1))
                {
                    v->sx = p->sx;
                    v->sy = p->sy;
                }
                else
                {
                    v->sx = 0xFFFF;
                    v->sy = 0xFFFF;
                }
                if (dump_file)
                {
                    fprintf(dump_file, "v\t%04X\t%04X\t%04X\t%04X\t%04X\t%04X\t%04X\t%d\n",
                        v->sx,
                        v->sy,
                        (UFIXP16)v->x,
                        (UFIXP16)v->y,
                        (UFIXP16)v->z,
                        0xFFFF,
                        0xFFFF,
                        v->flags
                    );
                }
                vlast->next = v;
                vlast = v;
            }
            //��������� ���������� next/prev ������ ��� ������� � ���������� ��������, ��� ��� ����� ����� � ��������
            vlast->next = NULL;
            f->vertex->prev = vlast;
            fout->next = f;
            fout = f;
            N++;
            if (pool >= R_DATA_THR_faces)
            {
                RDBG("Send %d faces to fmap\n", N);
                N = 0;
                fout->next = NULL;
                pool = fmap(RenderF_qhead, pool);
                fout = INIT_RFACEQ(RenderF_qhead);
            }
        }
    } while (Z > 8);
    fout->next = NULL;
    if (RenderF_qhead)
    {
        RDBG("Send %d faces to fmap (cleanup)\n", N);
        pool = fmap(RenderF_qhead, pool);
    }
    return pool;
}

FILE* f_render_log;

static void DBGF2(TFACE* f)
{
    FILE* flog = f_render_log;
    if (!flog) return;
    if (f->T)
    {
        fprintf(flog, "FACE_%d", f->id);
    }
    else
        fprintf(flog, "VFACE_%d", f->id);
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


void SortFacesAndDraw(void)
{
    f_render_log = fopen("render.log", "wt");
    if (DumpRequest)
    {
        dump_file = fopen("faces_omni.dmp", "wt");
    }
    TFACE* q1[SORTQ_SZ];
    TFACE* q2[SORTQ_SZ];
    TFACE* face, * fnext;
    TFACE** qf;
    face = Qfaces;
    memset(q1, 0, sizeof(q1));
    memset(q2, 0, sizeof(q2));
    //������ ����: ������ �������� � ���������� ������
    for (; face; face=fnext)
    {
        fnext = face->next;
        //if (face->id != 0x31) continue;
        UFIXP32 z;
        z = SQRtab[face->MX - CamX] + SQRtab[face->MY - CamY] + SQRtab[face->MZ - CamZ];
        if (z >= 1024)
        {
            DBGF2(face);
            RDBG(" Too far face with avg_z=%d\n", z);
            continue;
        }
        face->avg_z = z;
        qf = q1 + (z % SORTQ_SZ);
        face->next = *qf;
        *qf = face;
    }
    for (int i = 0; i < SORTQ_SZ; i++)
    {
        face = q1[i];
        if (face)
        {
            do
            {
                fnext = face->next;
                qf = q2 + (face->avg_z / SORTQ_SZ);
                face->next = *qf;
                *qf = face;
            } while ((face = fnext) != NULL);
        }
    }
    //���, ������ ����� ������ ������� ���� � ���.
    R_DATA** pool = RDataPool;
    pool = RenderF(q2, pool, 8);
    if (pool != RDataPool)
    {
        RDBG("Not all faces free in RenderF!\n");
    }
    pool=RenderT(q2, pool, 8);
    if (pool != RDataPool)
    {
        //�� ��� ����� ���������� � ����
        exit(5000);
    }
    if (f_render_log) fclose(f_render_log);
    f_render_log = NULL;
    if (dump_file) fclose(dump_file);
    dump_file = NULL;
}

static FILE* flog;

#define DBG(...) do{if (flog) {fprintf(flog,__VA_ARGS__); fflush(flog);}}while(0)

static void DBGF(TFACE* f)
{
    if (!flog) return;
    if (f->T)
    {
        fprintf(flog, "FACE_%d", f->id);
    }
    else
        fprintf(flog, "VFACE_%d", f->id);
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


void DBGTAB(int n)
{
    if (flog)
    {
        while (n)
        {
            fputc(' ', flog);
            n--;
        }
    }
}

int test_mode;
int test_mode2;

TFACE* RotateAllFaces(TFACE* face, FIXP32 CamX, FIXP32 CamY, FIXP32 CamZ)
{
    TFACE* list = NULL;
    TFACE* fnext;
    for (; face; face = fnext)
    {
        fnext = face->next;
        FIXP32 X, Y, Z;
        //���� �������
        X = face->MX;// v->point->X;
        Y = face->MY;// v->point->Y;
        Z = face->MZ;// v->point->Z;
        X -= CamX;
        Y -= CamY;
        Z -= CamZ;
        if ((UFIXP32)X + MAX_DISTANCE > MAX_DISTANCE * 2)
            goto L_return_to_far; //������� ������
        if ((UFIXP32)Y + MAX_DISTANCE > MAX_DISTANCE * 2)
            goto L_return_to_far;
        if ((UFIXP32)Z + MAX_DISTANCE > MAX_DISTANCE * 2)
            goto L_return_to_far;
        FIXP16 dot;
        dot = MUL(face->A14, X) + MUL(face->B14, Y) + MUL(face->C14, Z);
        if (dot > 0)
        {
        L_return_to_far:
            ;
        }
        else
        {
            //����� ��������, ���������
            //face->avg_z = 0x7FFF;
            prepared_faces_stage2++;
            face->next = list;
            list = face;
        }
    }
    face = list;
    //������������� ��� �������
    for (; face; face = fnext)
    {
        fnext = face->next;
        FIXP32 X, Y, Z, x, y, z;
        //����� ���� ��������, ������� �������� ��� �������
        FPXYZ* p;
        for (TVERTEX* v = face->vertexes; (p = v->p) != NULL; v++)
        {
            if (p->seq == FrameCounter) continue; //��� ����� ��� ��������
            int f;
            f = 0;
            //������ ����� ��������� ��-�� ��������� � ��������
            X = p->X - CamX;
            Y = p->Y - CamY;
            Z = p->Z - CamZ;
            x = MUL(X, RMTX[0]) + MUL(Y, RMTX[3]) + MUL(Z, RMTX[6]);
            y = MUL(X, RMTX[1]) + MUL(Y, RMTX[4]) + MUL(Z, RMTX[7]);
            z = MUL(X, RMTX[2]) + MUL(Y, RMTX[5]) + MUL(Z, RMTX[8]);
            p->x = x;
            p->y = y;
            p->z = z;
            p->seq = FrameCounter;
            if (z < MIN_Z_CLIP)
            {
                z = 0; //����� �������� �� �������� ������ ������ ���������� (-max,0,+max)
                f = SCOORD_NOT_VALID; //����� ������ �������� ���������
            }
            uint16_t logx, logy, logz;
            logz = LOGtab[z];
            logx = LOGtab[x];
            if (logx)
            {
                if (logz <= logx)
                {
                    f = SCOORD_NOT_VALID;
                    logx |= 0xFFFE;
                }
                else
                    logx -= logz;
            }
            p->sx = EXPtab[logx];
            logy = LOGtab[y];
            if (logy)
            {
                if (logz <= logy)
                {
                L_yclip:
                    f = SCOORD_NOT_VALID;
                    logy &= 0x0001;
                    logy |= LOG5_8;
                }
                else
                {
                    logy -= logz;
                    if (logy > LOG5_8) goto L_yclip;
                }
            }
            p->sy = EXPtab[logy] + YDISP_FP - XDISP_FP;
            if (p->sy < SCR_B8)
                p->sy = SCR_B8;
            if (p->sy > SCR_T8)
                p->sy = SCR_T8;
            p->flags = f;
            prepared_points++;
        }
    }
    //��� ������� ����������
    face = list;
    list = NULL;
    for (; face; face = fnext)
    {
        fnext = face->next;
        UFIXP16 x, y;
        FIXP16 z;
        //����� ���� ��������, ������� �������� ��� �������
        FIXP16 max_z = -32768;
        UFIXP16 l = 0xFFFF;
        UFIXP16 r = 0x0000;
        UFIXP16 b = 0xFFFF;
        UFIXP16 t = 0x0000;
        int face_flag;
        face_flag = face->flags & ~SCOORD_NOT_VALID;
        FPXYZ* p;
        for (TVERTEX* v = face->vertexes; (p = v->p) != NULL; v++)
        {
            z = p->z;
            if (z > max_z) max_z = z;
            x = p->sx;
            y = p->sy;
            //Compute box size
            if (x > r) r = x;
            if (x < l) l = x;
            if (y > t) t = y;
            if (y < b) b = y;
            face_flag |= p->flags;
            prepared_vertexes++;
        }
        if (max_z < MIN_Z_CLIP)
            continue;   //��������� ����� �� ����, �����������
        //��������� �� ����������� �� ���� �����
#if 1
        if (l >= SCR_R8-254)
            continue;
        if (r <= SCR_L8+255)
            continue;
        if (b >= SCR_T8-254)
            continue;
        if (t <= SCR_B8+255)
            continue;
#endif
        //face->avg_z = max_z;
        face->flags = face_flag;
        //�����-�����, ��������� � ������ �������
        prepared_faces_stage3++;
        face->next = list;
        list = face;
    }
    return list;
}

void PrepareRenderFrame(void)
{
    traveled_cubes = 0;
    prepared_cubes = 0;
    prepared_points = 0;
    prepared_vertexes = 0;
    prepared_faces_stage1 = 0;
    prepared_faces_stage2 = 0;
    prepared_faces_stage3 = 0;
    processed_faces = 0;
    send_to_render_faces = 0;
    dropped_in_render_faces = 0;
    Qfaces = NULL;
}

KDTREE_NODE* FindKDtreeNode(FIXP32 X, FIXP32 Y, FIXP32 Z)
{
    X >>= 9; //�.�. ��� ���������� � ��� 2.0
    if (X > 127 || X < -128)
    {
        exit(10000);
    }
    Y >>= 9;
    if (Y > 127 || Y < -128)
    {
        exit(10000);
    }
    Z >>= 9;
    if (Z > 127 || Z < -128)
    {
        exit(10000);
    }
    KDTREE_NODE* node;
    node = kd_tree;
    int8_t split;
    for (;;)
    {
        split = node->split;
        uint8_t axis = node->axis;
        if (axis > 2) return node;
        node = kd_tree + node->next_index * 2;
        switch (axis)
        {
        case 0:
            if (split <= X) node++;
            break;
        case 1:
            if (split <= Y) node++;
            break;
        case 2:
            if (split <= Z) node++;
            break;
        }
    }
}

static TFACE* DecrunchPVSleaf(TFACE* list, KDTREE_BIG_LEAF* leaf)
{
    traveled_cubes++;
    int32_t x, y, z;
    z = leaf->I[0];
    x = leaf->I[1];
    y = leaf->I[2];
    z += leaf->J[0];
    x += leaf->J[1];
    y += leaf->J[2];
    z += leaf->K[0];
    x += leaf->K[1];
    y += leaf->K[2];
    RDBG("%04d % 3d % 3d % 3d -> %08X %08X %08X\t",
        (int)(leaf - kd_tree_big_leafs),
        (int)(leaf->I - BMATRIX - 0x180)/3,
        (int)(leaf->J - BMATRIX - 0x480)/3,
        (int)(leaf->K - BMATRIX - 0x780)/3,
        (int32_t)x, (int32_t)y, (int32_t)z
    );
    //goto L_accept;
    if (z < 0)
    {
        RDBG("Drop Z\n");
        return list;
    }
    if (x > z)
    {
        RDBG("Drop X+\n");
        return list;
    }
    x += z;
    if (x < 0)
    {
        RDBG("Drop X-\n");
        return list;
    }
#if 1
    z += (160 * 14189 / 100) - 14189; //sqrt(3)/2*16384
    if (y > z)
    {
        RDBG("Drop Y+\n");
        return list;
    }
    y += z;
    if (y < 0)
    {
        RDBG("Drop Y-\n");
        return list;
    }
#endif
    //L_accept:
    RDBG("Accept\n");

    prepared_cubes++;

    KDTREE_FACE_NODE* p = leaf->FaceList;
    leaf++;
    KDTREE_FACE_NODE* ptop = leaf->FaceList;
    do
    {
        TFACE* face;
        face = GetFaceByOffset(p->face_offset);
        if (face->seq != FrameCounter)
        {
            face->seq = FrameCounter;
            face->next = list;
            list = face;
            prepared_faces_stage1++;
        }
        p++;
    } while (p != ptop);
    return list;
}

TFACE* DecrunchPVS(FIXP32 X, FIXP32 Y, FIXP32 Z)
{
    TFACE* list = NULL;
    KDTREE_NODE* node;
    node = FindKDtreeNode(CamX, CamY, CamZ);
    uint32_t idx;
    static uint32_t last_idx;
    idx = pvs_index[node - kd_tree];
    if (idx == 0xFFFFFFFF)
    {
        //return NULL;
        idx = last_idx;
    }
    last_idx = idx;
    uint8_t* s;
    s = packed_pvs + idx;
    //������ �� ������ �����������
    uint8_t b;
    KDTREE_BIG_LEAF* leaf = kd_tree_big_leafs;
    uint16_t l;
    while ((b = *s++) != 0)
    {
        if (b & 0x80)
        {
            //������� �����
            do
            {
                if (b & 1)
                {
                    list = DecrunchPVSleaf(list, leaf);
                }
                leaf++;
                b >>= 1;
            } while (b != 1);
        }
        else
        {
            if (b < 0x7E)
            {
                l = b >> 1;
            }
            else
            {
                l = *s++;
                l |= (*s++) << 8;
            }
            if (b & 1)
            {
                //������������������ ������
                do
                {
                    list = DecrunchPVSleaf(list, leaf);
                    leaf++;
                } while (--l);
            }
            else
            {
                //������������������ �����
                leaf += l;
            }
        }
    }
    return list;
}

TFACE* AddPlayerToQ(TFACE* fq)
{
    TFACE* f = PlayerSprite;
    FPXYZ* p;
    f->flags = 1;
    FIXP32 X, Y, Z;
    f->MX = PlayerX;
    f->MY = PlayerY;
    f->MZ = PlayerZ;
    X = PlayerX - CamX;
    Y = PlayerY - CamY;
    Z = PlayerZ - CamZ;
    if ((UFIXP32)X + MAX_DISTANCE > MAX_DISTANCE * 2)
        goto L_return_to_far; //������� ������
    if ((UFIXP32)Y + MAX_DISTANCE > MAX_DISTANCE * 2)
        goto L_return_to_far;
    if ((UFIXP32)Z + MAX_DISTANCE > MAX_DISTANCE * 2)
        goto L_return_to_far;

    FIXP32 x, y, z;
    x = MUL(X, RMTX[0]) + MUL(Y, RMTX[3]) + MUL(Z, RMTX[6]);
    y = MUL(X, RMTX[1]) + MUL(Y, RMTX[4]) + MUL(Z, RMTX[7]);
    z = MUL(X, RMTX[2]) + MUL(Y, RMTX[5]) + MUL(Z, RMTX[8]);
    if (z < MIN_Z8) return fq;
    for (TVERTEX* v = f->vertexes; (p = v->p) != NULL; v++)
    {
        p->flags = 1;
        p->x = x + p->X;
        p->y = y + p->Y;
        p->z = z + p->Z;
    }
    f->next = fq;
    fq = f;
L_return_to_far:
    return fq;
}


void ProcessWorld(void)
{
    FrameCounter++;
    PrepareRenderFrame();
    if (kd_tree)
    //if (0)
    {
        //f_render_log = fopen("checkpvs.log","wt");
        Qfaces = DecrunchPVS(CamX, CamY, CamZ);
        //fclose(f_render_log);
        f_render_log = NULL;
    }
    else
    {
        Qfaces = GetAllFacesFromPool();
    }
    FIXP32 cx, cy, cz;
    if (test_mode)
    {
        cx = MUL(0x800, RMTX[2]);
        cy = MUL(0x800, RMTX[5]);
        cz = MUL(0x800, RMTX[8]);
    }
    else
    {
        cx = 0;
        cy = 0;
        cz = 0;
    }
    Qfaces = RotateAllFaces(Qfaces, CamX - cx, CamY - cy, CamZ - cz);
#ifdef NO_PLAYER
#else
    Qfaces = AddPlayerToQ(Qfaces);
#endif
    if (flog) fclose(flog);
    InitRenderSlices();
    SortFacesAndDraw();
}

