/* Dynamically reconfigurable coloring for GUI
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __PALETTE__
#define __PALETTE__

#include <map>
using namespace std;

/*
  Palette allows color customization & push/pop of colors
  
  Palette files are formatted:
  NAME R G B [A]
  Whitespace is OK, comments are lines starting with #
*/

#define PALETTE_STACK_SIZE 4
class Color { 
 protected:
  string name;
  flo color[4];
  flo color_stack[PALETTE_STACK_SIZE][4]; // saved colors
  int stack_depth;

  Color(string name, flo r, flo g, flo b, flo a);
  friend class Palette;
 public:
  // temporary changes of color
  void push(flo r, flo g, flo b, flo a=1.0);
  void pop();
  void substitute_color(Color* overlay); // push overlay colors
};

class Palette {
 private:
  map<string,Color*> colors;
  set<string> registered; // track registration to warn of duplicates
  
 public:
  Palette();                   // create a default palette
  // Create color with default values r/g/b/a; files override default colors
  Color* register_color(string name, flo r, flo g, flo b, flo a=1.0);
  // replace colors with values from filename
  void overlay_from_file(const char* filename, bool warnfail=true);
  Color* lookup_color(string name, bool warnfail=true); // retrieve a Color
  void use_color(Color* c); // make the current Color c
  void scale_color(Color* c,flo r, flo g, flo b, flo a); // multiply
  void blend_color(Color* c1,Color* c2, flo frac1); // mix c1 and c2
  // some color calls need to be fed values directly
  void set_background(Color* c);

  static Palette* default_palette;
};

#endif // __PALETTE__
