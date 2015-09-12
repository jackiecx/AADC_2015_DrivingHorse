#include "stdafx.h"
#include "crossing.h"

#include <iostream>

/// Create filter shell
ADTF_FILTER_PLUGIN("Crossing", OID_ADTF_CROSSING_FILTER, cCrossing);

template<typename T>
T abs(T v) {
	return (T)((v < (T)0) ? -v : v);
}

cCrossing::cCrossing(const tChar* __info) : cFilter(__info) {
	m_distance_to_travel_on_left_turn = 200;
	SetPropertyFloat("Distanz Linksabbiegen", m_distance_to_travel_on_left_turn);
	SetPropertyFloat("Distanz Linksabbiegen" NSSUBPROP_REQUIRED, tTrue);
	SetPropertyFloat("Distanz Linksabbiegen"  NSSUBPROP_ISCHANGEABLE, tTrue);
	
	m_distance_to_travel_on_right_turn = 150;
	SetPropertyFloat("Distanz Rechtsabbiegen", m_distance_to_travel_on_right_turn);
	SetPropertyFloat("Distanz Rechtsabbiegen" NSSUBPROP_REQUIRED, tTrue);
	SetPropertyFloat("Distanz Rechtsabbiegen"  NSSUBPROP_ISCHANGEABLE, tTrue);
	
	m_distance_to_travel_on_straight = 150;
	SetPropertyFloat("Distanz Geradeausfahren", m_distance_to_travel_on_straight);
	SetPropertyFloat("Distanz Geradeausfahren" NSSUBPROP_REQUIRED, tTrue);
	SetPropertyFloat("Distanz Geradeausfahren"  NSSUBPROP_ISCHANGEABLE, tTrue);
	
	m_steering_angle_for_left = -20;
	SetPropertyFloat("Lenkwinkel Linksabbiegen", m_steering_angle_for_left);
	SetPropertyFloat("Lenkwinkel Linksabbiegen" NSSUBPROP_REQUIRED, tTrue);
	SetPropertyFloat("Lenkwinkel Linksabbiegen"  NSSUBPROP_ISCHANGEABLE, tTrue);
	
	m_steering_angle_for_right = 40;
	SetPropertyFloat("Lenkwinkel Rechtsabbiegen", m_steering_angle_for_right);
	SetPropertyFloat("Lenkwinkel Rechtsabbiegen"  NSSUBPROP_REQUIRED, tTrue);
	SetPropertyFloat("Lenkwinkel Rechtsabbiegen"  NSSUBPROP_ISCHANGEABLE, tTrue);
	
	m_distance_before = 15;
	SetPropertyFloat("Distanz vor Abbiegen", m_distance_before);
	SetPropertyFloat("Distanz vor Abbiegen" NSSUBPROP_REQUIRED, tTrue);
	SetPropertyFloat("Distanz vor Abbiegen"  NSSUBPROP_ISCHANGEABLE, tTrue);
	
	m_enabled = tFalse;
	m_crossing_type = 0;
	m_status = 0;
	
	m_starting_distance_left = 0;
	m_traveled_distance_left = 0;
	m_starting_distance_right = 0; 
	m_traveled_distance_right = 0;
}

cCrossing::~cCrossing() {

}

