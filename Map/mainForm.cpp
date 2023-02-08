#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include "mainForm.h"
#include <stdio.h>
#include "render.h"
#include "renderutils.h"

using namespace E3D;

[System::STAThread]
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    RenderAllocMemory();
    InitRender2D();
    InitRender3D();
    //LoadSpyro();
#ifndef NO_PLAYER
    CreatePlayerSprite();
#endif
    LoadOmni();
    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(false);
    Application::Run(gcnew mainForm);
    return 0;
}



FIXP32 CamDX, CamDY, CamDZ;

void mainForm::PrintCEdistances(void)
{
    char str[1024];
    DebugCEdistances(str);
    lblCEdistances->Text = gcnew String(str);
}

#define RAD2G (57.29577951f)

int VisibleCubeDirection;
int LastCubeDirection;

FIXP32 dROLL2;

void mainForm::PrintPosition(void)
{
    char str[1024];
    float pitch, roll, yaw;

    pitch = -asinf(RMTXp[5]/16384.0f) * RAD2G;
    roll = atan2f((float)RMTXp[3], (float)RMTXp[4]) * RAD2G;
    yaw = atan2f((float)RMTXp[6], (float)RMTXp[8]) * RAD2G;
    char* s = str;
    s += sprintf(s,"pitch=%f\n", pitch);
    s += sprintf(s,"roll=%f\n", roll);
    s += sprintf(s, "yaw=%f\n", yaw);
#if 0
    if (pitch > 45.0f)
    {
        VisibleCubeDirection = 4;
    }
    else if (pitch < -45.0f)
    {
        VisibleCubeDirection = 5;
    }
    else if (yaw > -45.0f && yaw <= 45.0f)
    {
        VisibleCubeDirection = 1;
    }
    else if (yaw > 45.0f && yaw <= 135.0f)
    {
        VisibleCubeDirection = 0;
    }
    else if (yaw > -135.0f && yaw <= -45.0f)
    {
        VisibleCubeDirection = 2;
    }
    else
    {
        VisibleCubeDirection = 3;
    }
    s += sprintf(s, "vcd=%d, lcd=%d\n", VisibleCubeDirection, LastCubeDirection);
#endif
    *s = 0;
    FIXP32 r;
    r = RMTXp[3];
    //if (RMTXp[4] < 0) r = -r;
    r >>= 4;
    if (r > 0x100) r = 0x100; else if (r < -0x100) r = -0x100;
    //if (r<10 && r>-10) r = 0;
    dROLL2 = -r;
    //dROLL2 = -roll / 10000.0f;
    lblPosition->Text = gcnew String(str);
}

FIXP32 dROLL = 0;

int mouse_moved;

FIXP32 PSpeedY;

static int AccumulatedMDX;
static int AccumulatedMDY;

