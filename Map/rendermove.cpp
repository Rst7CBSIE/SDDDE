#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include "render.h"
#include "renderutils.h"

//Где находится камера, 24.8
FIXP32 CamX=0;
FIXP32 CamY=0;
FIXP32 CamZ=0;

FIXP32 PlayerX = 0;
FIXP32 PlayerY = 0;
FIXP32 PlayerZ = 0;

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
    //fprintf(flog, "_[%.2f %.2f %.2f %.2f]", f->A / 65536.0, f->B / 65536.0, f->C / 65536.0, f->D / 256.0);
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

#if 0
FIXP32 ComputeDistanceToFace(FACE* face, FIXP32 X, FIXP32 Y, FIXP32 Z)
{
    FIXP64 s;
    s = (FIXP64)X * face->A;
    s += (FIXP64)Y * face->B;
    s += (FIXP64)Z * face->C; //40.24
    s >>= 16; //24.8
    return (FIXP32)s + face->D;
}
#endif

void DebugCEdistances(char *str)
{
    char* s = str;
    s += sprintf(s, "%08X %08X %08X \n", CamX, CamY, CamZ);
    s += sprintf(s, "%.2f %.2f %.2f\n", (double)CamX / 80.0, (double)CamY / 80.0, (double)CamZ / 80.0);
#if 0
    s += sprintf(s, "%d %d %d Cube%d\n", CamX, CamY, CamZ, CurrentCube->id);
    CUBE* cube = CurrentCube;
    int i = 0;
    for (FACE *f=cube->f; f; f=f->next, i++)
    {
        float d;
        d = ComputeDistanceToFace(f, CamX, CamY, CamZ) / 256.0f;
        s += sprintf(s, "%c%d=%f\n", f->portal_to_cube ? 'P' : 'F', i, d);
        if (i > 4)
        {
            s += sprintf(s, "....\n");
            break;
        }
    }
#endif
    extern FIXP32 mtx_error;
    s += sprintf(s, "mtx_error=%f\n", mtx_error / 16384.0f);

    s += sprintf(s, "TC=%d PC=%d PP=%d PV=%d PF1=%d PF2=%d PF3=%d PF=%d SF=%d DF=%d\n", 
        traveled_cubes,
        prepared_cubes,
        prepared_points,
        prepared_vertexes,
        prepared_faces_stage1,
        prepared_faces_stage2,
        prepared_faces_stage3,
        processed_faces, 
        send_to_render_faces,
        dropped_in_render_faces
    );
    extern int max_pool_used;
    s += sprintf(s, "max_pool_used=%03d\n", max_pool_used);
    *s = 0;
}

static FIXP32 isqrt(FIXP32 v)
{
    return (FIXP32)sqrt((double)v);
}

static inline FIXP32 ComputeVectorMagnitude(FIXP32 x, FIXP32 y, FIXP32 z)
{
    if (x < 0) x = -x;
    if (y < 0) y = -y;
    if (z < 0) z = -z;
    FIXP32 min, max;
    if (x < y)
    {
        min = x;
        max = y;
    }
    else
    {
        min = y;
        max = x;
    }
    FIXP32 a;
    a = max + (min >> 1);
    if (z < a)
    {
        min = z;
        max = a;
    }
    else
    {
        min = a;
        max = z;
    }
    a = max + (min >> 1);
    return a;
}

uint32_t* collision_index;
uint32_t* collision_list;

static TFACE* collided_face;

FIXP32 SD2leaf(FIXP32 X, FIXP32 Y, FIXP32 Z)
{
    DBG("*** SD2leaf ***\n");
    collided_face = NULL;
    KDTREE_NODE* node = FindKDtreeNode(X, Y, Z);
    uint32_t index = (uint32_t)(node - kd_tree);
    uint32_t* p = (uint32_t*)((size_t)collision_list + collision_index[index]);
    FIXP32 min_d;
    min_d = INT32_MAX;
    while ((index = *p++) != 0)
    {
        TFACE* face;
        face = GetFaceByOffset(index - 1);
        DBGF(face); DBG("\n");
        FIXP32 x, y, z;
        FPXYZ* p;
        TVERTEX* v = face->vertexes;
        p = v->p;
        FIXP32 xa, xp, ya, yp, za, zp;
        FIXP32 d;
        xp = X - p->X;
        yp = Y - p->Y;
        zp = Z - p->Z;
        //Сразу чекнем сторону
        //d = xp * face->A14 + yp * face->B14 + zp * face->C14;
        //if (d < 0) continue;
        do
        {
            FPXYZ* pp;
            pp = p;
            xa = p->X;
            ya = p->Y;
            za = p->Z;
            v++;
            p = v->p;
            if (!p)
            {
                v = face->vertexes;
                p = v->p;
                v = NULL;
            }
            xa -= p->X;
            ya -= p->Y;
            za -= p->Z;

            xp += xa;
            yp += ya;
            zp += za;

            x = ya * face->C14 - za * face->B14;
            y = za * face->A14 - xa * face->C14;
            z = xa * face->B14 - ya * face->A14;

            x >>= 16;
            y >>= 16;
            z >>= 16;

            if ((x * xp + y * yp + z * zp) < 0)
            {
                //Теперь считаем проекцию вектора p на вектор а
                DBG("\t...Camera outside...\n");
                FIXP32 w;
                w = xa * xa + ya * ya + za * za;
                w >>= 15;
                if (!w)
                    //    continue;       //Кстати, при вменяемой геометри мы сюда никогда не попадем
                    goto L1;
                FIXP32 h;
                h = xp * xa + yp * ya + zp * za;
                if (h < 0) h = 0;
                h /= w;
                if (h > 0x7FFF)
                {
                L1:
                    h = 0x7FFF;
                }
                xa = (xa * h) >> 15;
                ya = (ya * h) >> 15;
                za = (za * h) >> 15;
                xa -= xp;
                ya -= yp;
                za -= zp;
                //d = ComputeVectorMagnitude(xa, ya, za);
                d = xa * xa + ya * ya + za * za;
                goto L_brk;
            }
            //DBG("\t...Camera inside...\n");
        } while (v);
        d = xp * face->A14 + yp * face->B14 + zp * face->C14;
        d >>= 14;
        d = d * d;
    L_brk:
        //DBG("d=%08X\n", d);
        if (d < min_d)
        {
            min_d = d;
            DBG("\tNew min_d = %08X\n", min_d);
            collided_face = face;
        }
        ;
    }
    return min_d;
}

