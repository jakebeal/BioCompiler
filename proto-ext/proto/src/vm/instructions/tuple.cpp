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
/// Provides the Tuple instructions.

#include <machine.hpp>
#include <instructions.hpp>

namespace Instructions {
	
	/// \name Tuple instructions
	/// \{
	
	/// Get an element from a Tuple.
	/**
	 * \param Tuple The tuple to pick the element from.
	 * \param Number The index of the element.
	 * \return Data The element.
	 */
	void ELT(Machine & machine){
		Index element = machine.stack.popNumber();
		Tuple  tuple  = machine.stack.popTuple ();
		machine.stack.push(tuple[element]);
	}
	
	/// An empty tuple.
	/**
	 * \return Tuple An empty tuple.
	 */
	void NUL_TUP(Machine & machine){
		machine.stack.push(Tuple());
	}
	
#if MIT_COMPATIBILITY != NO_MIT
	/// \deprecated_mitproto{FAB_TUP}
	void TUP(Machine & machine){
		machine.nextInt8();
		Size elements = machine.nextInt8();
		Tuple tuple(elements);
		for(Index i = 0; i < elements; i++) tuple.push(machine.stack.peek(elements-1-i));
		machine.stack.pop(elements);
		machine.stack.push(tuple);
	}
#endif
	
	/// Create a tuple from one or more elements.
	/**
	 * \param Int The number of elements.
	 * \param Data <tt>[n]</tt> The elements.
	 * \return Tuple A tuple containing the elements.
	 */
	void FAB_TUP(Machine & machine){
		Size elements = machine.nextInt();
		Tuple tuple(elements);
		for(Index i = 0; i < elements; i++) tuple.push(machine.stack.peek(elements-i-1));
		machine.stack.pop(elements);
		machine.stack.push(tuple);
	}
	
	/// Create a tuple filled with one element.
	/**
	 * \param Int The number of elements in the resulting tuple.
	 * \param Data The element to fill the tuple with.
	 * \return Tuple A tuple filled with the specified element.
	 */
	void FAB_VEC(Machine & machine){
		Size elements = machine.nextInt();
		Tuple tuple(elements);
		Data element = machine.stack.pop();
		for(Index i = 0; i < elements; i++) tuple.push(element);
		machine.stack.push(tuple);
	}
	
	/// Create a tuple filled with zero's.
	/**
	 * \param Int The number of zero's to put in the tuple.
	 * \return Tuple A tuple filled with zero's.
	 */
	void FAB_NUM_VEC(Machine & machine){
		Size elements = machine.nextInt();
		Tuple tuple(elements);
		for(Index i = 0; i < elements; i++) tuple.push(0);
		machine.stack.push(tuple);
	}
	
	/// Get the number of elements in a tuple.
	/**
	 * \param Tuple The tuple.
	 * \return Number The number of elements in the tuple.
	 * 
	 * \note If a Number was given as parameter, 1 is returned.
	 */
	void LEN(Machine & machine){
		Data a = machine.stack.pop();
		machine.stack.push(a.type() == Data::Type_number ? 1 : a.asTuple().size());
	}
	
#if MIT_COMPATIBILITY != NO_MIT
	/// \deprecated_mitproto{ADD}
	void VADD(Machine & machine){
		machine.nextInt8();
		Tuple b = machine.stack.popTuple();
		Tuple a = machine.stack.popTuple();
		Size min_size = a.size() < b.size() ? a.size() : b.size();
		Size max_size = a.size() < b.size() ? b.size() : a.size();
		Tuple & largest = a.size() < b.size() ? b : a;
		Tuple result(max_size);
		for(Index i = 0       ; i < min_size; i++) result.push(a[i].asNumber() + b[i].asNumber());
		for(Index i = min_size; i < max_size; i++) result.push(largest[i].asNumber());
		machine.stack.push(result);
	}
	
	/// \deprecated_mitproto{SUB}
	void VSUB(Machine & machine){
		machine.nextInt8();
		Tuple b = machine.stack.popTuple();
		Tuple a = machine.stack.popTuple();
		Size min_size     = a.size() < b.size() ? a.size() : b.size();
		Size max_size     = a.size() < b.size() ? b.size() : a.size();
		Tuple & largest     = a.size() < b.size() ? b        : a       ;
		Number padding_sign = a.size() < b.size() ? -1       : 1       ;
		Tuple result(max_size);
		for(Index i = 0       ; i < min_size; i++) result.push(a[i].asNumber() - b[i].asNumber());
		for(Index i = min_size; i < max_size; i++) result.push(padding_sign * largest[i].asNumber());
		machine.stack.push(result);
	}
	
	/// \deprecated_mitproto{DOT}
	void VDOT(Machine & machine){
		Number result = 0;
		Tuple b = machine.stack.popTuple();
		Tuple a = machine.stack.popTuple();
		Size min_size = a.size() < b.size() ? a.size() : b.size();
		for(Index i = 0; i < min_size; i++) result += a[i].asNumber() * b[i].asNumber();
		machine.stack.push(result);
	}
	
	/// \deprecated_mitproto{MUL}
	void VMUL(Machine & machine){
		machine.nextInt8();
		Tuple  b = machine.stack.popTuple ();
		Number a = machine.stack.popNumber();
		Tuple result(b.size());
		for(Index i = 0; i < b.size(); i++) result.push(a * b[i].asNumber());
		machine.stack.push(result);
	}
	
	/// \deprecated_mitproto
	void VSLICE(Machine & machine){
		machine.nextInt8();
		Tuple source = machine.stack.popTuple();
		Index start = machine.stack.popNumber();
		Index stop  = machine.stack.popNumber();
		start = start >= 0 ? start : source.size() + start;
		stop  = stop  >= 0 ? stop  : source.size() + stop ;
		Tuple result(stop-start);
		for(Index i = start; i < stop; i++) result.push(source[i].asNumber());
		machine.stack.push(result);
	}
	
	/// \deprecated_mitproto{EQ}
	void VEQ(Machine & machine){
		machine.execute(EQ);
	}
	
	/// \deprecated_mitproto{LT}
	void VLT(Machine & machine){
		machine.execute(LT);
	}
	
	/// \deprecated_mitproto{LTE}
	void VLTE(Machine & machine){
		machine.execute(LTE);
	}
	
	/// \deprecated_mitproto{GT}
	void VGT(Machine & machine){
		machine.execute(GT);
	}
	
	/// \deprecated_mitproto{GTE}
	void VGTE(Machine & machine){
		machine.execute(GTE);
	}
	
	/// \deprecated_mitproto{MIN}
	void VMIN(Machine & machine){
		machine.execute(MIN);
	}
	
	/// \deprecated_mitproto{MAX}
	void VMAX(Machine & machine){
		machine.execute(MAX);
	}
#endif
	
	/// \}
	
}
