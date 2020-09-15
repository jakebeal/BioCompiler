/* Proto type system
Copyright (C) 2009, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <algorithm>
#include <iostream>

#include "config.h"

#include "ir.h"
#include "nicenames.h"

using namespace std;

/*****************************************************************************
 *  CLONING                                                                  *
 *****************************************************************************/

ProtoType* ProtoType::clone(ProtoType* t) {
  if(t->type_of()=="ProtoType") {
    ProtoType* newt = new ProtoType();
    newt->inherit_attributes(t); return newt;
  } else if(t->type_of()=="ProtoLocal") {
    ProtoLocal* newt = new ProtoLocal();
    newt->inherit_attributes(t); return newt;
  } else if(t->type_of()=="ProtoTuple") { 
    ProtoTuple* newt = new ProtoTuple(T_TYPE(t));
    return newt; // inheritance handled in constructor
  } else if(t->type_of()=="ProtoSymbol") {
    ProtoSymbol* oldt = &dynamic_cast<ProtoSymbol &>(*t);
    ProtoSymbol* newt = 
      (oldt->constant? new ProtoSymbol(oldt->value) : new ProtoSymbol());
    newt->inherit_attributes(t); return newt;
  } else if(t->type_of()=="ProtoNumber") {
    ProtoNumber* newt = new ProtoNumber();
    newt->inherit_attributes(t); return newt;
  } else if(t->type_of()=="ProtoScalar") {
    ProtoScalar* oldt = S_TYPE(t);
    ProtoScalar* newt = 
      (oldt->constant? new ProtoScalar(oldt->value) : new ProtoScalar());
    newt->inherit_attributes(t); return newt;
  } else if(t->type_of()=="ProtoBoolean") {
    ProtoScalar* oldt = S_TYPE(t);
    ProtoBoolean* newt = 
      (oldt->constant? new ProtoBoolean(oldt->value) : new ProtoBoolean());
    newt->inherit_attributes(t); return newt;
  } else if(t->type_of()=="ProtoVector") {
    ProtoTuple* newt = new ProtoVector(T_TYPE(t));
    return newt; // inheritance handled in constructor
  } else if(t->type_of()=="ProtoLambda") {
    ProtoLambda* newt = new ProtoLambda(L_VAL(t));
    newt->inherit_attributes(t); return newt;
  } else if(t->type_of()=="ProtoField") {
    ProtoField* newt = new ProtoField(F_VAL(t));
    newt->inherit_attributes(t); return newt;
  }
  ierror("Don't know how to clone ProtoType "+t->type_of());
  return NULL; // dummy return: terminates on error
}

/*****************************************************************************
 *  SUPERTYPE RELATIONS                                                      *
 *****************************************************************************/

bool ProtoTuple::supertype_of(ProtoType* sub) { 
  if(sub==NULL || this==NULL) ierror("supertype_of called on NULL type.");
  if(!sub->isA(type_of())) return false; // not supertype of non-tuples
  ProtoTuple *tsub = &dynamic_cast<ProtoTuple &>(*sub);
  // I'm bounded but sub is not 
  if(!tsub->bounded && bounded) return false;
  // are elements compatible?
  int ll = types.size(), sl = tsub->types.size(), len = max(ll, sl);
  for(int i=0;i<len;i++) {
    ProtoType *mine,*subs;
    // Get the type of the putative parent
    if(i<ll) { mine = types[i]; // normal element
    } else { // beyond the apparent end of the tuple
      if(bounded) return false; // if child is longer than parent, not a sub
      else { // otherwise, use the rest type
        if(ll==0) ierror("Unbounded tuple must have a rest element.");
        mine = types[ll-1]; //rest elem
      }
    }
    // Get the type of the putative child
    if(i<sl) { subs = tsub->types[i]; // normal element
    } else { // beyond the apparent end of the tuple
      if(tsub->bounded) { 
        return (!bounded && i==len-1); // sub if end aligns w. parent "rest" elt
      } else {
        if(sl==0) ierror("Unbounded tuple must have a rest element.");
        subs = tsub->types[sl-1]; // rest elem
      }
    }
    if(!mine->supertype_of(subs)) 
      return false;
  }
  return true;
}
bool ProtoSymbol::supertype_of(ProtoType* sub) { 
  if(!sub->isA("ProtoSymbol")) return false; // not supertype of non-symbols
  ProtoSymbol *ssub = &dynamic_cast<ProtoSymbol &>(*sub);
  return !constant || value==ssub->value;
}

