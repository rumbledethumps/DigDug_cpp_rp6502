// Hardware probe: draws known patterns so the byte/pixel order of the
// bitmap and sprite formats can be checked from an emulator screenshot.
#include <rp6502.h>
#include <stdio.h>
#include <stdint.h>

#define BMP_ADDR 0x0000          // 320x240 4bpp = 38400 bytes
#define PAL_ADDR 0x9600          // 16 entries x 2 bytes
#define CFG3_ADDR 0x9700         // mode 3 config
#define CFG5_ADDR 0x9720         // mode 5 sprite list
#define SPAL_ADDR 0x9780         // sprite palette (4 entries)
#define SPR_ADDR 0x9800          // sprite images
#define KBD_ADDR 0x9A00          // keyboard bitmap
#define TXT_ADDR 0x9B00          // mode 1 text data
#define CFG1_ADDR 0x9C00         // mode 1 config

#define COLOR(r, g, b) (((b) << 11) | ((g) << 6) | (r) | 0x20)

static void xram_fill(unsigned addr, unsigned n, unsigned char v) {
    RIA.addr0 = addr;
    RIA.step0 = 1;
    while (n--) RIA.rw0 = v;
}

static void put16(unsigned addr, unsigned v) {
    RIA.addr0 = addr;
    RIA.step0 = 1;
    RIA.rw0 = v & 0xFF;
    RIA.rw0 = v >> 8;
}

