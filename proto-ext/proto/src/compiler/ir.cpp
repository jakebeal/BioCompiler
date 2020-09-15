/* Intermediate representation for Proto compiler
Copyright (C) 2009-2010, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "ir.h"

#include <algorithm>
#include <string>

#include "config.h"

#include "compiler.h"
#include "nicenames.h"

using namespace std;

extern SE_Symbol *make_gensym(const string &root); // from interpreter

/*****************************************************************************
 *  Amorphous Mediums                                                        *
 *****************************************************************************/

AM::AM(CE* src, AM* parent, Field* f) { 
  inherit_attributes(src);
  this->parent=parent; selector=f; f->selectors.insert(this);
  parent->children.insert(this); container=parent->container; bodyOf=NULL;
  container->spaces.insert(this);
}

AM::AM(CE* src, DFG* root, CompoundOp* bodyOf) { 
  inherit_attributes(src);
  parent=NULL; selector = NULL; container=root;
  this->bodyOf = bodyOf; if(bodyOf!=NULL) bodyOf->body=this; 
  if(root!=NULL) root->spaces.insert(this);
}

int AM::size() // number of fields in this AM and its children
{ int s = fields.size(); for_set(AM*,children,i) s+= (*i)->size(); return s; }
void AM::all_spaces(AMset *out) {
  out->insert(this);
  for_set(AM*,children,i) (*i)->all_spaces(out);
}
void AM::all_fields(Fset *out) {
  for_set(Field*,fields,i) out->insert(*i);
  for_set(AM*,children,i) (*i)->all_fields(out);
}
void AM::all_ois(OIset *out) {
  for_set(Field*,fields,i) out->insert((*i)->producer);
  for_set(AM*,children,i) (*i)->all_ois(out);
}

bool AM::child_of(AM* am) { 
  if(am==parent) return true;
  if(parent) return parent->child_of(am); else return false;
}

void AM::print(ostream* out) {
  *out << "[Medium: " << this->nicename() << " = ";
  if(parent) { *out << parent->nicename() << " | ";
    if(selector) { *out << selector->nicename(); }
  } else { *out << "root"; }
  *out << "]";
}

/*****************************************************************************
 *  Fields                                                                   *
 *****************************************************************************/

Field::Field(CE* src, AM *domain,ProtoType *range,OperatorInstance *oi) { 
  if(domain==NULL||range==NULL||oi==NULL)
    ierror("Field created with null parameter");
  inherit_attributes(src);
  container=domain->container; container->edges.insert(this);
  this->domain=domain;  domain->fields.insert(this);
  this->range=range; producer=oi;
}

void Field::print(ostream* out)
{*out<<this->nicename()<<": "<<domain->nicename()<<" --> "; range->print(out);}

void Field::use(OI* oi,int i) { consumers.insert(make_pair(oi,i)); }
void Field::unuse(OI* oi,int i) { consumers.erase(make_pair(oi,i)); }
bool Field::is_output() {
  return container->output==this ||
    (domain->bodyOf && domain->bodyOf->output==this);
}

/*****************************************************************************
 *  Operators                                                                *
 *****************************************************************************/

Signature::Signature(CE* src, ProtoType* output) {
  inherit_attributes(src);
  this->output = (output!=NULL) ? output : new ProtoSymbol("SCRATCH~TYPE");
  rest_input=NULL; 
}

Signature::Signature(Signature* src) {
  rest_input = src->rest_input; output = src->output;
  for(int i=0;i<src->required_inputs.size();i++)
    required_inputs.push_back(src->required_inputs[i]);
  for(int i=0;i<src->optional_inputs.size();i++)
    optional_inputs.push_back(src->optional_inputs[i]);
  names = src->names; // copy names over
}

void Signature::print(ostream* out) {
  *out << "[Signature: ";
  bool first=true;
  for(int i=0;i<required_inputs.size();i++)
    { if(first) first=false; else *out<<" "; required_inputs[i]->print(out); }
  if(optional_inputs.size()) 
    { if(first) first=false; else *out<<" "; *out<<"&optional"; }
  for(int i=0;i<optional_inputs.size();i++) 
    { *out << " "; optional_inputs[i]->print(out);}
  if(rest_input) {
    if(first) first=false; else *out<<" "; *out<<"&rest ";
    rest_input->print(out);
  }
  *out << " --> ";
  output->print(out);
  *out << "]";
}

string Signature::num_arg_str() {
  int low=required_inputs.size(), high=n_fixed();
  if(rest_input) { return "at least "+i2s(low); 
  } else if(high!=low) { return "from "+i2s(low)+" to "+i2s(high);
  } else { return "exactly "+i2s(low);
  }
}

ProtoType* Signature::nth_type(int n) {
  if(n < required_inputs.size()) return required_inputs[n];
  n-= required_inputs.size();
  if(n < optional_inputs.size()) return optional_inputs[n];
  n-= optional_inputs.size();
  if(rest_input) return rest_input;
  return type_err(this,"Can't find type"+i2s(n)+" in "+to_str());
}

void Signature::set_nth_type(int n,ProtoType* val) {
  if(n < required_inputs.size()) {
     required_inputs[n] = val;
     return;
  }
  n-= required_inputs.size();
  if(n < optional_inputs.size()) {
     optional_inputs[n] = val;
     return;
  }
  n-= optional_inputs.size();
  if(rest_input) {
     rest_input = val;
  }
}

int Signature::parameter_id(SE_Symbol* p, bool err) {
  if(!names.count(p->name)) {
    if(err) compile_error(p,"No parameter named "+ce2s(p)+" in "+ce2s(this));
    return -2;
  }
  if(names[p->name]>=-1) return names[p->name];
  ierror("Bad parameter #:"+i2s(names[p->name])+" in "+ce2s(this));
  return 0; // dummy return: terminates on error
}

