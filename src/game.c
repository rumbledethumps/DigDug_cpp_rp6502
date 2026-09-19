// Dig Dug game logic (DDMOVE / COLLISON / FALL / CHKDEATH / INITDIRT ...)
#include "game.h"
#include "tables.h"
#include "video.h"
#include <string.h>

game_t G;

stamp_t* const ALL_STAMPS[19] = {
    &G.dd, &G.mon[0].s, &G.mon[1].s, &G.mon[2].s, &G.mon[3].s, &G.mon[4].s, &G.mon[5].s,
    &G.mon[6].s, &G.mon[7].s, &G.rock[0].s, &G.rock[1].s, &G.rock[2].s, &G.rock[3].s,
    &G.rock[4].s, &G.flame, &G.pump[0], &G.pump[1], &G.pump[2], &G.fruit};

static const uint8_t JOYSWCH[4] = {0x7F, 0xBF, 0xDF, 0xEF};
static const uint8_t REVSWCH[4] = {0xBF, 0x7F, 0xEF, 0xDF};
#define LOWSCOR 178

enum { SEQ_DEATH, SEQ_RACKEND, SEQ_PLAYREDY, SEQ_NEWPLAYR };

#define noise(n) sound_request(n)
#define rack() (G.racknum[G.playnum])

static void initrack(void);
static void initdirt(void);
static void dostart(void);
static void initrock(void);
static void ddmiddle(void);
static void finmov1(void);
static void pushSeq(uint8_t type, uint8_t arg);

void game_set_dirt(uint8_t idx, uint8_t v) {
    if (G.dirtmap[idx] != v) {
        G.dirtmap[idx] = v;
        video_dirty(idx);
    }
}
#define SETDIRT(i, v) game_set_dirt((uint8_t)(i), (uint8_t)(v))

static uint8_t iabs8(int16_t v) { return (uint8_t)(v < 0 ? -v : v); }

// ------------------------------------------------------------------ init ---
static void zero20(void) {
    G.animcnt = G.digging = G.pumpnum = G.squash = G.notgrid = G.needdir = G.fscore = G.fscorctr = 0;
    G.ghostime = G.escaper = G.pumpBtn = G.pumping = G.pumpcnt = G.walking = G.freezeDd = G.flamie = 0;
    G.flamtime = G.fruitc = G.digrest = G.scndtim = G.digtemp = 0;
}

static void initVars(void) {
    G.lastmove = 0; G.death = 0; G.dethwish = 0; G.racktime = 0; G.speedup = 0; G.flamwait = 0;
    G.ghostout = 0; G.flee = 0; G.fast = 0; G.nxtsec = 0; G.scndtim = 0; G.notunnel = 0; G.fallcnt = 0;
    G.brcreat = 0; G.brtunnl = 0; G.hitrock = 0; G.maxpump = 0; G.pumpct = 0;
    G.proport = G.proport2 = G.proport3 = PROPORT_INIT;
    G.digspeed = DIGFAST; G.dhorspd = DHFAST;
    G.astage = 0; G.ignore = 0; G.flamsize = 0; G.flamnum = 0; G.pumpie = -1; G.gp_dx = G.gp_dy = 0;
    zero20();
}

static void newDlst(void) {
    G.nonoise = G.attract;
    G.freeze = 40;
}

static void afterInitGame(void) {
    initrack();
    if (G.attract) {
        G.nummen[0] = 2; G.nummen[1] = 0; G.astage = 0;
    } else {
        G.nummen[0] = 5; G.nummen[1] = G.numplayr ? 5 : 0;
        G.score[0] = G.score[1] = 0;
    }
    newDlst();
}

void game_init(uint8_t numPlayers, uint8_t bonzo, uint8_t attract, uint8_t startRack) {
    uint8_t i;
    memset(&G, 0, sizeof G);
    G.numplayr = numPlayers > 1 ? 1 : 0;
    G.bonzo = bonzo ? 1 : 0;
    G.attract = attract ? 1 : 0;
    G.racknum[0] = G.racknum[1] = startRack;
    for (i = 0; i < 8; ++i) {
        G.mon[i].index = i;
        G.mon[i].stat = 0x80;
        G.mon[i].facing = 8;
        G.mon[i].seq = RPOOKA_SEQ;
        G.mon[i].seqlen = 8;
        G.mon[i].mode = MODE_VERT;
        G.mon[i].rocknum = -1;
        G.mon[i].s.cset = 2;
    }
    for (i = 0; i < 5; ++i) { G.rock[i].s.ix = STRTROCK; G.rock[i].s.cset = 2; G.rock[i].s.prior = 1; }
    G.dd.cset = 2; G.flame.cset = 2;
    for (i = 0; i < 3; ++i) { G.pump[i].cset = 2; G.pump[i].prior = 1; }
    G.fruit.cset = 1;
    G.fruit.prior = 1;      /* ZPRIOR1: rocks, pump, fruit are drawn first */
    G.entry = G.attract ? 0 : 1;
    initVars();
    G.pumpie = -1;
    // initGame
    if (!G.attract && G.numplayr) pushSeq(SEQ_PLAYREDY, 1);
    else afterInitGame();
}

uint8_t game_shift8(uint8_t* v) {
    uint8_t c = (*v >> 7) & 1;
    *v = (uint8_t)((*v << 1) | c);
    return c;
}

static uint8_t shift16(uint16_t* v) {
    uint8_t c = (uint8_t)((*v >> 15) & 1);
    *v = (uint16_t)((*v << 1) | c);
    return c;
}

// ----------------------------------------------------------------- score ---
void game_addscore(uint16_t pts) {
    uint32_t old, nw;
    uint8_t x = G.playnum;
    if (G.attract || G.entry) return;
    old = G.score[x];
    nw = old + pts;
    if (nw > 999990UL) nw = 999990UL;
    G.score[x] = nw;
    if (nw / 10000UL != old / 10000UL) {
        uint8_t h = (uint8_t)(nw / 10000UL);
        if (h == 2 || h % 10 == 0 || h % 10 == 5) {
            if (G.nummen[x] < 10) {
                G.nummen[x]++;
                noise(SNG11);
            }
        }
    }
    video_score_changed();
}

// ------------------------------------------------------------- per frame ---
static void secondTimer(void) {
    if (G.frmcnt == G.nxtsec) {
        G.nxtsec = (uint8_t)(G.nxtsec + 60);
        G.scndtim++;
        if (G.speedup) G.speedup--;
        if (G.ghostout) G.ghostout--;
    }
}

