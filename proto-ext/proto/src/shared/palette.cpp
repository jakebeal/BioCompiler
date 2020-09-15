/* Dynamically reconfigurable coloring for GUI
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include <stdio.h>
#include "visualizer.h"

/*****************************************************************************
 *  PALETTE                                                                  *
 *****************************************************************************/
Palette* Palette::default_palette = new Palette();

Palette::Palette() { 
  overlay_from_file("local.pal",false); // check for default override
}

void Palette::use_color(Color* c) {
  glColor4f(c->color[0],c->color[1],c->color[2],c->color[3]);
}
void Palette::scale_color(Color* c,flo r, flo g, flo b, flo a) {
  glColor4f(c->color[0]*r,c->color[1]*g,c->color[2]*b,c->color[3]*a);
}
void Palette::blend_color(Color* c1,Color* c2, flo frac1) {
  flo frac2 = 1-frac1;
  glColor4f(c1->color[0]*frac1+c2->color[0]*frac2,
            c1->color[1]*frac1+c2->color[1]*frac2,
            c1->color[2]*frac1+c2->color[2]*frac2,
            c1->color[3]*frac1+c2->color[3]*frac2);
}

void Palette::set_background(Color* c) {
  glClearColor(c->color[0],c->color[1],c->color[2],c->color[3]);
}

Color* Palette::register_color(string name, flo r, flo g, flo b, flo a) {
  if(colors.count(name)) {
    if(registered.count(name))
      debug("WARNING: ignoring duplicate color %s\n",name.c_str());
    return colors[name];
  } else {
    Color* c = new Color(name,r,g,b,a);
    colors[name] = c; registered.insert(name); return c;
  }
}

Color* Palette::lookup_color(string name, bool warnfail) {
  if(!colors.count(name)) {
    if(warnfail)
      debug("WARNING: no color named %s, defaulting to red\n",name.c_str());
    colors[name] = new Color(name,1,0,0,1);
  }
  return colors[name];
}

void Palette::overlay_from_file(const char* filename, bool warnfail) {
  FILE* file;
  if((file = fopen(filename, "r"))==NULL) {
    if(warnfail)
      debug("WARNING: Couldn't open palette file %s.\n",filename); 
  } else {
    char buf[255]; int line=0;
    while(fgets(buf,255,file)) {
      line++;
      char cname[255]; flo r, g, b, a;
      int n = sscanf(buf,"%255s %f %f %f %f",cname,&r,&g,&b,&a);
      if(n==EOF || n==0 || cname[0]=='#') continue; // whitespace or comment
      if(n>=4) {
	if(n==4) a=1;
        string name = cname;
        Color* c = lookup_color(name);
        c->color[0]=r; c->color[1]=g; c->color[2]=b; c->color[3]=a;
      } else {
	debug("WARNING: bad color at %s line %d; should be NAME R G B [A]\n",
	      filename,line);
      }
    }
    fclose(file);
  }
}

/*****************************************************************************
 *  COLOR                                                                    *
 *****************************************************************************/
Color::Color(string name, flo r, flo g, flo b, flo a) {
  color[0]=r; color[1]=g; color[2]=b; color[3]=a;
  stack_depth = 0;
}

void Color::push(flo r, flo g, flo b, flo a) {
  if(stack_depth>=PALETTE_STACK_SIZE) {
    debug("WARNING: Color stack depth exceeeded at %s\n",name.c_str());
  } else {
    for(int i=0;i<4;i++) color_stack[stack_depth][i] = color[i];
    color[0]=r; color[1]=g; color[2]=b; color[3]=a;
    stack_depth++;
  }
}
void Color::pop() {
  if(stack_depth<=0) {
    debug("WARNING: Empty color stack popped for %s\n",name.c_str());
  } else {
    stack_depth--;
    for(int i=0;i<4;i++) color[i] = color_stack[stack_depth][i];
  }
}

void Color::substitute_color(Color* overlay) {
  push(overlay->color[0],overlay->color[1],overlay->color[2],overlay->color[3]);
}
