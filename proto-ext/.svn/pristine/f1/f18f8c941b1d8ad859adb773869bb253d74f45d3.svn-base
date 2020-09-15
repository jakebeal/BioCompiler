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
/// Provides the feedback instructions.

#include <machine.hpp>
#include <instructions.hpp>

namespace Instructions {
	
	/// \name Feedback
	/// \{
	
	namespace {
		void INIT_FEEDBACK_set_state(Machine & machine){
			Data state = machine.stack.pop();
			Index state_index = machine.stack.popNumber();
			machine.state[state_index].data = state;
			machine.stack.push(state);
		}
	}
	
	/// Get and (if needed) initialize a state variable.
	/**
	 * \note A state variable will be reset when no feedback instruction is used on it in a single run.
	 *       It will be reinitialized by the next time INIT_FEEDBACK is executed.
	 * 
	 * \param Int The index of the state variable.
	 * \param Address The function to give the initial value, if not yet set.
	 * 
	 * \return Data The value of the state variable.
	 */
	void INIT_FEEDBACK(Machine & machine){
		Index state_index = machine.nextInt();
		Address initialization_function = machine.stack.popAddress();
		machine.state[state_index].is_executed = true;
		machine.state[state_index].thread = machine.currentThreadId();
		if (machine.state[state_index].data.isSet()){
			machine.stack.push(machine.state[state_index].data);
			machine.firstFeedbackUpdate.push(0);
		} else {
			machine.stack.push(Number(state_index));
			machine.firstFeedbackUpdate.push(1);
			machine.call(initialization_function, INIT_FEEDBACK_set_state);
		}
	}
	
#if MIT_COMPATIBILITY != MIT_ONLY
	/// Set a state variable.
	/**
	 * \param Int The index of the state variable.
	 * \param Data The new value for the state variable.
	 * 
	 * \return Data The new value of the state variable.
	 */
	void SET_FEEDBACK(Machine & machine){
		Index state_index = machine.nextInt();
		machine.state[state_index].data = machine.stack.peek();
		machine.state[state_index].is_executed = true;
	}
#endif
	
	/// \deprecated_mitproto
	void FEEDBACK(Machine & machine){
		Index state_index = machine.nextInt();
		Data value = machine.stack.pop();
		machine.state[state_index].data = value;
		machine.state[state_index].is_executed = true;
		machine.stack.pop(1);
		machine.stack.push(value);
		machine.firstFeedbackUpdate.pop();
	}
	
	/// \}
	
}
