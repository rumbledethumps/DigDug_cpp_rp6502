// VGA output for the RP6502: 4bpp bitmap for the dirt, 2bpp sprites for
// everything that moves, and a character plane for text.
#ifndef VIDEO_H
#define VIDEO_H
#include <stdint.h>

// XRAM map
#define XR_BITMAP   0x0000   // 320x240 4bpp (38400 bytes)
#define XR_BPAL     0x9600   // bitmap palette, 16 x 2 bytes
#define XR_SPAL     0x9640   // sprite palettes, 16 x 8 bytes
#define XR_CFG3     0x96C0   // mode 3 config
#define XR_CFG1     0x96E0   // mode 1 config
#define XR_SPRITES  0x9700   // sprite list, 40 x 8 bytes (to 0x983F)
#define XR_TEXT     0x9900   // 40 x 30 characters (to 0x9DAF)
#define XR_KBD      0x9E00   // keyboard bitmap (32)
#define XR_PAD      0x9E40   // gamepads (40)
#define XR_PSG      0x9F00   // PSG (64, page aligned)
// sprite images are loaded from assets/sprites.bin at SPRITE_XRAM (gfx_data.h)

#define NUM_SPRITES 40
#define TEXT_COLS 40
#define TEXT_ROWS 30

void video_init(void);
void video_wait_vsync(void);
void video_dirty(uint8_t idx);          // dirt cell changed
void video_redraw_all(void);            // whole dirt map + score line
void video_flush(void);                 // per frame: dirty cells, score, sprites
void video_palette_changed(void);       // vegetable / dirt colours changed
void video_score_changed(void);
void video_blank(uint8_t on);           // hide the playfield (message screens)
void video_text(uint8_t col, uint8_t row, const char* s);
void video_text_clear(void);
void video_text_num(uint8_t col, uint8_t row, uint32_t v, uint8_t width);

#endif