int main(void) {
    unsigned i;
    // palette: 0 black, 1 red, 2 green, 3 blue, 4 yellow, 5 cyan, 6 magenta, 7 white ...
    static const unsigned pal[16] = {
        COLOR(0, 0, 0), COLOR(31, 0, 0), COLOR(0, 31, 0), COLOR(0, 0, 31),
        COLOR(31, 31, 0), COLOR(0, 31, 31), COLOR(31, 0, 31), COLOR(31, 31, 31),
        COLOR(15, 0, 0), COLOR(0, 15, 0), COLOR(0, 0, 15), COLOR(15, 15, 0),
        COLOR(0, 15, 15), COLOR(15, 0, 15), COLOR(15, 15, 15), COLOR(31, 15, 0)};
    for (i = 0; i < 16; ++i) put16(PAL_ADDR + i * 2, pal[i]);
    // clear bitmap to colour 3 (blue) so black/transparent shows
    xram_fill(BMP_ADDR, 38400, 0x33);
    // top-left byte = 0x12 : tells us which nibble is the left pixel
    RIA.addr0 = BMP_ADDR;
    RIA.step0 = 1;
    RIA.rw0 = 0x12;
    RIA.rw0 = 0x44;   // yellow pair
    RIA.rw0 = 0x77;   // white pair
    // a 2-pixel-wide vertical bar at x=100 (byte 50) for rows 0..239
    for (i = 0; i < 240; ++i) {
        RIA.addr0 = BMP_ADDR + i * 160 + 50;
        RIA.rw0 = 0x77;
    }
    // mode 3 config
    RIA.addr0 = CFG3_ADDR;
    RIA.step0 = 1;
    RIA.rw0 = 0; RIA.rw0 = 0;                       // wrap
    RIA.rw0 = 0; RIA.rw0 = 0;                       // x
    RIA.rw0 = 0; RIA.rw0 = 0;                       // y
    RIA.rw0 = 320 & 0xFF; RIA.rw0 = 320 >> 8;        // width
    RIA.rw0 = 240 & 0xFF; RIA.rw0 = 240 >> 8;        // height
    RIA.rw0 = BMP_ADDR & 0xFF; RIA.rw0 = BMP_ADDR >> 8;
    RIA.rw0 = PAL_ADDR & 0xFF; RIA.rw0 = PAL_ADDR >> 8;

    // sprite palette (2bpp): 0 transparent, 1 red, 2 green, 3 white
    put16(SPAL_ADDR + 0, 0);
    put16(SPAL_ADDR + 2, COLOR(31, 0, 0));
    put16(SPAL_ADDR + 4, COLOR(0, 31, 0));
    put16(SPAL_ADDR + 6, COLOR(31, 31, 31));
    // sprite image: 16x16 2bpp = 4 bytes per row. Row 0: 0x1B,0x00,0x00,0x03
    // (pixels 0..3 = 0,1,2,3 ; last pixel of row = 3). Rows 1..14: 0xFF,0x00,0x00,0xFF
    // Row 15: 0xAA x4 (all colour 2 = green).
    RIA.addr0 = SPR_ADDR;
    RIA.step0 = 1;
    RIA.rw0 = 0x1B; RIA.rw0 = 0x00; RIA.rw0 = 0x00; RIA.rw0 = 0x03;
    for (i = 1; i < 15; ++i) { RIA.rw0 = 0xFF; RIA.rw0 = 0x00; RIA.rw0 = 0x00; RIA.rw0 = 0xFF; }
    RIA.rw0 = 0xAA; RIA.rw0 = 0xAA; RIA.rw0 = 0xAA; RIA.rw0 = 0xAA;
    // two sprites at (32,32) and (60,40) ; a third one partly off the left edge
    RIA.addr0 = CFG5_ADDR;
    RIA.step0 = 1;
    put16(CFG5_ADDR + 0, 32); put16(CFG5_ADDR + 2, 32); put16(CFG5_ADDR + 4, SPR_ADDR); put16(CFG5_ADDR + 6, SPAL_ADDR);
    put16(CFG5_ADDR + 8, 60); put16(CFG5_ADDR + 10, 40); put16(CFG5_ADDR + 12, SPR_ADDR); put16(CFG5_ADDR + 14, SPAL_ADDR);
    put16(CFG5_ADDR + 16, (unsigned)-8); put16(CFG5_ADDR + 18, 100); put16(CFG5_ADDR + 20, SPR_ADDR); put16(CFG5_ADDR + 22, SPAL_ADDR);

    // mode 1 text: 20 columns x 2 rows, built-in font and palette
    RIA.addr0 = CFG1_ADDR;
    RIA.step0 = 1;
    RIA.rw0 = 0; RIA.rw0 = 0;
    RIA.rw0 = 8; RIA.rw0 = 0;                        // x = 8
    RIA.rw0 = 200; RIA.rw0 = 0;                      // y = 200
    RIA.rw0 = 20; RIA.rw0 = 0;                       // width chars
    RIA.rw0 = 2; RIA.rw0 = 0;                        // height chars
    RIA.rw0 = TXT_ADDR & 0xFF; RIA.rw0 = TXT_ADDR >> 8;
    RIA.rw0 = 0xFF; RIA.rw0 = 0xFF;                  // palette
    RIA.rw0 = 0xFF; RIA.rw0 = 0xFF;                  // font
    {
        static const char* msg = "HELLO RP6502 DIG DUG";
        RIA.addr0 = TXT_ADDR;
        for (i = 0; i < 40; ++i) {
            RIA.rw0 = i < 20 ? msg[i] : ('A' + i - 20);
        }
    }

    xreg_vga_canvas(1);
    xreg_vga_mode(3, 2, CFG3_ADDR, 0, 0, 0);           // 4bpp bitmap, plane 0
    xreg_vga_mode(5, 1 | (1 << 3), CFG5_ADDR, 3, 1, 0, 0); // 2bpp 16x16 sprites, plane 1
    xreg_vga_mode(1, 0, CFG1_ADDR, 2, 0, 0);           // 1bpp text, plane 2
    xreg_ria_keyboard(KBD_ADDR);

    // wait ~ forever, printing the keyboard state occasionally
    {
        unsigned char last = RIA.vsync;
        unsigned frames = 0;
        for (;;) {
            while (RIA.vsync == last) {}
            last = RIA.vsync;
            if (++frames == 60) {
                RIA.addr1 = KBD_ADDR;
                RIA.step1 = 1;
                for (i = 0; i < 32; ++i) printf("%02x", RIA.rw1);
                printf("\n");
                frames = 0;
            }
        }
    }
    return 0;
}