static void ddmove(uint8_t joy);
static void eatveg(void);
static void collison(void);
static void blowup(void);
static void fall(void);
static uint8_t chkrack(void);
static void runSeq(void);

void game_frame(uint8_t joy, uint8_t button) {
    G.frmcnt++;
    secondTimer();
    if (G.freeze > 0) {
        G.freeze--;
        return;
    }
    if (G.nseq) {
        runSeq();
        return;
    }
    G.racktime++;
    if (G.death) {
        pushSeq(SEQ_DEATH, 0);
        return;
    }
    if (chkrack()) return;
    if (!G.dethwish) {
        ddmove(joy);
        eatveg();
        game_monsters_frame(button);
        collison();
        blowup();
    }
    fall();
}

// --------------------------------------------------------------- Dig Dug ---
static void nogo(void) {
    if (G.walking) G.walking--;
}

static void zjoy(uint8_t a);

static void prezjoy(uint8_t a) {
    G.digrest++;
    zjoy(a);
}

static void anim(uint8_t y) {
    G.animcnt = G.animcnt > 0 ? G.animcnt - 1 : 2;
    G.dd.ix = DANIMS[G.animcnt * 4 + y];
}

static int8_t chkrock(uint8_t yi) {
    uint8_t tx = (uint8_t)(G.dd.x + XTOPL[4 + yi]);
    uint8_t ty = (uint8_t)(G.dd.y + YTOPL[4 + yi]);
    int8_t x;
    for (x = 4; x >= 0; --x) {
        const rock_t* r = &G.rock[x];
        int16_t d;
        uint8_t c;
        if (tx != r->s.x) continue;
        if (r->s.ix >= LOWSCOR) continue;
        if (ty == r->s.y) return x;
        if (r->tumble == 0) continue;
        if (!(G.lastmove & 2)) continue;
        d = (int16_t)r->s.y + 7 - ty;
        if (d < 0 || d >= 17) continue;
        if (G.dd.y == MINDDY) return x;
        c = G.dirtmap[calcindx(tx, (uint8_t)(G.dd.y - 12))];
        if (c == 36 || c == 44) return x;
    }
    return -1;
}

static void adjustto(uint8_t y, uint8_t* tx, uint8_t* ty) {
    *tx = (uint8_t)(G.dd.x + XTOPL[y]);
    *ty = (uint8_t)(G.dd.y + YTOPL[y]);
}

static uint8_t chknxtd(uint8_t a) {
    uint8_t tx, ty, c;
    adjustto((uint8_t)(a + G.lastmove), &tx, &ty);
    c = G.dirtmap[calcindx(tx, ty)];
    if (c < 36) {
        G.digtemp = 0;
        return 0;
    }
    return 1;
}

static void digger(uint8_t i, uint8_t c, const uint8_t* table) {
    uint8_t k = (uint8_t)((c - 36) >> 1);
    uint8_t nw;
    if (c < 36 || k >= 7) return;
    nw = table[k];
    SETDIRT(i, nw);
    if (nw < 30) {
        game_addscore(10);
        if (c < 46) G.digtemp = 1;
    } else if (nw != c) {
        G.digtemp = 1;
    }
}

static void digdisp(uint8_t a, uint8_t y) {
    uint8_t tx, ty, i, c;
    if (a == VPRE1[y] || a == VPRE2[y] || a == VPRE3[y]) {
        chknxtd(12);
        return;
    }
    if (a == VONE[y]) {
        uint8_t fx = (uint8_t)(G.dd.x + XFROMPL[y]);
        uint8_t fy = (uint8_t)(G.dd.y + YFROMPL[y]);
        i = calcindx(fx, fy);
        c = G.dirtmap[i];
        if (c < 36) SETDIRT(i, FROMXFM[y][c >> 1]);
        G.digtemp = 0;
        adjustto(y, &tx, &ty);
        i = calcindx(tx, ty);
        c = G.dirtmap[i];
        if (c < 36) SETDIRT(i, TOXFM[y][c >> 1]);
        else digger(i, c, ENTRXFM[y]);
        return;
    }
    if (a == VHALF[y]) {
        G.digtemp = 0;
        adjustto(y, &tx, &ty);
        i = calcindx(tx, ty);
        c = G.dirtmap[i];
        if (c >= 36) digger(i, c, HALFXFM[y]);
        return;
    }
    if (a == VBOUND[y]) {
        if (rowhere(G.dd.y) && (G.dd.x & 7) == 0) {
            if (!chknxtd(4)) G.notgrid = 0;
        }
    }
}

static uint8_t rockpush(uint8_t r) {
    const rock_t* rk = &G.rock[r];
    return G.dirtmap[calcindx(rk->s.x, (uint8_t)(rk->s.y - 12))];
}

static void chkfall2(uint8_t r, int8_t a) {
    if (G.rock[r].tumble) return;
    G.needdir = a;
    G.hitrock = r;
}

static void betty(void) {
    int8_t r = chkrock((uint8_t)(12 + G.lastmove));
    if (r < 0 || G.rock[r].tumble) return;
    if (rockpush((uint8_t)r) < 36) chkfall2((uint8_t)r, (int8_t)CVTDIR[G.lastmove]);
}

static void finmov(void) {
    int8_t r;
    G.digrest = 0;
    if (G.needdir != 0) {
        if (G.needdir < 0 || (uint8_t)(G.needdir - 1) == G.lastmove) {
            G.rock[G.hitrock].tumble = ONEDGE;
            G.needdir = 0;
        }
    }
    r = chkrock((uint8_t)(4 + G.lastmove));
    if (r >= 0) {
        chkfall2((uint8_t)r, (int8_t)CVTDIR[G.lastmove]);
    } else {
        r = chkrock(3);
        if (r >= 0) chkfall2((uint8_t)r, -1);
        else betty();
    }
    finmov1();
}

static void finvert(uint8_t a, uint8_t y) {
    G.notgrid = 1;
    digdisp(a, y);
    finmov();
}

static void setdfast(void) {
    G.dhorspd = DHFAST;
    G.digspeed = DIGFAST;
}

void game_vanpump(void) {
    G.pumpie = -1;
    G.pump[0].y = G.pump[1].y = G.pump[2].y = 0;
}

static void harp(void) {
    uint8_t lm = G.lastmove;
    int16_t py;
    G.pump[0].ix = HARPSTMP[lm];
    G.pump[0].x = (uint8_t)(G.dd.x + HARPXPL[lm]);
    py = (int16_t)G.dd.y + HARPYPL[lm];
    if (py < MINDDY) game_vanpump();
    else G.pump[0].y = (uint8_t)py;
}

