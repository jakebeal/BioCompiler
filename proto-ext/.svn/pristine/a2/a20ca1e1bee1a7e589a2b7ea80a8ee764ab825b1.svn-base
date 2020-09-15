/* Proto compiler utilities
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

// Note: compiler will leak memory like mad, so it needs to be run
//       as a sub-application, then discarded.

#ifndef PROTO_COMPILER_COMPILER_UTILS_H
#define PROTO_COMPILER_COMPILER_UTILS_H

#include <stddef.h>
#include <stdint.h>

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "nicenames.h"
#include "utils.h"

// Compiler output streams.
extern std::ostream *cpout, *cperr, *cplog;

// Forward declaration.
struct CompilationElement;

/****** OUTPUT HANDLING ******/

/// Report a compiler internal error (i.e. not caused by user)
void ierror(const std::string &msg);
void ierror(CompilationElement *where, const std::string &msg);

/// pretty-print indenting
void pp_push(unsigned int n = 1);
void pp_pop();
std::string pp_indent();

std::string b2s(bool b);
std::string f2s(float num, unsigned int precision = 2);
std::string i2s(int num);

/********** ERROR REPORTING & VERBOSITY **********/

/*
 * Using globals is a little bit ugly, but we're not planning to have
 * the compiler be re-entrant any time soon, so it's fairly safe to assume
 * we'll always have precisely one compiler running.
 */

// Name of current phase, for error reporting.
extern std::string compile_phase;

// When true, terminate & exit gracefully.
extern bool compiler_error;

// When true, errors exit w. status 0.
extern bool compiler_test_mode;

void compile_error(const std::string &msg);
void compile_error(CompilationElement *where, const std::string &msg);
void compile_warn(const std::string &msg);
void compile_warn(CompilationElement *where, const std::string &msg);

/// Clean-up & kill application.
void terminate_on_error();

// Standard levels for verbosity:
#define V1 if (verbosity >= 1) *cpout // Major stages
#define V2 if (verbosity >= 2) *cpout << " "  // Actions
#define V3 if (verbosity >= 3) *cpout << "  " // Fine detail
#define V4 if (verbosity >= 4) *cpout << "   "
#define V5 if (verbosity >= 5) *cpout << "    "

/****** REFLECTION ******/

// Note: #t means the string literal for token t

// FIXME: to_str should be const, but because print isn't yet, it
// can't be.