ProtoType* Signature::parameter_type(SE_Symbol* p, bool err) {
  if(!names.count(p->name)) {
    if(err) return type_err(p,"No parameter named "+ce2s(p)+" in "+ce2s(this));
    else return NULL;
  }
  if(names[p->name]==-1) return output;
  if(names[p->name]>=0) return nth_type(names[p->name]);
  ierror("Bad parameter #:"+i2s(names[p->name])+" in "+ce2s(this));
  return NULL; // dummy return: terminates on error
}

bool Signature::legal_length(int n) { 
  if(n<required_inputs.size()) return false;
  if(rest_input==NULL && n>(required_inputs.size()+optional_inputs.size()))
    return false;
  return true;
}

Literal::Literal(CE* src,ProtoType *v) : Operator(src) { 
  if(!v->isLiteral()) ierror("Tried to make Literal from "+v->to_str());
  signature = new Signature(src,v); value = v;
}

Primitive::Primitive(CE* src) : Operator(src) {}
Primitive::Primitive(CE* src, string name, Signature* sig) : Operator(src)
{ this->name=name; signature=sig; }


int CompoundOp::lambda_count=0;

CompoundOp::CompoundOp(CE* src, DFG* container, string n) : Operator(src) {
  name = (n!="") ? n : make_gensym("lambda")->name;
  signature = &dynamic_cast<Signature &>(*dummy("Signature", src));
  body = new AM(src, container, this);
}

CompoundOp::CompoundOp(CompoundOp* src) : Operator(src) {
  name = make_gensym(src->name)->name;
  signature=new Signature(src->signature);
  // Tracking sets:
  AMset spaces; src->body->all_spaces(&spaces);
  OIset ois; src->body->all_ois(&ois);
  CEmap(AM*,AM*) amap; CEmap(OI*,OI*) omap; CEmap(Field*,Field*) fmap; 
  // Copy over contents:
  for_set(AM*,spaces,ai) { // copy AMs
    amap[*ai] = new AM(*ai,(*ai)->container);
    for_set(Field*,(*ai)->fields,fi) { // copy ops & fields
      OperatorInstance *oi = (*fi)->producer;
      omap[oi] = new OperatorInstance(oi,oi->op,amap[*ai]);
      fmap[*fi] = omap[oi]->output; // Note: range intentionally *not* copied
    }
  }
  // next, hook up all edges
  for_map(AM*,AM*,amap,ai) { // note: field relations handled by OI construction
    if(ai->first->parent) {
      ai->second->parent = amap[ai->first->parent];
      ai->second->parent->children.insert(ai->second);
      ai->second->selector = fmap[ai->first->selector]; // always in AM/child
      ai->second->selector->selectors.insert(ai->second);
    }
  }
  for_map(OI*,OI*,omap,oi)
    for(int i=0; i<oi->first->inputs.size(); i++) {
      if(amap.count(oi->first->inputs[i]->domain)) 
        oi->second->add_input(fmap[oi->first->inputs[i]]);
      else // external reference
        oi->second->add_input(oi->first->inputs[i]);
    }
  // Link to op:
  body = amap[src->body]; body->bodyOf=this;
  output = fmap[src->output]; // always in root AM
}

// looks through to see if any of its children has a side-effect
bool CompoundOp::compute_side_effects() {
  clear_attribute(":side-effect"); // clear old
  OIset ois; body->all_ois(&ois);
  for_set(OI*,ois,oit)
    if((*oit)->op->marked(":side-effect")) { return mark(":side-effect"); }
  return false;
}

Parameter::Parameter(CompoundOp* op, string name, int index, 
                     ProtoType* type, ProtoType* def) : Operator(op) { 
  if(type==NULL) type = new ProtoType();
  if(def==NULL) def = new ProtoType();
  signature = new Signature(op,type); defaultValue = def; 
  this->name=name; this->index=index; container=op;
}

void Parameter::print(ostream* out) { 
  *out << "[Parameter " << index << ": "<< name;
  ProtoType any;
  if(!ProtoType::equal(defaultValue,&any)) *out<<"|"<<ce2s(defaultValue);
  *out<<"]";
}

// table of FieldOps used to date
CEmap(Operator*,FieldOp*) FieldOp::fieldops;
Operator* FieldOp::get_field_op(OperatorInstance* oi) {
  if(oi->op->marked(":side-effect"))
     return op_err(oi,"Cannot apply operators with side effects to fields");
  
  if(oi->op->isA("LocalFieldOp"))
    return dynamic_cast<LocalFieldOp &>(*oi->op).base;
  if(!oi->op->isA("Primitive") || oi->pointwise()==0) return NULL;
  // reuse or create appropriate FieldOp
  if(!fieldops.count(oi->op)) fieldops[oi->op] = new FieldOp(oi->op);
  return fieldops[oi->op];
}

// assumes base is pointwise
ProtoType* fieldop_type(ProtoType* base) {
  ProtoLocal* lt = &dynamic_cast<ProtoLocal &>(*base);
  return new ProtoField(lt);
}
FieldOp::FieldOp(Operator* base) : Primitive(base) {
  this->base = base; name = "Field~~"+base->name;
  // make field-ified version of signature
  Signature *b = base->signature;
  signature = new Signature(b,fieldop_type(b->output));
  for(int i=0;i<b->required_inputs.size();i++)
    signature->required_inputs.push_back(fieldop_type(b->required_inputs[i]));
  for(int i=0;i<b->optional_inputs.size();i++)
    signature->optional_inputs.push_back(fieldop_type(b->optional_inputs[i]));
  if(b->rest_input) signature->rest_input = fieldop_type(b->rest_input);
  signature->names = b->names;
}

