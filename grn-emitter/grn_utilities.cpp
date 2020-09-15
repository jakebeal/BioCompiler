/* Utilities for managing GRN types
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#include "config.h"
#include <stdint.h>
#include <limits>
#include "grn_utilities.h"

namespace grn {

/*****************************************************************************
 *  NAME UTILITIES                                                           *
 *****************************************************************************/

// ensure that name contains only GraphViz & Matlab compatible characters
map<string,string> sanitized;
string sanitized_name(string name) {
  if(!sanitized.count(name)) {
    string removals = "*+\\-./<=>?&:"; // proto characters not allowed by dot IDs
    string newname = name;
    size_t found = newname.find_first_of(removals);
    while (found!=-1) {
      newname[found]='_';
      found = newname.find_first_of(removals,found+1);
    }
    if(!(newname == name))
      compile_warn("GraphViz/Matlab incompatible name '"+name+"' has been changed to '"+newname+"'");
    sanitized[name] = newname;
  }
  return sanitized[name];
}



/*****************************************************************************
 *  TYPE UTILITIES                                                           *
 *****************************************************************************/

bool is_constitutive_true(ProtoType* pt) {
  if(pt==NULL) return false;
  if(!pt->isA("ProtoBoolean")) return false;
  ProtoBoolean* b=dynamic_cast<ProtoBoolean*>(pt);
  return (b->constant && b->value);
}
bool is_constitutive_false(ProtoType* pt) {
  if(pt==NULL) return false;
  if(!pt->isA("ProtoBoolean")) return false;
  ProtoBoolean* b=dynamic_cast<ProtoBoolean*>(pt);
  return (b->constant && !b->value);
}

bool is_high_promoter(Promoter* p) { return is_constitutive_true(p->rate); }
bool is_low_promoter(Promoter* p) { return is_constitutive_false(p->rate); }

bool cmp_types(ProtoType* t1, ProtoType* t2) {
  if (t1->isA("ProtoBoolean")) {
    if (t2->isA("ProtoBoolean")) {
      ProtoBoolean* b1=dynamic_cast<ProtoBoolean*>(t1);
      ProtoBoolean* b2=dynamic_cast<ProtoBoolean*>(t2);
      if ((b1->constant == true) && (b2->constant == true))
        return (b1->value == b2->value);
      else if ((b1->constant == false) && (b2->constant == false))
        return true;
      else
        return false;
    }
    else
      return false;
  }
  else if (t1->isA("ProtoScalar")) {
    if (t2->isA("ProtoScalar")) {
      ProtoScalar* s1=dynamic_cast<ProtoScalar*>(t1);
      ProtoScalar* s2=dynamic_cast<ProtoScalar*>(t2);
      if ((s1->constant == true) && (s2->constant == true))
        return (s1->value == s2->value);
      else if ((s1->constant == false) && (s2->constant == false))
        return true;
      else
        return false;      
    }
    else 
      return false;
  }
  else if (t1->isA("ProtoNumber")) {
    if (t2->isA("ProtoNumber")) {
      return true;
    }
    else
      return false;
  }
  // we could handle more types here, but currently don't
  else
    return false;
}

bool equivalent_promoters(Promoter* p1, Promoter* p2) {
  return cmp_types(p1->rate, p2->rate) && equivalent_regulation(p1,p2);
}

bool equivalent_regulation(DNAComponent* a, DNAComponent* b) {
  if(a->regulators.size() != b->regulators.size()) return false;
  std::set<ExpressionRegulation*>::iterator ai = a->regulators.begin();
  std::set<ExpressionRegulation*>::iterator bi = b->regulators.begin();
  for ( ; ai != a->regulators.end(); ++ai, ++bi) {
    if(!(*ai)->same_regulation(*bi)) return false;
  }
  return true;
}

string scalar_to_str(ProtoScalar* s) {
  if (s->isA("ProtoBoolean")) {
    ProtoBoolean* b=dynamic_cast<ProtoBoolean*>(s);
    if (b->constant == true)
      return b->value?"high":"low";
    else
      return "boolean";
  }
  else {
    if (s->constant == true)
      return f2s(s->value,3);
    else
      return "scalar";
  }
}

float force_promoter_rate(Promoter* p) {
  // convert the Promoter rate to a numeric value
  if (is_low_promoter(p)) {
    return DEFAULT_LOW_RATE;
  } else if (is_high_promoter(p)) {
    return DEFAULT_HIGH_RATE;
  } else if (p->rate->isA("ProtoBoolean")) {
    ierror("Found a promoter not marked 'high' or 'low': assuming it should be 'low'.");
    // boolean, but not high or low, default to low (maybe should be an error
    return DEFAULT_LOW_RATE;
  } else if (p->rate->constant == true) {
    return p->rate->value;
  } else {
    ierror("Promoter rate constant required but neither supplied not able to be inferred: "+p->to_str());
    // no value for scalar
    return DEFAULT_LOW_RATE;
  }
}

