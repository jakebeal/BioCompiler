/* Interface between the trackball classes and Proto simulator
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include <cmath>
#include "Trackball.h"

double      Radius  = 0.8;
double      Xcenter = 0.0;
double      Ycenter = 0.0;
double      Zcenter = 0.0;
float     zoom_factor = 1.0;

GenericTrackball* TB = new Trackball(Radius);

#define NONE   0
#define ZOOM   1
#define ROTATE 2

typedef struct { float x; float y; float z; } Vec3f;

static Vec3f last_point = { 0.0, 0.0, 0.0 };
static int   movement   = NONE;

static const float m_ROTSCALE = 90.0;
static const float m_ZOOMSCALE = 0.008;
//static const float m_MOVESCALE = 0.0001; // No longer used

static Vec3f vec3f(float xx, float yy, float zz) {
  Vec3f v; v.x=xx; v.y=yy; v.z=zz; return v;
}

float view_width, view_height;

void NormalizeCoordinates(double& x, double& y) {
  x = 2.0 * x / view_width  - 1.0;
  if (x < -1.0) x = -1.0;
  if (x >  1.0) x =  1.0;

  y = -(2.0 * y / view_height - 1.0);
  if (y < -1.0) y = -1.0;
  if (y >  1.0) y =  1.0;
}

void load_trackball_transformation( void ) {
  const matrix3x3_type& Tmp = TB->get_current_rotation();
  int glindex = 0;
  GLdouble glmatrix[16];
  
  glLoadIdentity();

  for (unsigned int col = 0; col < 4; ++col) {
    for (unsigned int row = 0; row < 4; ++row) {
      glmatrix[glindex++] = Tmp[row][col];
    }
  }
  glLoadMatrixd(glmatrix);

}

// Loads the non-rotational parts of the trackball positioning
// Added by jsmb 5/13/06
void load_trackball_zoom_translate( void ) {
  glScalef( zoom_factor, zoom_factor, zoom_factor );
  glTranslatef( Xcenter, Ycenter, 0.0 );
}

void on_left_button_down (int x, int y) {
  double Xnorm = x; double Ynorm = y;
  NormalizeCoordinates(Xnorm, Ynorm);
  movement = ROTATE;
  TB->begin_drag(Xnorm, Ynorm);
}

void on_left_button_up (int x, int y) {
  double Xnorm = x; double Ynorm = y;
  NormalizeCoordinates(Xnorm, Ynorm);
  TB->end_drag(Xnorm, Ynorm);
  movement = NONE;
}

void on_right_button_down (int x, int y) {
  movement = ZOOM;
  last_point = vec3f(x, y, 0);
}

void on_right_button_up (int x, int y) {
  movement = NONE;
}

void on_mouse_move (int x, int y) {
  float pixel_diff;
  Vec3f cur_point;

  switch (movement) {
  case ROTATE :  { // Left-mouse button is being held down
    double Xnorm = x;
    double Ynorm = y;
    NormalizeCoordinates(Xnorm, Ynorm);
    TB->drag(Xnorm, Ynorm);
    break;
  }
  case ZOOM :  { // Right-mouse button is being held down
    //
    // Zoom into or away from the scene based upon how far the mouse moved in the x-direction.
    //   This implementation does this by scaling the eye-space.
    //   This should be the first operation performed by the GL_PROJECTION matrix.
    //   1. Calculate the signed distance
    //       a. movement to the left is negative (zoom out).
    //       b. movement to the right is positive (zoom in).
    //   2. Calculate a scale factor for the scene s = 1 + a*dx
    //   3. Call glScalef to have the scale be the first transformation.
    // 
    // post("ZOOM %d %f %f\n", x, last_point.x, zoom_factor);
    pixel_diff  = x - last_point.x; 
    zoom_factor *= 1.0 + pixel_diff * m_ZOOMSCALE;
    //
    // Set the current point, so the lastPoint will be saved properly below.
    //
    //glMatrixMode( GL_PROJECTION );
    //glScalef( zoom_factor, zoom_factor, zoom_factor );
    //glMatrixMode( GL_MODELVIEW );
    cur_point.x = (float) x;  cur_point.y = (float) y;  cur_point.z = (float)0;
    break;
  }
  default: return;
  }
  //
  // Save the location of the current point for the next movement. 
  //
  last_point = cur_point;
	
}

void reset_view ( void ) {
  TB->reset();
  Xcenter=Ycenter=Zcenter=0.0; // rezoom and recenter as well
  zoom_factor=1.0;
}

static Vec3f last_joy_point = { 0.0, 0.0, 0.0 };

void on_joy_move (float x, float y) {
  float pixel_diff_x, move_amount_x;
  float pixel_diff_y, move_amount_y;
  Vec3f cur_joy_point;

  move_amount_x = x / zoom_factor;
  move_amount_y = -y / zoom_factor;
  //glMatrixMode( GL_PROJECTION );
  //glTranslatef( move_amount_x, move_amount_y, 0.0 );
  Xcenter += move_amount_x;  Ycenter += move_amount_y;
  //glMatrixMode( GL_MODELVIEW );
  last_joy_point = cur_joy_point;
}

void on_key_zoom (float amt) {
  zoom_factor *= 1.0 + amt * m_ZOOMSCALE;
}
