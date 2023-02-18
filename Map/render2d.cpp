#define _CRT_SECURE_NO_WARNINGS
#include "stdio.h"
#include "render.h"

//uint8_t ChunkScreen[SCRWIDTH * SCRHEIGHT];

uint8_t ChunkScreenR[128 * 180];

uint32_t PALLETTE[MAX_COLORS];
void InitRender2D(void)
{
    //FILE* f = fopen("pal64.gpl", "wt");
    //fprintf(f, "GIMP Palette\nName : palette64\n#\n");
    for (int i = 0; i < MAX_COLORS; i++)
    {
        uint32_t color = 0xFF000000;
#if 1
        if (i & 0x20) color |= 0x00AA0000;
        if (i & 0x10) color |= 0x00550000;
        if (i & 0x08) color |= 0x0000AA00;
        if (i & 0x04) color |= 0x00005500;
        if (i & 0x02) color |= 0x000000AA;
        if (i & 0x01) color |= 0x00000055;
#else
        color |= (i << 18) | (i << 10) | (i << 2);
#endif
        PALLETTE[i] = color;
        //fprintf(f, "%d %d %d\n", (color >> 16) & 0xFF, (color >> 8) & 0xFF, (color >> 0) & 0xFF);
    }
    //fclose(f);
}

static void Plot(FIXP32 x, FIXP32 y, uint16_t color)
{
    if (y < 0) return;
    if (y > 127) return;
    if (x < 0) return;
    if (x > 179) return;
    if (color & 0xFF00)
        ChunkScreenR[y + x * 128] ^= color & 0xFF;
    else
        ChunkScreenR[y + x * 128] = color & 0xFF;
}

//Рисуем простую линию (на входе 24.8 уже в экранных координатах)
void DrawLine(FIXP32 x1, FIXP32 y1, FIXP32 x2, FIXP32 y2, uint16_t color)
{
    int dx,dy;
    x1 >>= 8;
    x2 >>= 8;
    y1 >>= 8;
    y2 >>= 8;
    dx = x2 - x1;
    dy = y2 - y1;
    int adx, ady;
    if (dx > 0) adx = dx; else adx = -dx;
    if (dy > 0) ady = dy; else ady = -dy;
    if (adx > ady)
    {
        //One pixel step by X
        if (adx)
        {
            dy <<= 8;
            y1 <<= 8;
            dy /= adx;
            while (x2 > x1)
            {
                int y = y1 >> 8;
                Plot(x1,y,color);
                y1 += dy;
                x1++;
            }
            while (x2 < x1)
            {
                int y = y1 >> 8;
                Plot(x1, y, color);
                y1 += dy;
                x1--;
            }
        }
    }
    else
    {
        //One pixel step by Y
        if (ady)
        {
            dx <<= 8;
            x1 <<= 8;
            dx /= ady;
            while (y2 > y1)
            {
                int x = x1 >> 8;
                Plot(x, y1, color);
                x1 += dx;
                y1++;
            }
            while (y2 < y1)
            {
                int x = x1 >> 8;
                Plot(x, y1, color);
                x1 += dx;
                y1--;
            }
        }
    }
}

#if 0
void DrawXorLine(FIXP32 x1, FIXP32 y1, FIXP32 x2, FIXP32 y2, uint8_t color)
{
    int dx, dy;
    x1 >>= 8;
    x2 >>= 8;
    y1 >>= 8;
    y2 >>= 8;
    dx = x2 - x1;
    dy = y2 - y1;
    int adx, ady;
    if (dx > 0) adx = dx; else adx = -dx;
    if (dy > 0) ady = dy; else ady = -dy;
    if (adx > ady)
    {
        //One pixel step by X
        if (adx)
        {
            dy <<= 8;
            y1 <<= 8;
            dy /= adx;
            while (x2 > x1)
            {
                int y = y1 >> 8;
                ChunkScreenR[y + x1 * 128] ^= color;
                y1 += dy;
                x1++;
            }
            while (x2 < x1)
            {
                int y = y1 >> 8;
                ChunkScreenR[y + x1 * 128] ^= color;
                y1 += dy;
                x1--;
            }
        }
    }
    else
    {
        //One pixel step by Y
        if (ady)
        {
            dx <<= 8;
            x1 <<= 8;
            dx /= ady;
            while (y2 > y1)
            {
                int x = x1 >> 8;
                ChunkScreenR[y1 + x * 128] ^= color;
                x1 += dx;
                y1++;
            }
            while (y2 < y1)
            {
                int x = x1 >> 8;
                ChunkScreenR[y1 + x * 128] ^= color;
                x1 += dx;
                y1--;
            }
        }
    }
}
#endif

