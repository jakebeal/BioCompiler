/* Proto optimizer Copyright (C) 2009, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

// The analyzer takes us from an initial interpretation to a concrete,
// optimized structure that's ready for compilation

#include "config.h"
#include "nicenames.h"
#include "type-inference.h"

#define DEBUG_FUNCTION(...) V5<<"In function " << __VA_ARGS__ << endl;

/*****************************************************************************
 *  TYPE CONSTRAINTS                                                         *
 *****************************************************************************/

SExpr* get_sexp(CE* src, string attribute) {
  Attribute* a = src->attributes[attribute];
  if(a==NULL || !a->isA("SExprAttribute"))
    return sexp_err(src,"Couldn't get expression for: "+a->to_str());
  return dynamic_cast<SExprAttribute &>(*a).exp;
}

/**
 * Return an unliteralized (maybe copy) of the type.
 * For example: deliteralize(<Scalar 2>) = <Scalar>.
 * Another example: deliteralize(<Scalar>) = <Scalar>.
 * The Deliteralization of a tuple is a tuple where each element has been
 * unliteralized.
 */
ProtoType* Deliteralization::deliteralize(ProtoType* base) {
  if(base->isA("ProtoVector")) {
    ProtoVector* t = &dynamic_cast<ProtoVector &>(*base);
    ProtoVector* newt = new ProtoVector(t->bounded);
    for(int i=0;i<t->types.size();i++)
      newt->types.push_back(deliteralize(t->types[i]));
    return newt;
  } else if(base->isA("ProtoTuple")) {
    ProtoTuple* t = &dynamic_cast<ProtoTuple &>(*base);
    ProtoTuple* newt = new ProtoTuple(t->bounded);
    for(int i=0;i<t->types.size();i++)
      newt->types.push_back(deliteralize(t->types[i]));
    newt = ProtoTuple::specialize_tuple_class(newt);
    return newt;
  } else if(base->isA("ProtoSymbol")) {
    ProtoSymbol* t = &dynamic_cast<ProtoSymbol &>(*base);
    return t->constant ? new ProtoSymbol() : t;
  } else if(base->isA("ProtoBoolean")) {
    ProtoBoolean* t = &dynamic_cast<ProtoBoolean &>(*base);
    return t->constant ? new ProtoBoolean() : t;
  } else if(base->isA("ProtoScalar")) {
    ProtoScalar* t = &dynamic_cast<ProtoScalar &>(*base);
    return t->constant ? new ProtoScalar() : t;
  } else if(base->isA("ProtoLambda")) {
    //TODO: This isnt the right place to do this
    //We want to propogate the output of a ProtoLambda, which would be the output value returned by the function
    //This isn't really "deliteralization" at all.
    ProtoLambda* t = &dynamic_cast<ProtoLambda &>(*base);
    return deliteralize(t->op->signature->output);
  } else if(base->isA("ProtoField")) {
    ProtoField* t = &dynamic_cast<ProtoField &>(*base);
    ProtoType* hood = deliteralize(t->hoodtype);
    if(!hood->isA("ProtoLocal"))
      ierror("Deliteralization of field Local isn't Local: "+ce2s(hood));
    ProtoLocal* lhood = &dynamic_cast<ProtoLocal &>(*hood);
    return new ProtoField(lhood);
  } else {
    return base;
  }
}

  /********** TYPE READING **********/
  ProtoType* TypeConstraintApplicator::get_op_return(Operator* op) {
    DEBUG_FUNCTION(__FUNCTION__);
    if(!op->isA("CompoundOp"))
      return type_err(op,"'return' used on non-compound operator:"+ce2s(op));
    return dynamic_cast<CompoundOp &>(*op).output->range;
  }

  ProtoTuple* TypeConstraintApplicator::get_all_args(OperatorInstance* oi) {
    DEBUG_FUNCTION(__FUNCTION__);
    ProtoTuple *tup = new ProtoTuple();
    for(int i=0;i<oi->inputs.size();i++) tup->add(oi->inputs[i]->range);
    tup = ProtoTuple::specialize_tuple_class(tup);
    return tup;
  }
  
  ProtoType* TypeConstraintApplicator::get_nth_arg(OperatorInstance* oi, int n) {
    DEBUG_FUNCTION(__FUNCTION__);
    if(n < oi->op->signature->required_inputs.size()) { // ordinary argument
      if(n<oi->inputs.size()) return oi->inputs[n]->range;
      return type_err(oi,"Can't find type"+i2s(n)+" in "+ce2s(oi));
    } else if(n < oi->op->signature->n_fixed()) {
      if(n<oi->inputs.size()) return oi->inputs[n]->range;
      ierror("Nth arg calls not handling non-asserted optional arguments yet");
    } else if(n==oi->op->signature->n_fixed() && oi->op->signature->rest_input) {// rest
      //return a tuple of remaining elements
      vector<ProtoType*> vec;
      for(int i=n;i<oi->inputs.size();i++) vec.push_back(oi->inputs[i]->range);
      ProtoTuple *t = tupleOrVector(vec);
      V4 << "Returning a tuple for &rest: " << ce2s(t) << endl;
      return t;
    }
    return type_err(oi,"Can't find input "+i2s(n)+" in "+ce2s(oi));
  }

  /**
   * Gets a reference for 'last'.  Expects a tuple.
   * For example: (last <3-Tuple<Scalar 0>,<Scalar 1>,<Scalar 2>>) = <Scalar 2>
   */
  ProtoType* TypeConstraintApplicator::get_ref_last(OperatorInstance* oi, SExpr* ref, SE_List_iter* li) {
    DEBUG_FUNCTION(__FUNCTION__);
    ProtoType* nextType = get_ref(oi,li->get_next("type"));
    if(!nextType->isA("ProtoTuple")) {
      ierror(ref,"Expected ProtoTuple, but got "+ce2s(nextType)); // temporary, to help test suite
      return type_err(ref,"Expected ProtoTuple, but got "+ce2s(nextType));
    }
    ProtoTuple* tup = &dynamic_cast<ProtoTuple &>(*nextType);
    return tup->types[tup->types.size()-1];
  }
  
  /**
   * Takes the Lowest-common-superclass of its arguments.
   * Operates on proto types and rest elements.
   * For example: (lcs <Scalar> <Scalar 2> <3-Vector>) = <Number>
   */
  ProtoType* TypeConstraintApplicator::get_ref_lcs(OperatorInstance* oi, SExpr* ref, SE_List_iter* li) {
    DEBUG_FUNCTION(__FUNCTION__);
    ProtoType* nextType = NULL;
    ProtoType* compound = NULL;
    //for each arg to lcs, e.g., (lcs arg0 arg1)
    while(li->has_next()) {
      SExpr* next = li->get_next("type");
      nextType = get_ref(oi,next);
      if(isRestElement(oi, next) && nextType->isA("ProtoTuple")) {
        ProtoTuple* tv = &dynamic_cast<ProtoTuple &>(*nextType);
        for(int i=0;i<tv->types.size();i++) {
          compound=(compound==NULL)? tv->types[i] : ProtoType::lcs(compound, tv->types[i]);
          V4 << "get_ref (rest) lcs oi=" << ce2s(oi)
             << ", nextType=" << ce2s(nextType)
             << ", next[" << i << "]=" << ce2s(tv->types[i])
             << ", compound=" << ce2s(compound) <<endl;
        }
      } else {
        compound=(compound==NULL) ? nextType : ProtoType::lcs(compound, nextType);
        V4 << "get_ref lcs oi=" << ce2s(oi)
           << ", nextType=" << ce2s(nextType)
           << ", compound=" << ce2s(compound) << endl;
      }
    }
    return compound;
  }
  
  /**
   * Returns the nth type of a tuple.
   * For example: (nth <3-Tuple<Scalar 0>,<Scalar 1>,<Scalar 2>>, 1) = <Scalar 1>
   */
  ProtoType* TypeConstraintApplicator::get_ref_nth(OperatorInstance* oi, SExpr* ref, SE_List_iter* li) {
    DEBUG_FUNCTION(__FUNCTION__);
    //get tuple
    ProtoType* nextType = get_ref(oi,li->get_next("type"));
    ProtoTuple* tup = NULL;
    if(nextType->isA("ProtoTuple"))
       tup = &dynamic_cast<ProtoTuple &>(*nextType);

    //get index
    ProtoType* indexType;
    if(li->peek_next()->isScalar()) indexType = new ProtoScalar(li->get_num());
    else indexType = get_ref(oi,li->get_next("type"));
    ProtoScalar* index = NULL;
    if( indexType->isA("ProtoScalar") )
       index = &dynamic_cast<ProtoScalar &>(*indexType);

    //get result if possible
    if(index != NULL && tup != NULL && tup->types.size() > index->value) {
      V4<<"- tup[index] = " << ce2s(tup->types[index->value]) << endl;
      return tup->types[index->value];
    } else if (tup != NULL) {
      V4<<"- can't get the nth type yet... trying LCS"<< endl;
      ProtoType* ret = NULL;  
      if(tup->types.size() > 0)
        ret = tup->types[0];
      for(int i=1; i<tup->types.size() && ret != NULL; i++) {
        ret = ProtoType::lcs(ret,tup->types[i]);
      }
      if(ret == NULL)
        return new ProtoType();
      return ret;
    } 
    return new ProtoType();
  }

  /**
   * If all elements of tuple are <Scalar>, then it returns a ProtoVector,
   * otherwise it returns tuple.
   */
  ProtoTuple* TypeConstraintApplicator::tupleOrVector(vector<ProtoType*> types) {
    ProtoTuple* ret = new ProtoTuple(true);
    for(int i=0; i<types.size(); i++) {
      ret->add(types[i]);
    }
    return ProtoTuple::specialize_tuple_class(ret);
  }
  
  /**
   * Returns a tuple of its arguments or a vector if all arguments are scalars.
   * For example: (tupof <Scalar 1> <Tuple<Any>...>) = <2-Tuple <Scalar 1>,<Tuple<Any>...>>
   * For example: (tupof <Scalar 1> <Scalar 2>) = <2-Vector <Scalar 1>,<Scalar 2>>
   */
  ProtoType* TypeConstraintApplicator::get_ref_tupof(OperatorInstance* oi, SExpr* ref, SE_List_iter* li) {
    DEBUG_FUNCTION(__FUNCTION__);
    string tupstr = "get_ref tupof ";
    ProtoType* reftype = NULL;
    vector<ProtoType*> types;
    while( li->has_next() ) {
      SExpr* expr = li->get_next("type");
      reftype = get_ref(oi,expr);
      if(isRestElement(oi,expr) && reftype->isA("ProtoTuple")) {
        ProtoTuple* tup = T_TYPE(reftype);
        for(int i=0; i<tup->types.size(); i++) {
          tupstr += ce2s(tup->types[i]) + ", ";
          types.push_back(tup->types[i]);
        }
      } else {
        tupstr += ce2s(reftype) + ", ";
        types.push_back(reftype);
      }
    }
    V4 << tupstr << endl;
    return tupleOrVector(types);
  }
  
  /**
   * Returns a field of its arguments.
   * For example: (fieldof <Scalar 1>) = <Field <Scalar 1>>
   * Expects input to be a <Local>.  Otherwise returns the <Field> directly.
   */
  ProtoType* TypeConstraintApplicator::get_ref_fieldof(OperatorInstance* oi, SExpr* ref, SE_List_iter* li) {
    DEBUG_FUNCTION(__FUNCTION__);
    ProtoType* hoodtype = get_ref(oi,li->get_next("type"));
    if(!hoodtype->isA("ProtoLocal"))
      return NULL;
    ProtoLocal* lhood = &dynamic_cast<ProtoLocal &>(*hoodtype);
    return new ProtoField(lhood);
  }
  
  /**
   * Returns the deliteralized value of its argument.
   * For example: (unlit <Scalar 1>) = <Scalar>
   */
  ProtoType* TypeConstraintApplicator::get_ref_unlit(OperatorInstance* oi, SExpr* ref, SE_List_iter* li) {
    DEBUG_FUNCTION(__FUNCTION__);
    // get the argument
    ProtoType* reftype = get_ref(oi,li->get_next("type"));
    // can have at most 1 argument
    if(li->has_next()) {
      SExpr* unexpected = li->get_next("type");
      ierror(ref,"Unexpected extra argument "+ce2s(unexpected)); // temporary, to help test suite
      return type_err(ref,"Unexpected extra argument "+ce2s(unexpected));
    }
    V4<<"get_ref unlit " << ce2s(reftype) <<endl;
    return Deliteralization::deliteralize(reftype);
  }
  
  /**
   * Returns the field-type of its input.
   * For example: (ft <Field <Scalar 1>>) = <Scalar 1>
   * Expects a field as input. 
   * Can coerce a ProtoField from a ProtoLocal e.g.,
   * (ft <Scalar 1>) = <Scalar 1>
   */
  ProtoType* TypeConstraintApplicator::get_ref_ft(OperatorInstance* oi, SExpr* ref, SE_List_iter* li) {
    DEBUG_FUNCTION(__FUNCTION__);
    ProtoType* reftype = get_ref(oi,li->get_next("type"));
    if(!reftype->isA("ProtoField")) {
      if(reftype->isA("ProtoLocal")) {
        V3<<"Coercing a ProtoField for {"<<ce2s(ref)<<"} of {"<<ce2s(oi)<<"}"<<endl;
        return reftype;
      } else if(reftype->type_of()=="ProtoType") { // top-level type
        return reftype;
      }
      // should never get here
      ierror(ref,"Unhandled ProtoField ref case: "+ce2s(reftype)); // temporary, to help test suite
    }
    ProtoField* field = &dynamic_cast<ProtoField &>(*reftype);
    return field->hoodtype;
  }

  /**
   * Returns the input types of its argument's signature.
   * For example: 
   *   (def foo (scalar vector) number)
   *   (inputs <Lambda foo>) = <2-Tuple <Scalar>,<Vector>>
   */
  ProtoType* TypeConstraintApplicator::get_ref_inputs(OperatorInstance* oi, SExpr* ref, SE_List_iter* li) {
    DEBUG_FUNCTION(__FUNCTION__);
    ProtoType* reftype = get_ref(oi,li->get_next("type"));
    if(!reftype->isA("ProtoLambda")) { 
      ierror(ref,"Expected ProtoLambda, but got "+ce2s(reftype)); // temporary, to help test suite
      return type_err(ref,"Expected ProtoLambda, but got "+ce2s(reftype));
    }
    ProtoLambda* lambda = &dynamic_cast<ProtoLambda &>(*reftype);
    Signature* sig = lambda->op->signature;
    // we only know the length of the inputs if there's no &rest
    ProtoTuple* ret = new ProtoTuple(!sig->rest_input);
    for(int i=0; i<sig->n_fixed(); i++) {
      ProtoType* elt = ProtoType::clone(sig->nth_type(i));
      ret->add(elt);
    }
    if(sig->rest_input!=NULL) // rest is unbounded end to tuple
      ret->add(sig->rest_input);
    ret = ProtoTuple::specialize_tuple_class(ret);
    return ret;
  }

  /**
   * Returns the input types of its argument's signature.
   * For example: 
   *   (def foo (scalar vector) number)
   *   (output <Lambda foo>) = <Number>
   */
  ProtoType* TypeConstraintApplicator::get_ref_output(OperatorInstance* oi, SExpr* ref, SE_List_iter* li) {
    DEBUG_FUNCTION(__FUNCTION__);
    ProtoType* reftype = get_ref(oi,li->get_next("type"));
    if(!reftype->isA("ProtoLambda")) { 
      ierror(ref,"Expected ProtoLambda, but got "+ce2s(reftype)); // temporary, to help test suite
      return type_err(ref,"Expected ProtoLambda, but got "+ce2s(reftype));
    }
    ProtoLambda* lambda = &dynamic_cast<ProtoLambda &>(*reftype);
    return lambda->op->signature->output;
  }

  /**
   * Gets the reference type for any 'list' type (any non-symbol)
   */
  ProtoType* TypeConstraintApplicator::get_ref_list(OperatorInstance* oi, SExpr* ref) {
    DEBUG_FUNCTION(__FUNCTION__);
    SE_List_iter* li = new SE_List_iter(ref);
    // "fieldof": field containing a local
    if(li->on_token("fieldof"))
       return get_ref_fieldof(oi,ref,li);
    // "ft": type of local contained by a field
    if(li->on_token("ft"))
       return get_ref_ft(oi,ref,li);
    // "inputs": the types of a function's input signature
    if(li->on_token("inputs"))
       return get_ref_inputs(oi,ref,li);
    // "last": find the last type in a tuple
    if(li->on_token("last"))
       return get_ref_last(oi,ref,li);
    // "lcs": finds the least common superclass of a set of types
    if(li->on_token("lcs")) 
       return get_ref_lcs(oi,ref,li);
    // "nth": finds the nth type in a tuple
    if(li->on_token("nth")) 
       return get_ref_nth(oi,ref,li);
    // "outputs": the type of a function's output
    if(li->on_token("output"))
       return get_ref_output(oi,ref,li);
    // "tupof": tuple w. a set of types as arguments
    if(li->on_token("tupof")) 
       return get_ref_tupof(oi,ref,li);
    // "unlit": generalize away literal values
    if(li->on_token("unlit")) 
       return get_ref_unlit(oi,ref,li);
  }
  
  /**
   * Return the best available information on the referred type
   */
  ProtoType* TypeConstraintApplicator::get_ref(OperatorInstance* oi, SExpr* ref) {
    V4<<"=========================="<< endl;
    DEBUG_FUNCTION(__FUNCTION__);
    V4<<"get_ref on " << ce2s(ref) << " from " << ce2s(oi) << endl;
    V4<<"Getting type reference for "<<ce2s(ref)<<endl;
    if(ref->isSymbol()) {
      // Named input/output argument:
      int n = oi->op->signature->parameter_id(&dynamic_cast<SE_Symbol &>(*ref),
          false);
      V4 << "n : " << n << endl;
      if(n>=0) {
        ProtoType* ret = get_nth_arg(oi,n);
        if (oi->attributes.count("LETFED-MUX")) {
          if (ret->isA("ProtoLambda")) {
            ProtoLambda* lambda = &dynamic_cast<ProtoLambda &>(*ret);
        	if (lambda->op->isA("CompoundOp")) {
    	      ProtoType* rtype = dynamic_cast<CompoundOp &>(*lambda->op).output->range;
    		  ret = rtype;
    	    }
          }
        }
    	V4 << "nth arg: " << ce2s(ret) << endl;
    	return ret;
      }
      if(n==-1) {
    	  V4 << "oi->output->range " << ce2s(oi->output->range) << endl;
    	  ProtoType *ret = oi->output->range;
    	  if(oi->attributes.count("LETFED-MUX")) {
    		if (oi->output->range->isA("ProtoLambda")) {
    	      ProtoLambda* lambda = &dynamic_cast<ProtoLambda &>(*oi->output->range);
    	      if (lambda->op->isA("CompoundOp")) {
    	        ProtoType* rtype = dynamic_cast<CompoundOp &>(*lambda->op).output->range;
    	  	    ret = rtype;
    	      }
    	    }
    	  }
    	  return ret;
      }
      // "args": a tuple of argument types
      if(*ref=="args") return get_all_args(oi);
      // return: the value of a compound operator's output field
      if(*ref=="return") return get_op_return(oi->op);
    } else if(ref->isList()) {
      ProtoType* ret = get_ref_list(oi,ref);
      V4<<"get_ref returns: " << ce2s(ret) << endl;
      return ret;
    }
    ierror(ref,"Unknown type reference: "+ce2s(ref)); // temporary, to help test suite
    return type_err(ref,"Unknown type reference: "+ce2s(ref));
  }

