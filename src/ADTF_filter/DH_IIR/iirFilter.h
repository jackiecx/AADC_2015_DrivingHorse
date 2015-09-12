#ifndef _IIR_FILTER_H_
#define _IIR_FILTER_H_

#define OID_ADTF_IIR_FILTER "adtf.user.IIR"


//*************************************************************************************************
class cIIR : public adtf::cFilter
{
    ADTF_FILTER(OID_ADTF_IIR_FILTER, "IIR Filter", adtf::OBJCAT_DataFilter);

protected:
	cInputPin          	m_pin_input;
	cOutputPin			m_pin_output;
	
	float				filter_reg;
	float				faktor;


public:
    cIIR(const tChar* __info);
    virtual ~cIIR();

protected:
    tResult Init(tInitStage eStage, __exception);
    tResult Shutdown(tInitStage eStage, __exception);

    // implements IPinEventSink
    tResult OnPinEvent(IPin* pSource, tInt nEventCode, tInt nParam1, tInt nParam2, IMediaSample* pMediaSample);


	cObjectPtr<IMediaTypeDescription> m_pCoderDescSignalValue;
};

//*************************************************************************************************
#endif // _IIR_PROJECT_FILTER_H_
