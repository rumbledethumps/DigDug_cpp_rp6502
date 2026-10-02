#include "sound.h"
#include "game.h"
#include "xram.h"
#include <rp6502.h>

uint8_t sound_muted;
static uint8_t sound_paused;

// ------------------------------------------------------------- song data ---
// byte: bits 0-4 AUDF, bits 5-6 extra repeats, bit 7 rest after the note
static const uint8_t F0_1[] = {0x0F, 0x0E, 0x0F, 0x0E, 0x0F, 0x0E, 0x0F, 0x0E, 0x1F, 0x1E, 0x1F, 0x1E, 0x1F, 0x1E, 0x1F, 0x1E};
static const uint8_t F0_2[] = {0x8F, 0x8F, 0xCF, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0x8F, 0xCF, 0x8F, 0x8F, 0x8F, 0x32,
                               0xD1, 0xD1, 0x31, 0x32, 0x31, 0x2F, 0xD1, 0xD1, 0x31, 0x32, 0x31, 0x10};
static const uint8_t F0_4[] = {0x60};
static const uint8_t F0_5[] = {0x1B, 0x18, 0x15, 0x14, 0x12, 0x10, 0x0E, 0x0D};
static const uint8_t F0_6[] = {0x14, 0x12, 0x14, 0x13, 0x0F, 0x0E, 0x0F, 0x0E};
static const uint8_t F0_7[] = {0x30, 0x2F, 0x2E, 0x2D, 0x2E};
static const uint8_t F0_8[] = {0x0B, 0x08, 0x06, 0x0B, 0x08, 0x06, 0x0B, 0x08, 0x06};
static const uint8_t F0_9[] = {0x9A, 0x99, 0x98, 0x97, 0x96, 0x95, 0x94, 0x93, 0x32, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C};
static const uint8_t F0_10[] = {0x1F, 0x1E, 0x1F, 0x1E, 0x1F, 0x1E, 0x1F, 0x1E, 0x0F, 0x0E, 0x0F, 0x10, 0x11, 0x10, 0x0E,
                                0x0D, 0x0E, 0x0F, 0x0E, 0x0F};
static const uint8_t F0_11[] = {0x1E, 0x1C, 0x1E, 0x1C, 0x14, 0x15, 0x13, 0x14};
static const uint8_t F0_12[] = {0x0F, 0x12, 0x15, 0x19, 0x0F};
static const uint8_t F0_13[] = {0x97, 0x97, 0x9C, 0x97, 0x94, 0x92, 0x91, 0x8F, 0x8D, 0x8D, 0x8D, 0x8D, 0x60, 0x40};
static const uint8_t F0_14[] = {0x12, 0x11, 0x10, 0x0F, 0x0D, 0x0F};
static const uint8_t F0_15[] = {0x17, 0x14, 0x12, 0x17, 0x9F, 0x8D, 0x6F};
static const uint8_t F0_16[] = {0x8F, 0x92, 0x91, 0x92, 0x8E, 0x92, 0x91, 0x92, 0x8D, 0x92, 0x91, 0x12};
static const uint8_t F0_17[] = {0x12, 0x11, 0x10, 0x8F, 0x12, 0x91, 0x14, 0x92, 0x17, 0x94, 0x18, 0x17, 0x97, 0x2B, 0x17};
static const uint8_t F0_18[] = {0x0D, 0x0E, 0x0F, 0x0E, 0x0F, 0x10, 0x0F, 0x10, 0x11, 0x10, 0x11, 0x12, 0x33, 0x2C};
static const uint8_t F0_19[] = {0x8F, 0x8F, 0x8F, 0x6F, 0x72, 0x2F, 0x72, 0x2F, 0x2D, 0x92, 0x92, 0x72, 0x34, 0x77, 0x76,
                                0x7F, 0x36, 0x7F, 0x76, 0x94, 0x94, 0x54};
