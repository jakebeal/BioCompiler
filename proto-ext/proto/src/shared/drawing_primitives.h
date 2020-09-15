/* Reusable OpenGL drawing routines
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __DRAWING_PRIMITIVES__
#define __DRAWING_PRIMITIVES__

#include "utils.h"
#define GL_BGR_EXT                        0x80E0
#define GL_BGRA_EXT                       0x80E1

// Required initialization function (idempotent)
extern void init_drawing_primitives ();

typedef enum {
  TD_LEFT,
  TD_CENTERED,
  TD_RIGHT, 
  TD_BOTTOM, 
  TD_TOP
} TXT_DIR;

// Drawing functions
extern void draw_circle (float r);
extern void draw_disk (float r);
extern void draw_square (float r);
extern void draw_rect (float w, float h);
extern void draw_quad (float w, float h);
extern void draw_3quad (float w, float h, float d);
extern void draw_text (flo w, flo h, const char *txt);
extern void draw_text_justified (TXT_DIR td, flo w, flo h, const char *txt);
extern void draw_text_raw (TXT_DIR td, flo w, flo h, const char *txt);
extern void draw_text_halo (flo w, flo h, const char *txt);
extern void draw_text_block (TXT_DIR td, flo w, flo h, const char *txt);
extern void draw_text_block_fuzz (TXT_DIR td, flo s, int n, flo w, flo h, const char *txt);
extern void hsv_to_rgb (flo h, flo s, flo v, flo *r, flo *g, flo *b);
extern void rgb_to_hsv (flo r, flo g, flo b, flo *h, flo *s, flo *v);
extern void draw_pixmap (flo x, flo y, flo w, flo h, int iw, int ih, void* image);

#endif // __DRAWING_PRIMITIVES__
