/* Main BioCompiler
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#include "config.h"
#include "biocompiler.h"
#include "grn_utilities.h"
#include "grn_optimizers.h"

using namespace grn;

Chemical* chem_error(CompilationElement* i,string err) {
  compile_error(i,err); return new Chemical("DUMMY");
}

/*****************************************************************************
 *  GENETIC REGULAORY NETWORK EMITTER                                        *
 *****************************************************************************/

// Note: does not handle domain change at all right now

GRNEmitter::GRNEmitter(NeoCompiler* parent, Args* args) : IRPropagator(false,false) {
  this->parent=parent;
  outstem = args->extract_switch("--grn-out")?args->pop_next():"grn_out";
  emit_to_stdout = (outstem=="stdout");
  verbosity = args->extract_switch("--emitter-verbosity")?args->pop_number():0;
  max_loops=args->extract_switch("--emitter-max-loops")?args->pop_number():10;
  paranoid = args->extract_switch("--emitter-paranoid");
  size_t found = outstem.find_last_of("/\\");
  circuit_name = (found==-1) ? outstem : outstem.substr(found+1);
  // which modes will we emit in?
  emit_matlab = args->extract_switch("--to-matlab");
  plot_all_motif_constants = args->extract_switch("--plot-all-motif-constants");
  plot_all_chemicals = args->extract_switch("--plot-all-chemicals");
  emit_sbol = !args->extract_switch("--no-sbol-output");
  emit_dot = !args->extract_switch("--no-dot-output");
  if(args->extract_switch("--no-output"))
    { emit_sbol = emit_dot = false; }
  emit_unoptimized = args->extract_switch("--output-unoptimized");
  emit_intermediate = args->extract_switch("--output-intermediates");
  
  // add path extension
  parent->proto_path.add_to_path(BIOCOMPILERDIR);
  
  SExpr* s = read_sexpr("GRN motif bootstrap","(include grn-motifs.proto)");
  if(s==NULL) ierror("Built-in core GRN library either could not be found or could not be parsed");
  parent->interpreter->interpret(s);

  bool anyplatform = false;
  while(args->extract_switch("--cellular-platform",false)) {
    string filename = args->pop_next();
    // Ensure filename ends with .proto
    size_t found = filename.find_last_of(".proto");
    if(filename.substr(found)!=".proto") filename+=".proto";
    SExpr* s = read_sexpr("Platform file","(include "+filename+")");
    if(s==NULL) ierror("Built-in platform file could either could not be found or could not be parsed: "+filename);
    parent->interpreter->interpret(s);
    anyplatform=true;
  }
  if(!anyplatform) {
    V1 << "No cellular platform specified: consider --cellular-platform NAME";
  }

  initialize_grn_optimizers(args);
}

void GRNEmitter::optimize() {
  V4<<grn.to_str();
  GRNCertifyBackpointers checker(verbosity);
  if(paranoid) checker.propagate(&grn); // make sure we're starting OK
  int step = 0;
  for(int i=0;i<max_loops;i++) {
    bool changed=false;
    for(int j=0;j<rules.size();j++) {
      bool local_change = rules[j]->propagate(&grn); terminate_on_error();
      changed |= local_change; 
      if(local_change) {
        V1<<"  Network changed" << endl;
        V4<<grn.to_str();
        if(emit_intermediate) {
          step++;
          if(emit_sbol)
            { to_sbol(get_stream_for("SBOL","_opt_"+i2s(step)+".sbol.xml")); }
          if(emit_dot)
            { to_dot(get_stream_for("GraphViz","_opt_"+i2s(step)+".dot")); }
        }
      }
      if(paranoid) checker.propagate(&grn);// make sure we didn't break anything
    }
    if(!changed) break;
    if(i==(max_loops-1))
      compile_warn("GRN optimizer ran "+i2s(max_loops)+" times without finishing: giving up and returning current partially optimized GRN.");
  }
  checker.propagate(&grn); // make sure we didn't break anything
}

