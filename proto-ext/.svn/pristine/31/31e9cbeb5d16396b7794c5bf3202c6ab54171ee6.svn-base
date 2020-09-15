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
/// Provides the control flow instructions.

#include <machine.hpp>
#include <instructions.hpp>

namespace Instructions {
	
	/// \name Control flow
	/// \{
	
#if MIT_COMPATIBILITY != NO_MIT
	/// Define the VM requirements.
	/**
	 * This should be executed before any other instruction.
	 * 
	 * \param Int8 The export length. (This parameter is currently not used.)
	 * \param Int8 The number of exports.
	 * \param Int16 The number of globals.
	 * \param Int8 The number of state variables.
	 * \param Int16 The size of the execution stack.
	 * \param Int8 The size of the environment stack.
	 * 
	 * \deprecated_mitproto{DEF_VM_EX}
	 */
	void DEF_VM(Machine & machine){
		/*Size export_length    =*/ machine.nextInt8 (); //TODO: ...
		Size     exports_size = machine.nextInt8 ();
		Size     globals_size = machine.nextInt16();
		Size       state_size = machine.nextInt8 ();
		Size       stack_size = machine.nextInt16();
		Size environment_size = machine.nextInt8 ();
		
		machine.      stack.reset(   stack_size+20); // MIT Proto calculates the stack size slightly different than how DelftProto uses it. Add 20 to be safe.
		machine.environment.reset(environment_size);
		machine.    globals.reset(    globals_size);
		machine.    threads.reset(               1);
		machine.      state.reset(      state_size);
		machine.       hood.reset(    exports_size);
		
		machine.current_thread = 0;
		machine.threads[0].activate();
		
		machine.hood.add(machine.id);
		
		Instruction callback = machine.callbacks.pop();
		machine.callbacks.reset(3); //TODO: ...
		machine.callbacks.push(callback);
	}
#endif
	
#if MIT_COMPATIBILITY != MIT_ONLY
	/// Define the (extended) VM requirements.
	/**
	 * This should be executed before any other instruction.
	 * 
	 * \param Int The size of the execution stack.
	 * \param Int The size of the environment stack.
	 * \param Int The number of globals.
	 * \param Int The number of threads.
	 * \param Int The number of state variables.
	 * \param Int The number of exports.
	 * \param Int The maximum execution depth (for instructions that execute functions, such as MAP).
	 */
	void DEF_VM_EX(Machine & machine){
		machine.      stack.reset(machine.nextInt());
		machine.environment.reset(machine.nextInt());
		machine.    globals.reset(machine.nextInt());
		machine.    threads.reset(machine.nextInt());
		machine.      state.reset(machine.nextInt());
		machine.       hood.reset(machine.nextInt());
		
		machine.current_thread = 0;
		
		machine.hood.add(machine.id);
		
		Instruction callback = machine.callbacks.pop();
		machine.callbacks.reset(machine.nextInt());
		machine.callbacks.push(callback);
	}
#endif
	
	/// Exit the installation script.
	/**
	 * This must be the last instruction of an installation script.
	 */
	void EXIT(Machine & machine){
		machine.callbacks.pop(machine.callbacks.size());
	}
	
	/// Return from a function.
	/**
	 * \param Data The return value.
	 * 
	 * \return Data The return value.
	 */
	void RET(Machine & machine){
		machine.retn();
	}
	
	/// Clean the stack.
	/**
	 * Preserve the top element while the rest of the given range is dropped.
	 * For example, after executing <tt>ALL_OP 3</tt> on the stack <tt>[1 2 3 4]</tt>, <tt>[1 4]</tt> is left.
	 * 
	 * \param Int The number of elements to clean up.
	 * \param Data <tt>[n]</tt> The elements on the stack.
	 * 
	 * \return Data The top element.
	 */
	void ALL(Machine & machine){
		Data data = machine.stack.peek();
		machine.stack.pop(machine.nextInt());
		machine.stack.push(data);
	}
	
