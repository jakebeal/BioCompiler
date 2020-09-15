/* Optimizers
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#include "grn_optimizers.h"

GRNConstantEliminator::GRNConstantEliminator(GRNEmitter* parent, Args* args) {
  verbosity = args->extract_switch("--grn-constant-eliminator-verbosity") ? 
    args->pop_number() : parent->verbosity;
}

void GRNConstantEliminator::act(FunctionalUnit* fu) {
  // Delete FUs with low constant constitutive promotion and 
  // no positive regulatory region
  { bool low_promoter = false, has_activator = false;
    for(int i=0;i<fu->sequence.size();i++)
      if(fu->sequence[i]->isA("Promoter")) {
        low_promoter = is_low_promoter(((Promoter*)fu->sequence[i]));
      }
    
    for(int i=0;i<fu->sequence.size();i++) {
      for_set(ExpressionRegulation*,fu->sequence[i]->regulators,er) {
        ProtoType* ct = NULL;
        if((*er)->signal->attributes.count("type"))
          ct = ((ProtoTypeAttribute*)(*er)->signal->attributes["type"])->type;
        
        if((*er)->repressor==false || (ct==NULL || !ct->isA("ProtoBoolean"))) {
          has_activator=true;
        }
      }
    }
    
    // if it isn't a low promoter or has a regulator region, skip this
    if(low_promoter && !has_activator) {
      V2 << "Eliminating low constant sequence "<<fu->to_str()<<endl;
      note_change(fu);
      grn->delete_functional_unit(fu);
      return;
    }
  }
  
  // Delete FUs with an always-high repressor
  { bool constant_repressor = false;
    for(int i=0;i<fu->sequence.size();i++)
      for_set(ExpressionRegulation*,fu->sequence[i]->regulators,er) {
        ProtoType* pt = get_chemical_type((*er)->signal);
        if((*er)->repressor && is_constitutive_true(pt))
          constant_repressor = true;
      }
    
    if(constant_repressor) {
      V2<<"Eliminating constantly repressed sequence "<<fu->to_str()<<endl;
      note_change(fu);
      grn->delete_functional_unit(fu);
      return;
    }
  }
}