// Given a chemical and a SExpr that should specify type, 
// convert SExpr to ProtoType (which does error checking internally)
// Then make sure that the chemical is not already typed
// with a conflicting type. 
void convert_sexp_to_type(SExpr* type, Chemical* chem, OperatorInstance* oi) {
  ProtoType* pt = NULL;
  // Check if type is a reference
  if(type->isSymbol())
    pt = oi->op->signature->parameter_type((SE_Symbol*)type,false);
  // If that fails, interpret as type
  if(pt==NULL) pt = ProtoInterpreter::sexp_to_type(type);
  
  // need to check to see if this conflicts with the type we 
  // have for chemical c
  if (chem->attributes["type"] == NULL)
    chem->attributes["type"] = new ProtoTypeAttribute(pt);
  else if(!cmp_types(((ProtoTypeAttribute*)chem->attributes["type"])->type, pt))
    compile_error("Conflicting types detected! "+pt->type_of()+" "+((ProtoTypeAttribute*)chem->attributes["type"])->type->type_of());
}

ProtoType* get_chemical_type(Chemical* chem) {
  if(!chem->attributes.count("type")) return NULL;
  ProtoTypeAttribute* pta = (ProtoTypeAttribute*)chem->attributes["type"];
  return pta->type;
}

/*****************************************************************************
 *  GRN COMPARISON AND EDITING                                               *
 *****************************************************************************/
bool one_to_one(Chemical* c) {
  return (c->producers.size()==1 && c->consumers.size()==1 && 
          c->regulatorFor.size()==0 && c->regulatedBy.size()==0);
}

bool sole_regulator(ExpressionRegulation* er) {
  FunctionalUnit* fu = er->target->container;
  bool found = false;
  for(int i=0;i<fu->sequence.size();i++) { 
    for_set(ExpressionRegulation*,fu->sequence[i]->regulators,er2) {
      if(er->same_regulation(*er2)) { found = true;
      } else { return false;
      }
    }
  }
  return found;
}

void delete_feature(DNAComponent* dc) {
  FunctionalUnit* fu = dc->container;
  int i=0;
  for(;i<fu->sequence.size();i++) { if(fu->sequence[i]==dc) break; }
  for(;i<fu->sequence.size()-1;i++) { fu->sequence[i]=fu->sequence[i+1]; }
  fu->sequence.pop_back();
}

// use naive graph distance to return a value between diameter and diameter*2

class DiameterEstimator : public GRNPropagator {
public:
  int diameter_estimate;
  static const string TAG;

  DiameterEstimator() { verbosity = 0; }
  virtual void print(ostream* out=0) { *out << "DiameterEstimator"; }

  // preprop: initialize minimum diameter estimate, mark an arbitrary object
  void preprop() { 
    diameter_estimate = 0; 
    // mark precisely one functional unit (if one exists)
    for_set(DNAComponent*,grn->dnacomponents,i) { 
      (*i)->attributes[TAG] = new IntAttribute(0);
      break;
    }
    for_set(DNAComponent*,grn->dnacomponents,i) {
      if((*i)->marked(TAG)) {
        V4 <<(*i)->to_str()<<" has tag: "<< ((IntAttribute*)(*i)->attributes[TAG])->value<<endl;
      } else {
        V4 <<(*i)->to_str()<<" has no tag."<<endl;
      }
    }
  }
  // postprop: remove markers
  void postprop() {
    for_set(DNAComponent*,grn->dnacomponents,i){ (*i)->clear_attribute(TAG); }
    for_map(string,Chemical*,grn->chemicals,i){i->second->clear_attribute(TAG);}
  }
  void act(Chemical* c) {
    // Start with old value
    int oldvalue = std::numeric_limits<int32_t>::max();
    if(c->marked(TAG)) { oldvalue = ((IntAttribute*)c->attributes[TAG])->value;}

    // Walk all neighbors to get new value
    int newvalue = oldvalue;
    for_set(CodingSequence*,c->producers,i) {
      FunctionalUnit *fu = (*i)->container;
      if(fu->marked(TAG)) 
        newvalue = min(newvalue,1+((IntAttribute*)fu->attributes[TAG])->value);
    }
    for_set(ExpressionRegulation*,c->consumers,i) {
      FunctionalUnit *fu = (*i)->target->container;
      if(fu->marked(TAG)) 
        newvalue = min(newvalue,1+((IntAttribute*)fu->attributes[TAG])->value);
    }
    for_set(RegulatoryReaction*,c->regulatedBy,i) {
      Chemical* c = (*i)->regulator;
      if(c->marked(TAG)) 
        newvalue = min(newvalue,1+((IntAttribute*)c->attributes[TAG])->value);
    }
    for_set(RegulatoryReaction*,c->regulatorFor,i) {
      Chemical* c = (*i)->substrate;
      if(c->marked(TAG)) 
        newvalue = min(newvalue,1+((IntAttribute*)c->attributes[TAG])->value);
    }

    // Finally, update value and return
    if(newvalue < oldvalue) { 
      c->attributes[TAG] = new IntAttribute(newvalue);
      diameter_estimate = max(diameter_estimate,newvalue);
      note_change(c); 
    }
  }