bool ProtoScalar::supertype_of(ProtoType* sub) { 
  if(!sub->isA(type_of())) return false; // not supertype of non-scalars
  ProtoScalar *ssub = &dynamic_cast<ProtoScalar &>(*sub);
  return !constant || value==ssub->value || (isnan(value)&&isnan(ssub->value));
}

bool ProtoLambda::supertype_of(ProtoType* sub) { 
  if(!sub->isA(type_of())) return false; // not supertype of non-lambdas
  ProtoLambda* lsub = &dynamic_cast<ProtoLambda &>(*sub);
  // generic is super of all fields
  if(!op) return true; if(!lsub->op) return false;
  return op==lsub->op; // lambdas are the same only if they have the same op
}

bool ProtoField::supertype_of(ProtoType* sub) { 
  if(!sub->isA(type_of())) return false; // not supertype of non-fields
  ProtoField *fsub = &dynamic_cast<ProtoField &>(*sub);
  // generic is super of all fields
  if(!hoodtype) return true; if(!fsub->hoodtype) return false;
  return hoodtype->supertype_of(fsub->hoodtype);
}


/*****************************************************************************
 *  LEAST COMMON SUPERTYPE                                                   *
 *****************************************************************************/

ProtoType* ProtoType::lcs(ProtoType* t1, ProtoType* t2) {
  // easy case: if they're ordered, return the super
  if(t1->supertype_of(t2)) return t1;
  if(t2->supertype_of(t1)) return t2;
  // harder case: start w. one and walk up to find second
  return t1->lcs(t2);
}

ProtoType* ProtoLocal::lcs(ProtoType* t) { 
  if(!t->isA("ProtoLocal")) return ProtoType::lcs(t);
  return new ProtoLocal(); // no possible substructure
}

// Tuple/vector LCS
// Generalize elts to minimum fixed length, then generalize rests
// e.g. T<3,4,5> + T<3,6> -> T<3,Scalar,Scalar...>
void element_lcs(ProtoTuple *a, ProtoTuple* b, ProtoTuple* out) {
  int nfixed
    = min(a->types.size() - (!a->bounded), b->types.size() - (!b->bounded));
  for(int i=0;i<nfixed;i++) 
    out->types.push_back(ProtoType::lcs(a->types[i],b->types[i]));
  // make a "rest" from remaining types, when needed
  ProtoType* rest = NULL;
  for(int j=nfixed;j<a->types.size();j++)
    { if(!rest) rest = a->types[j]; else rest=ProtoType::lcs(a->types[j],rest);}
  for(int j=nfixed;j<b->types.size();j++)
    { if(!rest) rest = b->types[j]; else rest=ProtoType::lcs(b->types[j],rest);}
  if(rest!=NULL) { out->bounded=false; out->types.push_back(rest); }
  else out->bounded=true;
}

ProtoType* ProtoTuple::lcs(ProtoType* t) {
  if(!t->isA("ProtoTuple")) return ProtoType::lcs(t);
  ProtoTuple* tt = &dynamic_cast<ProtoTuple &>(*t);
  ProtoTuple* nt = new ProtoTuple(true); element_lcs(this,tt,nt); return nt;
}

ProtoType* ProtoSymbol::lcs(ProtoType* t) {
  if(!t->isA("ProtoSymbol")) return ProtoLocal::lcs(t);
  return new ProtoSymbol(); // LCS -> not super -> different symbols
}