/********** TYPE ASSERTION **********/
bool TypeConstraintApplicator::assert_range(Field* f,ProtoType* range) {
  // Assertion only narrows types, it does not widen them:
  if(f->range->supertype_of(range)) { return parent->maybe_set_range(f,range); }
  else { return false; }
}

  /**
   * Returns true if n is the &rest element of oi
   */
  bool isNthArgRest(OI* oi, int n) {
    return (n==oi->op->signature->n_fixed() && oi->op->signature->rest_input);
  }
  
  /**
   * Asserts that the nth argument of oi is value.
   */
  bool TypeConstraintApplicator::assert_nth_arg(OperatorInstance* oi, int n, ProtoType* value) {
    DEBUG_FUNCTION(__FUNCTION__);
    if(n >= 0 && n <= oi->inputs.size()) {
      if(isNthArgRest(oi,n) && value->isA("ProtoTuple")) {
        ProtoTuple* inputTup = new ProtoTuple(true);
        // add all the inputs to a tuple
        for(int i=oi->op->signature->n_fixed(); i<oi->inputs.size(); i++) {
          inputTup->add(oi->inputs[i]->range);
        }
        V4<<"assert_nth_arg (rest): oi["<<n<<"]= "<<ce2s(inputTup)<<", val="<<ce2s(value)<<endl;
        // for all the inputs, change their type if necessary
        ProtoTuple* valueTup = T_TYPE(value);
        bool retVal = false;
        for(int i=0; i<inputTup->types.size(); i++) {
           ProtoType* valType = NULL;
           // if valueTup is unbounded, use the last element for all remaining
           if(i>=valueTup->types.size()) {
             valType = valueTup->types[valueTup->types.size()-1];
           } else {
             valType = valueTup->types[i];
           }
           int which_input = i+oi->op->signature->n_fixed();
           retVal |= assert_range(oi->inputs[which_input],valType);
        }
        return retVal;
      }
      V4<<"assert_nth_arg: oi["<<n<<"]= "<<ce2s(oi->inputs[n]->range)<<", val="<<ce2s(value)<<endl;
      if(n<oi->inputs.size())
        return assert_range(oi->inputs[n],value);
    }
    ierror("Could not find argument "+i2s(n)+" of "+ce2s(oi));
  }

  /**
   * Asserts that all the arguments of oi are value.
   * @WARNING not implemented
   */
  bool TypeConstraintApplicator::assert_all_args(OperatorInstance* oi, ProtoType* value) {
    DEBUG_FUNCTION(__FUNCTION__);
    ierror("Not yet implemented");
    return false;
  }

  /**
   * Asserts that output oi is value.
   * @WARNING not implemented
   */
  bool TypeConstraintApplicator::assert_op_return(Operator* f, ProtoType* value) {
    DEBUG_FUNCTION(__FUNCTION__);
    ierror("Not yet implemented");
    return false;
  }
  
  /**
   * Asserts that (fieldof ref) = value.
   * Expects ref to resolve to a ProtoLocal.
   */
  bool TypeConstraintApplicator::assert_on_field(OperatorInstance* oi, SExpr* ref, ProtoType* value) {
    DEBUG_FUNCTION(__FUNCTION__);
    if(!value->isA("ProtoField"))
      ierror("'fieldof' assertion on non-field type: "+ref->to_str());
    return assert_ref(oi,ref,F_VAL(value));
  }
  
  /**
   * Asserts that (tupof ref) = value.
   * Expects ref to resolve to a ProtoLocal.
   */
  bool TypeConstraintApplicator::assert_on_tup(OperatorInstance* oi, SExpr* ref, ProtoType* value) {
    DEBUG_FUNCTION(__FUNCTION__);
    if(!value->isA("ProtoTuple"))
      ierror("'tupof' assertion on non-tuple type: "+ref->to_str());
    return assert_ref(oi,ref,T_TYPE(value));
  }
  
  /**
   * Asserts that (ft ref) = value.
   * Expects ref to resolve to a ProtoField.
   */
  bool TypeConstraintApplicator::assert_on_ft(OperatorInstance* oi, SExpr* ref, ProtoType* value) {
    DEBUG_FUNCTION(__FUNCTION__);
    if(!value->isA("ProtoField")) {
      if(value->isA("ProtoLocal")) {
        V4 << "coercing a ProtoLocal into a ProtoField" << endl;
        ProtoLocal* lvalue = &dynamic_cast<ProtoLocal &>(*value);
        return assert_ref(oi,ref,new ProtoField(lvalue));
      }
      ierror("'ft' assertion on non-field type: "+ref->to_str());
    }
    return assert_ref(oi,ref, F_TYPE(value));
  }
  
  /**
   * Asserts that (output ref) = value.
   * Expects ref to resolve to a ProtoLambda.
   */
  bool TypeConstraintApplicator::assert_on_output(OperatorInstance* oi, SExpr* ref, ProtoType* value) {
    DEBUG_FUNCTION(__FUNCTION__);
    ProtoType* lambda_arg = get_ref(oi,ref);
    if(!lambda_arg->isA("ProtoLambda"))
       ierror("'output' assertion on a non-lambda type: "+ref->to_str()+" (it's a "+lambda_arg->to_str()+")");
    ProtoLambda* lambda = &dynamic_cast<ProtoLambda &>(*lambda_arg);
    //Signature* sig = lambda->op->signature;
    return assert_ref(oi,ref,lambda);
  }
  
  /**
   * Asserts that (inputs ref) = value.
   * Expects ref to resolve to a ProtoLambda.
   * Expects value to be a ProtoTuple.
   */
  bool TypeConstraintApplicator::assert_on_inputs(OperatorInstance* oi, SExpr* ref, ProtoType* value) {
    DEBUG_FUNCTION(__FUNCTION__);
    ProtoType* lambda_arg = get_ref(oi,ref);
    if(!lambda_arg->isA("ProtoLambda"))
       ierror("'inputs' assertion on a non-lambda type: "+ref->to_str()+" (it's a "+lambda_arg->to_str()+")");
    ProtoLambda* lambda = &dynamic_cast<ProtoLambda &>(*lambda_arg);
    Signature* sig = lambda->op->signature;
    if(!value->isA("ProtoTuple"))
       ierror("'inputs' assertion on a non-tuple type: value (it's a "+value->to_str()+")");
    ProtoTuple* tup = T_TYPE(value);
    for(int i=0;i<sig->n_fixed()&&i<tup->types.size();i++) {
      sig->set_nth_type(i, ProtoType::gcs(sig->nth_type(i),tup->types[i]));
    }
    /* TODO: Not sure how to do REST elements yet
    if(sig->rest_input) {
       ProtoType* gcs = 
          ProtoType::gcs(sig->rest_input,tup->types[tup->types.size()-1]);
       if(gcs)
         sig->rest_input = gcs;
    }
    */
    return assert_ref(oi,ref,lambda);
  }
  
  /**
   * Asserts that (last ref) = value.
   * Expects ref to resolve to a ProtoTuple.
   */
  bool TypeConstraintApplicator::assert_on_last(OperatorInstance* oi, SExpr* ref, ProtoType* value) {
    DEBUG_FUNCTION(__FUNCTION__);
    ProtoType* argtype = get_ref(oi, ref);
    ProtoTuple* tup = NULL;
    if(!argtype->isA("ProtoTuple")) {
      V3<<"Coercing "<<ce2s(argtype)<<" to ProtoTuple"<<endl;
      tup = new ProtoTuple(); tup->add(value);
    } else {
      // clone and replace last element
      tup = new ProtoTuple(T_TYPE(argtype));
      if(tup->bounded) {
        tup->types[tup->types.size()-1] = value;
      } else {
        // can't establish the last value of an unbounded tuple
      }
    }
    tup = ProtoTuple::specialize_tuple_class(tup);
    return assert_ref(oi,ref,tup);
  }

  /**
   * Gets the LCS of nextType and prevType.  isRestElem must be true if
   * nextType is a ProtoTuple derived from a &rest element in its function
   * signature.
   */
  ProtoType* TypeConstraintApplicator::getLCS(ProtoType* nextType, ProtoType* prevType, bool isRestElem) {
    ProtoType* compound = prevType;
    // if it's a &rest element, take the LCS of each sub-argument
    if(isRestElem && nextType->isA("ProtoTuple")) {
      ProtoTuple* tv = &dynamic_cast<ProtoTuple &>(*nextType);
      for(int i=0;i<tv->types.size();i++) {
        compound=(compound==NULL)? tv->types[i] : ProtoType::lcs(compound, tv->types[i]);
        V4 << "Assert (rest) lcs: next=" << ce2s(nextType)
           << ", next[" << i << "]=" << ce2s(tv->types[i])
           << ", compound=" << ce2s(compound) <<endl;
      }
    }
    // if it's not a &rest, take the LCS of each argument
    else {
      compound=(compound==NULL)? nextType : ProtoType::lcs(compound, nextType);
      V4 << "Assert lcs: next=" << ce2s(nextType)
         << ", compound=" << ce2s(compound) <<endl;
    }
    return compound;
  }
  
  /**
   * Asserts that (lcs ref) = value.
   * Reads the remainder of arguments from li and takes the LCS of all the arguments in ref.
   */
  bool TypeConstraintApplicator::assert_on_lcs(OperatorInstance* oi, SExpr* ref, ProtoType* value, SE_List_iter* li) {
    DEBUG_FUNCTION(__FUNCTION__);
    SExpr* next = NULL;
    // get the first arg to LCS
    V4 << "assert LCS ref:   " << ce2s(ref) << endl;
    V4 << "assert LCS value: " << ce2s(value) << endl;
    ProtoType* compound = getLCS(value,NULL,isRestElement(oi,ref));
    bool ret = assert_ref(oi,ref,compound);
    // get the remaining args to LCS
    while(li->has_next()) {
      next = li->get_next("type");
      compound = getLCS(value,compound,isRestElement(oi,next));
      ret |= assert_ref(oi,next,compound);
      V4 << "assert LCS ref:   " << ce2s(next) << endl;
      V4 << "assert LCS value: " << ce2s(value) << endl;
    }
    return ret;
  }

  /**
   * Adds items to an unbounded 'tup' to ensure that it has at least
   * 'n' elements.  Return true if the size changed.
   */
  bool TypeConstraintApplicator::fillTuple(ProtoTuple* tup, int n) {
    if(tup->bounded) ierror("Cannot fill a bounded ProtoTuple: "+ce2s(tup));
    bool changed = false;
    for(int i=tup->types.size()-1; i<n; i++) {
      ProtoType* elt = ProtoType::clone(tup->types[i]); // copy rest element
      tup->add(elt); // add to end of tuple
      changed = true;
    }
    return changed;
  }
  
  /**
   * Asserts that (nth next ...li...) = value.
   * Expects next to be a ProtoTuple and reads the next type from li, which it
   * expects to be a ProtoScalar.  Uses the ProtoScalar as the index into the
   * ProtoTuple.  Asserts that the nth item of the tuple is value.
   */
  bool TypeConstraintApplicator::assert_on_nth(OperatorInstance* oi, SExpr* next, ProtoType* value, SE_List_iter* li) {
    DEBUG_FUNCTION(__FUNCTION__);
    //1) ---tuple---
    //ProtoType* tupType = ProtoType::gcs(get_ref(oi,next),new ProtoTuple());
    ProtoType* tupType = get_ref(oi,next);
    // ensure that <Any> get treated as ProtoTuple
    ProtoType* gcsTup = ProtoType::gcs(tupType, new ProtoTuple());
    if(gcsTup == NULL || !gcsTup->isA("ProtoTuple")) {
       ierror("'nth' assertion on a non-tuple type: "+next->to_str()
              +" (it's a "+tupType->to_str()+")");
       return false;
    }
    //we can at least say that the first argument is a <Tuple <Any>...>
    bool changedTup = assert_ref(oi,next,T_TYPE(gcsTup)); 

    //2) ---scalar---
    SExpr* indexExpr = li->get_next("type");
    ProtoType* indexType;
    bool changedScalar = false;
    if(indexExpr->isScalar()) {
      indexType = new ProtoScalar(dynamic_cast<SE_Scalar &>(*indexExpr).value);
      V4 << "nth assertion found constant index: " << ce2s(indexType) << endl;
    } else {
      indexType = ProtoType::gcs(get_ref(oi,indexExpr),new ProtoScalar());
      V4 << "nth assertion found var index type: " << ce2s(indexType) << endl;
      //we can at least say that the second arg is a <Scalar>
      changedScalar = assert_ref(oi,indexExpr,S_TYPE(indexType));
    }
    if(indexType == NULL || !indexType->isA("ProtoScalar")) {
       ierror("'nth' assertion on a non-scalar type: "+indexExpr->to_str()+" (it's a "+indexType->to_str()+")");
    }
    
    //3) ---nth of tup---
    tupType = get_ref(oi,next); // re-get the tuple-ized tuple
    V4 << "asserting nth element "<<ce2s(indexType)<<" as "<<ce2s(value)
       <<" on "<<ce2s(tupType)<<endl;
    ProtoTuple* newt = new ProtoTuple(T_TYPE(tupType));
    if(!tupType->isA("ProtoTuple"))
       ierror("'nth' assertion failed to coerce a tuple from type: "+next->to_str()
              +" (it's a "+tupType->to_str()+")");
    if(indexType->isLiteral() //we know the index & it's valid
              && (int)S_VAL(indexType) >= 0) {
      int i = S_VAL(indexType);
      if(newt->bounded) {
        if(i >= newt->types.size())
           ierror("'nth' assertion index ("+indexType->to_str()+
                  ") exceeded tuple length of bounded Tuple: "+ce2s(tupType));
        newt->types[i] = value;
      } else {
        int min_size_of_tup = i+1;
        fillTuple(newt,min_size_of_tup);
        newt->types[i] = value;
      }
    }
    bool changedNth = assert_ref(oi,next,newt);
    return changedTup || changedScalar || changedNth;
  }
  
  /**
   * Asserts that (unlit ref) = value.
   * Asserts a de-literalized value onto its arguments.
   */
  bool TypeConstraintApplicator::assert_on_unlit(OperatorInstance* oi, SExpr* next, ProtoType* value) {
    DEBUG_FUNCTION(__FUNCTION__);
    return assert_ref(oi,next,Deliteralization::deliteralize(value));
  }

  /**
   * Returns true iff type is a Tuple derived from a &rest element.
   */
  bool TypeConstraintApplicator::isRestElement(OperatorInstance* oi, SExpr* ref) {
    if(!ref->isSymbol()) return false;
    Signature* s = oi->op->signature;
    return s->rest_input
      && (s->parameter_id(&dynamic_cast<SE_Symbol &>(*ref)) == s->n_fixed());
  }

  /**
   * Asserts value onto a list (ref) from oi.
   */
  bool TypeConstraintApplicator::assert_ref_list(OperatorInstance* oi, SExpr* ref, ProtoType* value) {
    DEBUG_FUNCTION(__FUNCTION__);
    SE_List_iter li(ref);
    // "fieldof": field containing a local
    if(li.on_token("fieldof")) 
      return assert_on_field(oi,li.get_next("type"),value);
    // "ft": type of local contained by a field
    if(li.on_token("ft")) 
      return assert_on_ft(oi,li.get_next("type"),value);
    // "inputs": the types of a function's input signature
    if(li.on_token("inputs")) 
      return assert_on_inputs(oi,li.get_next("type"),value);
    // "last": find the last type in a tuple
    if(li.on_token("last")) 
       return assert_on_last(oi,li.get_next("type"),value);
    // "lcs": finds the least common superclass of a set of types
    if(li.on_token("lcs")) 
       return assert_on_lcs(oi,li.get_next("type"),value,&li);
    // "nth": finds the nth type in a tuple
    if(li.on_token("nth")) 
       return assert_on_nth(oi,li.get_next("type"),value,&li);
    // "outputs": the type of a function's output
    if(li.on_token("output"))
      return assert_on_output(oi,li.get_next("type"),value);
    // "tupof": tuple w. a set of types as arguments
    if(li.on_token("tupof"))
      return assert_on_tup(oi,li.get_next("type"),value);
    // "unlit": generalize away literal values
    if(li.on_token("unlit")) 
      return assert_on_unlit(oi,li.get_next("type"),value);
  }

  /** 
   * Core assert dispatch function.
   * If possible, modify the referred type with a GCS.
   */
  bool TypeConstraintApplicator::assert_ref(OperatorInstance* oi, SExpr* ref, ProtoType* value) {
    V4<<"=========================="<< endl;
    DEBUG_FUNCTION(__FUNCTION__);
    V2<<"assert_ref "<< ce2s(ref) << " as " << ce2s(value) << endl;
    V4<<"Asserting type "<<ce2s(value)<<" for reference "<<ce2s(ref)<<endl;
    if(ref->isSymbol()) {
      // Named input/output argument:
      int n = oi->op->signature->parameter_id(&dynamic_cast<SE_Symbol &>(*ref),
          false);
      if(n>=0) return assert_nth_arg(oi,n,value);
      if(n==-1) return assert_range(oi->output,value);
      // "args": a tuple of argument types
      if(*ref=="args") return assert_all_args(oi,value);
      // return: the value of a compound operator's output field
      if(*ref=="return") return assert_op_return(oi->op,value);
    } else if(ref->isList()) {
      return assert_ref_list(oi,ref,value);
    }
    ierror(ref,"Unknown type reference: "+ce2s(ref)); // temporary, to help test suite
    return type_err(ref,"Unknown type reference: "+ce2s(ref));
  }

  bool TypeConstraintApplicator::repair_field_constraint(OI* oi, 
                                                         ProtoField* field,
                                                         ProtoLocal* local) {
    ProtoType* newt = ProtoType::gcs(field,new ProtoField(local));
    V4 << "Repair changing " << ce2s(local) << " to " << ce2s(newt) << endl;
    if(oi->pointwise()==1) {
      //Fieldify
      Operator* fo = FieldOp::get_field_op(oi);
      if(fo) {
        V4 << "Repair changing op"<<ce2s(oi->op)<<" to "<<ce2s(fo)<<endl;
        oi->op = fo;
      }
    }
    if(oi->output->range->isA("ProtoLocal")) {
      //Change output
      ProtoLocal* lrange = &dynamic_cast<ProtoLocal &>(*oi->output->range);
      ProtoType* newt = new ProtoField(lrange);
      V4 << "Repair changing output " << ce2s(oi->output->range) 
         << " to " << ce2s(newt) << endl;
      oi->output->range = newt;
    }
    return true;
  }

