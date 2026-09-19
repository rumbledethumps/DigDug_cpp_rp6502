// Game data tables transcribed from the Atari 7800 Dig Dug assembly source.
#ifndef TABLES_H
#define TABLES_H
#include <stdint.h>

#define DDSTRTX 96
#define DDSTRTY 191
#define DDMIDLX 56
#define DDMIDLY 107
#define MINDDX 8
#define MAXDDX 112
#define MINDDY 23
#define MAXDDY 191

#define FULLDIRT 36
#define FULLSKY 30
#define DTOPEND 0x16
#define DVMIDDLE 0x14
#define DBOTEND 0x1C
#define DLEFTEND 0x1A
#define DHMIDDLE 0x0A
#define DRHTEND 0x0E
#define BIGFLOWR 32
#define SMLFLOWR 34
#define BLANKR 74
#define BLANKL 76
#define LSKYHALF 140
#define RSKYHALF 136
#define CHARDF 0x1E
#define CHARD12 0x24

#define DHFAST 0xAAD5
#define DHSLOW 0x5252
#define DIGFAST 0xF7
#define DIGSLOW 0x5B
#define PROPORT_INIT 0xB5

#define SQDIGDUG 160
#define ZBLANK 222
#define DPUMP6 172
#define SQPOOKA 164
#define SQFYGAR 162
#define STRTROCK 166
#define TOTRROCK 86
#define ONEDGE 2
#define TEDDY 50
#define NUMVEG 17
#define MAXEFFR 17
#define FULL0 150
#define NUMCREAT 7
#define NUMROCK 4
#define WHITE 0x0E

static const uint8_t TOPZONE[17] = {11, 23, 35, 47, 59, 71, 83, 95, 107, 119, 131, 143, 155, 167, 179, 191, 191};
static const uint8_t MAPLOW[16] = {0xEF, 0xE0, 0xD0, 0xC0, 0xB0, 0xA0, 0x90, 0x80, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00};

static const uint8_t SOUTH7[7] = {36, 38, 40, 42, 28, 46, 20};
static const uint8_t SOUTH1[7] = {44, 12, 48, 24, 44, 8, 48};
static const uint8_t NORTH7[7] = {36, 38, 22, 42, 44, 46, 20};
static const uint8_t NORTH1[7] = {40, 6, 40, 18, 48, 2, 48};
static const uint8_t WEST5[7] = {36, 38, 40, 26, 44, 10, 48};
static const uint8_t WEST1[7] = {42, 46, 18, 42, 24, 46, 16};
static const uint8_t EAST5[7] = {36, 14, 40, 42, 44, 10, 48};
static const uint8_t EAST1[7] = {38, 38, 6, 46, 12, 46, 4};
static const uint8_t VERTTOP[18] = {0, 2, 4, 6, 0, 2, 4, 6, 16, 18, 20, 22, 16, 18, 20, 30, 32, 34};
static const uint8_t VERTBOT[18] = {0, 0, 4, 4, 8, 8, 12, 12, 16, 16, 20, 20, 24, 24, 28, 30, 32, 34};
static const uint8_t HORRGT[18] = {0, 2, 4, 6, 8, 10, 12, 14, 0, 2, 4, 6, 8, 10, 12, 30, 32, 34};
static const uint8_t HORLFT[18] = {0, 2, 0, 2, 8, 10, 8, 10, 16, 18, 16, 18, 24, 26, 24, 30, 32, 34};

static const uint8_t* const HALFXFM[4] = {EAST5, WEST5, SOUTH7, NORTH7};
static const uint8_t* const FROMXFM[4] = {HORLFT, HORRGT, VERTTOP, VERTBOT};
static const uint8_t* const TOXFM[4] = {HORRGT, HORLFT, VERTBOT, VERTTOP};
static const uint8_t* const ENTRXFM[4] = {EAST1, WEST1, SOUTH1, NORTH1};

static const uint8_t VONE[4] = {1, 7, 10, 0};
static const uint8_t VBOUND[4] = {0, 0, 11, 11};
static const uint8_t VHALF[4] = {5, 3, 4, 6};
static const uint8_t VPRE1[4] = {7, 1, 0, 10};
static const uint8_t VPRE2[4] = {6, 2, 1, 9};
static const uint8_t VPRE3[4] = {6, 2, 2, 8};

