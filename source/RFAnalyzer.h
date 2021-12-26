#ifndef RF_ANALYZER_H
#define RF_ANALYZER_H

#include <Analyzer.h>
#include "RFAnalyzerResults.h"
#include "RFSimulationDataGenerator.h"

enum RFframeType { RF_SYNC, RF_BIT };

class RFAnalyzerSettings;
class ANALYZER_EXPORT RFAnalyzer : public Analyzer2
{
public:
	RFAnalyzer();
	virtual ~RFAnalyzer();

	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected: //vars
	std::auto_ptr< RFAnalyzerSettings > mSettings;
	std::auto_ptr< RFAnalyzerResults > mResults;
	AnalyzerChannelData* mRF;

	void RecordFrameV1(U64 starting_sample, U64 ending_sample, RFframeType type, U64 data1, U64 data2);
	void RecordFrameV2(U64 starting_sample, U64 ending_sample, char data);

	RFSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	U32 mSampleRateHz;

};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //RF_ANALYZER_H