	/// Waste clockcycles.
	void NOP(Machine & machine){
		// No Operation
	}
	
	/// Pick one of two values, using a condition.
	/**
	 * \param Number The condition.
	 * \param Data The 'true' value.
	 * \param Data The 'false' value.
	 * 
	 * \return Data The 'false' value when the condition is 0, the 'true' value otherwise.
	 */
	void MUX(Machine & machine){
		Data false_value = machine.stack.pop();
		Data  true_value = machine.stack.pop();
		Number condition = machine.stack.popNumber();
		machine.stack.push(condition ? true_value : false_value);
	}
	
#if MIT_COMPATIBILITY != NO_MIT
	/// \deprecated_mitproto{MUX}
	void VMUX(Machine & machine){
		machine.nextInt8();
		Data false_value = machine.stack.pop();
		Data  true_value = machine.stack.pop();
		Number condition = machine.stack.popNumber();
		machine.stack.push(condition ? true_value : false_value);
	}
#endif
	
	/// A conditional jump.
	/**
	 * Skips the given number of bytes if the condition is not 0.
	 * 
	 * \param Int The number of bytes to skip when the condition is true.
	 * \param Number The condition.
	 */
	void IF(Machine & machine){
		Size skip = machine.nextInt();
		if (machine.stack.popNumber()) machine.skip(skip);
	}
	
#if MIT_COMPATIBILITY != NO_MIT
	/// A conditional jump.
	/**
	 * Skips the given number of bytes if the condition is not 0.
	 * 
	 * \param Int16 The number of bytes to skip when the condition is true.
	 * \param Number The condition.
	 * 
	 * \deprecated_mitproto{IF}
	 */
	void IF16(Machine & machine){
		Size skip = machine.nextInt16();
		if (machine.stack.popNumber()) machine.skip(skip);
	}
#endif
	
	/// Jump to another address.
	/**
	 * \note You can only jump forward.
	 * 
	 * \param Int the number of bytes to jump (relative).
	 */
	void JMP(Machine & machine){
		machine.skip(machine.nextInt());
	}
	
#if MIT_COMPATIBILITY != NO_MIT
	/// Jump to another address.
	/**
	 * \note You can only jump forward.
	 * 
	 * \param Int16 The number of bytes to jump (relative).
	 * 
	 * \deprecated_mitproto{JMP}
	 */
	void JMP16(Machine & machine){
		machine.skip(machine.nextInt16());
	}
#endif
	
	namespace {
		
		void FUNCALL_end(Machine & machine){
			Data result    = machine.stack.pop();
			Size arguments = machine.stack.popNumber();
			machine.environment.pop(arguments);
			machine.stack.push(result);
		}
		
	}
	
	/// Call a function.
	/**
	 * \param Int The number of arguments to pass to the function.
	 * \param Data <tt>[n]</tt> The arguments.
	 * \param Address The address of the function.
	 * \return The return value of the function.
	 */
	template<int arguments>
	void FUNCALL_N(Machine & machine){
		Address function = machine.stack.popAddress();
		for(Size i = 0; i < arguments; i++){
			machine.environment.push(machine.stack.peek(arguments-i-1));
		}
		machine.stack.pop(arguments);
		machine.stack.push(arguments);
		machine.call(function,FUNCALL_end);
	}
	
	/// Call a function.
	/**
	 * \param Int The number of arguments to pass to the function.
	 * \param Data <tt>[n]</tt> The arguments.
	 * \param Address The address of the function.
	 * \return The return value of the function.
	 */
	void FUNCALL(Machine & machine){
		Address function = machine.stack.popAddress();
		Size arguments = machine.nextInt();
		for(Size i = 0; i < arguments; i++){
			machine.environment.push(machine.stack.peek(arguments-i-1));
		}
		machine.stack.pop(arguments);
		machine.stack.push(arguments);
		machine.call(function,FUNCALL_end);
	}
	
	/// \}
	
}
