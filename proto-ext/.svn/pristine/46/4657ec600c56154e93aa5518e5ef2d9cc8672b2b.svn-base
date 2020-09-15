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
/// Provides the Thread classs.

#ifndef __THREAD_HPP
#define __THREAD_HPP

#include "types.hpp"
#include "data.hpp"
#include "time.hpp"

class BasicThread {
	
	public:
		/// An index in the Machine::threads list.
		/** \memberof Thread */
		typedef Int8 Id;
		
		/// The last time this thread was executed.
		/** \memberof Thread */
		Time last_time;
		
		/// The desired period for this thread.
		/** \memberof Thread */
		/**
		 * \warning This variable is currently not used in the scheduler.
		 */
		Time desired_period; //TODO: Use this in the scheduler.
		
	protected:
		bool is_triggered;
		bool is_active;
		
	public:
		BasicThread() : desired_period(1) {} //TODO: Find a better way to get a value for desired_period.
		
		/// Trigger this thread.
		/**
		 * The specified thread will be executed once.
		 * 
		 * There is no waiting line. Triggering an already triggered thread has no effect.
		 * 
		 */
		/** \memberof Thread */
		void trigger() {
			is_triggered = true;
		}
		
		/// Activate this thread.
		/**
		 * The specified thread will be executed infinitely, until deactivation.
		 * 
		 */
		/** \memberof Thread */
		void activate() {
			is_active = true;
		}
		
		/// Deactivate this thread.
		/*
		 * The selected thread will stop executing infinitely.
		 * 
		 * This has no effect for any pending triggers.
		 */
		/** \memberof Thread */
		void deactivate() {
			is_active = false;
		}
		
		/// Check whether this thread is pending (true) or not (false).
		/** \memberof Thread */
		bool pending() const {
			return is_active || is_triggered;
		}
		
		/// The result of the last execution of this thread.
		/** \memberof Thread */
		Data result;
		
	protected:
		/// Remove a pending trigger.
		/** \memberof Thread */
		void untrigger() {
			is_triggered = false;
		}
		
};

/** \cond */
#define Thread BasicThread
#include "extensions.hpp"
typedef Thread ExtendedThread;
#undef Thread
/** \endcond */

/// A thread.
/**
 * The 'main function' of each thread is stored in the Machine::globals list of the machine.
 * The first thread corresponds to the last global, the second thread to the second last global, and so on.
 * 
 * A thread can be triggered and activated. It is called 'pending' when it is either triggered, activated or both.
 * The Machine::run() starts the execution of the next pending Thread, Round-robin style.
 * 
 * \note This class is \ref extending "extensible".
 * 
 * \extends BasicThread
 */
class Thread : public ExtendedThread {
	friend class Machine;
};

#endif
