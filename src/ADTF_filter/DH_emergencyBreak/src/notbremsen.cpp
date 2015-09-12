#include "stdafx.h"
#include "notbremsen.h"

#include <iostream>

/// Create filter shell
ADTF_FILTER_PLUGIN("Notbremsen", OID_ADTF_NOTBREMSE_FILTER, cNotbremse);

cNotbremse::cNotbremse(const tChar* __info):cFilter(__info) {
	m_sensor_front_long = m_sensor_front_short = 0;

	m_stopDistance = 8;
	SetPropertyFloat("Hard Stop Distance", m_stopDistance);
	SetPropertyFloat("Hard Stop Distance" NSSUBPROP_REQUIRED, tTrue);
	m_slowDistance15 = 15;
	SetPropertyFloat("Slow down to 15 Distance", m_slowDistance15);
	SetPropertyFloat("Slow down to 15 Distance" NSSUBPROP_REQUIRED, tTrue);
	m_slowDistance25 = 20;
	SetPropertyFloat("Slow down to 25 Distance", m_slowDistance25);
	SetPropertyFloat("Slow down to 25 Distance" NSSUBPROP_REQUIRED, tTrue);
	m_slowDistance30 = 50;
	SetPropertyFloat("Slow down to 30 Distance", m_slowDistance30);
	SetPropertyFloat("Slow down to 30 Distance" NSSUBPROP_REQUIRED, tTrue);
	m_slowDistance_back = 15;
	SetPropertyFloat("Back Distance", m_slowDistance_back);
	SetPropertyFloat("Back Distance" NSSUBPROP_REQUIRED, tTrue);
	m_stuck_time = 5;
	SetPropertyFloat("Stuck Time in s", m_stuck_time);
	SetPropertyFloat("Stuck Time in s" NSSUBPROP_REQUIRED, tTrue);
	m_stuck_distance = 10;
	SetPropertyFloat("Stuck Distance", m_stuck_distance);
	SetPropertyFloat("Stuck Distance" NSSUBPROP_REQUIRED, tTrue);
	
	m_cTimer = 0;
	t_accelerate_old = 0;
	m_hTimer = NULL;
}

cNotbremse::~cNotbremse() {

}

tResult cNotbremse::Init(tInitStage eStage, __exception) {
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
	RETURN_IF_FAILED(m_pin_input_ir_front_center_long.Create("ir_front_center_long", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_ir_front_center_long));
	RETURN_IF_FAILED(m_pin_input_ir_front_center_short.Create("ir_front_center_short", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_ir_front_center_short));
	RETURN_IF_FAILED(m_pin_input_ir_rear_center_short.Create("ir_rear_center_short", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_ir_rear_center_short));

	RETURN_IF_FAILED(m_pin_input_accelerate.Create("accelerate_in", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_accelerate));
	
	//RESET
	RETURN_IF_FAILED(m_pin_input_reset.Create("reset", pTypeBoolSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_reset));

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

	RETURN_IF_FAILED(m_pin_output_stuck.Create("got_stuck", pTypeBoolSignalValueOutput, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_output_stuck));

	// get properties
	m_stopDistance = tFloat32(GetPropertyFloat("Hard Stop Distance"));
	m_slowDistance15 = tFloat32(GetPropertyFloat("Slow down to 15 Distance"));
	m_slowDistance25 = tFloat32(GetPropertyFloat("Slow down to 25 Distance"));
	m_slowDistance30 = tFloat32(GetPropertyFloat("Slow down to 30 Distance"));
	m_slowDistance_back = tFloat32(GetPropertyFloat("Back Distance"));
	m_stuck_time = tFloat32(GetPropertyFloat("Stuck Time in s"));
	m_stuck_distance = tFloat32(GetPropertyFloat("Stuck Distance"));
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

tResult cNotbremse::Shutdown(tInitStage eStage, __exception)
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

tResult cNotbremse::OnPinEvent(IPin* pSource, tInt nEventCode, tInt nParam1, tInt nParam2, IMediaSample* pMediaSample)
{
    // first check what kind of event it is
	RETURN_IF_POINTER_NULL(pMediaSample);
	RETURN_IF_POINTER_NULL(pSource);

	if (nEventCode == IPinEventSink::PE_MediaSampleReceived)
	{	if (pSource == &m_pin_input_reset) {
			//write values with zero			
			tBool bValue = tFalse;			
			
			{   // focus for sample read lock
				// read-out the incoming Media Sample                
		        __adtf_sample_read_lock_mediadescription(m_pCoderDescBoolSignalInput,pMediaSample,pCoderInput);

		        //get values from media sample        
		        pCoderInput->Get("bValue", (tVoid*)&bValue);
			}
			
			
			if(bValue == tTrue) {
				m_gotStuck = tFalse;
				m_accelerate = 0;
				t_accelerate = 0;
				t_accelerate_old = 0;
				if(m_hTimer)
    				destroyTimer();
				m_cTimer = 0;
				TransmitAcc(0);
			}
		} else {
	
			//write values with zero
			tFloat32 signalValue = 0;
			tUInt32  timeStamp = 0;
			if (pMediaSample != NULL && m_pCoderDescSignalInput != NULL)
			{   // focus for sample read lock
				__adtf_sample_read_lock_mediadescription(m_pCoderDescSignalInput,pMediaSample,pCoderInput);

				pCoderInput->Get("ui32ArduinoTimestamp", (tVoid*)&timeStamp);
				pCoderInput->Get("f32Value", (tVoid*)&signalValue);
			}

			//Input Management
			if (pSource == &m_pin_input_accelerate) {
				m_accelerate = signalValue;
			} else if (pSource == &m_pin_input_ir_front_center_long) {
				m_sensor_front_long = signalValue;
			} else if (pSource == &m_pin_input_ir_front_center_short) {
				m_sensor_front_short = signalValue;
			} else if (pSource == &m_pin_input_ir_rear_center_short) {
				m_sensor_back = signalValue;
			}
		
			if(!m_gotStuck) {
				//Check, if not stuck
		
				t_accelerate = 0;

				if ((m_sensor_front_short < m_stopDistance) && (m_accelerate > 0)){
					t_accelerate = 0;
					if(!m_hTimer) {
						m_cTimer = 0;
						cout << "Notbremse! starte Timer...";
						createTimer();
					}
				} else if ((m_sensor_front_short < m_slowDistance15) && (m_accelerate > 15)) {
					t_accelerate = 15;
				} else if ((m_sensor_front_short < m_slowDistance25) && (m_accelerate > 25)) {
					t_accelerate = 25;
				} else if ((m_sensor_front_long < m_slowDistance30) && (m_accelerate > 30)) {
					t_accelerate = 30;
				} else if ((m_sensor_back < m_slowDistance_back) && (m_accelerate < -25)) {
					t_accelerate = -25;
				} else {
					t_accelerate = m_accelerate;
				}
		
				//Destroy Timer if still running but not stuck anymore
				if( m_hTimer && ((t_accelerate != 0) || (m_accelerate == 0)) ){
					cout << "Abbruch, zerstöre Timer!" << endl;
					destroyTimer();
					m_cTimer = 0;
				}
		
				if(t_accelerate != t_accelerate_old){
					TransmitAcc(t_accelerate);
					t_accelerate_old = t_accelerate;
				}
			}
		}
		
	}

    RETURN_NOERROR;
}

tResult cNotbremse::TransmitAcc(tFloat32 acc) {
    //write values with zero
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
        __adtf_sample_read_lock_mediadescription(m_pCoderDescSignalOutput, pMediaSample, pCoderOutput);

        pCoderOutput->Set("f32Value", (tVoid*)&(acc));    
        pCoderOutput->Set("ui32ArduinoTimestamp", (tVoid*)&timeStamp);    
    }

    //transmit media sample over output pin
    pMediaSample->SetTime(_clock->GetStreamTime());
	m_pin_output_accelerate.Transmit(pMediaSample);

    RETURN_NOERROR;
}

