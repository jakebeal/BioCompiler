/* Miscellaneous modularity functions, which cannot be included in headers
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "spatialcomputer.h"

Layer::Layer(SpatialComputer* p) {
  parent = p;
  can_dump = p->is_dump_default;
}

Distribution::Distribution(int n, Rect *volume) {
  this->n=n; this->volume=volume;
  width = volume->r-volume->l; height = volume->t-volume->b; depth=0;
  if(volume->dimensions()==3) depth=((Rect3*)volume)->c-((Rect3*)volume)->f;
}

