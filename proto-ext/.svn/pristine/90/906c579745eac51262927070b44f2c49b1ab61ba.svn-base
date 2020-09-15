/* Intermediate representation for Proto compiler
Copyright (C) 2009-2010, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

// This is where the semantics of Proto are represented directly,
// and is the target for interpretation of textual Proto code.
// Type inference and most optimizations are performed on this
// representation.  After this stage, a program is transformed into
// first abstract bytecodes and then concrete bytecodes

#ifndef PROTO_COMPILER_IR_H
#define PROTO_COMPILER_IR_H

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "sexpr.h"

// forward pointers:
struct AmorphousMedium;
struct DataflowGraph;
struct Field;
struct Operator;
struct OperatorInstance;

#define AM AmorphousMedium
#define OI OperatorInstance
#define DFG DataflowGraph
#define AMset CEset(AM*)
#define Fset CEset(Field*)
#define OIset CEset(OI*)

/*****************************************************************************
 *  TYPES                                                                    *
 *****************************************************************************/
// Note: virtual inheritance requires dynamic_cast<type>(val) to downcast
struct ProtoType : public CompilationElement {
  reflection_sub(ProtoType,CompilationElement);
  virtual void print(std::ostream* out=0) { *out << "<Any>"; }
  // default means of testing for supertype-ness
  virtual bool supertype_of(ProtoType* sub){ return sub->isA(this->type_of()); }
  static ProtoType* clone(ProtoType* t); // copy the type and its attributes
  static bool equal(ProtoType* a, ProtoType* b)
  { return a->supertype_of(b) && b->supertype_of(a); }
  static ProtoType* lcs(ProtoType* a, ProtoType* b); // Least Common Supertype
  static ProtoType* gcs(ProtoType* a, ProtoType* b); // Greatest Common Subtype
  virtual ProtoType* lcs(ProtoType* t) { return new ProtoType(); }
  virtual ProtoType* gcs(ProtoType* t);
  virtual bool isLiteral() { return false; }
  virtual int pointwise() { return -1; } // uncertain whether it's pointwise
};

struct ProtoLocal : virtual public ProtoType {
  reflection_sub(ProtoLocal,ProtoType);
  ProtoLocal() {}
  virtual ~ProtoLocal() {}
  virtual void print(std::ostream* out=0) { *out << "<Local>"; }
  virtual ProtoType* lcs(ProtoType* t);
  virtual ProtoType* gcs(ProtoType* t);
  virtual int pointwise() { return 1; } // definitely pointwise
};

// A general Tuple may contain Fields within it, and thus be neither
// local nor field.
struct ProtoTuple : virtual public ProtoType {
  reflection_sub(ProtoTuple,ProtoType);
  std::vector<ProtoType*> types;
  bool bounded; // when false, last type is "rest" type
  ProtoTuple() { this->bounded=false; this->types.push_back(new ProtoType()); }
  ProtoTuple(bool bounded) { this->bounded=bounded; }
  ProtoTuple(ProtoTuple* src) {
    bounded=src->bounded;
    for(int i=0;i<src->types.size();i++) add(src->types[i]);
    inherit_attributes(src);
  }
  virtual ~ProtoTuple() {}
  static ProtoTuple* specialize_tuple_class(ProtoTuple* t);
  void add(ProtoType* t) { types.push_back(t); }
  void clear() { types.clear(); }
  virtual void print(std::ostream* out=0);
  virtual bool supertype_of(ProtoType* sub);
  virtual ProtoType* lcs(ProtoType* t);
  virtual ProtoType* gcs(ProtoType* t);
  virtual bool isLiteral() {
    if(!bounded) return false;
    for(int i=0;i<types.size();i++) if(!types[i]->isLiteral()) return false;
    return true;
  }
  virtual int pointwise() { return -1; } // uncertain whether it's pointwise
};
#define T_TYPE(x) (&dynamic_cast<ProtoTuple &>(*(x)))

