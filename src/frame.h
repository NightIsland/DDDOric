/* frame.h - the film frame: title screen and cards (src/frame.c; docs/PLAN.md
   F4; DESIGN "Presentation").  frame_X draws mode X's screen whole;
   frame_X_step changes what moves on it, once a step. */
#ifndef FRAME_H
#define FRAME_H

#include "scene.h"

void frame_init(void);
void frame_title(void);
void frame_title_step(void);
void frame_card(const struct scene *s);
void frame_card_step(void);
void frame_cut(void);               /* the megaphone and "CUT!" in the title strip */
void frame_print(void);
void frame_fired(void);
void frame_name(void);
void frame_name_step(void);
void frame_wrap(void);

#endif
