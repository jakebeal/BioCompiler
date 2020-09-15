#include <instructions.hpp>

#include <cmath>

using std::sqrt;
using std::atan2;

extern Number read_speed();
extern Number read_bearing();
extern Number read_radio_range();
extern void flex(Number);
extern void mov(Tuple);
extern void set_probe(Data, Int8);

namespace Instructions {
	
	void VEC(Machine & machine){
		// ?
	}
	
	void VFIL(Machine & machine){
		// ?
	}
	
	void SPEED(Machine & machine){
		machine.stack.push(read_speed());
	}
	
	void BEARING(Machine & machine){
		machine.stack.push(read_bearing());
	}
	
	void FLEX(Machine & machine){
		flex(machine.stack.peek().asNumber());
	}
	
	void MOV(Machine & machine){
		mov(machine.stack.peek().asTuple());
	}
	
	void PROBE(Machine & machine){
		Int8 i = machine.stack.pop().asNumber();
		set_probe(machine.stack.peek(), i);
	}
	
	void NBR_LAG(Machine & machine){
		machine.stack.push(machine.currentNeighbour().lag);
	}
	
	void NBR_RANGE(Machine & machine){
		float x = machine.currentNeighbour().x;
		float y = machine.currentNeighbour().y;
		float z = machine.currentNeighbour().z;
		machine.stack.push(sqrt(x*x + y*y + z*z));
	}
	
	void NBR_BEARING(Machine & machine){
		float x = machine.currentNeighbour().x;
		float y = machine.currentNeighbour().y;
		machine.stack.push(atan2(y,x));
	}
	
	void NBR_VEC(Machine & machine){
		machine.nextInt8();
		Tuple v(3);
		v.push(machine.currentNeighbour().x);
		v.push(machine.currentNeighbour().y);
		v.push(machine.currentNeighbour().z);
		machine.stack.push(v);
	}
	
	namespace {
		Number machine_disc_area(Machine & machine) {
			return read_radio_range() * read_radio_range() * 3.14;
		}
		
		Number machine_area(Machine & machine) {
			return machine_disc_area(machine) / static_cast<Number>(machine.hood.size());
		}
		
		Number machine_density(Machine & machine) {
			return static_cast<Number>(machine.hood.size()) / machine_disc_area(machine);
		}
	}
	
	void AREA(Machine & machine){
		machine.stack.push(machine_area(machine));
	}
	
	void HOOD_RADIUS(Machine & machine){
		machine.stack.push(read_radio_range());
	}
	
	void INFINITESIMAL(Machine & machine){
		machine.stack.push(machine_area(machine));
	}
	
	void DENSITY(Machine & machine){
		machine.stack.push(machine_density(machine));
	}
	
}
