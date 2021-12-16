#include "RFAnalyzer.h"
#include "RFAnalyzerSettings.h"
#include <AnalyzerChannelData.h>
#include <iostream>
#include <string>

RFAnalyzer::RFAnalyzer()
:	Analyzer2(),  
	mSettings( new RFAnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
}

RFAnalyzer::~RFAnalyzer()
{
	KillThread();
}

void RFAnalyzer::SetupResults()
{
	mResults.reset( new RFAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannel );
}

void RFAnalyzer::WorkerThread() {

	U8 bitCount = 25;

	U64 markerPossition = 0;
	U64 start_samplenum_high = 0;
	U64 start_samplenum_low = 0;
	U64 end_samplenum_low = 0;

	float samplesPer1uS = 0;

	mSampleRateHz = GetSampleRate();
	samplesPer1uS = 1000000.0 / mSampleRateHz;	//convert the sample rate into number of samples needed to complete 1uS

	mRF = GetAnalyzerChannelData( mSettings->mInputChannel );

	//according to datasheet, the minimum pulse period is 230uS(13v + 47k res)
	U64 minPulsePeriod = 300;				//minimum pulse period in uS

	//one sync period is 8x pulse period
	U64 minSyncPeriod = minPulsePeriod * 8;	//minimum sync pulse period in uS

	U64 minPulseSamples = minPulsePeriod * samplesPer1uS;	//samples needed for minimum pulse
	U64 minSyncSamples = minSyncPeriod * samplesPer1uS;		//samples needed for minimum sync
	
	mRF->AdvanceToNextEdge();

	//wait for next rising edge to start
	if (mRF->GetBitState() == BIT_LOW) {
		mRF->AdvanceToNextEdge();
	}

	end_samplenum_low = mRF->GetSampleNumber();

	for( ; ; ){
		start_samplenum_high = end_samplenum_low;
		mResults->AddMarker(start_samplenum_high, AnalyzerResults::UpArrow, mSettings->mInputChannel);

		mRF->AdvanceToNextEdge(); //falling edge (end of the high part of the pulse)

		start_samplenum_low = mRF->GetSampleNumber();
		//mResults->AddMarker(start_samplenum_low, AnalyzerResults::DownArrow, mSettings->mInputChannel);

		mRF->AdvanceToNextEdge(); //falling edge (end of the high part of the pulse)
		
		end_samplenum_low = mRF->GetSampleNumber();

		//calculate the period, the duty cycle, and its ratio.
		U64 period = (end_samplenum_low - start_samplenum_high) * samplesPer1uS;
		U64 duty = (start_samplenum_low - start_samplenum_high) * samplesPer1uS;

		U64 syncMinDuty = period >> 6;
		U64 syncMaxDuty = period >> 4;
		U64 dutyLowMax = period >> 1;

		//decode sync and bits
		//discard pulses with very short period to avoid noise
		if (period > minPulsePeriod) {
			markerPossition = (start_samplenum_high + start_samplenum_low) / 2;

			//SYNC - duty should not exceed period / 16
			if (duty < syncMaxDuty) {
				//valid sync should be higher than period / 64
				if ((duty > syncMinDuty) && (period > minSyncPeriod)) {
					mResults->AddMarker(markerPossition, AnalyzerResults::Start, mSettings->mInputChannel);

					bitCount = 0;

					RecordFrameV1(start_samplenum_high, end_samplenum_low, RF_SYNC, 'S', 'z');
				}
			}
			else {
				if ( bitCount < 24 ) {
					//zero bit
					if (duty < dutyLowMax) {
						mResults->AddMarker(markerPossition, AnalyzerResults::Zero, mSettings->mInputChannel);
						
						RecordFrameV1(start_samplenum_high, end_samplenum_low, RF_BIT, '0', 'o');
					}

					//one bit
					else {
						mResults->AddMarker(markerPossition, AnalyzerResults::One, mSettings->mInputChannel);
						
						RecordFrameV1(start_samplenum_high, end_samplenum_low, RF_BIT, '1', 'i');
					}

					bitCount++;
				}
			}
		}


	/*
		//mRF->AdvanceToNextEdge(); //rising edge (start of the high part of the pulse)

		start_samplenum_high = mRF->GetSampleNumber();

		mRF->AdvanceToNextEdge(); //falling edge (end of the high part of the pulse)

		start_samplenum_low = mRF->GetSampleNumber();

		mRF->AdvanceToNextEdge(); //rising edge (end of the low part of the pulse)

		U64 end_samplenum_low = mRF->GetSampleNumber();

		//calculate the period, the duty cycle, and its ratio.
		U64 period = (end_samplenum_low - start_samplenum_high) * samplesPer1uS;
		U64 duty = (end_samplenum_high - start_samplenum_high) * samplesPer1uS;

		U64 syncMinDuty = period >> 6;
		U64 syncMaxDuty = period >> 4;
		U64 dutyLowMax = period >> 1;

		//decode sync and bits
		//discard pulses with very short period to avoid noise
		if(period > minPulsePeriod){
			//sync - duty should not exceed period / 16
			if(duty < syncMaxDuty){
				//print("Duty: %d / syncMinDuty: %d\n" % (duty, syncMinDuty))
				//print("S: %d / E: %d\n" % (self.ss_block, self.es_block))

				//valid sync should be higher than period / 64
				if((duty > syncMinDuty) && (period > minSyncPeriod)){
					//self.putx([2, ['Sync']]);
					//markerPossition = (start_samplenum_high + start_samplenum_high) / 2;
					mResults->AddMarker(start_samplenum_high, AnalyzerResults::Start, mSettings->mInputChannel);
					frameStart = start_samplenum_high;
					validSync = true;
				}
				else{
					frameStart = 0;
					frameEnd = 0;
					validSync = false;

					bitCount = 0;
				}
			}
			else if (validSync == true) {
				//start address frame
				if(bitCount == 0){
					addrStart = start_samplenum_high;
				}

				//start byte frame
				if((bitCount == 0) || (bitCount == 8) || (bitCount == 16)){
					byteStart = start_samplenum_high;
				}

				//zero bit
				if(duty < dutyLowMax){
					//self.putx([0, ['0']])
					//self.putx([3, ['%d' % bitCount]])
					mResults->AddMarker(start_samplenum_high, AnalyzerResults::Zero, mSettings->mInputChannel);
				
					//if (bitCount > 19) {
						//self.putx([6, ['0']])
					//}

					bitCount += 1;
				}

				//one bit
				else{
					byteVal = byteVal + (1 << (bitCount & 7));
					//self.putx([1, ['1']])
					//self.putx([3, ['%d' % bitCount]])
					mResults->AddMarker(start_samplenum_high, AnalyzerResults::One, mSettings->mInputChannel);

					//if (bitCount > 19) {
						//self.putx([6, ['1']])
					//}

					bitCount += 1;
				}

				//close address frame
				if(bitCount == 20){
					addrEnd = end_samplenum_low;
					addrVal2 = byteVal;
					//self.puta(addrStart, addrEnd, addrVal0, addrVal1, addrVal2)
					addrVal0 = 0;
					addrVal1 = 0;
					addrVal2 = 0;
				}

				//close byte frame
				if((bitCount == 8) || (bitCount == 16) || (bitCount == 24)){
					byteEnd = end_samplenum_low;

					if(bitCount == 8){
						addrVal0 = byteVal;
						//pos = 1;
					}
					else if(bitCount == 16){
						addrVal1 = byteVal;
						//pos = 2;
					}
					else{
						//pos = 3;
					}

					//self.putb(byteStart, byteEnd, byteVal, pos)
					byteVal = 0;
				}

				//close period frame
				if(bitCount == 24){
					if(frameStart != 0){
						frameEnd = end_samplenum_low;
						totalPeriod = frameEnd - frameStart;
						totalPeriod_t = totalPeriod * samplesPer1uS;
						bitCount = 0;

						//we have a frame to save. 
						Frame frame;
						frame.mData1 = totalPeriod_t;
						frame.mFlags = 0;
						frame.mStartingSampleInclusive = frameStart;
						frame.mEndingSampleInclusive = frameEnd;

						mResults->AddFrame( frame );
						mResults->CommitResults();
						ReportProgress( frame.mEndingSampleInclusive );
					}
				}
			}
		}*/
	}
}