// TODO: this code and the other field repair code ought to be merged, somehow
  bool TypeConstraintApplicator::repair_constraint_failure(OI* oi, 
                                                           ProtoType* ftype, 
                                                           ProtoType* ctype) {
    if(ftype->isA("ProtoField") && ctype->isA("ProtoLocal")) {
      ProtoLocal* ltype = &dynamic_cast<ProtoLocal &>(*ctype);
      return repair_field_constraint(oi, F_TYPE(ftype), ltype);
    } else if(ctype->isA("ProtoField") && ftype->isA("ProtoLocal")) {
      ProtoLocal* ltype = &dynamic_cast<ProtoLocal &>(*ftype);
      return repair_field_constraint(oi, F_TYPE(ctype), ltype);
    }
    if(oi->op==Env::core_op("local") && ftype->isA("ProtoField")) {
      return true; // defer repair to later
    }
    return false;
  }
  
  /********** External Interface **********/
  bool TypeConstraintApplicator::apply_constraint(OperatorInstance* oi, SExpr* constraint) {
    DEBUG_FUNCTION(__FUNCTION__);
    SE_List_iter li(constraint,"type constraint");
    if(!li.has_next())
      { compile_error(constraint,"Empty type constraint"); return false; }
    if(li.on_token("=")) { // types should be identical
      V4<<"type constraint: "+ce2s(li.peek_next())<<endl;
      SExpr *aref = li.get_next("type reference");
      SExpr *bref=li.get_next("type reference");
      ProtoType *a = get_ref(oi,aref), *b = get_ref(oi,bref);
      V3<<"apply_constraint: aref("<<ce2s(aref)<<")="<<ce2s(a)
         <<", bref("<<ce2s(bref)<<")="<<ce2s(b) << endl;
      // first, take the GCS
      ProtoType *joint = ProtoType::gcs(a,b);
      if(joint==NULL) { // if GCS shows conflict, attempt to correct
        if(verbosity > 4)
          parent->root->printdot(cpout);
        if(!repair_constraint_failure(oi,a,b))
          type_err(oi,"Type constraint "+ce2s(constraint)+" violated: \n  "
                   +a->to_str()+" vs. "+b->to_str()+" at "+ce2s(oi));
        return true; // note that a change has occurred
      } else { // if GCS succeeded, assert onto referred locations
        V3<<"apply_constraint: joint="<<ce2s(joint)<< endl;
        bool ret = assert_ref(oi,aref,joint) | assert_ref(oi,bref,joint);
        V4<<"apply_constraint: done"<< endl;
        if(verbosity > 4)
          parent->root->printdot(cpout);
        return ret;
      }
      // if it's OK, then push back:  maybe_set_ref(oi,constraint[i]);
      // if it's not OK, then call for resolution
    } else {
      compile_error("Unknown constraint '"+ce2s(li.peek_next())+"'");
    }
    return false;
  }
  
  bool TypeConstraintApplicator::apply_constraints(OperatorInstance* oi, SExpr* constraints) {
    DEBUG_FUNCTION(__FUNCTION__);
    bool changed=false;
    SE_List_iter li(constraints,"type constraint set");
    while(li.has_next()) 
      { changed |= apply_constraint(oi,li.get_next("type constraint")); }
    return changed;
  }

