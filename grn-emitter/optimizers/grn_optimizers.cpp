/* Optimizers
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#include "grn_optimizers.h"

void GRNEmitter::initialize_grn_optimizers(Args* args) {
  // set up rule collection
  if(!args->extract_switch("--no-grn-optimization")) {
    if(!args->extract_switch("--no-grn-double-negative-eliminator"))
      rules.push_back(new DoubleNegativeEliminator(this,args));
    if(!args->extract_switch("--no-grn-dead-code-eliminator"))
      rules.push_back(new GRNDeadCodeEliminator(this,args));
    if(!args->extract_switch("--no-grn-constant-eliminator"))
      rules.push_back(new GRNConstantEliminator(this,args));
    if(!args->extract_switch("--no-grn-chemical-type-inference"))
      rules.push_back(new GRNInferChemicalType(this,args));
    if(!args->extract_switch("--no-grn-copy-propagator"))
      rules.push_back(new GRNCopyPropagator(this,args));
    if(!args->extract_switch("--no-grn-output-consolidator"))
      rules.push_back(new OutputConsolidator(this,args));
    if(!args->extract_switch("--no-grn-merge-duplicate-inputs"))
      rules.push_back(new MergeDuplicateInputs(this,args));
    if(!args->extract_switch("--no-grn-nor-compressor"))
      rules.push_back(new NorCompressor(this,args));
    if(!args->extract_switch("--no-duplicate-reaction-consolidator"))
      rules.push_back(new DuplicateReactionConsolidator(this,args));
  }
  // post-processing rules
  if(args->extract_switch("--single-products-only"))
    postprocessing.push_back(new MultiProductEliminator(this,args));
}
