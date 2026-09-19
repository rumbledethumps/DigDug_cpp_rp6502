// Pooka / Fygar behaviour (MONSTER.S)
#include "game.h"
#include "tables.h"

#define noise(n) sound_request(n)
#define rack() (G.racknum[G.playnum])

static void mode(monster_t* m, uint8_t a);
static void ghoster(monster_t* m, uint8_t a);

static uint8_t mshift(monster_t* m) {
    uint8_t c = (uint8_t)((m->speed >> 15) & 1);
    m->speed = (uint16_t)((m->speed << 1) | c);
    return c;
}

static void dispchar(monster_t* m) {
    uint8_t n = m->seqn;
    if (n >= m->seqlen) n = 0;
    m->s.ix = m->seq[n];
    m->seqn = (uint8_t)(n + 1);
}

uint8_t game_getdirt(const monster_t* m, uint8_t* idx) {
    *idx = game_getdirt_idx(m->s.x, m->s.y);
    return G.dirtmap[*idx];
}

uint8_t game_chkother(uint8_t c, uint8_t idx) {
    uint8_t t = c;
    if (!(c & 2) && G.dirtmap[(uint8_t)(idx - 16)] >= CHARD12) t |= 2;
    if (!(c & 4) && G.dirtmap[(uint8_t)(idx + 1)] >= CHARD12) t |= 4;
    if (!(c & 8) && G.dirtmap[(uint8_t)(idx + 16)] >= CHARD12) t |= 8;
    if (!(c & 16) && G.dirtmap[(uint8_t)(idx - 1)] >= CHARD12) t |= 16;
    return t;
}

static uint8_t norev(const monster_t* m) {
    uint8_t d = m->dir;
    if (d & 0x0A) return (uint8_t)(d | 5);
    return (uint8_t)(d | 0x0A);
}

static uint8_t getpoint(uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1) {
    uint8_t p = 0;
    int16_t dx = (int16_t)x0 - x1, dy = (int16_t)y0 - y1;
    if (dx > 0) p |= 2;
    else if (dx < 0) p |= 8;
    if (dy > 0) p |= 1;
    else if (dy < 0) p |= 4;
    G.gp_dx = dx;
    G.gp_dy = dy;
    return p;
}

static uint8_t sub11(const monster_t* m) {
    return getpoint(m->s.x, G.dd.x, m->s.y, G.dd.y);
}

void game_newseq(monster_t* m, uint8_t d) {
    uint8_t i;
    if (d & 0x0A) m->facing = (uint8_t)(d & 0x0A);
    i = (uint8_t)((((m->index >= 4 ? 4 : 0) | m->facing) >> 1) ^ (m->ghost ? 1 : 0)) & 7;
    m->seq = SEQTAB[i];
    m->seqlen = SEQLEN[i];
    m->seqn = 0;
    m->s.ix = m->seq[0];
}

void game_speeder(monster_t* m) {
    uint8_t i = (uint8_t)((m->index >= 4 ? 4 : 0) | (G.fast ? 2 : 0) | (m->ghost ? 1 : 0));
    m->speed = SPEEDTAB[i];
}

static uint8_t sub1(monster_t* m, uint8_t a, uint8_t* r) {
    uint8_t p = sub11(m);
    if (m->ghost) {
        m->dir = p;
        if (p & 0x0A) m->facing = (uint8_t)(p & 0x0A);
        return 0;
    }
    *r = (uint8_t)(p & a);
    if (!*r) *r = a;
    return 1;
}

static void pick(monster_t* m, uint8_t r, const uint8_t* order) {
    uint8_t i;
    for (i = 0; i < 4; ++i) {
        if (r & order[i]) { r = order[i]; break; }
    }
    m->dir = r;
    game_newseq(m, r);
}

static const uint8_t ORDER_VERT[4] = {1, 4, 2, 8};
static const uint8_t ORDER_HORIZ[4] = {8, 2, 4, 1};
static const uint8_t ORDER_DVERT[4] = {4, 1, 8, 2};

static void escont(monster_t* m, uint8_t r) {
    int16_t ady = G.gp_dy < 0 ? -G.gp_dy : G.gp_dy;
    int16_t adx = G.gp_dx < 0 ? -G.gp_dx : G.gp_dx;
    if (ady >= adx) pick(m, r, ORDER_DVERT);
    else pick(m, r, ORDER_HORIZ);
}

void game_offscr2(monster_t* m) {
    m->stat = 0x80;
    m->s.y = 0;
}

