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
/// Provides the Instructions namespace.

#ifndef __INSTRUCTIONS_HPP
#define __INSTRUCTIONS_HPP

/** \cond */
#define NO_MIT         0
#define MIT_COMPATIBLE 1
#define MIT_ONLY       2

#ifndef MIT_COMPATIBILITY
#define MIT_COMPATIBILITY MIT_COMPATIBLE
#endif
/** \endcond */

/** \namespace Instructions
 * \brief All instructions and their opcodes.
 * 
 * Opcodes are declared as the name of the instruction with \c _OP appended.
 * 
 * \note
 * Int, Int8, Int16 and IEEE754binary32 parameters are read from the byte(s) following the opcode.
 * Address, Number, Tuple and Data parameters are taken from the execution stack.
 * %Stack parameters should be pushed in the same order as they are listed in this documentation.
 * 
 * \note
 * The return value (if any) is pushed on the execution stack.
 * If a mathematical formula is given in the documentation as return value, it is (obviously) a Number.
 * In all other cases, the return type is explicitly described.
 */
namespace Instructions {}

class Machine;

/// An instruction (implementation).
/**
 * A pointer to to the function that executes the instruction on the given machine.
 */
typedef void (*Instruction)(Machine &);

/// Lookup table for all instructions by their opcode.
extern Instruction instructions[256];

/** \cond */

namespace Instructions {
	
#	define INSTRUCTION(name)                   void name    (Machine &);
#	define INSTRUCTION_N(name,n) template<int> void name##_N(Machine &);
#	include "delftproto.instructions"
#	undef INSTRUCTION
#	undef INSTRUCTION_N
	
	enum {
#		define INSTRUCTION(name)     name##_OP,
#		define INSTRUCTION_N(name,n) name##_##n##_OP,
#		include "delftproto.instructions"
#		undef INSTRUCTION
#		undef INSTRUCTION_N
	};
}

/** \endcond */

#endif