ProtoType* ProtoNumber::lcs(ProtoType* t) { 
  if(!t->isA("ProtoNumber")) return ProtoLocal::lcs(t);
  return new ProtoNumber(); // no possible substructure
}

ProtoType* ProtoScalar::lcs(ProtoType* t) {
  if(!t->isA("ProtoScalar")) return ProtoNumber::lcs(t);
  return new ProtoScalar(); // LCS -> not super -> different values
}

ProtoType* ProtoBoolean::lcs(ProtoType* t) {
  if(!t->isA("ProtoBoolean")) return ProtoScalar::lcs(t);
  return new ProtoBoolean(); // LCS -> not super -> one true, other false
}

ProtoType* ProtoLocalTuple::lcs(ProtoType* t) {
  if(!t->isA("ProtoLocalTuple")) { // split inheritance
    if(t->isA("ProtoTuple")) return ProtoTuple::lcs(t);
    if(t->isA("ProtoLocal")) return ProtoLocal::lcs(t);
    return ProtoType::lcs(t); // join point in inheritance
  }
  ProtoLocalTuple* tt = &dynamic_cast<ProtoLocalTuple &>(*t);
  ProtoLocalTuple* nt = new ProtoLocalTuple(true); element_lcs(this,tt,nt); return nt;
}

ProtoType* ProtoVector::lcs(ProtoType* t) {
  if(!t->isA("ProtoVector")) { // split inheritance
    if(t->isA("ProtoNumber")) return ProtoNumber::lcs(t);
    if(t->isA("ProtoLocalTuple")) return ProtoLocalTuple::lcs(t);
    return ProtoLocal::lcs(t); // join point in inheritance
  }
  ProtoVector* tv = &dynamic_cast<ProtoVector &>(*t);
  ProtoVector* nv = new ProtoVector(true); element_lcs(this,tv,nv); return nv;
}

ProtoType* ProtoLambda::lcs(ProtoType* t) {
  if(!t->isA("ProtoLambda")) return ProtoLocal::lcs(t);
  return new ProtoLambda(); // LCS -> not super -> different ops
}

ProtoType* ProtoField::lcs(ProtoType* t) {
  if(!t->isA("ProtoField")) return ProtoType::lcs(t);
  ProtoField* tf = &dynamic_cast<ProtoField &>(*t); //not super -> hoodtype != null
  ProtoType* merged = ProtoType::lcs(hoodtype,tf->hoodtype);
  ProtoLocal* lt = &dynamic_cast<ProtoLocal &>(*merged);
  return new ProtoField(lt);
}

/*****************************************************************************
 *  GREATEST COMMON SUBTYPE                                                  *
 *****************************************************************************/

ProtoType* ProtoType::gcs(ProtoType* t1, ProtoType* t2) {
  // easy case: if they're ordered, return the super
  if(t1->supertype_of(t2)) return t2;
  if(t2->supertype_of(t1)) return t1;
  // harder case: start w. one and walk down to find second
  return t1->gcs(t2);
}

ProtoType* ProtoType::gcs(ProtoType* t) {
  ierror("GCS dispatch failed for "+this->to_str()+" and "+t->to_str());
  return NULL; // dummy return: terminates on error
}
ProtoType* ProtoLocal::gcs(ProtoType* t) { 
//   if(t->isA("ProtoTuple")) {
//     return t->gcs(this);
//   }
  return NULL;
}
ProtoType* ProtoSymbol::gcs(ProtoType* t) { return NULL; }
ProtoType* ProtoScalar::gcs(ProtoType* t) { return NULL; } // covers boolean

