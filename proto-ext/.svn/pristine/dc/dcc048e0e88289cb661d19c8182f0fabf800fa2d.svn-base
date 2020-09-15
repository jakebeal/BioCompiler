/* OpenGL container for drawing
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include "visualizer.h"
#include "Trackball.h"
#include "drawing_primitives.h"

Palette* palette; // current palette
// NOTE: these interfaces need fixing!
extern float xscale, yscale; // kludge connection to drawing_primitives.cpp
extern float view_width, view_height; // kludge connection to Trackball.cpp

/*****************************************************************************
 *  CREATION AND DESTRUCTION                                                 *
 *****************************************************************************/
// glutInit consumes a standard set of command-line arguments.
// Of importance are: -display DISPLAY and -geometry WxH +X +Y
Visualizer::Visualizer(Args* args) 
  : bounds(0,1,0,1) { // default computer size, changed by spatialcomputer
  glutInitWindowSize(640, 480); // default size
  glutInitWindowPosition (100, 100); // default position
  glutInitDisplayMode (GLUT_DEPTH | GLUT_RGBA | GLUT_DOUBLE);
  glutInit(&args->argc, args->argv); // allows user override of geometry
  const char* default_name = "Proto Simulator";
  const char* name = 
    (args->extract_switch("-window-name")) ? name=args->pop_next()
    : default_name;
  window = glutCreateWindow(name); // title window with cmd line
  // get dimensions from window, since they might've been set by cmd line
  left=glutGet(GLUT_WINDOW_X); top=glutGet(GLUT_WINDOW_Y);
  width=glutGet(GLUT_WINDOW_WIDTH); height=glutGet(GLUT_WINDOW_HEIGHT);
  aspect_ratio = (flo)width/(flo)height;
  
  is_full_screen = args->extract_switch("-f");
  if(is_full_screen) { glutFullScreen(); }
  
  // use the default palette, register our colors, and patch files
  palette = Palette::default_palette;
  ensure_colors_registered("Visualizer");
  
  // set up drawing environment
  glClear(GL_ACCUM_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  palette->set_background(BACKGROUND);
  glLineWidth(1);
  glPointSize(1);
  glEnable(GL_DEPTH_TEST);	// Enables Depth Testing
  glDepthFunc(GL_LEQUAL);	// The Type Of Depth Testing (Less Or Equal)
  glClearDepth(1.0f);		// Depth Buffer Setup
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
  glShadeModel (GL_SMOOTH);	// Select Smooth Shading
  glEnable(GL_TEXTURE_2D);
  glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  // select modulate to mix texture with color for shading
  glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  reset_view();  // reset the trackball
  init_drawing_primitives(); // make sure the drawing primitives are live

  // patch kludge-connections to un-fixed files
  xscale = width  / (bounds.r-bounds.l);
  yscale = height / (bounds.t-bounds.b);
  view_width=width; view_height=height;
}


Color* Visualizer::BACKGROUND;
void Visualizer::register_colors() {
#ifdef WANT_GLUT
  BACKGROUND = palette->register_color("BACKGROUND",0,0,0,0.5); // black
#endif
}

Visualizer::~Visualizer() {
  glutDestroyWindow(window);
}

/*****************************************************************************
 *  EVENT MANAGEMENT                                                         *
 *****************************************************************************/
// when the window is resized, reset the view parameters
void Visualizer::resize(int width, int height) {
  // set the rendering area to equal the new window size
  glViewport(0, 0, width, height);
  aspect_ratio = (flo)width/(flo)height;
  this->width=width; this->height=height;

  // patch kludge-connections to un-fixed files
  xscale = width  / (bounds.r-bounds.l);
  yscale = height / (bounds.t-bounds.b);
  view_width=width; view_height=height;
}

bool Visualizer::handle_key(KeyEvent* key) {
  if(key->normal) {
    if(key->ctrl) {
      switch(key->key) {
      case 24: /* zoom_out(2.0); */ return true; // X = zoom out
      case 26: /* zoom_in(2.0); */ return true; // Z = zoom in
      }
    } else {
      switch(key->key) {
      case 'f':
	is_full_screen = !is_full_screen;
	if (is_full_screen) { // remember position and then fullscreen
	  old_width=glutGet(GLUT_WINDOW_WIDTH); 
     old_height=glutGet(GLUT_WINDOW_HEIGHT); 
	  glutFullScreen();
	} else { // restore pre-zoom window parameters
	  glutReshapeWindow(old_width, old_height);
	}
	return true;
      case 'z': reset_view(); return true;
      }
    }
  } else {
    if(!key->ctrl) {
      switch(key->special) {    
      case GLUT_KEY_RIGHT: on_joy_move(-2, 0); break;
      case GLUT_KEY_LEFT: on_joy_move(2, 0); break;
      case GLUT_KEY_UP: on_joy_move(0, 2); break;
      case GLUT_KEY_DOWN: on_joy_move(0, -2); break;      
      case GLUT_KEY_PAGE_UP: on_key_zoom(5); break;
      case GLUT_KEY_PAGE_DOWN: on_key_zoom(-5); break;
      
      }
    }
  }
  return false;
}

// Left-Drag rotates the display, Right-Drag zooms the display
bool Visualizer::handle_mouse(MouseEvent* mouse) {
  if(!mouse->shift) {
    if(mouse->button==GLUT_LEFT_BUTTON) { // drag = rotate
      switch(mouse->state) {
      case 1: // drag start
	on_left_button_down(mouse->x,mouse->y);
	return true;
      case 2: // drag continue
	on_mouse_move(mouse->x,mouse->y);
	return true;
      case 3: // drag end
	on_left_button_up(mouse->x,mouse->y);
	return true;
      }
    } else if(mouse->button==GLUT_RIGHT_BUTTON) { // drag = zoom
      switch(mouse->state) {
      case 1: // drag start
	on_right_button_down(mouse->x,mouse->y);
	return true;
      case 2: // drag continue
	on_mouse_move(mouse->x,mouse->y);
	return true;
      case 3: // drag end
	on_right_button_up(mouse->x,mouse->y);
	return true;
      }
    } else if (mouse->button == GLUT_WHEEL_UP) {
      on_key_zoom(5); return true;
    } else if (mouse->button == GLUT_WHEEL_DOWN) {
      on_key_zoom(-5); return true;
    }
  }
  return false;
}

/*****************************************************************************
 *  DRAWING - SETUP AND EXECUTION                                            *
 *****************************************************************************/
// Sets the view to fit the standard space populated by agents.
// visualizer's contribution to frame
void Visualizer::visualize() {
  // nothing at all
}

// start drawing a frame
void Visualizer::prepare_frame() {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-width/2,width/2,-height/2,height/2);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);
}

