#include "stdafx.h"
#include "iirFilter.h"

#include <iostream>

/// Create filter shell
ADTF_FILTER_PLUGIN("IIR Filter", OID_ADTF_IIR_FILTER, cIIR);

cIIR::cIIR(const tChar* __info):cFilter(__info) {
	faktor = 50;
	SetPropertyFloat("Faktor", faktor);
	SetPropertyFloat("Faktor" NSSUBPROP_REQUIRED, tTrue);
}

cIIR::~cIIR() {

}

tResult cIIR::Init(tInitStage eStage, __exception) {
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
	RETURN_IF_FAILED(pTypeSignalValue->GetInterface(IID_ADTF_MEDIA_TYPE_DESCRIPTION, (tVoid**)&m_pCoderDescSignalValue)); 
	
	// create and register the input pin

	RETURN_IF_FAILED(m_pin_input.Create("input_value", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_input));

		
	// create and register the output pin
	RETURN_IF_FAILED(m_pin_output.Create("output_value", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&m_pin_output));


	// get FILTER_SHIFT value
	faktor = float(GetPropertyFloat("Faktor"));
	
	// Register auf 0 setzen
	filter_reg = 0;
	
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

tResult cIIR::Shutdown(tInitStage eStage, __exception)
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

tResult cIIR::OnPinEvent(IPin* pSource, tInt nEventCode, tInt nParam1, tInt nParam2, IMediaSample* pMediaSample)
{
    // first check what kind of event it is
	RETURN_IF_POINTER_NULL(pMediaSample);
	RETURN_IF_POINTER_NULL(pSource);

	if (nEventCode == IPinEventSink::PE_MediaSampleReceived)
	{	
		//write values with zero
		tFloat32 f32Value = 0;
		tUInt32  timeStamp = 0;
		if (m_pCoderDescSignalValue != NULL) {
		
			    {   // focus for sample read lock
			        __adtf_sample_read_lock_mediadescription(m_pCoderDescSignalValue,pMediaSample,pCoder);

					pCoder->Get("ui32ArduinoTimestamp", (tVoid*)&timeStamp);
			        pCoder->Get("f32Value", (tVoid*)&f32Value);
			    }

			float output = 0;
		
			output = (((filter_reg * faktor) + ((100 - faktor) * (float)f32Value))) / 100;
			filter_reg = output;
		
		
			timeStamp = 0;
			//create new media sample
			cObjectPtr<IMediaSample> pMediaSample;
			AllocMediaSample((tVoid**)&pMediaSample);

			//allocate memory with the size given by the descriptor
			cObjectPtr<IMediaSerializer> pSerializer;
			m_pCoderDescSignalValue->GetMediaSampleSerializer(&pSerializer);
			tInt nSize = pSerializer->GetDeserializedSize();
			pMediaSample->AllocBuffer(nSize);
			   
			//write date to the media sample with the coder of the descriptor
			{
				__adtf_sample_write_lock_mediadescription(m_pCoderDescSignalValue, pMediaSample, pCoder);
				
				pCoder->Set("f32Value", (tVoid*)&(output));    
				pCoder->Set("ui32ArduinoTimestamp", (tVoid*)&timeStamp);    
			}
		
			//transmit media sample over output pin
			pMediaSample->SetTime(_clock->GetStreamTime());
			m_pin_output.Transmit(pMediaSample);
		}
    }
	RETURN_NOERROR;
}
