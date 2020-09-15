/* Reusable OpenGL drawing routines
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include <stdlib.h>
#include <math.h>
#include "visualizer.h"

// ******   DRAWING PRIMITIVES   ******
// These all draw a simple object centered at (0,0,0), assuming that 
// translation to the appropriate part of space is handled by the caller.
// 2D drawing routines use the XY plane.

// Circles are expensive, so an approximation is cached for fast rendering
#define N_CIRCLE_VERTICES 16
// This structure can be used to represent either a 3D point or 3D vector
struct Vertex {
  flo x,y;
} circle_vertices[N_CIRCLE_VERTICES];

// initialization routine which must be called before drawing begins
// Right now, all it needs to do is create the circle approximation
void init_drawing_primitives () {
  int i, n = N_CIRCLE_VERTICES;
  for (i = 0; i < n; i++) {
    double d = (double)i/(n-1);
    double t = (2*3.1415*(1-d));
    float sx = sin(t);
    float sy = cos(t);
    circle_vertices[i].x = sx;
    circle_vertices[i].y = sy;
  }
}

void draw_cube (float r) {
  float w = r;
  float h = r;
  float d = r;
  glBegin( GL_LINE_STRIP );
  // top
  glVertex3d(-w, -h, -d);
  glVertex3d(-w,  h, -d);
  glVertex3d( w,  h, -d);
  glVertex3d( w, -h, -d);
  glVertex3d(-w, -h, -d);
  // bot
  glVertex3d(-w, -h,  d);
  glVertex3d(-w,  h,  d);
  glVertex3d( w,  h,  d);
  glVertex3d( w, -h,  d);
  glVertex3d(-w, -h,  d);
  // sides
  glVertex3d( w, -h,  d);
  glVertex3d( w, -h, -d);
  glVertex3d( w,  h, -d);
  glVertex3d( w,  h,  d);
  glVertex3d(-w,  h,  d);
  glVertex3d(-w,  h, -d);
  glEnd();
}

// Draws an approximate radius r circle (unfilled)
void draw_circle (float r) {
  int i;
  glPushMatrix(); // save state
  glScalef(r, r, r);
#ifndef FAST_LINES
  glBegin(GL_LINE_LOOP);
#else
  glBegin(GL_POINTS);
#endif
  for (i = 0; i < N_CIRCLE_VERTICES-1; i++)
    glVertex2f(circle_vertices[i].x, circle_vertices[i].y);
  glEnd();
  glPopMatrix();
}

// Draws an approximate radius r circle (filled), with a heavy edge
void draw_disk (float r) {
  int i;
  glPushMatrix(); // save state
  glScalef(r, r, r);
  glBegin(GL_TRIANGLE_FAN);
  glVertex2f(0, 0);
  for (i = 0; i < N_CIRCLE_VERTICES-1; i++) {
    glVertex2f(circle_vertices[i].x, circle_vertices[i].y);
  }
  glVertex2f(circle_vertices[0].x, circle_vertices[0].y);
  glEnd(); 
  glPopMatrix();
  draw_circle(r); // draw the circle over it, to make the edge heavy
}

// Draws a size 2r square
void draw_square (float r) {
  glBegin(GL_LINE_LOOP);
  glVertex2f(-r, r); glVertex2f( r, r); glVertex2f( r,-r); glVertex2f(-r,-r);
  glEnd();
}
void draw_rect (float w, float h) {
  glBegin(GL_LINE_LOOP);
  float nw=w/2, nh=h/2; 
  glVertex2f(-nw,nh);glVertex2f(nw,nh);glVertex2f(nw,-nh);glVertex2f(-nw,-nh);
  glEnd();
}

// Draws a rectangle of size width x height
void draw_quad (float w, float h) {
  glPushMatrix(); // save state
  glScalef(w/2, h/2, 1);
  glBegin(GL_QUADS);
  glVertex2f(-1,-1); glVertex2f(-1, 1); glVertex2f( 1, 1); glVertex2f( 1,-1);
  glEnd(); 
  glPopMatrix();
}

// Draws a 3D rectangle centered at (0,0,0).
void draw_3quad (float w, float h, float d) {
  glPushMatrix();  // save state
  glScalef(w/2, h/2, d/2);
  glBegin(GL_QUADS);
  glVertex3f(-1,  1, -1); glVertex3f(-1,  1,  1); // Top face
  glVertex3f( 1,  1,  1); glVertex3f( 1,  1, -1);

  glVertex3f(-1, -1, -1); glVertex3f(-1, -1,  1); // Bottom face
  glVertex3f( 1, -1,  1); glVertex3f( 1, -1, -1);

  glVertex3f(-1, -1,  1); glVertex3f(-1,  1,  1); // Front face
  glVertex3f( 1,  1,  1); glVertex3f( 1, -1,  1);

  glVertex3f(-1, -1, -1); glVertex3f(-1,  1, -1); // Back face
  glVertex3f( 1,  1, -1); glVertex3f( 1, -1, -1);

  glVertex3f( 1, -1, -1); glVertex3f( 1, -1,  1); // Right face
  glVertex3f( 1,  1,  1); glVertex3f( 1,  1, -1);

  glVertex3f(-1, -1, -1); glVertex3f(-1, -1,  1); // Left face
  glVertex3f(-1,  1,  1); glVertex3f(-1,  1, -1);

  glEnd(); 
  glPopMatrix();
}

flo glutStrokeHeight (void *font) {
  return 119.05;
}

// Measure the size of a piece of text when drawn in the specified font.
flo stroke_text_size (void* font, const char* text, flo *height) {
  int i; float l = 0.0;
  for (i=0; i<strlen(text); i++) { l += glutStrokeWidth(font, text[i]); }
  *height = glutStrokeHeight(font);
  return l;
}

#define MAX_LINES 128
#define MAX_STR   1000

int find_lines ( char *str, char **strs ) {
  if (strlen(str) == 0)
    return 0;
  else {
    int  n = 1, i;
    char *p = str;
    strs[0] = p;
    for (i = 0; *p != 0 && i < MAX_STR; i++) {
      if (*p == '\r') {
	*p++ = 0;
      } else if (*p == '\n') {
	*p        = 0;
	strs[n++] = ++p;
	if (n >= MAX_LINES)
	  return n;
      } else {
	p++;
      }
    }
    return n;
  }
}

flo stroke_multitext_size (void* font, int n, char** strs, flo *height) {
  flo w = 0, h = 0;
  int i;
  for (i = 0; i < n; i++) {
    char *s = strs[i];
    flo sw, sh;
    sw = stroke_text_size(font, s, &sh);
    w  = max(w, sw);
    h += sh*2;
  }
  *height = h;
  return w;
}

void stroke_text (void *font, const char *txt) {
  int i;
  glPushMatrix();
  for (i=0; i<strlen(txt); i++)
    glutStrokeCharacter(font, txt[i]);
  glPopMatrix();
}

// Draw text centered in a box of specified width and height
// Text is scaled to the box based on width only: height adjusts to
// keep the characters proportional
void draw_text_raw (TXT_DIR td, flo w, flo h, const char *txt) {
  int i;
  float tw, ts, th, aw, ah;
  glPushMatrix();
  tw = stroke_text_size(GLUT_STROKE_ROMAN, txt, &th);
  ts = min(w/tw, h/th);
  aw = ts * tw;
  ah = ts * th;
  // glPushMatrix(); glScalef(w/2, h/2, 1); draw_square(1); glPopMatrix();
  switch(td) {
  case TD_LEFT:
  case TD_CENTERED:
  case TD_RIGHT:
    glTranslatef(- aw/2, -0.25 * ah, 0.0); break;
  case TD_BOTTOM:
    glTranslatef(- aw/2, (0.35 * ah) - h/2, 0.0); break;
  case TD_TOP:
    glTranslatef(- aw/2, h/2 - (1 * ah), 0.0); break;
  }
  // post("TXT %s\n", txt);
  glScalef(ts, ts, 1.0);
  stroke_text(GLUT_STROKE_ROMAN, txt);
  glPopMatrix();
}

void draw_centered_text_block_raw (flo w, flo h, const char *txt) {
  int i, j, n;
  float tw, ts, th, aw, ah;
  char str[MAX_STR];
  char *strs[MAX_LINES];
  strcpy(str, txt);
  n = find_lines(str, strs);
  // glPushMatrix(); glScalef(w/2, h/2, 1); draw_square(1); glPopMatrix();
  aw = 0.9 * w;
  ah = 0.9 * h;
  glPushMatrix();
  glTranslatef(0, ah/2 - ah/n/2, 0.0);
  for (j = 0; j < n; j++) {
    draw_text_raw(TD_CENTERED, aw, ah / n, strs[j]);
    glTranslatef(0, -ah / n, 0);
  }
  glPopMatrix();
}

void draw_justified_text_block_raw (flo w, flo h, const char *txt) {
  int i, j, n;
  float bh, bw, ah, aw, bs;
  float tw, th;
  char str[MAX_STR];
  char *strs[MAX_LINES];
  strcpy(str, txt);
  n = find_lines(str, strs);
  bh = bw = 0;
  for (j = 0; j < n; j++) {
    tw  = stroke_text_size(GLUT_STROKE_ROMAN, strs[j], &th);
    bw  = max(bw, tw);
    bh += 1.5 * th;
  }
  bs = 0.9 * min(w/bw, h/bh);
  aw = bs * bw;
  ah = bs * bh;
  // glPushMatrix(); glScalef(w/2, h/2, 1); draw_square(1); glPopMatrix();
  glPushMatrix();
  glTranslatef(- aw/2, ah/2 - 0.75 * ah/n, 0.0);
  glScalef(bs, bs, 1.0);
  for (j = 0; j < n; j++) {
    tw = stroke_text_size(GLUT_STROKE_ROMAN, txt, &th);
    stroke_text(GLUT_STROKE_ROMAN, strs[j]);
    glTranslatef(0, -th * 1.5, 0);
  }
  glPopMatrix();
}

void draw_text_block_raw (TXT_DIR td, flo w, flo h, const char *txt) {
  switch (td) {
  case TD_LEFT:     draw_justified_text_block_raw(w, h, txt); break;
  case TD_RIGHT:    draw_justified_text_block_raw(w, h, txt); break;
  case TD_CENTERED: draw_centered_text_block_raw(w, h, txt); break;
  }
}

void draw_text_block (TXT_DIR td, flo w, flo h, const char *txt) {
  glPushAttrib(GL_LINE_BIT | GL_CURRENT_BIT); // save state
  glLineWidth(2);
  draw_text_block_raw(td, w, h, txt);
  glPopAttrib();
}

flo flo_rnd (flo min, flo max) {
  flo val = (double)rand() / (double)RAND_MAX;
  return (val * (max - min)) + min;
}


void draw_text_block_fuzz (TXT_DIR td, flo s, int n, flo w, flo h, const char *txt) {
  int i, j, k; 
  glPushAttrib(GL_LINE_BIT | GL_CURRENT_BIT); // save state
  glLineWidth(n > 0 ? 3 : 2);
  flo inc = n == 0 ? 0 : s / n;
  // for (k = 0; k <= n/2; k++) {
  for (j = -n; j <= n; j++) {
  for (i = -n; i <= n; i++) {
    glPushMatrix();
    // glTranslatef(0.5 * s*i + flo_rnd(-inc, inc), 0.5 * s*j + flo_rnd(-inc, inc), 0);
    glTranslatef(2 * flo_rnd(-s, s), 2 * flo_rnd(-s, s), 0);
    // glTranslatef(3 * n * flo_rnd(-inc, inc), 3 * n * flo_rnd(-inc, inc), 0);
    draw_text_block_raw(td, w, h, txt);
    glPopMatrix();
  }
  }
  // }
  // draw_text_block_raw(w, h, txt);
  glPopAttrib();
}

// Draw text centered in a box of specified width and height
// Text is scaled to the box based on width only: height adjusts to
// keep the characters proportional
void draw_text_justified (TXT_DIR td, flo w, flo h, const char *txt) {
  glPushAttrib(GL_LINE_BIT | GL_CURRENT_BIT); // save state
  glLineWidth(2);
  draw_text_raw(td, w, h, txt);
  glPopAttrib();
}

void draw_text (flo w, flo h, const char *txt) {
  draw_text_justified(TD_CENTERED, w, h, txt);
}


// Draw text centered in a box of specified width and height
// Text is scaled to the box based on width only: height adjusts to
// keep the characters proportional
void draw_text_halo (flo w, flo h, const char *txt) {
  glPushAttrib(GL_LINE_BIT | GL_CURRENT_BIT); // save state
  glLineWidth(2);
  draw_rect(w,h);
  draw_text_raw(TD_CENTERED, w, h, txt);
  glPopAttrib();
}

// a kludgey scaling factor set by Visualizer, this should be replaced
float xscale=1, yscale=1;

void draw_pixmap
    (float x, float y, float w, float h, int iw, int ih, void* image) {
  float zx, zy;
  glPushMatrix();
  zx = w * xscale / iw;
  zy = h * yscale / ih;
  // glRectf(x, y, x+w, y+h);
  // debug("X %f Y %f W %f H %f\n", x, y, w, h);
  // post("IW %d IH %d\n", iw, ih);
  glRasterPos2f(x - w/2, y - h/2);
  glPixelZoom(zx, zy);
  // glDrawPixels(iw, ih, GL_RGB, GL_UNSIGNED_BYTE, image);
  glDrawPixels(iw, ih, GL_BGR_EXT, GL_UNSIGNED_BYTE, image);
  /*
  glColor3f(x/w, y/h, 0.5);
  a = x - w/2;
  b = y - h/2;
  glLineWidth(2.0);
  glBegin(GL_LINE_LOOP);
  glVertex3f(a, b, 0);
  glVertex3f(a+w, b, 0);
  glVertex3f(a+w, b+h, 0);
  glVertex3f(a, b+h, 0);
  glEnd();
  */
  glPopMatrix();
}


