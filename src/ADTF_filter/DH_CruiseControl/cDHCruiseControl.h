#ifndef _DHCruiseControl_H_
#define _DHCruiseControl_H_

#define OID_ADTF_DHCruiseControl_FILTER "adtf.dh.CruiseControl"

//*************************************************************************************************
class cDHCruiseControl : public adtf::cFilter
{
    ADTF_FILTER(OID_ADTF_DHCruiseControl_FILTER, "DH CruiseControl", adtf::OBJCAT_DataFilter);

	public:
		cDHCruiseControl(const tChar* __info);
		virtual ~cDHCruiseControl();

	protected:
		tResult Init(tInitStage eStage, __exception);
		tResult Shutdown(tInitStage eStage, __exception);
		tResult Start(__exception = NULL);
		tResult Stop(__exception = NULL);

		tResult OnPinEvent(IPin* pSource, tInt nEventCode, tInt nParam1, tInt nParam2, IMediaSample* pMediaSample);
	private:
		cInputPin	iPin_acc;
		cInputPin	iPin_rpm_right;
		cInputPin	iPin_rpm_left;
		
		cOutputPin	oPin_acc;
		
		cObjectPtr<IMediaTypeDescription> m_pCoderDescSignalValue;
		
		tFloat32	m_rpm_left, m_rpm_right, m_rpm;
		tFloat32	m_acc, m_acc_in;
		tBool		timer;
		
		tResult TransmitValue(float value);
		
		tHandle m_hTimer;
		
		cCriticalSection m_oCriticalSectionTransmit;
		cCriticalSection m_oCriticalSectionTimerSetup;
		tResult createTimer();
		tResult destroyTimer(__exception = NULL);
	
		tResult Run(tInt nActivationCode, const tVoid* pvUserData, tInt szUserDataSize, ucom::IException** __exception_ptr=NULL);
};


//*************************************************************************************************
#endif