struct ProtoLocalTuple : virtual public ProtoLocal, public ProtoTuple {
  reflection_sub2(ProtoLocalTuple,ProtoLocal,ProtoTuple);
  ProtoLocalTuple() : ProtoTuple(false) { this->bounded=false; this->types.push_back(new ProtoLocal()); }
  ProtoLocalTuple(bool bounded) : ProtoTuple(bounded) { }
  virtual ~ProtoLocalTuple() {}
  virtual void print(std::ostream* out=0);
  virtual ProtoType* lcs(ProtoType* t);
  virtual ProtoType* gcs(ProtoType* t) { return ProtoTuple::gcs(t); }
  virtual int pointwise() { return 1; } // definitely pointwise
};
#define LT_TYPE(x) (&dynamic_cast<ProtoLocalTuple &>(*(x)))

struct ProtoSymbol : public ProtoLocal {
  reflection_sub(ProtoSymbol,ProtoLocal);
  bool constant; std::string value;
  ProtoSymbol() : constant(false), value("") {}
  ProtoSymbol(const std::string &v) :  constant(true), value(v) {}
  virtual void print(std::ostream* out=0)
  { *out << "<Symbol"; if(constant) { *out << " " << value; } *out << ">"; }
  virtual bool supertype_of(ProtoType* sub);
  virtual ProtoType* lcs(ProtoType* t);
  virtual ProtoType* gcs(ProtoType* t);
  virtual bool isLiteral() { return constant; }
};

struct ProtoNumber : virtual public ProtoLocal {
  reflection_sub(ProtoNumber,ProtoLocal);
  virtual void print(std::ostream* out=0) { *out << "<Number>"; }
  virtual ProtoType* lcs(ProtoType* t);
  virtual ProtoType* gcs(ProtoType* t);
};

struct ProtoScalar : public ProtoNumber{
  reflection_sub(ProtoScalar,ProtoNumber);
  bool constant; float value;
  ProtoScalar() { constant=false; value=7734; }
  ProtoScalar(float v) { constant=true; value=v; }
  virtual ~ProtoScalar() {}
  virtual void print(std::ostream* out=0)
  { *out << "<Scalar"; if(constant) { *out << " " << value; } *out << ">"; }
  virtual bool supertype_of(ProtoType* sub);
  virtual ProtoType* lcs(ProtoType* t);
  virtual ProtoType* gcs(ProtoType* t);
  virtual bool isLiteral() { return constant; }
};
#define S_TYPE(x) (&dynamic_cast<ProtoScalar &>(*(x)))
#define S_VAL(x) (dynamic_cast<ProtoScalar &>(*(x)).value)

struct ProtoBoolean : public ProtoScalar {
  reflection_sub(ProtoBoolean,ProtoScalar);
  ProtoBoolean() { constant=false; value=7734; }
  ProtoBoolean(bool b) { constant=true; value=b; }
  virtual void print(std::ostream* out=0);
  virtual ProtoType* lcs(ProtoType* t);
};

// Vectors are a Tuple of scalars
struct ProtoVector : public ProtoLocalTuple, public ProtoNumber {
  reflection_sub2(ProtoVector,ProtoNumber,ProtoLocalTuple);
  ProtoVector() : ProtoLocalTuple(false) {this->types.push_back(new ProtoScalar());}
  ProtoVector(bool bounded) : ProtoLocalTuple(bounded) { }
  virtual ~ProtoVector() {}
  virtual void print(std::ostream* out=0);
  virtual ProtoType* lcs(ProtoType* t);
  virtual ProtoType* gcs(ProtoType* t) { return ProtoLocalTuple::gcs(t); }
  virtual int pointwise() { return 1; } // definitely pointwise
};
#define V_TYPE(x) (&dynamic_cast<ProtoVector &>(*(x)))

struct ProtoLambda: public ProtoLocal {
  reflection_sub(ProtoLambda,ProtoLocal);
  Operator* op;
  ProtoLambda() { op=NULL; }
  ProtoLambda(Operator* op) { this->op = op; }
  virtual void print(std::ostream* out=0);
  virtual bool supertype_of(ProtoType* sub);
  virtual ProtoType* lcs(ProtoType* t);
  virtual ProtoType* gcs(ProtoType* t);
  virtual bool isLiteral() { return op!=NULL; }
};
#define L_TYPE(x) (&dynamic_cast<ProtoLambda &>(*(x)))
#define L_VAL(x) (dynamic_cast<ProtoLambda &>(*(x)).op)

