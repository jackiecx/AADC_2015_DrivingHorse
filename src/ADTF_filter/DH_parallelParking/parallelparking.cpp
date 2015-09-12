#include "stdafx.h"
#include "parallelparking.h"

#include <iostream>

/// Create filter shell
ADTF_FILTER_PLUGIN("Parallel Parking", OID_ADTF_PPARKING_FILTER, cPParking);

template<typename T>
T abs(T v) {
	return (T)((v < (T)0) ? -v : v);
}

cPParking::cPParking(const tChar* __info):cFilter(__info) {
	m_sensor_front_long = m_sensor_front_short = 0;

	m_pparkluecke = 80;
	SetPropertyFloat("Parallel Parkluecke", m_pparkluecke);
	SetPropertyFloat("Parallel Parkluecke" NSSUBPROP_REQUIRED, tTrue);
	m_pdistanz_a = 100;
	SetPropertyFloat("Parallel Distanz A", m_pdistanz_a);
	SetPropertyFloat("Parallel Distanz A" NSSUBPROP_REQUIRED, tTrue);
	m_pdistanz_b = 180;
	SetPropertyFloat("Parallel Distanz B", m_pdistanz_b);
	SetPropertyFloat("Parallel Distanz B" NSSUBPROP_REQUIRED, tTrue);
	m_pwinkel1 = 30;
	SetPropertyFloat("Parallel Winkel 1", m_pwinkel1);
	SetPropertyFloat("Parallel Winkel 1" NSSUBPROP_REQUIRED, tTrue);
	m_pwinkel2 = 30;
	SetPropertyFloat("Parallel Winkel 2", m_pwinkel2);
	SetPropertyFloat("Parallel Winkel 2" NSSUBPROP_REQUIRED, tTrue);
	
	m_cparkluecke = 35;
	SetPropertyFloat("Cross Parkluecke", m_cparkluecke);
	SetPropertyFloat("Cross Parkluecke" NSSUBPROP_REQUIRED, tTrue);
	m_cdistanz_a = 50;
	SetPropertyFloat("Cross Distanz A", m_cdistanz_a);
	SetPropertyFloat("Cross Distanz A" NSSUBPROP_REQUIRED, tTrue);
	m_cwinkel1 = 30;
	SetPropertyFloat("Cross Winkel 1", m_cwinkel1);
	SetPropertyFloat("Cross Winkel 1" NSSUBPROP_REQUIRED, tTrue);
	m_cwinkel2 = 90;
	SetPropertyFloat("Cross Winkel 2", m_cwinkel2);
	SetPropertyFloat("Cross Winkel 2" NSSUBPROP_REQUIRED, tTrue);
	m_cdistanz_b = 180;
	SetPropertyFloat("Cross Distanz B", m_cdistanz_b);
	SetPropertyFloat("Cross Distanz B" NSSUBPROP_REQUIRED, tTrue);
	
}

cPParking::~cPParking() {

}

