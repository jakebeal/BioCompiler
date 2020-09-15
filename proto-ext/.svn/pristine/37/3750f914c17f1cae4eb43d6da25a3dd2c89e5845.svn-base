
void platform_operation(Int8); // This will be called for any unknown opcode.

class SimMachine : public Machine {
	
	protected:
		void execute_unknown(Int8 opcode) {
			platform_operation(opcode);
		}
		
};

#undef Machine
#define Machine SimMachine
