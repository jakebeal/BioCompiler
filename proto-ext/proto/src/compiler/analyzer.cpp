/* Proto optimizer Copyright (C) 2009, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

// The analyzer takes us from an initial interpretation to a concrete,
// optimized structure that's ready for compilation

#include "analyzer.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "config.h"

#include "compiler.h"
#include "nicenames.h"

using namespace std;

extern SE_Symbol *make_gensym(const string &root); // From interpreter

#include "type-inference.h"

/*****************************************************************************
 *  TYPE CONCRETENESS                                                        *
 *****************************************************************************/

class Concreteness {
 public:
  // concreteness of fields comes from their types
  static bool acceptable(Field* f) { return acceptable(f->range); }
  // concreteness of types
  static bool acceptable(ProtoType* t) { 
    if(t->isA("ProtoNumber")) { return true;
    } else if(t->isA("ProtoSymbol")) { return true;
    } else if(t->isA("ProtoTuple")) { 
      ProtoTuple* tp = T_TYPE(t);
      for(int i=0;i<tp->types.size();i++) {
        if(!acceptable(tp->types[i])) {
          return false;
        }
      }
      return true;
    } else if(t->isA("ProtoLambda")) {
      return L_VAL(t)==NULL || acceptable(L_VAL(t));
    } else if(t->isA("ProtoField")) {
      return F_VAL(t)==NULL || acceptable(F_VAL(t));
    } else return false; // all others
  }
  // concreteness of operators, for ProtoLambda:
  static bool acceptable(Operator* op) { 
    if(op->isA("Literal") || op->isA("Parameter") || op->isA("Primitive")) {
      return true;
    } else if(op->isA("CompoundOp")) {
      return true; // its fields are checked elsewhere
    } else return false; // generic operator
  }
};

void CheckTypeConcreteness::act(Field* f) {
  if(!Concreteness::acceptable(f->range))
    //ierror(f,"Type is ambiguous: "+f->to_str());
    compile_error(f,"Type is ambiguous: "+f->to_str());
}

/*****************************************************************************
 *  CONSTANT FOLDING                                                         *
 *****************************************************************************/

// NOTE: constantfolder is *not* a type checker!

// scalars & shorter vectors are treated as having 0s all the way out
int compare_numbers(ProtoType* a, ProtoType* b) { // 1-> a>b; -1-> a<b; 0-> a=b
  if(a->isA("ProtoScalar") && b->isA("ProtoScalar")) {
    return (S_VAL(a)==S_VAL(b)) ? 0 : ((S_VAL(a)>S_VAL(b)) ? 1 : -1);
  } else if(a->isA("ProtoScalar") || b->isA("ProtoScalar")) {
    int sx = a->isA("ProtoScalar") ? -1 : 1;
    double s = S_VAL(a->isA("ProtoScalar")?a:b);
    ProtoTuple* v = T_TYPE(a->isA("ProtoScalar")?b:a);
    if(s==S_VAL(v->types[0])) {
      for(int i=1;i<v->types.size();i++) {
        if(S_VAL(v->types[i])!=0) return sx*((S_VAL(v->types[1])>0)?1:-1);
      }
      return 0;
    } else return (sx * ((S_VAL(v->types[0]) > s) ? 1 : -1));
  } else { // 2 vectors
    ProtoTuple *va = T_TYPE(a), *vb = T_TYPE(b);
    int la = va->types.size(), lb = vb->types.size(), len = max(la, lb);
    for(int i=0;i<len;i++) {
      double sa=(i<la)?S_VAL(va->types[i]):0, sb=(i<lb)?S_VAL(vb->types[i]):0;
      if(sa!=sb) { return (sa>sb) ?  1 : -1; }
    }
    return 0;
  }
}

ProtoNumber* add_consts(ProtoType* a, ProtoType* b) {
  if(a->isA("ProtoScalar") && b->isA("ProtoScalar")) {
    return new ProtoScalar(S_VAL(a)+S_VAL(b));
  } else if(a->isA("ProtoScalar") || b->isA("ProtoScalar")) {
    double s = S_VAL(a->isA("ProtoScalar")?a:b);
    ProtoTuple* v = T_TYPE(a->isA("ProtoScalar")?b:a);
    ProtoVector* out = new ProtoVector(v->bounded);
    for(int i=0;i<v->types.size();i++) { 
       ProtoScalar* element = &dynamic_cast<ProtoScalar &>(*v->types[i]);
       out->add(new ProtoScalar(element->value));
    }
    dynamic_cast<ProtoScalar &>(*out->types[0]).value += s;
    return out;
  } else { // 2 vectors
    ProtoTuple *va = T_TYPE(a), *vb = T_TYPE(b);
    ProtoVector* out = new ProtoVector(va->bounded && vb->bounded);
    int la = va->types.size(), lb = vb->types.size(), len = max(la, lb);
    for(int i=0;i<len;i++) {
      double sa=(i<la)?S_VAL(va->types[i]):0, sb=(i<lb)?S_VAL(vb->types[i]):0;
      out->add(new ProtoScalar(sa+sb));
    }
    return out;
  }
}

class ConstantFolder : public IRPropagator {
 public:
  ConstantFolder(DFGTransformer* parent, Args* args) : IRPropagator(false,true) {
    verbosity = args->extract_switch("--constant-folder-verbosity") ? 
      args->pop_int() : parent->verbosity;
  }
  virtual void print(ostream* out=0) { *out << "ConstantFolder"; }

  // FieldOp compatible accessors
  ProtoType* nth_type(OperatorInstance* oi, int i) {
    ProtoType* src = oi->inputs[i]->range;
    if(oi->op->isA("FieldOp")) src = F_VAL(src);
    return src;
  }
  double nth_scalar(OI* oi, int i) {return S_VAL(nth_type(oi,i));}
  ProtoTuple* nth_tuple(OI* oi, int i) { return T_TYPE(nth_type(oi,i)); }
  void maybe_set_output(OperatorInstance* oi,ProtoType* content) {
    if(oi->op->isA("FieldOp")) {
      if(!content->isA("ProtoLocal"))
        ierror("ConstantFolder should only set output of FieldOps to locals");
      ProtoLocal* lcontent = &dynamic_cast<ProtoLocal &>(*content);
      content = new ProtoField(lcontent);
    }
    maybe_set_range(oi->output,content);
  }