/*****************************************************************************
 *  TYPE RESOLUTION                                                          *
 *****************************************************************************/

// Type resolution needs to be applied to anything that isn't
// concrete...

TypePropagator::TypePropagator(DFGTransformer* parent, Args* args)
  : IRPropagator(true,true,true) {
  verbosity = args->extract_switch("--type-propagator-verbosity") ? 
    args->pop_int() : parent->verbosity;
}
  
// implicit type conversion or other modification to fix conflict
// returns true if repair is successful
bool TypePropagator::repair_conflict(Field* f,Consumer c,ProtoType* ftype,ProtoType* ctype) {
  
  if (c.first != NULL) {
    OI *cfirst = c.first;
    if(cfirst->op == Env::core_op("mux") &&
       cfirst->inputs.size() >= 3 &&
       cfirst->domain() != cfirst->inputs[1]->domain &&
       cfirst->domain() != cfirst->inputs[2]->domain ) {
      // Compilation error if a field crosses OUT of an AM boundary
      compile_error("Cannot restrict (if) inside a neighborhood operation ("+ce2s(c.first->op)+")");
    }
  }
  
  // if vector is needed and scalar is provided, convert to a size-1 vector
  if(ftype->isA("ProtoScalar") && ctype->isA("ProtoVector")) {
    V4<<"repair scalar->vector"<< endl;
    if(c.first==NULL) return true; // let it be repaired in Field action stage
    V2<<"Converting Scalar to 1-Vector\n";
    OI* tup = new OperatorInstance(f->producer,Env::core_op("tup"),f->domain);
    tup->add_input(f);
    root->relocate_source(c.first,c.second,tup->output);
    note_change(tup); return true; // let constraint retry later...
  }
  
  // if source is local and user wants a field, add a "local" op
  // or replace no-argument source with a field op
  if(ftype->isA("ProtoLocal") && ctype->isA("ProtoField")) {
    V4<<"repair local->field "<< ce2s(ftype) << " " << ce2s(ctype) << endl;
    V4 << "f->producer: " << ce2s(f->producer) << endl;
    V4 << "f->producer-op: " << ce2s(f->producer->op) << endl;
    if(c.first==NULL) return true; // let it be repaired in Field action stage
    ProtoField* ft = &dynamic_cast<ProtoField &>(*ctype);
    if(ProtoType::gcs(ftype,ft->hoodtype)) {
      Operator* fo = FieldOp::get_field_op(f->producer);
      
      if(f->producer->op == Env::core_op("mux") &&
         f->producer->inputs.size() >= 3 &&
         f->producer->domain() != f->producer->inputs[1]->domain &&
         f->producer->domain() != f->producer->inputs[2]->domain ) {
        // Compilation error if a field crosses OUT of an AM boundary
        compile_error("Cannot restrict (if) inside a neighborhood operation ("+ce2s(c.first->op)+")");
      } else if (f->producer->inputs.size()==0 && f->producer->pointwise()!=0 && fo) {
        V2<<"Fieldify pointwise: "<<ce2s(f->producer)<<endl;
        OI* foi = new OperatorInstance(f->producer,fo,f->domain);
        root->relocate_source(c.first,c.second,foi->output);
        note_change(foi); return true; // let constraint retry later...
      } else {
        
        // Otherwise, insert a local
        V2<<"Inserting 'local' at "<<ce2s(f)<<endl;
        OI* local = new OI(f->producer,Env::core_op("local"),f->domain);
        local->add_input(f);
        root->relocate_source(c.first,c.second,local->output);
        note_change(local); return true; // let constraint retry later...
      }
    }
  }
  
  // if source is field and user is "local", send to the local's consumers
  if(ftype->isA("ProtoField") && c.first&&c.first->op==Env::core_op("local")){
    V4<<"repair field->local"<< endl;
    if(c.first==NULL) return true; // let it be repaired in Field action stage
    V2<<"Bypassing 'local' at "<<ce2s(f)<<endl;
    root->relocate_consumers(c.first->output,f);
    return true;
  }
  
  // if source is field and user is pointwise, upgrade to field op
  if(ftype->isA("ProtoField") && ctype->isA("ProtoLocal")) {
    V4<<"repair field->pointwise "<< c.first << endl;
    if(c.first==NULL) return true; // let it be repaired in Field action stage
    ProtoField* ft = &dynamic_cast<ProtoField &>(*ftype);
    int pw = c.first->pointwise();
    V4<<"pointwise? "<< pw << endl;
    if(f->producer->op == Env::core_op("mux") &&
       f->producer->inputs.size() >= 3 &&
       f->producer->domain() != f->producer->inputs[1]->domain &&
       f->producer->domain() != f->producer->inputs[2]->domain ) {
      // Compilation error if a field crosses OUT of an AM boundary
      compile_error("Cannot restrict (if) inside a neighborhood operation ("+ce2s(c.first->op)+")");
    }
    if(pw!=0 && ProtoType::gcs(ctype,ft->hoodtype)) {
      Operator* fo = FieldOp::get_field_op(c.first); // might be upgradable
      V4<<"fieldop = "<< ce2s(fo) << endl;
      if(fo) {
        V2<<"Fieldify pointwise: "<<ce2s(c.first->op->signature)<<" to "<<ce2s(fo->signature)<<endl;
        c.first->op = fo; 
        note_change(c.first);
      }
      return true; // in any case, don't fail out now
    }
  }
  
  V4<<"no matching repairs"<< endl;
  return false; // repair has failed
}