static const uint8_t F1_2[] = {0xCF, 0xCF, 0xD0, 0xD0, 0xD1, 0xD1, 0xD2, 0xD2, 0xD3, 0xD3, 0xD3, 0xD3, 0xD4, 0xD4, 0xD2, 0x50};
static const uint8_t F1_5[] = {0x1C, 0x19, 0x16, 0x15, 0x13, 0x11, 0x0F, 0x0E};
static const uint8_t F1_7[] = {0x8A, 0x8A, 0x8A, 0x8A, 0x8A};
static const uint8_t F1_12[] = {0x0E, 0x11, 0x14, 0x18, 0x0E};
static const uint8_t F1_13[] = {0x8B, 0x8F, 0x20, 0x8F, 0x8B, 0x8F, 0x20, 0x8F, 0x8B, 0x8F, 0x20, 0x8F, 0x20, 0x8F, 0x8F, 0x0F};
static const uint8_t F1_16[] = {0x20, 0x93, 0x93, 0x93, 0x20, 0x93, 0x93, 0x93, 0x20, 0x93, 0x93, 0x13};
static const uint8_t F1_17[] = {0x40, 0x94, 0x00, 0x93, 0x00, 0x92, 0x00, 0x90, 0x00, 0x8F, 0x40, 0x0F};
static const uint8_t F1_18[] = {0x0E, 0x0F, 0x10, 0x0F, 0x10, 0x11, 0x10, 0x11, 0x12, 0x11, 0x12, 0x13, 0x34, 0x2D};
static const uint8_t F1_19[] = {0x34, 0x32, 0x30, 0x2F, 0x34, 0x2F, 0x34, 0x2F, 0x34, 0x2F, 0x34, 0x2F, 0x34, 0x2F, 0x34, 0x2F,
                                0x34, 0x2F, 0x34, 0x31, 0x94, 0x94, 0x94, 0x31, 0x34, 0x31, 0x34, 0x30, 0x34, 0x30, 0x34, 0x10};

static const uint8_t* const SONG_F0[20] = {0, F0_1, F0_2, F0_2, F0_4, F0_5, F0_6, F0_7, F0_8, F0_9,
                                           F0_10, F0_11, F0_12, F0_13, F0_14, F0_15, F0_16, F0_17, F0_18, F0_19};
static const uint8_t SONG_F0_LEN[20] = {0, 16, 26, 26, 1, 8, 8, 5, 9, 15, 20, 8, 5, 14, 6, 7, 12, 15, 14, 22};
static const uint8_t* const SONG_F1[20] = {0, 0, F1_2, F1_2, 0, F1_5, 0, F1_7, 0, 0,
                                           0, 0, F1_12, F1_13, 0, 0, F1_16, F1_17, F1_18, F1_19};
static const uint8_t SONG_F1_LEN[20] = {0, 0, 16, 16, 0, 8, 0, 5, 0, 0, 0, 0, 5, 16, 0, 0, 12, 12, 14, 32};
static const uint8_t SNGC0[20] = {0xD4, 0x04, 0xD4, 0xD4, 0x00, 0x11, 0x0D, 0x47, 0x04, 0x04,
                                  0x04, 0x00, 0xD4, 0xD4, 0x04, 0x04, 0xDD, 0xD4, 0xDD, 0xD4};
static const uint8_t SNGVV0[20] = {0xD4, 0x03, 0x22, 0x22, 0x00, 0x33, 0x03, 0x33, 0x03, 0x00,
                                   0x04, 0x07, 0x74, 0x34, 0x07, 0x05, 0x33, 0x44, 0x00, 0x44};
static const uint8_t SNGDM[20] = {22, 1, 2, 1, 7, 1, 1, 1, 2, 0, 0, 2, 3, 3, 2, 6, 3, 6, 3, 4};
static const uint8_t SNG7C0[8] = {0x0D, 0x0D, 0x0D, 0x0D, 0x04, 0x04, 0x04, 0x04};
static const uint8_t SNG4V0[15] = {10, 10, 10, 10, 10, 10, 10, 10, 4, 4, 4, 4, 4, 4, 4};
static const uint8_t SNG14V0[14] = {6, 6, 6, 5, 5, 5, 4, 4, 4, 3, 3, 3, 2, 8};
static const uint8_t SNG14V1[14] = {5, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 1, 8};

// ------------------------------------------------------------- sequencer ---
static uint8_t cursong, sngtemp1;
static int8_t curindx[2], tunindx[2], durctr;
static uint8_t rest[2];
static int8_t saved[7];
static uint8_t audc[2], audf[2], audv[2];
static uint8_t lastc[2], lastf[2], lastv[2];