struct ProtoField : public ProtoType {
  reflection_sub(ProtoField,ProtoType);
  ProtoLocal* hoodtype; // must be local
  ProtoField() { hoodtype=new ProtoLocal(); }
  ProtoField(ProtoLocal* hoodtype) { this->hoodtype = hoodtype; }
  virtual void print(std::ostream* out=0) { *out<<"<Field "<<ce2s(hoodtype)<<">"; }
  virtual bool supertype_of(ProtoType* sub);
  virtual ProtoType* lcs(ProtoType* t);
  virtual ProtoType* gcs(ProtoType* t);
  virtual bool isLiteral() { return hoodtype->isLiteral(); }
  virtual int pointwise() { return 0; } // definitely not pointwise
};
#define F_TYPE(x) (&dynamic_cast<ProtoField &>(*(x)))
#define F_VAL(x) (dynamic_cast<ProtoField &>(*(x)).hoodtype)

/*****************************************************************************
 *  OPERATORS & MACROS                                                       *
 *****************************************************************************/
// SIGNATURES specify the I/O compatibility of an operator
struct Signature : public CompilationElement { reflection_sub(Signature,CE);
  ProtoType *output;
  std::vector<ProtoType*> required_inputs;
  std::vector<ProtoType*> optional_inputs;
  ProtoType* rest_input;

  // map from names to I/O locations (output = -1)
  std::map<std::string,int> names;

  Signature(CE* src, ProtoType* output=NULL);
  /// copy constructor
  Signature(Signature* src);
  virtual void print(std::ostream* out=0);

  /// true if n a permissible number of args
  bool legal_length(int n);

  /**
   * Returns the nth type in the signature.
   * For example:
   * [mod].nth_type(0) = Scalar
   *
   * If the nth type is a rest element, the whole element is returned.
   * For example:
   * [+].nth_type(0) = Tuple<<Number>...>
   */
  ProtoType* nth_type(int n);

  /// get type of parameter named p; if it doesn't exist, return error or NULL
  ProtoType* parameter_type(SE_Symbol* p, bool err=true);
  /// get index of parameter named p; if it doesn't exist, return error or -2
  int parameter_id(SE_Symbol* p, bool err=true);

  /**
   * Sets the nth_type (see function) of the signature to val.
   */
  void set_nth_type(int n, ProtoType* val);

  /**
   * Returns the fixed size of a function signature.
   * This is the sum of required_inputs and optional_inputs.
   */
  int n_fixed() { return required_inputs.size()+optional_inputs.size(); }

  /// English description of number of allowed types
  std::string num_arg_str();
};

// OPERATORS are functions mapping from fields to fields
struct Operator : public CompilationElement { reflection_sub(Operator,CE);
  Signature *signature;
  std::string name; // only set by some operators

  Operator(CE* src) { inherit_attributes(src); }
  Operator(CE* src, Signature *sig) { inherit_attributes(src); signature=sig; }
  virtual void print(std::ostream* out=0) {*out<<"[Op"<<signature->to_str()<<"]";}
};

struct Literal : public Operator { reflection_sub(Literal,Operator);
  ProtoType *value;
  Literal(CE* src,ProtoType *v);
  virtual void print(std::ostream* out=0) {*out << "[Lit: "<<value->to_str()<< "]";}
};

struct Primitive : public Operator { reflection_sub(Primitive,Operator);
  Primitive(CE* src); // used by subclasses that initialize separately
  Primitive(CE* src, std::string name, Signature* sig);
  virtual void print(std::ostream* out=0) { *out << "[" << name << "]"; }
};

// Note: usage data tracked by DFG only
struct CompoundOp : public Operator { reflection_sub(CompoundOp,Operator);
  AM *body;
  Field *output;
  static int lambda_count; // uids for unnamed lambdas
  CompoundOp(CE* src, DFG* container, std::string n="");
  CompoundOp(CompoundOp* src);
  bool compute_side_effects(); // marks self & returns
  virtual void print(std::ostream* out=0)  {*out << "[Fun: " << name << "]";}
  virtual void printbody(std::ostream* out=0);
};

 // something input to a function
struct Parameter : public Operator { reflection_sub(Parameter,Operator);
  ProtoType *defaultValue;
  CompoundOp* container;
  int index; // which signature variable is it?
  Parameter(CompoundOp* op, std::string name, int index, ProtoType* type=NULL,
            ProtoType* def=NULL);
  virtual void print(std::ostream* out=0);
};

