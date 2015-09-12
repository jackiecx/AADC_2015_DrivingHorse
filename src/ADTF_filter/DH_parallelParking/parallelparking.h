#ifndef _PPARKING_FILTER_H_
#define _PPARKING_FILTER_H_

#define OID_ADTF_PPARKING_FILTER "adtf.user.PPARKING"


//*************************************************************************************************
class cPParking : public adtf::cFilter
{
    ADTF_FILTER(OID_ADTF_PPARKING_FILTER, "Parallel Parking", adtf::OBJCAT_DataFilter);

protected:
//	cInputPin    				m_pin_input_accelerate;
	
//	cInputPin               	m_pin_input_range_front_left;
//	cInputPin               	m_pin_input_range_front_right;
//	cInputPin               	m_pin_input_range_rear_left;
//	cInputPin               	m_pin_input_range_rear_right;

	cInputPin               	m_pin_input_ir_front_center_long;
	cInputPin              		m_pin_input_ir_front_center_short;
//	cInputPin               	m_pin_input_ir_front_right_long;
	cInputPin               	m_pin_input_ir_front_right_short;

	cInputPin     	            m_pin_input_ir_rear_center_short;
//	cInputPin               	m_pin_input_ir_rear_right_short;

	cInputPin					m_pin_input_distance_left;          //!< traveld distance since boot
	cInputPin					m_pin_input_distance_right;         //!< traveld distance since boot

//	cInputPin					m_pin_input_steering_angle_sensor;  //!< Current steering_angle meassurement
	
	cInputPin					m_pin_input_gyro_yaw;
//	cInputPin					m_pin_input_gyro_roll;
	
	cInputPin     	            m_pin_input_parallel_active;
	cInputPin     	            m_pin_input_cross_active;
	cInputPin					m_pin_input_reset;

	cOutputPin					m_pin_output_accelerate;
//	cOutputPin					m_pin_output_turn_left_enabled;
//	cOutputPin					m_pin_output_turn_right_enabled;
	cOutputPin					m_pin_output_steer_angle;


	tFloat32				m_accelerate;
	tFloat32 				m_sensor_front_long, m_sensor_front_short, m_sensor_back, m_sensor_front_right_short, m_sensor_front_right_long, m_gyro_yaw, m_gyro_yaw_old, m_yaw;
	tFloat32 				m_ussensor_back_left;
	tFloat32				m_distance_right, m_distance_left, m_distance;
	tFloat32				distance_a, distance_b, gyro_a;
	tFloat32				m_pdistanz_a, m_pdistanz_b, m_pwinkel1, m_pwinkel2, m_pparkluecke;
	tFloat32				m_cdistanz_a, m_cdistanz_b, m_cwinkel1, m_cwinkel2, m_cparkluecke;
	tFloat32				m_steering_angle;

	tInt32					status; // 1 => Parklücke suchen; 2 => Parklücke ausmessen; 3 => Parklücke gefunden; 4 => Rückwärts; 5 => Umlenken; 6 => Richtig positionieren
	tInt32					active;

public:
    cPParking(const tChar* __info);
    virtual ~cPParking();

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

	tResult ProcessPParking();
    
	tResult TransmitAcc(float t_accelerate);
	tResult TransmitAngle(float t_angle);
	tResult TurnSignal(bool state);



	tHandle m_hTimer;
	cCriticalSection m_oCriticalSectionTransmit;
	cCriticalSection m_oCriticalSectionTimerSetup;
	tResult createTimer();
	tResult destroyTimer(__exception = NULL);
	
	tResult Run(tInt nActivationCode, const tVoid* pvUserData, tInt szUserDataSize, ucom::IException** __exception_ptr=NULL);


	/*! Coder Descriptor for the output pins*/
	cObjectPtr<IMediaTypeDescription> m_pCoderDescSignalOutput;
    cObjectPtr<IMediaTypeDescription> m_pCoderDescBoolSignalOutput;
    
    /*! Coder Descriptor for the input pins*/
	cObjectPtr<IMediaTypeDescription> m_pCoderDescSignalInput;
	cObjectPtr<IMediaTypeDescription> m_pCoderDescBoolSignalInput;
};

//*************************************************************************************************
#endif // _PPARKING_PROJECT_FILTER_H_