tResult cNotbremse::TransmitStuck(tBool state) {
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
	m_pin_output_stuck.Transmit(pMediaSample);

    RETURN_NOERROR;
}

tResult cNotbremse::Run(tInt nActivationCode, const tVoid* pvUserData, tInt szUserDataSize, ucom::IException** __exception_ptr/* =NULL */)
{
    if (nActivationCode == IRunnable::RUN_TIMER)
    {
		if(m_cTimer < (m_stuck_time * 10)){
			m_cTimer++;
			cout << m_cTimer << "...";
		} else {
			m_gotStuck = tTrue;
			cout << "Fahre zurück!" << endl;
		}
		
		if(m_gotStuck) {
			if( (m_sensor_front_short < m_stuck_distance) && (m_sensor_back > 10) ) {
				if(t_accelerate_old == 0){
					TransmitAcc(-25);
					t_accelerate_old = -25;
				}
			} else {
				if(t_accelerate_old != 0){
					TransmitAcc(0);
					t_accelerate_old = 0;
				}
				cout << "Fahre vorbei!" << endl;
				m_gotStuck = tFalse;
				m_accelerate = 0;
				TransmitStuck(tTrue);
				destroyTimer();
				m_cTimer = 0;
			}
		}
    }
    		    
    return cFilter::Run(nActivationCode, pvUserData, szUserDataSize, __exception_ptr);
}

tResult cNotbremse::createTimer()
{
     // creates timer with 0.5 sec
     __synchronized_obj(m_oCriticalSectionTimerSetup);
     if (m_hTimer == NULL)
     {
            m_hTimer = _kernel->TimerCreate(0.1*1000000, 0, static_cast<IRunnable*>(this),
                                        NULL, NULL, 0, 0, adtf_util::cString::Format("%s.timer", OIGetInstanceName()));
     }
     else
     {
        LOG_ERROR("Timer is already running. Unable to create a new one.");
     }
     RETURN_NOERROR;
}

tResult cNotbremse::destroyTimer(__exception)
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

tResult cNotbremse::Start(__exception)
{
    return cFilter::Start(__exception_ptr);
}

tResult cNotbremse::Stop(__exception)
{
	m_cTimer = 0;
	
    __synchronized_obj(m_oCriticalSectionTimerSetup);
    
	if(m_hTimer)
    	destroyTimer(__exception_ptr);
	

    return cFilter::Stop(__exception_ptr);
}