tResult cPParking::Init(tInitStage eStage, __exception) {
    // never miss calling the parent implementation!!
    RETURN_IF_FAILED(cFilter::Init(eStage, __exception_ptr))
    
    // in StageFirst you can create and register your static pins.
    if (eStage == StageFirst)
    {
        // get a media type for the input pin
		cObjectPtr<IMediaDescriptionManager> pDescManager;
        RETURN_IF_FAILED(_runtime->GetObject(OID_ADTF_MEDIA_DESCRIPTION_MANAGER,IID_ADTF_MEDIA_DESCRIPTION_MANAGER,(tVoid**)&pDescManager,__exception_ptr));
        
        //input descriptor
        tChar const * strDescSignalValue = pDescManager->GetMediaDescription("tSignalValue");
        RETURN_IF_POINTER_NULL(strDescSignalValue);        
        cObjectPtr<IMediaType> pTypeSignalValue = new cMediaType(0, 0, 0, "tSignalValue", strDescSignalValue,IMediaDescription::MDF_DDL_DEFAULT_VERSION);	
        RETURN_IF_FAILED(pTypeSignalValue->GetInterface(IID_ADTF_MEDIA_TYPE_DESCRIPTION, (tVoid**)&m_pCoderDescSignalInput)); 
		
		tChar const * strDescBoolSignalValueInput = pDescManager->GetMediaDescription("tBoolSignalValue");
        RETURN_IF_POINTER_NULL(strDescBoolSignalValueInput);        
        cObjectPtr<IMediaType> pTypeBoolSignalValue = new cMediaType(0, 0, 0, "tBoolSignalValue", strDescBoolSignalValueInput,IMediaDescription::MDF_DDL_DEFAULT_VERSION);	
		RETURN_IF_FAILED(pTypeBoolSignalValue->GetInterface(IID_ADTF_MEDIA_TYPE_DESCRIPTION, (tVoid**)&m_pCoderDescBoolSignalInput)); 
		
		
        // create and register the input pin

//USS
/*	RETURN_IF_FAILED(m_pin_input_range_front_left.Create("range_front_left", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_range_front_left));
	RETURN_IF_FAILED(m_pin_input_range_front_right.Create("range_front_right", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_range_front_right));
	RETURN_IF_FAILED(m_pin_input_range_rear_left.Create("range_rear_left", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_range_rear_left));
	RETURN_IF_FAILED(m_pin_input_range_rear_right.Create("range_rear_right", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_range_rear_right));*/

//IR
	RETURN_IF_FAILED(m_pin_input_ir_front_center_long.Create("ir_front_center_long", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_ir_front_center_long));
	RETURN_IF_FAILED(m_pin_input_ir_front_center_short.Create("ir_front_center_short", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_ir_front_center_short));

/*	RETURN_IF_FAILED(m_pin_input_ir_front_right_long.Create("ir_front_right_long", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_ir_front_right_long));*/
	RETURN_IF_FAILED(m_pin_input_ir_front_right_short.Create("ir_front_right_short", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_ir_front_right_short));

	RETURN_IF_FAILED(m_pin_input_ir_rear_center_short.Create("ir_rear_center_short", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_ir_rear_center_short));
/*	RETURN_IF_FAILED(m_pin_input_ir_rear_right_short.Create("ir_rear_right_short", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_ir_rear_right_short));*/

//DISTANCE
	RETURN_IF_FAILED(m_pin_input_distance_left.Create("distance_left", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_distance_left));
	RETURN_IF_FAILED(m_pin_input_distance_right.Create("distance_right", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_distance_right));

//ACC
/*	RETURN_IF_FAILED(m_pin_input_accelerate.Create("accelerate_in", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_accelerate));

//ANGLE
	RETURN_IF_FAILED(m_pin_input_steering_angle_sensor.Create("steering_angle", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_steering_angle_sensor));*/

//PARALLEL PARKING ACTIVE
	RETURN_IF_FAILED(m_pin_input_parallel_active.Create("activate_parallel_parking", pTypeBoolSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_parallel_active));
	
//CROSS PARKING ACTIVE
	RETURN_IF_FAILED(m_pin_input_cross_active.Create("activate_cross_parking", pTypeBoolSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_cross_active));
	
//RESET
	RETURN_IF_FAILED(m_pin_input_reset.Create("reset", pTypeBoolSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_reset));
	
//GYRO
	RETURN_IF_FAILED(m_pin_input_gyro_yaw.Create("yaw", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_gyro_yaw));
/*	RETURN_IF_FAILED(m_pin_input_gyro_roll.Create("roll", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_gyro_roll));*/

        // get a media type for the output pin
	tChar const * strDescSignalValueOutput = pDescManager->GetMediaDescription("tSignalValue");
        RETURN_IF_POINTER_NULL(strDescSignalValueOutput);        
        cObjectPtr<IMediaType> pTypeSignalValueOutput = new cMediaType(0, 0, 0, "tSignalValue", strDescSignalValueOutput,IMediaDescription::MDF_DDL_DEFAULT_VERSION);	
        RETURN_IF_FAILED(pTypeSignalValueOutput->GetInterface(IID_ADTF_MEDIA_TYPE_DESCRIPTION, (tVoid**)&m_pCoderDescSignalOutput)); 
        
        tChar const * strDescBoolSignalValue = pDescManager->GetMediaDescription("tBoolSignalValue");
        RETURN_IF_POINTER_NULL(strDescBoolSignalValue);        
        cObjectPtr<IMediaType> pTypeBoolSignalValueOutput = new cMediaType(0, 0, 0, "tBoolSignalValue", strDescBoolSignalValue,IMediaDescription::MDF_DDL_DEFAULT_VERSION);	
	RETURN_IF_FAILED(pTypeBoolSignalValueOutput->GetInterface(IID_ADTF_MEDIA_TYPE_DESCRIPTION, (tVoid**)&m_pCoderDescBoolSignalOutput)); 
        
        // create and register the output pin
	RETURN_IF_FAILED(m_pin_output_accelerate.Create("accelerate_out", pTypeSignalValueOutput, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_output_accelerate));

	RETURN_IF_FAILED(m_pin_output_steer_angle.Create("steerAngle", pTypeSignalValueOutput, static_cast<IPinEventSink*> (this)));
        RETURN_IF_FAILED(RegisterPin(&m_pin_output_steer_angle));

/*	RETURN_IF_FAILED(m_pin_output_turn_left_enabled.Create("turnSignalLeftEnabled", pTypeBoolSignalValueOutput, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_output_turn_left_enabled));
	RETURN_IF_FAILED(m_pin_output_turn_right_enabled.Create("turnSignalRightEnabled", pTypeBoolSignalValueOutput, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_output_turn_right_enabled));*/


	// get properties
	m_pparkluecke = tFloat32(GetPropertyFloat("Parallel Parkluecke"));
	m_pdistanz_a = tFloat32(GetPropertyFloat("Parallel Distanz A"));
	m_pdistanz_b = tFloat32(GetPropertyFloat("Parallel Distanz B"));
	m_pwinkel1 = tFloat32(GetPropertyFloat("Parallel Winkel 1"));
	m_pwinkel2 = tFloat32(GetPropertyFloat("Parallel Winkel 2"));
	m_cparkluecke = tFloat32(GetPropertyFloat("Cross Parkluecke"));
	m_cdistanz_a = tFloat32(GetPropertyFloat("Cross Distanz A"));
	m_cdistanz_b = tFloat32(GetPropertyFloat("Cross Distanz B"));
	m_cwinkel1 = tFloat32(GetPropertyFloat("Cross Winkel 1"));
	m_cwinkel2 = tFloat32(GetPropertyFloat("Cross Winkel 2"));
	
	//set distance, status and active
	distance_a = 0;
	active = 0;
	status = 0;
	gyro_a = 0;
	m_yaw = 0;
	m_hTimer = NULL;
    }
    else if (eStage == StageNormal)
    {
        // In this stage you would do further initialisation and/or create your dynamic pins.
        // Please take a look at the demo_dynamicpin example for further reference.
    }
    else if (eStage == StageGraphReady)
    {
        // All pin connections have been established in this stage so you can query your pins
        // about their media types and additional meta data.
        // Please take a look at the demo_imageproc example for further reference.
    }

    RETURN_NOERROR;
}