// post-processing runs just one pass through all post-processing rules
void GRNEmitter::post_process() {
  GRNCertifyBackpointers checker(verbosity);
  if(paranoid) checker.propagate(&grn); // make sure we're starting OK
  for(int i=0;i<postprocessing.size();i++) {
    postprocessing[i]->propagate(&grn); terminate_on_error();
    if(paranoid) checker.propagate(&grn);// make sure we didn't break anything
  }
  checker.propagate(&grn); // make sure we didn't break anything
}


Chemical* GRNEmitter::parse_chemical(SExpr* def, map<string,Chemical*> *locals, OperatorInstance* oi) {
  if(def->isSymbol()) {
    string cname = ((SE_Symbol*)def)->name;
    // first, try lookup
    if(locals->count(cname)) return (*locals)[cname]; // argK, defined ? vars
    if(grn.chemicals.count(cname)) return grn.chemicals[cname];
    // if lookup fails, create local or global chemical
    Chemical* c;
    if(cname[0]=='?') { c = new Chemical(); (*locals)[cname] = c; } // local
    else { c = new Chemical(cname);  // global: assume is a motif-constant
      c->attributes[":motif-constant"]=new MarkerAttribute(true);
      (*locals)[cname] = c; // add to "local" list for things used locally
    }
    grn.chemicals[c->name]=c; return c;
  } else if(def->isList()) {
    SE_List_iter li((SE_List*)def);
    Chemical* c = parse_chemical(li.get_next("chemical"),locals,oi);
    while(li.has_next()) { // parse properties list
      if(li.on_token("|")) { // type
        convert_sexp_to_type(li.get_next("type"), c, oi); continue;
      } else if(li.on_token("t")) { // halflife
        c->halflife = li.get_num("halflife"); continue;
      } else if(li.on_token("H")) { // Hill coefficient
        c->hill_coefficient = li.get_num("Hill coefficient"); continue;
      } else {
        SExpr* i = li.get_next();
        compile_error(i,"Unknown chemical property: "+i->to_str());
      }
    }
    return c;
  } else {
    return chem_error(def,"Expecting a chemical identifier here, but don't know to interpret this as a chemical: "+def->to_str());
  }
}

// want to check to see if we have type information -
// check for the | and then consume the next SExpr as a type.
void look_for_chemical_type(Chemical* c,SE_List_iter *i,OperatorInstance* oi){
  if(i->on_token("|")) { convert_sexp_to_type(i->get_next("type"), c, oi); }
}

// Attach any regulations that have been waiting for something to regulate
void attach_pending_regulations(set<DNAComponent*> *targets,vector<ExpressionRegulation*> *pending) {
  for_set(DNAComponent*,*targets,dc) {
    for(int i=0;i<pending->size();i++) {
      (*dc)->regulators.insert((*pending)[i]);
      (*pending)[i]->target = (*dc);
    }
  }
  if(targets->size()) pending->clear();
}

void clear_targets_unless(set<DNAComponent*> *targets,string type) {
  if(targets->size() && !(*targets->begin())->isA(type))
    targets->clear();
}

