// The SOUND.S sequencer driving two channels of the RP6502 PSG.
#ifndef SOUND_H
#define SOUND_H
#include <stdint.h>

void sound_init(void);
void sound_request(uint8_t song);
void sound_frame(uint8_t walking, uint8_t fast, uint8_t dying, uint8_t entry, uint8_t nonoise);
void sound_stop(void);
extern uint8_t sound_muted;

#endif