tResult cPParking::Shutdown(tInitStage eStage, __exception)
{
    // In each stage clean up everything that you initiaized in the corresponding stage during Init.
    // Pins are an exception: 
    // - The base class takes care of static pins that are members of this class.
    // - Dynamic pins have to be cleaned up in the ReleasePins method, please see the demo_dynamicpin
    //   example for further reference.
    
    if (eStage == StageGraphReady)
    {
    }
    else if (eStage == StageNormal)
    {
    }
    else if (eStage == StageFirst)
    {
    }

    // call the base class implementation
    return cFilter::Shutdown(eStage, __exception_ptr);
}

tResult cPParking::OnPinEvent(IPin* pSource, tInt nEventCode, tInt nParam1, tInt nParam2, IMediaSample* pMediaSample)
{
    // first check what kind of event it is
	RETURN_IF_POINTER_NULL(pMediaSample);
	RETURN_IF_POINTER_NULL(pSource);

	if (nEventCode == IPinEventSink::PE_MediaSampleReceived)
	{     

		//INPUT
		
		//Aktivieren
		if (pSource == &m_pin_input_parallel_active) {
			
			cout << "SIGNAL AUF INPUT PARALLEL ACITVE" << endl;
			
			//write values with zero			
			tBool bValue = tFalse;		
				
			{   // focus for sample read lock
				// read-out the incoming Media Sample                
		        __adtf_sample_read_lock_mediadescription(m_pCoderDescBoolSignalInput,pMediaSample,pCoderInput);

		        //get values from media sample        
		        pCoderInput->Get("bValue", (tVoid*)&bValue);
			}
			
			if((bValue == tTrue) && (active == 0)) {
				active = 1;
				TransmitAcc(25);
				createTimer();
				status = 1;
				cout << "status = 1" << endl;
			}
		} else if (pSource == &m_pin_input_cross_active) {
			
			//write values with zero			
			tBool bValue = tFalse;			
			
			{   // focus for sample read lock
				// read-out the incoming Media Sample                
		        __adtf_sample_read_lock_mediadescription(m_pCoderDescBoolSignalInput,pMediaSample,pCoderInput);

		        //get values from media sample        
		        pCoderInput->Get("bValue", (tVoid*)&bValue);
			}
			
			
			if((bValue == tTrue) && (active == 0)) {
				active = 2;
				TransmitAcc(25);
				createTimer();
				status = 1;
				cout << "status = 1" << endl;
			}
		} else if (pSource == &m_pin_input_reset) {
			//write values with zero			
			tBool bValue = tFalse;			
			
			{   // focus for sample read lock
				// read-out the incoming Media Sample                
		        __adtf_sample_read_lock_mediadescription(m_pCoderDescBoolSignalInput,pMediaSample,pCoderInput);

		        //get values from media sample        
		        pCoderInput->Get("bValue", (tVoid*)&bValue);
			}
			
			
			if((bValue == tTrue) && (active != 0)) {
				TransmitAcc(0);
				TransmitAngle(0);
				distance_a = 0;
				active = 0;
				status = 0;
				gyro_a = 0;
				m_yaw = 0;
				destroyTimer();
			}
		} else {
			
			
			//write values with zero
			tFloat32 signalValue = 0;
			tUInt32  timeStamp = 0;
			
			{   // focus for sample read lock
				// read-out the incoming Media Sample                
		        __adtf_sample_read_lock_mediadescription(m_pCoderDescSignalInput,pMediaSample,pCoderInput);

		        //get values from media sample        
		        pCoderInput->Get("f32Value", (tVoid*)&signalValue);
		        pCoderInput->Get("ui32ArduinoTimestamp", (tVoid*)&timeStamp);		
			}
			
			//Lücke suchen
			if (pSource == &m_pin_input_ir_front_right_short) {
				m_sensor_front_right_short = signalValue;
			}
			else if ((pSource == &m_pin_input_distance_right) || (pSource == &m_pin_input_distance_left)) {
				if (pSource == &m_pin_input_distance_left) {
					m_distance_left = signalValue;
				}
				if (pSource == &m_pin_input_distance_right) {
					m_distance_right = signalValue;
				}
				m_distance = (m_distance_right + m_distance_left) / 2;
			}
			else if (pSource == &m_pin_input_ir_front_center_short) {
				m_sensor_front_short = signalValue;
			} 
			
			/*else if (pSource == &m_pin_input_accelerate) {
				m_accelerate = signalValue;
			} */
			
			else if (pSource == &m_pin_input_ir_front_center_long) {
				m_sensor_front_long = signalValue;
			} 
			
			/*else if (pSource == &m_pin_input_ir_front_right_long) {
				m_sensor_front_right_long = signalValue;
			} */
			
			/*else if (pSource == &m_pin_input_range_rear_left) {
				m_ussensor_back_left = signalValue;
			}*/
			
			else if (pSource == &m_pin_input_ir_rear_center_short) {
				m_sensor_back = signalValue;
			} 
			
			/*else if (pSource == &m_pin_input_ir_rear_right_short) {
				m_sensor_back_right = signalValue;
			} */
			
			/*else if (pSource == &m_pin_input_steering_angle_sensor) {
				m_steering_angle = signalValue;
			} */
			
			/*else if (pSource == &m_pin_input_gyro_roll) {
				m_gyro_roll = signalValue;
			} */
			
			else if (pSource == &m_pin_input_gyro_yaw) {
				m_gyro_yaw = signalValue;

			} 
		}
	}

    RETURN_NOERROR;
}

