#ifndef RF_ANALYZER_SETTINGS
#define RF_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class RFAnalyzerSettings : public AnalyzerSettings
{
public:
	RFAnalyzerSettings();
	virtual ~RFAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

	
	Channel mInputChannel;
	//U32 mBitRate;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mInputChannelInterface;
};

#endif //RF_ANALYZER_SETTINGS