// table of LocalFieldOps used to date
CEmap(Operator*,LocalFieldOp*) LocalFieldOp::localops;
Operator* LocalFieldOp::get_local_op(Operator* op) {
  if(op->isA("FieldOp")) return dynamic_cast<FieldOp &>(*op).base;
  if(!op->isA("Primitive") || !op->attributes.count(":space")) return NULL;
  // reuse or create appropriate LocalFieldOp
  if(!localops.count(op)) localops[op] = new LocalFieldOp(op);
  return localops[op];
}

ProtoType* localop_type(ProtoType* base) {
  if(base->isA("ProtoField")) return F_VAL(base);
  ierror("Tried to localize non-field type: "+base->to_str());
  return NULL; // dummy return: terminates on error
}
LocalFieldOp::LocalFieldOp(Operator* base) : Primitive(base) {
  this->base = base; name = "Local~~"+base->name;
  // make field-ified version of signature
  Signature *b = base->signature;
  signature = new Signature(b,localop_type(b->output));
  for(int i=0;i<b->required_inputs.size();i++)
    signature->required_inputs.push_back(localop_type(b->required_inputs[i]));
  for(int i=0;i<b->optional_inputs.size();i++)
    signature->optional_inputs.push_back(localop_type(b->optional_inputs[i]));
  if(b->rest_input) signature->rest_input = localop_type(b->rest_input);
  signature->names = b->names;
}

/*****************************************************************************
 *  OperatorInstances                                                        *
 *****************************************************************************/

void collect_op_references(ProtoType* t,vector<CompoundOp*> *refs) {
  // lambdas might reference; tuples, fields might contain a lambda
  if(t->isA("ProtoLambda")) {
    if(L_VAL(t) && L_VAL(t)->isA("CompoundOp"))
      refs->push_back(&dynamic_cast<CompoundOp &>(*L_VAL(t)));
  } else if(t->isA("ProtoTuple")) {
    ProtoTuple* tt = T_TYPE(t);
    for(int i=0;i<tt->types.size();i++) 
      collect_op_references(tt->types[i],refs);
  } else if(t->isA("ProtoField")) {
    if(F_VAL(t)) collect_op_references(F_VAL(t),refs);
  }
}
void collect_op_references(Operator* op,vector<CompoundOp*> *refs) {
  if(op->isA("CompoundOp")) {
    refs->push_back(&dynamic_cast<CompoundOp &>(*op));
  } else if(op->isA("Literal")) {
    collect_op_references(dynamic_cast<Literal &>(*op).value, refs);
  }
}

void add_op_references(OI* oi) {
  vector<CompoundOp*> refs; collect_op_references(oi->op,&refs);
  for(int i=0;i<refs.size();i++) {
    oi->container->funcalls[refs[i]].insert(oi);
    // Note: doesn't handle compiler-revealed relevance
    oi->container->relevant.insert(refs[i]->body);
  }
}
void delete_op_references(OI* oi) {
  vector<CompoundOp*> refs; collect_op_references(oi->op,&refs);
  for(int i=0;i<refs.size();i++) {
    oi->container->funcalls[refs[i]].erase(oi);
    if(oi->container->funcalls[refs[i]].empty()) {
      oi->container->funcalls.erase(refs[i]); 
      oi->container->relevant.erase(refs[i]->body);
    }
  }
}

OperatorInstance::OperatorInstance(CE* src,Operator *op, AM* space) {
  if(!op||!space) ierror("OperatorInstance created with null parameter");
  inherit_attributes(src);
  container=space->container; container->nodes.insert(this);
  this->op=op; output = new Field(src,space,op->signature->output,this);
  add_op_references(this);
}

Field* OperatorInstance::add_input(Field* f) {
  if(f==NULL) ierror("OperatorInstance given NULL input");
  f->use(this,inputs.size()); inputs.push_back(f); return f; 
}
Field* OperatorInstance::insert_input(vector<Field*>::iterator pos, Field* f) {
  if(f==NULL) ierror("OperatorInstance given NULL input");
  f->use(this,inputs.size()); inputs.insert(pos, f); return f; 
}
Field* OperatorInstance::remove_input(int i) {
  if(inputs.size()<=i)ierror("OperatorInstance can't remove nonexistant input");
  inputs[i]->unuse(this,i); return delete_at(&inputs,i);
}

ProtoType* OperatorInstance::nth_input(int n) {
  if(n < op->signature->n_fixed()) { // ordinary argument
    return inputs[n]->range;
  } else if(n>=op->signature->n_fixed() 
            && n < inputs.size() 
            && op->signature->rest_input) { // rest
    return inputs[n]->range;
  } else if(n==0 && op->signature->rest_input) {
    return op->signature->rest_input;
  }
  return type_err(this,"Can't find input "+i2s(n)+" in "+to_str());
}


// returns 1 if recursive, 0 if not, and -1 if unresolved
int OperatorInstance::recursive() {
  // TODO: handle lambdas in inputs
  if(!op->isA("CompoundOp")) return 0; // only compounds can be recursive
  return
    (output->domain == dynamic_cast<CompoundOp &>(*op).body); // same space?
}