  void act(OperatorInstance* oi) {
    if(!oi->op->isA("Primitive")) return; // only operates on primitives
    //if(oi->output->range->isLiteral()) return; // might change...
    const string &name
      = ((oi->op->isA("FieldOp"))
         ? dynamic_cast<FieldOp &>(*oi->op).base->name
         : dynamic_cast<Primitive &>(*oi->op).name);
    // handled by type inference: elt, min, max, tup, all argk pass-throughs
    if(name=="mux") { 
      if(oi->inputs[0]->range->isLiteral()) { // case 1: arg0 is literal
        maybe_set_output(oi,oi->inputs[nth_scalar(oi,0) ? 1 : 2]->range);
      } else if(oi->inputs[1]->range->isLiteral() && // case 2: branches equal
                ProtoType::equal(oi->inputs[1]->range,oi->inputs[2]->range)) {
        maybe_set_output(oi,oi->inputs[1]->range);
      }
      return;
    } else if(name=="len") { // len needs only tuple to be bounded
      ProtoTuple* tt = nth_tuple(oi,0);
      if(tt->bounded) 
        maybe_set_output(oi,new ProtoScalar(tt->types.size()));
      return;
    }

    // rest only apply if all inputs are literals
    for(int i=0;i<oi->inputs.size();i++) {
      if(!oi->inputs[i]->range->isLiteral()) return;
    }
    if(name=="not") {
      maybe_set_output(oi,new ProtoBoolean(!nth_scalar(oi,0)));
    } else if(name=="+") {
      ProtoNumber* sum = new ProtoScalar(0);
      for(int i=0;i<oi->inputs.size();i++) sum=add_consts(sum,nth_type(oi,i));
      maybe_set_output(oi,sum);
    } else if(name=="-") {
      ProtoNumber* sum = new ProtoScalar(0); // sum all but first
      for(int i=1;i<oi->inputs.size();i++) 
        sum=add_consts(sum,nth_type(oi,i));
      // multiply by negative 1
      if(sum->isA("ProtoScalar")) { S_VAL(sum) *= -1;
      } else { // vector
        ProtoVector* s = &dynamic_cast<ProtoVector &>(*sum);
        for(int i=0;i<s->types.size();i++) S_VAL(s->types[i]) *= -1;
      }
      // add in first
      sum = add_consts(sum,nth_type(oi,0));
      maybe_set_output(oi,sum);
    } else if(name=="*") {
      double mults = 1;
      ProtoTuple* vnum = NULL;
      for(int i=0;i<oi->inputs.size();i++) {
        if(nth_type(oi,i)->isA("ProtoScalar")) { mults *= nth_scalar(oi,i);
        } else if(vnum) { compile_error(oi,">1 vector in multiplication");
        } else { vnum = nth_tuple(oi,i); 
        }
      }
      // final optional stage of vector multiplication
      if(vnum) {
        ProtoVector *pv = new ProtoVector(vnum->bounded);
        for(int i=0;i<vnum->types.size();i++)
          pv->add(new ProtoScalar(S_VAL(vnum->types[i])*mults));
        maybe_set_output(oi,pv);
      } else {
        maybe_set_output(oi,new ProtoScalar(mults));
      }
    } else if(name=="/") {
      double denom = 1;
      for(int i=1;i<oi->inputs.size();i++) denom *= nth_scalar(oi,i);
      if(oi->inputs[0]->range->isA("ProtoScalar")) {
        double num = nth_scalar(oi,0);
        maybe_set_output(oi,new ProtoScalar(num/denom));
      } else if(oi->inputs[0]->range->isA("ProtoVector")) { // vector
        ProtoTuple *num = nth_tuple(oi,0);
        ProtoVector *pv = new ProtoVector(num->bounded);
        for(int i=0;i<num->types.size();i++)
          pv->add(new ProtoScalar(S_VAL(num->types[i])/denom));
        maybe_set_output(oi,pv);
      }
    } else if(name==">") {
      int cp = compare_numbers(nth_type(oi,0),nth_type(oi,1));
      maybe_set_output(oi,new ProtoBoolean(cp==1));
    } else if(name=="<") {
      int cp = compare_numbers(nth_type(oi,0),nth_type(oi,1));
      maybe_set_output(oi,new ProtoBoolean(cp==-1));
    } else if(name=="=") {
      int cp = compare_numbers(nth_type(oi,0),nth_type(oi,1));
      maybe_set_output(oi,new ProtoBoolean(cp==0));
    } else if(name=="<=") {
      int cp = compare_numbers(nth_type(oi,0),nth_type(oi,1));
      maybe_set_output(oi,new ProtoBoolean(cp!=1));
    } else if(name==">=") {
      int cp = compare_numbers(nth_type(oi,0),nth_type(oi,1));
      maybe_set_output(oi,new ProtoBoolean(cp!=-1));
    } else if(name=="abs") {
      maybe_set_output(oi,new ProtoScalar(fabs(nth_scalar(oi,0))));
    } else if(name=="floor") {
      maybe_set_output(oi,new ProtoScalar(floor(nth_scalar(oi,0))));
    } else if(name=="ceil") {
      maybe_set_output(oi,new ProtoScalar(ceil(nth_scalar(oi,0))));
    } else if(name=="round") {
      maybe_set_output(oi,new ProtoScalar(rint(nth_scalar(oi,0))));
    } else if(name=="mod") {
      //TODO: this is also implemented in kernel/proto.c,
      //      merge these implementations!
      double dividend = nth_scalar(oi,0), divisor = nth_scalar(oi,1);
      double val = fmod(fabs(dividend), fabs(divisor)); 
      if(divisor < 0 && dividend < 0) val *= -1;
      else if(dividend < 0) val = divisor - val;
      else if(divisor < 0) val += divisor;
      maybe_set_output(oi,new ProtoScalar(val));
    } else if(name=="rem") {
      double a = nth_scalar(oi,0), b = nth_scalar(oi,1);
      maybe_set_output(oi,new ProtoScalar(fmod(a,b)));
    } else if(name=="pow") {
      double a = nth_scalar(oi,0), b = nth_scalar(oi,1);
      maybe_set_output(oi,new ProtoScalar(pow(a,b)));
    } else if(name=="sqrt") { 
      maybe_set_output(oi,new ProtoScalar(sqrt(nth_scalar(oi,0))));
    } else if(name=="log") {
      maybe_set_output(oi,new ProtoScalar(log(nth_scalar(oi,0))));
    } else if(name=="sin") {
      maybe_set_output(oi,new ProtoScalar(sin(nth_scalar(oi,0))));
    } else if(name=="cos") {
      maybe_set_output(oi,new ProtoScalar(cos(nth_scalar(oi,0))));
    } else if(name=="tan") {
      maybe_set_output(oi,new ProtoScalar(tan(nth_scalar(oi,0))));
    } else if(name=="asin") {
      maybe_set_output(oi,new ProtoScalar(asin(nth_scalar(oi,0))));
    } else if(name=="acos") {
      maybe_set_output(oi,new ProtoScalar(acos(nth_scalar(oi,0))));
    } else if(name=="atan2") {
      double a = nth_scalar(oi,0), b = nth_scalar(oi,1);
      maybe_set_output(oi,new ProtoScalar(atan2(a,b)));
    } else if(name=="sinh") {
      maybe_set_output(oi,new ProtoScalar(sinh(nth_scalar(oi,0))));
    } else if(name=="cosh") {
      maybe_set_output(oi,new ProtoScalar(cosh(nth_scalar(oi,0))));
    } else if(name=="tanh") {
      maybe_set_output(oi,new ProtoScalar(tanh(nth_scalar(oi,0))));
    } else if(name=="vdot") {
      // check two, equal length vectors
      ProtoVector* v1 = NULL;
      ProtoVector* v2 = NULL;
      if(2==oi->inputs.size()
         && nth_type(oi,0)->isA("ProtoVector")
         && nth_type(oi,1)->isA("ProtoVector")) {
        v1 = &dynamic_cast<ProtoVector &>(*nth_type(oi,0));
        v2 = &dynamic_cast<ProtoVector &>(*nth_type(oi,1));
        if(v1->types.size() != v2->types.size()) {
          compile_error("Dot product requires 2, *equal size* vectors");
        }
      } else {
        compile_error("Dot product requires exactly *2* vectors");
      }
      // sum of products
      ProtoScalar* sum = new ProtoScalar(0);
      for(int i=0; i<v1->types.size(); i++) {
        sum->value += dynamic_cast<ProtoScalar &>(*v1->types[i]).value *
          dynamic_cast<ProtoScalar &>(*v2->types[i]).value;
      }
      maybe_set_output(oi,sum);
    } else if(name=="min-hood" || name=="max-hood" || name=="any-hood"
              || name=="all-hood") {
      maybe_set_output(oi,F_VAL(oi->inputs[0]->range));
    }
  }
  
};