tResult cPParking::TransmitAcc(float t_accelerate) {
    tFloat32 flValue= (tFloat32)(t_accelerate);
    tUInt32 timeStamp = 0;
                            
    //create new media sample
    cObjectPtr<IMediaSample> pMediaSample;
    AllocMediaSample((tVoid**)&pMediaSample);

    //allocate memory with the size given by the descriptor
    cObjectPtr<IMediaSerializer> pSerializer;
    m_pCoderDescSignalOutput->GetMediaSampleSerializer(&pSerializer);
    tInt nSize = pSerializer->GetDeserializedSize();
    pMediaSample->AllocBuffer(nSize);
       
    //write date to the media sample with the coder of the descriptor
    {
        __adtf_sample_write_lock_mediadescription(m_pCoderDescSignalOutput, pMediaSample, pCoder);
        
        pCoder->Set("f32Value", (tVoid*)&(flValue));    
        pCoder->Set("ui32ArduinoTimestamp", (tVoid*)&timeStamp);    
    }
    
    //transmit media sample over output pin
    pMediaSample->SetTime(_clock->GetStreamTime());
    m_pin_output_accelerate.Transmit(pMediaSample);
	
    RETURN_NOERROR;
}

tResult cPParking::TransmitAngle(float t_angle) {
	tFloat32 flValue = (tFloat32) t_angle;
    tUInt32 timeStamp = 0;
                    
    //LOG_INFO(cString::Format("slot speed: %f",flValue));    
    
    //create new media sample
    cObjectPtr<IMediaSample> pMediaSample;
    AllocMediaSample((tVoid**)&pMediaSample);

    //allocate memory with the size given by the descriptor3000
    cObjectPtr<IMediaSerializer> pSerializer;
    m_pCoderDescSignalOutput->GetMediaSampleSerializer(&pSerializer);
    tInt nSize = pSerializer->GetDeserializedSize();
    pMediaSample->AllocBuffer(nSize);
       
    //write date to the media sample with the coder of the descriptor
    {
        __adtf_sample_write_lock_mediadescription(m_pCoderDescSignalOutput, pMediaSample, pCoder);
        
        pCoder->Set("f32Value", (tVoid*)&(flValue));    
        pCoder->Set("ui32ArduinoTimestamp", (tVoid*)&timeStamp);    
    }
    //transmit media sample over output pin
    pMediaSample->SetTime(_clock->GetStreamTime());
    m_pin_output_steer_angle.Transmit(pMediaSample);
	
    RETURN_NOERROR;
}

