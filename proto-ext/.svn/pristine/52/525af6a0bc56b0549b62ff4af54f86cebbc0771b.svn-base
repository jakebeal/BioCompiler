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
/// Provides the State class.

#ifndef __STATE_HPP
#define __STATE_HPP

#include <data.hpp>
#include <thread.hpp>

/// A state variable.
/**
 * \see Instructions::INIT_FEEDBACK
 * \see Instructions::FEEDBACK
 */
class State {
	public:
		
		/// The data contained in this state variable.
		Data data;
		
		/// A boolean indicating wheter this state variable was used in the last round (true) or not (false).
		/**
		 * When a state variable is not used in a run of the thread this state variable belongs to, it is reset by the machine.
		 */
		bool is_executed;
		
		/// The thread that this state variable belongs to.
		Thread::Id thread;
		
		/// The default constructor.
		State() : is_executed(false) {}
		
};

#endif