/*****************************************************************************
 *  LITERALIZER                                                              *
 *****************************************************************************/

class Literalizer : public IRPropagator {
 public:
  Literalizer(DFGTransformer* parent, Args* args) : IRPropagator(true,true) {
    verbosity = args->extract_switch("--literalizer-verbosity") ? 
      args->pop_int() : parent->verbosity;
  }
  virtual void print(ostream* out=0) { *out << "Literalizer"; }
  void act(Field* f) {
    if(f->producer->op->isA("Literal")) return; // literals are already set
    if(f->producer->op->attributes.count(":side-effect")) return; // keep sides
    if(f->range->isLiteral()) {
      OI *oldoi = f->producer;
      OI *newoi = root->add_literal(f->range,f->domain,oldoi)->producer;
      V2<<"Literalizing:\n  "<<ce2s(oldoi)<<" \n  into "<<ce2s(newoi)<<endl;
      root->relocate_consumers(f,newoi->output); note_change(f);
      root->delete_node(oldoi); // kill old op
    }
  }

  void act(OperatorInstance* oi) {
    if(!oi->op->isA("Primitive")) return; // only operates on primitives
    string name = dynamic_cast<Primitive &>(*oi->op).name;
    // change "apply" of literal lambda into just an OI of that operator
    if(name=="apply") {
      if(oi->inputs[0]->range->isA("ProtoLambda") && 
         oi->inputs[0]->range->isLiteral()) {
        OI* newoi=new OI(oi,L_VAL(oi->inputs[0]->range),oi->output->domain);
        for(int i=1;i<oi->inputs.size();i++) 
          newoi->add_input(oi->inputs[i]);
        V2<<"Literalizing:\n  "<<ce2s(oi)<<" \n  into "<<ce2s(newoi)<<endl;
        root->relocate_consumers(oi->output,newoi->output); note_change(oi);
        root->delete_node(oi); // kill old op
      }
    }
  }
};

/*****************************************************************************
 *  DEAD CODE ELIMINATOR                                                     *
 *****************************************************************************/

//  - mark the output and each Operator w. a side-effect attribute["SideEffect"]
//  - mark every node as "dead"
//  - erase marks on everything leading to a side-effect or an output
//  - erase everything still marked as "dead"

map<string,pair<bool,int> > arg_mem;
int mem_arg(Args* args,string name,int defaultv) {
  if(!arg_mem.count(name)) {
    if(args->extract_switch(name.c_str()))
      arg_mem[name] = make_pair<bool,int>(true,args->pop_int());
    else
      arg_mem[name] = make_pair<bool,int>(false,-1);
  }
  return arg_mem[name].first ? arg_mem[name].second : defaultv;
}

class DeadCodeEliminator : public IRPropagator {
 public:
  Fset kill_f; AMset kill_a;
  DeadCodeEliminator(DFGTransformer* par, Args* args)
    : IRPropagator(true,false,true) {
    verbosity=mem_arg(args,"--dead-code-eliminator-verbosity",par->verbosity);
  }
  
  virtual void print(ostream* out=0) { *out << "DeadCodeEliminator"; }
  void preprop() { kill_f = worklist_f; kill_a = worklist_a; }
  void postprop() {
    any_changes = !kill_f.empty() || !kill_a.empty();
    while(!kill_f.empty()) {
      Field* f = *kill_f.begin(); kill_f.erase(f);
      V2<<"Deleting field "<<ce2s(f)<<endl;
      root->delete_node(f->producer);
    }
    while(!kill_a.empty()) {
      AM* am = *kill_a.begin(); kill_a.erase(am);
      V2<<"Deleting AM "<<ce2s(am)<<endl;
      root->delete_space(am);
    }
  }
  
  void act(Field* f) {
	V5 << "Dead Code Eliminator examining field " << ce2s(f) << endl;
    if(!kill_f.count(f)) return; // only check the dead
    bool live=false; string reason = "";
    if(f->is_output()) { live=true; reason="output";} // output is live
    if(f->producer->op->attributes.count(":side-effect"))
      { live=true; reason="side effect"; }
    for_set(Consumer,f->consumers,i) { // live consumer -> live
      V5 << "Consumer: " << ce2s((*i).first) << endl;
      // ignore non-called functions
      if(root->relevant.count((*i).first->domain()->root())==0) continue;
      // check for live consumers
      if(!kill_f.count((*i).first->output)) 
        {live=true;reason="consumer";break;}
    }
    for_set(AM*,f->selectors,ai) // live selector -> live
      if(!kill_a.count(*ai)) {live=true;reason="selector";break;}
    V4<<"Is field "<<ce2s(f)<<" live? "<<b2s(live)<<"("<<reason<<")"<<endl;
    if(live) { kill_f.erase(f); note_change(f); }
  }
  
  void act(AM* am) {
	V5 << "Dead Code Eliminator examining AM " <<ce2s(am) << endl;
    if(!kill_a.count(am)) return; // only check the dead
    bool live=false;
    for_set(AM*,am->children,i)
      if(!kill_a.count(*i)) {live=true;break;} // live child -> live
    for_set(Field*,am->fields,i)
      if(!kill_f.count(*i)) {live=true;break;} // live domain -> live
    if(am->bodyOf!=NULL) {live=true;} // don't delete root AMs
    if(live) { kill_a.erase(am); note_change(am); }
  }
};