static void offscr(monster_t* m) {
    m->ghost = 0;
    m->dir = 2;
    game_newseq(m, 2);
    game_speeder(m);
    if (m->s.x >= 128 || m->s.x == 0) game_offscr2(m);
}

static void scram(monster_t* m) {
    m->mode = MODE_OFFSCR;
    offscr(m);
}

static void escape(monster_t* m, uint8_t a) {
    uint8_t p = getpoint(m->s.x, 0x10, m->s.y, 0xBF);
    uint8_t r;
    if (p == 0) { scram(m); return; }
    if (m->ghost) {
        m->dir = p;
        if (p & 0x0A) m->facing = (uint8_t)(p & 0x0A);
        return;
    }
    r = (uint8_t)(p & a);
    if (r == 0) {
        if (m->s.y < 0xBF) {
            m->ghost = 1;
            m->dir = 4;
            game_newseq(m, 4);
            game_speeder(m);
            return;
        }
        if (m->s.x >= 0x10) {
            m->dir = 2;
            game_newseq(m, 2);
            game_speeder(m);
            return;
        }
        scram(m);
        return;
    }
    escont(m, r);
}

static void mode(monster_t* m, uint8_t a) {
    uint8_t r;
    switch (m->mode) {
        case MODE_VERT:
            if (sub1(m, a, &r)) pick(m, r, ORDER_VERT);
            return;
        case MODE_HORIZ:
            if (sub1(m, a, &r)) pick(m, r, ORDER_HORIZ);
            return;
        case MODE_DIAG:
            if (sub1(m, a, &r)) escont(m, r);
            return;
        case MODE_ESCAPE:
            escape(m, a);
            return;
        default:
            offscr(m);
            return;
    }
}

static void ghoster2(monster_t* m, uint8_t d) {
    m->dir = d;
    game_newseq(m, d);
    if (m->mode == MODE_ESCAPE) mode(m, m->dir);
}

static void znxt9(monster_t* m, uint8_t a, uint8_t ghosttmp, uint8_t v) {
    uint8_t p;
    G.ghostime = v;
    G.digrest = 0;
    p = sub11(m);
    if (a == ghosttmp || (p & a) == 0) {
        G.ghostout = 4;
        m->ghost = 1;
        m->dir = p;
        game_newseq(m, p);
        game_speeder(m);
        return;
    }
    ghoster2(m, a);
}

static void ghoster(monster_t* m, uint8_t a) {
    uint8_t ghosttmp = (uint8_t)((~norev(m)) & 0x0F);
    uint8_t rk;
    uint16_t v;
    if (m->mode == MODE_ESCAPE) { ghoster2(m, a); return; }
    rk = G.bonzo ? 1 : rack();
    v = (uint16_t)(((rk * 2) & 0xFF) + 10);
    if (v <= 255) {
        v += G.flee;
        if (v <= 255) {
            v += G.ghostime;
            if (v <= 255) {
                G.ghostime = (uint8_t)v;
                v += (uint8_t)(G.digrest >> 5);
                if (v <= 255) {
                    G.ghostime = (uint8_t)v;
                    if (G.ghostout) { ghoster2(m, a); return; }
                    znxt9(m, a, ghosttmp, (uint8_t)v);
                    return;
                }
            }
        }
    }
    znxt9(m, a, ghosttmp, (uint8_t)v);
}

static void movecont(monster_t* m) {
    uint8_t idx, c, t, i, nr, below, t1, a;
    c = game_getdirt(m, &idx);
    t = game_chkother(c, idx);
    i = (uint8_t)(t >> 1);
    if (i > 15) i = 15;
    nr = norev(m);
    switch (i) {
        case 0: mode(m, nr); return;
        case 1: mode(m, nr & 0x0B); return;
        case 2: mode(m, nr & 0x07); return;
        case 3: ghoster(m, nr & 0x03); return;
        case 4: mode(m, nr & 0x0E); return;
        case 5: case 10: return;
        case 6: ghoster(m, nr & 0x06); return;
        case 7: ghoster(m, 2); return;
        case 8: mode(m, nr & 0x0D); return;
        case 9: ghoster(m, nr & 0x09); return;
        case 11: ghoster(m, 1); return;
        case 12: ghoster(m, nr & 0x0C); return;
        case 13: ghoster(m, 8); return;
        case 14: ghoster(m, 4); return;
        default: break;
    }
    below = G.dirtmap[(uint8_t)(idx + 16)];
    t1 = (uint8_t)(((below & 0xE2) ? 1 : 0) ^ 0x0B);
    a = (uint8_t)(nr & t1);
    if (idx == 1) { mode(m, a & 0xFD); return; }
    if (idx == 0x0E) { ghoster(m, a & 0xF7); return; }
    mode(m, a);
}