static const int8_t XTOPL[20] = {8, 0, 0, 0, 8, -8, 0, 0, 3, -3, 0, 0, 16, -8, 0, 0, 7, -7, 0, 0};
static const int8_t YTOPL[20] = {0, 0, -12, 0, 0, 0, -12, 12, 12, 12, 0, 17, 0, 0, -24, 12, 12, 12, 0, 23};
static const int8_t XFROMPL[4] = {0, 8, 0, 0};
static const int8_t YFROMPL[4] = {0, 0, 0, -12};
static const uint8_t CVTDIR[4] = {2, 1, 4, 3};

static const uint8_t RDIGDUG[3] = {0, 2, 4};
static const uint8_t LDIGDUG[3] = {6, 8, 10};
static const uint8_t DANIMS[12] = {0, 6, 154, 148, 2, 8, 156, 150, 4, 10, 158, 152};
static const uint8_t ADEATH[10] = {222, 106, 112, 110, 108, 222, 106, 118, 116, 114};
static const uint8_t DANIMIX[4] = {4, 9, 9, 4};
static const uint8_t HARPSTMP[4] = {16, 18, 122, 120};
static const int8_t HARPXPL[4] = {2, -2, 0, 0};
static const int8_t HARPYPL[4] = {0, 0, -4, 4};

static const uint8_t RPUMP[6] = {53, 52, 51, 50, 49, 48};
static const uint8_t LPUMP[6] = {69, 67, 64, 60, 55, 46};
static const uint8_t PDOWNS0[6] = {172, 173, 174, 175, 174, 175};
static const uint8_t PDOWNS1[6] = {222, 222, 172, 173, 174, 175};
static const uint8_t PDOWNS2[6] = {222, 222, 222, 222, 172, 173};
static const uint8_t PUPS0[6] = {176, 177, 177, 177, 177, 177};
static const uint8_t PUPS1[6] = {222, 222, 104, 105, 105, 105};
static const uint8_t PUPS2[6] = {222, 222, 222, 222, 104, 105};

static const int8_t XTABLE[16] = {0, 0, -1, -1, 0, 0, -1, -1, 1, 1, 0, 0, 1, 1, 0, 0};
static const int8_t YTABLE[16] = {0, -1, 0, -1, 1, 0, 1, 0, 0, -1, 0, -1, 1, 0, 1, 0};
static const uint16_t SPEEDTAB[8] = {0xEEEE, 0x9248, 0x7FFF, 0x954A, 0xF777, 0x8924, 0xFFFF, 0xA954};

static const uint8_t RPOOKA_SEQ[8] = {126, 126, 124, 124, 124, 124, 124, 124};
static const uint8_t LPOOKA_SEQ[8] = {130, 130, 128, 128, 128, 128, 128, 128};
static const uint8_t GPOOKA_SEQ[4] = {140, 142, 142, 142};
static const uint8_t RFYGAR_SEQ[8] = {132, 132, 134, 134, 132, 132, 134, 134};
static const uint8_t LFYGAR_SEQ[8] = {136, 136, 138, 138, 136, 136, 138, 138};
static const uint8_t GFYGAR_SEQ[4] = {144, 146, 146, 146};
static const uint8_t* const SEQTAB[8] = {GPOOKA_SEQ, LPOOKA_SEQ, GFYGAR_SEQ, LFYGAR_SEQ,
                                         RPOOKA_SEQ, GPOOKA_SEQ, RFYGAR_SEQ, GFYGAR_SEQ};
static const uint8_t SEQLEN[8] = {4, 8, 4, 8, 8, 4, 8, 4};
static const uint8_t FLAMTABL[8] = {134, 138, 12, 14, 134, 138, 12, 14};
static const uint8_t RFLAMSEQ[4] = {20, 22, 24, 28};
static const uint8_t LFLAMSEQ[4] = {21, 34, 36, 40};
static const uint8_t FLAMPOS[4] = {4, 8, 16, 24};
static const uint8_t BLWSTMPS[16] = {232, 234, 236, 238, 224, 226, 228, 230, 248, 250, 252, 254, 240, 242, 244, 246};
static const uint8_t BPOINTS[8] = {184, 98, 181, 178, 218, 187, 101, 98};
static const uint16_t BURSTPTS[8] = {500, 400, 300, 200, 1000, 800, 600, 400};
static const uint8_t MODETAB[8] = {MODE_DIAG, MODE_VERT, MODE_HORIZ, MODE_VERT, MODE_HORIZ, MODE_DIAG, MODE_VERT, MODE_HORIZ};
static const uint8_t WHNSPEED[5] = {2, 3, 4, 5, 6};
static const uint8_t CRYOFST[8] = {0, 12, 24, 12, 0, 12, 24, 12};
static const uint8_t CRXOFST[8] = {8, 16, 8, 0, 8, 16, 8, 0};