bool element_gcs(ProtoTuple *a, ProtoTuple* b, ProtoTuple* out) {
  out->bounded = a->bounded || b->bounded;
  int afix=a->types.size()-(!a->bounded), bfix=b->types.size()-(!b->bounded);
  int nfixed = min(afix, bfix);
  if((a->bounded && afix<bfix) || (b->bounded && bfix<afix)) return false;
  // start by handling the fixed portion
  for(int i=0;i<nfixed;i++) {
    ProtoType* sub = ProtoType::gcs(a->types[i],b->types[i]);
    if(sub) out->add(sub); else return false;
  }
  // next, handle any excess rest portion
  ProtoTuple* longer = (afix>bfix) ? a : b;
  ProtoType* ctrest = (afix>bfix) ? b->types[bfix] : a->types[afix];
  for(int i=nfixed;i<longer->types.size()-(!longer->bounded);i++) {
    ProtoType* sub = ProtoType::gcs(longer->types[i],ctrest);
    if(sub) out->add(sub); else return false;
  }
  // last, handle the rest type
  if(!out->bounded) {
    ProtoType* sub = ProtoType::gcs(a->types[afix],b->types[bfix]);
    if(sub) out->add(sub); else return false;
  }
  return true;
}

ProtoType* ProtoTuple::gcs(ProtoType* t) { // covers locals, vectors, localtuples too
  if(t->isA("ProtoTuple")) {
    ProtoTuple* tt = &dynamic_cast<ProtoTuple &>(*t);
    ProtoTuple* newt = new ProtoTuple(false);
    if(!element_gcs(this,tt,newt)) { delete newt; return NULL; }
    return specialize_tuple_class(newt); // map to vector/local as needed
  }
  if(t->type_of()=="ProtoNumber") {
    // check if all elements are scalars... if so, result is a vector
    ProtoVector *newv = new ProtoVector(bounded);
    ProtoScalar *constraint = new ProtoScalar(); bool ctused=false;
    for(int i=0;i<types.size();i++) {
      ProtoType* sub = ProtoType::gcs(types[i],constraint);
      if(sub==constraint) ctused=true; // don't delete if needed
      if(sub) newv->add(sub); else {delete newv;delete constraint; return NULL;}
    }
    if(!ctused) delete constraint;
    return newv;
  }
  // Generic local --> convert to localtuple if possible
  if(t->isA("ProtoLocal")) {
    ProtoLocalTuple* newt = new ProtoLocalTuple(bounded);
    ProtoLocal *constraint = new ProtoLocal(); bool ctused=false;
    for(int i=0;i<types.size();i++) {
      ProtoType* sub = ProtoType::gcs(types[i],constraint);
      if(sub==constraint) ctused=true; // don't delete if needed
      if(sub) newt->add(sub); else {delete newt;delete constraint; return NULL;}
    }
    if(!ctused) delete constraint;
    return newt;
  }
  return NULL;
}
ProtoType* ProtoNumber::gcs(ProtoType* t) {
  if(t->isA("ProtoTuple")) return t->gcs(this); // write it once, thank you
  return NULL; // otherwise, since not sub/super relation, is conflict
}
ProtoType* ProtoLambda::gcs(ProtoType* t) {
  if(!t->isA("ProtoLambda")) return NULL;
  return NULL; // since not sub/super relation, is conflict
}
ProtoType* ProtoField::gcs(ProtoType* t) {
  if(!t->isA("ProtoField")) return NULL;
  ProtoField* tf = &dynamic_cast<ProtoField &>(*t); //not super -> hoodtype != null
  ProtoType* hood = ProtoType::gcs(tf->hoodtype,hoodtype);
  if (hood==NULL) {
    return NULL;
  } else {
    ProtoLocal* lhood = &dynamic_cast<ProtoLocal &>(*hood);
    return new ProtoField(lhood);
  }
}

/*****************************************************************************
 *  MISCELLANEOUS OPS                                                        *
 *****************************************************************************/

