/*
 * Copyright 2020, David Schmenk
 * Released under GPL 2.0 (www.gnu.org)
 */
#include <dos.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <time.h>
#include "gfx.h"
#include "keyboard.h"
#include "sound.h"

#define FALSE   0
#define TRUE    1
//
// Screen coordinates
//
#define X_PIXELS        320
#define Y_PIXELS        200
#define X_ORG           (X_PIXELS/2)
#define Y_ORG           (Y_PIXELS/2)
//
// Line drawing operation
//
void (*linestyle)(int x1, int y1, int x2, int y2);
//
// Fixed point sin/cos table
//
int SinTab[32] = {0};
int CosTab[32] = {16384};
//
// Ship coordinates
//
#define PLAYER_WIDTH    15
#define PLAYER_HEIGHT   15
#define X_PLAYER_ORG    (PLAYER_WIDTH/2)
#define Y_PLAYER_ORG    (PLAYER_HEIGHT/2)
#define MAX_SPEED       64//512
int xship[32][4];
int yship[32][4];
unsigned char aplayer = 0;
int xplayer  = X_ORG;
int yplayer  = Y_ORG;
int xsplayer = 0;
int ysplayer = 0;
//
// Covoid coordinates/bitmaps
//
#define COVOID_WIDTH    24
#define COVOID_HEIGHT   24
#define X_COVOID_ORG    (COVOID_WIDTH/2)
#define Y_COVOID_ORG    (COVOID_HEIGHT/2)
#define COVOID_DIST_SQR 169
#define MAX_COVOIDS     16
unsigned char covoid[] =
{
    0x00, 0x18, 0x00,
    0x00, 0x3C, 0x00,
    0x08, 0x18, 0x10,
    0x18, 0xFF, 0x18,
    0x3F, 0xFF, 0xFC,
    0x0F, 0xFF, 0xF0,
    0x0F, 0xFF, 0xF0,
    0x0F, 0xFF, 0xF0,
    0x1F, 0xFF, 0xF8,
    0x1F, 0xFF, 0xF8,
    0x5F, 0xFF, 0xFA,
    0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF,
    0x5F, 0xFF, 0xFA,
    0x1F, 0xFF, 0xF8,
    0x1F, 0xFF, 0xF8,
    0x0F, 0xFF, 0xF0,
    0x0F, 0xFF, 0xF0,
    0x0F, 0xFF, 0xF0,
    0x3F, 0xFF, 0xFC,
    0x18, 0xFF, 0x18,
    0x08, 0x18, 0x10,
    0x00, 0x3C, 0x00,
    0x00, 0x18, 0x00
};
#define NUCLEUS_WIDTH    16
#define NUCLEUS_HEIGHT   16
#define X_NUCLEUS_ORG    (NUCLEUS_WIDTH/2)
#define Y_NUCLEUS_ORG    (NUCLEUS_HEIGHT/2)
unsigned char nucleus[] =
{
    0x03, 0xC0,
    0x0F, 0xF0,
    0x1F, 0xF8,
    0x3F, 0xFC,
    0x7F, 0xFE,
    0x7F, 0xFE,
    0xFF, 0xFF,
    0xFF, 0xFF,
    0xFF, 0xFF,
    0xFF, 0xFF,
    0x7F, 0xFE,
    0x7F, 0xFE,
    0x3F, 0xFC,
    0x1F, 0xF8,
    0x0F, 0xF0,
    0x03, 0xC0
};
int xcovoid[MAX_COVOIDS];
int ycovoid[MAX_COVOIDS];
int xscovoid[MAX_COVOIDS];
int yscovoid[MAX_COVOIDS];
int ccovoid[MAX_COVOIDS];
int cscovoid[MAX_COVOIDS];
unsigned char alive[MAX_COVOIDS], dying[MAX_COVOIDS];
//
// Missile coordinates
//
int shooting = 0;
int xmissile;
int ymissile;
int xsmissile;
int ysmissile;
//
// Sound sequences
//
struct s_tone tempo[] = {{110, 0, 18},
                          {88, 0, 18}};