// A pointwise operator, "pushed down" to operator over field data types
struct FieldOp : public Primitive { reflection_sub(FieldOp,Primitive);
 private:
  static CEmap(Operator*,FieldOp*) fieldops;
  FieldOp(Operator* base);
 public:
  static Operator* get_field_op(OI* oi); // null if can't convert
  Operator* base; // must be a pointwise primitive
};

// A field operator, localized to operate on individual nbr data
struct LocalFieldOp : public Primitive { reflection_sub(LocalFieldOp,Primitive);
 private:
  static CEmap(Operator*,LocalFieldOp*) localops;
  LocalFieldOp(Operator* base);
 public:
  static Operator* get_local_op(Operator* op); // null if can't convert
  Operator* base; // must be a field primitive
};

// MACROS are syntactic operations on S-Expressions
struct Macro : public CompilationElement { reflection_sub(Macro,CE);
  SExpr* pattern;
  std::string name;
  Macro(const std::string &name_, SExpr* pattern_)
    : name(name_), pattern(pattern_) {}
  virtual void print(std::ostream* out=0)  {*out << "[Macro: " << name << "]";}
};

struct MacroSymbol : public Macro { reflection_sub(MacroSymbol,Macro);
 MacroSymbol(const std::string &name, SExpr* pattern) : Macro(name, pattern) {}
};

struct MacroOperator : public Macro { reflection_sub(MacroOperator,Macro);
  static int gensym_count;
  Signature *signature; // All ProtoSymbols w. constant value equal to var name
 MacroOperator(const std::string &name, Signature* sig, SExpr* pattern)
   : Macro(name,pattern) { signature=sig; }
};


/*****************************************************************************
 *  DATAFLOW GRAPHS                                                          *
 *****************************************************************************/

// AMORPHOUS MEDIUMS are the spaces where a program executes
struct AmorphousMedium : public CompilationElement { reflection_sub(AM,CE);
  AmorphousMedium *parent; // nil if a root
  Field *selector; // must be coercable to boolean
  // connections of this AM elsewhere
  Fset fields;
  AMset children;
  DFG* container; // the DFG this is in (set by constructor)
  CompoundOp* bodyOf;

  AmorphousMedium(CE* src, DFG* root, CompoundOp* bodyOf=NULL);
  AmorphousMedium(CE* src, AmorphousMedium* parent, Field* f);

  virtual void print(std::ostream* out=0);
  virtual bool child_of(AM* am); // is this a child of am?
  virtual AM* root(){ if(parent==NULL) return this; else return parent->root();}

  // For using AMs as function bodies
  int size(); // number of fields/ops in this AM and its children
  // These function fill "out" set with all elements from AM and its children:
  void all_spaces(AMset *out);
  void all_fields(Fset *out);
  void all_ois(OIset *out);
};

// FIELDS are sets of data across space
typedef std::pair<OI*,int> Consumer;
struct Field : public CompilationElement { reflection_sub(Field,CE);
  AM *domain; ProtoType *range;
  // connections of this field elsewhere
  DFG* container; OperatorInstance* producer;
  std::set<Consumer, CompilationElementIntPair_cmp > consumers;
  AMset selectors;

  Field(CE* src, AM *domain, ProtoType *range, OperatorInstance *oi);
  virtual void print(std::ostream* out=0);
  bool is_output(); // is this field an program/CompoundOp output?
  // producer/consumer maintenance: do not call directly
  void use(OperatorInstance* oi,int i); // add a consumer
  void unuse(OperatorInstance* oi,int i); // remove a consumer
};

/// OPERATOR INSTANCES are computations performed on particular fields.

struct OperatorInstance : public CompilationElement { reflection_sub(OI,CE);
  DFG* container; Operator* op; // set by constructor
  std::vector<Field*> inputs;
  Field* output; // generated automatically

  /// Constructor
  OperatorInstance(CE* src, Operator *op, AM* space);

  // Mutators
  /// add field as last input, updating its consumers
  Field* add_input(Field* f);
  /// insert input field at pos, updating its consumers
  Field* insert_input(std::vector<Field*>::iterator pos, Field* f);
  /// disconnects the field, updating its consumers
  Field* remove_input(int i);

