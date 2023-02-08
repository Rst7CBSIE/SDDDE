#include "render.h"

#if 0
void MulMatrixFIXP32(FIXP32* a, FIXP32* b)
{
    FIXP32 t0,t1,t2;
    t0 = a[0] * b[0] + a[1] * b[3] + a[2] * b[6];
    t1 = a[0] * b[1] + a[1] * b[4] + a[2] * b[7];
    t2 = a[0] * b[2] + a[1] * b[5] + a[2] * b[8];
    a[0] = t0>>14; a[1] = t1>>14; a[2] = t2>>14;
    t0 = a[3] * b[0] + a[4] * b[3] + a[5] * b[6];
    t1 = a[3] * b[1] + a[4] * b[4] + a[5] * b[7];
    t2 = a[3] * b[2] + a[4] * b[5] + a[5] * b[8];
    a[3] = t0>>14; a[4] = t1>>14; a[5] = t2>>14;
    t0 = a[6] * b[0] + a[7] * b[3] + a[8] * b[6];
    t1 = a[6] * b[1] + a[7] * b[4] + a[8] * b[7];
    t2 = a[6] * b[2] + a[7] * b[5] + a[8] * b[8];
    a[6] = t0>>14; a[7] = t1>>14; a[8] = t2>>14;
}
#endif

FIXP32 RMTXp[9]=
{
#if 0
    0x3FFF,0,0,
    0,0x3FFF,0,
    0,0,0x3FFF
#else
        0,0,0x4000,
        0,0x4000,0,
        0,0,0x4000,
#endif
};

FIXP32 RMTX[9]=
{
    0x3FFF,0,0,
    0,0x3FFF,0,
    0,0,0x3FFF
};

FIXP32 BMATRIX[256 * 3 * 3];

#define BM_BITS (7)

void ConstructBMATRIX(FIXP32* m)
{
    FIXP32 cc[3];
    cc[0] = CamX;
    cc[1] = CamY;
    cc[2] = CamZ;
    FIXP32* D = BMATRIX;
    for (int j = 0; j < 3; j++)
    {
        FIXP32 Z;
        FIXP32 dZ;
        FIXP32 X;
        FIXP32 dX;
        FIXP32 Y;
        FIXP32 dY;

        D += 3 * (128 - (1UL << BM_BITS));

        //X
        dX = m[0];
        X = dX >> 1;
        X -= dX << BM_BITS;
        X -= (dX * cc[j]) >> 9;
        //Y
        dY = m[1] * 160 / 100;
        Y = dY >> 1;
        Y -= dY << BM_BITS;
        Y -= (dY * cc[j]) >> 9;
        //Z
        dZ = m[2];
        Z = dZ >> 1;
        Z -= dZ << BM_BITS;
        Z -= (dZ * cc[j]) >> 9;
        Z += 14189 / 3;
        //
        for (int i = 0; i < (2<<BM_BITS); i++)
        {
            *D++ = Z;
            Z += dZ;
            *D++ = X;
            X += dX;
            *D++ = Y;
            Y += dY;
        }

        D += 3 * (128 - (1UL << BM_BITS));

        m += 3;
    }
}

FIXP32 IMATRIX[256 * 3 * 3];

void UpdateIMTX(void)
{
    ConstructBMATRIX(RMTX);
    FIXP32* d = IMATRIX;
    //x-координата
    FIXP32 v0,v1,v2;
    FIXP32 dv0, dv1, dv2;
    int i;
    //Координата X
    v0 = -(dv0 = RMTX[0]) << 7;
    v1 = -(dv1 = RMTX[1] * 160 / 100) << 7;
    v2 = -(dv2 = RMTX[2]) << 7;
    v0 += dv0 >> 1;
    v1 += dv1 >> 1;
    v2 += dv2 >> 1;
    v0 -= (CamX * dv0) >> 9;
    v1 -= (CamX * dv1) >> 9;
    v2 -= (CamX * dv2) >> 9;
    i = 256;
    do
    {
        *d++ = v0; v0 += dv0;
        *d++ = v1; v1 += dv1;
        *d++ = v2; v2 += dv2;
    } while (--i);
    //Координата Y
    //Тут мы учтем, что Y у нас надо увеличить на пропорции экрана, чтобы правильно обрезать
    v0 = -(dv0 = RMTX[3]) << 7;
    v1 = -(dv1 = RMTX[4] * 160 / 100) << 7;
    v2 = -(dv2 = RMTX[5]) << 7;
    v0 += dv0 >> 1;
    v1 += dv1 >> 1;
    v2 += dv2 >> 1;
    v0 -= (CamY * dv0) >> 9;
    v1 -= (CamY * dv1) >> 9;
    v2 -= (CamY * dv2) >> 9;
    i = 256;
    do
    {
        *d++ = v0; v0 += dv0;
        *d++ = v1; v1 += dv1;
        *d++ = v2; v2 += dv2;
    } while (--i);
    //Координата Z, тут мы сразу прибавим R
    v0 = -(dv0 = RMTX[6]) << 7;
    v1 = -(dv1 = RMTX[7] * 160 / 100) << 7;
    v2 = -(dv2 = RMTX[8]) << 7;
    v0 += dv0 >> 1;
    v1 += dv1 >> 1;
    v2 += dv2 >> 1;
    v0 -= (CamZ * dv0) >> 9;
    v1 -= (CamZ * dv1) >> 9;
    v2 -= (CamZ * dv2) >> 9;
    v2 += 14189; //sqrt(3)/2*16384
    i = 256;
    do
    {
        *d++ = v0; v0 += dv0;
        *d++ = v1; v1 += dv1;
        *d++ = v2; v2 += dv2;
    } while (--i);
}

