#ifndef _CHECKTRAFFIC_FILTER_H_
#define _CHECKTRAFFIC_FILTER_H_

#define OID_ADTF_CHECKTRAFFIC_FILTER "adtf.user.CHECKTRAFFIC"


//*************************************************************************************************
class cCheckTraffic : public adtf::cFilter
{
    ADTF_FILTER(OID_ADTF_CHECKTRAFFIC_FILTER, "Check Traffic", adtf::OBJCAT_DataFilter);

protected:
	cInputPin               	m_pin_input_range_front_left;
	cInputPin               	m_pin_input_range_front_right;

	cInputPin               	m_pin_input_ir_front_center_long;
	cInputPin              		m_pin_input_ir_front_center_short;

	cInputPin     	            m_pin_input_ir_rear_center_short;
	cInputPin               	m_pin_input_ir_rear_right_short;
	
	cInputPin					m_pin_input_crossing_type;
	cInputPin					m_pin_input_reset;


	cOutputPin					m_pin_output_crossing_type;

	tFloat32 					m_sensor_front_long, m_sensor_front_short;
	tFloat32 					m_ussensor_front_left, m_ussensor_front_right;
	tFloat32					m_maxdistright, m_maxdistfront, m_maxdistleft, m_minchange, m_recheckint;

	tInt32						status;
	tInt32						active;

	tFloat32					m_front_old, m_left_old, m_right_old;
	tInt8						m_type;

	tInt8						m_count, m_master_count;
	

public:
    cCheckTraffic(const tChar* __info);
    virtual ~cCheckTraffic();

protected:
    tResult Init(tInitStage eStage, __exception);
    tResult Shutdown(tInitStage eStage, __exception);
	tResult Start(__exception = NULL);
	tResult Stop(__exception = NULL);
	tResult PropertyChanged(const char* strProperty);

    // implements IPinEventSink
    tResult OnPinEvent(IPin* pSource,
                       tInt nEventCode,
                       tInt nParam1,
                       tInt nParam2,
                       IMediaSample* pMediaSample);
                       
   	tResult ProcessCheckTraffic();
    
	tResult TransmitCrossingType(const tInt8 crossingType);
	tResult FilterReset();



	tHandle m_hTimer;
	cCriticalSection m_oCriticalSectionTransmit;
	cCriticalSection m_oCriticalSectionTimerSetup;
	tResult createTimer();
	tResult destroyTimer(__exception = NULL);
	
	tResult Run(tInt nActivationCode, const tVoid* pvUserData, tInt szUserDataSize, ucom::IException** __exception_ptr=NULL);


	/*! Coder Descriptor for the output pins*/
	cObjectPtr<IMediaTypeDescription> m_pCoderDescSignalOutput;
    cObjectPtr<IMediaTypeDescription> m_pCoderDescBoolSignalOutput;
    cObjectPtr<IMediaTypeDescription> m_pCoderDescCrossingTypeInput;
    
    /*! Coder Descriptor for the input pins*/
	cObjectPtr<IMediaTypeDescription> m_pCoderDescSignalInput;
	cObjectPtr<IMediaTypeDescription> m_pCoderDescBoolSignalInput;
	cObjectPtr<IMediaTypeDescription> m_pCoderDescCrossingTypeOutput;
};

//*************************************************************************************************
#endif // _CHECKTRAFFIC_PROJECT_FILTER_H_