// pointwise test returns 1 if pointwise, 0 if not, and -1 if unresolved
int OperatorInstance::pointwise() {
  int opp = output->range->pointwise(); if(opp==0) return 0;
  if(op->isA("FieldOp"))
    return 0;
  if(op->isA("Literal") || op->isA("Parameter")) { 
    return opp; // Literal, Parameter: depends only on value
  } else if(op->isA("Primitive")) {
    if(op->attributes.count(":space") || op->attributes.count(":time") ||
       op->attributes.count(":side-effect"))
      return 0; // primitives involving space, time, actuators aren't pointwise
    return opp; // others may depend on what they're operating on...
  } else if(op->isA("CompoundOp")) {
    OIset ois; dynamic_cast<CompoundOp &>(*op).body->all_ois(&ois);
    for_set(OI*,ois,i)
      {int inop = (*i)->pointwise(); if(inop==0) return 0; if(inop==-1) opp=-1;}
    return opp; // compound op is pointwise if all its contents are pointwise
  } else { // FieldOp, generic Operator
    return 0;
  }
}

void OperatorInstance::print(ostream* out) {
  for(int i=0;i<inputs.size();i++) {
    if(i) *out << ", ";
    if (inputs[i] == NULL) {
    	*out << "NULL";
    } else {
        *out << inputs[i]->nicename(); inputs[i]->range->print(out);
    }
  }
  if(inputs.size()) *out << " --> "; 
  op->print(out); 
  *out << " --> " << output->nicename(); output->range->print(out);
}

/*****************************************************************************
 *  Dataflow Graphs                                                          *
 *****************************************************************************/

void dfg_print_function(ostream* out,AM* root,Field* output) {
  AMset spaces; root->all_spaces(&spaces);
  Fset edges; root->all_fields(&edges);
  OIset nodes; root->all_ois(&nodes);
  *out << pp_indent() << "Amorphous Mediums:\n"; pp_push(2);
  for_set(AM*,spaces,ait)
    { *out << pp_indent(); (*ait)->print(out); *out << endl; }
  pp_pop(); *out << pp_indent()  << "Fields:\n"; pp_push(2);
  for_set(Field*,edges,fit)
    { *out << pp_indent(); (*fit)->print(out); 
      if((*fit)==output) *out<< " OUTPUT"; *out << endl; }
  pp_pop(); *out << pp_indent() << "Operator Instances:\n"; pp_push(2);
  for_set(OI*,nodes,oit)
    { *out << pp_indent(); (*oit)->print(out); 
      if((*oit)->output==output) *out<< " OUTPUT"; *out << endl; }
  pp_pop();
}

void indentSS(ostream* ss, int indent) {
  for(int i=0; i<indent; i++)
    *ss << "    ";
}

static string remove_invalid_chars(string in) {
   std::replace(in.begin(), in.end(), '~', '_');
   return in;
}

// Dot output naming utilities
string dot_oname(OI* oi) { return "OI_"+oi->output->nicename(); }
string dot_oname(Field* f) { return "OI_"+f->nicename(); }
string dot_fname(Field* f) { return f->nicename(); }
/// Print a DOT directed graph for an amorphous medium
void dot_print_AM(ostream* ss, int stepn, AM* root, Field* output,int indent=1,bool field_nodes=false){
  string outport = ""; // ":s";
  // print child amorphous mediums
  for_set(AM*,root->children,ait) {
    AM* am = *ait;
    string sub = "NO_FIELDS";
    if(am->fields.size()) sub = dot_oname(*am->fields.begin());
    string am_name = remove_invalid_chars(am->nicename());
    string cname = "cluster_fn_" + am_name;
    indentSS(ss,indent);
    *ss << "subgraph " << cname << stepn << " {\n";
    indentSS(ss,indent+1);
    *ss << "color=green;\n";
    indentSS(ss,indent+1);
    *ss << "label=\"AM: " << am_name << "\";\n";
    dot_print_AM(ss,stepn,am,output,indent+1,field_nodes);
    indentSS(ss,indent+1);
    *ss << "}\n";
    indentSS(ss,indent);
    string selector=field_nodes?dot_fname(am->selector):dot_oname(am->selector);
    string label = (field_nodes?"":(dot_fname(am->selector)+"\\n"))
      + ce2s(am->selector->range);
    string inport = ""; // ":n";
    *ss << selector << stepn << outport << " -> " << sub << stepn << inport 
        << " [label=\"" << label << "\", lhead="<<cname<<stepn<<"];\n";
  }
  // print operators
  for_set(Field*,root->fields,f) {
    OI* oi = (*f)->producer;
    //operator name
    string fname = (*f)->nicename(), oname = "OI_"+fname;
    string name = "UNKNOWN: ERROR";
    if(oi->op->name.length()>0)
      name = oi->op->name;
    else if(oi->op->isA("Literal"))
      name = ce2s(dynamic_cast<Literal &>(*oi->op).value);
    indentSS(ss,indent);
    *ss << oname << stepn <<"[label=\""<<name<<"\" shape=box];" << endl;
    //inputs
    for(int i=0; i<oi->inputs.size(); i++) {
      string label = ce2s(oi->inputs[i]->range);
      string iname = dot_fname(oi->inputs[i]);
      if(!field_nodes) { // if not showing fields...
        label = dot_fname(oi->inputs[i]) + "\\n" + label; // put name on edge
        iname = dot_oname(oi->inputs[i]); // and connect to OI instead
      } else {
        indentSS(ss,indent);
        *ss << iname << stepn << " [label=\"" << iname << "\"];" << endl;
      }
      string inport = "";
      //if(oi->inputs.size()==1 || (oi->inputs.size()==3 && i==1)) { inport==":n";
      //} else if(oi->inputs.size()<=3 && i==0) { inport=":nw";
      //} else if(oi->inputs.size()<=3) { inport=":ne";
      //}
      indentSS(ss,indent);
      *ss << iname << stepn << outport << " -> " << oname << stepn 
          << inport << " [label=\"" << label << "\"];" << endl;
    }
    //operator output
    if(field_nodes || (oi->output==output)) {
      string inport = ""; // ":n";
      indentSS(ss,indent);
      *ss << oname << stepn << outport << " -> " << fname << stepn << inport
          << " [label=\"" << oi->output->range->to_str() << "\"];" << endl;
      indentSS(ss,indent);
      // DFG output shown as double octagon
      string shape = (oi->output==output)?" shape=doubleoctagon":"";
      *ss << fname<<stepn <<" [label=\""<<fname<<"\""<<shape<<"];\n";
    }
  }
}