#define isDD(s) ((s) == SNGD || (s) == SNGDF)

static void savdun(void) {
    saved[0] = curindx[0]; saved[1] = curindx[1];
    saved[2] = (int8_t)rest[0]; saved[3] = (int8_t)rest[1];
    saved[4] = durctr;
    saved[5] = tunindx[0]; saved[6] = tunindx[1];
}

static void restoreDD(void) {
    curindx[0] = saved[0]; curindx[1] = saved[1];
    rest[0] = (uint8_t)saved[2]; rest[1] = (uint8_t)saved[3];
    durctr = saved[4];
    tunindx[0] = saved[5]; tunindx[1] = saved[6];
}

static void initalls(void) {
    curindx[0] = curindx[1] = 0;
    rest[0] = rest[1] = 0;
    durctr = 0;
    tunindx[0] = tunindx[1] = -1;
}

static void initdigs(void) {
    uint8_t i;
    for (i = 0; i < 5; ++i) saved[i] = 0;
    saved[5] = saved[6] = -1;
}

void sound_init(void) {
    cursong = sngtemp1 = 0;
    initalls();
    initdigs();
    audv[0] = audv[1] = 0;
    lastv[0] = lastv[1] = 0xFF;
    // clear the PSG block and enable it
    xram0_set(XRAM_PSG, 0, sizeof(psg_t));
    xreg_ria_psg(XRAM_PSG);
}

void sound_request(uint8_t song) {
    if (song > sngtemp1) sngtemp1 = song;
}

static void psg_write(void);

void sound_stop(void) {
    cursong = 0;
    sngtemp1 = 0;
    audv[0] = audv[1] = 0;
    psg_write();
}

void sound_pause(uint8_t on) {
    sound_paused = on;
    psg_write();
}

static void newsong(uint8_t walking, uint8_t fast, uint8_t dying, uint8_t entry) {
    uint8_t x = cursong;
    audv[0] = audv[1] = 0;
    sngtemp1 = 0;
    cursong = 0;
    if (x >= 5) { sound_request(SNGNULL); return; }
    if (dying || entry) { sound_request(SNGNULL); return; }
    if (walking) { sound_request(fast ? SNGDF : SNGD); return; }
    if (x == SNGNULL || x == SNGC) { sound_request(SNGC); return; }
    sound_request(SNGNULL);
}

// returns 1 when the song ended
static uint8_t decode(void) {
    uint8_t x = cursong;
    const uint8_t* f0 = SONG_F0[x];
    const uint8_t* f1;
    uint8_t b, vol;
    curindx[0]--;
    if (curindx[0] < 0) {
        if (rest[0] & 0x80) {
            rest[0] = 0;
            audv[0] = 0;
            curindx[0] = 0;
        } else {
            tunindx[0]++;
            if (tunindx[0] >= (int8_t)SONG_F0_LEN[x]) {
                if (isDD(x)) initdigs();
                return 1;
            }
            b = f0[tunindx[0]];
            rest[0] = b;
            audf[0] = (uint8_t)(b & 0x1F);
            curindx[0] = (int8_t)((b >> 5) & 3);
            if ((b & 0x1F) == 0) {
                vol = 0;
            } else {
                vol = (uint8_t)(SNGVV0[x] & 0x0F);
                if (vol == 0) vol = (x == SNG4) ? SNG4V0[tunindx[0] % 15] : SNG14V0[tunindx[0] % 14];
            }
            audv[0] = vol;
            audc[0] = (uint8_t)(SNGC0[x] & 0x0F);
            if (audc[0] == 0) audc[0] = SNG7C0[tunindx[0] & 7];
        }
    }
    durctr = (int8_t)SNGDM[x];
    f1 = SONG_F1[x];
    if (!f1) {
        audv[1] = 0;
        return 0;
    }
    curindx[1]--;
    if (curindx[1] >= 0) return 0;
    if (rest[1] & 0x80) {
        rest[1] = 0;
        curindx[1] = 0;
        audv[1] = 0;
        return 0;
    }
    tunindx[1]++;
    b = f1[tunindx[1] % SONG_F1_LEN[x]];
    rest[1] = b;
    audf[1] = (uint8_t)(b & 0x1F);
    curindx[1] = (int8_t)((b >> 5) & 3);
    if ((b & 0x1F) == 0) {
        vol = 0;
    } else {
        vol = (uint8_t)((SNGVV0[x] >> 4) & 0x0F);
        if (vol == 0) vol = SNG14V1[tunindx[1] % 14];
    }
    audv[1] = vol;
    audc[1] = (uint8_t)((SNGC0[x] >> 4) & 0x0F);
    return 0;
}

