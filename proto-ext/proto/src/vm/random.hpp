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
/// Provides the Random class.

#include <cstdlib>

#ifndef __RANDOM_HPP
#define __RANDOM_HPP

#include "types.hpp"

/// Pseudo random Number generator.
class Random {
	
	public:
		
		/// Get a random Number.
		/**
		 * The random Number is picked from an uniform distribution.
		 *
		 * \param min The minimum value.
		 * \param max The maximum value.
		 *
		 * \return A random Number between min and max.
		 */
		static Number number(Number min, Number max) {
			return Number(std::rand()) / Number(RAND_MAX) * (max - min) + min;
		}
		
};

#endif