// CLIP forces the number x into the range [min,max]
#define CLIP(x, minimum, maximum) max((minimum), min((maximum), (x)))

// Convert hue/saturation/value to red/green/blue.  Output returned in args.
void hsv_to_rgb (flo h, flo s, flo v, flo *r, flo *g, flo *b) {
  flo rt, gt, bt;
  s = CLIP(s, static_cast<flo>(0), static_cast<flo>(1));
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
  //! Why are these commented out? --jsmb 5/12/06
  // rt = CLIP(rt, 0, 255);
  // gt = CLIP(gt, 0, 255);
  // bt = CLIP(bt, 0, 255);
  *r = rt*255; *g = gt*255; *b = bt*255;
}

// Convert hue/saturation/value to red/green/blue.  Output returned in args.
void rgb_to_hsv (flo r, flo g, flo b, flo *h, flo *s, flo *v) {
  flo min   = std::min(r, std::min(g, b));
  flo max   = std::max(r, std::max(g, b));
  flo delta = max - min;
  *v = max / 255;
  if (max == 0) {
    *s = 0;
    *h = 0; // -1;
    return;
  } else {
    *s = delta / max;
    if (r == max) {
      *h = (g - b) / delta;
    } else if (g == max) {
      *h = 2 + (b - r) / delta;
    } else {
      *h = 4 + (r - g) / delta;
    }
    *h *= 60;
    if (*h < 0)
      *h += 360;
  }
}


