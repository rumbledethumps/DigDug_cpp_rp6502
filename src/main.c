// Dig Dug (Atari 7800) -- RP6502 Picocomputer port.
//
// Controls
//   Arrow keys / WASD  move          Space / Z / Ctrl  pump
//   Enter              start game    Up/Down           1 or 2 players (title)
//   Left/Right         easy/normal   P pause   M mute   Escape quit
//   Gamepad: d-pad / left stick, A/B/X/Y pump, Start to begin.
#include <rp6502.h>
#include <stdint.h>
#include <stdio.h>
#include "game.h"
#include "video.h"
#include "sound.h"
#include "xram.h"

#define TITLE_TIME (60 * 12)

enum { ST_TITLE, ST_ATTRACT, ST_PLAY, ST_GAMEOVER };

static uint8_t state = ST_TITLE;
static uint16_t titleTimer, gameoverTimer;
static uint8_t numPlayers = 1, bonzo = 0, paused = 0;
static uint32_t hiscore = 10000;
static keyboard_t keyboard;
static gamepad_player_t pad;
static uint8_t prevKeys;
static uint8_t rngState = 0x5A;
static uint8_t titleDrawn;

uint8_t game_random(void) {
    uint8_t x = rngState;
    x ^= (uint8_t)(x << 3);
    x ^= (uint8_t)(x >> 5);
    x ^= (uint8_t)(x << 1);
    rngState = (uint8_t)(x + ria_vsync());
    return rngState;
}

#define KEY(code) KEYBOARD_PRESSED(keyboard.keys, code)
#define K_RIGHT 0x4F
#define K_LEFT 0x50
#define K_DOWN 0x51
#define K_UP 0x52
#define K_SPACE 0x2C
#define K_Z 0x1D
#define K_ENTER 0x28
#define K_ESC 0x29
#define K_P 0x13
#define K_M 0x10
#define K_W 0x1A
#define K_A 0x04
#define K_S 0x16
#define K_D 0x07
#define K_LCTRL 0xE0
#define K_RCTRL 0xE4

// edge-detected "menu" keys packed in a byte
#define E_ENTER 1
#define E_UP 2
#define E_DOWN 4
#define E_LEFT 8
#define E_RIGHT 16
#define E_P 32
#define E_M 64
#define E_ESC 128

static void read_input(uint8_t* joy, uint8_t* button, uint8_t* edges) {
    uint8_t now = 0;
    uint8_t j = 0xFF, b = 0;
    xram0_read(&keyboard, XRAM_KEYBOARD, sizeof keyboard);
    xram0_read(&pad, XRAM_GAMEPAD, sizeof pad);
    if (KEY(K_RIGHT) || KEY(K_D)) j &= (uint8_t)~JOY_E;
    if (KEY(K_LEFT) || KEY(K_A)) j &= (uint8_t)~JOY_W;
    if (KEY(K_DOWN) || KEY(K_S)) j &= (uint8_t)~JOY_S;
    if (KEY(K_UP) || KEY(K_W)) j &= (uint8_t)~JOY_N;
    if (KEY(K_SPACE) || KEY(K_Z) || KEY(K_LCTRL) || KEY(K_RCTRL)) b = 1;
    if (pad.dpad & GAMEPAD_FEAT_CONNECTED) {
        uint8_t d = (uint8_t)(pad.dpad | pad.sticks);   // d-pad bits or left stick digital
        if (d & GAMEPAD_DPAD_UP) j &= (uint8_t)~JOY_N;
        if (d & GAMEPAD_DPAD_DOWN) j &= (uint8_t)~JOY_S;
        if (d & GAMEPAD_DPAD_LEFT) j &= (uint8_t)~JOY_W;
        if (d & GAMEPAD_DPAD_RIGHT) j &= (uint8_t)~JOY_E;
        if (pad.btn0 & (GAMEPAD_BTN0_A | GAMEPAD_BTN0_B | GAMEPAD_BTN0_X | GAMEPAD_BTN0_Y)) b = 1;
    }
    if (KEY(K_ENTER) || (pad.btn1 & GAMEPAD_BTN1_START)) now |= E_ENTER;
    if (KEY(K_UP)) now |= E_UP;
    if (KEY(K_DOWN)) now |= E_DOWN;
    if (KEY(K_LEFT)) now |= E_LEFT;
    if (KEY(K_RIGHT)) now |= E_RIGHT;
    if (KEY(K_P)) now |= E_P;
    if (KEY(K_M)) now |= E_M;
    if (KEY(K_ESC)) now |= E_ESC;
    *edges = (uint8_t)(now & ~prevKeys);
    prevKeys = now;
    *joy = j;
    *button = b;
}