static int step=0;
static string pastSteps = "";
void DFG::dot_print_function(ostream* out,AM* root,Field* output,bool field_nodes) {
  stringstream ss, head;
  step++;
  
  for_set(AM*,relevant,i) {
    CompoundOp* op = (*i)->bodyOf;
    if(op) {
       ss << "    subgraph cluster_fn_" << remove_invalid_chars(op->name) << step << " {\n";
       ss << "        color=green;\n";
       ss << "        label=\"Function: " << op->name << "\";\n";
       dot_print_AM(&ss, step, *i, op->output, 2, field_nodes);
       ss << "    }\n";
    }
  }

  ss << "    subgraph cluster_base_fn" << step << " {\n";
  ss << "        color=green;\n";
  ss << "        label=\"main expression\";\n";
  dot_print_AM(&ss, step, root, output, 2, field_nodes);
  ss << "    }\n";

  *out << "digraph dfg {" << endl;
  *out << "  compound = true;" << endl; // allow edges to AM clusters
  if( ss.str().length() > 0 ) {
    ss << "  }\n";
    head << "  subgraph cluster" << step << " {\n";
    head << "    color=blue;\n";
    head << "    label=\"Step #" << step << "\";\n";
    head << ss.str();
    pastSteps += head.str();
  }
  *out << pastSteps;
  *out << "}" << endl;
}


Field* DFG::add_literal(ProtoType* val,AM* space,CompilationElement* src)
{ return (new OI(src,new Literal(src,val),space))->output; }
Field* DFG::add_parameter(CompoundOp* op,string name,int idx,AM* space,CE* src)
{ return (new OI(src,new Parameter(op,name,idx),space))->output; }

void DFG::determine_relevant() {
  if(output==NULL) return; // can't search yet
  relevant.clear(); funcalls.clear();
  set<AM*> q; q.insert(output->domain);
  while(q.size()) {
    AM* next = *q.begin(); q.erase(next); relevant.insert(next);
    OIset ois; next->all_ois(&ois);
    for_set(OI*,ois,oi) {
      vector<CompoundOp*> refs; collect_op_references((*oi)->op,&refs);
      for(int i=0;i<refs.size();i++) {
        funcalls[refs[i]].insert(*oi);
        if(!relevant.count(refs[i]->body)) { q.insert(refs[i]->body); }
      }
    }
  }
}

void DFG::relocate_input(OI* src, int src_loc, OI* dst,int dst_loc) {
  Field* f = src->inputs[src_loc];
  if(!f->consumers.erase(make_pair(src,src_loc)))
    ierror("Attempted to relocate output with missing trackbacks");
  insert_at(&dst->inputs,dst_loc,f); delete_at(&src->inputs,src_loc);
  for(int i=src_loc;i<src->inputs.size();i++) { // fix back-pointers
    src->inputs[i]->consumers.erase(make_pair(src,i+1));
    src->inputs[i]->consumers.insert(make_pair(src,i));
  }
  f->consumers.insert(make_pair(dst,dst_loc));
}
void DFG::relocate_inputs(OI* src, OI* dst,int insert) {
  while(!src->inputs.empty()) { relocate_input(src,0,dst,insert); insert++; }
}

void DFG::relocate_source(OperatorInstance* consumer, int in, Field* newsrc) {
  Field* oldsrc = consumer->inputs[in];
  oldsrc->consumers.erase(make_pair(consumer,in));
  consumer->inputs[in] = newsrc;
  newsrc->consumers.insert(make_pair(consumer,in));
}

void DFG::relocate_consumers(Field* src, Field* dst) {
  for_set(Consumer,src->consumers,i) // first consumers
    { (*i).first->inputs[(*i).second]=dst; dst->consumers.insert(*i); }
  for_set(AM*,src->selectors,ai) // then selectors
    { (*ai)->selector = dst; dst->selectors.insert(*ai); }
  src->consumers.clear(); src->selectors.clear(); // purge old
  // move outputs
  if(src->domain->bodyOf && src->domain->bodyOf->output==src)
    src->domain->bodyOf->output=dst;
  if(src->container->output==src) src->container->output = dst;
}


void DFG::delete_node(OperatorInstance* oi) {
  // release the inputs
  for(int i=0;i<oi->inputs.size();i++)
    if(oi->inputs[i]) oi->inputs[i]->unuse(oi,i);
  // blank the consumers (which should be about to be deleted)
  for_set(Consumer,oi->output->consumers,i)
    (*i).first->inputs[(*i).second] = NULL;
  // remove the space's record of the field
  if(oi->output->domain) oi->output->domain->fields.erase(oi->output);
  // remove any selector uses of the output
  for_set(AM*,oi->output->selectors,ai) (*ai)->selector=NULL;
  // remove any compound-op references
  delete_op_references(oi);
  // discard the elements
  nodes.erase(oi); edges.erase(oi->output);
  //delete oi->output; delete oi; // finally, release our memory
}

void DFG::delete_space(AM* am) {
  // release the parent & selector
  if(am->parent) am->parent->children.erase(am);
  if(am->selector) am->selector->selectors.erase(am);
  // blank children and domains (which should be able to be deleted)
  for_set(AM*,am->children,i) (*i)->parent=NULL;
  for_set(Field*,am->fields,i) (*i)->domain=NULL;
  // discard the element & release memory
  relevant.erase(am); spaces.erase(am); //delete am;
}