void RFAnalyzer::RecordFrameV1(U64 starting_sample, U64 ending_sample, RFframeType type, U64 data1, U64 data2) {
	Frame frame;

	frame.mStartingSampleInclusive = starting_sample;
	frame.mEndingSampleInclusive = ending_sample;
	frame.mFlags = 0;
	frame.mType = (U8)type;
	frame.mData1 = data1;
	frame.mData2 = data2;

	mResults->AddFrame(frame);
	mResults->CommitResults();
	ReportProgress(frame.mEndingSampleInclusive);
}

void RFAnalyzer::RecordFrameV2(U64 starting_sample, U64 ending_sample, RFframeType type, U64 data1, U64 data2) {
	FrameV2 frame;

	frame.mStartingSampleInclusive = starting_sample;
	frame.mEndingSampleInclusive = ending_sample;
	frame.mFlags = 0;
	frame.mType = (U8)type;
	frame.mData1 = data1;
	frame.mData2 = data2;

	mResults->AddFrameV2(frame);
	mResults->CommitResults();
	ReportProgress(frame.mEndingSampleInclusive);
}

bool RFAnalyzer::NeedsRerun()
{
	return false;
}

U32 RFAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 RFAnalyzer::GetMinimumSampleRateHz()
{
	return 25000;
}

const char* RFAnalyzer::GetAnalyzerName() const
{
	return "RF signal";
}

const char* GetAnalyzerName()
{
	return "RF signal";
}

Analyzer* CreateAnalyzer()
{
	return new RFAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}