/*tResult cPParking::TurnSignal(bool state) {
    //write values with zero
    tUInt32 timeStamp = 0;
    
    //create new media sample
    cObjectPtr<IMediaSample> pMediaSample;
    AllocMediaSample((tVoid**)&pMediaSample);

    //allocate memory with the size given by the descriptor
    cObjectPtr<IMediaSerializer> pSerializer;
    m_pCoderDescBoolSignalOutput->GetMediaSampleSerializer(&pSerializer);
    tInt nSize = pSerializer->GetDeserializedSize();
    pMediaSample->AllocBuffer(nSize);
       
    //write date to the media sample with the coder of the descriptor
    {

        __adtf_sample_read_lock_mediadescription(m_pCoderDescBoolSignalOutput, pMediaSample, pCoderOutput);

        pCoderOutput->Set("bValue", (tVoid*)&(state));    
        pCoderOutput->Set("ui32ArduinoTimestamp", (tVoid*)&timeStamp);    
    
    }

    //transmit media sample over output pin
    pMediaSample->SetTime(_clock->GetStreamTime());
    m_pin_output_turn_right_enabled.Transmit(pMediaSample);
    
    RETURN_NOERROR;
}*/


tResult cPParking::Run(tInt nActivationCode, const tVoid* pvUserData, tInt szUserDataSize, ucom::IException** __exception_ptr/* =NULL */)
{
    if (nActivationCode == IRunnable::RUN_TIMER)
    {
    	if (active > 0) {
    	   	m_yaw = m_yaw + abs(m_gyro_yaw - m_gyro_yaw_old);
     		m_gyro_yaw_old = m_gyro_yaw;
     	}
     	
     	//PARALLEL PARKEN
     	
     	if (active == 1) {
			if((status == 1) && (m_sensor_front_right_short < 20)) {
				status = 2;
				cout << "status = 2" << endl;					
			}
			if((status == 2) && (m_sensor_front_right_short > 20)) {
				status = 3;
				distance_a = m_distance;
				cout << "status = 3" << endl;					
			}
			if((status == 3) && (m_sensor_front_right_short < 20)) {
				if((m_distance_right - distance_a) < m_pparkluecke) {
					status = 1;
				}
			}
			if((status == 3) && ((m_distance - distance_a) > m_pdistanz_a)) {
				status = 4;
				TransmitAcc(23);
			cout << "status = 4" << endl;					
			}
			if((status == 4) && ((m_distance - distance_a) > m_pdistanz_b)) {
				TransmitAcc(-35);
				status = 5;
				TransmitAngle(23);
			cout << "status = 5" << endl;					
				m_yaw = 0;
				m_gyro_yaw_old = m_gyro_yaw;
			}
			if((status == 5) && (m_yaw > m_pwinkel1)) {
				status = 6;
				TransmitAngle(-23);
				m_yaw = 0;
				m_gyro_yaw_old = m_gyro_yaw;
				cout << "status = 6" << endl;
			}
			if((status == 6) && ((m_yaw > 45) || (m_sensor_back < 8))) {
				TransmitAcc(0);
				TransmitAngle(400);
				distance_a = 0;
				active = 0;
				status = 0;
				gyro_a = 0;
				m_yaw = 0;
				destroyTimer();
			} 
			
			
			//CROSS PARKEN
		} else if(active == 2) {
			if((status == 1) && (m_sensor_front_right_short < 20)) {
				status = 2;
				cout << "status = 2" << endl;					
			}
			if((status == 2) && (m_sensor_front_right_short > 20)) {
				status = 3;
				distance_a = m_distance;
				cout << "status = 3" << endl;					
			}
			if((status == 3) && (m_sensor_front_right_short < 20)) {
				if((m_distance_right - distance_a) < m_cparkluecke) {
					status = 1;
				}
			}
			if((status == 3) && ((m_distance - distance_a) > m_cdistanz_a)) {
				m_yaw = 0;
				m_gyro_yaw_old = m_gyro_yaw;
				status = 4;
				TransmitAcc(23);
				TransmitAngle(-23);
				cout << "status = 4" << endl;					
			}
			if((status == 4) && (m_yaw > m_pwinkel1)) {
				TransmitAcc(-35);
				TransmitAngle(23);
				status = 5;
				cout << "status = 5" << endl;					
			}
			if((status == 5) && (m_yaw > m_pwinkel2)) {
				TransmitAngle(0);
				status = 6;
				m_yaw = 0;
				m_gyro_yaw_old = m_gyro_yaw;
				cout << "status = 6" << endl;
			}
			if(((status == 6) && (m_sensor_front_right_short > 10)) || (m_sensor_back < 8)) {
				TransmitAcc(0);
				TransmitAngle(400);
				distance_a = 0;
				active = 0;
				status = 0;
				gyro_a = 0;
				m_yaw = 0;
				destroyTimer();
			} 		
		}     	
    }
    

    
    return cFilter::Run(nActivationCode, pvUserData, szUserDataSize, __exception_ptr);
}

