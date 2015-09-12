#ifndef _CROSSING_FILTER_H_
#define _CROSSING_FILTER_H_

#define OID_ADTF_CROSSING_FILTER "adtf.user.CROSSING"


//*************************************************************************************************
class cCrossing : public adtf::cFilter
{
    ADTF_FILTER(OID_ADTF_CROSSING_FILTER, "Crossing", adtf::OBJCAT_DataFilter);

protected:
	cInputPin			m_pin_input_traveled_distance_left;          //!< traveld distance since boot
	cInputPin			m_pin_input_traveled_distance_right;         //!< traveld distance since boot
	cInputPin     	    m_pin_input_crossing_type;
	cInputPin			m_pin_input_reset;
	
	int					m_crossing_type;
	float				m_starting_distance_left, m_starting_distance_right;

	cOutputPin			m_pin_output_acceleration;
	cOutputPin			m_pin_output_steering_angle;

	float				m_acceleration;
	float				m_steering_angle;
	
	float				m_traveled_distance_left, m_traveled_distance_right;
	
	float				m_distance_to_travel_on_left_turn, m_distance_to_travel_on_right_turn, m_distance_to_travel_on_straight;
	float				m_steering_angle_for_left, m_steering_angle_for_right;
	float				m_distance_before;
	tInt8				m_status;
	

public:
    cCrossing(const tChar* __info);
    virtual ~cCrossing();

protected:
    tResult Init(tInitStage eStage, __exception);
    tResult Shutdown(tInitStage eStage, __exception);
    tResult PropertyChanged(const char* strProperty);

    // implements IPinEventSink
    tResult OnPinEvent(IPin* pSource,
                       tInt nEventCode,
                       tInt nParam1,
                       tInt nParam2,
                       IMediaSample* pMediaSample);

	tResult ProcessCrossing();
    
	tResult TransmitAcc(float t_accelerate);
	tResult TransmitAngle(float t_angle);
	//tResult TransmitAng(tTimeStamp sampleTimeStamp, const tTimeStamp timeStampValue, const tFloat32 ang);
	tResult TurnSignal(bool state);


	/*! Coder Descriptor for the output pins*/
	cObjectPtr<IMediaTypeDescription> m_pCoderDescSignalOutput;
    cObjectPtr<IMediaTypeDescription> m_pCoderDescBoolSignalOutput;
    
    cObjectPtr<IMediaTypeDescription> m_pCoderDescCrossingType;
    
    /*! Coder Descriptor for the input pins*/
	cObjectPtr<IMediaTypeDescription> m_pCoderDescSignalInput;
	cObjectPtr<IMediaTypeDescription> m_pCoderDescBoolSignalInput;
	
	tBool m_enabled;
};

//*************************************************************************************************
#endif // _CROSSING_PROJECT_FILTER_H_
