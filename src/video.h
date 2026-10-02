// VGA output for the RP6502: 4bpp bitmap for the dirt, 2bpp sprites for
// everything that moves, and a character plane for text.
#ifndef VIDEO_H
#define VIDEO_H
#include <stdint.h>

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