void game_setflam(void) {
    G.flamsize = 0;
    G.flamnum = -24;
    G.flamwait = 7;
}

static void movemode(monster_t* m) {
    uint16_t ft;
    movecont(m);
    if (m->stat & 0x80) return;
    if (m->index < 4 || m->ghost || !(m->dir & 0x0A)) return;
    ft = (uint16_t)G.flamtime + 16;
    G.flamtime = (uint8_t)ft;
    if (ft < 256) return;
    if (G.flamie) return;
    G.flame.ix = ZBLANK;
    m->stat = 1;
    game_setflam();
    G.flamie = m->index;
    G.flame.y = m->s.y;
    G.flame.x = (uint8_t)(m->s.x + 8);
}

static void ghostonx(monster_t* m) {
    uint8_t idx, c, above, t;
    c = game_getdirt(m, &idx);
    if (c >= CHARD12) { mode(m, c); return; }
    if (c >= CHARDF) {
        m->ghost = 0;
        game_speeder(m);
        mode(m, 0x0A);
        return;
    }
    above = G.dirtmap[(uint8_t)(idx - 16)];
    if (above >= CHARD12) { mode(m, above); return; }
    if (((above >> 2) | c) & 2) { mode(m, (uint8_t)((above >> 2) | c)); return; }
    m->ghost = 0;
    game_speeder(m);
    if (m->ymodc != 0) { mode(m, 5); return; }
    t = game_chkother(c, idx);
    if ((t >> 1) & 4) {
        m->dir = 4;
        mode(m, 4);
        return;
    }
    mode(m, 5);
}

static void ghostony(monster_t* m) {
    uint8_t idx, c, right;
    c = game_getdirt(m, &idx);
    if (c >= CHARD12) { mode(m, c); return; }
    if (c >= CHARDF) {
        m->ghost = 0;
        game_speeder(m);
        mode(m, 0x0A);
        return;
    }
    right = G.dirtmap[(uint8_t)(idx + 1)];
    if (right >= CHARD12) { mode(m, right); return; }
    if (((right >> 2) | c) & 4) { mode(m, (uint8_t)((right >> 2) | c)); return; }
    m->ghost = 0;
    game_speeder(m);
    mode(m, 0x0A);
}

static void movemons(monster_t* m) {
    int16_t ny;
    if (m->stat != 0) return;
    if (m->dir & 0x0A) {
        if (!(G.proport & 0x80)) return;
        if (!mshift(m)) return;
        dispchar(m);
    } else {
        if (!mshift(m)) return;
        if (game_shift8(&G.proport2)) dispchar(m);
    }
    m->s.x = (uint8_t)(m->s.x + XTABLE[m->dir]);
    ny = (int16_t)m->s.y + YTABLE[m->dir];
    if (ny >= 23 && ny < 192) {
        int8_t v;
        m->s.y = (uint8_t)ny;
        v = (int8_t)(m->ymodc + YTABLE[m->dir]);
        if (v == 12) v = 0;
        else if (v == -1) v = 11;
        m->ymodc = (uint8_t)v;
    }
    if (m->ghost) {
        if ((m->s.x & 7) == 0) ghostonx(m);
        else if (m->ymodc == 0) ghostony(m);
        return;
    }
    if ((m->s.x & 7) || m->ymodc) return;
    movemode(m);
}

void game_monsters_frame(uint8_t button) {
    int8_t x;
    game_shift8(&G.proport);
    if (G.entry) return;
    game_pumper(button);
    game_fruiter();
    game_flamer();
    for (x = 7; x >= 0; --x) movemons(&G.mon[x]);
    if (G.speedup == 0 && !G.fast && !G.escaper) {
        uint8_t i;
        G.fast = 1;
        noise(SNG12);
        for (i = 0; i < 8; ++i)
            if (!(G.mon[i].stat & 0x80)) game_speeder(&G.mon[i]);
    }
}

