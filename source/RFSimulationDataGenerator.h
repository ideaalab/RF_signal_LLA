#ifndef RF_SIMULATION_DATA_GENERATOR
#define RF_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
#include <bitset>

class RFAnalyzerSettings;

class RFSimulationDataGenerator
{
public:
	RFSimulationDataGenerator();
	~RFSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, RFAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	RFAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected:
	void CreateRFstream();
	std::bitset<24> mRFbits[3];
	U32 mRFindex;

	SimulationChannelDescriptor mRFSimulationData;

};
#endif //RF_SIMULATION_DATA_GENERATOR