// TIA volume is linear and PSG attenuation is logarithmic, so each TIA
// volume takes the attenuation of the nearest PSG level.
static const uint8_t ATTEN[16] = {15, 13, 11, 9, 7, 6, 5, 4, 3, 3, 2, 1, 1, 1, 0, 0};

// TIA register values -> PSG channel
static void psg_channel(uint8_t ch) {
    psg_channel_t chan;
    uint8_t c = audc[ch], f = audf[ch], v = audv[ch];
    uint16_t hz3;
    uint8_t wave = PSG_WAVE_SQUARE;
    uint8_t duty = 128;
    if (sound_muted || sound_paused) v = 0;
    if (c == lastc[ch] && f == lastf[ch] && v == lastv[ch]) return;
    lastc[ch] = c; lastf[ch] = f; lastv[ch] = v;
    switch (c) {
        case 4: case 5: hz3 = (uint16_t)(47100UL / (f + 1)); break;                       // 31400/2 * 3
        case 12: case 13: hz3 = (uint16_t)(15700UL / (f + 1)); break;                     // 31400/6 * 3
        case 6: case 10: hz3 = (uint16_t)(3038UL / (f + 1)); break;                       // /31
        case 1: hz3 = (uint16_t)(6280UL / (f + 1)); wave = PSG_WAVE_NOISE; break;         // 4-bit poly ~ noise
        case 7: case 9: hz3 = (uint16_t)(3038UL / (f + 1)); wave = PSG_WAVE_NOISE; break; // 5-bit poly
        case 8: hz3 = (uint16_t)(94200UL / (f + 1) / 8); wave = PSG_WAVE_NOISE; break;    // 9-bit poly
        case 15: case 3: case 2: hz3 = (uint16_t)(6280UL / (f + 1)); wave = PSG_WAVE_NOISE; break;
        case 14: hz3 = (uint16_t)(1012UL / (f + 1)); break;
        default: v = 0; hz3 = 0; break;
    }
    chan.freq = hz3;
    chan.duty = duty;
    chan.attack = (uint8_t)(ATTEN[v] << 4);       // attack: volume, fastest rate
    chan.decay = chan.attack;                     // decay: same volume (sustain)
    chan.release_wave = wave;                     // waveform, release rate
    chan.pan_gate = v ? PSG_GATE : 0;             // pan centre, gate
    xram0_write(XRAM_PSG + ch * sizeof(psg_channel_t), &chan, offsetof(psg_channel_t, reserved));
}

static void psg_write(void) {
    psg_channel(0);
    psg_channel(1);
}

void sound_frame(uint8_t walking, uint8_t fast, uint8_t dying, uint8_t entry, uint8_t nonoise) {
    uint8_t doSwitch = 0;
    if (nonoise) {
        audv[0] = audv[1] = 0;
        cursong = 0;
        sngtemp1 = 0;
        psg_write();
        return;
    }
    if (cursong == sngtemp1) {
        if (cursong == SNG1 || cursong == SNG4) doSwitch = 1;
    } else if (sngtemp1 > cursong) {
        doSwitch = 1;
    }
    if (doSwitch) {
        if (isDD(cursong)) savdun();
        cursong = sngtemp1;
        sngtemp1 = 0;
        if (isDD(cursong)) restoreDD();
        else initalls();
    }
    if (cursong == 0) {
        newsong(walking, fast, dying, entry);
        psg_write();
        return;
    }
    if (isDD(cursong) && (!walking || dying)) {
        savdun();
        newsong(walking, fast, dying, entry);
        psg_write();
        return;
    }
    durctr--;
    if (durctr < 0 && decode()) newsong(walking, fast, dying, entry);
    psg_write();
}
