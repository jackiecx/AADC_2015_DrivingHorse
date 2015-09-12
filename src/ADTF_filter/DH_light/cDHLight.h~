#ifndef _DHLIGHT_H_
#define _DHLIGHT_H_

#define OID_ADTF_DHLIGHT_FILTER "adtf.dh.light"

//*************************************************************************************************
class cDHLight : public adtf::cFilter
{
    ADTF_FILTER(OID_ADTF_DHLIGHT_FILTER, "DH light", adtf::OBJCAT_DataFilter);

	public:
		cDHLight(const tChar* __info);
		virtual ~cDHLight();

	protected:
		tResult Init(tInitStage eStage, __exception);
		tResult Start(__exception = NULL);
		tResult Shutdown(tInitStage eStage, __exception);
		tResult Stop(__exception = NULL);

		tResult OnPinEvent(IPin* pSource, tInt nEventCode, tInt nParam1, tInt nParam2, IMediaSample* pMediaSample);
	private:
/* 		cInputPin	iPin_rpmLeft;
		cInputPin 	iPin_rpmRight; */
		cInputPin 	iPin_acc;
	
		cOutputPin	oPin_brakeLight;
		cOutputPin	oPin_headLight;
		cOutputPin	oPin_reverseLight;
		
		tBool		m_bBrakeLight;
		tBool		m_bHeadLight;
		tBool		m_bReverseLight;
		
 		cObjectPtr<IMediaTypeDescription> m_pCoderDescSignalValue;
		cObjectPtr<IMediaTypeDescription> m_pCoderDescBoolSignalValue;
		
/* 		tFloat32 	m_rpmLeft;
		tFloat32 	m_rpmRight;
		tFloat32 	m_rpm[10];
		tUInt8		cRPM;
		tFloat32	m_mrpm;
		tFloat32	m_mrpm_old;
		tBool		m_bFirstTransmit; */
		
		tFloat32 	m_acc, m_acc_old;
		tInt32		m_cTimer;
		
		/*tBool		m_brakeLight[5];
		tUInt8		cBrakeLight;*/

		tResult TransmitBrakeLight(tBool);
		tResult TransmitHeadLight(tBool);
		tResult TransmitReverseLight(tBool);
		
		
		tHandle m_hTimer;
		cCriticalSection m_oCriticalSectionTransmit;
		cCriticalSection m_oCriticalSectionTimerSetup;
		tResult createTimer();
   		tResult destroyTimer(__exception = NULL);
		
		tResult Run(tInt nActivationCode, const tVoid* pvUserData, tInt szUserDataSize, ucom::IException** __exception_ptr=NULL);
};


//*************************************************************************************************
#endif