static const uint8_t RFALLTBL[7] = {1, 2, 2, 2, 1, 2, 2};
static const uint8_t RKANIM[2] = {168, 170};
static const uint8_t RKTIM[3] = {16, 16, 30};
static const uint8_t DISPTS[8] = {218, 190, 194, 198, 202, 206, 210, 214};
static const uint16_t FLATPTS[8] = {1000, 2500, 4000, 6000, 8000, 10000, 12000, 15000};

static const uint8_t FRUITAB[19] = {50, 52, 54, 56, 58, 58, 60, 60, 62, 62, 64, 64, 66, 66, 68, 68, 70, 70, 72};
static const uint8_t FPOINTS[19] = {178, 98, 101, 187, 218, 218, 70, 70, 74, 74, 194, 194, 78, 78, 198, 198, 82, 82, 202};
static const uint16_t VEGPTS[19] = {200, 400, 600, 800, 1000, 1000, 2000, 2000, 3000, 3000, 4000, 4000,
                                    5000, 5000, 6000, 6000, 7000, 7000, 8000};
static const uint8_t VEGCOL1[19] = {0xD2, 0x1E, 0x3F, 0xF4, 0xF4, 0x50, 0x50, 0xD4, 0xD4, 0x32, 0x32, 0x26, 0x26, 0x34, 0x34, 0x74, 0x74, 0xD2, 0x16};
static const uint8_t VEGCOL2[19] = {0x34, 0xD6, 0x22, 0xD6, 0xD6, 0x52, 0x52, 0x3A, 0x3A, 0xD2, 0xD2, 0x1E, 0x1E, 0xD8, 0xD8, 0x1A, 0x1A, 0x1A, 0x1A};
static const uint8_t VEGCOL3[19] = {0x38, 0xEE, 0x04, 0xFC, 0xFC, 0x58, 0x58, 0xE8, 0xE8, 0x36, 0x36, 0x16, 0x16, 0x1A, 0x1A, 0x54, 0x54, 0x24, 0x14};

static const uint8_t DIRTCOLR[12] = {0x1A, 0x27, 0x24, 0x10, 0x0A, 0x15, 0x1E, 0x27, 0x0A, 0x15, 0xB2, 0x04};
static const uint8_t PEBBCOLR[12] = {0x24, 0x14, 0x17, 0x26, 0x52, 0x42, 0x24, 0x10, 0x52, 0x42, 0x00, 0x52};
static const uint8_t IPALETTE[8][4] = {{0x00, 0x85, 0x0E, 0x00}, {0x00, 0x34, 0x1A, 0x0E},
                                       {0x00, 0x30, 0x00, 0x44}, {0x00, 0xD6, 0x24, 0x0E},
                                       {0x00, 0x1A, 0x85, 0xA0}, {0x00, 0x86, 0x86, 0x0E},
                                       {0x00, 0x1A, 0x85, 0x24}, {0x00, 0xD4, 0x86, 0x34}};

