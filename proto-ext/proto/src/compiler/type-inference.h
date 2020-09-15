/* Proto type inference Copyright (C) 2009, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "analyzer.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "compiler.h"

using namespace std;

class TypePropagator : public IRPropagator {
 public:
  TypePropagator(DFGTransformer* parent, Args* args);
  virtual void print(ostream* out=0) { *out << "TypePropagator"; }
  
  // implicit type conversion or other modification to fix conflict
  // returns true if repair is successful
  bool repair_conflict(Field* f,Consumer c,ProtoType* ftype,ProtoType* ctype);

  /// apply a consumer constraint, managing implicit type conversions 
  bool back_constraint(ProtoType** tmp,Field* f,pair<OperatorInstance*,int> c);

  void act(Field* f);
  void act(OperatorInstance* oi);
  void act(AmorphousMedium* am);
};

class TypeConstraintApplicator {
 public:
  int verbosity;
  TypeConstraintApplicator(IRPropagator* parent) {
    (parent)?verbosity=parent->verbosity:verbosity=0;
    this->parent = parent;
  }
  bool apply_constraint(OperatorInstance* oi, SExpr* constraint);
  bool apply_constraints(OperatorInstance* oi, SExpr* constraints);
 private:
  IRPropagator* parent;
  ProtoType* get_op_return(Operator* op);
  ProtoTuple* get_all_args(OperatorInstance* oi);
  ProtoType* get_nth_arg(OperatorInstance* oi, int n);
  ProtoType* get_ref_last(OperatorInstance* oi, SExpr* ref, SE_List_iter* li);
  ProtoType* get_ref_lcs(OperatorInstance* oi, SExpr* ref, SE_List_iter* li);
  ProtoType* get_ref_nth(OperatorInstance* oi, SExpr* ref, SE_List_iter* li);
  ProtoTuple* tupleOrVector(std::vector<ProtoType*> types);
  ProtoType* get_ref_tupof(OperatorInstance* oi, SExpr* ref, SE_List_iter* li);
  ProtoType* get_ref_fieldof(OperatorInstance* oi, SExpr* ref, SE_List_iter* li);
  ProtoType* get_ref_unlit(OperatorInstance* oi, SExpr* ref, SE_List_iter* li);
  ProtoType* get_ref_ft(OperatorInstance* oi, SExpr* ref, SE_List_iter* li);
  ProtoType* get_ref_inputs(OperatorInstance* oi, SExpr* ref, SE_List_iter* li);
  ProtoType* get_ref_output(OperatorInstance* oi, SExpr* ref, SE_List_iter* li);
  ProtoType* get_ref_list(OperatorInstance* oi, SExpr* ref);
  ProtoType* get_ref(OperatorInstance* oi, SExpr* ref);
  bool assert_range(Field*,ProtoType* range);
  bool assert_nth_arg(OperatorInstance* oi, int n, ProtoType* value);
  bool assert_all_args(OperatorInstance* oi, ProtoType* value);
  bool assert_op_return(Operator* f, ProtoType* value);
  bool assert_on_field(OperatorInstance* oi, SExpr* ref, ProtoType* value);
  bool assert_on_tup(OperatorInstance* oi, SExpr* ref, ProtoType* value);
  bool assert_on_ft(OperatorInstance* oi, SExpr* ref, ProtoType* value);
  bool assert_on_output(OperatorInstance* oi, SExpr* ref, ProtoType* value);
  bool assert_on_inputs(OperatorInstance* oi, SExpr* ref, ProtoType* value);
  bool assert_on_last(OperatorInstance* oi, SExpr* ref, ProtoType* value);
  ProtoType* getLCS(ProtoType* nextType, ProtoType* prevType, bool isRestElem);
  bool assert_on_lcs(OperatorInstance* oi, SExpr* ref, ProtoType* value, SE_List_iter* li);
  bool assert_on_nth(OperatorInstance* oi, SExpr* next, ProtoType* value, SE_List_iter* li);
  bool assert_on_unlit(OperatorInstance* oi, SExpr* next, ProtoType* value);
  bool isRestElement(OperatorInstance* oi, SExpr* ref);
  bool assert_ref_list(OperatorInstance* oi, SExpr* ref, ProtoType* value);
  bool assert_ref(OperatorInstance* oi, SExpr* ref, ProtoType* value);
  bool fillTuple(ProtoTuple* tup, int index);
  ProtoType* coerceType(ProtoType* a, ProtoType* b);
  bool repair_constraint_failure(OI* oi, ProtoType* ftype,ProtoType* ctype);
  bool repair_field_constraint(OI* oi, ProtoField* field, ProtoLocal* local);
};
