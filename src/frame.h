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
void popup_cut(void);               /* popups over the scene (Bob, BUILD 12) */
void popup_pause(void);
void popup_ask(void);
void popup_restore(void);           /* takes away the pause or quit popup */
void frame_print(void);             /* print, fired, name, wrap: popups too */
void frame_fired(void);
void frame_name(void);
void frame_name_step(void);
void frame_wrap(void);

#endif
