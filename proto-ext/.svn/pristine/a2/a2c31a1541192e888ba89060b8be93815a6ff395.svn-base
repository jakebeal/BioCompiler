/*  This file is adapted from OpenTissue, and has been altered:

Copyright (c) 2001-2007 Kenny Erleben http://www.opentissue.org

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.

*/

#ifndef OPENTISSUE_TRACKBALL_TRACKBALL_H
#define OPENTISSUE_TRACKBALL_TRACKBALL_H

#if (_MSC_VER >= 1200)
# pragma once
# pragma warning(default: 56 61 62 191 263 264 265 287 289 296 347 529 686)
# pragma warning(disable: 503)
#endif

#include <math.h>
#ifdef _WIN32
#include <windows.h>
/* #include <gl\gl.h> */
#include <GL/gl.h>
#else
#include <unistd.h>
#include <stdlib.h>

#ifdef HAVE_OPENGL_GL_H
#include <OpenGL/gl.h> 
#elif HAVE_GL_GL_H
#include <GL/gl.h>
#endif

#endif

typedef  double    real_type;
typedef  real_type vector3_type[3];
typedef  real_type matrix3x3_type[4][4];
typedef  real_type quaternion_type[4];
typedef  real_type gl_transform_type[16];

class GenericTrackball {
 protected:
  
  real_type         m_radius;                 ///< TBD
  vector3_type      m_anchor_position;        ///< TBD
  vector3_type      m_current_position;       ///< TBD
  real_type         m_angle;                  ///< TBD
  vector3_type      m_axis;                   ///< TBD
  matrix3x3_type    m_xform_anchor;           ///< TBD
  matrix3x3_type    m_xform_incremental;      ///< TBD
  matrix3x3_type    m_xform_current;          ///< TBD
  gl_transform_type m_gl_xform_current;       ///< TBD
  
 public:
  
  real_type          & radius()        { return m_radius; }
  real_type const    & radius()  const { return m_radius; }
  real_type          & angle()         { return m_angle; }
  real_type const    & angle()   const { return m_angle; }
  
  void mat_mult(matrix3x3_type a, matrix3x3_type b, matrix3x3_type out) {
    for(int i=0;i<4;i++)
      for(int j=0;j<4;j++) {
	real_type sum = 0;
	for(int k=0;k<4;k++) sum += a[i][k]*b[k][j];
	out[i][j]=sum;
      }
  }
  
  matrix3x3_type const & get_current_rotation()
  {
    // Compute the the rotation from Panchor to Pcurrent, i.e.
    // the rotation form (Xanchor, Yanchor, Zanchor) to
    // (Xcurrent, Ycurrent, Zcurrent) along a great circle.
    // Multiply the IncrementalTransformation and the AnchorTransformation
    // to get the CurrentTransformation.
    mat_mult(m_xform_incremental,m_xform_anchor,m_xform_current);
    return m_xform_current;
  }
  
  gl_transform_type const & get_gl_current_rotation()
  {
    // Compute the the rotation from Panchor to Pcurrent, i.e.
    // the rotation form (Xanchor, Yanchor, Zanchor) to
    // (Xcurrent, Ycurrent, Zcurrent) along a great circle.
    // Multiply the IncrementalTransformation and the AnchorTransformation
    // to get the CurrentTransformation.
    mat_mult(m_xform_incremental,m_xform_anchor,m_xform_current);
    
    m_gl_xform_current[ 0 ] = m_xform_current[ 0 ][ 0 ];
    m_gl_xform_current[ 1 ] = m_xform_current[ 1 ][ 0 ];
    m_gl_xform_current[ 2 ] = m_xform_current[ 2 ][ 0 ];
    m_gl_xform_current[ 3 ] = 0;
    m_gl_xform_current[ 4 ] = m_xform_current[ 0 ][ 1 ];
    m_gl_xform_current[ 5 ] = m_xform_current[ 1 ][ 1 ];
    m_gl_xform_current[ 6 ] = m_xform_current[ 2 ][ 1 ];
    m_gl_xform_current[ 7 ] = 0;
    m_gl_xform_current[ 8 ] = m_xform_current[ 0 ][ 2 ];
    m_gl_xform_current[ 9 ] = m_xform_current[ 1 ][ 2 ];
    m_gl_xform_current[ 10 ] = m_xform_current[ 2 ][ 2 ];
    m_gl_xform_current[ 11 ] = 0;
    m_gl_xform_current[ 12 ] = 0;
    m_gl_xform_current[ 13 ] = 0;
    m_gl_xform_current[ 14 ] = 0;
    m_gl_xform_current[ 15 ] = 1;
    
    return m_gl_xform_current;
  }
  
  matrix3x3_type const & get_incremental_rotation() const { return m_xform_incremental;   }
  
 public:
  
  GenericTrackball() { m_radius=1.0; }
  
  GenericTrackball(real_type radius) { m_radius=radius; }
  
  virtual ~GenericTrackball(){}
 
  void to_identity( matrix3x3_type m) {
    for(int i=0;i<4;i++)
      for(int j=0;j<4;j++)
	m[i][j] = (i==j)?1:0;
  }
  
  virtual void reset()
  {    
    for(int i=0;i<3;i++) {
      m_anchor_position[i]=0;
      m_current_position[i]=0;
      m_axis[i]=0;
    }
    m_angle     = 0.0;
    to_identity(m_xform_anchor);
    to_identity(m_xform_incremental);
    to_identity(m_xform_current);
  }
  