#pragma optimize("t",on)
void mainForm::TimerProc(void)
{
    //RotateMTX(RMTXp, x * 32, y * 32, -x * 8);
    FIXP32 s, c;
    s = 0;
    c = 0;
#if 0
    static float a;
    if (1/*!mouse_moved*/)
    {
        s = (FIXP32)(sinf(a/* + 1.57f / 2.0f*/) * 150.f);
        c = (FIXP32)(cosf(a * 2.0f) * 150.f);

        a = a + .1f; if (a > M_PI * 2.0f) a -= M_PI * 2.0f;

    }
    else
    {
        mouse_moved = 0;
    }
#endif


    RotateMTX(RMTXp, AccumulatedMDX * 32 + s, AccumulatedMDY * 32 + c, dROLL + (dROLL==0?dROLL2:0) - AccumulatedMDX * 8);
    AccumulatedMDX = 0;
    AccumulatedMDY = 0;
#ifdef NO_PLAYER
    NormalizeMTX(RMTXp);
    //NormalizeMTX(RMTXp);
    memcpy(RMTX, RMTXp, sizeof(RMTX));
#else
    float d;
    d = (CamX - PlayerX) * RotMatrix[0] +
        (CamY - PlayerY) * RotMatrix[3] +
        (CamZ - PlayerZ) * RotMatrix[6];
    UpdateRotMatrix(-d/10000.0f, -(RotMatrix[5]-0.5f)/10.0f, -RotMatrix[3]/10.0f);
#endif
#ifdef NO_PLAYER
    MovePlayer(CamDX, CamDY, CamDZ);
    UpdateIMTX();
#else
    MovePlayer(CamDX, CamDY+PSpeedY, CamDZ);
    PSpeedY += Player_G;
#endif
    {
        int x = 0;
        do
        {
            //memset(ChunkScreenR + x * 128, 0, 128);
            memset(ChunkScreenR + x * 128 + 0, 0x15, 14);
            memset(ChunkScreenR + x * 128 + 14, 0, 100);
            memset(ChunkScreenR + x * 128 + 114, 0x15, 14);
        } while ((++x) < 10);
        do
        {
            memset(ChunkScreenR + x * 128 + 0, 0, 14);
            memset(ChunkScreenR + x * 128 + 14, SKY_COLOR2, 100);
            memset(ChunkScreenR + x * 128 + 114, 0, 14);
        } while ((++x) < 170);
        do
        {
            //memset(ChunkScreenR + x * 128, 0, 128);
            memset(ChunkScreenR + x * 128 + 0, 0x15, 14);
            memset(ChunkScreenR + x * 128 + 14, 0, 100);
            memset(ChunkScreenR + x * 128 + 114, 0x15, 14);
        } while ((++x) < 180);
    }
#if 0
    for (int y = 0; y < 14; y++)
    {
        for (int x = 0; x < 13*8; x++)
        {
            int c = x >> 3;
            int gc = c;
            static int green_t[16] =
            {
#if 1
                0,16,32,4,
                20,36,8,24,
                40,12,28,44,
                60,60,60,60
#else
                0,16,16,4,
                20,/*32,*/36,8,24,
                40,12,28,44,
                60,60,60,60
#endif
            };
            gc = green_t[c];
            ChunkScreenR[y + (x + 10) * 128] = gc;
        }
    }
#endif
    //memset(ChunkScreenR, 22, sizeof(ChunkScreenR));
    ProcessWorld();
    //Convert chunky screen;
    System::Drawing::Imaging::BitmapData^ bdata;
    bdata = ScreenBitmap->LockBits(
        System::Drawing::Rectangle(0, 0, ScreenBitmap->Width, ScreenBitmap->Height),
        System::Drawing::Imaging::ImageLockMode::ReadWrite,
        ScreenBitmap->PixelFormat);
    BitmapArray = bdata->Scan0;
    C2P((uint32_t*)(BitmapArray.ToPointer()));
    ScreenBitmap->UnlockBits(bdata);
    pictureBox1->Image = ScreenBitmap;
    PrintCEdistances();
    PrintPosition();
}

void mainForm::SetLockMouse(int lock)
{
    if (lock)
    {
        if (!LockMouse)
        {
            LockMouse = 1;
            ShowCursor(0);
            ResetMouse();
        }
    }
    else
    {
        if (LockMouse)
        {
            LockMouse = 0;
            ShowCursor(1);
        }
    }
}

void JumpToCubeCenter(void)
{
}

#define CAM_STEP (25)

