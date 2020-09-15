/* ProtoKernel code emitter
Copyright (C) 2009, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

// This turns a Proto program representation into ProtoKernel bytecode to
// execute it.

#include <stdint.h>

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "config.h"

#include "compiler.h"
#include "plugin_manager.h"
#include "proto_opcodes.h"
#include "scoped_ptr.h"

using namespace std;

map<int,string> opnames;
map<string,int> primitive2op;
map<int,int> op_stackdeltas;

map<OI*, set<Instruction*> > letsForFolders;

// forward declaration for Block
struct Block;

/// instructions like SQRT_OP that have no special rules or pointers
struct Instruction : public CompilationElement { reflection_sub(Instruction,CE);
  /// sequence links
  Instruction *next,*prev; 
  /// code block containing this instruction
  Block* container;
  /// index in opcode chain, unknown=-1
  int location; 
  /// instructions "neighboring" this one
  set<Instruction*, CompilationElement_cmp> dependents;
  /// instruction's opcode
  OPCODE op;
  /// values consumed after op
  vector<uint8_t> parameters; 
  /// change in stack size following this instruction
  int stack_delta; 
  /// change in environment size following this instruction
  int env_delta; 

  Instruction(OPCODE op, int ed=0) {
    this->op=op; stack_delta=op_stackdeltas[op]; env_delta=ed; 
    location=-1; next=prev=NULL; container = NULL;
  }
  virtual void print(ostream* out=0) {
    *out << (opnames.count(op) ? opnames[op] : "<UNKNOWN OP>");
    if(ProtoKernelEmitter::op_debug) {
      *out << " [" << location << " S: " << stack_delta << " E: " << env_delta << "]";
    }
    for(int i=0;i<parameters.size();i++) { *out << ", " << i2s(parameters[i]); }
  }
  
  virtual int size() { return 1 + parameters.size(); }
  virtual bool resolved() { return location>=0; }
  virtual void output(uint8_t* buf) {
    if(!resolved()) ierror("Attempted to output unresolved instruction.");
    buf[location]=op;
    for(int i=0;i<parameters.size();i++) { buf[location+i+1] = parameters[i]; }
    if(next) next->output(buf);
  }
  
  virtual int start_location() { return location; }
  virtual int next_location() { 
    if(location==-1 || size()==-1) return -1;
    else return location+size();
  }
  virtual void set_location(int l) { location = l; } 
  virtual int net_env_delta() { return env_delta; }
  virtual int max_env_delta() { return max(0, env_delta); }
  virtual int net_stack_delta() { return stack_delta; }
  virtual int max_stack_delta() { return max(0, stack_delta); }

  void padd8(uint8_t param) {
  	  parameters.push_back(param);
  }
  
  void padd16(uint16_t param) {
  	  parameters.push_back(param >> 8);
  	  parameters.push_back(param & 0xFF);
  }
  
  void padd(unsigned int param) {
  	uint8_t a[5];
  	a[0] = 0x80 | ((param >> 28) & 0x7F);
  	a[1] = 0x80 | ((param >> 21) & 0x7F);
  	a[2] = 0x80 | ((param >> 14) & 0x7F);
  	a[3] = 0x80 | ((param >>  7) & 0x7F);
  	a[4] =          param        & 0x7F ;
  	size_t i = 0;
  	while(a[i] == 0x80) i++;
  	for(;i<5;i++) parameters.push_back(a[i]);
  }
};

struct Global : public Instruction { reflection_sub(Global,Instruction);
  int index;
  Global(OPCODE op) : Instruction(op) { index = -1; }
};

/// DEF_VM
struct iDEF_VM : public Instruction { reflection_sub(iDEF_VM,Instruction);
  int export_len, n_exports, n_globals, n_states, max_stack, max_env;
  iDEF_VM() : Instruction(DEF_VM_OP)
  { export_len=n_exports=n_globals=n_states=max_stack=max_env=-1;}
  bool resolved() {
    return export_len>=0 && n_exports>=0 && n_globals>=0 && n_states>=0
             && max_stack>=0 && max_env>=0 && Instruction::resolved();
  }
  virtual void output(uint8_t* buf) {
    padd8(export_len); padd8(n_exports); padd16(n_globals); padd8(n_states);
    padd16(max_stack+1); padd8(max_env); // +1 for enclosing function call
    Instruction::output(buf);
  }
  virtual void print(ostream* out=0) {
     if(ProtoKernelEmitter::op_debug)
        *out << "VM Definition "
           << "[ export_len:" << export_len
           << ", n_exports:"  << n_exports
           << ", n_globals:"  << n_globals
           << ", n_states:"   << n_states
           << ", max_stack:"  << max_stack
           << ", max_env:"    << max_env
           << " ]";
     else
        Instruction::print(out);
  }
  int size() { return 9; }
};

/// DEF_FUN_k_OP, DEF_FUN_OP, DEF_FUN16_OP
struct iDEF_FUN : public Global { reflection_sub(iDEF_FUN,Global);
  Instruction* ret;
  int fun_size;
  iDEF_FUN(CompoundOp* src=NULL) : Global(DEF_FUN_OP) {
    if(src) this->attributes["function~def"] = new CEAttr(src);
    ret=NULL; fun_size=-1;
  }
  bool resolved() { return fun_size>=0 && Instruction::resolved(); }
  int size() { return (fun_size<0) ? -1 : Instruction::size(); }
};

/// DEF_TUP_OP, DEF_VEC_OP, DEF_NUM_VEC_OP, DEV_NUM_VEC_k_OP
struct iDEF_TUP : public Global { reflection_sub(iDEF_TUP,Global);
  int size;
  iDEF_TUP(int size,bool literal=false) : Global(DEF_TUP_OP) {
    this->size=size;
    if(!literal) { // change to DEF_NUM_VEC_OP
      stack_delta = 0;
      if(size <= MAX_DEF_NUM_VEC_OPS) { op = DEF_NUM_VEC_OP + size;
      } else { op = DEF_NUM_VEC_OP; padd(size); }
    } else { // keep as DEF_TUP_OP
      stack_delta = -size;
      padd(size);
    }
  }
};

/// A block is a sequence of instructions
struct Block : public Instruction { reflection_sub(Block,Instruction);
  Instruction* contents;
  Block(Instruction* chain);
  virtual void print(ostream* out=0);
  virtual int size() { 
    Instruction* ptr = contents; int s=0;
    while(ptr){ if(ptr->size()==-1){return -1;} s+=ptr->size(); ptr=ptr->next; }
    return s;
  }
  virtual bool resolved() { 
    Instruction* ptr = contents; 
    while(ptr) { if(!ptr->resolved()) return false; ptr=ptr->next; }
    return true;
  }
  virtual void output(uint8_t* buf) {
    Instruction* ptr = contents; while(ptr) { ptr->output(buf); ptr=ptr->next; }
    if(next) next->output(buf);
  }

  virtual int net_env_delta() { 
    env_delta = 0;
    Instruction* ptr = contents;
    bool defFun = false;
    if(ptr->isA("iDEF_FUN")) {
       defFun = true;
    }
    while(ptr && (!defFun || ptr->op != RET_OP)) {
       env_delta+=ptr->net_env_delta();
       ptr=ptr->next;
    }
    return env_delta;
  }
  virtual int max_env_delta() { 
    int delta = 0, max_delta = 0;
    Instruction* ptr = contents;
    bool defFun = false;
    if(ptr->isA("iDEF_FUN")) {
       defFun = true;
    }
    while(ptr && (!defFun || ptr->op != RET_OP)) {
      max_delta = max(max_delta, delta + ptr->max_env_delta());
      delta+=ptr->net_env_delta(); ptr=ptr->next;
    }
    return max_delta;
  }
  virtual int net_stack_delta() {
    stack_delta = 0;
    Instruction* ptr = contents;
    bool defFun = false;
    if(ptr->isA("iDEF_FUN")) {
       defFun = true;
    }
    while(ptr && (!defFun || ptr->op != RET_OP)) {
    	int thisdelta = ptr->net_stack_delta();
    	stack_delta += thisdelta;
    	//stack_delta+=ptr->net_stack_delta();
    	ptr=ptr->next;
    }
    // Ret
    if (defFun) {
      stack_delta = stack_delta - 1;
    }
    return stack_delta;
  }
  virtual int max_stack_delta() {
    int delta = 0, max_delta = 0;
    Instruction* ptr = contents;
    bool defFun = false;
    if(ptr->isA("iDEF_FUN")) {
       defFun = true;
    }
    while(ptr && (!defFun || ptr->op != RET_OP)) {
      max_delta = max(max_delta, delta + ptr->max_stack_delta());
      delta+=ptr->net_stack_delta(); ptr=ptr->next;
    }
    return max_delta;
  }
};


/* For manipulating instruction chains */
Instruction* chain_end(Instruction* chain) 
{ return (chain->next==NULL) ? chain : chain_end(chain->next); } 
Instruction* chain_start(Instruction* chain) 
{ return (chain->prev==NULL) ? chain : chain_start(chain->prev); } 
Instruction* chain_i(Instruction** chain, Instruction* newi) {
  if (newi == NULL) {
	return *chain;
  }
  if(*chain) { 
    (*chain)->next=newi;
    if((*chain)->container) {
      Instruction* p = newi;
      while(p) { 
        p->container = (*chain)->container;
        (*chain)->container->dependents.insert(p);
        p = p->next;
      }
    }
  } 
  newi->prev=*chain;
  return *chain=chain_end(newi);
}
void chain_insert(Instruction* after, Instruction* insert) {
  if(after->next) after->next->prev=chain_end(insert);
  chain_end(insert)->next=after->next; insert->prev=after; after->next=insert;
}
Instruction* chain_split(Instruction* newstart) {
  Instruction* prev = newstart->prev;
  newstart->prev=NULL; if(prev) prev->next=NULL;
  return prev;
}
void chain_delete(Instruction* start,Instruction* end) {
  if(end->next) end->next->prev=start->prev;
  if(start->prev) start->prev->next=end->next;
}

string wrap_print(ostream* out, string accumulated, string newblock, int len) {
  if(len==-1 || accumulated.size()+newblock.size()<=len) {
    return accumulated + newblock;
  } else if(len==0) { // never accumulate
    *out << newblock; return "";
  } else { // gone past wrap boundary
    *out << accumulated << endl; return "  "+newblock;
  }
}

string print_chain_comments(Instruction* chain, int compactness) {
  if(compactness) return ""; // no comments if at all compact

  if(chain->isA("iDEF_FUN")) { // add function name
    string name = "[unknown]"; 
    if(chain->attributes.count("function~def")) {
      CE* fndef = ((CEAttr*)chain->attributes["function~def"])->value;
      name = ((CompoundOp*)fndef)->name;
    }
    return " // Function: " + name;
  } else { // default: no comment
    return "";
  }
}

string print_raw_chain(Instruction* chain, string line, int line_len,
                       ostream* out, int compactness, bool recursed=false) {
  bool defFun = false;
  if (chain->isA("iDEF_FUN")) {
    defFun = true;
  }
  while(chain) {
    if(chain->isA("Block")) {
      line = print_raw_chain(dynamic_cast<Block &>(*chain).contents, line,
          line_len, out, compactness, true);
      chain = chain->next;
    } else {
      string block = chain->to_str();
      string comments = print_chain_comments(chain,compactness);
      if (defFun && chain->op == RET_OP) {
    	  chain = NULL;
      } else {
          chain = chain->next;
      }
      if(chain || recursed) block += (compactness ? ", " : ","+comments+"\n  ");
      line = wrap_print(out,line,block,line_len);
    }
  }
  return line;
}

/** 
 * walk through instructions, printing:
 * compactness: 0: one instruction/line, 1: 70-char lines, 2: single line
 */
void print_chain(Instruction* chain, ostream* out, int compactness=0) {
  int line_len = (compactness ? (compactness==1 ? 70 : -1) : 0);
  string header = "uint8_t script[] = {"; 
  string line = wrap_print(out,"",header+(compactness ? " " : "\n  "),line_len);
  line = print_raw_chain(chain,line,line_len,out,compactness);
  *out << wrap_print(out,line," };\n",line_len);
  int code_len = chain_end(chain)->next_location();
  *out<<"uint16_t script_len = " << code_len << ";" << endl;
}

Block::Block(Instruction* chain) : Instruction(-1) { 
  contents = chain_start(chain);
  Instruction* ptr = chain;
  while(ptr){ ptr->container=this; dependents.insert(ptr); ptr=ptr->next; }
}
void Block::print(ostream* out) 
{ *out<<"{"<<print_raw_chain(contents,"",-1,out,3)<<"}"; }

