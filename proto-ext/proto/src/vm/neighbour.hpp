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
/// Provides the Neighbour class.

#ifndef __NEIGHBOUR_HPP
#define __NEIGHBOUR_HPP

#include "array.hpp"
#include "data.hpp"
#include "machineid.hpp"

class BasicNeighbour {
	
	public:
		/// The unique ID of the machine.
		/** \memberof Neighbour */
		MachineId const id;
		
		/// The imports from this machine.
		/** \memberof Neighbour */
		Array<Data> imports;
		
		BasicNeighbour(MachineId const & id, Size imports) : id(id), imports(imports) {}
		
};

/** \cond */
#define Neighbour BasicNeighbour
#include "extensions.hpp"
typedef Neighbour ExtendedNeighbour;
#undef Neighbour
/** \endcond */

/// A node in the neighbourhood.
/**
 * \note This class is \ref extending "extensible".
 * 
 * \note A machine is also a neighbour of itself.
 * \see Machine::thisMachine()
 */
class Neighbour : public ExtendedNeighbour {
	
	public:
		/// The constructor.
		Neighbour(MachineId const & id, Size imports) : ExtendedNeighbour(id, imports) {}
		
};

//asdf
#endif