uint32_t TPALLETTE[32];

void C2P(uint32_t *d)
{
    uint8_t* ss = ChunkScreenR;// +128 * SCREEN_LEFT + SCREEN_TOP;
    for (int y = 0; y < 128*4-1; y++)
    {
        uint8_t *s = ss;
        uint32_t prev_pix = 0xFF000000;
#ifdef CMODE256
        for (int x = 0; x < 180*4;)
#else
        for (int x = 0; x < 180 * 4;)
#endif
        {
            //ScreenBitmap->SetPixel(x, y, Color::FromArgb(PALLETTE[*s]));
            //d[x + y * 180 * 4] = c;
#ifdef CMODE256
            uint32_t c = s[(y >> 2) + (x >> 2) * 128];
            uint32_t c1 = (c >> 6) & 3;
            uint32_t c2 = (c >> 4) & 3;
            uint32_t c3 = (c >> 2) & 3;
            uint32_t c4 = (c >> 0) & 3;
            if (c2 == 2 && c3 == 0) c2 = 1;

            c1 *= 0x550000;
            c2 *= 0x5500;
            c3 *= 0x5500;
            c4 *= 0x55;
            if (y & 2) x++;
            prev_pix = (prev_pix & 0xFF00FFFF) | c1;
            d[x + y * 180 * 4] = prev_pix; x++;
            prev_pix = (prev_pix & 0xFFFF00FF) | c2;
            d[x + y * 180 * 4] = prev_pix; x++;
            prev_pix = (prev_pix & 0xFFFF00FF) | c3;
            d[x + y * 180 * 4] = prev_pix; x++;
            prev_pix = (prev_pix & 0xFFFFFF00) | c4;
            d[x + y * 180 * 4] = prev_pix; x++;
            if (y & 2) x--;
#else
#ifdef CMODE_PAL32
            uint32_t c = TPALLETTE[s[(y >> 2) + (x >> 2) * 128]];
            c |= 0xFF000000;
#else
            uint32_t c = PALLETTE[s[(y >> 2) + (x >> 2) * 128]];
#endif
#if 0
            prev_pix = (prev_pix & 0xFF00FFFF) | (c & 0x00FF0000);
            d[x + y * 180 * 4] = prev_pix; x++;
            prev_pix = (prev_pix & 0xFFFF00FF) | (c & 0x0000FF00);
            d[x + y * 180 * 4] = prev_pix; x++;
            prev_pix = (prev_pix & 0xFFFFFF00) | (c & 0x000000FF);
            d[x + y * 180 * 4] = prev_pix; x++;
            prev_pix = (prev_pix & 0xFFFFFF00) | (c & 0x000000FF);
            d[x + y * 180 * 4] = prev_pix; x++;
#else
#if 0
            if (y & 2)
            {
                uint32_t c2 = PALLETTE[s[(y >> 2) - 1 + (x >> 2) * 128]];
                c &= 0xAAAAAAAA;
                c2 >>= 1;
                c2 &= 0x55555555;
                c |= c2;
            }
#endif
#if 0
            if (y & 2)
            {
                c &= 0xFEFEFEFE;
                c >>= 1;
                c |= 0x80000000;
            }
#endif
            d[x + y * 180 * 4] = c; x++;
            d[x + y * 180 * 4] = c; x++;
            d[x + y * 180 * 4] = c; x++;
            d[x + y * 180 * 4] = c; x++;
#endif
#endif
            //if ((x & 3) == 3) s++;
        }
        //if ((y & 3) == 3) ss += SCRWIDTH;
    }
}