// NoInstruction is a placeholder for deleted bits
struct NoInstruction : public Instruction {
  reflection_sub(NoInstruction,Instruction);
  NoInstruction() : Instruction(-1) {}
  virtual void print(ostream* out=0) { *out << "<No Instruction>"; }
  virtual int size() { return 0; }
  virtual bool resolved() { return start_location()>=0; }
  virtual void output(uint8_t* buf) {
    if(next) next->output(buf);
  }
};

int debugIndexCounter = 0;

struct PopLet : public Instruction { reflection_sub(PopLet, Instruction);
  std::set<int> debugIndices;
  PopLet() : Instruction(POP_LET_OP) { }
  PopLet(OPCODE op) : Instruction(op) { }
  void addDebugIndex(int i) {
	  debugIndices.insert(i);
  }
  virtual void print(ostream* out=0) {
  	Instruction::print(out);
    if(ProtoKernelEmitter::op_debug) {
  	  set<int>::iterator it;
  	  bool first = true;
  	  *out << " {";
  	  for ( it=debugIndices.begin() ; it != debugIndices.end(); it++ ) {
  		if (first)
  			first = false;
  		else
  	      *out << " ";
  		*out << *it;
  	  }
      *out << "}";
    }
  }
};

/// LET_OP, LET_k_OP
struct iLET : public Instruction { reflection_sub(iLET,Instruction);
  Instruction* pop;
  int debugIndex;
  CEset(Instruction*) usages;
  iLET() : Instruction(LET_1_OP,1) { pop=NULL; debugIndex = debugIndexCounter++; }
  bool resolved() { return pop!=NULL && Instruction::resolved(); }
  virtual void print(ostream* out=0) {
	 Instruction::print(out);
	 if (ProtoKernelEmitter::op_debug) {
	    *out << " {" << debugIndex << "}";
	 }
   }
};

struct DummyiLET : public iLET {
  DummyiLET() : iLET() { pop = NULL; debugIndex = 0; debugIndexCounter--;}
};

/// REF_OP, REF_k_OP, GLO_REF16_OP, GLO_REF_OP, GLO_REF_k_OP
struct Reference : public Instruction { reflection_sub(Reference,Instruction);
  Instruction* store; // either an iLET or a Global
  int offset; bool vec_op;
  Reference(Instruction* store,OI* source) : Instruction(GLO_REF_OP) { 
    attributes["~Ref~Target"] = new CEAttr(source);
    this->store=store; classify_reference();
  }
  // vector op form
  Reference(OPCODE op, Instruction* store,OI* source) : Instruction(op){ 
    attributes["~Ref~Target"] = new CEAttr(source);
    if(!store->isA("Global")) ierror("Vector reference to non-global");
    this->store=store; store->dependents.insert(this);
    offset=-1; padd8(255); vec_op=true;
  }
  // Reference to a function not yet serialized (e.g. due to mutual recursion)
  // This requires classify_reference() to be called one it is set
  CompoundOp* target;
  Reference(CompoundOp* target,OI* source) : Instruction(GLO_REF_OP) {
    attributes["~Ref~Target"] = new CEAttr(source);
    store = NULL; this->target = target;
  }

  void classify_reference() {
    bool global = store->isA("Global"); // else is a let
    this->store=store; store->dependents.insert(this);
    offset=-1; vec_op=false; 
    if (!global) {
      op = REF_OP;
      dynamic_cast<iLET &>(*store).usages.insert(this);
    }
  }

  void moveStore(Instruction* newStore) {
	  bool global = store->isA("Global");
	  if (store != NULL) {
		  store->dependents.erase(this);
		  if (!global) {
		    dynamic_cast<iLET &>(*store).usages.erase(this);
		  }
	  }
	  this->store = newStore;
	  global = store->isA("Global");
	  store->dependents.insert(this);
	  if (!global) {
		op = REF_OP;
	    dynamic_cast<iLET &>(*store).usages.insert(this);
	  }
  }

  bool resolved() { return offset>=0 && Instruction::resolved(); }
  int size() { return (offset<0) ? -1 : Instruction::size(); }
  void set_offset(int o) {
    offset = o;
    if(vec_op) {
      if(o < 256) parameters[0] = o;
      else ierror("Vector reference too large: "+i2s(o));
    } else if(store->isA("Global")) {
      parameters.clear();
      if(o < MAX_GLO_REF_OPS) { op = GLO_REF_0_OP + o;
      } else { op = GLO_REF_OP; padd(o); }
    } else {
      parameters.clear();
      if(o < MAX_REF_OPS) { op = REF_0_OP + o;
      } else  { op = REF_OP; padd(o); }
    }
  }
  virtual void print(ostream* out=0) {
  	 Instruction::print(out);
  	 if (ProtoKernelEmitter::op_debug) {
       if (store != NULL) {
         bool global = store->isA("Global"); // else is a let
         if (!global) {
           iLET l = dynamic_cast<iLET &>(*store);
           *out << " {" << l.debugIndex << "}";
         }
       } else {
    	   *out << " {unresolved}";
       }
  	 }
  }
};

/// IF_OP, IF_16_OP, JMP_OP, JMP_16_OP
struct Branch : public Instruction { reflection_sub(Branch,Instruction);
  Instruction* after_this;
  int offset; bool jmp_op;
  Branch(Instruction* after_this,bool jmp_op=false) : Instruction(IF_OP) {
    this->after_this = after_this; this->jmp_op=jmp_op; offset=-1;
    if(jmp_op) { op=JMP_OP; }
  }
  bool resolved() { return offset>=0 && Instruction::resolved(); }
  void set_offset(int o) {
    offset = o;
    parameters.clear();
    op = (jmp_op?JMP_OP:IF_OP); padd(o);
  }
};

/** 
 * Class for handling instructions to call functions
 * (e.g., FUNCALL_0_OP, FUNCALL_1_OP, FUNCALL_OP, etc.)
 */
struct FunctionCall : public Instruction { reflection_sub(FunctionCall,Instruction);
   public:
      CompoundOp* compoundOp;

      /**
       * Creates a new function call instruction.
       */
      FunctionCall(CompoundOp* compoundOpParam)
         : Instruction(FUNCALL_0_OP) {
         compoundOp = compoundOpParam;
         // dynamically compute op, env_delta, & stack_delta
         int nArgs = getNumParams(compoundOp);
         op = FUNCALL_0_OP+nArgs;
         stack_delta = -nArgs;
         env_delta = 0; // we clear the env stack when done, delta is 0
      }
   private:
      /**
       * @returns the number of parameters of this function call
       */
      static int getNumParams(CompoundOp* compoundOp) {
         // n_fixed = required + optional parameters
         // rest_input = single tuple of remaining parameters
        return compoundOp->signature->n_fixed() +
            (compoundOp->signature->rest_input!=NULL);
      }
};

// Structure for instructions that index a state or export.

struct InstructionWithIndex : public Instruction {
  reflection_sub(InstructionWithIndex, Instruction);

  int index;

  InstructionWithIndex(OPCODE opcode) : index(-1), Instruction(opcode) {}
  bool resolved(void)
  { return ((0 <= index) && Instruction::resolved()); }

  // FIXME: Should be size_t, not int.
  int size() { return 1 + parameters.size() + 1; }

  void
  print(ostream *out = 0)
  {
    Instruction::print(out);
    *out << ", " << index;
  }

  void
  output(uint8_t *buf)
  {
    if (!resolved())
      ierror("Attempted to output unresolved instruction.");
    if (! ((0 <= index) && (index <= 0xff)))
      ierror("Invalid index: " + index);
    buf[location] = op;
    for (size_t i = 0; i < parameters.size(); i++)
      buf[location + 1 + i] = parameters[i];
    buf[location + 1 + parameters.size() + 1] = static_cast<uint8_t>(index);
    if (next)
      next->output(buf);
  }
};

struct Fold : public InstructionWithIndex {
  CompoundOp *folder;
  CompoundOp *nbrop;

  reflection_sub(Fold, InstructionWithIndex);
  Fold(OPCODE opcode, CompoundOp *folder_, CompoundOp *nbrop_)
    : InstructionWithIndex(opcode), folder(folder_), nbrop(nbrop_)
  {}
};

struct InitFeedback : public Instruction {
  CompoundOp *init;
  CompoundOp *update;
  Instruction* letAfter;
  int index;

  reflection_sub(InitFeedback, Instruction);
  InitFeedback(OPCODE opcode, CompoundOp *init_, CompoundOp *update_)
    : Instruction(opcode), init(init_), update(update_), letAfter(NULL)
  {}
  void set_index(int o) {
     index = o;
     parameters.clear();
     padd(o);
   }
};

struct Feedback : public Instruction {
   InitFeedback *matchingInit;
   int index;

   reflection_sub(Feedback, Instruction);
   Feedback(OPCODE opcode) : Instruction(opcode), matchingInit(NULL) {}
   void set_index(int o) {
        index = o;
        parameters.clear();
        padd(o);
  }
};

/*****************************************************************************
 *  PROPAGATOR                                                               *
 *****************************************************************************/

class InstructionPropagator : public CompilationElement {
 public:
  // behavior variables
  int verbosity;
  int loop_abort; // # equivalent passes through worklist before assuming loop
  // propagation work variables
  set<Instruction*, CompilationElement_cmp> worklist_i;
  bool any_changes;
  Instruction* root;
  
  InstructionPropagator(int abort=10) { loop_abort=abort; }
  bool propagate(Instruction* chain); // act on worklist until empty
  virtual void preprop() {} virtual void postprop() {} // hooks
  // action routines to be filled in by inheritors
  virtual void act(Instruction* i) {}
  // note_change: adds neighbors to the worklist
  void note_change(Instruction* i);
 private:
  void queue_chain(Instruction* chain);
  void queue_nbrs(Instruction* i, int marks=0);
};

CompilationElement* isrc;
set<CompilationElement*, CompilationElement_cmp> iqueued;
void InstructionPropagator::note_change(Instruction* i) 
{ iqueued.clear(); any_changes=true; isrc=i; queue_nbrs(i); }
void InstructionPropagator::queue_nbrs(Instruction* i, int marks) {
  if(i->prev) worklist_i.insert(i->prev); // sequence neighbors
  if(i->next) worklist_i.insert(i->next);
  if(i->container) worklist_i.insert(i->container); // block
  // plus any asking for wakeup...
  for_set(Instruction*,i->dependents,j) worklist_i.insert(*j);
 }

void InstructionPropagator::queue_chain(Instruction* chain) {
  while(chain) {
    worklist_i.insert(chain);
    if (chain->isA("Block"))
      queue_chain(dynamic_cast<Block &>(*chain).contents);
    chain=chain->next;
  }
}

bool InstructionPropagator::propagate(Instruction* chain) {
  V1 << "Executing analyzer " << to_str() << ":"; V2 << endl;
  any_changes=false; root = chain_start(chain);
  // initialize worklists
  worklist_i.clear(); queue_chain(chain); 
  // walk through worklists until empty
  preprop();
  int steps_remaining = loop_abort*(worklist_i.size());
  while(steps_remaining>0 && !worklist_i.empty()) {
    // each time through, try executing one from each worklist
    if(!worklist_i.empty()) {
      Instruction* i = *worklist_i.begin(); worklist_i.erase(i); 
      act(i); steps_remaining--;
    }
  }
  if(steps_remaining<=0) ierror("Aborting due to apparent infinite loop.");
  postprop();
  V2 << "Finished analyzer " << to_str() << ":";
  V1 << " changes = " << b2s(any_changes) << endl;
  return any_changes;
}

class StackEnvSizer : public InstructionPropagator {
 public:
  // height = after instruction; maxes = anytime during
  CEmap(Instruction *, int) stack_height, stack_maxes;
  CEmap(Instruction *, int) env_height, env_maxes;
  CEmap(Instruction *, CEset(Instruction *)) dependents;

  StackEnvSizer(ProtoKernelEmitter *parent, Args *args) : emitter_(parent)
  { verbosity = parent->verbosity; }

  void print(ostream *out = 0) { *out << "StackEnvSizer"; }

  void
  note_change(Instruction *instruction)
  {
    InstructionPropagator::note_change(instruction);
    CEmap(Instruction *, CEset(Instruction *))::const_iterator i
      = dependents.find(instruction);
    if (i != dependents.end())
      for_set (Instruction *, (*i).second, j)
        worklist_i.insert(*j);
  }

  void
  preprop()
  {
    stack_height.clear();
    env_height.clear();
    dependents.clear();
  }

