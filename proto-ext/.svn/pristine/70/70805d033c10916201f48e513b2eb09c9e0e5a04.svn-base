/* A collection of simple distributions
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef _DISTRIBUTIONS_PLUGIN_
#define	_DISTRIBUTIONS_PLUGIN_

#include "proto_plugin.h"
#include "spatialcomputer.h"
#include "UniformRandom.h"

/*************** Various Distributions ***************/
class FixedPoint : public UniformRandom {
public:
  int fixed; int n_fixes;
  Population fixes;
  FixedPoint(Args* args, int n, Rect* volume);
  virtual ~FixedPoint();
  bool next_location(METERS *loc); 
};

class FixedPointFile : public UniformRandom {
public:
  int fixed; int n_fixes;
  Population fixes;
  FixedPointFile(Args* args, int n, Rect* volume);
  virtual ~FixedPointFile();
  bool next_location(METERS *loc); 
};

class Grid : public Distribution {
public:
  int rows,columns,layers;
  int i;
  Grid(int n, Rect* volume) ;
  virtual bool next_location(METERS *loc) ;
};

// Y is random
class XGrid : public Distribution {
public:
  int rows,columns,layers;
  int i;
  XGrid(int n, Rect* volume);
  bool next_location(METERS *loc);
};

class GridRandom : public Grid {
public:
  METERS epsilon;
  GridRandom(Args* args, int n, Rect* volume);
  bool next_location(METERS *loc);
};

class HexGrid : public Distribution {
public:
  int rows,columns,layers;
  int i; flo unit;
  HexGrid(int n, Rect* volume) ;
  virtual bool next_location(METERS *loc) ;
};

class Cylinder : public Distribution {
public:
  METERS r;
  Cylinder(int n, Rect* volume);
  bool next_location(METERS *loc);
};

class Ovoid : public Distribution {
public:
  Ovoid(int n, Rect* volume);
  bool next_location(METERS *loc);
};

class Torus : public Distribution {
public:
  METERS r, r_inner;
  Torus(Args* args, int n, Rect *volume);

  bool next_location(METERS *loc); 
};


/*************** Plugin Interface ***************/
class DistributionsPlugin : public ProtoPluginLibrary {
public:
  void* get_sim_plugin(string type, string name, Args* args,
                       SpatialComputer* cpu, int n);
  static string inventory();
};

#endif // _DISTRIBUTIONS_PLUGIN_