tResult cPParking::createTimer()
{
     // creates timer with 0.05 sec
     __synchronized_obj(m_oCriticalSectionTimerSetup);
     if (m_hTimer == NULL)
     {
            m_hTimer = _kernel->TimerCreate(0.05*1000000, 0, static_cast<IRunnable*>(this),
                                        NULL, NULL, 0, 0, adtf_util::cString::Format("%s.timer", OIGetInstanceName()));
     }
     else
     {
        LOG_ERROR("Timer is already running. Unable to create a new one.");
     }
     RETURN_NOERROR;
}

tResult cPParking::destroyTimer(__exception)
{
    __synchronized_obj(m_oCriticalSectionTimerSetup);
    //destroy timer
    if (m_hTimer != NULL) 
    {        
        tResult nResult = _kernel->TimerDestroy(m_hTimer);
        if (IS_FAILED(nResult))
        {
            LOG_ERROR("Unable to destroy the timer.");
            THROW_ERROR(nResult);
        }
        m_hTimer = NULL;
    }
    //check if handle for some unknown reason still exists
    else       
    {
        LOG_WARNING("Timer handle not set, but I should destroy the timer. Try to find a timer with my name.");
        tHandle hFoundHandle = _kernel->FindHandle(adtf_util::cString::Format("%s.timer", OIGetInstanceName()));
        if (hFoundHandle)
        {
            tResult nResult = _kernel->TimerDestroy(hFoundHandle);
            if (IS_FAILED(nResult))
            {
                LOG_ERROR("Unable to destroy the found timer.");
                THROW_ERROR(nResult);
            }
        }
    }

    RETURN_NOERROR;
}

tResult cPParking::Start(__exception)
{
    return cFilter::Start(__exception_ptr);
}

tResult cPParking::Stop(__exception)
{
    __synchronized_obj(m_oCriticalSectionTimerSetup);
    
    if(m_hTimer)
    	destroyTimer(__exception_ptr);

    return cFilter::Stop(__exception_ptr);
}