// tunnels: 8 entries per rack, 0 padded
static const uint8_t PTUNNELS[25][8] = {
    {0x2A, 0xA3, 0x22, 0x9A, 0, 0, 0, 0},
    {0x97, 0xBB, 0x3B, 0x62, 0x92, 0, 0, 0},
    {0x2B, 0x52, 0xC9, 0x38, 0x96, 0, 0, 0},
    {0x34, 0xA4, 0xC8, 0x38, 0x7C, 0, 0, 0},
    {0x32, 0x93, 0xAB, 0x36, 0x2A, 0x6B, 0, 0},
    {0x34, 0x34, 0x8A, 0x92, 0xB6, 0x53, 0, 0},
    {0x3A, 0x3A, 0x93, 0x93, 0xB9, 0x26, 0x26, 0},
    {0x32, 0x4A, 0xAA, 0xC5, 0x36, 0x36, 0x83, 0},
    {0x59, 0x59, 0x96, 0xB2, 0xB2, 0xBB, 0x35, 0x5D},
    {0x48, 0x48, 0xA4, 0xBA, 0x2C, 0x2C, 0x54, 0x6A},
    {0x44, 0x44, 0x62, 0x62, 0x4A, 0x4A, 0x6D, 0x93},
    {0x43, 0x43, 0x58, 0x58, 0xAA, 0xAA, 0xB5, 0x4D},
    {0x3A, 0x3A, 0xA6, 0xA6, 0xCB, 0xCB, 0x6C, 0x54},
    {0x38, 0x38, 0x8A, 0xA4, 0xA4, 0x34, 0x34, 0x4D},
    {0x2B, 0x6A, 0x72, 0x72, 0x36, 0x36, 0x96, 0x98},
    {0x3B, 0xA2, 0, 0, 0, 0, 0, 0},
    {0x23, 0xA7, 0, 0, 0, 0, 0, 0},
    {0x6B, 0x42, 0xBA, 0, 0, 0, 0, 0},
    {0x23, 0x93, 0x3A, 0, 0, 0, 0, 0},
    {0x6B, 0x42, 0xA7, 0, 0, 0, 0, 0},
    {0xB2, 0xBA, 0x23, 0x3A, 0, 0, 0, 0},
    {0x23, 0xBA, 0, 0, 0, 0, 0, 0},
    {0xB2, 0xA7, 0xBA, 0, 0, 0, 0, 0},
    {0x23, 0x3A, 0xA7, 0, 0, 0, 0, 0},
    {0x93, 0x6B, 0x42, 0x3A, 0, 0, 0, 0},
};
static const uint8_t CREATS[17] = {0x47, 0x63, 0xA3, 0x4B, 0x65, 0x3C, 0xA9, 0xA3, 0x3C, 0x63, 0x47, 0x4E, 0x3C, 0x3A, 0x33, 0x87, 0x78};
static const uint8_t TUNLDIR[17] = {0x30, 0x08, 0x18, 0x18, 0x1C, 0x04, 0x06, 0x0E, 0x03, 0x0F, 0x0F, 0x01, 0x03, 0x07, 0x0F, 0x40, 0x80};
static const uint8_t RANDCRET[8] = {0xAA, 0xE1, 0x55, 0x0F, 0x8D, 0x69, 0x2D, 0x96};
static const uint8_t ROCKS[17][5] = {
    {0x35, 0xB4, 0x9B, 0x00, 0x00}, {0x23, 0x39, 0x8D, 0xB5, 0x00}, {0x23, 0x6C, 0xBD, 0x94, 0x00},
    {0x43, 0x2B, 0x5C, 0xBD, 0x84}, {0x63, 0x29, 0x3D, 0xBB, 0xA6}, {0x28, 0x6B, 0x97, 0xB3, 0x00},
    {0x29, 0x52, 0x6D, 0xAC, 0x00}, {0x54, 0x6D, 0x96, 0xB8, 0x00}, {0x28, 0x3C, 0x7A, 0x72, 0xB6},
    {0x34, 0x72, 0x8C, 0xB8, 0x00}, {0x33, 0x3B, 0xBC, 0xA6, 0x00}, {0x39, 0x7B, 0x82, 0xCA, 0x00},
    {0x32, 0x29, 0xBA, 0xA3, 0x00}, {0x32, 0x4B, 0xB9, 0x83, 0x00}, {0x52, 0x39, 0x9A, 0xB4, 0x00},
    {0x53, 0x9B, 0x00, 0x00, 0x00}, {0x3B, 0x00, 0x00, 0x00, 0x00}};
static const uint8_t RANDROKS[8] = {0x29, 0x35, 0x64, 0x82, 0x8D, 0x9A, 0xA5, 0xC9};

static inline uint8_t calcindx(uint8_t x, uint8_t y) {
    uint8_t zone = (uint8_t)((y >> 2) / 3);
    if (zone > 15) zone = 15;
    return (uint8_t)(MAPLOW[zone] + (x >> 3));
}

static inline uint8_t rowhere(uint8_t y) {
    uint8_t i;
    for (i = 0; i < 16; ++i)
        if (TOPZONE[i] == y) return 1;
    return 0;
}

#endif