/*****************************************************************************
 *  INLINING                                                                 *
 *****************************************************************************/

#define DEFAULT_INLINING_THRESHOLD 10
class FunctionInlining : public IRPropagator {
 public:
  int threshold; // # ops to make something an inlining target
  FunctionInlining(DFGTransformer* parent, Args* args)
    : IRPropagator(false,true,false) {
    verbosity = args->extract_switch("--function-inlining-verbosity") ? 
      args->pop_int() : parent->verbosity;
    threshold = args->extract_switch("--function-inlining-threshold") ?
      args->pop_int() : DEFAULT_INLINING_THRESHOLD;
  }
  virtual void print(ostream* out=0) { *out << "FunctionInlining"; }
  
  void act(OperatorInstance* oi) {
    if(!oi->op->isA("CompoundOp")) return; // can only inline compound ops
    if(oi->recursive()) return; // don't inline recursive
    // check that either the body or the container is small
    Fset bodyfields;
    int bodysize = dynamic_cast<CompoundOp &>(*oi->op).body->size();
    int containersize = oi->output->domain->root()->size();
    if(threshold!=-1 && bodysize>threshold && containersize>threshold) return;
    
    // actually carry out the inlining
    V2<<"Inlining function "<<ce2s(oi)<<endl;
    note_change(oi); root->make_op_inline(oi);
  }
};


/*****************************************************************************
 *  TOP-LEVEL ANALYZER                                                       *
 *****************************************************************************/

// some test classes
class InfiniteLoopPropagator : public IRPropagator {
public:
  InfiniteLoopPropagator() : IRPropagator(true,false) {}
  void act(Field* f) { note_change(f); }
};
class NOPPropagator : public IRPropagator {
public:
  NOPPropagator() : IRPropagator(true,true,true) {}
};


ProtoAnalyzer::ProtoAnalyzer(NeoCompiler* parent, Args* args) {
  verbosity = args->extract_switch("--analyzer-verbosity") ? 
    args->pop_int() : parent->verbosity;
  max_loops=args->extract_switch("--analyzer-max-loops")?args->pop_int():10;
  paranoid = args->extract_switch("--analyzer-paranoid")|parent->paranoid;
  // set up rule collection
  rules.push_back(new TypePropagator(this,args));
  rules.push_back(new ConstantFolder(this,args));
  rules.push_back(new Literalizer(this,args));
  rules.push_back(new DeadCodeEliminator(this,args));
  rules.push_back(new FunctionInlining(this,args));
}


/*****************************************************************************
 *  GLOBAL-TO-LOCAL TRANSFORMER                                              *
 *****************************************************************************/

// From interpreter.
Operator *op_err(CompilationElement *where, string msg);

// Change neighborhood operations to fold-hoods.
//
// Mechanism:
// 1. Find the summary operation.
// 2. Get the tree of field ops leading to it and turn them into a compound op.
// 3. Combine inputs to nbr ops into a tuple input.
// 4. Mark inputs to locals with "loopref".  (XXX ?)
// 5. ???
// 6. Profit!

class HoodToFolder : public IRPropagator {
public:
  HoodToFolder(GlobalToLocal *parent, Args *args) : IRPropagator(false, true) {
    verbosity = args->extract_switch("--hood-to-folder-verbosity") ?
      args->pop_int() : parent->verbosity;
  }
  virtual void print(ostream *out = 0) { *out << "HoodToFolder"; }
  void act(OperatorInstance *oi);

private:
  CEmap(CompoundOp *, CompoundOp *) localization_cache;
  Operator *localize_operator(Operator *op);
  CompoundOp *localize_compound_op(CompoundOp *cop);
  CompoundOp *tuplize_inputs(CompoundOp *cop);
  CompoundOp *add_detuplization(CompoundOp *cop);
  CompoundOp *make_nbr_subroutine(OperatorInstance *oi, Field **exportf);
  Operator *nbr_op_to_folder(OperatorInstance *oi);
  void restrict_elements(OIset* elts,vector<Field *>* exports, AM* am,OI* summary_op);
};

/**
 * Turn the inputs of this compound op into a tuple
 * This is done when we change the output to a tuple and need the inputs to match,
 *   since the function is used recursively
 */
CompoundOp *
HoodToFolder::tuplize_inputs(CompoundOp *cop)
{
	Signature* sig = cop->signature;
	V3 << "Signature before tuplize " << ce2s(sig) << endl;
	ProtoTuple* ret = new ProtoTuple(!sig->rest_input);
	for(int i=0; i<sig->n_fixed(); i++) {
	   ProtoType* elt = ProtoType::clone(sig->nth_type(i));
	   ret->add(elt);
	}
	 if(sig->rest_input!=NULL) // rest is unbounded end to tuple
	   ret->add(sig->rest_input);

    V3 << "Tuplize inputs tup ret " << ce2s(ret) << endl;

	sig->required_inputs.clear();
	sig->optional_inputs.clear();
	sig->required_inputs.push_back(ret);
	sig->rest_input = NULL;

	V3 << "Tuplize inputs new Signature " << ce2s(sig) << endl;
	return cop;
}

/**
 * Detuplization:
 *   Only called when the input is a tuple
 *    Create a new parameter for the tuple input
 *    Replace all Parameters with an elt
 *      link the input to the elt to the tuple parameter
 *      link anything that had the original parameter as an input to the output of the elt instead
 */
CompoundOp *
HoodToFolder::add_detuplization(CompoundOp *cop)
{
	V3 << "Sig in add_detup " << ce2s(cop->signature) << endl;

	Field* tupParam = NULL;

	CEmap(OI*,OI*) paramMap;
	OIset ois;
	cop->body->all_ois(&ois);
	int eltIndex = 0;

	for_set(OI *, ois, i) {
	  OI *oi = *i;
	  V4 << "Examining operator " << ce2s(oi->op) << endl;
	  if (oi->op->isA("Parameter")) {
		  if (tupParam == NULL) {
			tupParam = root->add_parameter(cop,make_gensym("tuparg")->name,eltIndex,cop->body,oi);
			tupParam->range = new ProtoTuple();
			V3 << "Adding new tup Parameter " << ce2s(tupParam) << endl;
		  }
		  OI* newElt = new OperatorInstance(cop, Env::core_op("elt"), cop->body);
		  paramMap[oi] = newElt;
		  newElt->output->range = oi->output->range;
		  ProtoScalar* si = new ProtoScalar(eltIndex);
		  Field* sField = root->add_literal(si, oi->domain(), oi);
		  eltIndex++;
		  newElt->add_input(tupParam);
		  newElt->add_input(sField);
		  V3 << "Adding new Elt " << ce2s(newElt) << endl;
	  }
	}

	for_set(OI *, ois, i) {
	   OI *oi = *i;
	 
	   for (int j=0; j<oi->inputs.size(); ++j) {
		  Field* iField = oi->inputs[j];
		  OI* mOi = paramMap[iField->producer];
		  if (mOi != NULL) {
			  V5 << "Relocating input " << j << " for " << ce2s(oi) << " to " << ce2s(mOi) << endl;
			  root->relocate_source(oi, j, mOi->output);
		  }
	   }
	}

}