static void finmov1(void) {
    if (G.digtemp != G.digging) {
        G.digging = G.digtemp;
        if (G.digging) {
            G.dhorspd = DHSLOW;
            G.digspeed = DIGSLOW;
        } else {
            setdfast();
        }
    }
    if (G.digging) harp();
    else game_vanpump();
}

static void mddn(uint8_t y) {
    uint8_t a;
    int8_t r;
    uint8_t ny;
    if (G.dd.x & 7) { zjoy(JOYSWCH[G.lastmove]); return; }
    a = JOYSWCH[G.lastmove];
    if (!game_shift8(&G.digspeed)) {
        if (a != REVSWCH[y]) { G.digrest = 0; return; }
    }
    G.lastmove = y;
    if (a == JOYSWCH[y]) {
        if (game_shift8(&G.proport3)) anim(y);
    } else {
        G.digtemp = 0;
        anim(y);
    }
    r = chkrock(y);
    if (r >= 0) {
        if (!(y == DIR_S && G.rock[r].tumble >= 7)) {
            G.digtemp = 0;
            finmov1();
            return;
        }
    }
    ny = G.dd.y;
    if (y == DIR_S) {
        if (ny == MINDDY) { G.digtemp = 0; finmov1(); return; }
        ny--;
    } else {
        ny++;
        if (ny >= TOPZONE[15]) {
            G.dd.ix = DANIMS[0];
            G.dd.y = 191;
            G.lastmove = 0;
            return;
        }
    }
    G.dd.y = ny;
    finvert((uint8_t)(ny % 12), y);
}

static void mdde(uint8_t y) {
    uint8_t a, nx;
    if (!rowhere(G.dd.y)) { zjoy(JOYSWCH[G.lastmove]); return; }
    a = JOYSWCH[G.lastmove];
    if (!shift16(&G.dhorspd)) {
        if (a != REVSWCH[y]) { G.digrest = 0; return; }
    }
    G.lastmove = y;
    if (a != JOYSWCH[y]) G.digtemp = 0;
    anim(y);
    if (chkrock(y) >= 0) { G.digtemp = 0; finmov1(); return; }
    nx = G.dd.x;
    if (y == DIR_W) {
        if (nx == MINDDX) { G.digtemp = 0; finmov1(); return; }
        nx--;
    } else {
        if (nx == MAXDDX) { G.digtemp = 0; finmov1(); return; }
        nx++;
    }
    G.dd.x = nx;
    finvert((uint8_t)(nx & 7), y);
}

static void zjoy(uint8_t a) {
    uint8_t y;
    for (y = 0; y < 4; ++y)
        if (!(a & (0x80 >> y))) break;
    if (y == 4) { nogo(); return; }
    if (y >= 2) mddn(y);
    else mdde(y);
}

static void ddmove(uint8_t joy) {
    uint8_t a, code;
    G.digtemp = G.digging;
    if (G.attract) {
        uint8_t x = G.lastmove, r, y;
        if (G.squash) { nogo(); return; }
        if (G.digrest == 0 && G.astage != 0) {
            G.astage--;
            prezjoy(JOYSWCH[x]);
            return;
        }
        r = game_random();
        G.astage = (uint8_t)((r & 3) + ((r >> 4) & 1));
        y = r & 3;
        while (JOYSWCH[y] == REVSWCH[x]) y = (y + 1) & 3;
        prezjoy(JOYSWCH[y]);
        return;
    }
    if (G.entry) {
        if (G.dd.x == DDSTRTX && G.dd.y == DDSTRTY) {
            G.nonoise = 0;
            noise(SNG15);
        }
        if (G.dd.y != DDSTRTY) {
            if (G.dd.y == DDMIDLY) {
                G.dd.ix = RDIGDUG[1];
                G.entry = 0;
                dostart();
                G.freeze = 40;
                return;
            }
            prezjoy(0xDF);
            return;
        }
        if (G.dd.x != DDMIDLX) { prezjoy(0xBF); return; }
        prezjoy(0xDF);
        return;
    }
    if (G.squash || G.freezeDd) { nogo(); return; }
    a = joy | 0x0F;
    if (a == 0xFF) {
        G.digrest++;
        nogo();
        return;
    }
    G.walking = 10;
    if ((a & 0xC0) == 0 || (a & 0x30) == 0) code = 0xFF;
    else if ((a & 0x90) == 0) code = 0x7F;
    else if ((a & 0x10) == 0) code = 0xEF;
    else if ((a & 0x40) == 0) code = 0xBF;
    else if ((a & 0x20) == 0) code = 0xDF;
    else code = 0x7F;
    prezjoy(code);
}

// ------------------------------------------------------------ vegetables ---
void game_dfruit(void) {
    uint8_t y = 0;
    if (!G.bonzo) { y = rack(); if (y > 0x12) y = 0x12; }
    G.fruit.ix = FRUITAB[y];
    G.fruit.cset = 1;
    G.fruit.x = 56;
    G.fruit.y = 107;
}

static void eatveg(void) {
    uint8_t x = 0;
    if (G.fscore) {
        G.fscorctr--;
        if (G.fscorctr) return;
        G.fscore = 0;
        G.fruit.y = 0;
    }
    if (G.fruitc < 0x0F) return;
    if (G.dd.y != 0x6B || G.dd.x != 0x38) return;
    G.fscore = 1;
    G.fruitc = 0;
    G.fscorctr = 0x30;
    if (!G.bonzo) { x = rack(); if (x > 18) x = 18; }
    G.fruit.ix = FPOINTS[x];
    G.fruit.cset = 2;
    G.fruit.x = 53;
    game_addscore(VEGPTS[x]);
    noise(SNG10);
}

// ------------------------------------------------------------ collisions ---
void game_yescoll(void) {
    G.dethwish = 1;
    G.dd.ix = ADEATH[DANIMIX[G.lastmove]];
    game_vanpump();
    noise(SNG8);
}

static void collison(void) {
    int8_t x;
    if (G.squash) return;
    for (x = 7; x >= 0; --x) {
        const monster_t* m = &G.mon[x];
        if (m->stat >= 2) continue;
        if (iabs8((int16_t)G.dd.x - m->s.x) >= 5) continue;
        if (iabs8((int16_t)G.dd.y - m->s.y) >= 7) continue;
        game_yescoll();
        return;
    }
}

