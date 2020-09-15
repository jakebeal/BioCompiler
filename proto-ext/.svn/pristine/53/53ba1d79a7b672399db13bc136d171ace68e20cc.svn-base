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
/// Provides the special form instructions.

#include <machine.hpp>
#include <instructions.hpp>
#include <tuple.hpp>

namespace Instructions {
	
	/// \name Special form instructions
	/// \{
	
	namespace {
		void apply_end(Machine & machine){
			Data result = machine.stack.pop();
			Tuple arguments = machine.stack.popTuple();
			machine.stack.popAddress();
			machine.stack.push(result);
			machine.environment.pop(arguments.size());
		}
	}
	
	/// Call a function.
	/**
	 * \param Address The (address of the) function to call.
	 * \param Tuple The arguments to give to the function.
	 * \return Data The return value of the function.
	 */
	void APPLY(Machine & machine){
		Tuple arguments  = machine.stack.peek(0).asTuple();
		Address function = machine.stack.peek(1).asAddress();
		for(Index i = 0; i < arguments.size(); i++) machine.environment.push(arguments[i]);
		machine.call(function, apply_end);
	}
	
	namespace {
		void map_step(Machine & machine){
			machine.environment.pop(1);
			Data new_element = machine.stack.pop();
			Tuple & result = machine.stack.peek(0).asTuple();
			Tuple & values = machine.stack.peek(1).asTuple();
			result.push(new_element);
			if (result.size() < values.size()){
				machine.environment.push(values[result.size()]);
				Address filter = machine.stack.peek(2).asAddress();
				machine.call(filter, map_step);
			} else {
				Data result = machine.stack.popTuple();
				machine.stack.popTuple();
				machine.stack.popAddress();
				machine.stack.push(result);
			}
		}
	}
	
	/// Map all elements of a Tuple using a function.
	/**
	 * The given function will be called for every element in the tuple.
	 * The elements will be replaced by the corresponding return value of the function.
	 * 
	 * \param Address The (address of the) function to call.
	 * \param Tuple The tuple of which the elements wil be mapped.
	 * 
	 * \return Tuple The tuple with all the mapped elements.
	 */
	void TUP_MAP(Machine & machine){
		Tuple & values = machine.stack.peek(0).asTuple();
		if (values.empty()){
			machine.stack.popTuple();
			machine.stack.popAddress();
			machine.stack.push(Tuple());
		} else {
			Address filter = machine.stack.peek(1).asAddress();
			machine.stack.push(Tuple(values.size()));
			machine.environment.push(values[0]);
			machine.call(filter, map_step);
		}
	}
	
#if MIT_COMPATIBILITY != NO_MIT
	/// \deprecated_mitproto{TUP_MAP}
	void MAP(Machine & machine){
		machine.nextInt8();
		TUP_MAP(machine);
	}
#endif
	
	namespace {
		void fold_step(Machine & machine) {
			machine.environment.pop(2);
			Data      result      = machine.stack.pop();
			Number  & fold_index  = machine.stack.peek(0).asNumber ();
			Tuple   & fold_values = machine.stack.peek(1).asTuple  ();
			Address & fold_fuse   = machine.stack.peek(2).asAddress();
			if (++fold_index < fold_values.size()){
				machine.environment.push(result);
				machine.environment.push(fold_values[Index(fold_index)]);
				machine.call(fold_fuse, fold_step);
			} else {
				machine.stack.popNumber();
				machine.stack.popTuple();
				machine.stack.popAddress();
				machine.stack.push(result);
			}
		}
	}
	
	/// Fold a Tuple to a single element with a given 'fuse function'.
	/**
	 * The fuse function is used to consecutively fuse the previous fuse result with the next element of the Tuple.
	 * The first element will be fused with the given starting value.
	 * 
	 * For example, a tuple with three elements is folded like this:
	 * \dot
	 * digraph Folding {
	 *   node [fontsize=10,width=0,height=0];
	 *   start -> a -> b -> c -> result;
	 *   "first element" -> a;
	 *   "second element" -> b;
	 *   "third element" -> c;
	 *   a [shape=box,label=fuse];
	 *   b [shape=box,label=fuse];
	 *   c [shape=box,label=fuse];
	 * }
	 * \enddot
	 * 
	 * If an empty Tuple is given, the starting value is returned.
	 * 
	 * \param Address The (address of the) fuse function. (The fold function must take two parameters.)
	 * \param Data The value to start with.
	 * \param Tuple The tuple to fold.
	 * 
	 * \return Data The result of the folding process.
	 */
	void FOLD(Machine & machine){
		Tuple   fold_values = machine.stack.pop().asTuple();
		Data    result      = machine.stack.pop();
		Address fold_fuse   = machine.stack.peek().asAddress();
		
		if (fold_values.empty()){
			machine.stack.popAddress();
			machine.stack.push(result);
		} else {
			machine.stack.push(fold_values);
			machine.stack.push(0);
			machine.environment.push(result);
			machine.environment.push(fold_values[0]);
			machine.call(fold_fuse, fold_step);
		}
	}
	
	/// \deprecated_mitproto{FOLD}
	void VFOLD(Machine & machine){
		machine.nextInt8();
		FOLD(machine);
	}
	
	/// \}
	
}
