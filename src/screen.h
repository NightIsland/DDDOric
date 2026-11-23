/* screen.h - F0 placeholder display (docs/PLAN.md F0; DESIGN "Screen
   structure").  F1 replaces it with the real display code. */
#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

#define HIRES       ((uint8_t *)0xA000)
#define CHARSET     ((uint8_t *)0x9800)
#define TEXTROW(r)  ((uint8_t *)0xBF68 + (r) * 40)   /* rows 25-27 as 0-2 */
#define MODE_CELL   (*(volatile uint8_t *)0xBFDF)    /* never overwrite after init */

#define TITLE_LINES 8
#define BAND_LINES  48
#define RIDER_BASE  43          /* band line of the rider's lowest line */
#define ROAD_LINE   44          /* first road line of a band */

void screen_init(void);
void __fastcall__ rider_xor(uint8_t track, uint8_t xpx, uint8_t height);
void rider_move(uint8_t ot, uint8_t ox, uint8_t oh, uint8_t nt, uint8_t nx, uint8_t nh);
void status_update(void);

#endif