// ------------------------------------------------------------------ pump ---
static void inputButton(uint8_t button) {
    if (!button) {
        G.pumpcnt = 0; G.pumping = 0; G.pumpBtn = 0;
        return;
    }
    if (G.pumpcnt == 0) {
        G.pumpBtn = 1;
        G.pumpcnt = 1;
        if (G.pumpie != -1 && G.pumpct == 0) G.pumpct = 0x10;
        return;
    }
    G.pumpcnt++;
    if (G.pumpcnt == 0x18) {
        G.pumping = 1;
        G.pumpcnt = 1;
    } else {
        G.pumping = 0;
        G.pumpBtn = 0;
    }
}

static void pumpOff(void) {
    G.freezeDd = 0;
    G.pumpnum = 0;
    game_vanpump();
}

static uint8_t disney(uint8_t a) {
    if (a < 12 + 23) return 23;
    return (uint8_t)(a - 12);
}

static void disppump(void) {
    uint8_t lm = G.lastmove;
    stamp_t* p0 = &G.pump[0];
    stamp_t* p1 = &G.pump[1];
    stamp_t* p2 = &G.pump[2];
    uint8_t t1, t2, a, i;
    if (lm < 2) {
        t1 = (G.pumpnum & 1) ? 2 : 0;
        if (lm == DIR_E) {
            p0->x = (uint8_t)(G.dd.x + t1 + 6);
            p0->y = G.dd.y;
            p0->ix = RPUMP[G.pumpnum >> 1];
        } else {
            t2 = (uint8_t)(G.pumpnum >> 1);
            p0->x = (uint8_t)(G.dd.x - t1 - 2 - t2 * 4);
            p0->y = G.dd.y;
            p0->ix = LPUMP[t2];
        }
        p1->ix = ZBLANK;
        p2->ix = ZBLANK;
        return;
    }
    if (lm == DIR_N) {
        int16_t v;
        t2 = (uint8_t)(G.pumpnum >> 1);
        p0->ix = PUPS0[t2]; p1->ix = PUPS1[t2]; p2->ix = PUPS2[t2];
        v = (int16_t)G.pumpnum * 3 + G.dd.y + 3;
        if (v >= 192) v = 191;
        a = (uint8_t)v;
    } else {
        int16_t v;
        t1 = (G.pumpnum & 1) ? 3 : 0;
        t2 = (uint8_t)(G.pumpnum >> 1);
        p0->ix = PDOWNS0[t2]; p1->ix = PDOWNS1[t2]; p2->ix = PDOWNS2[t2];
        v = (int16_t)G.dd.y - t1 - 9;
        if (p0->ix == DPUMP6) v += 5;
        if (v < 23) v = 23;
        a = (uint8_t)v;
    }
    p0->y = a;
    if (p1->ix == DPUMP6) a += 6;
    a = disney(a);
    p1->y = a;
    if (p2->ix == DPUMP6) a += 6;
    a = disney(a);
    p2->y = a;
    for (i = 0; i < 3; ++i) G.pump[i].x = (uint8_t)(G.dd.x + 2);
}

static uint8_t pumpcoll(void) {
    uint8_t lm = G.lastmove;
    const stamp_t* p0 = &G.pump[0];
    uint8_t t1, t2;
    int8_t x;
    if (lm < 2) {
        if (lm == DIR_E) t1 = (uint8_t)(G.pumpnum * 2 + G.dd.x + 1);
        else t1 = (uint8_t)(p0->x - 5);
        t2 = (uint8_t)(p0->y - 4);
    } else {
        if (lm == DIR_N) t2 = p0->y;
        else t2 = (uint8_t)(G.dd.y - G.pumpnum * 3 - 9);
        t1 = (uint8_t)(p0->x - 5);
    }
    for (x = 7; x >= 0; --x) {
        monster_t* m = &G.mon[x];
        int16_t d;
        uint8_t my;
        if (m->stat & 0xC0) continue;
        if ((m->stat & 0x0C) == 4) continue;
        if ((m->stat & 6) == 2) continue;
        d = (int16_t)m->s.x - t1;
        if (d < 0 || d >= 6) continue;
        my = ((m->stat & 2) && m->blwypos) ? m->blwypos : m->s.y;
        d = (int16_t)my - t2;
        if (d < 0 || d >= 8) continue;
        G.pumpie = x;
        if (x == G.flamie) game_flamout4(m);
        m->stat = 7;
        G.pumping = 1;
        return 1;
    }
    return G.pumping != 0;
}

static void extend2(void) {
    int8_t a;
    disppump();
    if (pumpcoll()) {
        G.pumpnum = -1;
        return;
    }
    a = G.pumpnum;
    G.pumpnum++;
    if ((uint8_t)a == G.maxpump && G.maxpump != 1) pumpOff();
}

static uint8_t lookdirt(uint8_t idx) {
    uint8_t c = G.dirtmap[idx];
    if (c >= CHARD12) return 0x0F;
    if (c >= CHARDF) return 2;
    return (uint8_t)(c >> 1);
}

uint8_t game_getdirt_idx(uint8_t x, uint8_t y) {
    uint8_t z = (uint8_t)(((uint8_t)(y - 11) >> 2) / 3);
    uint8_t row;
    if (y < 11) z = 0;
    if (z > 15) z = 15;
    row = (uint8_t)(15 - z);
    return (uint8_t)(row * 16 + (x >> 3));
}

static void sub2(uint8_t a) {
    uint8_t v = (uint8_t)(a + G.maxpump);
    if (v >= 12) v = 11;
    G.maxpump = v;
}

static void getmaxp(void) {
    uint8_t idx, lm, a;
    int8_t x = 2;
    G.maxpump = 0;
    G.pump[0].x = G.dd.x;
    G.pump[0].y = G.dd.y;
    idx = game_getdirt_idx(G.dd.x, G.dd.y);
    lm = G.lastmove;
    if (lm == DIR_E) {
        if (G.notgrid == 0) G.maxpump += 4;
        while (x >= 0) {
            idx++;
            if (lookdirt(idx) & 2) break;
            G.maxpump += 4;
            x--;
        }
        a = G.dd.x & 7;
        if (a) a = (uint8_t)(((a - 1) >> 1) ^ 3);
        sub2(a);
        return;
    }
    if (lm == DIR_W) {
        if (G.notgrid == 0) {
            G.maxpump += 4;
            idx--;
            x--;
        }
        while (x >= 0) {
            if (lookdirt(idx) & 8) break;
            G.maxpump += 4;
            idx--;
            x--;
        }
        sub2((uint8_t)((G.dd.x & 7) >> 1));
        return;
    }
    if (lm == DIR_N) {
        if (G.notgrid == 0) G.maxpump += 4;
        while (x >= 0) {
            idx -= 16;
            if (lookdirt(idx) & 1) break;
            G.maxpump += 4;
            x--;
        }
        a = (uint8_t)((G.dd.y + 1) % 12);
        if (a) a = (uint8_t)(12 - a);
        sub2((uint8_t)(a / 3));
        return;
    }
    if (G.notgrid == 0) {
        G.maxpump += 4;
        idx += 16;
        x--;
    }
    while (x >= 0) {
        if (lookdirt(idx) & 4) break;
        G.maxpump += 4;
        idx += 16;
        x--;
    }
    a = (uint8_t)((G.dd.y + 1) % 12);
    sub2((uint8_t)(a / 3));
}