void GRNEmitter::interpret_template(OperatorInstance* oi, SE_List* tmpl,vector<Chemical*> *ins, vector<Chemical*> *outs) {
  if(verbosity>=3){*cpout<<"Operator Instance: "; oi->op->signature->print(cpout); *cpout<<endl;}
  
  map<string,Chemical*> locals;
  for(int i=0;i<ins->size();i++) locals["arg"+i2s(i)] = (*ins)[i];// ins -> argK
  bool output_used = false;

  for(int x=0;x<tmpl->len();x++) {
    if(!(*tmpl)[x]->isList()) {
      compile_error((*tmpl)[x],"In GRN motif, expected a list specifying functional unit or reaction, but got "+(*tmpl)[x]->to_str());
      return;
    }
    SE_List_iter li((SE_List*)(*tmpl)[x]);
    FunctionalUnit* fu = new FunctionalUnit();
    set<DNAComponent*> targets;
    vector<ExpressionRegulation*> pending;
    bool after_first_regs = false, reg_last = false;
    while(li.has_next()) {
      attach_pending_regulations(&targets,&pending);
      if(li.on_token("RXN")) {
        // Pattern: 'chemical ["activates"|"represses"] chemical'
        // first chemical is regulator
        Chemical* regulator=parse_chemical(li.get_next("chemical"),&locals,oi);
        look_for_chemical_type(regulator, &li, oi);
        // then get regulation type
        bool represses = li.on_token("represses");
        if(!represses && !li.on_token("activates")) { 
          SExpr* e = li.get_next();
          compile_error(e,"'"+e->to_str()+"' was not expected keyword 'represses' or 'activates'"); return;
        }
        // finally, regulated substrate
        Chemical* substrate=parse_chemical(li.get_next("chemical"),&locals,oi);
        look_for_chemical_type(substrate, &li, oi);
        // add it to the set of reactions
        grn.reactions.insert(new RegulatoryReaction(regulator,substrate,represses));
      } else if(li.on_token("P")) {
        if(after_first_regs) pending.clear();
        after_first_regs = true;
        ProtoScalar* type = new ProtoBoolean(false); // default is low promoter
        if(li.peek_next()->isScalar()) {
          type=new ProtoScalar(li.get_num("type"));
        } else if(li.on_token("high")) { type=new ProtoBoolean(true);
        } else if(li.on_token("low")) { type=new ProtoBoolean(false);
        }
        Promoter* promoter = new Promoter(type);
        targets.clear(); targets.insert(promoter); 
        if(verbosity>=3){*cpout<<"promoter type: ";if (type==NULL)*cpout<<"null";else type->print(cpout);*cpout<<endl;}
        fu->add(promoter);
        reg_last = true;
      } else if(li.on_token("R+") || li.on_token("R-")) {
        li.unread(); bool repress = (li.get_token()=="R-");
        Chemical* c = parse_chemical(li.get_next("chemical"),&locals,oi);
        look_for_chemical_type(c, &li, oi);
        float str=li.peek_next()->isScalar() ? li.get_num():DEFAULT_STRENGTH;
        float d=li.peek_next()->isScalar()?li.get_num():DEFAULT_DISSOCIATION;
        ExpressionRegulation* er = new ExpressionRegulation(c,NULL,repress,str,d);
        pending.push_back(er);
        reg_last = true;
      } else if(li.on_token("value")) {
        if(after_first_regs && reg_last) targets.clear();
        clear_targets_unless(&targets,"CodingSequence");
        after_first_regs = true;
        output_used = true;
        for(int i=0;i<outs->size();i++) {
          CodingSequence* cds = new CodingSequence((*outs)[i]);
          fu->add(cds);
          targets.insert(cds);
        }
        reg_last = false;
      } else if(li.on_token("T")) {
        after_first_regs = true;
        fu->add(new Terminator());
        reg_last = false;
      } else { // all others are assumed to be coding sequences
        if(after_first_regs && reg_last) targets.clear();
        clear_targets_unless(&targets,"CodingSequence");
        after_first_regs = true;
        Chemical* c = parse_chemical(li.get_next("chemical"),&locals,oi);
        look_for_chemical_type(c, &li, oi);
        CodingSequence* cds = new CodingSequence(c);
        fu->add(cds); 
        targets.insert(cds);
        reg_last = false;
      }
    }
    // clean up any leftovers
    attach_pending_regulations(&targets,&pending);
    for(int i=0;i<pending.size();i++) { delete pending[i]; } // not attachable
    // only add if there's content (e.g. not for reactions)
    if(fu->sequence.size()>0) grn.dnacomponents.insert(fu);
  }
  // check to make sure every chemical got a type
  map<string,Chemical*>::iterator it;
  for(it=locals.begin(); it!=locals.end(); it++) {
    if (!((*it).second->attributes["type"]))
      compile_error(oi,"Chemical "+(*it).first+" was not assigned a type");
  }
  // check to make sure all inputs and outputs are used
  for(int i=0;i<ins->size();i++) {
    bool found=false;
    map<string,Chemical*>::iterator it;
    for(it=locals.begin(); it!=locals.end(); it++) {
      if((*it).second == (*ins)[i]) found=true;
    }
    if(!found) compile_error(oi,"Input "+i2s(i)+" is not used in GRN motif");
  }
  if(!output_used) compile_error(oi,"Output is not used in GRN motif");
}

