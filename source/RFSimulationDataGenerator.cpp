#include "RFSimulationDataGenerator.h"
#include "RFAnalyzerSettings.h"

#include <AnalyzerHelpers.h>

RFSimulationDataGenerator::RFSimulationDataGenerator()
:	mRFbits{ 0x401fe4, 0xeafaa8, 0x4608aa },
	mRFindex ( 0 )
{
}

RFSimulationDataGenerator::~RFSimulationDataGenerator()
{
}

void RFSimulationDataGenerator::Initialize( U32 simulation_sample_rate, RFAnalyzerSettings* settings )
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	mRFSimulationData.SetChannel( mSettings->mInputChannel );
	mRFSimulationData.SetSampleRate( simulation_sample_rate );
	mRFSimulationData.SetInitialBitState( BIT_LOW );
}

U32 RFSimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel )
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested, sample_rate, mSimulationSampleRateHz );

	while(mRFSimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested )
	{
		CreateRFstream();
	}

	*simulation_channel = &mRFSimulationData;
	return 1;
}

void RFSimulationDataGenerator::CreateRFstream()
{
//bit high = 3 alpha high + 1 alpha low
//bit low = 1 alpha high + 3 alpha low
//bit sync = 1 alpha high + 31 alpha low

	U32 samples_per_alpha = mSimulationSampleRateHz / 2000;

	if (mRFindex == 0) {
		mRFSimulationData.Advance( samples_per_alpha );
	}

	//show data
	for( U32 x = 0; x < 24; x++ ){
		bool bit = mRFbits[mRFindex].test(x);

		if( bit == true ){
			mRFSimulationData.TransitionIfNeeded(BIT_HIGH);
			mRFSimulationData.Advance( samples_per_alpha * 3 );
			mRFSimulationData.TransitionIfNeeded(BIT_LOW);
			mRFSimulationData.Advance( samples_per_alpha * 1 );
			//mRFSimulationData.TransitionIfNeeded(BIT_HIGH);
		}
		else{
			mRFSimulationData.TransitionIfNeeded(BIT_HIGH);
			mRFSimulationData.Advance( samples_per_alpha * 1 );
			mRFSimulationData.TransitionIfNeeded(BIT_LOW);
			mRFSimulationData.Advance( samples_per_alpha * 3 );
			//mRFSimulationData.TransitionIfNeeded(BIT_HIGH);
		}
	}

	//show sync
	mRFSimulationData.TransitionIfNeeded(BIT_HIGH);
	mRFSimulationData.Advance(samples_per_alpha * 1);
	mRFSimulationData.TransitionIfNeeded(BIT_LOW);
	mRFSimulationData.Advance(samples_per_alpha * 31);
	//mRFSimulationData.TransitionIfNeeded(BIT_HIGH);

	mRFindex++;

	if (mRFindex == 3) {
		mRFindex = 0;
	}
}
