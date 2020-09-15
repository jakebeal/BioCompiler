
class SimNeighbour : public Neighbour {
	public:
		Counter data_age; // The number of rounds executed since the last update of the data of this neighbour.
		Number x, y, z;
		Number lag;
		
		bool in_range;
		
		SimNeighbour(MachineId const & id, Size imports) : Neighbour(id, imports) {x = 0; y = 0; z = 0; lag = 0; in_range = false;}
};

#undef Neighbour
#define Neighbour SimNeighbour
