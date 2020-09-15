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
/// Provides the Script class.

#ifndef __SCRIPT_HPP
#define __SCRIPT_HPP

#include <types.hpp>
#include <array.hpp>

/// A proto script (bytecode).
class Script {
	
	protected:
	
		/// The location of the script.
		Int8 const * script;
		
		/// The length of the script.
		Size script_size;
		
	public:
		/// The default constructor.
		Script() : script(0), script_size(0) {}
		
		/// The constructor
		Script(Int8 const * script, Size size) : script(script), script_size(size) {}
		
		/// Get the script.
		operator Int8 const * () const {
			return script;
		}
		
		/// Get the length of the script.
		Size size() const {
			return script_size;
		}
		
};

#endif