#define reflection_base(t)                                              \
  virtual std::string type_of() const { return #t; }                    \
  virtual bool isA(const std::string &c) const { return (c == #t); }    \
  std::string to_str() { std::ostringstream s; print(&s); return s.str(); }

#define reflection_sub(t, parent)                                       \
  virtual std::string type_of() const { return #t; }                    \
  virtual bool isA(const std::string &c) const                          \
    { return ((c == #t) || (parent::isA(c))); }

#define reflection_sub2(t, parent1, parent2)                            \
  virtual std::string type_of() const { return #t; }                    \
  virtual bool isA(const std::string &c) const                          \
    { return ((c == #t) || (parent1::isA(c)) || (parent2::isA(c))); }

/****** COMPILATION ELEMENTS & ATTRIBUTES ******/

struct Attribute { reflection_base(Attribute);
  // FIXME: The print function should be const, but it is not
  // practical to do that right now.
  virtual void print(std::ostream *out = cpout) = 0;
  virtual Attribute *inherited() { return 0; }

  // FIXME: This should take a const Attribute &, not an Attribute *,
  // once we can safely change to_str and print to be const member
  // functions.
  virtual void merge(Attribute *addition) {
    ierror("Attempt to merge incompatible attributes: " + this->to_str()
        + ", " + addition->to_str());
  };
};

struct Context : Attribute { reflection_sub(Context, Attribute);
  // Map from file to <start, end> line positions.
  std::map<std::string, std::pair<int, int> > places;

  Context(const std::string &file_name, int line) {
    places[file_name] = std::make_pair<int, int>(line, line);
  }

  Attribute *inherited() { return new Context(*this); }

  void merge(Attribute *_addition) {
    const Context &addition = dynamic_cast<const Context &>(*_addition);
    std::map<std::string, std::pair<int, int> >::const_iterator i;
    for (i = addition.places.begin(); i != addition.places.end(); ++i)
      if (places.find(i->first) != places.end()) {
	places[i->first].first
          = std::min(places[i->first].first, i->second.first);
	places[i->first].second
          = std::max(places[i->first].second, i->second.second);
      } else {
	places[i->first] = i->second;
      }
  }

  void print(std::ostream *out = cpout) {
    std::map<std::string, std::pair<int, int> >::const_iterator i;
    for (i = places.begin(); i != places.end(); ++i) {
      if (i != places.begin())
        *out << ", ";
      *out << i->first << ":" << i->second.first;
      if (i->second.first != i->second.second)
        *out << "-" << i->second.second;
    }
  }
};

struct Error : Attribute { reflection_sub(Error, Attribute);
  std::string msg;
  Error(const std::string &msg_) : msg(msg_) {}
  void print(std::ostream *out = cpout) { *out << msg; }
};

/**
 * A boolean attribute that defaults to false, but is true when marked
 */
struct MarkerAttribute : Attribute {
  reflection_sub(MarkerAttribute, Attribute);

  bool inherit;
  MarkerAttribute(bool inherit_) : inherit(inherit_) {}

  void print(std::ostream *out = cpout) { *out << "MARK"; }
  virtual Attribute *inherited() { return (inherit ? this : 0); }
  void merge(Attribute *addition) {
    inherit |= dynamic_cast<const MarkerAttribute &>(*addition).inherit;
  }
};

struct IntAttribute : Attribute { reflection_sub(IntAttribute, Attribute);
  int value;
  bool inherit;
  IntAttribute(bool inherit_ = false) : inherit(inherit_) {}

  void print(std::ostream *out = cpout) { *out << "MARK"; }
  virtual Attribute *inherited() { return (inherit ? this : 0); }
  void merge(Attribute *addition) {
    inherit |= dynamic_cast<const MarkerAttribute &>(*addition).inherit;
  }
};


// By default, attributes that are passed around are *not* duplicated
#define CE CompilationElement
struct CompilationElement : public Nameable { reflection_base(CE);
  static uint32_t max_id;
  uint32_t elmt_id;

  // Should end up with null in default.
  std::map<std::string, Attribute *> attributes;
  typedef std::map<std::string, Attribute *>::iterator att_iter;
  typedef std::map<std::string, Attribute *>::const_iterator const_att_iter;

  CompilationElement() : elmt_id(max_id++) {}
  virtual ~CompilationElement() {}
  virtual void inherit_attributes(CompilationElement *src) {
    if (src == 0)
      ierror("Tried to inherit attributes from null source");
    for (const_att_iter i = src->attributes.begin();
         i != src->attributes.end();
         ++i) {
      Attribute *a = src->attributes[i->first]->inherited();
      if (a != 0) {
	if (attributes.count(i->first))
          attributes[i->first]->merge(a);
        else
          attributes[i->first] = a;
      }
    }
  }

  // Attribute utilities
  void clear_attribute(const std::string &a) {
    //if (attributes.count(a)) {
      //delete attributes[a];
      attributes.erase(a);
    //}
  }

  bool marked(const std::string &a) const { return attributes.count(a); }
  bool mark(const std::string &a) {
    attributes[a] = new MarkerAttribute(true);
    return true;
  }

  // Typing and printing.
  virtual void print(std::ostream *out = cpout) {
    *out << pp_indent() << "Attributes [" << attributes.size() << "]\n";
    pp_push(2);
    for (const_att_iter i = attributes.begin(); i != attributes.end(); ++i) {
      *out<< pp_indent() << i->first <<": ";
      i->second->print(out);
      *out<<"\n";
    }
    pp_pop();
  }
};

struct CEAttr : Attribute { reflection_sub(CEAttr, Attribute);
  CE *value;
  CEAttr(CE *value_) : value(value_) {}
  void print(std::ostream *out = cpout)
    { *out << "CE: " << (value ? value->to_str() : "NULL"); }
};


struct CompilationElement_cmp {
  bool operator()(const CompilationElement *ce1,
                  const CompilationElement *ce2) const {
    return ce1->elmt_id < ce2->elmt_id;
  }
};
#define CEset(x) std::set<x, CompilationElement_cmp>
#define CEmap(x, y) std::map<x, y, CompilationElement_cmp>
#define ce2s(t) ((t) ? (t)->to_str() : "NULL")

/**
 * Used for some indices.
 */
struct CompilationElementIntPair_cmp {
  bool operator()(const std::pair<CompilationElement *, int> &ce1,
                  const std::pair<CompilationElement *, int> &ce2) const {
    return ((ce1.first->elmt_id < ce2.first->elmt_id) ||
            (ce1.first->elmt_id == ce2.first->elmt_id &&
             ce1.second < ce2.second));
  }
};

#endif  // PROTO_COMPILER_COMPILER_UTILS_H