void
HoodToFolder::act(OperatorInstance *oi)
{
  // Conversions begin at summaries (field-to-local ops).
  if (oi->inputs.size() == 1 && oi->inputs[0] != NULL
      && oi->inputs[0]->range->isA("ProtoField")
      && oi->output->range->isA("ProtoLocal")) {
    V2 << "Changing to fold-hood: " << ce2s(oi) << endl;
    // (fold-hood-plus folder fn input)
    AM *space = oi->output->domain;
    Field *exportf;
    CompoundOp *nbrop = make_nbr_subroutine(oi, &exportf);
    Operator *folder = nbr_op_to_folder(oi);
    OI *noi = new OperatorInstance(oi, Env::core_op("fold-hood-plus"), space);
    // Hook the inputs up to the fold-hood.
    V3 << "Connecting inputs to foldhood\n";
    noi->add_input(root->add_literal(new ProtoLambda(folder), space, oi));
    noi->add_input(root->add_literal(new ProtoLambda(nbrop), space, oi));
    noi->add_input(exportf);
    // Switch consumers and quit.
    V3 << "Changing over consumers\n";
    root->relocate_consumers(oi->output, noi->output); note_change(oi);
    root->delete_node(oi);
  }
}

Operator *
HoodToFolder::localize_operator(Operator *op)
{
  V4 << "Localizing operator: " << ce2s(op) << endl;
  if (op->isA("Literal")) {
    Literal *literal = &dynamic_cast<Literal &>(*op);
    // Strip any field.
    if (literal->value->isA("ProtoField"))
      return new Literal(op, F_VAL(literal->value));
    else
      return op;
  } else if (op->isA("Primitive")) {
    Operator *local = LocalFieldOp::get_local_op(op);
    return local ? local : op;
  } else if (op->isA("CompoundOp")) {
    return localize_compound_op(&dynamic_cast<CompoundOp &>(*op));
  } else if (op->isA("Parameter")) {
    // Parameters are always OK.
    return op;
  } else {
    ierror("Don't know how to localize operator: " + ce2s(op));
  }
}

CompoundOp *
HoodToFolder::localize_compound_op(CompoundOp *cop)
{
  V5 << "Checking localization cache\n";
  if (localization_cache.count(cop))
    return localization_cache[cop];

  // If it's already pointwise, just return.
  V5 << "Checking whether op is already pointwise\n";
  Fset fields;
  cop->body->all_fields(&fields);
  bool local = true;
  for_set(Field *, fields, i) {
    V5 << "Considering field " << ce2s(*i) << endl;
    if ((*i)->range->isA("ProtoField"))
      local = false;
  }
  if (local)
    return cop;

  // Walk through ops: local & nbr ops are flattened, others are localized.
  V5 << "Transforming op to local\n";
  CompoundOp *new_cop = new CompoundOp(cop);
  OIset ois;
   new_cop->body->all_ois(&ois);
   for_set(OI *, ois, i) {
     OI *oi = *i;
     if (oi->op == Env::core_op("nbr")) {
      root->relocate_consumers(oi->output, oi->inputs[0]);
    } else if (oi->op == Env::core_op("local")) {
      ierror("No locals should be found in an extracted hood function");
    } else if (oi->op == Env::core_op("restrict")) {
      // restrict is not change to reference here: it will be handled later.
      OI *src = (oi->inputs[0] ? oi->inputs[0]->producer : 0);
      if (src != 0 && src->op == Env::core_op("local")) {
        root->relocate_source(oi, 0, src->inputs[0]);
        V4 << "Relocated source: " << ce2s(oi) << endl;
        V4 << "OI->Inputs[0] = " << ce2s(oi->inputs[0]) << endl;
      }
    } else if (!oi->op->isA("FieldOp") && oi->op->isA("Primitive") && (oi->op->signature->output->isA("ProtoField"))) {
    	V2 << "Output is a Field but this isn't a FieldOp, this must be a primitive field function" << endl;
    	// We can localize it using localize_operator, but we want to keep the op name the same, since it will emit as the primitive
    	string pName = oi->op->name;
    	oi->op = localize_operator(oi->op);
    	// Fix output.
    	oi->output->range = oi->op->signature->output;
    	// Set the name back
    	oi->op->name = pName;
    } else {
      oi->op = localize_operator(oi->op);
      // Fix output.
      oi->output->range = oi->op->signature->output;
    }
  }
  localization_cache[cop] = new_cop;
  return new_cop;
}

/// If any elements in OIset* have domains in am's parents, 
/// replace them with copies restricted to am.  Likewise,
/// change exports to reference restricted versions
void
HoodToFolder::restrict_elements(OIset* elts,vector<Field *>* exports,AM* am,OI* summary_op) {
  CEmap(OI*,OI*) omap; // remapped OIs
  CEmap(Field*,Field*) fmap; // remapped fields

  // create all replacements
  for_set(OI*,*elts,oi) {
    if((*oi)->op==Env::core_op("restrict")) {
      omap[*oi] = NULL;  // Restrictions are simply elided
    } else if((*oi)->output->domain == am || 
              (*oi)->output->domain->child_of(am)) {
      continue; // ignore elements that don't need restriction
    } else if(am->child_of((*oi)->output->domain)) {
      // remap elements computed with a larger domain to a new instance
      omap[*oi] = new OperatorInstance(*oi,(*oi)->op,am);
      omap[*oi]->output->range = (*oi)->output->range;
      fmap[(*oi)->output] = omap[*oi]->output;
      for(int i=0;i<(*oi)->inputs.size();i++)
        omap[*oi]->add_input((*oi)->inputs[i]);
    } else {
      ierror("Can't restrict "+(*oi)->output->to_str()+" to "+am->to_str());
    }
  }

  // remap restriction outputs
  for_set(OI*,*elts,oi) {
    if((*oi)->op==Env::core_op("restrict")) {
      // search upstream for a non-restrict input
      OI* src = *oi; 
      while(src->op==Env::core_op("restrict")) { src=src->inputs[0]->producer; }
      // ensure this is part of the restricted set (it should always be)
      if(!elts->count(src) || !omap.count(src)) 
        ierror("Restrict leads out of expected neighborhood computation ops");
      fmap[(*oi)->output] = fmap[src->output];
    }
  }

  // change contents of elts
  for_map(OI*,OI*,omap,i) {
    elts->erase(i->first);
    if(i->second!=NULL) { elts->insert(i->second); }
  }
  // remap all input fields (outputs have been handled by OI copying)
  for_set(OI*,*elts,oi) {
    for(int i=0;i<(*oi)->inputs.size();i++)
      if(fmap.count((*oi)->inputs[i]))
        root->relocate_source(*oi,i,fmap[(*oi)->inputs[i]]);
  }
  // swap output if appropriate
  if(fmap.count(summary_op->inputs[0]))
    root->relocate_source(summary_op,0,fmap[summary_op->inputs[0]]);

  // Now walk through the exports, restricting and replacing as needed
  for(int i=0;i<exports->size();i++) {
    Field* ef = (*exports)[i];
    if(ef->domain == am || ef->domain->child_of(am)) {
      continue; // ignore elements that don't need restriction
    } else if(am->child_of(ef->domain)) {
      // create a new restriction
      OI* restrict = new OperatorInstance(ef,Env::core_op("restrict"),am);
      restrict->add_input(ef); 
      restrict->add_input(am->selector); // selector known to be non-null
      restrict->output->range = ef->range; // copy range
      // remap elements that used the old export to the new restricted one
      for_set(OI*,*elts,oi) {
        for(int j=0;j<(*oi)->inputs.size();j++) {
          if((*oi)->inputs[j] == ef) {
            root->relocate_source(*oi,j,restrict->output);
          }
        }
      }
      // change the export
      (*exports)[i] = restrict->output;
    } else {
      ierror("Can't restrict export "+ef->to_str()+" to "+am->to_str());
    }    
  }
}