void game_pumper(uint8_t button) {
    inputButton(button);
    if (G.pumpct) {
        G.pumpct--;
        if (G.pumpct == 0 && G.pumpie != -1) {
            G.pumping = 1;
            G.pumpcnt = 1;
        }
    }
    if (G.pumpnum == 0) {
        if (G.digging || G.squash || !G.pumpBtn) return;
        G.freezeDd = 1;
        noise(SNG5);
        getmaxp();
        extend2();
        return;
    }
    if (G.pumpnum < 0) {
        int8_t x = G.pumpie;
        if (x != -1) {
            const monster_t* m = &G.mon[x];
            if (!(m->stat & 0xC0) && (m->stat & 4)) {
                if (G.pumpcnt) return;
                G.freezeDd = 0;
                if (G.walking == 0) return;
            }
        }
        pumpOff();
        return;
    }
    if (G.maxpump == 1) { pumpOff(); return; }
    extend2();
}

// --------------------------------------------------------------- blow up ---
static void puff(monster_t* m) {
    uint8_t i = (uint8_t)((m->blwstat - 1) + ((m->facing & 8) ? 4 : 0) + (m->index >= 4 ? 8 : 0));
    m->s.ix = BLWSTMPS[i & 15];
}

static void carol(monster_t* m) {
    uint8_t a = (uint8_t)(m->blwypos - 4);
    uint8_t y = 0;
    while (a >= 12) { a -= 12; y++; }
    y >>= 2;
    if (m->index >= 4 && !(m->blwlstmv & 2)) y += 4;
    m->s.ix = BPOINTS[y & 7];
    if (m->s.x < 8) m->s.x = 8;
    m->s.y = m->blwypos;
    m->s.prior = 1;
    game_addscore(BURSTPTS[y & 7]);
    m->blwctr = 30;
    m->blwstat++;
}

static void blowup(void) {
    int8_t x;
    for (x = NUMCREAT; x >= 0; --x) {
        monster_t* m = &G.mon[x];
        uint8_t y;
        if (m->stat & 0x80) continue;
        y = m->blwstat;
        if (m->stat & 2) {
            if (y == 5) {
                if (--m->blwctr) continue;
                game_offscr2(m);
                game_getmode();
                continue;
            }
            if (y == 4) {
                if (--m->blwctr) continue;
                carol(m);
                continue;
            }
        }
        if (G.pumping && x == G.pumpie) {
            if (m->blwstat == 0) {
                m->preblwix = m->s.ix;
                m->blwypos = m->s.y;
                m->s.y = (uint8_t)(m->s.y + 3 > 191 ? 191 : m->s.y + 3);
            }
            m->blwstat++;
            m->blwctr = 0x20;
            puff(m);
            noise(SNG4);
            if (m->blwstat != 4) continue;
            noise(SNG6);
            m->stat &= (uint8_t)~4;
            m->blwlstmv = G.lastmove;
            m->blwctr = 30;
            game_vanpump();
            continue;
        }
        if (!(m->stat & 2)) continue;
        if (--m->blwctr) continue;
        m->blwctr = 0x20;
        m->blwstat--;
        if (m->blwstat) {
            puff(m);
            continue;
        }
        m->stat &= (uint8_t)~7;
        m->s.ix = m->preblwix;
        m->s.y = m->blwypos;
        if (x == G.pumpie) game_vanpump();
    }
}

// ----------------------------------------------------------------- rocks ---
static void endstat2(rock_t* r) {
    r->stat = 0;
    r->tumble++;
}

static void rtotter(rock_t* r) {
    r->s.ix = (r->stat & 8) ? STRTROCK : TOTRROCK;
    r->stat++;
    if (r->stat >= 0x19) { endstat2(r); return; }
    if (r->stat < 8) return;
    if (r->stat == 0x18) G.flee = 0x20;
    game_avoid();
}

static uint8_t crunch(const rock_t* r, uint8_t vx, uint8_t vy, uint8_t* ny) {
    int16_t d, t;
    if (iabs8((int16_t)r->s.x - vx) >= 6) return 0;
    d = (int16_t)r->s.y + 6 - vy;
    if (d < 0 || d >= 17) return 0;
    t = (int16_t)r->s.y - 5;
    if (t < MINDDY) { *ny = MINDDY; return 1; }
    if (t - vy < 4) { *ny = (uint8_t)t; return 1; }
    *ny = vy;
    return 1;
}

static void crush(rock_t* r, uint8_t x) {
    int8_t y;
    uint8_t ny;
    for (y = NUMCREAT; y >= 0; --y) {
        monster_t* m = &G.mon[y];
        uint8_t vy;
        if ((m->stat & 0x80) || (m->stat & 0x86) == 2) continue;
        vy = ((m->stat & 6) == 6) ? m->blwypos : m->s.y;
        if (!crunch(r, m->s.x, vy, &ny)) continue;
        m->s.y = ny;
        if (!(m->stat & 0x40)) {
            m->rocknum = (int8_t)x;
            noise(SNG1);
        }
        m->stat = (uint8_t)((m->stat & 0xF9) | 0x41);
        m->s.ix = m->index >= 4 ? SQFYGAR : SQPOOKA;
        if (y == G.flamie) {
            G.flamie = 0;
            G.flame.y = 0;
        }
        if (y == G.pumpie) game_vanpump();
    }
    if (crunch(r, G.dd.x, G.dd.y, &ny)) {
        G.dd.y = ny;
        game_vanpump();
        if (!G.squash) noise(SNG1);
        G.squash = 1;
        G.dd.ix = SQDIGDUG;
    }
    r->stat++;
    game_avoid();
}