// move contents of medium src into medium target, then destroy src
void DFG::remap_medium(AM* src, AM* target) {
  if(src->parent) ierror("Attempted to remap non-root amorphous medium");
  // assuming root, can ignore parent, selector, bodyOf, and container links
  for_set(Field*,src->fields,i)
    { (*i)->domain = target; target->fields.insert(*i); }
  for_set(AM*,src->children,i)
    { (*i)->parent = target; target->children.insert(*i); }
  src->fields.clear(); src->children.clear();
  delete_space(src);
}

// can only inline CompoundOps
void DFG::make_op_inline(OperatorInstance* target) {
  if(!target->op->isA("CompoundOp"))
    ierror("Cannot inline operator: "+target->to_str());
  CompoundOp* nop
    = new CompoundOp(&dynamic_cast<CompoundOp &>(*target->op)); // copy op
  OIset body_ois; nop->body->all_ois(&body_ois); // remember contents
  remap_medium(nop->body,target->domain()); // add into target location
  // final stage: relocate Parameters & outputs, discard old OI
  if(nop->signature->rest_input!=NULL || nop->signature->optional_inputs.size())
    ierror("Inlining doesn't know how to handle variable argument fns yet");
  for_set(OI*,body_ois,i) {
    // Note: the bodyOf backpointer is not valid, so output is changed manually.
    if((*i)->op->isA("Parameter")) {
      // Parameters are rewired to connect to inputs
      Field* newsrc
        = target->inputs[dynamic_cast<Parameter &>(*(*i)->op).index];
      if((*i)->output==nop->output) nop->output = newsrc;
      relocate_consumers((*i)->output,newsrc); delete_node(*i);
    } else if((*i)->op==Env::core_op("restrict") && (*i)->inputs.size()==1) {
      if((*i)->inputs[0]->domain == target->output->domain) {
        // References to variables from inlining target domain are
        // accessed directly.
        if((*i)->output==nop->output) nop->output = (*i)->inputs[0];
        relocate_consumers((*i)->output,(*i)->inputs[0]); delete_node(*i);
      } else if(target->output->domain->child_of((*i)->inputs[0]->domain)) {
        // References to variables from a parent of the target domain
        // gain a second input
        (*i)->add_input(target->output->domain->selector);
      } else {
        // References to variables from other functions are simply left as is
      }
    }
  }
  relocate_consumers(target->output,nop->output);
  delete_node(target); delete nop;
}

CompoundOp* DFG::derive_op(OIset *elts,AM* space,vector<Field*> *in,Field *out,string stem){
  CompoundOp* cop = new CompoundOp(out->producer,this,make_gensym(stem)->name);
  CEmap(AM*,AM*) amap; CEmap(OI*,OI*) omap; CEmap(Field*,Field*) fmap; 
  // Create signature from I/O
  cop->signature = new Signature(cop,new ProtoType());
  for(int i=0;i<in->size();i++)
    cop->signature->required_inputs.push_back((*in)[i]->range);
  
  // create parameters
  for(int i=0;i<in->size();i++) {
    fmap[(*in)[i]] =
      add_parameter(cop,make_gensym("arg")->name,i,cop->body,(*in)[i]);
    fmap[(*in)[i]]->range = (*in)[i]->range; // set the range
  }
  
  // copy AMs & OIs
  amap[space]=cop->body;
  for_set(OI*,*elts,i) {
    AM* am = (*i)->domain();
    if(!amap.count(am)) {
      if(!am->child_of(space))
        ierror("Non-tree derived op spaces: "+am->to_str()+","+space->to_str());
      amap[am] = new AM(*i,this);
    }
  }
  for_set(OI*,*elts,i) {
    omap[*i] = new OI(*i,(*i)->op,amap[(*i)->domain()]);
    fmap[(*i)->output] = omap[*i]->output;
  }
  
  // next, hook up all edges
  for_map(AM*,AM*,amap,ai) {
    if(ai->first!=space) {
      ai->second->parent = amap[ai->first->parent];
      ai->second->parent->children.insert(ai->second);
      ai->second->selector = fmap[ai->first->selector]; // always in AM/child
      ai->second->selector->selectors.insert(ai->second);
    }
    // fields handled by OI construction; children handled by parent relations
  }
  for_map(OI*,OI*,omap,oi) {
    for(int i=0; i<oi->first->inputs.size(); i++) {
      if(amap.count(oi->first->inputs[i]->domain)) {
        if(fmap.count(oi->first->inputs[i])) { // internal field
          oi->second->add_input(fmap[oi->first->inputs[i]]);
        } else {  // new reference
          OI* restrict = new OI(oi->second,Env::core_op("restrict"),
                                oi->second->output->domain);
          restrict->add_input(oi->first->inputs[i]);
          oi->second->add_input(restrict->output);
        }
      } else {
        oi->second->add_input(oi->first->inputs[i]); // external reference
      }
    }
	// Relocate source for external references 
    for_set(Consumer,oi->first->output->consumers,j) {
      //cout << "OI consumer: " << ce2s((*j).first) << endl;
      if ((*j).first != NULL && (*j).first->op != NULL) {
   		if ((*j).first->op->name == "reference") {
   		  // External reference
   		  //cout << "Relocating source for " << ce2s((*j).first) << " to " << ce2s(oi->second->output) << endl;
   		  relocate_source((*j).first, 0, oi->second->output);
   		  break;
   	    }
      }
    }
  }

  // Assign the output
  if(fmap.count(out)) {
    cop->output = fmap[out]; // always in root AM
  } else {
    // if the output is not in the fmap, then insert a new reference
    OI* restrict = new OI(out,Env::core_op("restrict"),cop->body);
    restrict->add_input(out); restrict->output->range = out->range;
    cop->output = restrict->output;
  }
  
  // complete!
  return cop;
}