CompoundOp *
HoodToFolder::make_nbr_subroutine(OperatorInstance *oi, Field **exportf)
{
  V3 << "Creating subroutine for: " << ce2s(oi) << endl;

  // First, find all field-valued ops feeding this summary operator.
  OIset elts;
  vector<Field *> exports;
  OIset q;
  q.insert(oi);
  while (q.size()) {
    OperatorInstance *next = *q.begin();
    q.erase(next);
    for (size_t i = 0; i < next->inputs.size(); i++) {
      Field *f = next->inputs[i];
      // Record exported fields.
      if (f->producer->op == Env::core_op("nbr")) {
        if (!f->producer->inputs.size())
          ierror("No input for nbr operator: " + f->producer->to_str());
        if (index_of(&exports, f->producer->inputs[0]) == -1)
          exports.push_back(f->producer->inputs[0]);
      }
      if (f->range->isA("ProtoField")
          && !elts.count(f->producer)
          && f->producer->op != Env::core_op("local"))
        { elts.insert(f->producer); q.insert(f->producer); }
    }
  }

  V3 << "Found " << elts.size() << " elements" << endl;
  for_set(OI*,elts,e) { V5 << (*e)->to_str() << endl; }
  V3 << "Found " << exports.size() << " exports" << endl;
  for(int i=0;i<exports.size();i++) { V5 << exports[i]->to_str() << endl; }

  // Some of the computation elements may have been computed on a
  // larger AM, and need to be restricted.  This cannot happen in the
  // compound op form.  However, since there are no "if" ops allowed
  // within a neighborhood computation, it is safe to map all elements
  // of the neighborhood computation into restricted copies; computation
  // that would have happened on other neighbors would just be discarded
  // In practice, what happens here is that any "restrict" is bubbled up
  // to be be at the exports instead.
  restrict_elements(&elts,&exports,oi->output->domain,oi);
  V3 << "Restricted elements" << endl;
  for_set(OI*,elts,e) { V5 << (*e)->to_str() << endl; }
  V3 << "Restricted exports" << endl;
  for(int i=0;i<exports.size();i++) { V5 << exports[i]->to_str() << endl; }
  V5 << "Summary is now: " << oi->to_str() << endl;

  // Create the compound op.
  V4 << "Creating compound operator from elements" << endl;
  CompoundOp *cop
    = root->derive_op(&elts, oi->domain(), &exports, oi->inputs[0],"Hood");
  V5 << "Localizing new compound operator " << ce2s(cop) << endl;
  cop = localize_compound_op(cop);

  // Construct input structure.
  if (exports.size() == 0) {
    V4 << "Zero exports: adding a scratch export" << endl;
    // Add a scratch input.
    ProtoType *scratch = new ProtoScalar(0);
    *exportf = root->add_literal(scratch, oi->domain(), oi);
    cop->signature->required_inputs.push_back(scratch);
    cop->signature->names["arg0"] = 0;
  } else if (exports.size() == 1) {
    V4 << "One export: using "<< ce2s(exports[0]) << " directly" << endl;
    *exportf = exports[0];
  } else {
    V4 << "Multiple exports: binding into a tuple" << endl;
    OI *tup = new OperatorInstance(oi, Env::core_op("tup"), oi->domain());
    for (size_t i = 0; i < exports.size(); i++)
      tup->add_input(exports[i]);
    *exportf = tup->output;
    tuplize_inputs(cop);
    add_detuplization(cop);
    V5 << "Cop signature after tuplize " << ce2s(cop->signature) << endl;
    V5 << "Cop export after tuplize " << ce2s(tup->output) << endl;
  }

  vector<Field *> in;
  in.push_back(*exportf);

  // Add multiplier if needed.
  if (oi->op->name == "int-hood") {
    V3 << "Adding int-hood multiplier\n";
    Operator *infinitesimal = Env::core_op("infinitesimal");
    OI *oin = new OperatorInstance(oi, infinitesimal, cop->body);
    OI *tin = new OperatorInstance(oi, Env::core_op("*"), cop->body);
    tin->add_input(oin->output);
    tin->add_input(cop->output);
    cop->output = tin->output;
  }
  return cop;
}

Operator *
HoodToFolder::nbr_op_to_folder(OperatorInstance *oi)
{
  const string &name = oi->op->name;
  V3 << "Selecting folder for: " << name << endl;
  if      (name == "min-hood") return Env::core_op("min");
  else if (name == "max-hood") return Env::core_op("max");
  else if (name == "any-hood") return Env::core_op("max");
  else if (name == "all-hood") return Env::core_op("min");
  else if (name == "int-hood") return Env::core_op("+");
  else if (name == "sum-hood") return Env::core_op("+");
  else
    return
      op_err(oi, "Can't convert summary '" + name + "' to local operator");
}

// Changes restrict/mux complexes to branches
// Mechanism:
// 1. find complementary AM selector pairs
// 2. turn each sub-AM into a no-argument function
// 3. hook these functions into a branch operator and delete the old

class RestrictToBranch : public IRPropagator {
public:
  RestrictToBranch(GlobalToLocal* parent, Args* args) : IRPropagator(false,true) {
    verbosity = args->extract_switch("--restrict-to-branch-verbosity") ?
      args->pop_int() : parent->verbosity;
  }
  virtual void print(ostream* out=0) { *out << "RestrictToBranch"; }