static void start_game(uint8_t attract) {
    titleTimer = 0;
    sound_stop();
    game_init(numPlayers, bonzo, attract, 1);
    state = attract ? ST_ATTRACT : ST_PLAY;
    paused = 0;
    video_text_clear();
    video_blank(0);
    video_redraw_all();
    video_score_changed();
}

static void draw_title(void) {
    video_text_clear();
    video_text(15, 4, "DIG  DUG");
    video_text(13, 5, "ATARI 7800 PORT");
    video_text(12, 9, numPlayers == 2 ? "PLAYERS   2" : "PLAYERS   1");
    video_text(12, 11, bonzo ? "LEVEL  TEDDY (EASY)" : "LEVEL  PINEAPPLE   ");
    video_text(9, 14, "UP/DOWN      PLAYERS");
    video_text(9, 15, "LEFT/RIGHT   LEVEL");
    video_text(9, 17, "ARROWS MOVE  SPACE PUMP");
    video_text(9, 20, "PRESS ENTER TO START");
    video_text(11, 23, "HIGH SCORE");
    video_text_num(22, 23, hiscore, 6);
    video_text(4, 27, "(C)1983 ATARI  (C)1982 NAMCO");
}

static void show_message(void) {
    static uint8_t last = 0xFF;
    uint8_t m = G.messageBlank ? G.message : 0;
    if (m == last) return;
    last = m;
    video_blank(m != 0);
    if (m == 1) {
        video_text_clear();
        video_text(16, 14, G.playnum ? "PLAYER 2" : "PLAYER 1");
    } else if (m == 2) {
        video_text_clear();
        video_text(15, 14, "GAME OVER");
    } else {
        video_text_clear();
        if (state == ST_ATTRACT) video_text(14, 14, "PRESS ENTER");
    }
}

int main(void) {
    uint8_t joy, button, edges;
    // enable keyboard and gamepad reporting into XRAM
    xreg_ria_keyboard(XRAM_KEYBOARD);
    xreg_ria_gamepad(XRAM_GAMEPAD);
    video_init();
    sound_init();
    for (;;) {
        video_wait_vsync();
        read_input(&joy, &button, &edges);
        if (edges & E_ESC) break;
        if (edges & E_M) {
            sound_muted = !sound_muted;
            sound_stop();
        }
        if (state == ST_TITLE) {
            if (!titleDrawn) {
                video_blank(1);
                draw_title();
                titleDrawn = 1;
            }
            if (edges & E_UP) { numPlayers = 2; draw_title(); }
            if (edges & E_DOWN) { numPlayers = 1; draw_title(); }
            if (edges & E_LEFT) { bonzo = 1; draw_title(); }
            if (edges & E_RIGHT) { bonzo = 0; draw_title(); }
            if (edges & E_ENTER) {
                titleDrawn = 0;
                start_game(0);
            } else if (++titleTimer > TITLE_TIME) {
                titleDrawn = 0;
                start_game(1);
            }
            continue;
        }
        if (state == ST_ATTRACT) {
            if (edges & E_ENTER) {
                start_game(0);
                continue;
            }
            game_frame(0xFF, 0);
            show_message();
            if (G.gameOver) {
                state = ST_TITLE;
                sound_stop();
                continue;
            }
            video_flush();
            continue;
        }
        // playing / game over
        if (edges & E_P) {
            paused = !paused;
            sound_pause(paused);
        }
        if (!paused) {
            game_frame(joy, button);
            if (G.score[0] > hiscore) hiscore = G.score[0];
            if (G.score[1] > hiscore) hiscore = G.score[1];
            sound_frame(G.walking, (uint8_t)(G.fast || G.escaper),
                        (uint8_t)(G.dethwish || G.death || G.squash), G.entry, G.nonoise);
            show_message();
            if (state == ST_PLAY && G.gameOver) {
                state = ST_GAMEOVER;
                gameoverTimer = 0;
            }
            if (state == ST_GAMEOVER && ++gameoverTimer > 240) {
                state = ST_TITLE;
                sound_stop();
                continue;
            }
            video_flush();
        }
    }
    sound_stop();
    xreg_vga_canvas(CANVAS_CONSOLE);
    return 0;
}
