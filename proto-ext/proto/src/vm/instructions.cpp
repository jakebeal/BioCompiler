/*   ____       _  __ _   ____            _
 *  |  _ \  ___| |/ _| |_|  _ \ _ __ ___ | |_ ___
 *  | | | |/ _ \ | |_| __| |_) | '__/ _ \| __/ _ \
 *  | |_| |  __/ |  _| |_|  __/| | ( (_) | |( (_) )
 *  |____/ \___|_|_|  \__|_|   |_|  \___/ \__\___/
 *
 * This file is part of DelftProto.
 * See COPYING for license details.
 */

/** \file
 * \brief Provides the \ref Instructions "Instruction" implementations.
 * 
 * This file includes the source files in the folder <tt>instructions/</tt>,
 * and then defines the \ref instructions lookup table,
 * to make sure the all the used template functions are instantiated.
 */

#include <instructions.hpp>

#include <instructions/flow.cpp>
#include <instructions/environment.cpp>
#include <instructions/globals.cpp>
#include <instructions/threads.cpp>
#include <instructions/literals.cpp>
#include <instructions/math.cpp>
#include <instructions/tuple.cpp>
#include <instructions/specialform.cpp>
#include <instructions/feedback.cpp>
#include <instructions/hood.cpp>
#include <instructions/platform.cpp>

/** \cond */
Instruction instructions[256] = {
#	define INSTRUCTION(name) Instructions::name,
#	define INSTRUCTION_N(name,n) Instructions::name##_N<n>,
#	include <delftproto.instructions>
#	undef INSTRUCTION
#	undef INSTRUCTION_N
};
/** \endcond */