struct s_tone pew[] = {{660, 0, 1},
                       {600,  0, 1},                        
                       {550,  0, 1},                        
                       {490,  0, 1},                        
                       {440,  0, 1}};
//
// Game state
//
unsigned char level      = 1;
unsigned char thrust     = FALSE;
unsigned char won        = 0;
unsigned char infected   = 0;
unsigned char recharging = 0;
//
// Generate rotated ship coordinates
//
void GenShip(void)
{
    double hexangle, c, s;
    int i, j;

    xship[0][0] =  X_PLAYER_ORG;   yship[0][0] =  0;
    xship[0][1] = -X_PLAYER_ORG;   yship[0][1] =  Y_PLAYER_ORG;
    xship[0][2] = -X_PLAYER_ORG/2; yship[0][2] =  0;
    xship[0][3] = -X_PLAYER_ORG;   yship[0][3] = -Y_PLAYER_ORG;
    //
    // Calculate first quadrant and reflect remaining quadrants (faster and looks better)
    //
    for (i = 1; i < 8; i++)
    {
        hexangle = (double)i * M_PI / 16.0;
        s = sin(hexangle);
        c = cos(hexangle);
        SinTab[i] = s * 16384.0; // 2.14 fixed point
        CosTab[i] = c * 16384.0; // 2.14 fixed point
        //
        // Slightly scale up non-90 degree rotated coordinates
        //
        s = s * 1.06;
        c = c * 1.06;
        for (j = 0; j < 4; j++)
        {
            xship[i][j] = (double)xship[0][j] * c - (double)yship[0][j] * s;
            yship[i][j] = (double)xship[0][j] * s + (double)yship[0][j] * c;
        }
    }
    for (i = 0; i < 8; i++)
    {
        SinTab[i+8] =  CosTab[i];
        CosTab[i+8] = -SinTab[i];
        for (j = 0; j < 4; j++)
        {
            xship[i+8][j] = -yship[i][j];
            yship[i+8][j] =  xship[i][j];
        }
    }
    for (i = 0; i < 16; i++)
    {
        SinTab[i+16] = -SinTab[i];
        CosTab[i+16] = -CosTab[i];
        for (j = 0; j < 4; j++)
        {
            xship[i+16][j] = -xship[i][j];
            yship[i+16][j] = -yship[i][j];
        }
    }
}
//
// Generate virus locations
//
void GenCovoids(void)
{
    int i;

    randomize();
    for (i = 0; i < level; i++)
    {

        alive[i] = TRUE;
        do
        {
            xcovoid[i] = random(X_PIXELS - COVOID_WIDTH*2) + COVOID_WIDTH;
        } while (xcovoid[i] > X_ORG - PLAYER_WIDTH*2 && xcovoid[i] < X_ORG + PLAYER_WIDTH*2);
        do
        {
            ycovoid[i] = random(Y_PIXELS - COVOID_HEIGHT*2) + COVOID_HEIGHT;
        } while (ycovoid[i] > Y_ORG - PLAYER_HEIGHT*2 && ycovoid[i] < Y_ORG + PLAYER_HEIGHT*2);
        //
        // Make covoid coordinates and speed 14.2 fixed point
        //
        xcovoid[i] <<= 2;
        ycovoid[i] <<= 2;
        do
        {    
            xscovoid[i] = random(17) - 8;
            yscovoid[i] = random(17) - 8;
        } while (xscovoid[i] == 0 && yscovoid[i] == 0);
        //
        // Assign an initial color cycle
        //
        ccovoid[i]  = random(256);
        cscovoid[i] = (random(33) - 16);
        if (!cscovoid[i])
            cscovoid[i] = 1;
        if (ccovoid[i] < -cscovoid[i] || ccovoid[i] > 255-cscovoid[i])
            ccovoid[i] = 128 + cscovoid[i];
    }
}
void EraseObjects(void)
{
    int i;

    color(0, 0, 0);
    clear();
}
void DrawObjects(void)
{
    int i;
    char levelstr[20];

    //
    // Draw covoids
    //
    for (i = 0; i < level; i++)
    {
        if (alive[i])
        {
            color(ccovoid[i], 255-ccovoid[i], 0);
            bitblt((xcovoid[i]>>2)-X_COVOID_ORG, (ycovoid[i]>>2)-Y_COVOID_ORG, COVOID_WIDTH, COVOID_HEIGHT, 0, 0, covoid, 3);
            color(128, 32, 64);
            bitblt((xcovoid[i]>>2)-X_NUCLEUS_ORG, (ycovoid[i]>>2)-Y_NUCLEUS_ORG, NUCLEUS_WIDTH, NUCLEUS_HEIGHT, 0, 0, nucleus, 2);
            ccovoid[i] += cscovoid[i];
            if (ccovoid[i] <= -cscovoid[i] || ccovoid[i] >= 255-cscovoid[i])
                cscovoid[i] = -cscovoid[i];
        }
        else if (dying[i])
        {
            color(dying[i], dying[i], dying[i]);
            bitblt((xcovoid[i]>>2)-X_COVOID_ORG, (ycovoid[i]>>2)-Y_COVOID_ORG, COVOID_WIDTH, COVOID_HEIGHT, 0, 0, covoid, 3);
            dying[i] -= 16;
        }
    }
    //
    // Draw spacechip
    //
    color(0, 128-recharging, 255);
    linestyle(xplayer+xship[aplayer][0], yplayer+yship[aplayer][0], xplayer+xship[aplayer][1], yplayer+yship[aplayer][1]);
    linestyle(xplayer+xship[aplayer][3], yplayer+yship[aplayer][3], xplayer+xship[aplayer][0], yplayer+yship[aplayer][0]);
    if (thrust)
        color(255, 0, 0);
    linestyle(xplayer+xship[aplayer][1], yplayer+yship[aplayer][1], xplayer+xship[aplayer][2], yplayer+yship[aplayer][2]);
    linestyle(xplayer+xship[aplayer][2], yplayer+yship[aplayer][2], xplayer+xship[aplayer][3], yplayer+yship[aplayer][3]);
    //
    // Draw missile
    //
    if (shooting)
    {
        color(shooting, 0, 0);
        linestyle(xmissile, ymissile, xmissile - xsmissile, ymissile - ysmissile);
    }
    //
    // Print any messages
    //
    if (infected)
    {
        color(0xFF, 0, 0);
        text(100, 100, "You are infected!");
        SoundPlaySeq(0, NULL);
        SoundBackgroundLoop(0, NULL);
    }
    else if (won)
    {
        color(128, 255, 128);
        if (level < MAX_COVOIDS)
        {
            sprintf(levelstr, "Level: %d", level + 1);
            text(136, 100, levelstr);
        }
        else
            text(100, 100, "You beat the virus!");
        SoundPlaySeq(0, NULL);
        SoundBackgroundLoop(0, NULL);
    }
    //
    // Update the display with the off screen buffer
    //
    flip();
}
void UpdateObjects(void)
{
    int i;

    //
    // Update covoids
    //
    for (i = 0; i < level; i++)
    {
        if (alive[i] || dying[i])
        {
            xcovoid[i] += xscovoid[i];
            if (xcovoid[i] >= (X_PIXELS - X_COVOID_ORG - 1)<<2)
                xcovoid[i] = X_COVOID_ORG<<2;
            else if (xcovoid[i] <= X_COVOID_ORG<<2)
                xcovoid[i] = (X_PIXELS - X_COVOID_ORG - 1)<<2;
            ycovoid[i] += yscovoid[i];
            if (ycovoid[i] >= (Y_PIXELS - Y_COVOID_ORG - 1)<<2)
                ycovoid[i] = Y_COVOID_ORG<<2;
            else if (ycovoid[i] <= Y_COVOID_ORG<<2)
                ycovoid[i] = (Y_PIXELS - Y_COVOID_ORG - 1)<<2;
        }
    }
    //
    // Update player
    //
    xplayer = ((xplayer << 5) + xsplayer) >> 5;
    if (xplayer >= X_PIXELS - X_PLAYER_ORG*1.5)
        xplayer = X_PLAYER_ORG*1.5;
    else if (xplayer <= X_PLAYER_ORG*1.5)
        xplayer = X_PIXELS - X_PLAYER_ORG*1.5;
    yplayer = ((yplayer << 5) + ysplayer) >> 5;
    if (yplayer >= Y_PIXELS - Y_PLAYER_ORG*1.5)
        yplayer = Y_PLAYER_ORG*1.5;
    else if (yplayer <= Y_PLAYER_ORG*1.5)
        yplayer = Y_PIXELS - Y_PLAYER_ORG*1.5;
    if (!infected)
    {
        if (KeyboardGetKey(SCAN_KP_4) || KeyboardGetKey(SCAN_LEFT_ARROW))
        {
            //
            // Rotate left
            //
            aplayer = (aplayer - 1) & 31;
        }
        if (KeyboardGetKey(SCAN_KP_6) || KeyboardGetKey(SCAN_RIGHT_ARROW))
        {
            //
            // Rotate right
            //
            aplayer = (aplayer + 1) & 31;
        }
        if (KeyboardGetKey(SCAN_KP_8) || KeyboardGetKey(SCAN_UP_ARROW))
        {
            //
            // Thrust
            //
            thrust = TRUE;
            xsplayer += CosTab[aplayer] >> 9;
            ysplayer += SinTab[aplayer] >> 9;
            if (xsplayer > MAX_SPEED)
                xsplayer = MAX_SPEED;
            else if (xsplayer < -MAX_SPEED)
                xsplayer = -MAX_SPEED;
            if (ysplayer > MAX_SPEED)
                ysplayer = MAX_SPEED;
            else if (ysplayer < -MAX_SPEED)
                ysplayer = -MAX_SPEED;
        }
        else
            thrust = FALSE;
        if (recharging)
            recharging -= 8;
        if (KeyboardGetKey(SCAN_SPACE) && !recharging)
        {
            //
            // Fire missile
            //
            SoundPlaySeq(sizeof(pew)/sizeof(struct s_tone), &pew[0]);
#if 0
            xsmissile  = ((CosTab[aplayer] >> 6) + xsplayer) >> 5; // Determines missile speed
            ysmissile  = ((SinTab[aplayer] >> 6) + ysplayer) >> 5; // and direction
#else
            xsmissile  = CosTab[aplayer] >> 11; // Determines missile speed
            ysmissile  = SinTab[aplayer] >> 11; // and direction
#endif
            xmissile   = xplayer + (xsmissile >> 1);
            ymissile   = yplayer + (ysmissile >> 1);
            shooting   = 255; // Determines missile range
            recharging = 128; // Determines recharge time
        }
    }
    else
        infected++;
    if (shooting)
    {
        shooting -= 16; // This will fade the missile color over time
        if (shooting > 0)
        {
            //
            // Still shooting
            //
            xmissile += xsmissile;
            if (xmissile < 0)
                xmissile = X_PIXELS + xsmissile;
            else if (xmissile >= X_PIXELS)
                xmissile = xsmissile;
            ymissile += ysmissile;
            if (ymissile < 0)
                ymissile = Y_PIXELS + ysmissile;
            else if (ymissile >= Y_PIXELS)
                ymissile = ysmissile;
        }
        else
            shooting = 0;
    }
}
void HitObjects(void)
{
    int i, dx, dy;

    if (shooting)
    {
        //
        // Hit test missile with covoids
        //
        for (i = 0; i < level; i++)
        {
            if (alive[i])
            {
                dx = abs(xmissile - (xcovoid[i]>>2));
                dy = abs(ymissile - (ycovoid[i]>>2));
                if (dx <= X_COVOID_ORG && dy <= Y_COVOID_ORG)
                {
                    if (dx*dx+dy*dy < COVOID_DIST_SQR)
                    {
                        alive[i] = FALSE;
                        dying[i] = 128;
                        shooting = 0;
                    }
                }
            }
        }
    }
    //
    // Hit test player with covoids
    //
    if (!won)
    {
        won = 1;
        for (i = 0; i < level; i++)
        {
            if (alive[i])
            {
                won = 0;
                dx = abs(xplayer - (xcovoid[i]>>2));
                dy = abs(yplayer - (ycovoid[i]>>2));
                if (dx <= X_COVOID_ORG && dy <= Y_COVOID_ORG)
                {
                    if (dx*dx+dy*dy < COVOID_DIST_SQR)
                    {
                        infected = 1;
                    }
                }
            }
        }
    }
    else
        won++;
}
int main(int argc, char *argv[])
{
    int aa, mode, music, frameskip, i;
    t_timer throttle;

    //
    // Graphics setup
    //
    aa     = 0;
    mode   = MODE_BEST;
    music  = TRUE;
    while (argc > 1 && argv[1][0] == '-')
    {
        if (argv[1][1] == 's')
            music = FALSE;
        else if (argv[1][1] == 'a')
            aa = argv[1][2] - '0';
        else if (argv[1][1] == 'd')
            switch (argv[1][2] - '0')
            {
                case 2: mode = (mode & (MODE_MONO|MODE_NODITHER)) | MODE_2BPP;
                    break;
                case 4: mode = (mode & (MODE_MONO|MODE_NODITHER)) | MODE_4BPP;
                    break;
                case 8: mode = (mode & (MODE_MONO|MODE_NODITHER)) | MODE_8BPP;
                    break;
            }
        else if (argv[1][1] == 'l')
        {
            level = toupper(argv[1][2]) < 'A' ? argv[1][2] - '0' : toupper(argv[1][2]) - 'A' + 10;
            if (level < 1)  level = 1;
            if (level > 15) level = 15;
        }
        else if (argv[1][1] == 'm')
            mode |= MODE_MONO;
        else if (argv[1][1] == 'n')
            mode |= MODE_NODITHER;
        argc--;
        argv++;
    }
    linestyle = aa ? aaline : line;
    gfxmode(mode);
    GenShip();
    //
    // Keyboard control
    //
    KeyboardInstallDriver();
    //
    // Sound setup
    //
    TimerInstall();
    SoundPrepSeq(sizeof(pew)/sizeof(struct s_tone), &pew[0]);
    //
    // Intro
    //
    render(FRONT_PAGE);
    clear();
    for (i = 0; i < 255; i+=4)
    {
        color(i, i, i);
        text(120, 90, "COVOIDS!");
        TimerDelay(1);
    }
    color(0, 0, 255);
    text(144, 110, "by");
    color(0, 255, 0);
    text(128, 120, "RESMAN");
    color(128, 128, 128);
    text(128, 140, "(Still)");
    text(24, 150, "Sheltering at Home Edition 1.05");
    TimerDelay(48);
    //
    // Main game loop
    //
    do
    {
        won          = 0;
        xplayer      = X_ORG;
        yplayer      = Y_ORG;
        xsplayer     = 0;
        ysplayer     = 0;
        recharging   = 0;
        shooting     = 0;
        thrust       = FALSE;
        frameskip    = 1;
        GenCovoids();
        color(0, 0, 0);
        render(FRONT_PAGE);
        clear();
        render(BACK_PAGE);
        clear();
        tempo[0].Frequency  = 110 + level * 11;
        tempo[0].Duration   = 18  - level;
        tempo[1].Frequency  = 88  + level * 11;
        tempo[1].Duration   = 18  - level;
        SoundPrepSeq(sizeof(tempo)/sizeof(struct s_tone), &tempo[0]);
        if (music)
            SoundBackgroundLoop(sizeof(tempo)/sizeof(struct s_tone), &tempo[0]);
        do
        {
            throttle = TimerCount();
            EraseObjects();
            while (frameskip--)
            {
                UpdateObjects();
                HitObjects();
            }
            DrawObjects();
            while ((frameskip = TimerCount() - throttle) == 0);
            if (frameskip < 0)
                frameskip = TimerCount();
        } while (won < 40 && infected < 40 && !KeyboardGetKey(SCAN_ESC));
        SoundPlaySeq(0, NULL);
        level++;
    } while (won && level <= MAX_COVOIDS);
    //
    // Restore H/W state
    //
    TimerUninstall();
    KeyboardUninstallDriver();
    restoremode();
    puts("Help flatten the curve:");
    puts("Remember to wash your hands and practice physical distancing.");
    puts("We got this...");
    return 0;
}