static void rfall(rock_t* r, uint8_t x) {
    if (r->s.y == MINDDY) { endstat2(r); return; }
    r->s.y -= RFALLTBL[r->stat % 7];
    if (r->stat >= 6) {
        uint8_t off = (uint8_t)(r->s.y % 12);
        uint8_t idx, c;
        if (off == 11) {
            idx = calcindx(r->s.x, (uint8_t)(r->s.y - 12));
            c = G.dirtmap[idx];
            if (idx >= 0xF0) { endstat2(r); return; }
            if (c >= 36 && c != 48 && c != 44) { endstat2(r); return; }
        } else if (off == 4) {
            idx = calcindx(r->s.x, (uint8_t)(r->s.y - 12));
            c = G.dirtmap[idx];
            if (c >= 36) {
                if (c != 48) { endstat2(r); return; }
                SETDIRT(idx, 20);
            }
        } else if (off == 10) {
            idx = calcindx(r->s.x, (uint8_t)(r->s.y - 12));
            c = G.dirtmap[idx];
            if (c < 36) SETDIRT(idx, VERTBOT[c >> 1]);
            idx = calcindx(r->s.x, r->s.y);
            c = G.dirtmap[idx];
            if (c < 36) SETDIRT(idx, VERTTOP[c >> 1]);
        }
    }
    crush(r, x);
}

static void rbottom(rock_t* r, uint8_t x) {
    int8_t y;
    G.flee = 0;
    r->stat++;
    if (r->stat < 0x10) {
        if (G.escaper) return;
        if (r->stat == 1) game_getmode();
        return;
    }
    endstat2(r);
    r->numsqsh = 0;
    for (y = NUMCREAT; y >= 0; --y) {
        monster_t* m = &G.mon[y];
        if ((m->stat & 0x40) && m->rocknum == (int8_t)x) {
            r->numsqsh++;
            m->s.y = 0;
        }
    }
    noise(SNG2);
}

static void rfinish(rock_t* r, uint8_t x) {
    int8_t y;
    uint8_t n;
    r->tumble = 0;
    r->s.y = 0;
    G.rocksnow--;
    G.fallcnt++;
    if (G.fallcnt == 2) G.fruitc = 1;
    if (G.squash) {
        G.squash = 0;
        G.dethwish = 1;
    }
    for (y = NUMCREAT; y >= 0; --y) {
        monster_t* m = &G.mon[y];
        if (m->rocknum == (int8_t)x && (m->stat & 0x40)) m->stat = 0x80;
    }
    n = r->numsqsh;
    if (n) {
        if (n > 8) n = 8;
        game_addscore(FLATPTS[n - 1]);
    }
    game_getmode();
}

static void rockmov(uint8_t x) {
    rock_t* r = &G.rock[x];
    uint8_t t;
    if (r->tumble < ONEDGE) return;
    G.rocksnow++;
    t = r->tumble;
    if (t == 2) {
        noise(SNG7);
        endstat2(r);
    } else if (t == 3) {
        rtotter(r);
    } else if (t == 4) {
        rfall(r, x);
    } else if (t == 5) {
        rbottom(r, x);
    } else if (t == 6 || t == 7) {
        r->s.ix = RKANIM[t - 6];
        r->stat++;
        if (r->stat >= RKTIM[t - 6]) endstat2(r);
    } else if (t == 8) {
        if (r->numsqsh == 0) {
            endstat2(r);
        } else {
            uint8_t n = r->numsqsh > 8 ? 8 : r->numsqsh;
            r->s.ix = DISPTS[n - 1];
            r->stat++;
            if (r->stat >= RKTIM[2]) endstat2(r);
        }
    } else {
        rfinish(r, x);
    }
}

static void fall(void) {
    int8_t x;
    G.rocksnow = 0;
    for (x = NUMROCK; x >= 0; --x) rockmov((uint8_t)x);
    if (G.rocksnow == 0 && G.dethwish) {
        G.dethwish = 0;
        G.death = 1;
    }
}

// ------------------------------------------------------------- rack flow ---
static uint8_t testrackCondition(void) {
    uint8_t i;
    if (G.dethwish || G.squash) return 0;
    for (i = 0; i < 8; ++i)
        if (!(G.mon[i].stat & 0x80)) return 0;
    return 1;
}

static uint8_t chkrack(void) {
    if (testrackCondition()) {
        pushSeq(SEQ_RACKEND, 0);
        return 1;
    }
    return 0;
}

static void ddmiddle(void) {
    G.dd.y = DDMIDLY;
    G.dd.x = DDMIDLX;
    G.dd.ix = RDIGDUG[0];
}

static void hcarve(uint8_t x) {
    SETDIRT(x, DLEFTEND);
    SETDIRT(x + 1, DHMIDDLE);
    SETDIRT(x + 2, DRHTEND);
}

static void initrack(void) {
    uint8_t i, rk, big, small, y;
    for (i = 0; i < 8; ++i) { G.mon[i].stat = 0; G.mon[i].s.y = 0; G.mon[i].s.prior = 0; }
    G.flame.y = 0;
    G.fruit.y = 0;
    for (i = 0; i < 3; ++i) G.pump[i].y = 0;
    for (i = 0; i < 5; ++i) G.rock[i].s.y = 0;
    G.notunnel = 0;
    G.fallcnt = 0;
    G.racktime = 0;
    initdirt();
    initrock();
    if (G.entry) {
        G.dd.x = DDSTRTX;
        G.dd.y = DDSTRTY;
        G.dd.ix = LDIGDUG[0];
    } else {
        ddmiddle();
        for (i = 1; i < 7; ++i) SETDIRT(i * 16 + 7, 20);
        SETDIRT(7 * 16 + 7, 8);
    }
    rk = rack();
    big = rk / 10;
    small = rk % 10;
    y = 0x0E;
    for (i = 0; i < big; ++i) {
        SETDIRT(y, BIGFLOWR);
        if (--y == 0) return;
    }
    for (i = 0; i < small; ++i) {
        SETDIRT(y, SMLFLOWR);
        if (--y == 0) return;
    }
    while (y > 0) { SETDIRT(y, FULLSKY); y--; }
}