ProtoTuple* ProtoTuple::specialize_tuple_class(ProtoTuple* t) {
  bool all_local = true, all_scalar = true;
  // create all the subelements
  vector<ProtoType*> copied_types;
  for(int i=0;i<t->types.size();i++) {
    if(t->types[i]->isA("ProtoTuple")) {
      all_scalar = false;
      ProtoTuple* ti = &dynamic_cast<ProtoTuple &>(*t->types[i]);
      ProtoTuple* subtuple = specialize_tuple_class(ti);
      if(!subtuple->isA("ProtoLocalTuple")) all_local = false;
      copied_types.push_back(subtuple);
    } else {
      if(!t->types[i]->isA("ProtoLocal")) {
        all_local = false;
        all_scalar = false;
      }
      copied_types.push_back(t->types[i]);
    }
  }
  // make a tuple of the correct type, and add all the subelements
  ProtoTuple* newt;
  if(all_scalar) newt = new ProtoVector(t->bounded);
  else if(all_local) newt = new ProtoLocalTuple(t->bounded);
  else newt = new ProtoTuple(t->bounded);
  for(int i=0;i<copied_types.size();i++)
    newt->add(copied_types[i]);
  return newt;
}

/*****************************************************************************
 *  PRINTING                                                                 *
 *****************************************************************************/
void ProtoTuple::print(ostream* out) { 
  //*out<<"[ID="<<elmt_id<<"]";
  *out << "<"; if(bounded) *out << types.size() << "-"; *out << "Tuple";
  for(int i=0;i<types.size();i++) { if(i) *out<<","; types[i]->print(out); }
  if(!bounded) *out<<"...";
  *out << ">";
}
void ProtoLocalTuple::print(ostream* out) { 
  *out << "<"; if(bounded) *out << types.size() << "-"; *out << "LocalTuple";
  for(int i=0;i<types.size();i++) { if(i) *out<<","; types[i]->print(out); }
  if(!bounded) *out<<"...";
  *out << ">";
}
void ProtoBoolean::print(ostream* out) {
  *out << "<Boolean"; 
  if(constant) { *out << " " << (value?"true":"false"); } 
  *out << ">";
}
void ProtoVector::print(ostream* out) {
  *out << "<"; if(bounded) *out << types.size() << "-"; *out << "Vector";
  for(int i=0;i<types.size();i++) { if(i) *out<<","; types[i]->print(out); }
  if(!bounded) *out<<"...";
  *out << ">";
}
void ProtoLambda::print(ostream* out)
{ *out << "<Lambda"; if(op) { *out << " "; op->print(out); } *out << ">"; }



/*****************************************************************************
 *  INTERNAL TESTING                                                         *
 *****************************************************************************/

string typeorder_test(ProtoType* ta,ProtoType* tb) {
  return ta->to_str()+" > "+tb->to_str()+": " + bool2str(ta->supertype_of(tb)) +
    ", reverse: " + bool2str(tb->supertype_of(ta));
}

string lcs_test(ProtoType* ta,ProtoType* tb) {
  ProtoType *tlcs = ProtoType::lcs(ta,tb), *tr = ProtoType::lcs(tb,ta);
  bool inv_ok = tlcs->supertype_of(tr) && tr->supertype_of(tlcs); // must be =
  return "LCS("+ta->to_str()+","+tb->to_str()+") = " + tlcs->to_str() +
    "; inverse match = "+bool2str(inv_ok);
}

string gcs_test(ProtoType* ta,ProtoType* tb) {
  ProtoType *tgcs = ProtoType::gcs(ta,tb), *tr = ProtoType::gcs(tb,ta);
  bool inv_ok = false; // is reverse same as forward?
  if(tgcs && tr) { inv_ok = tgcs->supertype_of(tr) && tr->supertype_of(tgcs);
  } else { inv_ok = tgcs==tr; }
  string gstr = (tgcs==NULL) ? "NULL" : tgcs->to_str();
  return "GCS("+ta->to_str()+","+tb->to_str()+") = " + gstr + 
    "; inverse match = "+bool2str(inv_ok);
}