void mainForm::ProcessKeyPress(int ch, int f, int _shift, int _ctrl, int _alt)
{
    switch (ch)
    {
    case 'W':
        if (f)
            CamDZ = CAM_STEP;
        else if (CamDZ>0)
            CamDZ = 0;
        break;
    case 'S':
        if (f && _ctrl)
        {
            //Square split
            break;
        }
        if (f)
            CamDZ = -CAM_STEP;
        else if (CamDZ<0)
            CamDZ = 0;
        break;
    case 'A':
        if (f)
            CamDX = -CAM_STEP;
        else if (CamDX<0)
            CamDX = 0;
        break;
    case 'D':
        if (_ctrl)
        {
            if (f) DumpRequest = 1;
            break;
        }
        if (f)
            CamDX = CAM_STEP;
        else if (CamDX>0)
            CamDX = 0;
        break;
    case 'Z':
        if (f)
            CamDY = -CAM_STEP;
        else if (CamDY < 0)
            CamDY = 0;
        break;
    case 'X':
        if (f)
            CamDY = CAM_STEP;
        else if (CamDY > 0)
            CamDY = 0;
        break;
    case 'Q':
        if (f)
            dROLL = -1024;
        else if (dROLL<0)
            dROLL = 0;
        break;
    case 'E':
        if (f)
            dROLL = 1024;
        else if (dROLL>0)
            dROLL = 0;
        break;
    case 'C':
        if (f)
        {
            JumpToCubeCenter();
        }
        break;
    case 0x1B:
        SetLockMouse(0);
        break;
    case 'I':
        if (f)
        {
            display_wires ^= 1;
        }
        break;
    case 'U':
        if (f)
        {
            test_mode ^= 1;
        }
        break;
    case 'Y':
        if (f)
        {
            test_mode2 ^= 1;
        }
        break;
#if 0
    case 0x25: //Left
        if (f) StepRootCube(0, _ctrl);
        break;
    case 0x26: //Up
        if (f) StepRootCube(1, _ctrl);
        break;
    case 0x27: //Right
        if (f) StepRootCube(2, _ctrl);
        break;
    case 0x28: //Down
        if (f) StepRootCube(3, _ctrl);
        break;
    case 'N':
        if (f) StepRootCube(4, _ctrl);
        break;
    case 'M':
        if (f) StepRootCube(5, _ctrl);
        break;
    case 'L':
        if (f) LinkRootCubeWithNearest();
        break;
    case 'P':
        if (f)
        {
            StepRootCube(VisibleCubeDirection, _ctrl);
            JumpToCubeCenter();
        }
        break;
    case 'F':
        if (f && _ctrl)
        {
            while (!RootCube->tfaces[0])
            {
                CUBE* c;
                c = RootCube;
                if (c->MX > 0x4000) break;
                if (c->MX < -0x4000) break;
                if (c->MY > 0x0000) break;
                if (c->MY < -0x2000) break;
                if (c->MZ > 0x4000) break;
                if (c->MZ < -0x4000) break;
                StepRootCube(VisibleCubeDirection, 1);
                JumpToCubeCenter();
                LinkRootCubeWithNearest();
            }
        }
        break;
    case 'J':
        RootCube = FindNearestCube(CamX, CamY, CamZ);
        break;
#endif
    }
}

static uint8_t mouse_fuse = 1;

void mainForm::ProcessMouse(int x, int y, int buttons)
{
    char str[256];
    char* s = str;
    s += sprintf(s, "%04d %04d", x, y);
    *s = 0;
    int dx, dy;
    static int old_x, old_y;
    dx = x - old_x;
    dy = y - old_y;
    old_x = x;
    old_y = y;
    lblMouse->Text = gcnew String(str);
    if (!LockMouse) return;
    x -= pictureBox1->Width / 2;
    y -= pictureBox1->Height / 2;
    if (x != 0 || y != 0) mouse_moved = 1;
    if (mouse_fuse)
    {
        mouse_fuse = 0;
    }
    else
    {
#ifdef NO_PLAYER
        //RotateMTX(RMTXp, x * 32, y * 32, -x * 8);
        AccumulatedMDX += x;
        AccumulatedMDY += y;
#else
        UpdateRotMatrixP((float)x / 1000.0f, /*(float)y / 1000.0f*/0.0f, 0.0f);
#endif
    }
    if (x != 0 || y != 0)
    {
        Point p;
        p.X = 0;
        p.Y = 0;
        p = pictureBox1->PointToScreen(p);
        SetCursorPos(p.X + pictureBox1->Width / 2, p.Y + pictureBox1->Height / 2);
    }
}

void mainForm::ResetMouse(void)
{
    Point p;
    p.X = 0;
    p.Y = 0;
    p = pictureBox1->PointToScreen(p);
    mouse_fuse = 1;
    SetCursorPos(p.X + pictureBox1->Width / 2, p.Y + pictureBox1->Height / 2);
}

void mainForm::FlyByWheel(int move)
{
    MovePlayer(0, 0, move / 1);
}