 public:
  
  virtual void begin_drag (real_type const & x, real_type const & y) = 0;
  virtual void drag       (real_type const & x, real_type const & y) = 0;
  virtual void end_drag   (real_type const & x, real_type const & y) = 0;
  
 private:
  
  virtual void compute_incremental(vector3_type const & anchor, vector3_type const & current, matrix3x3_type & transform) = 0;
  
};


class Trackball : public GenericTrackball {
 public:
  
 Trackball() : GenericTrackball() { reset(); }
 Trackball(real_type const & radius) : GenericTrackball(radius) { reset(); }
  
  void reset()
  {
    GenericTrackball::reset();
    project_onto_surface(this->m_anchor_position);
    project_onto_surface(this->m_current_position);
  }
  
  void begin_drag(real_type const & x, real_type const & y)
  {
    this->m_angle = 0.0;
    for(int i=0;i<3;i++) this->m_axis[i]=0;
    
    for(int i=0;i<4;i++) 
      for(int j=0;j<4;j++)
	this->m_xform_anchor[i][j] = this->m_xform_current[i][j];
    to_identity(this->m_xform_incremental);
    to_identity(this->m_xform_current);
    
    this->m_current_position[0] = this->m_anchor_position[0] = x;
    this->m_current_position[1] = this->m_anchor_position[1] = y;
    this->m_current_position[2] = this->m_anchor_position[2] = 0;
    project_onto_surface(this->m_anchor_position);
    project_onto_surface(this->m_current_position);
  }
  
  void drag(real_type const & x, real_type const & y)
  {
    this->m_current_position[0] = x;
    this->m_current_position[1] = y;
    this->m_current_position[2] = 0;
    project_onto_surface(this->m_current_position);
    compute_incremental(this->m_anchor_position,this->m_current_position,this->m_xform_incremental);
  }
  
  void end_drag(real_type const & x, real_type const & y)
  {
    this->m_current_position[0] = x;
    this->m_current_position[1] = y;
    this->m_current_position[2] = 0;
    project_onto_surface(this->m_current_position);
    compute_incremental(this->m_anchor_position,this->m_current_position,this->m_xform_incremental);
  }
  
 private:
  
  real_type length(const vector3_type & v) {
    return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
  }

  void to_unit(vector3_type & v) {
    real_type len = length(v);
    if(len==0) return;
    for(int i=0;i<3;i++) { v[i] /= len; }
  }

  void project_onto_surface(vector3_type & P)
  {
    const static real_type radius2 = this->m_radius * this->m_radius;
    real_type length2 = P[0]*P[0] + P[1]*P[1];
    if (length2 <= radius2 / 2.0) {
      P[2] = sqrt(radius2 - length2);
    } else {
      P[2] = radius2 / (2.0 * sqrt(length2));
      real_type length = sqrt(length2 + P[2] * P[2]);
      for(int i=0;i<3;i++) P[i] /= length;
    }
    to_unit(P);
  }
  
  real_type dot(vector3_type const & a, vector3_type const & b) {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
  }

  inline void Ru(real_type const & radians, vector3_type const & axis, matrix3x3_type & out) {
    real_type cosinus = cos(radians);
    real_type sinus   = sin(radians);
    vector3_type u = { axis[0],axis[1],axis[2] };
    to_unit(u);

    //Foley p.227 (5.76)
    to_identity(out);
    out[0][0] = u[0]*u[0] + cosinus*(1.0 - u[0]*u[0]);
    out[0][1] = u[0]*u[1]*(1.0-cosinus) - sinus*u[2];
    out[0][2] = u[0]*u[2]*(1.0-cosinus) + sinus*u[1];
    out[1][0] = u[0]*u[1]*(1.0-cosinus) + sinus*u[2];
    out[1][1] = u[1]*u[1] + cosinus*(1.0 - u[1]*u[1]);
    out[1][2] = u[1]*u[2]*(1.0-cosinus) - sinus*u[0];
    out[2][0] = u[0]*u[2]*(1.0-cosinus) - sinus*u[1];
    out[2][1] = u[1]*u[2]*(1.0-cosinus) + sinus*u[0];
    out[2][2] = u[2]*u[2] + cosinus*(1.0 - u[2]*u[2]);
  }

  void cross(vector3_type const a, vector3_type const b, vector3_type out) {
    out[0] = a[1]*b[2]-b[1]*a[2];
    out[1] = b[0]*a[2]-a[0]*b[2];
    out[2] = a[0]*b[1]-b[0]*a[1];
  }

  void compute_incremental(vector3_type const & anchor, vector3_type const & current, matrix3x3_type & transform)
  {
    cross(anchor, current, m_axis);
    real_type len = length(m_axis);
    to_unit(m_axis);
    this->m_angle = atan2(len, dot(anchor,current) );
    Ru(this->m_angle, this->m_axis, transform);
  }
  
};

// Interface w. simulator
extern "C" {
  extern void load_trackball_transformation( void ); // model coords
  extern void load_trackball_zoom_translate( void ); // projection coords
  extern void on_left_button_down (int x, int y);
  extern void on_left_button_up (int x, int y);
  extern void on_right_button_down (int x, int y);
  extern void on_right_button_up (int x, int y);
  extern void on_mouse_move (int x, int y);
  extern void on_joy_move (float x, float y);
  extern void on_key_zoom (float amt);
  extern void reset_view ( void );
  extern float view_width, view_height;
}

#endif // OPENTISSUE_TRACKBALL_BELL_H
