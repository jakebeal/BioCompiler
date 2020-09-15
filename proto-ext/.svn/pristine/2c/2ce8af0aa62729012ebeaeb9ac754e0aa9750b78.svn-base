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
/// Provides the global variables instructions.

#include <machine.hpp>
#include <instructions.hpp>

namespace Instructions {
	
	/// \name Globals
	/// \{
	
	/// \deprecated_mitproto
	void DEF(Machine & machine){
		machine.globals.push(machine.stack.pop());
	}
	
	/// \deprecated_mitproto
	void DEF_TUP(Machine & machine){
		machine.execute(FAB_TUP);
		machine.globals.push(machine.stack.pop());
	}
	
	/// \deprecated_mitproto
	void DEF_VEC(Machine & machine){
		machine.execute(FAB_VEC);
		machine.globals.push(machine.stack.pop());
	}
	
	/// \deprecated_mitproto
	template<int elements>
	void DEF_NUM_VEC_N(Machine & machine){
		Tuple tuple(elements);
		for(Index i = 0; i < elements; i++) tuple.push(0);
		machine.globals.push(tuple);
	}
	
	/// \deprecated_mitproto
	void DEF_NUM_VEC(Machine & machine){
		machine.execute(FAB_NUM_VEC);
		machine.globals.push(machine.stack.pop());
	}
	
	/// Push a global variable on the execution stack.
	/**
	 * \tparam index The index of the global in the gobals list.
	 * \return The value of the global variable.
	 */
	template<int index>
	void GLO_REF_N(Machine & machine){
		machine.stack.push(machine.globals[index]);
	}
	
	/// Push a global variable on the execution stack.
	/**
	 * \param Int The index of the global in the gobals list.
	 * \return The value of the global variable.
	 */
	void GLO_REF(Machine & machine){
		Index index = machine.nextInt();
		machine.stack.push(machine.globals[index]);
	}
	
#if MIT_COMPATIBILITY != NO_MIT
	/// Push a global variable on the execution stack.
	/**
	 * \param Int16 The index of the global in the gobals list.
	 * \return The value of the global variable.
	 * 
	 * \deprecated_mitproto{GLO_REF}
	 */
	void GLO_REF16(Machine & machine){
		Index index = machine.nextInt16();
		machine.stack.push(machine.globals[index]);
	}
#endif
	
	/// Define a function as a global.
	/**
	 * The address of the next instruction is pushed on the globals list
	 * and execution continues after the function definition.
	 * 
	 * \tparam size The number of (following) bytes that define the function.
	 */
	template<int size>
	void DEF_FUN_N(Machine & machine){
		machine.globals.push(machine.currentAddress());
		machine.skip(size);
	}
	
	/// Define a function as a global.
	/**
	 * The address of the next instruction is pushed on the globals list
	 * and execution continues after the function definition.
	 * 
	 * \param Int The number of (following) bytes that define the function.
	 */
	void DEF_FUN(Machine & machine){
		Size size = machine.nextInt();
		machine.globals.push(machine.currentAddress());
		machine.skip(size);
	}
	
#if MIT_COMPATIBILITY != NO_MIT
	/// Define a function as a global.
	/**
	 * The address of the next instruction is pushed on the globals list
	 * and execution continues after the function definition.
	 * 
	 * \param Inti16 The number of (following) bytes that define the function.
	 * 
	 * \deprecated_mitproto{DEF_FUN}
	 */
	void DEF_FUN16(Machine & machine){
		Size size = machine.nextInt16();
		machine.globals.push(machine.currentAddress());
		machine.skip(size);
	}
#endif
	
	/// \}
	
}
