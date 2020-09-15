/* OpenGL container for drawing
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __VISUALIZER__
#define __VISUALIZER__

#include "utils.h"

#include "drawing_primitives.h"
#include "palette.h"

#ifdef WANT_GLUT
#ifdef HAVE_APPLE_OPENGL_FRAMEWORK
#include <GLUT/glut.h> 
#else
#include <GL/glut.h> // Linux, Windows
#endif
#endif // WANT_GLUT

extern Palette* palette; // current palette

class Visualizer : public EventConsumer {
  bool is_full_screen; // is the window in fullscreen mode?
  int window; // identifier for window
  flo aspect_ratio;
  Rect bounds; // area expected to be filled by computer
  int old_width,old_height; // saved pixel size of window
  int left,top; // screen location of window
 public:
  int width,height; // pixel size of window

  
 public:
  Visualizer(Args* args);
  ~Visualizer();

  void set_bounds(Rect* r) { 
    bounds.t=r->t; bounds.l=r->l; bounds.b=r->b; bounds.r=r->r; }
  void resize(int width, int height);
  bool handle_key(KeyEvent* key);
  bool handle_mouse(MouseEvent* mouse); // handle clicks and drags
  void visualize(); // visualizer's contribution to frame
  
  void prepare_frame(); // start drawing a frame
  void view_3D(); // shift from 2D overlay to spatial computer drawing mode
  void end_3D(); // shift back to 2D overlay for final drawing
  void complete_frame(); // finish drawing a frame

  // selection routines
  void start_select_3D(Rect* rgn, int max_names);
  void end_select_3D(int max_names, Population* result);
  void click_3d(int winx, int winy, double *pt);
  
  virtual void register_colors();
  static Color* BACKGROUND;
};

#endif // __VISUALIZER__
