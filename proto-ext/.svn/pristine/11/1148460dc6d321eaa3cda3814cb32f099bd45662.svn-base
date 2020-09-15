/* S-expressions
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef PROTO_COMPILER_SEXPR_H
#define PROTO_COMPILER_SEXPR_H

#include <stack>
#include <vector>

#include "compiler-utils.h"

struct SExpr : CompilationElement { reflection_sub(SExpr, CE);
  static bool NO_LINE_BREAKS;

  bool isSymbol() const { return (type_of() == "SE_Symbol"); }
  bool isList() const { return (type_of() == "SE_List"); }
  bool isScalar() const { return (type_of() == "SE_Scalar"); }
  bool isKeyword() const;

  // FIXME: Why does == behave differently on a string and a char *?
  virtual bool operator==(const std::string &s) const { return false; }
  virtual bool operator==(float v) const { return false; }
  virtual bool operator==(const SExpr *ex) const = 0;
  bool operator==(const char *s) const { return (*this) == std::string(s); }

  virtual SExpr *copy() = 0;
};

// bool operator!=(const SExpr *e, const std::string &s) { return !(e == s); }
// bool operator!=(const SExpr *e, float v) { return !(e == v); }
// bool operator!=(const SExpr *e0, const SExpr *e1) { return !(e0 == e1); }
// bool operator!=(const SExpr *e, const char *s) { return !(e == es); }

struct SE_Scalar: SExpr { reflection_sub(SE_Scalar, SExpr);
  float value;

  SE_Scalar(float value) { this->value = value; }
  SE_Scalar(SE_Scalar *src) : value(src->value) { inherit_attributes(src); }

  SExpr *copy() { return new SE_Scalar(this); }
  void print(std::ostream *out = cpout) { *out << value; }

  virtual bool operator==(float v) const { return (v == value); }
  virtual bool operator==(const SExpr *ex) const {
    return
      ((ex->isScalar())
       && (value == (dynamic_cast<const SE_Scalar &>(*ex).value)));
  }
};

struct SE_Symbol : SExpr { reflection_sub(SE_Symbol, SExpr);
  // Neocompiler is case sensitive, Paleo is not
  static bool case_insensitive;

  std::string name;

  SE_Symbol(const std::string &name_) : name(name_) {
    if (case_insensitive) {
      // Downcase the symbol.
      for (size_t i = 0; i < name.size(); i++)
        name[i] = tolower(name[i]);
    }
  }

  SE_Symbol(SE_Symbol *src) : name(src->name) { inherit_attributes(src); }

  SExpr *copy() { return new SE_Symbol(this); }
  void print(std::ostream *out = cpout) { *out << name; }

  virtual bool operator==(const std::string &s) const { return name == s; }
  virtual bool operator==(const SExpr *ex) const {
    return
      ((ex->isSymbol())
       && (name == (dynamic_cast<const SE_Symbol &>(*ex).name)));
  }
};

struct SE_List : SExpr { reflection_sub(SE_List, SExpr);
  std::vector<SExpr *> children;

  SExpr *copy() {
    SE_List *dst = new SE_List();
    dst->inherit_attributes(this);
    for (size_t i = 0; i < len(); i++)
      dst->add((*this)[i]->copy());
    return dst;
  }

  void add(SExpr *e) { children.push_back(e); inherit_attributes(e); }
  size_t len() const { return children.size(); }
  SExpr *op() const { return children[0]; }
  SExpr *operator[](size_t i) const { return children[i]; }

  std::vector<SExpr *>::iterator args() { return ++children.begin(); }
  std::vector<SExpr *>::const_iterator args() const
    { return ++children.begin(); }

  void print(std::ostream *out = cpout) {
    *out << "("; pp_push(1);
    for (std::vector<SExpr *>::const_iterator i = children.begin();
         i != children.end();
         ++i) {
      if (i != children.begin()) {
        if (NO_LINE_BREAKS)
          *out << " ";
        else
          *out << std::endl << pp_indent();
      }
      (*i)->print(out);
    }
    pp_pop(); *out << ")";
  }

  virtual bool operator==(const SExpr *ex) const {
    if (!ex->isList()) return false;
    const SE_List *l = &dynamic_cast<const SE_List &>(*ex);
    if (l->len() != len()) return false;
    for (size_t i = 0; i < len(); i++)
      if (children[i] != l->children[i])
        return false;
    return true;
  }
};

/****** Attribute for carrying SExprs around ******/

struct SExprAttribute : Attribute { reflection_sub(SExprAttribute, Attribute);
  bool inherit;
  SExpr *exp;

  SExprAttribute(SExpr *exp_, bool inherit_ = true)
    : inherit(inherit_), exp(exp_)
  {}

  void print(std::ostream *out = cpout) {
    *out << "S: ";
    exp->print(out);
  }

  virtual Attribute *inherited() { return (inherit ? this : NULL); }
  // no merge defined
};

/****** Lexical analyzer for reading SExprs ******/

extern SExpr *read_sexpr(const std::string &name, const std::string &in);
extern SExpr *read_sexpr(const std::string &name, std::istream *in = 0,
  std::ostream *out = 0);


/****** Utility to make parsing SEList structures easier ******/

// Error & return dummy.
SExpr *sexp_err(CompilationElement *where, std::string msg);

struct SE_List_iter {
  SE_List *container;
  size_t index;

  SE_List_iter(SE_List *s) : container(s), index(0) {}

  SE_List_iter(SExpr *s, const std::string &name = "expression")
    : index(0), container(0) {
    if (!s->isList()) {
      compile_error(s, "Expected " + name + " to be a list: " + ce2s(s));
      SE_List *list = new SE_List();
      list->inherit_attributes(s);
      container = list;
    } else {
      container = &dynamic_cast<SE_List &>(*s);
    }
  }

  // True if there are more elements.
  bool has_next() const { return (index < container->children.size()); }

  // If next element is SE_Symbol named s, return true and advances.
  bool on_token(const std::string &s) {
    if (!has_next()) return false;
    if (!container->children[index]->isSymbol()) return false;
    const SE_Symbol &symbol
      = dynamic_cast<const SE_Symbol &>(*container->children[index]);
    if (symbol.name != s) return false;
    index++;
    return true;
  }

  // Return next expression (if it exists); error if it does not.
  SExpr *peek_next(const std::string &name = "expression") const {
    if (!has_next())
      return sexp_err(container, "Expected " + name + " is missing");
    return container->children[index];
  }

  // Return next expression (if it exists) and increment; error if it does not.
  SExpr *get_next(const std::string &name = "expression") {
    if (!has_next())
      return sexp_err(container, "Expected " + name + " is missing");
    return container->children[index++];
  }

  // Getters specialized for strings and numbers.
  std::string get_token(const std::string &name = "expression") {
    SExpr *tok = get_next(name);
    if (tok->isSymbol()) {
      return dynamic_cast<const SE_Symbol &>(*tok).name;
    } else {
      compile_error(tok, "Expected " + name + " is not a symbol");
      return "ERROR";
    }
  }

  float get_num(const std::string &name = "expression") {
    SExpr *tok = get_next(name);
    if (tok->isScalar()) {
      return dynamic_cast<const SE_Scalar &>(*tok).value;
    } else {
      compile_error(tok, "Expected " + name + " is not a number");
      return -1;
    }
  }

  // Backs up one expression; attempting to unread 0 gets 0.
  void unread(unsigned int n = 1) {
    if (index < n)
      index = 0;
    else
      index -= n;
  }
};

#endif  // PROTO_COMPILER_SEXPR_H