void add_standard_completion(FunctionalUnit* fu,vector<Chemical*> *outs) {
  //fu->add(new RBS());
  for(int i=0;i<outs->size();i++)
    fu->add(new CodingSequence((*outs)[i]));
  fu->add(new Terminator());
}

SE_List* get_template(Operator* op) {
  if(!op->attributes.count(":grn-motif")) return NULL;
  Attribute* a = op->attributes[":grn-motif"];
  if(!a->isA("SExprAttribute") || !((SExprAttribute*)a)->exp->isList()) 
    { compile_error(op,":grn-motif description should be a list of functional units and reactions, but was not a list"); return NULL; }
  return (SE_List*)((SExprAttribute*)a)->exp;
      
}

// return the FU that the outputs should be added to the end of
// start by hardwiring a few templates to try out:
void GRNEmitter::add_template(OperatorInstance *oi,vector<Chemical*> *ins, vector<Chemical*> *outs) {
  if(oi->op->isA("Primitive")) {
    // First: is there a template attached to the primitive?
    SE_List* tmpl = get_template(oi->op);
    if(tmpl) { interpret_template(oi, tmpl,ins,outs); return; }
    // if not, see if we have a special-case handler:
    string name = ((Primitive*)oi->op)->name;
    if(name=="all") {
      FunctionalUnit* fu = new FunctionalUnit(); 
      Promoter* p = new Promoter();
      new ExpressionRegulation((*ins)[ins->size()-1],p);
      fu->add(p);
      add_standard_completion(fu,outs); 
      grn.dnacomponents.insert(fu); return;
    }
  } else if(oi->op->isA("Literal")) {
    ProtoType* type = ((Literal*)oi->op)->value;
    if(!type->isLiteral()) ierror("Ask to associate a GRN motif for a non-constant literal, which should never happen");
    if(type->isA("ProtoBoolean")) {
      FunctionalUnit* fu = new FunctionalUnit();
      ProtoBoolean* b = dynamic_cast<ProtoBoolean*>(type);
      fu->add(new Promoter(b));
      add_standard_completion(fu,outs);
      grn.dnacomponents.insert(fu); return;
    } else if(type->isA("ProtoScalar")) {
      compile_warn(oi,"Operator "+oi->op->to_str()+" has at least one instance that returns a number, rather than a Boolean value.  Numbers are not yet fully supported, and may produce incorrect GRNs.");
      ProtoScalar* sc = dynamic_cast<ProtoScalar*>(type);
      FunctionalUnit* fu = new FunctionalUnit();
      fu->add(new Promoter(sc));
      add_standard_completion(fu,outs);
      grn.dnacomponents.insert(fu); return;
    }
  } // fall-through case = error
  compile_error("Don't know how to make a GRN motif from "+oi->op->to_str());
}

