#ifndef RENDER_H
#define RENDER_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>

#define XYZF_SCOORD_NOT_VALID (1)

#define XYZF_HULLQ (2)

#define XYZF_ALLOCATED (4)

struct VERTEX;
struct FACE;

typedef struct FACES_BY_POINT
{
    struct FACES_BY_POINT* next;
    struct FACE* face;
    struct VERTEX* v;
}FACES_BY_POINT;

typedef struct FPXYZ
{
    double X; //World X, 24.8
    double Y; //World Y, 24.8
    double Z; //World Z, 24.8
    uint32_t flags;
    uint32_t id;
    struct FPXYZ* next;
    FACES_BY_POINT* FbyP; //Faces that use this point (for hull compute)
}FPXYZ;

#define VERTEXF_SCOORD_NOT_VALID (1)

//#define VERTEXF_ALREADY_TESTED (1)
#define VERTEXF_ALREADY_IN_Q (2)

typedef struct VERTEX
{
    struct VERTEX* next; //Следующая вершина (для обхода)
    struct VERTEX* prev; //Предыдущая вершина (для обхода)
    //FPXYZ* point; //Указатель на исходную точку
    int point_id; //Пригодится для обратного преобразования
    int TVn;
    double X;
    double Y;
    double Z;
    double U;
    double V;
    double x;
    double y;
    double z;
    double sx;
    double sy;
    //Владелец
    struct FACE* face;
    uint32_t flags;
}VERTEX;

typedef struct RVERTEX
{
    struct RVERTEX* next;
    struct RVERTEX* prev;
    double sx;
    double sy;
    double x;
    double y;
    double z;
    uint16_t flags;
}RVERTEX;


//Необходимо рисовать с полноценным рендером
#define FACEF_FULL_RENDER (1)

#define FACEF_ALREADY_IN_Q (16)
#define FACEF_DROPPED (32)

#define FACEF_DUMPED (64)

struct CUBE;

typedef struct FACE
{
    uint32_t id;
    struct FACE* next;
    struct FACE* next2;
    struct FACE* next_tri;
    uint16_t flags;
    VERTEX *v;
    int T;
    struct CUBE *portal_to_cube;
    //Коэффициенты уравнения плоскости. Кстати, A..C - заодно и вектор нормали
    double A; //16.16
    double B;
    double C;
    double D; //24.8
    //Для простоты, сразу посчитаем среднюю точку грани для вычисления хуллов
    double MX;
    double MY;
    double MZ;
    double R; //Радиус сферы, в которой лежит грань. Чтобы точно не проверять дальние на пересечение
    struct FACE* parent_face;
    //struct CUBE* parent_cube; //Куб-родитель
    //struct FACE* paired_portal; //Парный портал
    int seq;
    int side_hit;
    int32_t avg_z;
    uint16_t leaf_idx[16];
    int leaf_cnt;
    int N; //Количество вертексов, чтобы быстре принимать решение, бить ли квады на треугольники
}FACE;

typedef struct FACEQ
{
    struct FACEQ* next;
    FACE* f;
}FACEQ;

/*
typedef struct
{
    double x;
    double y;
    double z;
}VEC3;
*/

typedef double VEC3[3];

typedef struct PVS
{
    struct PVS* next;
    CUBE* c;
    FACE* f;
}PVS;

//============= rendinit.cpp ======================
void RenderAllocMemory(void);
int ComputeABCD(FACE* face);
FACE* AllocFace(void);
void FreeFace(FACE* face);
VERTEX* AllocVertex();
VERTEX* CreateVertex(double X, double Y, double Z, double U, double V);
void CreateWorld(void);
void CreateWorldFromFile(void);

void LoadObj(void);

//Эти две функции мы используем в разбиении граней при рендере с учетом перспективы
VERTEX* CreateVertexFast(double X, double Y, double Z, double U, double V);
void DestroyVertexFast(VERTEX* v);

#endif