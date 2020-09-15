/*   ____       _  __ _   ____            _
 *  |  _ \  ___| |/ _| |_|  _ \ _ __ ___ | |_ ___
 *  | | | |/ _ \ | |_| __| |_) | '__/ _ \| __/ _ \
 *  | |_| |  __/ |  _| |_|  __/| | ( (_) | |( (_) )
 *  |____/ \___|_|_|  \__|_|   |_|  \___/ \__\___/
 *
 * This file is part of DelftProto.
 * See COPYING for license details.
 */

/// \file
/// Provides the hood instructions

#include <machine.hpp>
#include <instructions.hpp>

struct HoodInstructions {
	
	static void fold_hood(Machine & machine) {
		Index import_index = machine.nextInt();
		Data export_value = machine.stack.pop();
		Data result = machine.stack.pop();
		Address fuse = machine.stack.peek().asAddress();
		
		machine.current_import = import_index;
		
		machine.thisMachine().imports[import_index] = export_value;
		
		machine.current_neighbour = machine.hood.begin();
		
		machine.environment.push(result);
		machine.environment.push(export_value);
		machine.call(fuse,fold_hood_step);
	}
	
	static void fold_hood_step(Machine & machine) {
		machine.environment.pop(2);
		while(++machine.current_neighbour != machine.hood.end() && !machine.current_neighbour->imports[machine.current_import].isSet());
		if (machine.current_neighbour != machine.hood.end()){
			machine.environment.push(machine.stack.pop());
			machine.environment.push(machine.current_neighbour->imports[machine.current_import]);
			Address fuse = machine.stack.peek().asAddress();
			machine.call(fuse,fold_hood_step);
		} else {
			Data result = machine.stack.pop();
			machine.stack.pop(1);
			machine.stack.push(result);
		}
	}
	
	static void fold_hood_plus(Machine & machine) {
		Index import_index = machine.nextInt();
		Data export_value = machine.stack.pop();
		Address filter = machine.stack.peek().asAddress();
		
		machine.current_import = import_index;
		
		machine.thisMachine().imports[import_index] = export_value;
		
		machine.current_neighbour = machine.hood.begin();
		
		machine.environment.push(export_value);
		machine.call(filter,fold_hood_plus_first_filter);
	}
	
	static void fold_hood_filter_next(Machine & machine){
		while(++machine.current_neighbour != machine.hood.end() && !machine.current_neighbour->imports[machine.current_import].isSet());
		if (machine.current_neighbour != machine.hood.end()){
			machine.environment.push(machine.current_neighbour->imports[machine.current_import]);
			Address filter = machine.stack.peek(1).asAddress();
			machine.call(filter,fold_hood_plus_step_filter);
		} else {
			Data result = machine.stack.pop();
			machine.stack.pop(2);
			machine.stack.push(result);
		}
	}
	
	static void fold_hood_plus_first_filter(Machine & machine){
		machine.environment.pop(1);
		fold_hood_filter_next(machine);
	}
	
	static void fold_hood_plus_step_filter(Machine & machine){
		machine.environment.pop(1);
		Data import = machine.stack.pop();
		machine.environment.push(machine.stack.pop());
		machine.environment.push(import);
		Address fuse   = machine.stack.peek(1).asAddress();
		machine.call(fuse,fold_hood_plus_step_fuse);
	}
	
	static void fold_hood_plus_step_fuse(Machine & machine){
		machine.environment.pop(2);
		fold_hood_filter_next(machine);
	}
	
  static void fold_field(Machine & machine, Data& init, Instruction fuser) {
    Data f = machine.stack.pop();
    FieldData::const_iterator fi = f.asField().begin();
    machine.stack.push(init);
    while(fi.hasNext()) {
      machine.stack.push(fi.value());
      fuser(machine); // run fuser instruction, leaving result on stack for next
      fi++;
    }
  }
};

namespace Instructions {
	
  void MIN_HOOD(Machine & machine) {
    Data init = Data(Number_infinity);
    HoodInstructions::fold_field(machine,init,Instructions::MIN);
  }

  // WARNING: THIS IS A TEST INSTRUCTION AND NOT COMPATIBLE WITH IF
  void NBR_IDS(Machine & machine) {
    NeighbourHood::iterator nbr = machine.hood.begin();
    FieldData result = FieldData();
    while(nbr != machine.hood.end()) {
      result.push(nbr->id,nbr->id);
      nbr++;
    }
    machine.stack.push(result);
  }

	/// \name Hood instructions
	/// \{
	
	/// Get the ID of this machine.
	/**
	 * \return Number The (unique) ID of this machine.
	 */
	void MID(Machine & machine){
		machine.stack.push(machine.id);
	}
	