// shift from 2D overlay to spatial computer drawing mode
#define Z_NEAR (1.0f)
#define Z_FAR (5000.0f)   // used to be 50K
void Visualizer::view_3D() {
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  // using the viewport's aspect ratio means there's no X/Y distortion
  gluPerspective(45.0f, aspect_ratio, Z_NEAR, Z_FAR);
  // We want to pull back until the XY plane contains all the 
  // agents (plus a little extra space)
  float y_angle = M_PI/8; // half of 45 degrees, in radians
  float fit_zy = ((bounds.t-bounds.b)/2) / tan(y_angle);
  float x_angle = atan(tan(y_angle)*aspect_ratio);
  float fit_zx = ((bounds.r-bounds.l)/2) / tan(x_angle);
  glTranslatef( 0, 0, -max(fit_zx, fit_zy)*1.05);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  load_trackball_transformation();
  load_trackball_zoom_translate();
}

void Visualizer::end_3D() {
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

// finish drawing a frame
void Visualizer::complete_frame() {
  glutSwapBuffers(); // implicitly makes sure drawing completes
}

void Visualizer::click_3d(int winx, int winy, double *pt) {
  view_3D();
  GLint viewport[4]; glGetIntegerv(GL_VIEWPORT, viewport);
  GLdouble model[16]; glGetDoublev(GL_MODELVIEW_MATRIX,model);
  GLdouble proj[16]; glGetDoublev(GL_PROJECTION_MATRIX,proj);
  // calculate where to put the Z at
  float y_angle = M_PI/8; // half of 45 degrees, in radians
  float fit_zy = ((bounds.t-bounds.b)/2) / tan(y_angle);
  float x_angle = atan(tan(y_angle)*aspect_ratio);
  float fit_zx = ((bounds.r-bounds.l)/2) / tan(x_angle);
  float Z = max(fit_zx, fit_zy)*1.05;
  float z = (1.0/Z_NEAR - 1.0/Z) / (1.0/Z_NEAR - 1.0/Z_FAR); //depth->screen
  winy = viewport[3]-winy; // convert the y coordinate
  gluUnProject(winx,winy,z,model,proj,viewport,&pt[0],&pt[1],&pt[2]);
  end_3D();
}

GLuint* vis_select_buf;
void Visualizer::start_select_3D(Rect* rgn, int max_names) {
  GLint hits;
  // setup selection buffer
  vis_select_buf = (GLuint*)calloc(max_names,4*sizeof(GLuint));
  glSelectBuffer(max_names*4, vis_select_buf);
  glRenderMode(GL_SELECT);
  glInitNames();
  glPushName((GLuint)-1);
  
  // setup picking matrix
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  GLint viewport[4]; glGetIntegerv(GL_VIEWPORT, viewport);
  gluPickMatrix((rgn->l+rgn->r)/2, viewport[3]-(rgn->t+rgn->b)/2, 
                rgn->r-rgn->l, rgn->t-rgn->b, viewport);
  // using the viewport's aspect ratio means there's no X/Y distortion
  gluPerspective(45.0f, aspect_ratio, 1.0f, 50000.0f); 
  // We want to pull back until the XY plane contains all the 
  // agents (plus a little extra space)
  float y_angle = M_PI/8; // half of 45 degrees, in radians
  float fit_zy = ((bounds.t-bounds.b)/2) / tan(y_angle);
  float x_angle = atan(tan(y_angle)*aspect_ratio);
  float fit_zx = ((bounds.r-bounds.l)/2) / tan(x_angle);
  glTranslatef( 0, 0, -max(fit_zx, fit_zy)*1.05);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  load_trackball_transformation();
  load_trackball_zoom_translate();
}

void Visualizer::end_select_3D(int max_names, Population* result) {
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glFlush(); // make sure drawing completes
  // process hits to find selection
  result->clear();
  int hits = glRenderMode(GL_RENDER);
  GLuint* ptr = (GLuint *)vis_select_buf;
  for(int i=0;i<hits;i++) {
    int names=*ptr++; ptr++; ptr++;
    for(int j=0;j<names;j++) {
      result->add((void*)*ptr++);
    }
  }
  free(vis_select_buf); // clean up selection buffer
}
