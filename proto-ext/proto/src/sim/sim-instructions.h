#ifndef SIM_INSTRUCTIONS
#define SIM_INSTRUCTIONS

	enum {
#		define INSTRUCTION(name)     name,
#		define INSTRUCTION_N(name,n) name##_##n,
#		include <shared/instructions.def>
#		undef INSTRUCTION
#		undef INSTRUCTION_N
	};

map<string,uint8_t> create_opcode_map() {
   map<string,uint8_t> m;
  #define INSTRUCTION(name) m[#name "_OP"] = name;
  #define INSTRUCTION_N(name,n) m[#name "_" #n "_OP"] = name##_##n;
  #include "shared/instructions.def"
  #undef INSTRUCTION_N
  #undef INSTRUCTION
   return m;
}

#endif