/// apply a consumer constraint, managing implicit type conversions 
bool TypePropagator::back_constraint(ProtoType** tmp,Field* f,pair<OperatorInstance*,int> c) {
  DEBUG_FUNCTION(__FUNCTION__);
  ProtoType* ct = c.first->op->signature->nth_type(c.second);
  V3<<"Back constraint on: "<<ce2s(c.first)<<", input "<<i2s(c.second)<<endl;
  V3<<"- back type: "<<ce2s(*tmp)<<" vs. "<<ce2s(ct)<<"..."<<endl;
  // Attempt to narrow the type:
  ProtoType* newtype = ProtoType::gcs(*tmp,ct);
  if(newtype) { V3<<"- ok\n"; *tmp = newtype; return true; }
  else V3<<"- FAIL\n";
  
  // On merge failure, attempt to repair conflict
  if(repair_conflict(f,c,*tmp,ct)) return false;
  // having fallen through all cases, throw type error
  type_err(f,"conflict: "+f->to_str()+" vs. "+c.first->to_str());
  return false;
}

void TypePropagator::act(Field* f) {
  V3 << "Considering field "<<ce2s(f)<<endl;
  // Ignore old type (it may change) [except Parameter]; use producer type
  ProtoType* tmp = f->producer->op->signature->output;
  if(f->producer->op->isA("Parameter")) {
    Parameter* p = (Parameter*)f->producer->op;
    tmp=ProtoType::gcs(f->range,p->container->signature->nth_type(p->index));
    V4<<"Parameter: "<<p->to_str()<<" range "<<f->range->to_str()<<" sigtype "
      <<p->container->signature->nth_type(p->index)->to_str()<<endl;
  }
  // GCS against current value
  ProtoType* newtmp = ProtoType::gcs(tmp,f->range);
  if(!newtmp) {
    V3 << "Attempting to repair conflict with producer "<<ce2s(tmp)<<endl;
    if(!repair_conflict(f, make_pair(static_cast<OI *>(0), -1), tmp,
                        f->range)) {
      type_err(f,"incompatible type and signature "+f->to_str()+": "+
               ce2s(tmp)+" vs. "+ce2s(f->range));
      return;
    }
    // go on after repair
  } else {
    tmp=newtmp;
  }
  
  // GCS against consumers
  for_set(Consumer,f->consumers,i)
    if(!back_constraint(&tmp,f,*i)) return; // type problem handled within
  // GCS against selectors
  if(f->selectors.size()) {
    Operator *fop = f->producer->op;
    ProtoType *fout = fop->signature->output;
    if (fout->isA("ProtoField")) {
      V4 << "FieldOps are ok now" << endl;
      // check that its a field of scalars?
    } else {
      ProtoType *newtmp = ProtoType::gcs(tmp,new ProtoScalar());
      if (newtmp) {
        tmp = newtmp;
      } else {
        type_err(f,"non-scalar selector "+f->to_str()); return;
      }
    }
  }
  maybe_set_range(f,tmp);
  
  /*
    if(f->range->type_of()=="ProtoTuple") { // look for tuple->vector upgrades
    ProtoTuple* tt = &dynamic_cast<ProtoTuple &>(*f->range);
    for(int i=0;i<tt->types.size();i++) 
    if(!tt->types[i]->isA("ProtoScalar")) return; // all scalars?
    ProtoVector* v = new ProtoVector(tt->bounded); v->types = tt->types;
    maybe_set_range(f,v);
    }
  */
}