  void act(FunctionalUnit* fu) {
    // Start with old value
    int oldvalue = std::numeric_limits<int32_t>::max();
    if(fu->marked(TAG)){oldvalue = ((IntAttribute*)fu->attributes[TAG])->value;}

    // Walk all neighbors to get new value
    int newvalue = oldvalue;
    for(int i=0;i<fu->sequence.size();i++) {
      DNAComponent* dc = fu->sequence[i];
      for_set(ExpressionRegulation*,dc->regulators,er) {
        Chemical* c = (*er)->signal;
        if(c->marked(TAG)) 
          newvalue = min(newvalue,1+((IntAttribute*)c->attributes[TAG])->value);
      }
      if(dc->isA("CodingSequence")) {
        Chemical* c = ((CodingSequence*)dc)->product;
        if(c->marked(TAG)) 
          newvalue = min(newvalue,1+((IntAttribute*)c->attributes[TAG])->value);
      }
    }
    // Finally, update value and return
    if(newvalue < oldvalue) { 
      fu->attributes[TAG] = new IntAttribute(newvalue);
      diameter_estimate = max(diameter_estimate,newvalue);
      note_change(fu); 
    }
  }

};

const string DiameterEstimator::TAG = "DiameterEstimator:tmp";

int estimate_diameter(GRN* net) {
  DiameterEstimator de;
  de.propagate(net);
  return de.diameter_estimate;
}

/*****************************************************************************
 *  PROPAGATOR                                                               *
 *****************************************************************************/

bool GRNPropagator::propagate(GRN* net) {
  if(verbosity>=1) *cpout << "Executing GRN analyzer " << to_str() << endl;
  any_changes=false; grn=net;
  
  // check for an empty network:
  if (net->dnacomponents.empty() && net->chemicals.empty()) {
    compile_error("Optimization reduced the network to an empty network.  Does your program have sensors and actuators?"); 
    terminate_on_error();
  }
    
  // initialize worklists
  worklist_f.clear(); worklist_f = net->dnacomponents;
  worklist_c.clear(); 
  map<string,Chemical*>::iterator i=net->chemicals.begin();
  for( ;i!=net->chemicals.end();i++) worklist_c.insert((*i).second);
  // walk through worklists until empty
  preprop();
  int steps_remaining = loop_abort*(worklist_c.size()+worklist_f.size());
  while(steps_remaining>0 && (!worklist_c.empty() || !worklist_f.empty())) {
    // each time through, try executing one from each worklist
    if(!worklist_c.empty()) {
      Chemical* c = *worklist_c.begin(); worklist_c.erase(c); 
      if(grn->chemicals.count(c->name)) // ignore deleted
        { act(c); steps_remaining--; }
    }
    if(!worklist_f.empty()) {
      FunctionalUnit* f = (FunctionalUnit*)*worklist_f.begin(); worklist_f.erase(f); 
      if(grn->dnacomponents.count(f)) // ignore deleted
        { act(f); steps_remaining--; }
    }
    V5<<grn->to_str();
  }
  if(steps_remaining<=0) ierror("A GRN analyzer aborted due to apparent infinite loop.");
  postprop();
  if(verbosity>=3) *cpout << "Done executing GRN analyzer " << to_str() << endl;
  return any_changes;
}

enum { FU_MARK=1, C_MARK=2 };
CompilationElement* bsrc;
set<CompilationElement*,CompilationElement_cmp> bqueued;
void GRNPropagator::queue_nbrs(Chemical* c, int marks) {
  if(marks&C_MARK || bqueued.count(c)) return;   bqueued.insert(c);
  if(c!=bsrc) { worklist_c.insert(c); marks |= C_MARK; }

  for_set(ExpressionRegulation*,c->consumers,ci)
    { queue_nbrs((*ci)->target->container,marks); }
  for_set(CodingSequence*,c->producers,pi)
    { queue_nbrs((*pi)->container,marks); }
  for_set(RegulatoryReaction*,c->regulatorFor,ri)
    { worklist_c.insert((*ri)->substrate); }
  for_set(RegulatoryReaction*,c->regulatedBy,ri)
    { worklist_c.insert((*ri)->regulator); }
}
void GRNPropagator::queue_nbrs(FunctionalUnit* fu, int marks) {
  if(marks&FU_MARK || bqueued.count(fu)) return;   bqueued.insert(fu);
  if(fu!=bsrc) { worklist_f.insert(fu); marks |= FU_MARK; }

  for(int i=0;i<fu->sequence.size();i++) {
    DNAComponent* dc = fu->sequence[i];
    for_set(ExpressionRegulation*,dc->regulators,er) {
      queue_nbrs((*er)->signal,marks);
    }
    if(dc->isA("CodingSequence")) {
      queue_nbrs(((CodingSequence*)dc)->product,marks);
    }
  }
}

void GRNPropagator::note_change(Chemical* c) 
{ bqueued.clear(); any_changes=true; bsrc=c; queue_nbrs(c); }
void GRNPropagator::note_change(FunctionalUnit* f) 
{ bqueued.clear(); any_changes=true; bsrc=f; queue_nbrs(f); }

}