  CompoundOp* am_to_lambda(AM* space,Field *out,string stem) {
    // discard immediate-child restrict functions:
    V4 << "Converting 2-input 'restrict' operators to references\n";
    Fset fields; fields = space->fields; // delete invalidates original iterator
    for_set(Field*,fields,i) {
      V4 << "Checking field: " << ce2s(*i) << endl;
      V4 << "  Producer: " << ce2s((*i)->producer) << endl;
      if((*i)->producer->op==Env::core_op("restrict") &&
         (*i)->producer->inputs.size()==2) {
        V4 << "Converting to reference: "+ce2s((*i)->producer)+"\n";
        (*i)->producer->remove_input(1);
        (*i)->producer->op = Env::core_op("reference");
      }
    }
    // make fn from all operators in the space, and all its children
    V4 << "Deriving operators from space "+ce2s(space)+"\n";
    OIset elts; space->all_ois(&elts);
    vector<Field*> ins; // no inputs
    CompoundOp* res = root->derive_op(&elts,space,&ins,out,stem);
    for_set(OI *, elts, i) {
      V2 << "OI ELT: " << ce2s(*i) << endl;
    }
    return res;
  }
  void act(OperatorInstance* oi) {
	// properly formed muxes are candidates for if patterns
    if(oi->op==Env::core_op("mux") && oi->inputs.size()==3) {
      //&& !oi->attributes.count("LETFED-MUX")) {
      bool letfedmux = false;
      if (oi->attributes.count("LETFED-MUX")) {
    	  letfedmux = true;
      }
      V3 << "Considering If->Branch candidate:\n   "<< ce2s(oi) <<endl;
      OI *join; Field *test, *testnot; AM *trueAM, *falseAM; AM* space;
      // fill in blanks
      join = oi; space = oi->output->domain; test = oi->inputs[0];
      trueAM = oi->inputs[1]->domain;
      falseAM = oi->inputs[2]->domain;
      testnot = falseAM->selector;
      // check for validity
      V4 << "Checking not operator\n";
      if(!testnot) return;
      if(!(testnot->producer->op==Env::core_op("not")
           && testnot->selectors.size()==1
           && testnot->producer->inputs[0]==test)) return;
      for_set(Consumer,testnot->consumers,i) // only restricts can consume
        if(!(i->first->op==Env::core_op("restrict") && i->second==1)) return;
      V4 << "Checking true-expression space\n";
      if(!(trueAM->selector==test && trueAM->parent==space)) return;
      V4 << "Checking false-expression space\n";
      if(!(falseAM->selector==testnot && falseAM->parent==space)) return;
      OI* branch = new OperatorInstance(oi,Env::core_op("branch"),space);
      V3 << "Transforming to true AM to a lambda function" << endl;
      CompoundOp *tf = am_to_lambda(trueAM,oi->inputs[1],letfedmux?"Init":"TrueBranch");
      V3 << "Transforming to false AM to a lambda function" << endl;
      CompoundOp *ff = am_to_lambda(falseAM,oi->inputs[2],letfedmux?"Update":"FalseBranch");
      OI* newoi;
      if (!letfedmux) {
        // Swap the mux for a branch:
        V3 << "Transforming to branch\n";
        tf->body->mark("branch-fn"); ff->body->mark("branch-fn");
        branch->add_input(test);
        branch->add_input(root->add_literal(new ProtoLambda(tf),space,tf));
        branch->add_input(root->add_literal(new ProtoLambda(ff),space,ff));
        root->relocate_consumers(oi->output,branch->output);
        newoi = branch;
      } else {
         V3 << "ff Signature: " << ce2s(ff->signature) << endl;
         ff->signature->required_inputs.push_back(oi->inputs[2]->range);
         V3 << "ff Signature: " << ce2s(ff->signature) << endl;
    	  // Easier to create a new mux than move everything
    	 OI* newmux = new OperatorInstance(oi, Env::core_op("mux"), space);
       	 newmux->add_input(test);
    	 newmux->add_input(root->add_literal(new ProtoLambda(tf),space,tf));
    	 newmux->add_input(root->add_literal(new ProtoLambda(ff),space,ff));
    	 newmux->output->range = oi->output->range;
    	 root->relocate_consumers(oi->output, newmux->output);
    	 V5 << "Test: " << ce2s(test) << endl;
    	 for_set(Consumer,test->consumers,i) {
           V5 << "Test Consumer: " << ce2s((*i).first) << endl;
    	 }
    	 newoi = newmux;
      }
      V3 << "Removing old OIs" << endl;
      // delete old elements

      // 1) Find the new Delay, if there is one
      OI* newDelay = NULL;
      OIset br_ops1; tf->body->all_ois(&br_ops1); ff->body->all_ois(&br_ops1);
      for_set(OI*,br_ops1,i) {
      	  if ((*i)->op == Env::core_op("delay")) {
      		  if (newDelay == NULL) {
    		    newDelay = *i;
    	      } else {
    	    	  ierror("Multiple delays in the same update function, this is unexpected.");
    	      }
      	  }
      }

      V3 << "New Delay is " << ce2s(newDelay) << endl;
      OIset elts; trueAM->all_ois(&elts); falseAM->all_ois(&elts);
      // Move any other references to this delay to the new branch
      for_set(OI*,elts,i) {
    	V3 << "Removing OI: " << ce2s(*i) << endl;
    	if ((*i)->op == Env::core_op("delay")) {
          OI* delay = *i;
    	  std::vector<OI*> relocaters;
          V3 << "Delay output " << ce2s(delay->output) << endl;
    	  for_set(Consumer,delay->output->consumers,j) {
    	    OI* delayConsumer = (*j).first;
       		if (delayConsumer != NULL) {
    		  if (delayConsumer->op != NULL) {
    		    V3 << "Delay Consumer: " << ce2s(delayConsumer) << endl;
    			relocaters.push_back(delayConsumer);
    		  }
       		}
    	  }
    	  for (int k=0; k<relocaters.size(); ++k) {
    		  OI* delayConsumer = relocaters[k];
    		  V3 << "Relocating source from " << ce2s(delayConsumer) << " to " << ce2s(newDelay->output) << endl;
    		  root->relocate_source(delayConsumer, 0, newDelay->output);
    	  }
    	}
    	root->delete_node(*i);
      }
      root->delete_node(oi);
      root->delete_space(trueAM); root->delete_space(falseAM);
      root->delete_node(testnot->producer);
      // note all changes
      note_change(newoi);
      OIset br_ops; tf->body->all_ois(&br_ops); ff->body->all_ois(&br_ops);
      for_set(OI*,br_ops,i) {
    	  V5 << "Note change for " << ce2s(*i) << endl;
    	  note_change(*i);
      }
      V5 << "Test: " << ce2s(test) << endl;
      for_set(Consumer,test->consumers,i) {
         V5 << "Test Consumer: " << ce2s((*i).first) << endl;
      }
    }
  }
};

