/* Abstract genetic regulatory network representation
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#ifndef __GRN__
#define __GRN__

#include <proto/compiler.h>
#include <proto/nicenames.h>

using namespace std;

namespace grn {

// Defaults for draft standard
//#define DEFAULT_STRENGTH 101
//#define DEFAULT_LOW_RATE 0.002
//#define DEFAULT_HIGH_RATE 0.193
//#define DEFAULT_HILL 3.0
//#define DEFAULT_DISSOCIATION 190
//#define DEFAULT_HALFLIFE 1800

// Ting/Braun numbers:
#define DEFAULT_STRENGTH 500
#define DEFAULT_LOW_RATE 0.0002 // 0.002
#define DEFAULT_HIGH_RATE 0.333 // 0.193
#define DEFAULT_HILL 3.0
#define DEFAULT_DISSOCIATION 25
#define DEFAULT_HALFLIFE 1800
#define DEFAULT_LAYERTIME 10000 // Halflife * ~5

string scalar_to_str(ProtoScalar* s);

class FunctionalUnit; class ExpressionRegulation; 
class DNAComponent : public CompilationElement {
 public:
  set<ExpressionRegulation*,CompilationElement_cmp> regulators;
  FunctionalUnit* container;
  
  virtual bool isA(string c){ return (c=="DNAComponent")?true:CompilationElement::isA(c); }
  virtual ~DNAComponent();
  virtual DNAComponent* clone() = 0; // make a duplicate of all except the container
  DNAComponent() { container=NULL; }
  void print_regulators(ostream* out);
};

/* 5'UTR (formarly RBS) is treated as part of the coding sequence
class RBS : public DNAComponent { // ribosome binding site
public:
  void print(ostream* out) { *out<<"RBS"; }
  virtual bool isA(string c){ return (c=="RBS")?true:DNAComponent::isA(c); }
};
*/

// Note: the terminator class can refer either to the termination
// structures in prokaryotes or to the poly-A sequence in Eukaryotes
class Terminator : public DNAComponent {
public:
  void print(ostream* out) { *out<<"T"; }
  virtual bool isA(string c){ return (c=="Terminator")?true:DNAComponent::isA(c); }
  virtual ~Terminator() {}
  DNAComponent* clone() { return new Terminator(); }
};

class CodingSequence; class RegulatoryReaction;

class Chemical : public CompilationElement {
 public:
  string name;
  double halflife; // in seconds; range is ~ 5 min - 1 day
  float hill_coefficient; // ~1-4
  
  set<CodingSequence*,CompilationElement_cmp> producers;
  set<ExpressionRegulation*,CompilationElement_cmp> consumers;
  set<RegulatoryReaction*,CompilationElement_cmp> regulatedBy;
  set<RegulatoryReaction*,CompilationElement_cmp> regulatorFor;
  
  Chemical(float hill=DEFAULT_HILL, float halflife=DEFAULT_HALFLIFE) 
    { name=this->nicename(); hill_coefficient=hill; this->halflife=halflife; }
  Chemical(string name,float hill=DEFAULT_HILL,float halflife=DEFAULT_HALFLIFE) 
    { this->name=name; hill_coefficient=hill; this->halflife=halflife; }
  virtual ~Chemical() {
    if(!producers.empty() || !consumers.empty() || 
       !regulatedBy.empty() || !regulatorFor.empty())
      ierror("Tried to delete a chemical with live relations");
  }
  void print(ostream *out) { 
    *out<<name<<" ["; 
    if(hill_coefficient!=DEFAULT_HILL) *out<<" H="<<hill_coefficient;
    if(this->attributes["type"]) this->attributes["type"]->print(out);
    *out<<halflife<<"]"; 
  }
  virtual bool isA(string c){ return (c=="Chemical")?true:CompilationElement::isA(c); }
};

// coding sequence for any chemical product, e.g. protein, gRNA, miRNA
class CodingSequence : public DNAComponent {
public:
  Chemical* product;
  CodingSequence(Chemical* product)
    { this->product=product; product->producers.insert(this); }
  virtual ~CodingSequence() { 
    product->producers.erase(this);
  }
  void print(ostream* out) { 
    *out<<"["<<product->name;
    if(product->attributes["type"]) product->attributes["type"]->print(out);
    print_regulators(out);
    *out<<"]";
  }
  DNAComponent* clone(); 
  virtual bool isA(string c){ return (c=="CodingSequence")?true:DNAComponent::isA(c); }
};
// will also eventually want domains that can modify product behavior
// and to distinguish a regulation on translation and products from regulation
// on promoters

class Promoter : public DNAComponent {
 public:
  // rate is 0.01 - 10 mol/sec; Promoter+RBS amplification
  ProtoScalar* rate; // could be a number or high (true) or low (false)
  
  Promoter(ProtoScalar* r=new ProtoBoolean(false)) { rate=r; }
  virtual ~Promoter() {}
  DNAComponent* clone();
  void print(ostream* out) { 
    *out<<"[Promoter ["<<scalar_to_str(rate)<<"]";
    print_regulators(out);
    *out<<"]";
  }
  virtual bool isA(string c){ return (c=="Promoter")?true:DNAComponent::isA(c); }
};

