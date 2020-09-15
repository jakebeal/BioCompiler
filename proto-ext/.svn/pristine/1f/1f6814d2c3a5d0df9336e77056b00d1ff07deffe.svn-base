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
/// Provides the thread instructions.

#include <machine.hpp>
#include <instructions.hpp>

namespace Instructions {
	
	/// \name Threads
	/// \{
	
	/// Get the time between this and the previous execution of this thread.
	/**
	 * If this is the first run, the desired period will be given instead.
	 * 
	 * \return Number The time, in seconds, since the last run of this thread.
	 */
	void DT(Machine & machine){
		Number dt;
		if (machine.firstFeedbackUpdate.size() > 0) {
			// First update, execute with DT of 0
			Number ff = machine.firstFeedbackUpdate.peek(0);
			if (ff == 1) {
				dt = 0;
				machine.stack.push(dt);
				return;
			}
		}
                dt = machine.startTime() - machine.currentThread().last_time;
		machine.stack.push(dt);
	}
	
	/// Set the desired period of a thread.
	/**
	 * Set the minimum period for this thread.
	 * 
	 * \warning Currently, the scheduler ignores this setting.
	 */
	void SET_DT(Machine & machine){
		Number dt = machine.stack.peek().asNumber();
		machine.currentThread().desired_period = dt;
	}
	
#if MIT_COMPATIBILITY != MIT_ONLY
	/// Activate this or another Thread.
	/**
	 * \see Thread::activate()
	 * 
	 * \param Int The index of the Thread to activate.
	 */
	void ACTIVATE(Machine & machine){
		Thread::Id thread = machine.nextInt();
		machine.threads[thread].activate();
	}
	
	/// Deactivate this or another Thread.
	/**
	 * \see Thread::deactivate()
	 * 
	 * \param Int The index of the Thread to deactivate.
	 */
	void DEACTIVATE(Machine & machine){
		Thread::Id thread = machine.nextInt();
		machine.threads[thread].deactivate();
	}
	
	/// Trigger this or another Thread.
	/**
	 * \see Thread::trigger()
	 * 
	 * \param Int The index of the Thread to trigger.
	 */
	void TRIGGER(Machine & machine){
		Thread::Id thread = machine.nextInt();
		machine.threads[thread].trigger();
	}
	
	/// Get the result of the last execution of this or another Thread.
	/**
	 * \param Int The index of the Thread.
	 * \return Data The result of the last execution of the specified Thread.
	 */
	void RESULT(Machine & machine){
		Thread::Id thread = machine.nextInt();
		machine.stack.push(machine.threads[thread].result);
	}
	
#endif
	/// \}
	
}