#if 0
//Считаем матрицу поворота
void PrepareRotMatrix(float YAW, float PITCH, float ROLL)
{
    //Уже суммарный вариант, но оси не те
    float Sx, Sy, Sz, Cx, Cy, Cz;
    Sx = sinf(FM_PI * 2.0f * PITCH);
    Cx = cosf(FM_PI * 2.0f * PITCH);
    Sy = sinf(FM_PI * 2.0f * YAW);
    Cy = cosf(FM_PI * 2.0f * YAW);
    Sz = sinf(FM_PI * 2.0f * ROLL);
    Cz = cosf(FM_PI * 2.0f * ROLL);
    RotMatrix[0] = Cy * Cz;
    RotMatrix[1] = -Cy * Sz;
    RotMatrix[2] = Sy;
    RotMatrix[3] = Cz * Sx * Sy + Cx * Sz;
    RotMatrix[4] = Cx * Cz - Sx * Sy * Sz;
    RotMatrix[5] = -Cy * Sx;
    RotMatrix[6] = -Cx * Cz * Sy + Sx * Sz;
    RotMatrix[7] = Cz * Sx + Cx * Sy * Sz;
    RotMatrix[8] = Cx * Cy;
    UpdateRMTX();
}


void PrepareRotMatrixFIXP32(FIXP32 *m, float YAW, float PITCH, float ROLL)
{
    //Уже суммарный вариант, но оси не те
    float Sx, Sy, Sz, Cx, Cy, Cz;
    float RotMatrix[9];
    Sx = sinf(FM_PI * 2.0f * PITCH);
    Cx = cosf(FM_PI * 2.0f * PITCH);
    Sy = sinf(FM_PI * 2.0f * YAW);
    Cy = cosf(FM_PI * 2.0f * YAW);
    Sz = sinf(FM_PI * 2.0f * ROLL);
    Cz = cosf(FM_PI * 2.0f * ROLL);
    RotMatrix[0] = Cy * Cz;
    RotMatrix[1] = -Cy * Sz;
    RotMatrix[2] = Sy;
    RotMatrix[3] = Cz * Sx * Sy + Cx * Sz;
    RotMatrix[4] = Cx * Cz - Sx * Sy * Sz;
    RotMatrix[5] = -Cy * Sx;
    RotMatrix[6] = -Cx * Cz * Sy + Sx * Sz;
    RotMatrix[7] = Cz * Sx + Cx * Sy * Sz;
    RotMatrix[8] = Cx * Cy;
    for (int i = 0; i < 9; i++)
    {
        m[i] = (FIXP32)(RotMatrix[i] * 16384.0f/* + 0.5f*/); //Два защитных бита
    }
}
#endif

FIXP32 mtx_error;