uint8_t* GRNEmitter::emit_from(DFG* g, int* len) {
  V1 << "Starting GRN emitter..."<<endl;
  // Extract relevant portions from DFG
  if(g->relevant.size()!=1) {
    compile_error("All functions must be expanded into primitive operations, but some could not be.");
  }
  AMset spaces; g->output->domain->all_spaces(&spaces);
  Fset edges; g->output->domain->all_fields(&edges);
  OIset nodes; g->output->domain->all_ois(&nodes);

  V1 << "Assigning chemicals to fields..."<<endl;
  map<Field*,map<OperatorInstance*,Chemical*,CompilationElement_cmp>,
    CompilationElement_cmp> signals;
  // assign a regulator (e.g., protein) to each field, one per consumer 
  for(set<Field*>::iterator fit=edges.begin(); fit!=edges.end(); fit++) {
    set<pair<OperatorInstance*,int> >::iterator i=(*fit)->consumers.begin();
    int j=0;
    for(;i!=(*fit)->consumers.end();i++) {
      j++;
      string name = (*fit)->nicename()+(((*fit)->consumers.size()>1)?i2s(j):"");
      grn.chemicals[name] = new Chemical(name);
      ProtoType* type = (*fit)->range;
      grn.chemicals[name]->attributes["type"] = new ProtoTypeAttribute(type);
      signals[*fit][(*i).first] = grn.chemicals[name];
    }
  }

  V1 << "Mapping operators to motifs..."<<endl;
  // now go through and assign nodes to templates
  set<OperatorInstance*>::iterator oit;
  for(oit=nodes.begin(); oit!=nodes.end(); oit++) {
    // make the input & output vectors
    vector<Chemical*> ins, outs;
    for(int i=0;i<(*oit)->inputs.size();i++)
      ins.push_back(signals[(*oit)->inputs[i]][(*oit)]);
    map<OperatorInstance*,Chemical*,CompilationElement_cmp>* f_out = &signals[(*oit)->output];
    map<OperatorInstance*,Chemical*>::iterator it;
    for(it=f_out->begin(); it!=f_out->end(); it++) outs.push_back((*it).second);

    // now translate the template into DNA components and reactions
    add_template(*oit,&ins,&outs);
  }
  terminate_on_error(); // quit here if we've encountered an error

  // check to see if we want to emit unoptimized sbol
  if(emit_unoptimized) {
    if(emit_sbol) { to_sbol(get_stream_for("SBOL","_unoptimized.sbol.xml")); }
    if(emit_dot) { to_dot(get_stream_for("GraphViz","_unoptimized.dot")); }
  }

  // Next, optimize
  V1 << "Optimizing GRN..."<<endl;
  optimize(); // if optimization is turned off, ruleset is empty
  // Next, run post-processing
  V1 << "Postprocessing GRN..."<<endl;
  post_process();
  if(grn.dnacomponents.size()==0) {
    compile_warn("Program optimizes to empty genetic regulatory network; can any of your outputs ever be expressed?");
  }


  V1 << "Linearizing GRN to output streams..."<<endl;
  // render the network into a string, possibly printing to output stream
  ostringstream net;
  vector<string> grns;
  for(set<DNAComponent*>::iterator i=grn.dnacomponents.begin();i!=grn.dnacomponents.end();i++)
    { ostringstream dc; (*i)->print(&dc); grns.push_back(dc.str()); }
  for(set<RegulatoryReaction*>::iterator i=grn.reactions.begin();
      i!=grn.reactions.end();i++)
    { ostringstream rr; (*i)->print(&rr); grns.push_back(rr.str()); }
  sort(grns.begin(), grns.end());
  for(vector<string>::iterator i=grns.begin();i!=grns.end();i++)
    { net<<*i;net<<endl; }
  if(parent->is_dump_code) {
    *cpout<<"Genetic Regulatory Network:\n"<<net.str();
    *cpout<<"End of Genetic Regulatory Network\n" << flush; 
  }

  // and output to files
  if(emit_matlab) { to_matlab(net.str()); }
  if(emit_sbol) { to_sbol(get_stream_for("SBOL",".sbol.xml")); }
  if(emit_dot) { to_dot(get_stream_for("GraphViz",".dot")); }
  return NULL;
}

// Note: assumes file will get cleaned up by termination
ostream* GRNEmitter::get_stream_for(string fileclass,string extension) {
  ostream* s;
  if(emit_to_stdout) { s = cpout;
  } else {
    ofstream* sfile = new ofstream((outstem+extension).c_str());
    if(!sfile->is_open()) {
      compile_error("Can't open "+fileclass+" output files");
      terminate_on_error();
    }
    else {
    	s = sfile;
    }
  }
  return s;
}