  // Accessors & utilities
  ///get range of nth input (sets become vecs/tuples)
  ProtoType* nth_input(int i);
  /// get the output space
  AM* domain() { return output->domain; }
  virtual void print(std::ostream* out=0);

  /// op has space/time extent? 1=no, 0=yes, -1=unresolved
  int pointwise();
  /// is this a recursive call? 1=yes, 0=no, -1=unresolved
  int recursive();
};

/// A DATAFLOW GRAPH is a complete program
struct DataflowGraph : public CompilationElement { reflection_sub(DFG,CE);
  OIset nodes; Fset edges; AMset spaces;
  CEmap(Operator*,OIset) funcalls; // List of times each op is used
  Field* output;
  AMset relevant; // root (and use count) of funcalls that are used

  DataflowGraph() { output = NULL; } // base state
  void print(std::ostream* out=0);
  void printdot(std::ostream* out=0,bool field_nodes=false);

  // DFG manipulation
  void relocate_input(OI* src, int src_loc, OI* dst,int dst_loc);
  void relocate_inputs(OI* src, OI* dst, int insert);
  void relocate_consumers(Field* src, Field* dst); // + selectors, output
  void relocate_source(OI* consumer,int in,Field* newsrc);
  void delete_node(OperatorInstance* oi);
  void delete_space(AM* am);
  void remap_medium(AM* src, AM* target); // move src to target, destroying src
  void make_op_inline(OperatorInstance* oi); // CompoundOps only
  CompoundOp* derive_op(OIset *elts,AM* space,std::vector<Field*> *in,Field *out,std::string stem);
  void determine_relevant(); // figure out which AMs are relevant
  Field* add_literal(ProtoType* val,AM* space,CompilationElement* src);
  Field* add_parameter(CompoundOp* op,std::string name,int index,AM* space,CE* src);

 private:
  void dot_print_function(std::ostream* out,AM* root,Field* output,bool field_nodes);
};

/*****************************************************************************
 *  Propagator for working on DFGs                                           *
 *****************************************************************************/

class IRPropagator : public CompilationElement {
 public:
  // behavior variables
  bool act_fields, act_ops, act_am;
  int verbosity;
  int loop_abort; // # equivalent passes through worklist before assuming loop
  // propagation work variables
  Fset worklist_f; OIset worklist_o; AMset worklist_a;
  bool any_changes;
  DFG* root;

  IRPropagator(bool field, bool op, bool am=false, int abort=10)
    { act_fields=field; act_ops=op; act_am=am; loop_abort=abort; }
  bool propagate(DFG* g); // walk through worklist, acting until empty
  virtual void preprop() {} virtual void postprop() {} // hooks
  // action routines to be filled in by inheritors
  virtual void act(Field* f) {}
  virtual void act(OperatorInstance* oi) {}
  virtual void act(AM* am) {}
  // note_change: adds neighbors to the worklist
  void note_change(AM* am); void note_change(Field* f);
  void note_change(OperatorInstance* oi);
  bool maybe_set_range(Field* f,ProtoType* range); // change & note if different
 private:
  void queue_nbrs(AM* am, int marks=0); void queue_nbrs(Field* f, int marks=0);
  void queue_nbrs(OperatorInstance* oi, int marks=0);
};

class CertifyBackpointers : public IRPropagator {
 public:
  bool bad;
  CertifyBackpointers(int verbosity);
  virtual void print(std::ostream* out=0) { *out << "CertifyBackpointers"; }
  void preprop();
  void postprop();
  void act(Field* f);
  void act(OperatorInstance* f);
  void act(AmorphousMedium* f);
};

/*****************************************************************************
 *  SPECIALIZED ERRORS                                                       *
 *****************************************************************************/
// These errors return a dummy expression, allowing the compiler to
// return and exit gracefully
CompilationElement* dummy(std::string type, CompilationElement* context);
Field* field_err(CompilationElement *where,std::string msg);
Operator* op_err(CompilationElement *where,std::string msg);
Macro* macro_err(CompilationElement *where,std::string msg);
ProtoType* type_err(CompilationElement *where,std::string msg);
Signature* sig_err(CompilationElement *where,std::string msg);

#endif  // PROTO_COMPILER_IR_H