void game_avoid(void) {
    int8_t x;
    for (x = 7; x >= 0; --x) {
        monster_t* m = &G.mon[x];
        if ((m->stat & 0x80) || m->ghost || m->dir != 4 || m->ymodc < 3) continue;
        if (G.escaper) continue;
        m->mode = MODE_VERT;
        m->dir = 1;
    }
}

void game_getmode(void) {
    int8_t x;
    int8_t y = 0;
    uint8_t rk, xi;
    for (x = 7; x >= 0; --x) {
        monster_t* m = &G.mon[x];
        if (!(m->stat & 0x80)) m->mode = MODETAB[(y++) & 7];
    }
    y -= 1;
    if (y == 0) {
        monster_t* last = 0;
        for (x = 7; x >= 0; --x)
            if (!(G.mon[x].stat & 0x80)) { last = &G.mon[x]; break; }
        if (!last) return;
        if (!(last->stat & 0x40) && (last->stat & 6) != 2) noise(SNG9);
        last->mode = MODE_ESCAPE;
        G.escaper++;
        return;
    }
    if (G.fast || G.speedup < 3) return;
    rk = G.bonzo ? 1 : rack();
    if (rk < 4) return;
    xi = (uint8_t)(rk >> 2);
    if (xi > 5) xi = 5;
    if (y >= 0 && (uint8_t)y < WHNSPEED[xi - 1]) G.speedup = 1;
}

void game_fruiter(void) {
    uint8_t a;
    if (G.fruitc == 0) return;
    if (!(G.racktime & 1)) return;
    a = G.fruitc;
    G.fruitc++;
    if (a == 0xC0) {
        G.fruitc = 0;
        G.fruit.y = 0;
    } else if (a == 0x0E) {
        game_dfruit();
    }
}

void game_flamout4(monster_t* m) {
    G.flame.y = 0;
    G.flamie = 0;
    m->stat = 0;
}

static void flamcoll(monster_t* m) {
    int16_t d;
    uint8_t a;
    if (G.squash) return;
    d = (int16_t)G.flame.x - 7 - G.dd.x;
    if (d >= 0) return;
    a = (uint8_t)(((-d) + 2) >> 3);
    if ((int8_t)a >= G.flamnum) return;
    {
        int16_t dy = (int16_t)G.dd.y - G.flame.y;
        if (dy < 0) dy = -dy;
        if (dy >= 10) return;
    }
    m->stat = 0;
    G.flamie = 0;
    game_yescoll();
}

static void flamout2(monster_t* m) {
    G.flamwait--;
    if (G.flamwait) { flamcoll(m); return; }
    G.flame.ix = ZBLANK;
    if (G.proport & 0x80) game_flamout4(m);
    else game_setflam();
}

void game_flamer(void) {
    monster_t* m;
    int8_t size;
    if (!G.flamie) return;
    m = &G.mon[G.flamie];
    if (G.flamnum < 0) {
        uint8_t t, y;
        G.flamnum++;
        t = (uint8_t)(((~(G.flamnum - 1)) & 0x0F) >> 2);
        y = (uint8_t)(t * 2 + ((m->facing & 2) ? 1 : 0));
        m->s.ix = FLAMTABL[y & 7];
        return;
    }
    if (G.flamnum == 0 && G.flamsize == 0) {
        uint8_t idx, k, n = 0, hit = 0;
        game_getdirt(m, &idx);
        if (m->facing & 2) {
            for (k = 0; k < 3; ++k) {
                idx--;
                n++;
                if (G.dirtmap[idx] >= CHARD12) { hit = 1; break; }
            }
            if (!hit) n++;
            G.flamsize = (int8_t)-(n + 1);
        } else {
            for (k = 0; k < 3; ++k) {
                idx++;
                n++;
                if (G.dirtmap[idx] >= CHARD12) { hit = 1; break; }
            }
            if (!hit) n++;
            G.flamsize = (int8_t)n;
        }
        noise(SNG3);
    }
    size = G.flamsize;
    if (size < 0) {
        uint8_t sz = (uint8_t)(~size);
        uint8_t n;
        if (sz == (uint8_t)G.flamnum) { flamout2(m); return; }
        n = (uint8_t)(G.flamnum & 3);
        G.flame.x = (uint8_t)(m->s.x - FLAMPOS[n]);
        G.flame.ix = LFLAMSEQ[n];
    } else {
        if ((uint8_t)size == (uint8_t)G.flamnum) { flamout2(m); return; }
        G.flame.ix = RFLAMSEQ[G.flamnum & 3];
    }
    G.flamnum++;
    flamcoll(m);
}
