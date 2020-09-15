addInstruction(curOpcode++, "RED", function(machine) {
   machine.red = machine.stack.peek();
});

addInstruction(curOpcode++, "GREEN", function(machine) {
   machine.green = machine.stack.peek();
});

addInstruction(curOpcode++, "BLUE", function(machine) {
   machine.blue = machine.stack.peek();
});

addInstruction(curOpcode++, "RGB", function(machine) {
   var rgbTup = machine.stack.peek();
   machine.red = rgbTup[0];
   machine.green = rgbTup[1];
   machine.blue = rgbTup[2];
});

addInstruction(curOpcode++, "SENSE", function(machine) {
   var sensorNumber = machine.stack.pop();
   var sensorValue = machine.getSensor(sensorNumber);
   machine.stack.push(sensorValue ? 1 : 0);
});

addInstruction(curOpcode++, "NBR_RANGE", function(machine) {
    var x = machine.current_neighbor.x;
    var y = machine.current_neighbor.y;
    var z = machine.current_neighbor.z;
    dist = Math.sqrt(x*x + y*y + z*z);
    machine.stack.push(dist);
});

addInstruction(curOpcode++, "NBR_LAG", function(machine) {
    machine.stack.push(machine.current_neighbor.dataAge);
});

addInstruction(curOpcode++, "NBR_VEC", function(machine) {
    machine.nextInt8();
    machine.stack.push([
      machine.current_neighbor.x,
      machine.current_neighbor.y,
      machine.current_neighbor.z
      ]);
});

addInstruction(curOpcode++, "COORD", function(machine) {
    machine.stack.push([
      machine.x,
      machine.y,
      machine.z
      ]);
});

addInstruction(curOpcode++, "HOOD_RADIUS", function(machine) {
    machine.stack.push(machine.radius);
});

function machineDiscArea(machine) {
   return machine.radius * machine.radius * 3.14;
}

function machineArea(machine) {
   return machineDiscArea(machine) / machine.getNeighborhoodSize();
}

function machineDensity(machine) {
   return machine.getNeighborhoodSize() / machineDiscArea(machine);
}

addInstruction(curOpcode++, "AREA", function(machine) {
   machine.stack.push(machineArea(machine));
});

addInstruction(curOpcode++, "INFINITESIMAL", function(machine) {
   machine.stack.push(machineArea(machine));
});

addInstruction(curOpcode++, "DENSITY", function(machine) {
   machine.stack.push(machineDensity(machine));
});

addInstruction(curOpcode++, "MOV", function(machine) {
    var tuple = machine.stack.peek();
    if (tuple.length > 0) machine.dx = tuple[0]; else machine.dx = 0;
    if (tuple.length > 1) machine.dy = tuple[1]; else machine.dy = 0;
    if (tuple.length > 2) machine.dz = tuple[2]; else machine.dz = 0;
});