#if 0
static void RotateAndNormalizeMatrix(float * RotMatrix, float YAW, float PITCH, float ROLL)
{
    float um[9];
    //Проворот
    um[0] = 1.0f;
    um[1] = 0.0f;
    um[2] = 0.0f;
    um[3] = 0.0f;
    um[4] = 1.0f;
    um[5] = 0.0f;
    um[6] = 0.0f;
    um[7] = 0.0f;
    um[8] = 1.0f;
    float rm[9];
    float s, c;
    //Поворот по крену, вокруг оси Z
    s = sinf(FM_PI * 2.0f * ROLL);
    c = cosf(FM_PI * 2.0f * ROLL);
    rm[0] = c; rm[1] = -s; rm[2] = 0;
    rm[3] = s; rm[4] = c; rm[5] = 0;
    rm[6] = 0; rm[7] = 0; rm[8] = 1;
    MulMatrix(um, rm);
    //Поворот по питчу, вокруг оси X
    s = sinf(FM_PI * 2.0f * PITCH);
    c = cosf(FM_PI * 2.0f * PITCH);
    rm[0] = 1; rm[1] = 0; rm[2] = 0;
    rm[3] = 0; rm[4] = c; rm[5] = s;
    rm[6] = 0; rm[7] = -s; rm[8] = c;
    MulMatrix(um, rm);
    //Поворот по курсу, вокруг оси Y
    s = sinf(FM_PI * 2.0f * -YAW);
    c = cosf(FM_PI * 2.0f * -YAW);
    rm[0] = c; rm[1] = 0; rm[2] = -s;
    rm[3] = 0; rm[4] = 1; rm[5] = 0;
    rm[6] = s; rm[7] = 0; rm[8] = c;
    MulMatrix(um, rm);
    //Обновляем матрицу поворота
    MulMatrix(RotMatrix, um);
    //Нормализация матрицы
    float r;
    //Сначала исправляем ошибки неортогональности осей x и y
    r = RotMatrix[0] * RotMatrix[3] + RotMatrix[1] * RotMatrix[4] + RotMatrix[2] * RotMatrix[5];
    mtx_error = r;
    r = (-r) / 2.0f;
    um[0] = RotMatrix[0];
    um[1] = RotMatrix[1];
    um[2] = RotMatrix[2];
    RotMatrix[0] += RotMatrix[3] * r; RotMatrix[1] += RotMatrix[4] * r; RotMatrix[2] += RotMatrix[5] * r;
    RotMatrix[3] += um[0] * r; RotMatrix[4] += um[1] * r; RotMatrix[5] += um[2] * r;
    //Ось z считаем как векторное произведенние осей x и y
    RotMatrix[6] = RotMatrix[1] * RotMatrix[5] - RotMatrix[2] * RotMatrix[4];
    RotMatrix[7] = RotMatrix[2] * RotMatrix[3] - RotMatrix[0] * RotMatrix[5];
    RotMatrix[8] = RotMatrix[0] * RotMatrix[4] - RotMatrix[1] * RotMatrix[3];
    //Теперь каждую ось мы нормализуем, ибо ее длина должна быть равна 1
    r = RotMatrix[0] * RotMatrix[0] + RotMatrix[1] * RotMatrix[1] + RotMatrix[2] * RotMatrix[2];
    mtx_error += r;
    r = 1.5f - r / 2.0f;
    RotMatrix[0] *= r; RotMatrix[1] *= r; RotMatrix[2] *= r;
    r = RotMatrix[3] * RotMatrix[3] + RotMatrix[4] * RotMatrix[4] + RotMatrix[5] * RotMatrix[5];
    mtx_error += r;
    r = 1.5f - r / 2.0f;
    RotMatrix[3] *= r; RotMatrix[4] *= r; RotMatrix[5] *= r;
    r = RotMatrix[6] * RotMatrix[6] + RotMatrix[7] * RotMatrix[7] + RotMatrix[8] * RotMatrix[8];
    mtx_error += r;
    r = 1.5f - r / 2.0f;
    RotMatrix[6] *= r; RotMatrix[7] *= r; RotMatrix[8] *= r;
}

void UpdateRotMatrix(float YAW, float PITCH, float ROLL)
{
    RotateAndNormalizeMatrix(RotMatrix, YAW, PITCH, ROLL);
    UpdateRMTX();
    UpdateIMTX();
}

void UpdateRotMatrixP(float YAW, float PITCH, float ROLL)
{
    RotateAndNormalizeMatrix(RotMatrixP, YAW, PITCH, ROLL);
    UpdateRMTXp();
}
#endif