void type_system_tests() {
  ProtoType top; ProtoLocal local; ProtoTuple tuple; 
  ProtoLocalTuple ltuple; ProtoSymbol symbol;
  ProtoNumber number; ProtoScalar scalar; ProtoBoolean boolean;
  ProtoVector vector; ProtoLambda lambda; ProtoField field;
  *cpout << "Testing type relations:\n";
  // begin with generic type comparison
  *cpout << "Generic types:\n";
  *cpout << typeorder_test(&top,&top) << endl;
  *cpout << typeorder_test(&top,&local) << endl;
  *cpout << typeorder_test(&top,&field) << endl;
  *cpout << typeorder_test(&top,&lambda) << endl;
  *cpout << typeorder_test(&top,&scalar) << endl;
  *cpout << typeorder_test(&scalar,&number) << endl;
  *cpout << typeorder_test(&scalar,&field) << endl;
  *cpout << typeorder_test(&vector,&tuple) << endl;
  *cpout << typeorder_test(&scalar,&boolean) << endl;
  *cpout << typeorder_test(&scalar,&symbol) << endl;
  *cpout << typeorder_test(&top,&vector) << endl;
  *cpout << typeorder_test(&vector,&number) << endl;
  *cpout << typeorder_test(&vector,&boolean) << endl;
  *cpout << typeorder_test(&symbol,&boolean) << endl;
  *cpout << typeorder_test(&symbol,&local) << endl;
  *cpout << typeorder_test(&field,&local) << endl;
  *cpout << typeorder_test(&lambda,&local) << endl;
  *cpout << typeorder_test(&tuple,&local) << endl;
  *cpout << typeorder_test(&ltuple,&local) << endl;
  // next some literals
  *cpout << "Literals:\n";
  ProtoScalar l3(3), l4(4), l5(5), l1(1), l0(0);
  *cpout << typeorder_test(&l3,&local) << endl;
  *cpout << typeorder_test(&l4,&l5) << endl;
  *cpout << typeorder_test(&l4,&boolean) << endl;
  *cpout << typeorder_test(&scalar,&l3) << endl;
  ProtoBoolean bt(true), bf(false);
  *cpout << typeorder_test(&bt,&boolean) << endl;
  *cpout << typeorder_test(&bf,&symbol) << endl;
  *cpout << typeorder_test(&boolean,&bf) << endl;
  *cpout << typeorder_test(&bf,&bt) << endl;
  *cpout << typeorder_test(&bf,&l1) << endl; // bool/scalar comparisons
  *cpout << typeorder_test(&bf,&l0) << endl;
  *cpout << typeorder_test(&bt,&l1) << endl;
  *cpout << typeorder_test(&bt,&l0) << endl;
  ProtoSymbol sf("foo"), sb("bar");
  *cpout << typeorder_test(&sf,&number) << endl;
  *cpout << typeorder_test(&sb,&symbol) << endl;
  *cpout << typeorder_test(&top,&sf) << endl;
  *cpout << typeorder_test(&sf,&sb) << endl;
  // now compound types
  *cpout << "Tuples:\n";
  ProtoLocalTuple t2(true); t2.add(&l3); t2.add(&symbol);
  ProtoTuple t2u(false); t2u.add(&l3); t2u.add(&top); t2u.add(&field);
  *cpout << typeorder_test(&t2,&t2u) << endl;
  *cpout << typeorder_test(&t2,&tuple) << endl;
  *cpout << typeorder_test(&t2u,&tuple) << endl;
  ProtoLocalTuple t34s(false); t34s.add(&l3); t34s.add(&l4); t34s.add(&scalar);
  ProtoLocalTuple t34ss(false); t34ss.add(&l3); t34ss.add(&l4); t34ss.add(&scalar); t34ss.add(&scalar);
  ProtoLocalTuple t345s(false); t345s.add(&l3); t345s.add(&l4); t345s.add(&l5); t345s.add(&scalar);
  ProtoLocalTuple t34(true); t34.add(&l3); t34.add(&l4);
  ProtoLocalTuple t345(true); t345.add(&l3); t345.add(&l4); t345.add(&l5);
  ProtoTuple t3x5(true); t3x5.add(&l3); t3x5.add(&top); t3x5.add(&l5);
  ProtoLocalTuple t3l5(true); t3l5.add(&l3); t3l5.add(&local); t3l5.add(&l5);
  ProtoLocalTuple t3t5(true); t3t5.add(&l3); t3t5.add(&t345); t3t5.add(&l5);
  *cpout << typeorder_test(&tuple,&t34s) << endl;
  *cpout << typeorder_test(&t345s,&t34s) << endl;
  *cpout << typeorder_test(&t345,&t34s) << endl;
  *cpout << typeorder_test(&t3x5,&t34s) << endl;
  *cpout << typeorder_test(&t34,&t34s) << endl;
  *cpout << typeorder_test(&t3t5,&t34s) << endl;
  *cpout << typeorder_test(&t345,&t345s) << endl;
  *cpout << typeorder_test(&t3x5,&t345s) << endl;
  *cpout << typeorder_test(&t34,&t345s) << endl;
  *cpout << typeorder_test(&t3x5,&t345) << endl;
  *cpout << typeorder_test(&t3x5,&t34) << endl;
  *cpout << typeorder_test(&t3x5,&t3t5) << endl;
  *cpout << typeorder_test(&t34ss,&t34s) << endl;
  *cpout << typeorder_test(&t34ss,&t345s) << endl;
  *cpout << typeorder_test(&t34ss,&t345) << endl;
  *cpout << typeorder_test(&t34ss,&t34) << endl;
  *cpout << "Vectors:\n";
  ProtoVector v345(true); v345.add(&l3); v345.add(&l4); v345.add(&l5);
  ProtoVector v34s(false); v34s.add(&l3); v34s.add(&l4); v34s.add(&scalar);
  *cpout << typeorder_test(&v345,&t345) << endl;
  *cpout << typeorder_test(&v345,&t34) << endl;
  *cpout << typeorder_test(&v345,&t345s) << endl;
  *cpout << typeorder_test(&v345,&t3x5) << endl;
  *cpout << typeorder_test(&v34s,&v345) << endl;
  *cpout << typeorder_test(&v34s,&t345) << endl;
  *cpout << typeorder_test(&v34s,&t34s) << endl;
  *cpout << "Fields:\n";
  ProtoField f3(&l3), f3t5(&t3t5), f3l5(&t3l5);
  *cpout << typeorder_test(&f3,&field) << endl;
  *cpout << typeorder_test(&f3,&f3l5) << endl;
  *cpout << typeorder_test(&f3t5,&field) << endl;
  *cpout << typeorder_test(&f3t5,&f3l5) << endl;
  // LCS relations
  *cpout << "Testing least-common-supertype:\n";
  // first some ordered pairs
  *cpout << "LCS ordered pairs:\n";
  *cpout << lcs_test(&top,&f3) << endl; // = any
  *cpout << lcs_test(&l3,&local) << endl; // = local
  *cpout << lcs_test(&number,&boolean) << endl; // = number
  // now cross-class generalizations
  *cpout << "LCS cross-class:\n";
  *cpout << lcs_test(&f3t5,&t3t5) << endl; // = any
  *cpout << lcs_test(&bf,&t3t5) << endl; // = local
  *cpout << lcs_test(&boolean,&vector) << endl; // = number
  *cpout << lcs_test(&bt,&l5) << endl; // = scalar
  *cpout << lcs_test(&vector,&t3t5) << endl; // = tuple<Local...>
  *cpout << lcs_test(&boolean,&lambda) << endl; // = local
  *cpout << lcs_test(&vector,&field) << endl; // = any
  // finally, in-class generalizations
  *cpout << "LCS in-class generalization:\n";
  ProtoLocalTuple t31(true); t31.add(&l3); t31.add(&l1);
  ProtoVector v145(true); v145.add(&l1); v145.add(&l4); v145.add(&l5);
  *cpout << lcs_test(&bf,&bt) << endl; // = boolean
  *cpout << lcs_test(&bt,&l1) << endl; // = <Scalar 1>
  *cpout << lcs_test(&l1,&l3) << endl; // = scalar
  *cpout << lcs_test(&t31,&t345) << endl; // = T<3,Sc,5...>
  *cpout << lcs_test(&t345s,&t3x5) << endl; // = T<3,Any,5,Sc...>
  *cpout << lcs_test(&t3t5,&t345) << endl; // = T<3,Local,5>
  *cpout << lcs_test(&t34ss,&t34) << endl; // = T<3,4,Sc...>
  *cpout << lcs_test(&v34s,&v145) << endl; // = V<Sc,4,Sc,Sc...>
  *cpout << lcs_test(&v145,&l3) << endl; // = number
  *cpout << lcs_test(&v145,&t345) << endl; // = T<Sc,4,5>
  *cpout << lcs_test(&sf,&sb) << endl; // = symbol
  *cpout << lcs_test(&f3,&f3t5) << endl; // = F<local>
  // GCS relations
  *cpout << "Testing greatest-common-subtype:" << endl;
  *cpout << "GCS ordered pairs:\n";
  *cpout << gcs_test(&top,&f3) << endl; // = <Field <Sc 3>>
  *cpout << gcs_test(&l3,&local) << endl; // = <Scalar 3>
  *cpout << gcs_test(&number,&boolean) << endl; // = boolean
  // now cross-class generalizations
  *cpout << "GCS cross-class:\n";
  *cpout << gcs_test(&f3t5,&t3t5) << endl; // = null
  *cpout << gcs_test(&vector,&t3t5) << endl; // = null
  *cpout << gcs_test(&boolean,&lambda) << endl; // = null
  *cpout << gcs_test(&vector,&field) << endl; // = null
  *cpout << gcs_test(&number,&tuple) << endl; // = V<Sc...>
  *cpout << gcs_test(&t3t5,&number) << endl; // = null
  *cpout << gcs_test(&t3x5,&number) << endl; // = V<3S5>
  // finally, in-class specializations
  *cpout << "GCS in-class specialization:" << endl;
  ProtoTuple t34x(false); t34x.add(&l3); t34x.add(&l4); t34x.add(&top);
  ProtoTuple t3x5s(false); t3x5s.add(&l3); t3x5s.add(&top); t3x5s.add(&l5); t3x5s.add(&scalar);
  ProtoTuple t3x5f(false); t3x5f.add(&l3); t3x5f.add(&top); t3x5f.add(&l5); t3x5f.add(&field);
  ProtoVector vs45(true); vs45.add(&scalar); vs45.add(&l4); vs45.add(&l5);
  *cpout << gcs_test(&bf,&bt) << endl; // = NULL
  *cpout << gcs_test(&bt,&l1) << endl; // = <Boolean true>
  *cpout << gcs_test(&l1,&l3) << endl; // = NULL
  *cpout << gcs_test(&t34,&t345) << endl; // = null
  *cpout << gcs_test(&t34,&t34s) << endl; // = T<3,4>
  *cpout << gcs_test(&t34,&t34ss) << endl; // = null
  *cpout << gcs_test(&t345s,&t3x5) << endl; // = T<3,4,5>
  *cpout << gcs_test(&t34x,&t34ss) << endl; // = T<3,4,Sc,Sc...>
  *cpout << gcs_test(&t3x5,&t34x) << endl; // = T<3,4,5>
  *cpout << gcs_test(&t3x5s,&t34x) << endl; // = T<3,4,5,Sc...>
  *cpout << gcs_test(&t3x5s,&t3x5f) << endl; // = null
  *cpout << gcs_test(&t3x5f,&t34x) << endl; // = T<3,4,5,Field...>
  *cpout << gcs_test(&v34s,&t3x5) << endl; // = V<3,4,5>
  *cpout << gcs_test(&t3x5,&v34s) << endl; // = V<3,4,5>
  *cpout << gcs_test(&v34s,&vs45) << endl; // = V<3,4,5>
  *cpout << gcs_test(&sf,&sb) << endl; // = null
  *cpout << gcs_test(&f3l5,&f3t5) << endl; // = F<T<3,T<3,4,5>,5>>
  *cpout << gcs_test(&tuple,&t34) << endl; // = T<3,4>
  *cpout << gcs_test(&tuple,&local) << endl; // = Local
}
