#include "stdafx.h"
#include "cDHCruiseControl.h"
#include <iostream>

ADTF_FILTER_PLUGIN("DH CruiseControl", OID_ADTF_DHCruiseControl_FILTER, cDHCruiseControl);

cDHCruiseControl::cDHCruiseControl(const tChar* __info):cFilter(__info), m_hTimer(NULL) {
	/*SetPropertyFloat("Faktor", faktor);
	SetPropertyFloat("Faktor" NSSUBPROP_REQUIRED, tTrue);*/
	
	m_acc = 0;
	m_rpm = 0;
	m_rpm_left = 0;
	m_rpm_right = 0;
	m_acc_in = 0;
}

cDHCruiseControl::~cDHCruiseControl() {}

tResult cDHCruiseControl::Init(tInitStage eStage, __exception) {
    RETURN_IF_FAILED(cFilter::Init(eStage, __exception_ptr))
    
    if (eStage == StageFirst) {
		cObjectPtr<IMediaDescriptionManager> pDescManager;
		RETURN_IF_FAILED(_runtime->GetObject(OID_ADTF_MEDIA_DESCRIPTION_MANAGER,IID_ADTF_MEDIA_DESCRIPTION_MANAGER,(tVoid**)&pDescManager,__exception_ptr));
		    
		//input descriptor
		tChar const * strDescSignalValue = pDescManager->GetMediaDescription("tSignalValue");
		RETURN_IF_POINTER_NULL(strDescSignalValue);        
		cObjectPtr<IMediaType> pTypeSignalValue = new cMediaType(0, 0, 0, "tSignalValue", strDescSignalValue,IMediaDescription::MDF_DDL_DEFAULT_VERSION);	
		RETURN_IF_FAILED(pTypeSignalValue->GetInterface(IID_ADTF_MEDIA_TYPE_DESCRIPTION, (tVoid**)&m_pCoderDescSignalValue)); 
		
		// create and register the input pin
		RETURN_IF_FAILED(iPin_acc.Create("acc_in", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
		RETURN_IF_FAILED(RegisterPin(&iPin_acc));
		RETURN_IF_FAILED(iPin_rpm_right.Create("RPM_right_in", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
		RETURN_IF_FAILED(RegisterPin(&iPin_rpm_right));
		RETURN_IF_FAILED(iPin_rpm_left.Create("RPM_left_in", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
		RETURN_IF_FAILED(RegisterPin(&iPin_rpm_left));

		// create and register the output pin
		RETURN_IF_FAILED(oPin_acc.Create("acc_out", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
		RETURN_IF_FAILED(RegisterPin(&oPin_acc));
    } else if (eStage == StageNormal) {} else if (eStage == StageGraphReady) {}

    RETURN_NOERROR;
}

tResult cDHCruiseControl::Shutdown(tInitStage eStage, __exception) {
    if (eStage == StageGraphReady) {}
    else if (eStage == StageNormal) {}
    else if (eStage == StageFirst) {}

    // call the base class implementation
    return cFilter::Shutdown(eStage, __exception_ptr);
}

tResult cDHCruiseControl::OnPinEvent(IPin* pSource, tInt nEventCode, tInt nParam1, tInt nParam2, IMediaSample* pMediaSample) {
	RETURN_IF_POINTER_NULL(pMediaSample);
	RETURN_IF_POINTER_NULL(pSource);

	if (nEventCode == IPinEventSink::PE_MediaSampleReceived) {	
		tUInt32 ui32ArduinoTimestamp = 0;
		tFloat32 f32Value = 0;
		
		if (m_pCoderDescSignalValue != NULL && pMediaSample != NULL) {
	        {   // focus for sample read lock
	            __adtf_sample_read_lock_mediadescription(m_pCoderDescSignalValue,pMediaSample,pCoderInput);

				pCoderInput->Get("f32Value", (tVoid*)&f32Value);
				pCoderInput->Get("ui32ArduinoTimestamp", (tVoid*)&ui32ArduinoTimestamp);
	        }
	        
	        if ((pSource == &iPin_rpm_right) || (pSource == &iPin_rpm_left)) {
			    if (pSource == &iPin_rpm_right) {
			    	m_rpm_right = f32Value;
			    } else if (pSource == &iPin_rpm_left) {
			    	m_rpm_left = f32Value;
			    } 
			    
			    m_rpm = (m_rpm_right + m_rpm_right) / 2;
			      
			} else if (pSource == &iPin_acc) {	
				if((f32Value != 0) && (!m_hTimer)) {
					createTimer();
				} else if ((f32Value == 0) && m_hTimer) {
					destroyTimer();
				}
					
				m_acc_in = f32Value;
				m_acc = m_acc_in;
				TransmitValue(m_acc);
	        }
	    }
    }
	RETURN_NOERROR;
}

tResult cDHCruiseControl::TransmitValue(float value) {
    /*tFloat32 flValue= (tFloat32)(value);
    tUInt32 timeStamp = 0;
                            
    //create new media sample
    cObjectPtr<IMediaSample> pMediaSample;
    RETURN_IF_FAILED(AllocMediaSample((tVoid**)&pMediaSample));

    //allocate memory with the size given by the descriptor
    cObjectPtr<IMediaSerializer> pSerializer;
    m_pCoderDescSignalValue->GetMediaSampleSerializer(&pSerializer);
    tInt nSize = pSerializer->GetDeserializedSize();
    RETURN_IF_FAILED(pMediaSample->AllocBuffer(nSize));
    
    cObjectPtr<IMediaCoder> pCoder;
    RETURN_IF_FAILED(m_pCoderDescSignalValue->WriteLock(pMediaSample, &pCoder));
    
    pCoder->Set("f32Value", (tVoid*)&(flValue));    
    pCoder->Set("ui32ArduinoTimestamp", (tVoid*)&timeStamp);
    
    m_pCoderDescSignalValue->Unlock(pCoder);
    
    //transmit media sample over output pin
    pMediaSample->SetTime(_clock->GetStreamTime());
    oPin_acc.Transmit(pMediaSample);*/
       


    tFloat32 flValue= (tFloat32)(value);
    tUInt32 timeStamp = 0;
                            
    //create new media sample
    cObjectPtr<IMediaSample> pMediaSample;
    AllocMediaSample((tVoid**)&pMediaSample);

    //allocate memory with the size given by the descriptor
    cObjectPtr<IMediaSerializer> pSerializer;
    m_pCoderDescSignalValue->GetMediaSampleSerializer(&pSerializer);
    tInt nSize = pSerializer->GetDeserializedSize();
    RETURN_IF_FAILED(pMediaSample->AllocBuffer(nSize));
       
    //write date to the media sample with the coder of the descriptor
    {
        __adtf_sample_write_lock_mediadescription(m_pCoderDescSignalValue, pMediaSample, pCoderOutput);
        
        pCoderOutput->Set("f32Value", (tVoid*)&(flValue));    
        pCoderOutput->Set("ui32ArduinoTimestamp", (tVoid*)&timeStamp);    
    }
    
    //transmit media sample over output pin
    pMediaSample->SetTime(_clock->GetStreamTime());
    oPin_acc.Transmit(pMediaSample);
	
    RETURN_NOERROR;
}

tResult cDHCruiseControl::Run(tInt nActivationCode, const tVoid* pvUserData, tInt szUserDataSize, ucom::IException** __exception_ptr/* =NULL */)
{
    if (nActivationCode == IRunnable::RUN_TIMER)
    {		
		if(m_rpm == 0)  {
			if(m_acc_in > 0) m_acc = m_acc + 2;
			else m_acc = m_acc - 2;
			cout << "Gebe mehr Gas - jetzt: " << m_acc << endl;
			TransmitValue(m_acc);
	   	} else if ((m_rpm > 10) && (m_acc_in != m_acc)) {
	   		if(m_acc_in > 0) {
	   			m_acc = m_acc - 2;
	   			cout << "Gebe weniger Gas - jetzt: " << m_acc << endl;
	   			TransmitValue(m_acc);
	   		}
	   	}
    }
    return cFilter::Run(nActivationCode, pvUserData, szUserDataSize, __exception_ptr);
}

tResult cDHCruiseControl::createTimer()
{
     // creates timer with 0.5 sec
     __synchronized_obj(m_oCriticalSectionTimerSetup);
     // additional check necessary because input jury structs can be mixed up because every signal is sent three times
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

tResult cDHCruiseControl::destroyTimer(__exception)
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

tResult cDHCruiseControl::Start(__exception)
{
    return cFilter::Start(__exception_ptr);
}

tResult cDHCruiseControl::Stop(__exception)
{
    __synchronized_obj(m_oCriticalSectionTimerSetup);
    
    if(m_hTimer)
    destroyTimer(__exception_ptr);

    return cFilter::Stop(__exception_ptr);
}
