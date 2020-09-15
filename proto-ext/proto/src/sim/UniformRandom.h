/* Uniform random distribution
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef _UNIFORMRANDOM_
#define	_UNIFORMRANDOM_

#include "proto_plugin.h"
#include "spatialcomputer.h"

class UniformRandom : public Distribution {
public:
  UniformRandom(int n, Rect* volume) : Distribution(n,volume) {}
  virtual bool next_location(METERS *loc) {
    loc[0] = urnd(volume->l,volume->r);
    loc[1] = urnd(volume->b,volume->t);
    if(volume->dimensions()==3) {
      Rect3* r = (Rect3*)volume;
      loc[2] = urnd(r->f,r->c);
    } else loc[2]=0;
    return true;
  }
};

#endif	// _UNIFORMRANDOM_