// ExpressionRegulation represents transcriptional or translational 
// regulation of a functional unit's products.  This can be tied to
// either a promoter or a CDS
class ExpressionRegulation : public CompilationElement {
  reflection_sub(ExpressionRegulation, CompilationElement);

 public:
  Chemical* signal;
  DNAComponent* target;
  bool repressor;
  float strength; // k-fold amplification ~ 2 - 1000
  float dissociation; // dissociation constant ~10 - hundreds

 public:
  ExpressionRegulation(Chemical* s, DNAComponent* t, bool r=false, float str=DEFAULT_STRENGTH, float dc=DEFAULT_DISSOCIATION) {
    signal=s; target = t; repressor=r; strength=str; dissociation=dc;
    s->consumers.insert(this);
    if(t!=NULL) { t->regulators.insert(this); }
  }
  virtual ~ExpressionRegulation() {
    if(signal!=NULL) signal->consumers.erase(this);
    if(target!=NULL) target->regulators.erase(this);
  }
  ExpressionRegulation* clone(DNAComponent* new_target) {
    return new ExpressionRegulation(signal,new_target,repressor,strength,dissociation);
  }
  // is everything but the target equal?
  bool same_regulation(ExpressionRegulation* er) {
    if(signal!=er->signal) return false;
    if(repressor!=er->repressor) return false;
    if(strength!=er->strength) return false;
    if(dissociation!=er->dissociation) return false;
    return true;
  }
  void print(ostream* out) { 
    *out << "[" << signal->name << (repressor?" -":" +");
    if(strength!=DEFAULT_STRENGTH) *out<<" "<<strength<<"-fold";
    if(dissociation!=DEFAULT_DISSOCIATION) *out<<" dc="<<dissociation<<"";
    if(signal->attributes["type"]) signal->attributes["type"]->print(out);    
    *out<<"]"; 
  }
};

// This will give only a class of reactions for small-molecule regulation of
// protein (or other macromolecule) activity
class RegulatoryReaction : public CompilationElement {
 public:
  Chemical* regulator;
  Chemical* substrate;
  bool repressor;
  float dissociation;
  float alpha; // base-rate parameter
  RegulatoryReaction(Chemical* regulator, Chemical* substrate, bool r) {
    this->regulator = regulator; this->substrate=substrate; repressor=r;
    regulator->regulatorFor.insert(this); substrate->regulatedBy.insert(this);
  }
  virtual ~RegulatoryReaction() 
    { regulator->regulatorFor.erase(this); substrate->regulatedBy.erase(this); }
  void print(ostream* out) {
    *out << "Reaction: " << regulator->name << 
      (repressor?" represses ":" activates ") << substrate->name;
  }
  virtual bool isA(string c){ return (c=="RegulatoryReaction")?true:CompilationElement::isA(c); }
};

/* structure: promoter, 1-2 RRs (not both inducers), RBS, PCRs, Stop */
class FunctionalUnit : public DNAComponent {
public:
  vector<DNAComponent*> sequence;
  void add(DNAComponent* b) { b->container=this; sequence.push_back(b); }
  void remove(DNAComponent* b) { // remove first instance of b in FU
    bool found = false;
    for(int i=0;i<sequence.size();i++) {
      if(found) sequence[i-1] = sequence[i]; // shuffle back
      if(sequence[i]==b) found=true;
    }
  }
  void print(ostream* out) { 
    for(int i=0;i<sequence.size();i++) 
      { if(i) *out<<"--"; sequence[i]->print(out); }
  }
  virtual ~FunctionalUnit() 
    { for(int i=0;i<sequence.size();i++) delete sequence[i]; }
  DNAComponent* clone() { 
    FunctionalUnit* fu = new FunctionalUnit();
    for(int i=0;i<sequence.size();i++) fu->add(sequence[i]->clone());
    return fu;
  }
  virtual bool isA(string c){return(c=="FunctionalUnit")?true:DNAComponent::isA(c);}
};

class GRN { public: reflection_base(GRN);
 public:
  set<DNAComponent*,CompilationElement_cmp> dnacomponents; // generally a set of functional units
  set<RegulatoryReaction*,CompilationElement_cmp> reactions;
  map<string,Chemical*> chemicals;
  // need to add lists of Chemicals
  // also backpointers to allow network crawling

  void delete_functional_unit(FunctionalUnit* fu);
  void delete_chemical(Chemical* c);
  void replace_inputs(FunctionalUnit* target, FunctionalUnit* src, DNAComponent* src_cds=NULL,int verbosity=0);
  void invert_regulation(FunctionalUnit* target,int verbosity=0);
  void invert_regulation(ExpressionRegulation* er,int verbosity=0);
  void print(ostream* out);
};

struct ProtoTypeAttribute : Attribute {
  ProtoType* type;
  
  ProtoTypeAttribute(ProtoType* type) { this->type=type; }
  
  void print(ostream *out=cpout) { 
    *out << " type="; 
    type->print(out); 
    //*out << type->type_of()<<" ";
  }
};

}

#endif // __GRN__