class RestrictToReference : public IRPropagator {
public:
  RestrictToReference(GlobalToLocal* parent, Args* args) : IRPropagator(false,true) {
    verbosity = args->extract_switch("--restrict-to-reference-verbosity") ?
      args->pop_int() : parent->verbosity;
  }
  virtual void print(ostream* out=0) { *out << "RestrictToReference"; }

  void act(OperatorInstance* oi) {
      if(oi->op==Env::core_op("restrict") && oi->inputs.size()==1) {
    	  if (oi->inputs[0] != NULL) {
    		  V3 << "Converting restrict to reference: " << ce2s(oi) << endl;
              oi->op = Env::core_op("reference");
              note_change(oi);
    	  } else {
    		  V2 << "NULL Input in Restrict To Reference for " << ce2s(oi) << endl;
    	  }
      }
  }
};

// Map Stores to Reads
// TODO: Put this in the Read OperatorInstance, would need to extend OI to add a field and redo the below where we just change the delay to a read
// Need to map store and reads so we can get the right reference later in the emitter
std::map<OI*, OI*> readToStoreMap;

class DelayToStoreAndRead : public IRPropagator {
public:
  DelayToStoreAndRead(GlobalToLocal* parent, Args* args) : IRPropagator(false,true) {
    verbosity = args->extract_switch("--delay-to-store-and-read-verbosity") ?
    args->pop_int() : parent->verbosity;
  }
  virtual void print(ostream* out=0) { *out << "DelayToStoreAndRead"; }

  void act(OperatorInstance* oi) {
    if(oi->op==Env::core_op("delay")) {
      AM* space = oi->output->domain;
      V3 << "Converting delay to a store and a read: " << ce2s(oi) << endl;
      V3 << "Creating Store" << endl;
      OI* mux = oi->inputs[0]->producer;
      if (mux->op == Env::core_op("mux")) {
    	  V3 << "Changing the delay to a read" << endl;
    	  oi->op = Env::core_op("read");
    	  oi->output->range = mux->output->range;
    	  mux->output->unuse(oi, 1);
    	  oi->remove_input(0);

    	  OI* store = new OperatorInstance(mux, Env::core_op("store"), mux->output->domain);
       	  root->relocate_consumers(mux->output, store->output);
    	  store->add_input(mux->output);
    	  store->output->range = mux->output->range;
    	  store->clear_attribute("LETFED-MUX");

    	  readToStoreMap[oi] = store;

    	  note_change(oi);
    	  note_change(mux);
    	  note_change(store);
    	  V3 << "Mux: " << ce2s(mux) << endl;
    	  V3 << "Read: " << ce2s(oi) << endl;
    	  V3 << "Store: " << ce2s(store) << endl;
    	  V3 << "Mux output: " << ce2s(mux->output) << endl;
      } else {
    	  V1 << "Delays input is not from a letfedmux!" << endl;
      }
    } else if(oi->op==Env::core_op("mux")) {
       // Handle the case where we have a dchange with no delay
       // This can happen if the update function ignores the rep variable
       //   So it's probably bad code
       //   may want to throw a compiler error instead?  But for now it's allowed so we have to handle it.
    	if (oi->attributes.count("LETFED-MUX")) {
    		Field* out = oi->output;
    		// If output goes to a delay, leave it alone here, the delay will handle it
    		for_set(Consumer,out->consumers,i) {
    		    if(i->first->op==Env::core_op("delay")) {
    		    	return;
    		    }
    		    if(i->first->op==Env::core_op("store")) {
    		    	// already did it
    		    	return;
    		    }
    		}
    		// No delay, and it's a letfed mux
    		// This is the silly case where there is no reference to the rep variable in the update
    		//   So the rep is really doing nothing (rep t 0 (+ 1 2))

    		// Add in a store, which will be emitted as a FeedbackOp for completeness and to match what the paleo compiler does
    		V3 << "Creating Store" << endl;
            OI* store = new OperatorInstance(oi, Env::core_op("store"), oi->output->domain);
         	root->relocate_consumers(oi->output, store->output);
      	    store->add_input(oi->output);
      	    store->output->range = oi->output->range;
      	    store->clear_attribute("LETFED-MUX");

      	    note_change(oi);
      	    note_change(store);
        	V3 << "Store: " << ce2s(store) << endl;
      	    V3 << "Mux output: " << ce2s(oi->output) << endl;
    	}
     }
  }
};


GlobalToLocal::GlobalToLocal(NeoCompiler* parent, Args* args) {
  verbosity=args->extract_switch("--localizer-verbosity") ? 
    args->pop_int() : parent->verbosity;
  max_loops=args->extract_switch("--localizer-max-loops")?args->pop_int():10;
  paranoid = args->extract_switch("--localizer-paranoid")|parent->paranoid;
  // set up rule collection
  rules.push_back(new HoodToFolder(this,args));
  rules.push_back(new RestrictToReference(this,args));
  rules.push_back(new RestrictToBranch(this,args));
  rules.push_back(new DelayToStoreAndRead(this, args));
  rules.push_back(new DeadCodeEliminator(this,args));
}

/* GOING GLOBAL TO LOCAL:
   Three transformations: restriction, feedback, neighborhood

   Hood: key question - how far should local computations reach?
     Proposal: all no-input-ops are stuck into a let via "local" op
     So... can identify subgraph that bounds inputs...
       put that subgraph into a new compound - second LAMBDA
       put all NBR ops into a tuple, which is used for the 3rd input
       <LAMBDA>, <LAMBDA>, <LOCAL> --> [fold-hood-plus] --> <LOCAL>
       lambda of a primitive op -> FUN_4_OP, REF_1_OP, REF_0_OP, <prim>, RET_OP
     What about fields used in two different operations?
       e.g. (let ((v (nbr x))) (- (min-hood v) (max-hood v)))
       that looks like two different exports...
 */

/*****************************************************************************
 *  GENERIC TRANSFORMATION CYCLER                                            *
 *****************************************************************************/

void DFGTransformer::transform(DFG* g) {
  CertifyBackpointers checker(verbosity);
  if(paranoid) checker.propagate(g); // make sure we're starting OK
  for(int i=0;i<max_loops;i++) {
    bool changed=false;
    for(int j=0;j<rules.size();j++) {
      changed |= rules[j]->propagate(g); terminate_on_error();
      if(paranoid) checker.propagate(g); // make sure we didn't break anything
    }
    if(!changed) break;
    if(i==(max_loops-1))
      compile_warn("Transformer giving up after "+i2s(max_loops)+" loops");
  }
  g->determine_relevant();
  checker.propagate(g); // make sure we didn't break anything
}

