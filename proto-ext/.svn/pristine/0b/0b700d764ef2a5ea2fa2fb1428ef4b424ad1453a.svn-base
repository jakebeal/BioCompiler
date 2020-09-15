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
/// Provides the environment stack instructions.

#include <machine.hpp>
#include <instructions.hpp>

namespace Instructions {
	
	/// \name Environment stack
	/// \{
	
	/// Push an element from the environment stack on the execution stack.
	/**
	 * \tparam index The index (relative to the top) of the element on the environment stack.
	 * \return The element.
	 */
	template<int index>
	void REF_N(Machine & machine){
		machine.stack.push(machine.environment.peek(index));
	}
	
	/// Push an element from the environment stack on the execution stack.
	/**
	 * \param Int The index (relative to the top) of the element on the environment stack.
	 * \return The element.
	 */
	void REF(Machine & machine){
		Index index = machine.nextInt();
		machine.stack.push(machine.environment.peek(index));
	}
	
	/// Push one or more elements on the environment stack.
	/**
	 * The given number of elements will be moved from the top of the execution stack to the environment stack.
	 * 
	 * \tparam elements The number of elements.
	 * \param Data <tt>[n]</tt> The elements.
	 */
	template<int elements>
	void LET_N(Machine & machine){
		for(Index i = 1; i <= elements; i++) machine.environment.push(machine.stack.peek(elements-i));
		machine.stack.pop(elements);
	}
	
	/// Push one or more elements on the environment stack.
	/**
	 * The given number of elements will be moved from the top of the execution stack to the environment stack.
	 * 
	 * \param Int The number of elements.
	 * \param Data <tt>[n]</tt> The elements.
	 */
	void LET(Machine & machine){
		Size elements = machine.nextInt();
		for(Index i = 1; i <= elements; i++) machine.environment.push(machine.stack.peek(elements-i));
		machine.stack.pop(elements);
	}
	
	/// Remove one or more elements from the environment stack.
	/**
	 * \tparam elements The number of elements.
	 */
	template<int elements>
	void POP_LET_N(Machine & machine){
		machine.environment.pop(elements);
	}
	
	/// Remove one or more elements from the environment stack.
	/**
	 * \param Int The number of elements.
	 */
	void POP_LET(Machine & machine){
		Size elements = machine.nextInt();
		machine.environment.pop(elements);
	}
	
	/// \}
	
}