  void
  postprop()
  {
    Instruction *chain = root;
    int max_env = 0, max_stack = 0;
    string ss = "Stack heights: ", es = "Env heights:   ";
    stack<Instruction *> block_nesting;

    while (true) {
      if (chain == NULL) {
        if (block_nesting.empty())
          break;
        ss += "} ";
        es += "} ";
        chain = block_nesting.top()->next;
        block_nesting.pop();
        continue;
      } else if (chain->isA("Block")) {
        // Walk through subblocks.
        ss += "{ ";
        es += "{ ";
        block_nesting.push(chain);
        chain = dynamic_cast<Block &>(*chain).contents;
        continue;
      } else if (chain->isA("Reference")) {
        // Reference depths in environment.
        Reference *r = &dynamic_cast<Reference &>(*chain);
        if (r->store->isA("iLET")
            && env_height.count(r->store)
            && env_height.count(r)) {
          int rh = env_height[r], sh = env_height[r->store];
          if ((rh - sh) >= 0 && r->offset != (rh - sh)) {
            V3 << "Setting let ref offset: " << rh << "-" << sh
               << "=" << (rh - sh) << endl;
            r->set_offset(rh - sh);
          }
          if (r->marked("~Read-Reference")) {
        	  // If it's a read inside the update funcall for the rep update, the env stack size is the size at the funcall
        	  //   + the num of args of the funcall +the size at the rep
        	  // Find the funcall
        	  Instruction *findFuncall = r->store;
        	  while (findFuncall != NULL && !findFuncall->isA("FunctionCall")) {
        		  findFuncall = findFuncall->next;
        	  }
        	  if (findFuncall != NULL) {
        		  //findFuncall = findFuncall->prev; // Right at the funcall
        		  rh += env_height[findFuncall];
        	  }
        	  if ((rh - sh) >= 0 && r->offset != (rh - sh)) {
        		  V3 << "Setting read refs offset: " << rh << "-" << sh << "=" << (rh - sh) << endl;
        		  r->set_offset(rh - sh);
        	  }
          }
        }
      }
      
      if (! (stack_maxes.count(chain) && env_maxes.count(chain))) {
    	  V2 << "Wasn't able to entirely resolve: " << ce2s(chain) << endl;
    	  V2 << "Stack Maxes: " << stack_maxes.count(chain) << endl;
    	  V2 << "Env Maxes: " << env_maxes.count(chain) << endl;
    	  V2 << "Context: " << ce2s(chain->prev) << "; [inst]; " << ce2s(chain->next) << endl;
        // Wasn't able to entirely resolve.
    	  V2 << ss << endl;
    	  V2 << es << endl;
         return;
      }

      max_stack = max(max_stack, stack_maxes[chain]);
      max_env = max(max_env, env_maxes[chain]);
      ss += i2s(stack_height[chain]) + " ";
      es += i2s(env_height[chain]) + " ";
      chain = chain->next;
    }

    V2 << ss << endl;
    V2 << es << endl;

    int final = stack_height[chain_end(root)];
    if (final) {
      print_chain(chain_start(root), cpout);
      ierror("Stack resolves to non-zero height: " + i2s(final));
    }

    iDEF_VM *dv = &dynamic_cast<iDEF_VM &>(*root);
    if (! ((dv->max_stack == max_stack) && (dv->max_env == max_env))) {
      V2 << "Changed: Stack: " << dv->max_stack << " -> " << max_stack
         << " Env: " << dv->max_env << " -> " << max_env << endl;
      dv->max_stack = max_stack;
      dv->max_env = max_env;
      any_changes = true;
    } else {
      V2 << "No change to stack or env maximum size" << endl;
      any_changes = false;
    }
  }

  void
  maybe_set_stack(Instruction *i, int neth, int maxh)
  {
    V4 << "Inst(" << ce2s(i) << ")";
    if (!stack_height.count(i)) {
      V4 << " did not exist" << endl;
    } else {
      V4 << " was (" << stack_height[i] << ", " << stack_maxes[i] << ")";
      V4 << "\t now (" << neth << ", " << maxh << ")" << endl;
    }
    if (! (stack_height.count(i)
           && (stack_height[i] == neth)
           && (stack_maxes[i] == maxh))) {
      V4 << "\t Set stack height from " << stack_height[i] << " to " << neth << endl;
      V4 << "\t Set stack maxes from " << stack_maxes[i] << " to " << maxh << endl;
      stack_height[i] = neth;
      stack_maxes[i] = maxh;
      note_change(i);
    }
  }

  void
  maybe_set_env(Instruction *i, int neth, int maxh)
  {
    V4 << "Inst(" << ce2s(i) << ")";
    if (!env_height.count(i)) {
      V4 << " did not exist" << endl;
    } else {
      V4 << " was (" << env_height[i] << ", " << env_maxes[i] << ")";
      V4 << "\t now (" << neth << ", " << maxh << ")" << endl;
    }
    if (! (env_height.count(i)
           && (env_height[i] == neth)
           && (env_maxes[i] == maxh))) {
      V4 << "\t Set env height from " << env_height[i] << " to " << neth << endl;
      V4 << "\t Set env maxes from " << env_maxes[i] << " to " << maxh << endl;

      env_height[i] = neth;
      env_maxes[i] = maxh;

      note_change(i);
    }
  }

  void
  act(Instruction *i)
  {
    int baseh;

    // Stack heights.
    baseh = -1;
    
    if (!i->prev && (!i->container || (i->container && !i->container->prev)))
      // Base case: i is first instruction.
      baseh = 0;
    else if (!i->prev && stack_height.count(i->container->prev))
      // i is the first instruction of a block and inst prior to block
      // has been resolved.
      baseh = stack_height[i->container->prev];
    else if (i->prev && stack_height.count(i->prev))
      // i has a previous instruction whose value has been resolved.
      baseh = stack_height[i->prev];

    // If the instruction whose value i depends on has been resolved, then
    // resolve i's value.
    if (baseh >= 0) {
      int extra_net = i->net_stack_delta();
      int extra_max = i->max_stack_delta();
      // FIXME: Mega-kludgerific!  This is totally the wrong place to
      // do this computation.
      if (i->isA("Fold")) {
        V4 << "Handling fold... " << extra_net << ", " << extra_max << endl;
        Fold *fold = &dynamic_cast<Fold &>(*i);
        Instruction *i_folder = compound_op_block(fold->folder);
        Instruction *i_nbrop = compound_op_block(fold->nbrop);
        dependents[i_folder].insert(i);
        dependents[i_nbrop].insert(i);
        extra_net += i_folder->net_stack_delta() + i_nbrop->net_stack_delta();
        extra_max
          += max(i_folder->max_stack_delta(), i_nbrop->max_stack_delta());
        V4 << "Handling fold*... " << extra_net << ", " << extra_max << endl;
      }
      maybe_set_stack(i, baseh + extra_net, baseh + extra_max);
    } else {
      V4 << "Instruction " << ce2s(i) << " (stack) could not be resolved yet."
         << endl;
    }

    // Env heights.
    baseh = -1;
    if (!i->prev && (!i->container || (i->container && !i->container->prev)))
      baseh=0;
    else if (!i->prev && env_height.count(i->container->prev))
      baseh = env_height[i->container->prev];
    else if (i->prev && env_height.count(i->prev))
      baseh = env_height[i->prev];

    if (baseh >= 0) {
      int extra_net = i->net_env_delta();
      int extra_max = i->max_env_delta();
      if (i->isA("Fold")) {
        Fold *fold = &dynamic_cast<Fold &>(*i);
        Instruction *i_folder = compound_op_block(fold->folder);
        Instruction *i_nbrop = compound_op_block(fold->nbrop);
        dependents[i_folder].insert(i);
        dependents[i_nbrop].insert(i);
        // FIXME: This pattern of computing the extra depth must be
        // named somewhere.
        
        // This is broken, we don't put all the folder/nbrop args on the env stack
        //  nbrop arg is put on by the fold-hood-op, folder args are handled by the callback, and removed
        // So the end result of a fold-hood-op is +0 env stack size
        // But the max is +2
      
        V2 << "extra net: " << extra_net << endl;
        V2 << "extra max: " << extra_max << endl;
        int folder_arity = fold->folder->signature->required_inputs.size();
        V2 << "Folder Arity: " << folder_arity << endl;
        folder_arity = 0;
        int nbrop_arity = fold->nbrop->signature->required_inputs.size();
        V2 << "nbrop Arity: " << nbrop_arity << endl;
        nbrop_arity = 1;
        //extra_net += folder_arity + i_folder->net_env_delta()
        //  + nbrop_arity + i_nbrop->net_env_delta();
        extra_net = 0;
        //extra_max =
        //  max(extra_max,
        //      max(folder_arity + i_folder->max_env_delta(),
        //          nbrop_arity + i_nbrop->max_env_delta()));
        extra_max = 2;
      } else if (i->isA("FunctionCall")) {
    	extra_net = 0;
    	extra_max = 1;
      }
      maybe_set_env(i, baseh + extra_net, baseh + extra_max);
    } else {
      V4 << "Instruction " << ce2s(i) << " (env) could not be resolved yet."
         << endl;
    }
  }

 private:
  ProtoKernelEmitter *emitter_;

  // Utility.
  Block *
  compound_op_block(CompoundOp *cop)
  {
    map<CompoundOp *, Block *>::const_iterator iterator
      = emitter_->globalNameMap.find(cop);
    if (iterator == emitter_->globalNameMap.end())
      ierror("No instruction for compound operator!");
    return (*iterator).second;
  }
};

class InsertLetPops : public InstructionPropagator {
public:
  InsertLetPops(ProtoKernelEmitter* parent,Args* args) 
  { verbosity = parent->verbosity; }
  void print(ostream* out=0) { *out<<"InsertLetPops"; }

  void insert_pop_set(vector<iLET*> sources,Instruction* pointer) {
    // First, cluster the pops into branch-specific sets
    CEmap(Instruction*,CEset(iLET*)) dest_sets;
    for(int i=0;i<sources.size();i++) {
      // find the last reference to this source
      Instruction* last = NULL;
      V4 << "Source: " << ce2s(sources[i]) << endl;
      V4 << "  Usages: " << sources[i]->usages.size() << endl;
      for_set(Instruction*,sources[i]->usages,j) {
    	V4 << "Usage Instruction: " << ce2s(*j) << endl;
        if((*j)->marked("~Last~Reference")) { last = *j; break; }
      }
      if(last==NULL) {
    	  ierror("Trying to pop a let without its last usage marked");
      }
      // is it marked as being in a branch?
      V5 << "Considering last reference: "<<ce2s(last)<<endl;
      if(last->marked("~Branch~End")) {
        CE* inst
          = dynamic_cast<CEAttr &>(*last->attributes["~Branch~End"]).value;
        V5 << "Branch end ref: "<<ce2s(inst)<<endl;
        dest_sets[&dynamic_cast<Instruction &>(*inst)].insert(sources[i]);
      } else {
        V5 << "Default location: "<<ce2s(pointer)<<endl;
        dest_sets[pointer].insert(sources[i]);
      }
    }
    
    // Next, place each set
    for_map(Instruction*, CEset(iLET*), dest_sets, i) {
      // create the pop instruction
      PopLet* pop;
      if(i->second.size()<=MAX_LET_OPS) {
        //pop = new Instruction(POP_LET_OP+(i->second.size()));
    	  pop = new PopLet(POP_LET_OP+(i->second.size()));
      } else {
        //pop = new Instruction(POP_LET_OP); pop->padd(i->second.size());
    	  pop = new PopLet();
    	  pop->padd(i->second.size());
      }
      pop->env_delta = -i->second.size();
      // tag it onto the lets in the set
      for_set(iLET*,i->second,j) {
    	  (*j)->pop = pop;
    	  pop->addDebugIndex((*j)->debugIndex);
      }
      // and place the pop at the destination
      string type = (i->first==pointer)?"standard":"branch";
      V3 << "Inserting "+type+" " + ce2s(pop)+" after "<<ce2s(i->first)<<endl;
      chain_insert(i->first,pop);
    }
  }
  
