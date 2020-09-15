/* C implementation of various LISP-ish primitives
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __LIST__
#define __LIST__

#include "utils.h"

#include <string>
#include <vector>
#include <iostream>

enum LispType {
  LISP_NUMBER,
  LISP_STRING,
  LISP_SYMBOL,
  LISP_VECTOR,
  LISP_LIST
};

class Obj {
 public:
  Obj() {}
  virtual LispType lispType() = 0;
  virtual void print(std::ostream &stream = std::cerr);
  const char *typeName();
};

class Number : public Obj {
 private:
  double value;
 public:
  Number(double val);
  virtual LispType lispType() {return LISP_NUMBER;};
  virtual void print(std::ostream &stream = std::cerr);
  double getValue();
};

class String : public Obj {
 private:
  std::string value;
 public:
  String(const std::string &value);
  virtual LispType lispType() {return LISP_STRING;};
  virtual void print(std::ostream &stream = std::cerr);
  const std::string &getValue();
};

class Symbol : public Obj {
 private:
  std::string name;
 public:
  Symbol(const std::string &);
  virtual LispType lispType() {return LISP_SYMBOL;};
  virtual void print(std::ostream &stream = std::cerr);
  const std::string &getName();
  static Symbol *gensym(const std::string &name) {return new Symbol(name);}
};

class Vector : public Obj {
 private:
  std::vector<Obj *> vec;
 public:
  Vector();
  virtual LispType lispType() {return LISP_VECTOR;};
  virtual void print(std::ostream &stream = std::cerr);
  std::vector<Obj *> *getValue();
};

typedef Vector Tuple;

class List : public Obj {
 private:
  Obj *head;
  Obj *tail;
 public:
  List(Obj *, Obj *);
  virtual LispType lispType() {return LISP_LIST;};
  virtual void print(std::ostream &stream = std::cerr);
  Obj *getHead();
  Obj *getTail();
  void setHead(Obj *);
  void setTail(Obj *);
};

#define PAIR(a,b) (new List(a,b))

extern List *lisp_nil;

List *lst_rev(List *);
int lst_len(List *);
Obj *lst_elt(List *, int);
Obj *lst_head(List *);
List *lst_tail(List *);

const std::string &sym_name(Obj *obj);

List *_list(Obj *obj, ...);

inline bool numberp(Obj* l) {return l->lispType() == LISP_NUMBER;}
inline bool stringp(Obj* l) {return l->lispType() == LISP_STRING;}
inline bool symbolp(Obj* l) {return l->lispType() == LISP_SYMBOL;}
inline bool listp(Obj* l) {return l->lispType() == LISP_LIST;}

void init_lisp();

#endif