	/// Fold all imported values for a specific neighbourhood variable and update the corresponding export.
	/**
	 * The fuse function is used to consecutively fuse the previous fuse result with the import value of the next neighbour.
	 * The import value from the first neighbour (ie. this machine) will be fused with the given starting value.
	 * 
	 * The value of the neighbour hood variable (ie. export) of this machine is set before the folding process starts.
	 * 
	 * For example, a neighbourhood with three neighbours (including this machine) is folded like this:
	 * \dot
	 * digraph Folding {
	 *   node [fontsize=10,width=0,height=0];
	 *   start -> a -> b -> c -> result;
	 *   "new export value" -> "export of this machine" -> a;
	 *   "import of neighbour 1" -> b;
	 *   "import of neighbour 2" -> c;
	 *   a [shape=box,label=fuse];
	 *   b [shape=box,label=fuse];
	 *   c [shape=box,label=fuse];
	 * }
	 * \enddot
	 * 
	 * \deprecated Implemented for MIT Proto compatibility.
	 * \note There is currently no way of doing this without using deprecated instructions. (This will be fixed soon.)
	 * 
	 * \param Int The index of the neighbourhood (ie. import/export) variable.
	 * \param Address The (address of the) fuse function. (The fold function must take two parameters.)
	 * \param Data The value to start with.
	 * \param Data The new export value for this Machine.
	 * 
	 * \return Data the result of the folding process.
	 */
	void FOLD_HOOD(Machine & machine){
		HoodInstructions::fold_hood(machine);
	}
	
	/// \deprecated_mitproto
	void VFOLD_HOOD(Machine & machine){
		machine.nextInt8();
		HoodInstructions::fold_hood(machine);
	}
	
	/// Filter and fold all imported values for a specific neighbourhood variable and update the corresponding export.
	/**
	 * The fuse function is used to consecutively fuse the previous fuse result with result of the filter applied to the import value of the next neighbour.
	 * 
	 * The value of the neighbour hood variable (ie. export) of this machine is set before the folding process starts.
	 * 
	 * For example, a neighbourhood with three neighbours (including this machine) is folded like this:
	 * \dot
	 * digraph Folding {
	 *   node [fontsize=10,width=0,height=0];
	 *   "new export value" -> "export of this machine" -> x -> b -> c -> result;
	 *   "import of neighbour 1" -> y -> b;
	 *   "import of neighbour 2" -> z -> c;
	 *   b [shape=box,label=fuse];
	 *   c [shape=box,label=fuse];
	 *   x [shape=box,label=filter];
	 *   y [shape=box,label=filter];
	 *   z [shape=box,label=filter];
	 * }
	 * \enddot
	 * 
	 * \deprecated Implemented for MIT Proto compatibility.
	 * \note There is currently no way of doing this without using deprecated instructions. (This will be fixed soon.)
	 * 
	 * \param Int The index of the neighbourhood (ie. import/export) variable.
	 * \param Address The (address of the) fuse function. (The fold function must take two parameters.)
	 * \param Address The (address of the) filter function. (The filter function must take one parameter.)
	 * \param Data The new export value for this Machine.
	 * 
	 * \return Data the result of the folding process.
	 */
	void FOLD_HOOD_PLUS(Machine & machine){
		HoodInstructions::fold_hood_plus(machine);
	}
	
	/// \deprecated_mitproto
	void VFOLD_HOOD_PLUS(Machine & machine){
		machine.nextInt8();
		HoodInstructions::fold_hood_plus(machine);
	}
	
	/// \}
	
}

Data const &   FieldData::const_iterator::value() { return source->values[index]; }

// Two input pointwise:
void FieldData::pointwise_instruction(Machine &machine,Instruction instruction,Data & a) {
  FieldData::const_iterator ia = a.asField().begin();
  FieldData result = FieldData(a.asField().size());
  while(ia.hasNext()) {
    assert(ia.value().type() != Data::Type_field);
    machine.stack.push(ia.value());
    instruction(machine);
    result.push(ia.id(),machine.stack.pop());
    ia++;
  }
  machine.stack.push(result);
}

// Three cases: a, b, or both are fields
void FieldData::pointwise_instruction(Machine &machine,Instruction instruction,Data & a, Data & b) {
  bool a_field = a.type() == Data::Type_field;
  bool b_field = b.type() == Data::Type_field;
  assert(a_field || b_field);
  
  if(a_field && b_field) { // Both arguments are fields:
    FieldData::const_iterator ia = a.asField().begin();
    FieldData::const_iterator ib = b.asField().begin();
    FieldData result = FieldData(a.asField().size());
    while(ia.hasNext() && ib.hasNext()) {
      assert(ia.id() == ib.id());
      assert(ia.value().type() != Data::Type_field && ib.value().type() != Data::Type_field);
      machine.stack.push(ia.value());
      machine.stack.push(ib.value());
      instruction(machine);
      result.push(ia.id(),machine.stack.pop());
      ia++; ib++;
    }
    assert(!ia.hasNext() && !ib.hasNext());
    machine.stack.push(result);
  } else if(a_field) { // Only first argument is field:
    FieldData::const_iterator ia = a.asField().begin();
    FieldData result = FieldData(a.asField().size());
    while(ia.hasNext()) {
      assert(ia.value().type() != Data::Type_field);
      machine.stack.push(ia.value());
      machine.stack.push(b);
      instruction(machine);
      result.push(ia.id(),machine.stack.pop());
      ia++;
    }
    machine.stack.push(result);
  } else { // Only second argument is field:
    FieldData::const_iterator ib = b.asField().begin();
    FieldData result = FieldData(b.asField().size());
    while(ib.hasNext()) {
      assert(ib.value().type() != Data::Type_field);
      machine.stack.push(a);
      machine.stack.push(ib.value());
      instruction(machine);
      result.push(ib.id(),machine.stack.pop());
      ib++;
    }
    machine.stack.push(result);
  }
}
