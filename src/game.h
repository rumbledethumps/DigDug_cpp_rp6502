// Dig Dug game logic for the RP6502 -- C port of the Atari 7800 assembly.
#ifndef GAME_H
#define GAME_H
#include <stdint.h>

// direction indices 0=E 1=W 2=S 3=N ; joystick bits (active low, like SWCHA)
#define JOY_E 0x80
#define JOY_W 0x40
#define JOY_S 0x20
#define JOY_N 0x10

enum { DIR_E = 0, DIR_W = 1, DIR_S = 2, DIR_N = 3 };
enum { MODE_DIAG = 0, MODE_VERT, MODE_HORIZ, MODE_ESCAPE, MODE_OFFSCR };

typedef struct {
    uint8_t x, y;       // y = 0 : not displayed
    uint8_t ix;         // stamp index
    uint8_t cset;       // 1 or 2
    uint8_t prior;      // 1 = drawn first (behind)
} stamp_t;

typedef struct {
    stamp_t s;
    uint8_t index, stat, dir, facing, ghost, ymodc, seqn, seqlen, mode;
    uint8_t blwstat, blwctr, blwypos, preblwix, blwlstmv;
    int8_t rocknum;
    uint16_t speed;
    const uint8_t* seq;
} monster_t;

typedef struct {
    stamp_t s;
    uint8_t tumble, stat, numsqsh;
} rock_t;

typedef struct {
    uint8_t type, step, arg, x, cnt;
} seq_t;

typedef struct {
    uint8_t dirtmap[256];
    uint8_t backdirt[256];
    uint32_t score[2];          // in points
    uint8_t nummen[2];
    uint8_t racknum[2];
    uint8_t effrack[2];
    uint8_t playnum, numplayr, attract, bonzo, nonoise;
    uint8_t gameOver;
    uint8_t message;            // 0 none, 1 PLAYER n, 2 GAME OVER
    uint8_t messageBlank;
    uint8_t vegcol[3];
    uint8_t dirtScheme;
    uint8_t walking, fast, escaper, dethwish, death, squash, entry;

    stamp_t dd;
    stamp_t pump[3];
    stamp_t flame;
    stamp_t fruit;
    monster_t mon[8];
    rock_t rock[5];

    uint8_t lastmove, digging, pumpnum_u, maxpump;   // pumpnum kept signed below
    int8_t pumpnum;
    int8_t pumpie;
    uint8_t freeze;
    seq_t seqs[4];
    uint8_t nseq;

    // assembly named variables
    uint8_t frmcnt, p2init, racktime, speedup, flamwait, ghostout, flee;
    uint8_t nxtsec, scndtim, notunnel, fallcnt, brcreat, brtunnl;
    uint8_t hitrock, pumpct;
    uint8_t proport, proport2, proport3;
    uint8_t digspeed;
    uint16_t dhorspd;
    uint8_t astage, ignore;
    int8_t flamsize, flamnum;
    int16_t gp_dx, gp_dy;
    uint8_t rocksnow;
    uint8_t animcnt, pumpBtn, notgrid, fscore, fscorctr;
    int8_t needdir;
    uint8_t ghostime, pumping, pumpcnt, freezeDd, flamie, flamtime;
    uint8_t fruitc, digrest, digtemp;

    // snapshot of the other player's state (two player mode)
    uint8_t backValid;
    uint8_t bsx[19], bsy[19], bsix[19];
    uint8_t btumble[5], brstat[5], bnumsqsh[5];
    uint8_t bmonstat[8];
    uint8_t bhitrock, bfallcnt, bbrcreat, bbrtunnl;
} game_t;

extern game_t G;
extern stamp_t* const ALL_STAMPS[19];

// dirt map writes go through this so the video layer can redraw the cell
void game_set_dirt(uint8_t idx, uint8_t v);

void game_init(uint8_t numPlayers, uint8_t bonzo, uint8_t attract, uint8_t startRack);
void game_frame(uint8_t joy, uint8_t button);

// provided by sound.c
void sound_request(uint8_t song);
// provided by main.c / video.c
uint8_t game_random(void);

// ---- internal (shared between game.c and monsters.c)
uint8_t game_shift8(uint8_t* v);
uint8_t game_getdirt_idx(uint8_t x, uint8_t y);
void game_getmode(void);
void game_avoid(void);
void game_offscr2(monster_t* m);
void game_newseq(monster_t* m, uint8_t d);
void game_speeder(monster_t* m);
uint8_t game_chkother(uint8_t c, uint8_t idx);
uint8_t game_getdirt(const monster_t* m, uint8_t* idx);
void game_flamout4(monster_t* m);
void game_vanpump(void);
void game_yescoll(void);
void game_dfruit(void);
void game_pumper(uint8_t button);
void game_monsters_frame(uint8_t button);
void game_fruiter(void);
void game_flamer(void);
void game_setflam(void);
void game_addscore(uint16_t pts);

// song numbers
enum { SNGC = 1, SNGD = 2, SNGDF = 3, SNGNULL = 4, SNG1 = 5, SNG2 = 6, SNG3 = 7, SNG5 = 8, SNG4 = 9,
       SNG6 = 10, SNG7 = 11, SNG8 = 12, SNG9 = 13, SNG10 = 14, SNG11 = 15, SNG12 = 16, SNG13 = 17,
       SNG14 = 18, SNG15 = 19 };

#endif