void RotateMTX(FIXP32* RM, FIXP32 YAW, FIXP32 PITCH, FIXP32 ROLL)
{
    if (YAW > 0x3FFF) YAW = 0x3FFF;
    if (YAW < -0x3FFF) YAW = -0x3FFF;
    if (PITCH > 0x3FFF) PITCH = 0x3FFF;
    if (PITCH < -0x3FFF) PITCH = -0x3FFF;
    if (ROLL > 0x3FFF) ROLL = 0x3FFF;
    if (ROLL < -0x3FFF) ROLL = -0x3FFF;
#if 0
    FIXP32 um[9];
    um[0] = 0x4000;
    um[1] = -ROLL;
    um[2] = YAW;
    um[3] = ROLL;
    um[4] = 0x4000;
    um[5] = PITCH;
    um[6] = -YAW;
    um[7] = -PITCH;
    um[8] = 0x4000;
    MulMatrixFIXP32(RM, um);
#endif

#if 1
    FIXP32 t0, t1, t2;

    t0 = RM[0];
    t1 = RM[1];
    t2 = RM[2];
    RM[0] += (t1 * ROLL - t2 * YAW) >> 14;
    RM[1] += (-t0 * ROLL - t2 * PITCH) >> 14;
    RM[2] += (t0 * YAW + t1 * PITCH) >> 14;

    RM += 3;

    t0 = RM[0];
    t1 = RM[1];
    t2 = RM[2];
    RM[0] += (t1 * ROLL - t2 * YAW) >> 14;
    RM[1] += (-t0 * ROLL - t2 * PITCH) >> 14;
    RM[2] += (t0 * YAW + t1 * PITCH) >> 14;
#else
    UFIXP16* PY = SQRtab + YAW;
    UFIXP16* MY = SQRtab - YAW;
    UFIXP16* PR = SQRtab + ROLL;
    UFIXP16* MR = SQRtab - ROLL;
    UFIXP16* PP = SQRtab + PITCH;
    UFIXP16* MP = SQRtab - PITCH;

    FIXP32 t0, t1, t2;
    t0 = RM[0];
    t1 = RM[1];
    t2 = RM[2];
    RM[0] += PR[t1] - MR[t1] + MY[t2] - PY[t2]; //  RM[1] * ROLL - RM[2] * YAW;
    RM[1] += MR[t0] - PR[t0] + MP[t2] - PP[t2]; // -RM[0] * ROLL - RM[2] * PITCH;
    RM[2] += PY[t0] - MY[t0] + PP[t1] - MP[t1]; //  RM[0] * YAW  + RM[1] * PITCH;

    RM += 3;

    t0 = RM[0];
    t1 = RM[1];
    t2 = RM[2];

    RM[0] += PR[t1] - MR[t1] + MY[t2] - PY[t2]; //  RM[1] * ROLL - RM[2] * YAW;
    RM[1] += MR[t0] - PR[t0] + MP[t2] - PP[t2]; // -RM[0] * ROLL - RM[2] * PITCH;
    RM[2] += PY[t0] - MY[t0] + PP[t1] - MP[t1]; //  RM[0] * YAW  + RM[1] * PITCH;

#endif
}

void NormalizeMTX(FIXP32* RM)
{
    //Нормализация матрицы
    FIXP32 r;
    FIXP32 um[3];
    //Сначала исправляем ошибки неортогональности осей x и y
    r = RM[0] * RM[3] + RM[1] * RM[4] + RM[2] * RM[5];
    r >>= 14;
    mtx_error = r;
    r = (-r) / 2;
    um[0] = RM[0];
    um[1] = RM[1];
    um[2] = RM[2];
    RM[0] += RM[3] * r >> 14; RM[1] += RM[4] * r >> 14; RM[2] += RM[5] * r >> 14;
    RM[3] += um[0] * r >> 14; RM[4] += um[1] * r >> 14; RM[5] += um[2] * r >> 14;

    //Теперь каждую ось мы нормализуем, ибо ее длина должна быть равна 1
    r = RM[0] * RM[0] + RM[1] * RM[1] + RM[2] * RM[2];
    r >>= 14;
    mtx_error += r;
    r = 16384 * 3 / 2 - r / 2;
    RM[0] = RM[0] * r >> 14;
    RM[1] = RM[1] * r >> 14;
    RM[2] = RM[2] * r >> 14;
    //
    r = RM[3] * RM[3] + RM[4] * RM[4] + RM[5] * RM[5];
    r >>= 14;
    mtx_error += r;
    r = 16384 * 3 / 2 - r / 2;
    RM[3] = RM[3] * r >> 14;
    RM[4] = RM[4] * r >> 14;
    RM[5] = RM[5] * r >> 14;
    //
    //Ось z считаем как векторное произведенние осей x и y
    RM[6] = (RM[1] * RM[5] - RM[2] * RM[4]) >> 14;
    RM[7] = (RM[2] * RM[3] - RM[0] * RM[5]) >> 14;
    RM[8] = (RM[0] * RM[4] - RM[1] * RM[3]) >> 14;
#if 0
    r = RM[6] * RM[6] + RM[7] * RM[7] + RM[8] * RM[8];
    r >>= 14;
    mtx_error += r;
    r = 16384 * 3 / 2 - r / 2;
    RM[6] = RM[6] * r >> 14;
    RM[7] = RM[7] * r >> 14;
    RM[8] = RM[8] * r >> 14;
#endif
    //Сатурация
#if 0
    for (int i = 0; i < 9; i++)
    {
        r = RM[i];
        if (r > 0x3FFF) r = 0x3FFF;
        else if (r < -0x3FFF) r = -0x3FFF;
    }
#endif
}

