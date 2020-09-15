/* Protobo's extensions to the kernel
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <stdlib.h>
#include "proto.h"
#include "proto_vm.h"

#define CLIP(x, min, max) MAX(min, MIN(max, x))

static void hsv_to_rgb (flo h, flo s, flo v, flo *r, flo *g, flo *b) {
  flo rt = 0, gt = 0, bt = 0;
  s = CLIP(s, 0, 1);
  if (s == 0.0) {
    rt = gt = bt = v;
  } else {
    int i;
    flo h_temp = (h == 360.0) ? 0.0 : h;
    flo f, p, q, t; 
    h_temp /= 60.0;
    i = (int)h_temp;
    f = h_temp - i;
    p = v*(1-s);
    q = v*(1-(s*f));
    t = v*(1-(s*(1-f)));
    switch (i) {
    case 0: rt = v; gt = t; bt = p; break;
    case 1: rt = q; gt = v; bt = p; break;
    case 2: rt = p; gt = v; bt = t; break;
    case 3: rt = p; gt = q; bt = v; break;
    case 4: rt = t; gt = p; bt = v; break;
    case 5: rt = v; gt = p; bt = q; break;
    }
  }
  *r = rt; *g = gt; *b = bt;
}

MAYBE_INLINE DATA *vec_elt (VEC_VAL *vec, int i) {
  if (i < 0 || i >= vec->n)
    uerror("UNBOUND VEC ELT %d > %d\n", i, vec->n);
  return &vec->elts[i];
}

INLINE NUM_VAL num_vec_elt (VEC_VAL *v, int i) { return NUM_GET(vec_elt(v, i)); }
INLINE NUM_VAL num_vec_elt_set (VEC_VAL *v, int i, NUM_VAL x) { return NUM_SET(vec_elt(v, i), x); }

void hsv_exec (int off) {
  VEC_VAL *rgb = VEC_GET(GLO_GET(off));
  VEC_VAL *hsv = VEC_PEEK(0);
  flo r, g, b;
  hsv_to_rgb
    (num_vec_elt(hsv, 0), num_vec_elt(hsv, 1), num_vec_elt(hsv, 2), &r, &g, &b);
  num_vec_elt_set(rgb, 0, r);
  num_vec_elt_set(rgb, 1, g);
  num_vec_elt_set(rgb, 2, b);
  NPOP(1); VEC_PUSH(rgb);
}

void platform_operation(uint8_t op) {
  MACHINE *m = machine;
  switch(op) {
  case COORD_OP:
    VEC_PUSH(read_coord_sensor()); break;
  case BUTTON_OP: 
    NUM_PUSH(read_button((int)NUM_POP())); break;
  case LEDS_OP: {
    NUM_VAL val = NUM_PEEK(0);
    set_b_led((val > 0.25) != 0 ? 1.0 : 0);
    set_g_led((val > 0.50) != 0 ? 1.0 : 0);
    set_r_led((val > 0.75) != 0 ? 1.0 : 0);
    break; }
  case RED_OP: 
    set_r_led(NUM_PEEK(0)); break;
  case GREEN_OP: 
    set_g_led(NUM_PEEK(0)); break;
  case BLUE_OP: 
    set_b_led(NUM_PEEK(0)); break;
  case RGB_OP: {
    VEC_VAL *vec = VEC_PEEK(0);
    set_r_led(num_vec_elt(vec, 0));
    set_g_led(num_vec_elt(vec, 1));
    set_b_led(num_vec_elt(vec, 2));
    break; }
  case HSV_OP: 
    hsv_exec(NXT_OP(m)); break;
  default:
    uerror("UNKNOWN OPCODE %d %d\n", op, MAX_CMD_OPS);
  }
}