// At each round, consider types of all neighbors... 
// GCS against each that resolves
// Resolve shouldn't care about the current type!

void TypePropagator::act(OperatorInstance* oi) {
  V3 << "Considering op instance "<<ce2s(oi)<<endl;
  //check if number of inputs is legal
  if(!oi->op->signature->legal_length(oi->inputs.size())) {
    compile_error(oi,oi->to_str()+" has "+i2s(oi->inputs.size())+" argumen"+
                  "ts, but signature is "+oi->op->signature->to_str());
    return;
  }
  
  if(oi->op->isA("Primitive")) { // constrain against signature
    // new-style resolution
    if(oi->op->marked(":type-constraints")) {
      TypeConstraintApplicator tca(this);
      if(tca.apply_constraints(oi,get_sexp(oi->op,":type-constraints")))
        note_change(oi);
    }
    
    V4 << "Attributes of " << ce2s(oi) << ": " << endl;
    map<string,Attribute*>::const_iterator end = oi->attributes.end();
    for( map<string,Attribute*>::const_iterator it = oi->attributes.begin(); it != end; ++it) {
      V4 << "- " << it->first << endl;
    }
    if(oi->attributes.count("LETFED-MUX")) {
      V4 << "LETFED-MUX in oi: " << ce2s(oi) << endl;
      V4 << "output is: " << ce2s(oi->output->range) 
         << ((oi->output->range->isA("DerivedType"))?"(Derived)":"(non-derived)") 
         << ((oi->output->range->isLiteral())?"(Literal)":"(non-literal)") 
         << endl;
      V4 << "init is: " << ce2s(oi->inputs[1]->range) 
         << ((oi->inputs[1]->range->isA("DerivedType"))?"(Derived)":"(non-derived)") 
         << ((oi->inputs[1]->range->isLiteral())?"(Literal)":"(non-literal)") 
         << endl;
    }
    if(oi->attributes.count("LETFED-MUX")
       //                && oi->output->range->isA("DerivedType")
       ){
      // letfed mux resolves from init
      if(oi->inputs.size()<2)
        compile_error(oi,"Can't resolve letfed type: not enough mux arguments");
      Field* init = oi->inputs[1]; // true input = init
      if(!init->range->isA("DerivedType")) {
        V4 << "Resolving LETFED-MUX from init: " << ce2s(init->range) << endl;
        maybe_set_range(oi->output,Deliteralization::deliteralize(init->range));
      }
    }
    // ALSO: find GCS of producer, consumers, & field values
  } else if(oi->op->isA("Parameter")) { // constrain vs all calls, signature
    // find LCS of input types
    Parameter* p = &dynamic_cast<Parameter &>(*oi->op);
    OIset *srcs = &root->funcalls[p->container];
    ProtoType* inputs = NULL;
    for_set(OI*,*srcs,i) {
      if((*i)->op->isA("Literal")) return; // can't work with lambdas
      ProtoType* ti = (*i)->nth_input(p->index);
      inputs = inputs? ProtoType::lcs(inputs,ti) : ti;
    }
    ProtoType* sig_type = p->container->signature->nth_type(p->index);
    inputs = inputs ? ProtoType::gcs(inputs,sig_type) : sig_type;
    if(!inputs) return;
    // then take GCS of that against current field value
    ProtoType* newtype = ProtoType::gcs(oi->output->range,inputs);
    if(!newtype) {compile_error(oi,"type conflict for "+oi->to_str());return;}
    maybe_set_range(oi->output,newtype);
  } else if(oi->op->isA("CompoundOp")) { // constrain against params & output
    ProtoType* rtype = dynamic_cast<CompoundOp &>(*oi->op).output->range;
    V4 << "Constraining type "<<ce2s(rtype)<<" with op output "<<ce2s(oi->output->range)<<endl;
    ProtoType* newtype = ProtoType::gcs(rtype,oi->output->range);
    if(newtype==NULL) newtype = ProtoType::lcs(rtype,oi->output->range);
    maybe_set_range(oi->output,newtype);
  } else if(oi->op->isA("Literal")) { // ignore: already be fully resolved
    // ignored
  } else {
    ierror("Don't know how to do type inference on undefined operators");
  }
}

// AMs that are the body of a CompoundOp may affect their signature
void TypePropagator::act(AmorphousMedium* am) {
  CompoundOp* f = am->bodyOf; if(f==NULL) return; // only CompoundOp ams
  if(!ProtoType::equal(f->signature->output,f->output->range)) {
    V2<<"Changing signature output of "<<ce2s(f)<<" to "<<ce2s(f->output->range)<<endl;
    f->signature->output = f->output->range;
    note_change(am);
  }
}


