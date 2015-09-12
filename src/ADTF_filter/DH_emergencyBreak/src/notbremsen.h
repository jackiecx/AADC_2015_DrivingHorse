#ifndef _NOTBREMSE_FILTER_H_
#define _NOTBREMSE_FILTER_H_

#define OID_ADTF_NOTBREMSE_FILTER "adtf.user.Notbremse"


//*************************************************************************************************
class cNotbremse : public adtf::cFilter
{
    ADTF_FILTER(OID_ADTF_NOTBREMSE_FILTER, "Notbremsen", adtf::OBJCAT_DataFilter);

protected:
	cInputPin			m_pin_input_accelerate;
	cInputPin			m_pin_input_ir_front_center_long;
	cInputPin			m_pin_input_ir_front_center_short;
	cInputPin			m_pin_input_ir_rear_center_short;
	cInputPin			m_pin_input_reset;
	
	cOutputPin			m_pin_output_accelerate;
	cOutputPin			m_pin_output_stuck;

	tFloat32			m_stopDistance, m_slowDistance15, m_slowDistance25, m_slowDistance30, m_slowDistance_back;
	tFloat32			m_accelerate, t_accelerate, t_accelerate_old;
	tFloat32			m_sensor_front_long, m_sensor_front_short, m_sensor_back;
	tFloat32			m_stuck_distance, m_stuck_time;
	
	tInt32				m_cTimer;
	
	tBool				m_gotStuck;

public:
    cNotbremse(const tChar* __info);
    virtual ~cNotbremse();

protected:
    tResult Init(tInitStage eStage, __exception);
    tResult Shutdown(tInitStage eStage, __exception);
	tResult Start(__exception = NULL);
	tResult Stop(__exception = NULL);

    // implements IPinEventSink
    tResult OnPinEvent(IPin* pSource,
                       tInt nEventCode,
                       tInt nParam1,
                       tInt nParam2,
                       IMediaSample* pMediaSample);

	tResult TransmitAcc(tFloat32 acc);
	tResult TransmitStuck(tBool state);


	/*! Coder Descriptor for the output pins*/
	cObjectPtr<IMediaTypeDescription> m_pCoderDescSignalOutput;
    cObjectPtr<IMediaTypeDescription> m_pCoderDescBoolSignalOutput;
    
    /*! Coder Descriptor for the input pins*/
	cObjectPtr<IMediaTypeDescription> m_pCoderDescSignalInput;
	cObjectPtr<IMediaTypeDescription> m_pCoderDescBoolSignalInput;
	
	/*! Timer */
	tHandle m_hTimer;
	cCriticalSection m_oCriticalSectionTransmit;
	cCriticalSection m_oCriticalSectionTimerSetup;
	tResult createTimer();
	tResult destroyTimer(__exception = NULL);

	tResult Run(tInt nActivationCode, const tVoid* pvUserData, tInt szUserDataSize, ucom::IException** __exception_ptr=NULL);
	
};

//*************************************************************************************************
#endif // _NOTBREMSE_PROJECT_FILTER_H_
