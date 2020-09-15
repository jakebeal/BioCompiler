/* C implementation of various LISP-ish primitives
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include "lisp.h"
#include <stdio.h>
#include <stdarg.h>

List *lisp_nil;

void Obj::print(std::ostream &stream) {
  stream << "<OBJ>";
}

const char * Obj::typeName() {
  switch(this->lispType()) {
  case LISP_NUMBER: return "NUM";
  case LISP_STRING: return "STR";
  case LISP_SYMBOL: return "SYM";
  case LISP_VECTOR: return "VEC";
  case LISP_LIST:   return "LST";
  }
  return "";
}

Number::Number(double val) : value(val) {}

double Number::getValue() {
  return value;
}

void Number::print(std::ostream &stream) {
  stream << value;
}

String::String(const std::string &value) {
  this->value = value;
}

const std::string &String::getValue() {
  return value;
}

const std::string escapes = "\n\t\"";

std::string str_escape(std::string &str) {
  std::string out;
  int start = 0, i;
  while(1) {
    i = str.find_first_of(escapes, start);
    if(i == std::string::npos) {
      i = str.size();
    }
    out += str.substr(start, i-start);
    if(i == str.size())
      break;
    out += '\\';
    out += str[i];
    start = i;
  }
  return out;
}

void String::print(std::ostream &stream) {
  stream << '"';
  stream << str_escape(value);
  stream << '"';
}

Symbol::Symbol(const std::string &name) {
  this->name = name;
}

const std::string &Symbol::getName() {
  return name;
}

void Symbol::print(std::ostream &stream) {
  stream << name;
}

Vector::Vector() {

}

std::vector<Obj *> *Vector::getValue() {
  return &vec;
}

void Vector::print(std::ostream &stream) {
  int first = 1;
  stream << "[";
  for(std::vector<Obj *>::iterator it = vec.begin();
      it != vec.end(); it++) {
    (*it)->print(stream);
    if(first) {
      first = 0;
    } else {
      stream << " ";
    }
  }
  stream << "]";
}

List::List(Obj *h, Obj*t) {
  head = h;
  tail = t;
}

Obj* List::getHead() {return head;}
Obj* List::getTail() {return tail;}

void List::setHead(Obj *o) {head = o;}
void List::setTail(Obj *o) {tail = o;}

static void print_list_inner(List *list, std::ostream &stream) {
  if(list == lisp_nil) {
    return;
  }
  list->getHead()->print(stream);

  if(listp(list->getTail())) {
    stream << " ";
    print_list_inner(&dynamic_cast<List &>(*list->getTail()), stream);
  } else {
    stream << " . ";
    list->getTail()->print(stream);
  }
}

void List::print(std::ostream &stream) {
  stream << "(";
  print_list_inner(this, stream);
  stream << ")";
}

/* Utility methods */

void check_type (Obj *e, LispType t) {
  if (e->lispType() != t) {
    uerror("TYPE CHECK FAILURE %d SHOULD BE %d\n", e->lispType(), t);
  }
}

List *lst_rev (List *_list) {
  List *l = _list;
  List *r = lisp_nil;
  for (;;) {
    if (l == lisp_nil) {
      return r;
    } else {
      List *tail = &dynamic_cast<List &>(*l->getTail());
      l->setTail(r);
      r = l;
      l = tail;
    }
  }
}

int lst_len(List *e) {
  int i;
  for (i = 0; ; i++) {
    if (e == lisp_nil)
      return i;
    else 
      e = &dynamic_cast<List &>(*e->getTail());
  }
}

Obj *lst_elt(List *e, int offset) {
  int i;
  for (i = 0; i < offset; i++)
    e = &dynamic_cast<List &>(*e->getTail());
  return &dynamic_cast<Obj &>(*e->getHead());
}

Obj *lst_head (List *e) {
  check_type(e, LISP_LIST);
  return dynamic_cast<List &>(*e).getHead();
}

List *lst_tail (List *e) {
  check_type(e, LISP_LIST);
  return &dynamic_cast<List &>(*dynamic_cast<List &>(*e).getTail());
}

const std::string &sym_name(Obj *o) {
  check_type(o, LISP_SYMBOL);
  return dynamic_cast<Symbol &>(*o).getName();
}


List *_list (Obj *head, ...) {
  List *lst = new List(head, lisp_nil);
  List *l = lst;
  Obj *o;
  va_list ap;

  va_start(ap, head);

  while(1) {
    o = va_arg(ap, Obj*);
    if (o == NULL) break;
    l->setTail(new List(o, lisp_nil));
    l = &dynamic_cast<List &>(*l->getTail());
  }
  return lst;
}

void init_lisp() {
  lisp_nil = new List(NULL, NULL);
  lisp_nil->setHead(lisp_nil);
  lisp_nil->setTail(lisp_nil);
}