static void initdirt(void) {
    uint8_t px = G.playnum;
    uint8_t eff, rcreats, rtunnels, i, vx, rk, x;
    const uint8_t* tun;
    uint8_t pcount = 0, fcount = 4;
    if (!G.notunnel) {
        uint16_t k;
        for (k = 0x10; k < 0xF0; ++k) SETDIRT(k, FULLDIRT);
        hcarve(0x76);
        for (i = 1; i < 15; ++i) {
            SETDIRT(i * 16, BLANKL);
            SETDIRT(i * 16 + 15, BLANKR);
        }
        SETDIRT(0xEF, BLANKR);
        SETDIRT(0, LSKYHALF);
        SETDIRT(15, RSKYHALF);
        for (k = 0xF0; k < 0x100; ++k) G.dirtmap[k] = 0;
        if (G.attract) {
            eff = 0;
        } else {
            uint8_t y;
            rk = G.racknum[px];
            if (rk == 0) rk = G.racknum[px] = 1;
            y = (uint8_t)(rk - 1);
            if (G.bonzo) { eff = (uint8_t)(y + 15); if (eff > MAXEFFR) eff = MAXEFFR; }
            else if (y < 15) eff = y;
            else eff = (uint8_t)((rk & 3) + 11);
        }
        G.effrack[px] = eff;
    }
    eff = G.effrack[px];
    if (eff >= MAXEFFR) {
        if (G.notunnel) {
            rcreats = RANDCRET[G.brcreat];
            rtunnels = G.brtunnl;
            x = G.brcreat;
        } else {
            x = G.frmcnt & 7;
            rcreats = RANDCRET[x];
            G.brcreat = x;
            rtunnels = game_random();
            G.brtunnl = rtunnels;
        }
        tun = PTUNNELS[x + 17];
    } else {
        rcreats = CREATS[eff];
        rtunnels = TUNLDIR[eff];
        tun = PTUNNELS[eff];
    }
    for (i = 0; i < 8; ++i) {
        uint8_t t = tun[i];
        uint8_t vertical = (rtunnels & 0x80) != 0;
        uint8_t isFygar = (rcreats & 0x80) != 0;
        uint8_t cx;
        monster_t* m;
        int16_t x0, y0;
        rtunnels <<= 1;
        rcreats <<= 1;
        cx = isFygar ? fcount++ : pcount++;
        m = &G.mon[cx];
        if (t == 0) {
            m->s.y = 0;
            m->stat = 0x80;
            continue;
        }
        if (!G.notunnel) {
            if (vertical) {
                SETDIRT(t, DTOPEND);
                SETDIRT(t + 0x10, DVMIDDLE);
                SETDIRT(t + 0x20, DBOTEND);
            } else {
                hcarve(t);
            }
        }
        x0 = (t & 0x0F) * 8;
        y0 = TOPZONE[15 - (t >> 4)];
        if (vertical) {
            y0 -= CRYOFST[cx];
            if (y0 < 0) y0 = 0;
        }
        m->s.y = (uint8_t)y0;
        if (y0 == 0) {
            m->stat = 0x80;
            continue;
        }
        if (!vertical) x0 += CRXOFST[cx];
        m->s.x = (uint8_t)x0;
        if (m->stat & 0x80) m->s.y = 0;
        else m->stat = 0;
    }
    if (G.bonzo) {
        vx = NUMVEG + 1;
    } else {
        vx = G.racknum[px];
        if (vx) { vx--; if (vx > NUMVEG) vx = NUMVEG; }
    }
    G.vegcol[0] = VEGCOL1[vx];
    G.vegcol[1] = VEGCOL2[vx];
    G.vegcol[2] = VEGCOL3[vx];
    G.fruit.cset = 1;
    rk = G.racknum[px];
    x = rk ? (uint8_t)(rk - 1) : 0;
    if (rk <= 1 && !G.notunnel && (G.attract || !G.bonzo)) {
        SETDIRT(0x2C, DHMIDDLE);
        SETDIRT(0xA5, DHMIDDLE);
        SETDIRT(0x2D, DRHTEND);
        SETDIRT(0xA6, DRHTEND);
        SETDIRT(0x42, DVMIDDLE); SETDIRT(0x52, DVMIDDLE);
        SETDIRT(0xBA, DVMIDDLE); SETDIRT(0xCA, DVMIDDLE);
        SETDIRT(0x62, DBOTEND);
        SETDIRT(0xDA, DBOTEND);
    }
    G.dirtScheme = (uint8_t)((x % 12) & 0xC);
    video_palette_changed();
    // INCREATS
    dostart();
    G.dd.prior = 0;
    for (i = 0; i < 8; ++i) G.mon[i].s.prior = 0;
    for (i = 0; i < 8; ++i) {
        monster_t* m = &G.mon[i];
        uint8_t idx, c, t, d;
        m->blwstat = 0;
        m->ghost = 0;
        m->ymodc = 0;
        m->facing = 8;
        c = game_getdirt(m, &idx);
        t = (uint8_t)(game_chkother(c, idx) >> 1);
        d = 4;
        if (t & 1) {
            d = 8;
            if (t & 2) {
                d = 1;
                if (t & 4) {
                    d = 2;
                    if (t & 8) d = 0;
                }
            }
        }
        m->dir = d;
        game_newseq(m, d);
        game_speeder(m);
    }
    game_getmode();
}

static void dostart(void) {
    uint8_t r = G.bonzo ? 1 : rack();
    G.nxtsec = (uint8_t)(G.frmcnt + 60);
    G.speedup = r < 24 ? 67 : 2;
    setdfast();
    zero20();
    G.pumpie = -1;
    G.flee = 0;
    G.fast = 0;
    G.lastmove = 0;
    G.proport = G.proport2 = G.proport3 = PROPORT_INIT;
    G.ghostout = 4;
}

static void initrock(void) {
    uint8_t eff = G.effrack[G.playnum];
    uint8_t used = 0;
    int8_t y;
    for (y = 4; y >= 0; --y) {
        rock_t* r = &G.rock[y];
        uint8_t slot = (uint8_t)(4 - y);
        uint8_t idx = 0;
        if (eff >= MAXEFFR) {
            if (game_random() & 1) {
                uint8_t k;
                do { k = game_random() & 7; } while (used & (1 << k));
                used |= (uint8_t)(1 << k);
                idx = RANDROKS[k];
            }
        } else {
            idx = ROCKS[eff][slot];
        }
        if (idx) {
            r->s.x = (uint8_t)((idx & 0x0F) * 8);
            r->s.y = TOPZONE[15 - (idx >> 4)];
        } else {
            r->s.x = r->s.y = 0;
        }
        r->s.ix = STRTROCK;
        r->tumble = 0;
        r->stat = 0;
        r->numsqsh = 0;
    }
}

// ------------------------------------------------------------- sequences ---
static void pushSeq(uint8_t type, uint8_t arg) {
    seq_t* s;
    if (G.nseq >= 4) return;
    s = &G.seqs[G.nseq++];
    s->type = type; s->step = 0; s->arg = arg; s->x = 0; s->cnt = 0;
}

static void popSeq(void) {
    if (G.nseq) G.nseq--;
}

static void goAttract(void) {
    if (G.attract) {
        G.gameOver = 1;
        return;
    }
    G.attract = 1;
    G.ignore = 1;
    G.gameOver = 1;
    G.message = 2;
    G.messageBlank = 1;
    G.nonoise = 1;
}