  void act(Instruction* i) {
    if(i->isA("iLET")) {
      iLET* l = &dynamic_cast<iLET &>(*i);
      if(l->pop!=NULL) return; // don't do it when pops are resolved
      vector<iLET*> sources;
      sources.push_back(l);
      V2 << "Considering a LET " << l->debugIndex;
      vector<set<Instruction*, CompilationElement_cmp> > usages;
      set<Instruction*, CompilationElement_cmp>::iterator it;
      usages.push_back(l->usages); //1 per src
      V2 << "l->Usages size: " << l->usages.size() << endl;
      Instruction* pointer = l->next;
      stack<Instruction*> block_nesting;
      bool foldReferenceFound = true;
      if (i->marked("~Fold-Reference")) {
    	  foldReferenceFound = false;
      }

      // Ignore read references, they'll be used earlier in the program (in a function)
      //  The pop is handled by the function call (the funcall is a usage also)
      std::set<Instruction*> readRefs;
      for(int in=0; in<usages.size(); ++in) {
    	  V3 << "Usages[" << in << "] size: " << usages[in].size() << endl;
    	  for ( it=usages[in].begin() ; it != usages[in].end(); it++ ) {
    		 if ((*it)->marked("~Read-Reference")) {
    			 V3 << " " << "READ REFERENCE" << endl;
    			 readRefs.insert(*it);
    		 }
    	     V3 << " " << ce2s(*it) << endl;
    	  }
          std::set<Instruction*>::iterator it2;
          for (it2=readRefs.begin(); it2 != readRefs.end(); it2++) {
    	    usages[in].erase(*it2);
          }
          readRefs.clear();
      }

      V3 << "Fold Reference: " << i->marked("~Fold-Reference") << endl;
      while(!usages.empty() || !foldReferenceFound) {
        V3 << ". (Sources: " << sources.size() << " Usages: " << usages.size() << ") ";
        while(sources.size()>usages.size()) sources.pop_back(); // cleanup...
        if(pointer==NULL) {
          if(block_nesting.empty()) {
        	  V2 << "Instruction pointer: " << ce2s(pointer) << endl;
        	  ierror("Couldn't find all usages of let");
          }
          V3 << "^";
          pointer=block_nesting.top()->next; block_nesting.pop(); continue;
        }
        V3 << "Pointer: " << ce2s(pointer) << endl;
        if(pointer->isA("Block")) { // search for references in subs
          V3 << "v";
          block_nesting.push(pointer);
          pointer = dynamic_cast<Block &>(*pointer).contents;
          continue;
        } else if(pointer->isA("iLET")) { // add subs in
          iLET* sub = &dynamic_cast<iLET &>(*pointer);

          V3 << "\n Adding sub LET " << sub->debugIndex;
          sources.push_back(sub);
          V3 << "Sources size: " << sources.size() << endl;
          usages.push_back(sub->usages);
        } else if (pointer->isA("Reference")) { // it's somebody's reference?
          Reference* r = &dynamic_cast<Reference &>(*pointer);
          if(r->store->isA("Global")) {
        	  V3 << "\n is a global reference, ignoring" << endl;
        	  pointer=pointer->next;
        	  continue;
          }
          V3 << "\n Found reference...";
          for(int j=0;j<usages.size();j++) {
            if(usages[j].count(pointer)) { 
              V4 << "Erasing: " << ce2s(pointer) << endl;
              usages[j].erase(pointer);
              // mark last references for later use in pop insertion
              if(!usages[j].size()) {
            	  V4 << "  Marking LastReference" << endl;
            	  pointer->mark("~Last~Reference");
              }
              break;
            }
          }
          // trim any empty usages on top of the stack
          while(usages.size() && usages[usages.size()-1].empty()) {
            V3 << "\n Popping a LET " << l->debugIndex;
            usages.pop_back();
          }
        } else if (pointer->isA("Fold")) {
        	V3 << "\n\t is a FOLD" << endl;
            if (i->marked("~Fold-Reference")) {
        	  V3 << "\n Found folder...";
        	  for(int j=0;j<usages.size();j++) {
        	     if(usages[j].count(pointer)) {
        	      V4 << "Erasing: " << ce2s(pointer) << endl;
        	      usages[j].erase(pointer);
        	      // mark last references for later use in pop insertion
        	      if(!usages[j].size()) {
        	         V4 << "  Marking LastReference" << endl;
        	         pointer->mark("~Last~Reference");
        	      } else {
        	    	 V4 << usages[j].size() << " more usages left for let " << j << endl;
        	      }
        	     }
        	  }

              // trim any empty usages on top of the stack
              while(usages.size() && usages[usages.size()-1].empty())  {
                V3 << "\n Popping a Usage";
                usages.pop_back();
              }
              foldReferenceFound = true;
            }
        } else if (pointer->isA("FunctionCall")) {
          V3 << "\n Found FunctionCall usage...";
          for(int j=0;j<usages.size();j++) {
        	if(usages[j].count(pointer)) {
        	  V4 << "Erasing: " << ce2s(pointer) << endl;
        	  usages[j].erase(pointer);
        	  // mark last references for later use in pop insertion
        	  if(!usages[j].size()) pointer->mark("~Last~Reference");
        	    break;
        	  }
           }
           // trim any empty usages on top of the stack
           while(usages.size() && usages[usages.size()-1].empty()) {
        	 V3 << "\n Popping a Usage";
        	 usages.pop_back();
          }
        } else if (pointer->op == RET_OP) {

        }
        if(!usages.empty() || (foldReferenceFound == false && i->marked("~Fold-Reference"))) pointer=pointer->next;
      }
      // Now walk through and pop all the sources, clumping by destination
      V3 << "\n Adding set of pops, size: "<<sources.size()<<"\n";
      insert_pop_set(sources,pointer);
      V3 << "Completed LET resolution\n";
    }
  }
};

class DeleteNulls : public InstructionPropagator {
public:
  DeleteNulls(ProtoKernelEmitter* parent,Args* args) 
  { verbosity = parent->verbosity; }
  void print(ostream* out=0) { *out<<"DeleteNulls"; }
  void act(Instruction* i) {
    if(i->isA("NoInstruction")) {
      V2 << "Deleting NoInstruction placeholder\n";
      chain_delete(i,i);
    }
  }
};

class ResolveISizes : public InstructionPropagator {
public:
  ResolveISizes(ProtoKernelEmitter* parent,Args* args) 
  { verbosity = parent->verbosity; }
  void print(ostream* out=0) { *out<<"ResolveISizes"; }
  void act(Instruction* i) {
    if(i->isA("iDEF_FUN")) {
      iDEF_FUN *df = &dynamic_cast<iDEF_FUN &>(*i);
      bool ok=true; int size=1; // return's size
      Instruction* j = df->next;
      while(j!=df->ret) {
        if(j->size()>=0) size += j->size(); else ok=false;
        j=j->next;  if(!j) ierror("DEF_FUN_OP can't find matching RET_OP");
      }
      if(size>0 && df->fun_size != size) {
        V2 << "Fun size is " << size << endl;
        df->fun_size=size; df->parameters.clear();
        // now adjust the op
        if(df->fun_size>1 && df->fun_size<=MAX_DEF_FUN_OPS) 
          df->op=(DEF_FUN_2_OP+(df->fun_size-2));
        else
          { df->op = DEF_FUN_OP; df->padd(df->fun_size); }
        note_change(i);
      }
    }
    if(i->isA("Reference")) {
      Reference* r = &dynamic_cast<Reference &>(*i);
      if(r->offset==-1 && r->store->isA("Global") && 
         dynamic_cast<Global &>(*r->store).index >= 0) {
        V2<<"Global index to "<<ce2s(r->store)<<" is "<<
          dynamic_cast<Global &>(*r->store).index << endl;
        r->set_offset(dynamic_cast<Global &>(*r->store).index);
        note_change(i);
      }
    } 
    if(i->isA("Branch")) {
      Branch* b = &dynamic_cast<Branch &>(*i);
      Instruction* target = b->after_this;
      V5<<"Sizing branch: "<<ce2s(b)<<" over "<<ce2s(target)<<endl;
      if(b->start_location()>=0 && target->start_location()>=0) {
        int diff = target->next_location() - b->next_location();
        if(diff!=b->offset) {
          V2<<"Branch offset to follow "<<ce2s(target)<<" is "<<diff<<endl;
          b->set_offset(diff); note_change(i);
        }
      }
    }
  }
};

class ResolveLocations : public InstructionPropagator {
public:
  // Note: a possible concern: it's theoretically possible
  // to end up in an infinite loop when location is being nudged
  // such that reference lengths change, which could change op size
  // and therefore location...
  ProtoKernelEmitter* emitter;
  ResolveLocations(ProtoKernelEmitter* parent,Args* args) { 
     verbosity = parent->verbosity; 
     emitter = parent;
  }
  void print(ostream* out=0) { *out<<"ResolveLocations"; }
  void maybe_set_location(Instruction* i, int l) {
    if(i->start_location() != l) { 
      V4 << "Setting location of "<<ce2s(i)<<" to "<<l<<endl;
      i->set_location(l); note_change(i);
    } 
  }
  void maybe_set_index(Instruction* i, int l) {
    g_max = max(g_max, l + 1);
    if(dynamic_cast<Global &>(*i).index != l) {
      V4 << "Setting index of "<<ce2s(i)<<" to "<<l<<endl;
      dynamic_cast<Global &>(*i).index = l;
      note_change(i);
    }
  }
  void maybe_set_reference(Reference* i,int index) {
    if(i->offset!=index) { 
      V4 << "Setting reference offset "<<ce2s(i)<<" to "<<index<<endl;
      i->set_offset(index); note_change(i);
    }
  }
  void act(Instruction* i) {
    // Base case: set to zero or block-start
    if(!i->prev)
      maybe_set_location(i,(i->container?i->container->start_location():0));
    // Otherwise get it from previous instruction
    if(i->prev && i->prev->next_location()>=0)
      { maybe_set_location(i,i->prev->next_location()); }
    if(i->isA("Global")) {
      Instruction *ptr = i->prev; // find previous global...
      while (ptr && !ptr->isA("Global"))
        ptr = ptr->prev;
      if (ptr) {
        Global *g_prev = &dynamic_cast<Global &>(*ptr);
        ptr->dependents.insert(i); // make sure we'll get triggered when it sets
        if(g_prev->index!=-1) maybe_set_index(i,g_prev->index+1);
      } else { maybe_set_index(i,0); }
    }
    //if we can resolve the function call to its global index
    if(i->isA("FunctionCall")) {
       CompoundOp *compoundOp = dynamic_cast<FunctionCall &>(*i).compoundOp;
       map<CompoundOp *, Block *>::const_iterator iterator
         = emitter->globalNameMap.find(compoundOp);
       if (iterator != emitter->globalNameMap.end()) {
          Instruction *fnstart = (*iterator).second->contents;
          int index = -1;
          if (fnstart && fnstart->isA("Global"))
             index = dynamic_cast<Global &>(*fnstart).index;
          if(index >= 0 && i->prev && i->prev->isA("Reference"))
            maybe_set_reference(&dynamic_cast<Reference &>(*i->prev), index);
       }
    }
  }
  int g_max; // highest global index seen
  void preprop() { g_max = 0; }
  void postprop() {
    iDEF_VM* dv = &dynamic_cast<iDEF_VM &>(*root);
    if(dv->n_globals!=g_max) {
      V3 << "Setting global max from "<<dv->n_globals<<" to "<<g_max<<endl;
      dv->n_globals=g_max; any_changes|=true;
    }
  }
};

// Counts states and exports, and compute stack deltas.

class ResolveState : public InstructionPropagator {
 public:
  ResolveState(ProtoKernelEmitter *parent, Args *args)
    : emitter_(parent)
  { verbosity = parent->verbosity; }

  void print(ostream *out = 0) { *out << "ResolveState"; }

  void preprop(void)
  {
    resolved_ = true;
    n_states_ = n_exports_ = export_len_ = 0;
  }

  void
  postprop(void)
  {
    if (!resolved_)
      return;

    iDEF_VM *dv = &dynamic_cast<iDEF_VM &>(*root);
    dv->n_states = n_states_;
    dv->n_exports = n_exports_;
    dv->export_len = export_len_;
  }

  void
  act(Instruction *instruction)
  {
    if (instruction->isA("Fold")) {
      Fold *fold = &dynamic_cast<Fold &>(*instruction);
      fold->index = n_exports_++;
      export_len_ += 1;   // FIXME: Add up the tuple lengths.
    }  else if (instruction->isA("InitFeedback")) {
       	InitFeedback *initf = &dynamic_cast<InitFeedback &>(*instruction);
       	initf->set_index(n_states_++);
    } else if (instruction->isA("Feedback")) {
       	Feedback *fb = &dynamic_cast<Feedback &>(*instruction);
    	InitFeedback *initf = fb->matchingInit;
    	if (initf != NULL) {
    		fb->set_index(initf->index);
    	} else {
    		ierror("Unable to find matching InitFeedback for Feedback Op");
    	}
    }
  }

 private:
  ProtoKernelEmitter *emitter_;
  bool resolved_;
  unsigned int n_states_;
  unsigned int n_exports_;
  unsigned int export_len_;
};

class CheckResolution : public InstructionPropagator {
public:
  CheckResolution(ProtoKernelEmitter* parent) { verbosity = parent->verbosity; }
  void print(ostream* out=0) { *out<<"CheckResolution"; }
  void act(Instruction* i) { 
    if(!i->resolved()) {
       print_chain(chain_start(i), cpout);
       ierror("Instruction resolution failed for "+i->to_str());
    }
  }
};