tResult cCrossing::Init(tInitStage eStage, __exception) {
    // never miss calling the parent implementation!!
    RETURN_IF_FAILED(cFilter::Init(eStage, __exception_ptr))
    
    // in StageFirst you can create and register your static pins.
    if (eStage == StageFirst)
    {
        // get a media type for the input pin
		cObjectPtr<IMediaDescriptionManager> pDescManager;
        RETURN_IF_FAILED(_runtime->GetObject(OID_ADTF_MEDIA_DESCRIPTION_MANAGER,IID_ADTF_MEDIA_DESCRIPTION_MANAGER,(tVoid**)&pDescManager,__exception_ptr));
        
        tChar const * strDescCrossingType = pDescManager->GetMediaDescription("tCrossingType");
		if(!strDescCrossingType) LOG_ERROR(cString(OIGetInstanceName()) + ": Could not load mediadescription tCrossingType, check path");
		RETURN_IF_POINTER_NULL(strDescCrossingType);
		cObjectPtr<IMediaType> pTypeCrossingType = new cMediaType(0, 0, 0, "tCrossingType", strDescCrossingType, IMediaDescription::MDF_DDL_DEFAULT_VERSION);
//		RETURN_IF_FAILED(oPin_crossing.Create("crossingType_out", pTypeCrossingType, this));
//		RETURN_IF_FAILED(RegisterPin(&oPin_crossing));
		RETURN_IF_FAILED(pTypeCrossingType->GetInterface(IID_ADTF_MEDIA_TYPE_DESCRIPTION, (tVoid**)&m_pCoderDescCrossingType));
		RETURN_IF_FAILED(m_pin_input_crossing_type.Create("crossing_type", pTypeCrossingType, static_cast<IPinEventSink*> (this)));
		RETURN_IF_FAILED(RegisterPin(&m_pin_input_crossing_type));
        
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
	RETURN_IF_FAILED(m_pin_input_traveled_distance_left.Create("traveled_distance_left", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_traveled_distance_left));
	
	RETURN_IF_FAILED(m_pin_input_traveled_distance_right.Create("traveled_distance_right", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input_traveled_distance_right));
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
	RETURN_IF_FAILED(m_pin_output_acceleration.Create("acceleration", pTypeSignalValueOutput, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_output_acceleration));

	RETURN_IF_FAILED(m_pin_output_steering_angle.Create("steering_angle", pTypeSignalValueOutput, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_output_steering_angle));

	// get stop distance property
	m_distance_to_travel_on_left_turn = tFloat32(GetPropertyFloat("Distanz Linksabbiegen"));
	m_distance_to_travel_on_right_turn = tFloat32(GetPropertyFloat("Distanz Rechtsabbiegen"));
	m_distance_to_travel_on_straight = tFloat32(GetPropertyFloat("Distanz Geradeausfahren"));
	
	m_steering_angle_for_left = tFloat32(GetPropertyFloat("Lenkwinkel Linksabbiegen"));
	m_steering_angle_for_right = tFloat32(GetPropertyFloat("Lenkwinkel Rechtsabbiegen"));
	m_distance_before = tFloat32(GetPropertyFloat("Distanz vor Abbiegen"));
	
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

tResult cCrossing::Shutdown(tInitStage eStage, __exception)
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

/*tResult cCrossing::TransmitAng(tTimeStamp sampleTimeStamp, const tTimeStamp timeStampValue, const tFloat32 ang) {    
    //create new media sample
    cObjectPtr<IMediaSample> pMediaSample;
    AllocMediaSample((tVoid**)&pMediaSample);

    //allocate memory with the size given by the descriptor
    cObjectPtr<IMediaSerializer> pSerializer;
    m_pCoderDescSignalOutput->GetMediaSampleSerializer(&pSerializer);
    tInt nSize = pSerializer->GetDeserializedSize();
    pMediaSample->AllocBuffer(nSize);
      
    //write date to the media sample with the coder of the descriptor    
    {   // focus for sample write lock
        __adtf_sample_write_lock_mediadescription(m_pCoderDescSignalOutput,pMediaSample,pCoderOutput);
        pCoderOutput->Set("f32Value", (tVoid*)&(ang));    
        pCoderOutput->Set("ui32ArduinoTimestamp", (tVoid*)&timeStampValue);
    }    
    //transmit media sample over output pin
    pMediaSample->SetTime(sampleTimeStamp);
    m_pin_output_steering_angle.Transmit(pMediaSample);
    
    RETURN_NOERROR;
}*/

tResult cCrossing::OnPinEvent(IPin* pSource, tInt nEventCode, tInt nParam1, tInt nParam2, IMediaSample* pMediaSample)
{

    // first check what kind of event it is
	RETURN_IF_POINTER_NULL(pMediaSample);
	RETURN_IF_POINTER_NULL(pSource);

	if (nEventCode == IPinEventSink::PE_MediaSampleReceived)
	{     

		//INPUT
		
		if (pSource == &m_pin_input_crossing_type && m_pCoderDescCrossingType != NULL && m_enabled == tFalse) {
		    tInt8 i8CrossingType = -2;
		    
		        {   // focus for sample read lock
		            __adtf_sample_read_lock_mediadescription(m_pCoderDescCrossingType,pMediaSample,pCoder);

		            pCoder->Get("i8Identifier", (tVoid*)&i8CrossingType);
		        }
               
            if(i8CrossingType>0)
            {
            	m_enabled = tTrue;
            	m_crossing_type = i8CrossingType;
            	m_starting_distance_left = m_traveled_distance_left;
				m_starting_distance_right = m_traveled_distance_right;
				switch(m_crossing_type)
            	{
                	case 1:
                		TransmitAcc(25);
                		cout << "Biege links ab...";
                		m_status = 1;
                	break;
                	case 2:
                		TransmitAcc(23);
                		TransmitAngle(0);
                		cout << "Fahre gerade aus...";
                		m_status = 2;
                	break;
                	case 3:
                		TransmitAcc(26);
                		cout << "Biege rechts ab...";
                		m_status = 1;
            		case 4:
	            		TransmitAngle(m_steering_angle_for_left);
		        		TransmitAcc(25);
		        		cout << "Parke links aus...";
		        		m_status = 2;
                	break;
            	}
           	} else {
            	m_crossing_type = 0;
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
			
			
			if((bValue == tTrue) && (m_enabled  != tFalse)) {
				m_crossing_type = 0;
				m_enabled = tFalse;
			}
		} else if ((pSource == &m_pin_input_traveled_distance_left || pSource == &m_pin_input_traveled_distance_right) && m_pCoderDescSignalInput != NULL) {
			
			
			//write values with zero
			tFloat32 signalValue = 0;
			tUInt32  timeStamp = 0;
			
				{
				// read-out the incoming Media Sample                
		        __adtf_sample_read_lock_mediadescription(m_pCoderDescSignalInput,pMediaSample,pCoderInput);

		        //get values from media sample        
		        pCoderInput->Get("f32Value", (tVoid*)&signalValue);
		        pCoderInput->Get("ui32ArduinoTimestamp", (tVoid*)&timeStamp);
		        }		
			
			if (pSource == &m_pin_input_traveled_distance_left){
				m_traveled_distance_left = signalValue;
			}else if (pSource == &m_pin_input_traveled_distance_right){
				m_traveled_distance_right = signalValue;
			}
			if(m_status == 1) {
			    
				switch(m_crossing_type)
            	{
                	case 1:
                		if(m_traveled_distance_left > 10 + m_starting_distance_left){
		            		TransmitAngle(m_steering_angle_for_left);
		            		m_starting_distance_left = m_traveled_distance_left;
							m_starting_distance_right = m_traveled_distance_right;
							m_status = 2;
	            		}
                	break;
                	case 2:
                		TransmitAngle(0);
                	break;
                	case 3:
                		if(m_traveled_distance_right > m_distance_before + m_starting_distance_right){
		            		TransmitAngle(m_steering_angle_for_right);
		            		m_starting_distance_left = m_traveled_distance_left;
							m_starting_distance_right = m_traveled_distance_right;
							m_status = 2;
	            		}
                	break;
            	}		
			} else if(m_status == 2){
				switch(m_crossing_type)
				{
					case 1:
						if(m_traveled_distance_right > m_distance_to_travel_on_left_turn + m_starting_distance_right)
						{
							TransmitAngle(400);
							m_crossing_type = 0;
							m_enabled = tFalse;
							cout << "Fertig!" << endl;
						}
					break;
					case 2:
						if(m_traveled_distance_left > m_distance_to_travel_on_right_turn + m_starting_distance_left)
						{
							TransmitAngle(400);
							m_crossing_type = 0;
							m_enabled = tFalse;
							cout << "Fertig!" << endl;
						}
					break;
					case 3:
						if(m_traveled_distance_left > m_distance_to_travel_on_straight + m_starting_distance_left)
						{
							TransmitAngle(400);
							m_crossing_type = 0;
							m_enabled = tFalse;
							cout << "Fertig!" << endl;
						}
					case 4:
						if(m_traveled_distance_left > m_distance_to_travel_on_straight + m_starting_distance_left)
						{
							TransmitAngle(400);
							m_crossing_type = 0;
							m_enabled = tFalse;
							cout << "Fertig!" << endl;
						}
					break;
				}
			}
			
		}
	}

    RETURN_NOERROR;
}

tResult cCrossing::PropertyChanged(const char* strProperty) {

	m_distance_to_travel_on_left_turn = tFloat32(GetPropertyFloat("Distanz Linksabbiegen"));
	m_distance_to_travel_on_right_turn = tFloat32(GetPropertyFloat("Distanz Rechtsabbiegen"));
	m_distance_to_travel_on_straight = tFloat32(GetPropertyFloat("Distanz Geradeausfahren"));
	
	m_steering_angle_for_left = tFloat32(GetPropertyFloat("Lenkwinkel Linksabbiegen"));
	m_steering_angle_for_right = tFloat32(GetPropertyFloat("Lenkwinkel Rechtsabbiegen"));
	m_distance_before = tFloat32(GetPropertyFloat("Distanz vor Abbiegen"));

	RETURN_NOERROR;
}

tResult cCrossing::TransmitAcc(float t_accelerate) {
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
    m_pin_output_acceleration.Transmit(pMediaSample);
	
    RETURN_NOERROR;
}

tResult cCrossing::TransmitAngle(float t_angle) {
	tFloat32 flValue = (tFloat32) t_angle;
    tUInt32 timeStamp = 0;
                    
    //LOG_INFO(cString::Format("slot speed: %f",flValue));    
    
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
    m_pin_output_steering_angle.Transmit(pMediaSample);
	
    RETURN_NOERROR;
}