void DFG::print(ostream* out) {
  for_set(AM*,relevant,i) {
    CompoundOp* op = (*i)->bodyOf;
    if(op) {
      int n = funcalls[op].size();
      *out<<"Function: "<<op->name<<" "<<ce2s(op->signature)<<" called "
            <<n<<" times\n";
      pp_push(2); dfg_print_function(out,*i,op->output); pp_pop();
    }
  }
  dfg_print_function(out,output->domain->root(),output);
}

void DFG::printdot(ostream* out, bool field_nodes) {
  pastSteps = "";
  dot_print_function(out,output->domain->root(),output,field_nodes);
}

void CompoundOp::printbody(ostream* out) {
  if(body==NULL) ierror("Can't print CompoundOp: body is NULL");
  DFG* dfg = body->container;
  if(dfg==NULL) ierror("Can't print CompoundOp: body container is NULL");
  int n = dfg->funcalls[this].size();
  if(signature==NULL) ierror("Can't print CompoundOp: signature is NULL");
  *out<<"Function: "<<name<<" "<<ce2s(signature)<<" called "<<n<<" times\n";
  pp_push(2); dfg_print_function(out,body,output); pp_pop();
}

/*****************************************************************************
 *  PROPAGATOR                                                               *
 *****************************************************************************/

void queue_all_fields(DFG* g, IRPropagator* p) { 
  p->worklist_f.clear();
  for_set(AM*,g->relevant,i) (*i)->all_fields(&p->worklist_f);
}

void queue_all_ops(DFG* g, IRPropagator* p) {
  p->worklist_o.clear();
  for_set(AM*,g->relevant,i) (*i)->all_ois(&p->worklist_o);
}

void queue_all_ams(DFG* g, IRPropagator* p) { 
  p->worklist_a.clear();
  for_set(AM*,g->relevant,i) (*i)->all_spaces(&p->worklist_a);
}

// neighbor marking:
enum { F_MARK=1, O_MARK=2, A_MARK=4 };
CompilationElement* src;
CEset(CompilationElement*) queued;
void IRPropagator::queue_nbrs(Field* f, int marks) {
  if(marks&F_MARK || queued.count(f)) return;   queued.insert(f);
  if(f!=src) { if(act_fields) { worklist_f.insert(f); } marks |= F_MARK; }
  
  queue_nbrs(f->producer,marks); queue_nbrs(f->domain,marks);
  for_set(Consumer,f->consumers,i) queue_nbrs((*i).first,marks);
  for_set(AM*,f->selectors,ai) queue_nbrs(*ai,marks);
}
void IRPropagator::queue_nbrs(OperatorInstance* oi, int marks) {
  if(marks&O_MARK || queued.count(oi)) return;   queued.insert(oi);
  if(oi!=src) { if(act_ops) { worklist_o.insert(oi); } marks |= O_MARK; }

  queue_nbrs(oi->output,marks);
  for(int i=0;i<oi->inputs.size();i++) queue_nbrs(oi->inputs[i],marks);
  // if it's a compound op, queue the parameters & output
  if(oi->op->isA("CompoundOp")) {
    CompoundOp* cop = &dynamic_cast<CompoundOp &>(*oi->op);
    queue_nbrs(cop->body);
    queue_nbrs(cop->output,marks);
    for_set(Field*,cop->body->fields,i)
      if((*i)->producer->isA("Parameter")) queue_nbrs((*i)->producer);
  }
}
void IRPropagator::queue_nbrs(AM* am, int marks) {
  if(marks&A_MARK || queued.count(am)) return;   queued.insert(am);
  if(am!=src) { if(act_am) { worklist_a.insert(am); } marks |= A_MARK; }

  if(am==src) { // Fields & Ops don't affect one another through AM
    if(am->parent) queue_nbrs(am->parent,marks);
    if(am->selector) queue_nbrs(am->selector,marks);
    for_set(AM*,am->children,i) queue_nbrs(*i,marks);
    for_set(Field*,am->fields,i) queue_nbrs(*i,marks);
  }
}

void IRPropagator::note_change(AM* am) 
{ queued.clear(); any_changes=true; src=am; queue_nbrs(am); }
void IRPropagator::note_change(Field* f) 
{ queued.clear(); any_changes=true; src=f; queue_nbrs(f); }
void IRPropagator::note_change(OperatorInstance* oi) 
{ queued.clear(); any_changes=true; src=oi; queue_nbrs(oi); }

bool IRPropagator::maybe_set_range(Field* f,ProtoType* range) {
  if(!ProtoType::equal(f->range,range)) { 
    V2<<"Changing type of "<<ce2s(f)<<" to "<<ce2s(range)<<endl;
    f->range=range; note_change(f); return true;
  } else {
    V3<<"NOT changing type of "<<ce2s(f)<<" to "<<ce2s(range)<<endl;
    return false;
  }
}

