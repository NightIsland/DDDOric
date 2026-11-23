/* screen.h - the display (docs/PLAN.md F1; DESIGN "Screen structure").

   Lines 0-7 title strip, four 48-line track bands from line 8, status in text
   rows 25-27.  In every HIRES line cell 0 holds the paper colour and cell 1 the
   ink, so play uses cells 2-39 (228 pixels, x 0-227). */
#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>
#include "scene.h"

#define HIRES       ((uint8_t *)0xA000)
#define CHARSET     ((uint8_t *)0x9800)
#define TEXTROW(r)  ((uint8_t *)0xBF68 + (r) * 40)   /* rows 25-27 as 0-2 */
#define MODE_CELL   (*(volatile uint8_t *)0xBFDF)    /* never overwrite after init */

#define TITLE_LINES 8
#define BAND_LINES  48
#define RIDER_TOP   28          /* band line of the rider's top row on the road */
#define ROAD_LINE   44          /* first road line of a band */

void screen_init(void);
void scene_draw(const struct scene *s, uint8_t billing);
void rider_draw(uint8_t track, uint8_t xpx, uint8_t height, uint8_t frame);
void hazards_list(const struct scene *s);  /* after scene_draw */
void hazards_draw(void);
void status_update(void);
void play_reset(void);                     /* before a scene is drawn again */
const uint8_t *glyph(char c);              /* 5 x 7, bits 4-0; 0 = blank */
uint8_t *line_colours(uint8_t y, uint8_t paper, uint8_t ink);
uint8_t * __fastcall__ line_addr(uint8_t y);
#define PAPER(c) (16 + (c))

#endif