// Ensure that all stores in references are non-null
class ResolveForwardReferences : public InstructionPropagator {
public:
  ProtoKernelEmitter* emitter;
  ResolveForwardReferences(ProtoKernelEmitter* parent) 
  { verbosity = parent->verbosity;  emitter = parent;}
  void print(ostream* out=0) { *out<<"ResolveForwardReferences"; }
  void act(Instruction* i) {
    if(i->isA("Reference")) {
      Reference* r = (Reference*)i;
      V4 << "Checking reference store for " << ce2s(r) << endl;
      if(r->store==NULL) {
        V2 << "Checking for target of " << ce2s(r) << " in global store...\n";
        map<CompoundOp *, Block *>::const_iterator iterator
          = emitter->globalNameMap.find(r->target);
        if (iterator == emitter->globalNameMap.end()) {
          ierror("Cannot resolve forward reference " + ce2s(r));
        } else {
          r->store = (*iterator).second->contents; r->classify_reference();
          V2 << "Resolved forward reference to " << ce2s(r->store) << endl;
        }
      }
    }
  }
};


/*****************************************************************************
 *  STATIC/LOCAL TYPE CHECKER                                                *
 *****************************************************************************/
// ensures that all fields are local and concrete
class Emittable {
 public:
  // concreteness of fields comes from their types
  static bool acceptable(Field* f) { return acceptable(f->range); }
  // concreteness of types
  static bool acceptable(ProtoType* t) {
    if(t->isA("ProtoNumber")) { return true;
    } else if(t->isA("ProtoSymbol")) { return true;
    } else if(t->isA("ProtoTuple")) {
      ProtoTuple* tp = &dynamic_cast<ProtoTuple &>(*t);
      if(!tp->bounded) return false;
      for(int i=0;i<tp->types.size();i++)
        if(!acceptable(tp->types[i])) return false;
      return true;
    } else if(t->isA("ProtoLambda")) {
      ProtoLambda* tl = &dynamic_cast<ProtoLambda &>(*t);
      return acceptable(tl->op);
    } else return false; // all others, including ProtoField
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


class CheckEmittableType : public IRPropagator {
 public:
  CheckEmittableType(ProtoKernelEmitter* parent) : IRPropagator(true,false)
  { verbosity = parent->verbosity; }
  virtual void print(ostream* out=0) { *out << "CheckEmittableType"; }
  virtual void act(Field* f) {
    if(!Emittable::acceptable(f->range))
      ierror(f,"Type is not resolved to emittable form: "+f->to_str());
  }
};


class ReferenceToParameter : public IRPropagator {
 public:
  ReferenceToParameter(ProtoKernelEmitter* parent, Args* args)
     : IRPropagator(false,true,false) { 
    verbosity = args->extract_switch("--reference-to-parameter-verbosity") ? 
    args->pop_int() : parent->verbosity;
  }
  virtual void print(ostream* out=0) { *out << "ReferenceToParameter"; }
  virtual void act(OperatorInstance* oi) {
     AM* current_am = oi->domain();

     // Only act on non-branch reference operators
     if(oi->op == Env::core_op("reference") 
        && !current_am->marked("branch-fn")) {

        // 1) alter CompoundOp by adding a parameter 
        CompoundOp* cop = current_am->bodyOf;
        V3 << "CompoundOp " << ce2s(cop) << endl;
        int num_params = cop->signature->n_fixed();
        string fn_name = cop->name;
        // 1a) add an input to the function signature
        cop->signature->required_inputs.insert(
           cop->signature->required_inputs.begin(),
           oi->nth_input(0));
        V3 << "Cop->signature " << ce2s(cop->signature) << endl;
        // 1b) foreach funcall, insert an input (input to [reference])
        OIset srcs = root->funcalls[cop];
        for_set(OI*,srcs,i) {
           OI* op_inst = (*i);
           V3 << "Inserting reference input on:" << ce2s(op_inst) << endl;
           op_inst->insert_input(op_inst->inputs.begin(),
                                 oi->inputs[0]);
           V3 << "Op instance is now:" << ce2s(op_inst) << endl;
        }
        // TODO: 1c) ensure that # inputs/outputs and their types still match

        // 2) convert Reference into Parameter
        V3 << "Converting " << ce2s(oi->op) 
           << " to " << "__"+fn_name+"_"+i2s(num_params)+"__"
           << " of " << ce2s(current_am->bodyOf) 
           << " as " << ce2s(oi->nth_input(0))
           << endl;
        // 2a) add a parameter to the CompoundOp
        Field* param = root->add_parameter(cop,
                                           "__"+fn_name+"_"+i2s(num_params)+"__", 
                                           num_params,
                                           current_am,
                                           oi->nth_input(0));
        // 2b) relocate the [reference] consumers to the new parameter
        root->relocate_consumers(oi->output,param);
        // 2c) delete the old reference OI
        root->delete_node(oi);
     }
  }
};

class PrimitiveToCompound : public IRPropagator {
 public:
  PrimitiveToCompound(ProtoKernelEmitter *parent, Args *args)
    : IRPropagator(false, true) {
    verbosity = args->extract_switch("--primitive-to-compound-verbosity") ?
      args->pop_int() : parent->verbosity;
  }

  void print(ostream *out = 0) { *out << "PrimitiveToCompound"; }

  void
  act(OperatorInstance *oi)
  {
    // Check for a literal whose value is a protolambda whose operator
    // is a primitive.
    if (!oi->op->isA("Literal"))
      return;

    Literal *literal = &dynamic_cast<Literal &>(*oi->op);
    if (!literal->value->isA("ProtoLambda"))
      return;

    ProtoLambda *lambda = &dynamic_cast<ProtoLambda &>(*literal->value);
    if (!lambda->op->isA("Primitive"))
      return;

    // Fabricate a compound operator that invokes the primitive with
    // the parameters it was given.
    Operator *primitive_op = lambda->op;
    string name = "Compound~of~"+primitive_op->name;
    CompoundOp *compound_op = new CompoundOp(oi, root,name);
    AmorphousMedium *am = compound_op->body;
    compound_op->signature = primitive_op->signature;
    OperatorInstance *poi = new OperatorInstance(oi, primitive_op, am);

    size_t n = primitive_op->signature->required_inputs.size();

    // MONSTROUS KLUDGE ALERT -- FIXME: This nonsense turns an n-ary
    // primitive FOO into (lambda (x y) (FOO x y)).  This is what you
    // want in (min-hood x) = (fold-hood min id x) = (fold-hood
    // (lambda (x y) (min x y)) id x), but it's obviously wrong in
    // general.  Can we APPLY?
    if ((n == 0) && (primitive_op->signature->rest_input != 0)) {
      compound_op->signature = new Signature(primitive_op->signature);
      vector<ProtoType *> *inputs
        = &compound_op->signature->required_inputs;
      inputs->push_back(primitive_op->signature->rest_input);
      inputs->push_back(primitive_op->signature->rest_input);
      n = 2;
    }

    for (size_t i = 0; i < n; i += 1) {
      string name = primitive_op->name + "~" + i2s(i);
      poi->add_input(root->add_parameter(compound_op, name, i, am, oi));
    }

    // Replace the lambda's primitive operator by the newly
    // constructed compound operator.
    lambda->op = compound_op;
  }

  // We have added new compound operators and amorphous media, so we
  // need to repeat determination of the relevant ones in order to
  // coax the emitter into emitting them.
  void postprop(void) { root->determine_relevant(); }
};

/*****************************************************************************
 *  EMITTER PROPER                                                           *
 *****************************************************************************/

Instruction *
ProtoKernelEmitter::literal_to_instruction(ProtoType *literal, OI *context)
{
  if (literal->isA("ProtoScalar"))
    return scalar_literal_instruction(&dynamic_cast<ProtoScalar &>(*literal));
  else if (literal->isA("ProtoTuple"))
    return
      tuple_literal_instruction(&dynamic_cast<ProtoTuple &>(*literal),
          context);
  else if (literal->isA("ProtoLambda"))
	 return lambda_literal_instruction(&dynamic_cast<ProtoLambda &>(*literal), context);
  else
    ierror("Don't know how to emit literal: " + literal->to_str());
}

static bool
float_representable_as_uint_p(float value)
{
  return
    ((0 <= value)
     && (value <= 0xffffffff)
     && (value == static_cast<float>(static_cast<unsigned int>(value))));
}

Instruction *
ProtoKernelEmitter::scalar_literal_instruction(ProtoScalar *scalar)
{
  float value = scalar->value;
  if (float_representable_as_uint_p(value))
    return integer_literal_instruction(static_cast<unsigned int>(value));
  else
    return float_literal_instruction(value);
}

Instruction *
ProtoKernelEmitter::integer_literal_instruction(unsigned int value)
{
  Instruction *i;
  if (value < MAX_LIT_OPS) {
    i = new Instruction(LIT_0_OP + value);
  } else {
    i = new Instruction(LIT_OP);
    i->padd(value);
  }
  return i;
}

Instruction *
ProtoKernelEmitter::float_literal_instruction(float value)
{
  union { float f; uint8_t b[4]; } u;
  u.f = value;
  Instruction *i = new Instruction(LIT_FLO_OP);
  i->padd8(u.b[0]); i->padd8(u.b[1]); i->padd8(u.b[2]); i->padd8(u.b[3]);
  return i;
}

Instruction *
ProtoKernelEmitter::tuple_literal_instruction(ProtoTuple *tuple, OI *context)
{
  if (!tuple->bounded)
    ierror("Cannot emit unbounded literal tuple: " + tuple->to_str());

  if (tuple->types.size() == 0)
    return new Instruction(NUL_TUP_OP);

  // Declare a global tup initialized to the right values.
  Instruction *definition = 0;
  for (size_t i = 0; i < tuple->types.size(); i++)
    chain_i(&definition, literal_to_instruction(tuple->types[i], context));
  chain_i(&definition, new iDEF_TUP(tuple->types.size(), true));

  // Add it to the global program.
  chain_i(&end, chain_start(definition));

  // Make a global reference to it.
  return new Reference(definition, context);
}

Instruction *
ProtoKernelEmitter::lambda_literal_instruction(ProtoLambda *lambda,
    OI *context)
{
  // Branch lambdas are handled with a special case on the branch primitive.
  bool is_branch = true;
  for_set(Consumer, context->output->consumers, i)
    if (i->first->op != Env::core_op("branch")) {
      is_branch = false;
      break;
    }
  if (is_branch) {
    return new NoInstruction();
  } else {
    if (!lambda->op->isA("CompoundOp"))
      ierror("Non-compound operator in lambda: " + lambda->to_str());
    CompoundOp *cop = &dynamic_cast<CompoundOp &>(*lambda->op);
    map<CompoundOp *, Block *>::const_iterator iterator
      = globalNameMap.find(cop);
    if (iterator == globalNameMap.end()) {
      V3 << "Lambda has undefined operator: " << lambda->to_str() << endl;
      // create a forward reference
      return new Reference(cop, context);
    }
    Instruction *target = (*iterator).second->contents;
    return new Reference(target, context);
  }
}

Instruction *
ProtoKernelEmitter::parameter_to_instruction(Parameter *p)
{
  return new Instruction(REF_0_OP + p->index);
}

// Add a tuple to the global declarations, then reference it in vector op i.

Instruction *
ProtoKernelEmitter::vec_op_store(ProtoType *type)
{
  ProtoTuple *tuple_type = &dynamic_cast<ProtoTuple &>(*type);
  return chain_i(&end, new iDEF_TUP(tuple_type->types.size()));
}

// Takes an OperatorInstance rather than just the primitive because
// sometimes the operator type depends on it.

Instruction *
ProtoKernelEmitter::primitive_to_instruction(OperatorInstance *oi)
{
  Primitive *primitive = &dynamic_cast<Primitive &>(*oi->op);

  if (primitive2op.count(primitive->name))
    return standard_primitive_instruction(oi);
  else if (sv_ops.count(primitive->name))
    return vector_primitive_instruction(oi);
  else if (fold_ops.count(primitive->name))
    return fold_primitive_instruction(oi);
  else if (primitive->name == "dchange")
	  return init_feedback_instruction(oi);
  else if (primitive->name == "store")
	  return let_instruction(oi);
  else if (primitive->name == "read")
	  return ref_instruction(oi);
  else if (primitive->name == "/") // FIXME: Env::core_op?
    return divide_primitive_instruction(oi);
  else if (primitive->name == "tup") // FIXME: Env::core_op?
    return tuple_primitive_instruction(oi);
  else if (primitive == Env::core_op("branch")) // FIXME: primitive->name?
    return branch_primitive_instruction(oi);
  else if (primitive == Env::core_op("reference")) // FIXME: primitive->name?
    // Reference already created by input.
    return new NoInstruction();
  else
    ierror("Don't know how to convert op to instruction: "
        + primitive->to_str());
}

Instruction *
ProtoKernelEmitter::standard_primitive_instruction(OperatorInstance *oi)
{
  Primitive *p = &dynamic_cast<Primitive &>(*oi->op);
  ProtoType *output_type = oi->output->range;

  // MOV_OP is an exception, in that it just peeks at the vector, rather
  // than actually computing an placing a new vector.
  if (output_type->isA("ProtoTuple") && primitive2op[p->name]!=MOV_OP)
    return new Reference(primitive2op[p->name], vec_op_store(output_type), oi);
  else
    return new Instruction(primitive2op[p->name]);
}

Instruction *
ProtoKernelEmitter::vector_primitive_instruction(OperatorInstance *oi)
{
  Primitive *p = &dynamic_cast<Primitive &>(*oi->op);
  ProtoType *output_type = oi->output->range;

  // Operator switch happens if *any* input is non-scalar.
  bool output_tuple_p = output_type->isA("ProtoTuple");
  bool tuple_p = output_tuple_p;
  if (!tuple_p)
    for (size_t i = 0; i < oi->inputs.size(); i++)
      if (oi->inputs[i]->range->isA("ProtoTuple")) {
        tuple_p = true;
        break;
      }
  OPCODE opcode = (tuple_p ? sv_ops[p->name].second : sv_ops[p->name].first);

  // Now add ops: possible multiple if n-ary.
  size_t n_copies = (p->signature->rest_input ? (oi->inputs.size() - 1) : 1);
  Instruction *chain = 0;
  for (size_t i = 0; i < n_copies; i++)
    chain_i(&chain,
        ((output_tuple_p && !(p->name == "max" || p->name == "min"))
         ? new Reference(opcode, vec_op_store(output_type), oi)
         : new Instruction(opcode)));

  return chain_start(chain);
}

// FIXME: Reduce code duplicated in vector_primitive_instruction.

// Mark operator instances as they are emitted, to ensure they are
// not emitted multiple times
void ensure_one_emission(OI* oi) {
  if(oi->marked("emission_log")) {
    if(oi->op->name != "read") // reads may be emitted multiple times
      ierror("Duplicate emission of "+ce2s(oi));
    //compile_warn("Duplicate emission of "+ce2s(oi));
  } else { 
    oi->mark("emission_log");
  }
}

static CompoundOp *
fold_operand_cop(OperatorInstance *oi, size_t i)
{
  Field *field = oi->inputs[i];
  ProtoType *type = field->range;
  if (!type->isA("ProtoLambda"))
    ierror("Fold operand is not a lambda!");
  Operator *op = L_VAL(type);
  if (!op->isA("CompoundOp"))
    ierror("Lambda operator is not compound!");
  return &dynamic_cast<CompoundOp &>(*op);
}

Instruction *
ProtoKernelEmitter::fold_primitive_instruction(OperatorInstance *oi)
{
  Primitive *p = &dynamic_cast<Primitive &>(*oi->op);

  //if (oi->output->range->isA("ProtoTuple"))
  //  ierror("Fold can't handle output tuples yet!");

  OPCODE opcode = (*fold_ops.find(p->name)).second.first;
  CompoundOp *folder = fold_operand_cop(oi, 0);
  CompoundOp *nbrop = fold_operand_cop(oi, 1);
  Fold* newf = new Fold(opcode, folder, nbrop);
  set<Instruction*> flets = letsForFolders[oi];
  if (!flets.empty()) {
    set<Instruction*>::iterator flit;
    for (flit = flets.begin(); flit != flets.end(); flit++) {
      Instruction* ins = *flit;
   	  iLET* fi = &dynamic_cast<iLET &>(*ins);
	  fi->usages.insert(newf);
	  V3 << "Adding " << ce2s(newf) << " as usage for " << ce2s(fi) << endl;
    }
  } else {
	  V3 << "No lets found for " << ce2s(oi) << endl;
  }
  return newf;
}

Instruction *
ProtoKernelEmitter::init_feedback_instruction(OperatorInstance *oi)
{
  V3 << "Creating new Init_Feedback_OP" << ce2s(oi) << endl;
  // Find the mux
  OperatorInstance *mux;
  for_set(Consumer,oi->output->consumers,c) {
	  if (c->first->op->name == "mux") {
		  mux = c->first;
	  }
  }
  if (mux == NULL) {
	  cout << "ERROR: Null MUX as dchange consumer" << endl;
  }
  Instruction *chain = 0;
  CompoundOp *init = fold_operand_cop(mux, 1);
  CompoundOp *update = fold_operand_cop(mux, 2);
  // mark that these have been used
  ensure_one_emission(mux->inputs[1]->producer);
  ensure_one_emission(mux->inputs[2]->producer);

  V4 << "Init function: " << ce2s(init) << endl;
  V4 << "Update function: " << ce2s(update) << endl;

  InitFeedback* initFeedback = new InitFeedback(INIT_FEEDBACK_OP, init, update);
  dchangeMap[oi] = initFeedback;
  //lastInitFeedback = initFeedback;

  Block* initFunctionBlock = globalNameMap[init];
  // if the function is not defined yet (e.g., recursion), add a placeholder
  if(!initFunctionBlock) globalNameMap[init] = new Block(new iDEF_FUN(init));

  // get global ref to DEF_FUN
  Global* def_fun_init = &dynamic_cast<Global &>(*globalNameMap[init]->contents);

  // add GLO_REF, then init_feedback
  chain_i(&chain,new Reference(def_fun_init, oi));
  chain_i(&chain,initFeedback);
  iLET* lAfter = new iLET();
  // FIXME: I think this won't handle multivariable updates correctly?
  chain_i(&chain,lAfter);
  // Put the cur value of the variable on the env stack for the update function
  initFeedback->letAfter = lAfter;
  chain_i(&chain, new Reference(lAfter, oi));
  // Put implicit arguments for the update function (from references) onto the stack
  OI *updateOI = mux->inputs[2]->producer;
  for(int i=0;i<updateOI->inputs.size();i++) // only implicits linked to the lambda literal
    chain_i(&chain,tree2instructions(updateOI->inputs[i])); // push each implicit arg onto the stack
  // And stick the value back on the stack for the feedback storage
  chain_i(&chain, new Reference(lAfter, oi));

  // Fix the read if necessary
  std::set<Instruction*> readRefs = dchangeReadMap[oi];
  std::set<Instruction*>::iterator rrit;
  for (rrit=readRefs.begin(); rrit != readRefs.end(); rrit++) {
	  Instruction* ins = *rrit;
	  Reference* rRef = &dynamic_cast<Reference &>(*ins);
	  V4 << "Moved Read Reference to " << ce2s(lAfter) << endl;
	  rRef->moveStore(lAfter);
  }

  // Call the Update function after init-feedback
  Block* updateFunctionBlock = globalNameMap[update];
  // if the function is not defined yet (e.g., recursion), add a placeholder
  if(!updateFunctionBlock) globalNameMap[update] = new Block(new iDEF_FUN(update));

  // get global ref to DEF_FUN
  Global* def_fun_update = &dynamic_cast<Global &>(*globalNameMap[update]->contents);

  // add GLO_REF, then funcall
  chain_i(&chain,new Reference(def_fun_update, oi));
  FunctionCall *funccall = new FunctionCall(update);
  lAfter->usages.insert(funccall);
  chain_i(&chain,funccall);
  return chain_start(chain);
}

Instruction *
ProtoKernelEmitter::let_instruction(OperatorInstance *oi)
{
	V2 << "Store: " << ce2s(oi) << endl;
	// Get the dchange from this store.  Look back through the inputs
	// Store should have one input
	OperatorInstance *dchange;
	if (oi->inputs.size() == 1) {
		dchange = find_dchange(oi->inputs[0]->producer);
	} else {
		dchange = find_dchange(oi);
	}
	Instruction *chain = 0;
	Feedback* fb = new Feedback(FEEDBACK_OP);
	if (dchange != NULL) {
		Instruction* ins = dchangeMap[dchange];
		InitFeedback *initF = &dynamic_cast<InitFeedback &>(*ins);
		fb->matchingInit = initF;
	}
	chain_i(&chain, fb);
	return chain_start(chain);
}

OperatorInstance *
ProtoKernelEmitter::find_dchange(OperatorInstance *oi)
{
	if (oi->op->name == "dchange") {
		return oi;
	}
	for(int i=0;i<oi->inputs.size();i++) {
		if (oi->inputs[i]->producer->op->name == "dchange") {
			return oi->inputs[i]->producer;
		}
	}
	// backtrack?  When would this happen?  Should be dchange -> mux -> store
	for(int i=0;i<oi->inputs.size();i++) {
		OperatorInstance *dchange = find_dchange(oi->inputs[i]->producer);
		if (dchange != NULL) {
			return dchange;
		}
	}
	return NULL;
}

extern std::map<OI*, OI*> readToStoreMap;

Instruction *
ProtoKernelEmitter::ref_instruction(OperatorInstance *oi)
{
	V2 << "Read: " << ce2s(oi) << endl;
	// Assuming the update op is a one parameter function
	OI* store = readToStoreMap[oi];
	if (store != NULL) {
	  V2 << "Found Store in map: " << ce2s(store) << endl;
	} else {
	  V2 << "Unable to locate Store for this READ!" << endl;
	  return new Instruction(REF_0_OP);
	}
	// Find the dchange for this store
	OperatorInstance *dchange;
    if (store->inputs.size() == 1) {
		dchange = find_dchange(store->inputs[0]->producer);
	} else {
		dchange = find_dchange(store);
	}

    if (dchange != NULL) {
      Instruction* ins = dchangeMap[dchange];
      // We likely haven't resolved the initFeedback yet, so it might not be in the dchangeMap yet
      // If so, we'll have to fix this later when we create the initfeedback
      if (ins != NULL) {
        InitFeedback *initF = &dynamic_cast<InitFeedback &>(*ins);
        Instruction* iLet = initF->letAfter;
        V2 << "Found matching InitFeedback, iLet is " << ce2s(iLet) << endl;
        Reference *readRef = new Reference(iLet, oi);
        readRef->mark("~Read-Reference");
        return readRef;
      } else {
    	  // Dummy Store for now
    	 V2 << "Temporarily putting a dummy iLet as the store for the read reference" << endl;
    	 iLET* dummyiLet = new DummyiLET();
       	 Reference *readRef = new Reference(dummyiLet, oi);
    	 // Is there only one read for a dchange? Think so.  If not, can make this a set
       	 dchangeReadMap[dchange].insert(readRef);
         readRef->mark("~Read-Reference");
    	 return readRef;
      }
    } else {
    	V2 << "Unable to locate dchange for Store: " << ce2s(store) << endl;
    }
}


// Division is handled specially until VDIV is implemented.

Instruction *
ProtoKernelEmitter::divide_primitive_instruction(OperatorInstance *oi)
{
  ProtoType *output_type = oi->output->range;
  Instruction *chain = 0;

  // Multiply the divisors together.
  for (size_t i = 0; i < (oi->inputs.size() - 2); i++)
    chain_i(&chain, new Instruction(MUL_OP));

  if (!output_type->isA("ProtoTuple")) {
    chain_i(&chain, new Instruction(DIV_OP));
  } else {
    // Multiply by 1/divisor.
    chain_i(&chain, new Instruction(LET_2_OP, 2));
    chain_i(&chain, new Instruction(LIT_1_OP));
    chain_i(&chain, new Instruction(REF_0_OP));
    chain_i(&chain, new Instruction(DIV_OP));
    chain_i(&chain, new Instruction(REF_1_OP));
    chain_i(&chain, new Reference(VMUL_OP, vec_op_store(output_type), oi));
    chain_i(&chain, new Instruction(POP_LET_2_OP, -2));
  }

  return chain_start(chain);
}

Instruction *
ProtoKernelEmitter::tuple_primitive_instruction(OperatorInstance *oi)
{
  Instruction *i = new Reference(TUP_OP, vec_op_store(oi->output->range), oi);
  i->stack_delta = (1 - oi->inputs.size());
  i->padd(oi->inputs.size());
  return i;
}

static void mark_branch_references(Branch *, Block *, AM *);

Instruction *
ProtoKernelEmitter::branch_primitive_instruction(OperatorInstance *oi)
{
  Instruction *chain = 0;

  // Dump lambdas into branches.
  CompoundOp *t_cop, *f_cop;
  t_cop = &dynamic_cast<CompoundOp &>(*L_VAL(oi->inputs[1]->range));
  f_cop = &dynamic_cast<CompoundOp &>(*L_VAL(oi->inputs[2]->range));
  Instruction *t_br = dfg2instructions(t_cop->body);
  Instruction *f_br = dfg2instructions(f_cop->body);

  // Trim off DEF & RETURN.
  t_br = t_br->next;
  f_br = f_br->next;
  chain_delete(chain_start(t_br), chain_start(t_br));
  chain_delete(chain_end(t_br), chain_end(t_br));
  chain_delete(chain_start(f_br), chain_start(f_br));
  chain_delete(chain_end(f_br), chain_end(f_br));
  t_br = new Block(t_br);
  f_br = new Block(f_br);

  // Pull references for branch contents.
  OIset handled_frags;
  for_map(OI *, CE *, fragments, i) {
    // FIXME: Need descriptive names here.
    OI *other_oi = i->first;
    if (other_oi->output->domain == oi->output->domain) {
      Instruction *instruction = &dynamic_cast<Instruction &>(*i->second);
      Instruction *ptr = chain_start(instruction);
      chain_i(&chain, ptr);
      handled_frags.insert(other_oi);
    }
  }

  for_set(OI *, handled_frags, i)
    fragments.erase(*i);

  // String them together into a branch.
  Branch *jmp = new Branch(t_br, true);
  Block *t_block = &dynamic_cast<Block &>(*t_br);
  Block *f_block = &dynamic_cast<Block &>(*f_br);
  mark_branch_references(jmp, t_block, oi->output->domain);
  mark_branch_references(jmp, f_block, oi->output->domain);
  chain_i(&chain, new Branch(jmp));
  chain_i(&chain, f_br);
  chain_i(&chain, jmp);
  chain_i(&chain, t_br);
  return chain_start(chain);
}

static void
mark_branch_references(Branch *jmp, Block *branch, AM *source)
{
  for (Instruction *ptr = branch->contents; ptr != 0; ptr = ptr->next) {
    // Mark references that aren't already handled by an interior branch.
    if (ptr->isA("Reference")) {
      CEAttr *attr = &dynamic_cast<CEAttr &>(*ptr->attributes["~Ref~Target"]);
      OI *target = &dynamic_cast<OI &>(*attr->value);
      if (target->output->domain == source) {
        if (ptr->attributes.count("~Branch~End"))
          ierror("Tried to duplicate mark reference");
        //cout << "  Marking reference for " << ce2s(source) << endl;
        //cout << "    " << ce2s(jmp->after_this) << endl;
        ptr->attributes["~Branch~End"] = new CEAttr(jmp->after_this);
      }
    } else if (ptr->isA("Block")) {
      mark_branch_references(jmp, &dynamic_cast<Block &>(*ptr), source);
    }
  }
}

// Load a .ops file named `name', containing a single list of the form
//
//   ((<opcode-name0> <stack-delta0> [<primitive-name0>])
//    (<opcode-name1> <stack-delta1> [<primitive-name1>])
//    ...)

void
ProtoKernelEmitter::load_ops(const string &name)
{
  SExpr *sexpr;

  {
    // FIXME: Why find in path?
    scoped_ptr<ifstream> stream(parent->proto_path.find_in_path(name));
    if (stream == 0) {
      compile_error("Can't open op file: " + name);
      return;
    }

    sexpr = read_sexpr(name, stream.get());
  }

  if (sexpr == 0) {
    compile_error("Can't read op file: " + name);
    return;
  }

  if (!sexpr->isList()) {
    compile_error(sexpr, "Op file not a list");
    return;
  }

  SE_List &list = dynamic_cast<SE_List &>(*sexpr);
  for (int i = 0; i < list.len(); i++) {
    if (!list[i]->isList()) {
      compile_error(list[i], "Op not a list");
      continue;
    }

    SE_List &op = dynamic_cast<SE_List &>(*list[i]);
    if ((!op[0]->isSymbol())
        || ((op.len() == 3) ? (!op[2]->isSymbol()) : (op.len() != 2))) {
      compile_error(&op, "Op not formatted (name stack-delta [primitive])");
      continue;
    }

    opnames[i] = dynamic_cast<SE_Symbol &>(*op[0]).name;

    if (op[1]->isScalar()) {
      op_stackdeltas[i]
        = static_cast<int>(dynamic_cast<SE_Scalar &>(*op[1]).value);
    } else if (op[1]->isSymbol()
               && dynamic_cast<SE_Symbol &>(*op[1]).name == "variable") {
      // Give variable stack deltas a fixed bogus number.
      op_stackdeltas[i] = 7734;
    } else {
      compile_error(op[1], "Invalid stack delta");
      continue;
    }

    if (op.len() == 3)
      primitive2op[dynamic_cast<SE_Symbol &>(*op[2]).name] = i;
  }

  // Now add the special-case ops.
  sv_ops["+"] = make_pair(ADD_OP, VADD_OP);
  sv_ops["-"] = make_pair(SUB_OP, VSUB_OP);
  sv_ops["*"] = make_pair(MUL_OP, VMUL_OP);
  //sv_ops["/"] = make_pair(DIV_OP, VDIV_OP);
  sv_ops["<"] = make_pair(LT_OP, VLT_OP);
  sv_ops["<="] = make_pair(LTE_OP, VLTE_OP);
  sv_ops[">"] = make_pair(GT_OP, VGT_OP);
  sv_ops[">="] = make_pair(GTE_OP, VGTE_OP);
  sv_ops["="] = make_pair(EQ_OP, VEQ_OP);
  sv_ops["max"] = make_pair(MAX_OP, VMAX_OP);
  sv_ops["min"] = make_pair(MIN_OP, VMIN_OP);
  sv_ops["mux"] = make_pair(MUX_OP, VMUX_OP);

  fold_ops["fold-hood"] = make_pair(FOLD_HOOD_OP, VFOLD_HOOD_OP);
  fold_ops["fold-hood-plus"] = make_pair(FOLD_HOOD_PLUS_OP, VFOLD_HOOD_PLUS_OP);
}

// Load a .proto file named `name', containing not Proto code but
// Paleo-style op extensions of the form
//
//   (defop <opcode> <primitive> <argument-type>*)

void
ProtoKernelEmitter::load_extension_ops(const string &filename)
{
  SExpr *sexpr;

  {
    // FIXME: Why not find in path?
    ifstream stream(filename.c_str());
    if (!stream.good()) {
      compile_error("Can't open extension op file: " + filename);
      return;
    }

    sexpr = read_sexpr(filename, &stream);
  }

  if (sexpr == 0) {
    compile_error("Can't read extension op file: " + filename);
    return;
  }

  process_extension_ops(sexpr);
}

// Do the same, but from a string in memory rather than from a file.

void
ProtoKernelEmitter::setDefops(const string &defops)
{
  SExpr *sexpr = read_sexpr("defops", defops);

  if (sexpr == 0) {
    compile_error("Can't read defops: " + defops);
    return;
  }

  process_extension_ops(sexpr);
}

void
ProtoKernelEmitter::process_extension_ops(SExpr *sexpr)
{
  if (!sexpr->isList()) {
    compile_error(sexpr, "Op extension file not a list");
    return;
  }

  SE_List &list = dynamic_cast<SE_List &>(*sexpr);

  if (!list[0]->isSymbol()) {
    compile_error(sexpr, "Invalid op extension file");
    return;
  }

  if (*list[0] == "all")
    for (int i = 1; i < list.len(); i++)
      process_extension_op(list[i]);
  else
    process_extension_op(sexpr);
}

static ProtoType *
parse_paleotype(SExpr *sexpr)
{
  if (sexpr->isList()) {
    SE_List &list = dynamic_cast<SE_List &>(*sexpr);
    if (! (list.len() == 2 && *list[0] == "vector" && *list[1] == 3)) {
      compile_error(sexpr, "Invalid compound paleotype");
      return 0;
    }
    return new ProtoVector;
  } else if (sexpr->isSymbol()) {
    SE_Symbol &symbol = dynamic_cast<SE_Symbol &>(*sexpr);
    if (symbol == "scalar") {
      return new ProtoScalar;
    } else if (symbol == "boolean") {
      return new ProtoBoolean;
    } else {
      compile_error(sexpr, "Unknown primitive type: " + symbol.name);
      return 0;
    }
  } else {
    return 0;
  }
}

void
ProtoKernelEmitter::process_extension_op(SExpr *sexpr)
{
  if (!sexpr->isList()) {
    compile_error(sexpr, "Invalid extension op");
    return;
  }

  SE_List &list = dynamic_cast<SE_List &>(*sexpr);
  if (! (*list.op() == "defop")) {
    compile_error(sexpr, "Invalid extension op");
    return;
  }

  if (list.len() < 4) {
    compile_error(sexpr, "defop has too few arguments");
    return;
  }

  int opcode;
  SExpr &opspec = *list[1];
  if (opspec.isSymbol()) {
    SE_Symbol &symbol = dynamic_cast<SE_Symbol &>(opspec);
    if (symbol == "?") {
      opcode = opnames.size();
    } else if (primitive2op.count(symbol.name)) {
      opcode = primitive2op[symbol.name];
    } else {
      compile_error(sexpr, "unknown opcode: " + symbol.name);
      return;
    }
  } else if (opspec.isScalar()) {
    SE_Scalar &scalar = dynamic_cast<SE_Scalar &>(opspec);
    opcode = static_cast<int>(scalar.value);
  } else {
    compile_error(sexpr, "defop op not symbol or number");
    return;
  }

  SExpr &name_sexpr = *list[2];
  if (!name_sexpr.isSymbol()) {
    compile_error(sexpr, "defop name not symbol");
    return;
  }
  const string &name = dynamic_cast<SE_Symbol &>(name_sexpr).name;

  scoped_ptr<Signature> signature(new Signature(sexpr));
  ProtoType *type;
  if (0 == (type = parse_paleotype(list[3])))
    return;
  signature->output = type;

  size_t nargs = list.len() - 4;
  for (size_t i = 4; i < list.len(); i++)
    if (0 != (type = parse_paleotype(list[i])))
      signature->required_inputs.push_back(type);
    else
      return;

  // FIXME: Kludge.  What's the right thing?
  string opname = name;
  for (size_t i = 0; i < opname.size(); i++) {
    if (isalpha(opname[i]))
      opname[i] = toupper(opname[i]);
    else if (opname[i] == '-')
      opname[i] = '_';
  }
  opname += "_OP";

  opnames[opcode] = opname;
  op_stackdeltas[opcode] = 1 - nargs;
  primitive2op[name] = opcode;

  parent->interpreter->toplevel->force_bind
    (name, new Primitive(sexpr, name, signature.release()));
}

// small hack for getting op debugging into low-level print functions
bool ProtoKernelEmitter::op_debug = true;

ProtoKernelEmitter::ProtoKernelEmitter(NeoCompiler *parent, Args *args)
{
  // Set global variables.
  this->parent = parent;
  is_dump_hex = args->extract_switch("--hexdump");
  print_compact = (args->extract_switch("--emit-compact") ? 2 :
                   (args->extract_switch("--emit-semicompact") ? 1 : 0));
  verbosity = args->extract_switch("--emitter-verbosity") ?
    args->pop_int() : parent->verbosity;
  max_loops=args->extract_switch("--emitter-max-loops") ? args->pop_int() : 10;
  paranoid = args->extract_switch("--emitter-paranoid") | parent->paranoid;
  op_debug = args->extract_switch("--emitter-op-debug");
  // Load operation definitions.
  load_ops("core.ops");
  terminate_on_error();
  // Setup pre-emitter rule collection.
  preemitter_rules.push_back(new ReferenceToParameter(this, args));
  preemitter_rules.push_back(new PrimitiveToCompound(this, args));
  // Setup rule collection.
  rules.push_back(new DeleteNulls(this, args));
  rules.push_back(new InsertLetPops(this, args));
  rules.push_back(new ResolveISizes(this, args));
  rules.push_back(new ResolveLocations(this, args));
  rules.push_back(new StackEnvSizer(this, args));
  rules.push_back(new ResolveState(this, args));
  // Program starts empty.
  start = end = NULL;
}

void
ProtoKernelEmitter::init_standalone(Args *args)
{
  // Load platform-specific and layer plugin opcode extensions.
  const string &platform
    = args->extract_switch("--platform") ? args->pop_next() : "sim";
  const string &platform_directory
    = ProtoPluginManager::PLATFORM_DIR + "/" + platform + "/";
  load_extension_ops(platform_directory + ProtoPluginManager::PLATFORM_OPFILE);
  while (args->extract_switch("-L", false)) {
    string layer_name = args->pop_next();
    ensure_extension(layer_name, ".proto");
    load_extension_ops(platform_directory + layer_name);
  }
}

// A Field needs a let if it has:
// 1. More than one consumer in the same function
// 2. A consumer inside a different relevant function
bool needs_let(Field* f) {
  int consumer_count = 0;
  for_set(Consumer,f->consumers,c) {
    if(c->first->output->domain == f->domain) { // same function consumer
      consumer_count++;
      if(consumer_count > 1) {
    	  return true; // 2+ => let needed
      }
    } else if(f->container->relevant.count(c->first->output->domain)) {
      return true; // found a function reference
    }
  }
  return false;
}

//
OI* needs_fold_lambda_let(Field* f) {
  int consumer_count = 0;
  for_set(Consumer,f->consumers,c) {
    if((c->first->output->domain == f->domain) && // same function consumer
       (c->first->op->isA("Literal"))) {
      Literal *literal = &dynamic_cast<Literal &>(*c->first->op);
      if (literal->value->isA("ProtoLambda")) {
        ProtoLambda *lambda = &dynamic_cast<ProtoLambda &>(*literal->value);
        // consumer is a lambda function
        Field *cOut = c->first->output;
        for_set(Consumer, cOut->consumers, cc) {
          if (cc->first->op->isA("Primitive")) {
        	string ccName = cc->first->op->name;
        	if ((ccName == "fold-hood") || ccName == "fold-hood-plus") {
        	  //TODO: Check the lambda function for the reference and mark it
        	  return cc->first;
        	}
          }
        }
      }
    }
  }
  return NULL;
}

/* this walks a DFG in order as follows: 
   - producer before consumers
   - consumers in order
*/
Instruction* ProtoKernelEmitter::tree2instructions(Field* f) {
  V5 << "Tree2Instructions for " << ce2s(f) << endl;
  if(memory.count(f))
    return
      new Reference(&dynamic_cast<Instruction &>(*memory[f]), f->producer);
  OperatorInstance* oi = f->producer; Instruction* chain = NULL;
  // ensure that emission happens precisely once per operator instance
  f->domain->mark("emission_log");
  ensure_one_emission(oi);

  V5 << "producer " << ce2s(oi) << endl;
  if (oi->op->name == "mux") {
    V3 << "MUX operator, checking if we're part of a feedback" << endl;
    if (oi->attributes.count("LETFED-MUX")) {
      // find the dchange input
      for(int i=0;i<oi->inputs.size();i++) {
    	if (oi->inputs[i]->producer->op->name == "dchange") {
     	  V3 << "Found dchange, returning dchange instructions" << endl;
   		  return tree2instructions(oi->inputs[i]);
   	    }
      }
      V3 << "No dchange input, handling this mux normally" << endl;
    }
  }
  // first, get all the inputs
  for(int i=0;i<oi->inputs.size();i++) {
    V4 << "Chaining " << ce2s(oi->inputs[i]) << endl;
    Instruction *ins = tree2instructions(oi->inputs[i]);
    chain_i(&chain,ins);
  }
  // second, add the operation
  if(oi->op==Env::core_op("reference")) {
    V4 << "Reference is: " << ce2s(oi->op) << endl;
    if(oi->inputs.size()!=1) ierror("Bad number of reference inputs");
    Instruction* frag = chain_split(chain);
    if(frag) fragments[oi->inputs[0]->producer] = frag;
  } else if(oi->op->isA("Primitive")) {
    V4 << "Primitive is: " << ce2s(oi->op) << endl;
    chain_i(&chain,primitive_to_instruction(oi));
    if(verbosity>=4) print_chain(chain,cpout,2);
  } else if(oi->op->isA("Literal")) { 
    V4 << "Literal is: " << ce2s(oi->op) << endl;
    Instruction *ins = literal_to_instruction(dynamic_cast<Literal &>(*oi->op).value, oi);
    chain_i(&chain, ins);
  } else if(oi->op->isA("Parameter")) { 
    V4 << "Parameter is: " << ce2s(oi->op) << endl;
    chain_i(&chain,
        parameter_to_instruction(&dynamic_cast<Parameter &>(*oi->op)));
  } else if(oi->op->isA("CompoundOp")) { 
    V4 << "Compound OP is: " << ce2s(oi->op) << endl;
    CompoundOp* cop = &dynamic_cast<CompoundOp &>(*oi->op);
    Block* functionBlock = globalNameMap[cop];
    // if the function is not defined yet (e.g., recursion), add a placeholder
    if(!functionBlock) globalNameMap[cop] = new Block(new iDEF_FUN(cop));
    // get global ref to DEF_FUN
    Global* def_fun_instr
      = &dynamic_cast<Global &>(*globalNameMap[cop]->contents);
    // add GLO_REF, then FUN_CALL
    chain_i(&chain,new Reference(def_fun_instr,oi)); 
    chain_i(&chain,new FunctionCall(cop));
  } else { // also CompoundOp, Parameter
    ierror("Don't know how to emit instruction for "+oi->op->to_str());
  }
  // finally, put the result in the appropriate location
  if(f->selectors.size()) {
	cout << "Restrictions not all compiled out for field: " << ce2s(f) << endl;
	AMset fsel = f->selectors;
	for_set(AM*,fsel,i) cout << "Selector AM: " << ce2s(*i) << endl;
    ierror("Restrictions not all compiled out for: "+f->to_str());
  }
  OI* flLet = needs_fold_lambda_let(f);
  if (flLet == NULL && f->producer->op->name == "read") {
     V4 << "Reads don't need a let here" << endl;
  } else if (flLet != NULL) {
	// need a let to contain the input to a lambda function used by a folder
	// This is the uncurry operation
    iLET *l = new iLET();
    l->mark("~Fold-Reference");
    letsForFolders[flLet].insert(l);
    V4 << "Adding " << ce2s(l) << " as let for folder " << ce2s(flLet) << endl;
    memory[f] = chain_i(&chain,l);
    V4 << "Needs Fold Lambda Let: " << ce2s(flLet) << endl;
    V4 << "F's producer op name is: " <<ce2s(f->producer->op) << endl;
    if(verbosity>=2) print_chain(start,cpout,2);
    // Don't need to add a reference, the lambda function should have one
    // Could check
  } else if(needs_let(f)) { // need a let to contain this
	memory[f] = chain_i(&chain, new iLET());
	  V4 << "Needs Let: " << ce2s(memory[f]) << endl;
    if(verbosity>=2) print_chain(start,cpout,2);
    chain_i(&chain, // and we got here by a ref...
        new Reference(&dynamic_cast<Instruction &>(*memory[f]), oi));
    if(verbosity>=2) print_chain(start,cpout,2);
  }

  return chain_start(chain);
}

bool has_relevant_consumer(Field* f) {
  for_set(Consumer,f->consumers,c) {
    if(c->first->output->domain == f->domain) { // same function consumer
      return true;
    } else if(f->container->relevant.count(c->first->output->domain)) {
      return true; // found a function reference
    }
  }
  return false;
}

Instruction* ProtoKernelEmitter::dfg2instructions(AM* root) {
  Fset minima, f; root->all_fields(&f);
  for_set(Field*,f,i) {
	  if(!has_relevant_consumer(*i))
		  minima.insert(*i);
  }
  
  string s=ce2s(root)+":"+i2s(minima.size())+" minima identified: ";
  for_set(Field*,minima,i) { if(!(i==minima.begin())) s+=","; s+=ce2s(*i); }
  V3 << s << endl;
  iDEF_FUN *fnstart = new iDEF_FUN(root->bodyOf);
  Instruction *chain=fnstart;
  for_set(Field*,minima,i) {
    Instruction *ins = tree2instructions(*i);
    chain_i(&chain, ins);
  }
  if(minima.size()>1) { // needs an all
    Instruction* all = new Instruction(ALL_OP);
    if(minima.size()>=256) ierror("Too many minima: "+minima.size());
    all->stack_delta = -(minima.size()-1); all->padd(minima.size());
    chain_i(&chain, all);
  }
  chain_i(&chain, fnstart->ret = new Instruction(RET_OP));
  if(root->bodyOf!=NULL) {
     V3 << "Adding " << ce2s(root->bodyOf) << " to globalNameMap" << endl;
     globalNameMap[root->bodyOf] = new Block(fnstart);
  }
  return fnstart;
}

string hexbyte(uint8_t v) {
  string hex = "0123456789ABCDEF";
  string out = "xx"; out[0]=hex[v>>4]; out[1]=hex[v & 0xf]; return out;
}

uint8_t* ProtoKernelEmitter::emit_from(DFG* g, int* len) {
  CheckEmittableType echecker(this); echecker.propagate(g);

  V1<<"Pre-linearization steps...\n";
  for(int i=0; i<preemitter_rules.size(); i++) {
     IRPropagator *propagator
       = &dynamic_cast<IRPropagator &>(*preemitter_rules[i]);
     propagator->propagate(g);
  }

  if(parent->is_dump_dotfiles) {
    ofstream dotstream((parent->dotstem+".pre-emission.dot").c_str());
    if(dotstream.is_open()) g->printdot(&dotstream,parent->is_dotfields);
  }

  V1<<"Linearizing DFG to instructions...\n";
  start = end = new iDEF_VM(); // start of every script

  // Output all functions in arbitrary order.  Forward refs are resolved later
  for_set(AM*,g->relevant,i) { // translate each function
    // If we've already done this one, skip
    if (!globalNameMap[(*i)->bodyOf]) {
      if((*i)!=g->output->domain && !(*i)->marked("branch-fn")) {
        Instruction *idf = dfg2instructions(*i);
        chain_i(&end,idf);
      }
    }
  }
  
  chain_i(&end,dfg2instructions(g->output->domain)); // next the main
  chain_i(&end,new Instruction(EXIT_OP)); // add the end op
  if(verbosity>=2) print_chain(start,cpout,2);

  // Attempt to resolve all lingering forward references
  ResolveForwardReferences resolver(this); resolver.propagate(start);

  // Check that we were able to linearize everything:
  if(fragments.size()) {
    ostringstream s; 
    Instruction *i = &dynamic_cast<Instruction &>(*fragments.begin()->second);
    print_chain(chain_start(i), &s, 2);
    ierror("Unplaced fragment: "+ce2s(fragments.begin()->first)+"\n"+s.str());
  }
  // Check that every function (AM) that's touched has all of its subsections
  // output as well
  bool badnodes = false;
  for_set(AM*,g->spaces,am) {
    if(!(*am)->marked("emission_log")) continue;
    OIset contents;
    (*am)->all_ois(&contents);
    for_set(OI*,contents,i) {
      if(!(*i)->marked("emission_log")) {
        badnodes=true; compile_error("Unplaced operator instance: "+ce2s(*i));
      }
    }
  }
  if(badnodes) ierror("Not all operator instances were emitted.");

  // Fill in all of the blanks
  V1<<"Resolving unknowns in instruction sequence...\n";
  for(int i=0;i<max_loops;i++) {
    bool changed=false;
    for(int j=0;j<rules.size();j++) {
      changed |= rules[j]->propagate(start); terminate_on_error();
    }
    if(!changed) break;
    if(i==(max_loops-1))
      compile_warn("Emitter analyzer giving up after "+i2s(max_loops)+" loops");
  }
  CheckResolution rchecker(this); rchecker.propagate(start);
  
  // finally, output
  V1<<"Outputting final instruction sequence...\n";
  *len= end->next_location();
  uint8_t *buf = static_cast<uint8_t *>(calloc(*len,sizeof(uint8_t)));
  start->output(buf);
  
  if(parent->is_dump_code) print_chain(start,cpout,print_compact);

  if(is_dump_hex) {
    for(int i=0;i<*len;i++) // dump lines of 25 bytes each (75 chars)
      { *cpout << hexbyte(buf[i]) << " "; if((i % 25)==24) *cpout << endl; } 
    if(*len % 25) *cpout << endl; // close final line if needed
  }
  
  return buf;
}

// How will the emitter work:
// There are template mappings for;
// Literal: -> LIT_k_OP, LIT8_OP, LIT16_OP, LIT_FLO_OP
// Primitive: 
// Compound Op:
// Parameter -> REF_k_OP, REF_OP
// Output -> RET_OP
// field w. multiple consumers -> LET_k_OP, LET_OP; end w. POP_LET_k_OP, POP_LET_OP

// What about...? REF, 
// GLO_REF is used for function calls...
// DEF_FUN, IF (restrict) emission

/* LINEARIZING:
   let's give each element a number in the order it's created in;
   then, we'll have the set be in that order, so there's a stable
   order in which non-ordered program fragments are executed
     i.e. (all (set-dt 3) (green 2) 7)
   We need to figure out how to linearize a walk through a branching
   sequence: i.e. (let ((x 5)) (set-dt x) (+ 1 x))

  We are guaranteed that it's a partially ordered graph.
  Requirement: label all nodes "i" s.t. i={1...n}, no repeats, 
    no input > output

  1. segment the program into independent computations; order by elt#
  2. in each independent computation, find set of bottoms & start w. min elt#
  3. walk tree to linearize: ins before outs, out in rising elt# order

 */