static void snapshotSwap(void) {
    uint8_t i;
    uint8_t sx[19], sy[19], six[19], tumble[5], rstat[5], numsqsh[5], monstat[8];
    uint8_t hitrock = G.hitrock, fallcnt = G.fallcnt, brcreat = G.brcreat, brtunnl = G.brtunnl;
    for (i = 0; i < 19; ++i) { sx[i] = ALL_STAMPS[i]->x; sy[i] = ALL_STAMPS[i]->y; six[i] = ALL_STAMPS[i]->ix; }
    for (i = 0; i < 5; ++i) { tumble[i] = G.rock[i].tumble; rstat[i] = G.rock[i].stat; numsqsh[i] = G.rock[i].numsqsh; }
    for (i = 0; i < 8; ++i) monstat[i] = G.mon[i].stat;
    if (G.backValid) {
        for (i = 0; i < 19; ++i) { ALL_STAMPS[i]->x = G.bsx[i]; ALL_STAMPS[i]->y = G.bsy[i]; ALL_STAMPS[i]->ix = G.bsix[i]; }
        for (i = 0; i < 5; ++i) { G.rock[i].tumble = G.btumble[i]; G.rock[i].stat = G.brstat[i]; G.rock[i].numsqsh = G.bnumsqsh[i]; }
        for (i = 0; i < 8; ++i) G.mon[i].stat = G.bmonstat[i];
        G.hitrock = G.bhitrock; G.fallcnt = G.bfallcnt; G.brcreat = G.bbrcreat; G.brtunnl = G.bbrtunnl;
    }
    memcpy(G.bsx, sx, 19); memcpy(G.bsy, sy, 19); memcpy(G.bsix, six, 19);
    memcpy(G.btumble, tumble, 5); memcpy(G.brstat, rstat, 5); memcpy(G.bnumsqsh, numsqsh, 5);
    memcpy(G.bmonstat, monstat, 8);
    G.bhitrock = hitrock; G.bfallcnt = fallcnt; G.bbrcreat = brcreat; G.bbrtunnl = brtunnl;
    G.backValid = 1;
}

static void seqRackend(seq_t* s) {
    if (s->step == 0) {
        noise(SNG13);
        G.freeze = 154;
        s->step = 1;
        return;
    }
    G.racknum[G.playnum]++;
    initrack();
    G.lastmove = 0;
    ddmiddle();
    newDlst();
    popSeq();
}

static void seqPlayredy(seq_t* s) {
    uint8_t arg;
    if (s->step == 0) {
        G.nonoise = 1;
        G.message = 1;
        G.messageBlank = 1;
        G.freeze = 100;
        s->step = 1;
        return;
    }
    G.message = 0;
    G.messageBlank = 0;
    G.nonoise = 0;
    arg = s->arg;
    popSeq();
    if (arg == 1) afterInitGame();
}

static void seqDeath(seq_t* s) {
    switch (s->step) {
        case 0:
            if (G.fruitc > 0 && G.fruitc < 0x0F) game_dfruit();
            if (G.needdir) G.rock[G.hitrock].tumble = ONEDGE;
            G.freeze = 40;
            s->step = 1;
            return;
        case 1: {
            int8_t x;
            for (x = NUMCREAT; x >= 0; --x) {
                monster_t* m = &G.mon[x];
                if ((m->stat & 0x86) == 2) {
                    if (m->blwstat == 4) carol(m);
                    game_offscr2(m);
                }
                m->s.y = 0;
            }
            G.flame.y = 0;
            G.fruit.y = 0;
            G.freeze = 8;
            s->step = 2;
            return;
        }
        case 2:
            noise(SNG14);
            s->x = DANIMIX[G.lastmove];
            s->cnt = 5;
            s->step = 3;
            /* fall through */
        case 3:
            if (s->cnt > 0) {
                G.dd.ix = ADEATH[s->x];
                s->x--;
                s->cnt--;
                G.freeze = 18;
                return;
            }
            s->step = 4;
            /* fall through */
        case 4:
            s->step = 5;
            if (testrackCondition()) {
                pushSeq(SEQ_RACKEND, 0);
                return;
            }
            /* fall through */
        case 5: {
            uint8_t px = G.playnum;
            G.death = 0;
            G.nummen[px]--;
            if (G.nummen[px] == 0 && (G.numplayr == 0 || G.attract)) {
                popSeq();
                goAttract();
                return;
            }
            if (!G.attract && G.numplayr) {
                uint8_t other = px ^ 1;
                if (G.nummen[other]) {
                    popSeq();
                    pushSeq(SEQ_NEWPLAYR, other);
                    return;
                }
                if (G.nummen[px] == 0) {
                    popSeq();
                    goAttract();
                    return;
                }
            }
            s->step = 6;
            if (!G.attract && G.numplayr) {
                pushSeq(SEQ_PLAYREDY, 0);
                return;
            }
        }
            /* fall through */
        default:
            G.notunnel = 1;
            initdirt();
            ddmiddle();
            G.lastmove = 0;
            G.freeze = 40;
            popSeq();
            return;
    }
}

static void seqNewplayr(seq_t* s) {
    switch (s->step) {
        case 0: {
            uint8_t i;
            G.playnum = s->arg;
            for (i = 0; i < 255; ++i) { uint8_t t = G.dirtmap[i]; G.dirtmap[i] = G.backdirt[i]; G.backdirt[i] = t; }
            { uint8_t t = G.dirtmap[255]; G.dirtmap[255] = G.backdirt[255]; G.backdirt[255] = t; }
            video_redraw_all();
            snapshotSwap();
            for (i = 0; i < 5; ++i) G.rock[i].s.ix = STRTROCK;
            if (!G.p2init) {
                G.p2init = 1;
                G.entry = 1;
                s->step = 1;
            } else {
                s->step = 2;
            }
            pushSeq(SEQ_PLAYREDY, 0);
            return;
        }
        case 1:
            initrack();
            G.lastmove = 0;
            G.freeze = 40;
            popSeq();
            return;
        default:
            G.notunnel = 1;
            initdirt();
            ddmiddle();
            G.lastmove = 0;
            G.freeze = 40;
            popSeq();
            return;
    }
}

static void runSeq(void) {
    seq_t* s = &G.seqs[G.nseq - 1];
    switch (s->type) {
        case SEQ_DEATH: seqDeath(s); break;
        case SEQ_RACKEND: seqRackend(s); break;
        case SEQ_PLAYREDY: seqPlayredy(s); break;
        default: seqNewplayr(s); break;
    }
}