bool IRPropagator::propagate(DFG* g) {
  V1 << "Executing analyzer " << to_str(); V1 << endl;
  any_changes=false; root=g;
  // initialize worklists
  if(act_fields) queue_all_fields(g,this); else worklist_f.clear();
  if(act_ops) queue_all_ops(g,this); else worklist_o.clear();
  if(act_am) queue_all_ams(g,this); else worklist_a.clear();
  // walk through worklists until empty
  preprop();
  int steps_remaining = 
    1+loop_abort*(worklist_f.size()+worklist_o.size()+worklist_a.size());
  while(steps_remaining>0 && 
        (!worklist_f.empty() || !worklist_o.empty() || !worklist_a.empty())) {
    // each time through, try executing one from each worklist
    if(!worklist_f.empty()) {
      Field* f = *worklist_f.begin(); worklist_f.erase(f); 
      if(root->edges.count(f)) // ignore deleted elements
        { act(f); steps_remaining--; }
    }
    if(!worklist_o.empty()) {
      OperatorInstance* oi = *worklist_o.begin(); worklist_o.erase(oi);
      if(root->nodes.count(oi)) // ignore deleted elements
        { act(oi); steps_remaining--; }
    }
    if(!worklist_a.empty()) {
      AM* am = *worklist_a.begin(); worklist_a.erase(am); 
      if(root->spaces.count(am)) // ignore deleted elements
        { act(am); steps_remaining--; }
    }
  }
  if(steps_remaining<=0) 
    ierror("Aborting "+ce2s(this)+" due to apparent infinite loop.");
  postprop();
  V2 << "Finished analyzer " << to_str();
  V1 << " changes = " << b2s(any_changes) << endl;
  return any_changes;
}


/*****************************************************************************
 *  INTEGRITY CERTIFICATION                                                  *
 *****************************************************************************/

CertifyBackpointers::CertifyBackpointers(int verbosity)
  : IRPropagator(true,true,true) { this->verbosity = verbosity; }

void CertifyBackpointers::preprop() { 
  bad=false;
  if(!root->edges.count(root->output)) {bad=true;compile_error("Bad DFG root");}
}

void CertifyBackpointers::postprop()
{ if(bad) ierror("Backpointer certification failed."); }

void CertifyBackpointers::act(Field* f) {
  // field OK
  if(f==NULL) {bad=true; compile_error("Null field"); return; }
  
  // producer OK
  if(! (root->nodes.count(f->producer) && f->producer->output==f))
    { bad=true; compile_error(f,"Bad producer of "+f->to_str()); }
  // consumer OK
  for_set(Consumer,f->consumers,i) {
    if(! (root->nodes.count((*i).first) &&
          (*i).second < (*i).first->inputs.size() &&
          (*i).first->inputs[(*i).second]==f))
      { bad=true; compile_error(f,"Bad consumer "+i2s((*i).second)+" of "+f->to_str());}
  }
  // selectors OK
  for_set(AM*,f->selectors,ai) {
    if(! (root->spaces.count(*ai) && (*ai)->selector==f))
      { bad=true; compile_error(f,"Bad selector of "+f->to_str());}
  }
  // domain OK
  if(!root->spaces.count(f->domain))
    { bad=true; compile_error(f,"No such domain for "+f->to_str()); }
  if(!f->domain->fields.count(f))
    { bad=true; compile_error(f,"Domain doesn't track field: "+f->to_str()); }
  // container OK
  if(f->container!=root || !root->edges.count(f))
    { bad=true; compile_error(f,"Bad container of "+f->to_str()); }
}

void CertifyBackpointers::act(OperatorInstance* oi) {
  // OI OK
  if(oi==NULL) {bad=true; compile_error("Null operator instance"); return; }
  if(oi->op==NULL) {bad=true; compile_error("Null operator"); return; }
  
  // output OK
  if(! (root->edges.count(oi->output) && oi->output->producer==oi))
    { bad=true; compile_error(oi,"Bad output of "+oi->op->to_str()); }
  // inputs OK
  for(int i=0;i<oi->inputs.size();i++) {
    if(!root->edges.count(oi->inputs[i]) ||
       !oi->inputs[i]->consumers.count(make_pair(oi,i)))
      {bad=true;compile_error(oi,"Bad input "+i2s(i)+" of "+oi->op->to_str());}
  }
  // container OK
  if(oi->container!=root || !root->nodes.count(oi))
    { bad=true; compile_error(oi,"Bad container of "+oi->op->to_str()); }
  // if CompoundOp, body link and outputs OK
  if(oi->op->isA("CompoundOp")) {
    CompoundOp* co = &dynamic_cast<CompoundOp &>(*oi->op);
    if(co->body==NULL || co->body->bodyOf!=co)
      { bad=true; compile_error(oi,"Bad body of "+oi->to_str());}
    if(co->output==NULL)
      { bad=true; compile_error(oi,"Bad output link of "+oi->to_str());}
  }
}
  
void CertifyBackpointers::act(AmorphousMedium* am) {
  // AM OK
  if(am==NULL) {bad=true; compile_error("Null amorphous medium"); return; }
  
  // selector & parent OK
  if(am->parent) {
    if(!root->edges.count(am->selector) ||
       !am->selector->selectors.count(am))
      {bad=true;compile_error(am,"Bad selector of "+am->to_str()); }
    if(!root->spaces.count(am->parent) ||
       !am->parent->children.count(am))
      {bad=true;compile_error(am,"Bad parent of "+am->to_str()); }
  }
  // children OK
  for_set(AM*,am->children,i)
    if(!(root->spaces.count(*i) && (*i)->parent==am)) 
      { bad=true; compile_error(am,"Bad child of "+am->to_str()); }
  // fields OK
  for_set(Field*,am->fields,i)
    if(!(root->edges.count(*i) && (*i)->domain==am))
      { bad=true; compile_error(am,"Bad field of "+am->to_str()); }
  // container OK
  if(am->container!=root || !root->spaces.count(am))
    { bad=true; compile_error(am,"Bad container of "+am->to_str()); }
  // bodyOf OK
  if(am->bodyOf!=NULL && am->bodyOf->body!=am) 
    { bad=true; compile_error(am,"Bad bodyOf link for "+am->to_str());}
}