static inline FIXP32 AsrDecay(FIXP32 v)
{
    if (v > 0)
    {
        v >>= 1;
    }
    else
    {
        v = -((-v) >> 1);
    }
    return v;
}

//Перемещаем игрока, а следом за ним - камеру
void MovePlayer(FIXP32 dX, FIXP32 dY, FIXP32 dZ)
{
    FIXP32 X, Y, Z;
    FIXP32 dx, dy, dz;

    static FIXP32 Vx, Vy, Vz;

    dx = MUL(dX, RMTXp[0]) + MUL(dY, RMTXp[1]) + MUL(dZ, RMTXp[2]);
    dy = MUL(dX, RMTXp[3]) + MUL(dY, RMTXp[4]) + MUL(dZ, RMTXp[5]);
    dz = MUL(dX, RMTXp[6]) + MUL(dY, RMTXp[7]) + MUL(dZ, RMTXp[8]);

    dx += (Vx = AsrDecay(Vx));
    dy += (Vy = AsrDecay(Vy));
    dz += (Vz = AsrDecay(Vz));

    X = PlayerX + dx;
    Y = PlayerY + dy;
    Z = PlayerZ + dz;

    static FIXP32 last_d;

    if (!kd_tree)
    {
        PlayerX = X;
        PlayerY = Y;
        PlayerZ = Z;
        return;
    }

#if 1
    if (X <= Scene_min_X)
        return;
    if (X >= Scene_max_X)
        return;
    if (Y <= Scene_min_Y)
        return;
    if (Y >= Scene_max_Y)
        return;
    if (Z <= Scene_min_Z)
        return;
    if (Z >= Scene_max_Z)
        return;
#endif

    flog = fopen("movecam.log", "wt");
    FIXP32 min_d;
    min_d = SD2leaf(X, Y, Z);
    if (min_d >= MIN_D_WALK * MIN_D_WALK)
    {

    }
    else
    {
        X = PlayerX;
        Y = PlayerY;
        Z = PlayerZ;
        do
        {
            dx = AsrDecay(dx);
            dy = AsrDecay(dy);
            dz = AsrDecay(dz);
            min_d = (last_d + min_d + 1) / 2;
        } while (min_d < MIN_D_WALK * MIN_D_WALK);
        X += dx;
        Y += dy;
        Z += dz;
        TFACE* f;
        f = collided_face;
        //Vx = f->A14 >> 8;
        //Vy = f->B14 >> 8;
        //Vz = f->C14 >> 8;

#if 0
        TFACE* f;
        f = collided_face;
        FIXP32 t;
        t = dx * f->A14 + dy * f->B14 + dz * f->C14;
        t >>= 14;
        if (t > 0)
        {
            //goto L_err;
            //break;
            X += (f->A14 >> 9);
            Y += (f->B14 >> 9);
            Z += (f->C14 >> 9);
        }
        else
        {
            FIXP32 a, b, c;
            a = f->A14 * t;
            b = f->B14 * t;
            c = f->C14 * t;
            a >>= 14;
            b >>= 14;
            c >>= 14;
            a = dx - a;
            b = dy - b;
            c = dz - c;
            X += a;
            Y += b;
            Z += c;
        }
#endif
    }
    PlayerX = X;
    PlayerY = Y;
    PlayerZ = Z;
    last_d = min_d;
//L_err:
    if (flog) fclose(flog);
    flog = NULL;
    //Теперь корректируем положение и матрицу камеры
#ifdef NO_PLAYER
    CamX = PlayerX;
    CamY = PlayerY;
    CamZ = PlayerZ;
#else
    //Камера должна быть сзади и выше игрока
    X = MUL(CamDisp_X, RMTXp[0]) + MUL(CamDisp_Y, RMTXp[1]) + MUL(CamDisp_Z, RMTXp[2]) + PlayerX;
    Y = MUL(CamDisp_X, RMTXp[3]) + MUL(CamDisp_Y, RMTXp[4]) + MUL(CamDisp_Z, RMTXp[5]) + PlayerY;
    Z = MUL(CamDisp_X, RMTXp[6]) + MUL(CamDisp_Y, RMTXp[7]) + MUL(CamDisp_Z, RMTXp[8]) + PlayerZ;

    CamX -= (CamX - X) >> 4;
    CamY -= (CamY - Y) >> 4;
    CamZ -= (CamZ - Z) >> 4;
#endif
}
