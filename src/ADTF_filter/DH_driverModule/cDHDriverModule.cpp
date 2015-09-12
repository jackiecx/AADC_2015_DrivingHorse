/**
Copyright (c) 
Audi Autonomous Driving Cup. All rights reserved.
 
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3.  All advertising materials mentioning features or use of this software must display the following acknowledgement: “This product includes software developed by the Audi AG and its contributors for Audi Autonomous Driving Cup.”
4.  Neither the name of Audi nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY AUDI AG AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL AUDI AG OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


**********************************************************************
* $Author:: forchhe#$  $Date:: 2014-09-11 10:10:54#$ $Rev:: 25921   $
**********************************************************************/
#include "stdafx.h"
#include "cDHDriverModule.h"

#include <iostream>

ADTF_FILTER_PLUGIN("DH Driver Module", __guid, cDHDriverModule);

#define SC_PROP_FILENAME "ManeuverFile"


cDHDriverModule::cDHDriverModule(const tChar* __info) : cFilter(__info), m_bDebugModeEnabled(tFalse), m_hTimer(NULL) {
	m_acc = 0;
	m_ang = 0;

	SetPropertyBool("Debug Output to Console",false);

    SetPropertyStr(SC_PROP_FILENAME, "");
    SetPropertyBool(SC_PROP_FILENAME NSSUBPROP_FILENAME, tTrue); 
    SetPropertyStr(SC_PROP_FILENAME NSSUBPROP_FILENAME NSSUBSUBPROP_EXTENSIONFILTER, "XML Files (*.xml)");
    SetPropertyStr(SC_PROP_FILENAME NSSUBPROP_DESCRIPTION, "The maneuver file that should be processed.");
    
    SetPropertyInt("minimum roadsign size", 1000);
	SetPropertyBool("minimum roadsign size" NSSUBPROP_ISCHANGEABLE, tTrue);
	SetPropertyInt("maximum roadsign size", 1500);
	SetPropertyBool("maximum roadsign size" NSSUBPROP_ISCHANGEABLE, tTrue);
	SetPropertyInt("minimum parksign size", 900);
	SetPropertyBool("minimum parksign size" NSSUBPROP_ISCHANGEABLE, tTrue);
	SetPropertyInt("maximum parksign size", 4000);
	SetPropertyBool("maximum parksign size" NSSUBPROP_ISCHANGEABLE, tTrue);
	
	SetPropertyInt("default acceleration", 23);
	SetPropertyBool("default acceleration" NSSUBPROP_ISCHANGEABLE, tTrue);
	SetPropertyInt("delayed acceleration", 20);
	SetPropertyBool("delayed acceleration" NSSUBPROP_ISCHANGEABLE, tTrue);
	
	
    
    m_eMuxAng = eMuxAngLaneAssist;
    m_eMuxAcc = eMuxAccDefault;
    
    m_turnSignalState = 0;
    
    m_stopped = 0;
    m_stopped_left = tFalse;
    m_stopped_straight = tFalse;
    m_stopped_right = tFalse;
    m_parked = tFalse;
}

cDHDriverModule::~cDHDriverModule()
{
}

tResult cDHDriverModule::Init(tInitStage eStage, __exception)
{
    RETURN_IF_FAILED(cFilter::Init(eStage, __exception_ptr));

    // pins need to be created at StageFirst
    if (eStage == StageFirst)    
    {
        
        cObjectPtr<IMediaDescriptionManager> pDescManager;
        RETURN_IF_FAILED(_runtime->GetObject(OID_ADTF_MEDIA_DESCRIPTION_MANAGER,
                                             IID_ADTF_MEDIA_DESCRIPTION_MANAGER,
                                             (tVoid**)&pDescManager,
                                             __exception_ptr));
        /*
        * the MediaDescription for <struct name="tJuryNotAusFlag" .../> has to exist in a description file (e.g. in $ADTF_DIR\description\ or $ADTF_DIR\src\examples\src\description
        * before (!) you start adtf_devenv !! if not: the Filter-Plugin will not loaded because cPin.Create() and so ::Init() failes !
        */

		// input jury struct
		tChar const * strDescJuryStruct = pDescManager->GetMediaDescription("tJuryStruct");
		RETURN_IF_POINTER_NULL(strDescJuryStruct);
		cObjectPtr<IMediaType> pTypeJuryStruct = new cMediaType(0, 0, 0, "tJuryStruct", strDescJuryStruct, IMediaDescription::MDF_DDL_DEFAULT_VERSION);
		RETURN_IF_FAILED(iPin_juryStruct.Create("Jury_Struct", pTypeJuryStruct, this));
		RETURN_IF_FAILED(RegisterPin(&iPin_juryStruct));
		RETURN_IF_FAILED(pTypeJuryStruct->GetInterface(IID_ADTF_MEDIA_TYPE_DESCRIPTION, (tVoid**)&m_pCoderDescJuryStruct));

		// output driver struct
		tChar const * strDescDriverStruct = pDescManager->GetMediaDescription("tDriverStruct");
		RETURN_IF_POINTER_NULL(strDescDriverStruct);
		cObjectPtr<IMediaType> pTypeDriverStruct = new cMediaType(0, 0, 0, "tDriverStruct", strDescDriverStruct, IMediaDescription::MDF_DDL_DEFAULT_VERSION);
		RETURN_IF_FAILED(oPin_driverStruct.Create("Driver_Struct", pTypeDriverStruct, this));
		RETURN_IF_FAILED(RegisterPin(&oPin_driverStruct));
		RETURN_IF_FAILED(pTypeDriverStruct->GetInterface(IID_ADTF_MEDIA_TYPE_DESCRIPTION, (tVoid**)&m_pCoderDescDriverStruct));
		
		tChar const * strDescRoadSign = pDescManager->GetMediaDescription("tRoadSign");
		if(!strDescRoadSign) LOG_ERROR(cString(OIGetInstanceName()) + ": Could not load mediadescription tRoadSign, check path");
		RETURN_IF_POINTER_NULL(strDescRoadSign);
		cObjectPtr<IMediaType> pTypeRoadSign = new cMediaType(0, 0, 0, "tRoadSign", strDescRoadSign, IMediaDescription::MDF_DDL_DEFAULT_VERSION);
		RETURN_IF_FAILED(iPin_roadSign.Create("roadSign_in", pTypeRoadSign, this));
		RETURN_IF_FAILED(RegisterPin(&iPin_roadSign));
		RETURN_IF_FAILED(pTypeRoadSign->GetInterface(IID_ADTF_MEDIA_TYPE_DESCRIPTION, (tVoid**)&m_pCoderDescRoadSign));

		// inout signal value struct
		tChar const * strDescSignalValue = pDescManager->GetMediaDescription("tSignalValue");
		RETURN_IF_POINTER_NULL(strDescSignalValue);
		cObjectPtr<IMediaType> pTypeSignalValue = new cMediaType(0, 0, 0, "tSignalValue", strDescSignalValue, IMediaDescription::MDF_DDL_DEFAULT_VERSION);
		RETURN_IF_FAILED(oPin_acc.Create("acc_out", pTypeSignalValue, this));
		RETURN_IF_FAILED(RegisterPin(&oPin_acc));
		RETURN_IF_FAILED(oPin_ang.Create("ang_out", pTypeSignalValue, this));
		RETURN_IF_FAILED(RegisterPin(&oPin_ang));
		RETURN_IF_FAILED(pTypeSignalValue->GetInterface(IID_ADTF_MEDIA_TYPE_DESCRIPTION, (tVoid**)&m_pCoderDescSignalValue));
		
		// inout crossing type struct
		tChar const * strDescCrossingType = pDescManager->GetMediaDescription("tCrossingType");
		RETURN_IF_POINTER_NULL(strDescCrossingType);
		cObjectPtr<IMediaType> pTypeCrossingType = new cMediaType(0, 0, 0, "tCrossingType", strDescCrossingType, IMediaDescription::MDF_DDL_DEFAULT_VERSION);
		
		// inout bool type struct
        tChar const * strDescBoolSignalValue = pDescManager->GetMediaDescription("tBoolSignalValue");    
        RETURN_IF_POINTER_NULL(strDescBoolSignalValue);        
        cObjectPtr<IMediaType> pTypeBoolSignalValue = new cMediaType(0, 0, 0, "tBoolSignalValue", strDescBoolSignalValue,IMediaDescription::MDF_DDL_DEFAULT_VERSION);    
        RETURN_IF_FAILED(pTypeBoolSignalValue->GetInterface(IID_ADTF_MEDIA_TYPE_DESCRIPTION, (tVoid**)&m_pCoderDescBoolSignalValue));
		
		RETURN_IF_FAILED(iPin_laneAssist_ang.Create("laneAssist_ang_in", pTypeSignalValue, this));
		RETURN_IF_FAILED(RegisterPin(&iPin_laneAssist_ang));
		RETURN_IF_FAILED(iPin_crossing_ang.Create("crossing_ang_in", pTypeSignalValue, this));
		RETURN_IF_FAILED(RegisterPin(&iPin_crossing_ang));
		RETURN_IF_FAILED(iPin_crossing_acc.Create("crossing_acc_in", pTypeSignalValue, this));
		RETURN_IF_FAILED(RegisterPin(&iPin_crossing_acc));
		RETURN_IF_FAILED(iPin_parking_ang.Create("parking_ang_in", pTypeSignalValue, this));
		RETURN_IF_FAILED(RegisterPin(&iPin_parking_ang));
		RETURN_IF_FAILED(iPin_parking_acc.Create("parking_acc_in", pTypeSignalValue, this));
		RETURN_IF_FAILED(RegisterPin(&iPin_parking_acc));
		RETURN_IF_FAILED(iPin_lineChange_ang.Create("lineChange_ang_in", pTypeSignalValue, this));
		RETURN_IF_FAILED(RegisterPin(&iPin_lineChange_ang));
		RETURN_IF_FAILED(iPin_stuck.Create("stuck_in", pTypeBoolSignalValue, this));
		RETURN_IF_FAILED(RegisterPin(&iPin_stuck));
		

        RETURN_IF_FAILED(oPin_turnSignalLeft.Create("turnSignalLeft_out", pTypeBoolSignalValue, this));
		RETURN_IF_FAILED(RegisterPin(&oPin_turnSignalLeft));
		RETURN_IF_FAILED(oPin_turnSignalRight.Create("turnSignalRight_out", pTypeBoolSignalValue, this));
		RETURN_IF_FAILED(RegisterPin(&oPin_turnSignalRight));
		RETURN_IF_FAILED(oPin_turnSignalBoth.Create("turnSignalBoth_out", pTypeBoolSignalValue, this));
		RETURN_IF_FAILED(RegisterPin(&oPin_turnSignalBoth));
		RETURN_IF_FAILED(oPin_parallelParking.Create("parallel_parking_out", pTypeBoolSignalValue, this));
		RETURN_IF_FAILED(RegisterPin(&oPin_parallelParking));
		RETURN_IF_FAILED(oPin_crossParking.Create("cross_parking_out", pTypeBoolSignalValue, this));
		RETURN_IF_FAILED(RegisterPin(&oPin_crossParking));
		RETURN_IF_FAILED(oPin_reset.Create("reset_out", pTypeBoolSignalValue, this));
		RETURN_IF_FAILED(RegisterPin(&oPin_reset));
		RETURN_IF_FAILED(oPin_crossingTrigger.Create("crossingTrigger_out", pTypeCrossingType, this));
		RETURN_IF_FAILED(RegisterPin(&oPin_crossingTrigger));
		RETURN_IF_FAILED(pTypeCrossingType->GetInterface(IID_ADTF_MEDIA_TYPE_DESCRIPTION, (tVoid**)&m_pCoderDescCrossingType));
		RETURN_IF_FAILED(oPin_lineChangeTrigger.Create("lineChangeTrigger_out", pTypeCrossingType, this));
		RETURN_IF_FAILED(RegisterPin(&oPin_lineChangeTrigger));
		RETURN_IF_FAILED(pTypeCrossingType->GetInterface(IID_ADTF_MEDIA_TYPE_DESCRIPTION, (tVoid**)&m_pCoderDescCrossingType));
    
        
        m_bDebugModeEnabled = GetPropertyBool("Debug Output to Console");
        
        m_i16CurrentManeuverID = 0;

        m_state = stateCar_STARTUP; 

    }
    else if(eStage == StageGraphReady)
    {
        if IS_OK(loadManeuverList())
        {
            m_i16SectionListIndex = 0;
            m_i16ManeuverListIndex = 0;
        }
        else
        {
            m_i16SectionListIndex = -1;
            m_i16ManeuverListIndex =-1;
        }
    } else if (eStage == StageNormal) {
    	m_minRoadSignSize = GetPropertyInt("minimum roadsign size");
    	m_maxRoadSignSize = GetPropertyInt("maximum roadsign size");
    	m_minParkSignSize = GetPropertyInt("minimum parksign size");
    	m_maxParkSignSize = GetPropertyInt("maximum parksign size");
    	m_defaultAcc = GetPropertyInt("default acceleration");
    	m_delayedAcc = GetPropertyInt("delayed acceleration");
    }
    RETURN_NOERROR;
}

tResult cDHDriverModule::PropertyChanged(const char* strProperty) {
	m_minRoadSignSize = GetPropertyInt("minimum roadsign size");
	m_maxRoadSignSize = GetPropertyInt("maximum roadsign size");
	m_minParkSignSize = GetPropertyInt("minimum parksign size");
    m_maxParkSignSize = GetPropertyInt("maximum parksign size");
	m_defaultAcc = GetPropertyInt("default acceleration");
	m_delayedAcc = GetPropertyInt("delayed acceleration");
	
	RETURN_NOERROR;
}

tResult cDHDriverModule::Start(__exception)
{
    return cFilter::Start(__exception_ptr);
}

tResult cDHDriverModule::Run(tInt nActivationCode, const tVoid* pvUserData, tInt szUserDataSize, ucom::IException** __exception_ptr/* =NULL */)
{
    if (nActivationCode == IRunnable::RUN_TIMER)
    {
        TransmitDriverStruct(m_state,m_i16CurrentManeuverID);
        
        
        //Stopp
        if((0 < m_stopped) && (m_stopped < 7)){
        	m_stopped++;
        	cout << (int)m_stopped << "...";
        }
        else if(m_stopped == 7){
        	if(m_stopped_left){
				TransmitCrossingType(1);
				m_stopped_left = tFalse;
			}
			else if(m_stopped_straight){
				TransmitCrossingType(2);
				m_stopped_straight = tFalse;
			}
			else if(m_stopped_right){
				TransmitCrossingType(3);
				m_stopped_right = tFalse;
			}
			else if(m_parked){
				if(m_turnSignalState == 3){
					TransmitTurnSignalBoth(tFalse);
					m_turnSignalState = 0;
				}
	            m_eMuxAcc = eMuxAccDefault;
		        
		        if (getActionFromManeuverID(m_i16CurrentManeuverID - 1) == eActionCrossParking) {
					if (getActionFromManeuverID(m_i16CurrentManeuverID) == eActionPullOutLeft) {
						cout << "Parke nach links aus" << endl;
						m_eMuxAng = eMuxAngCrossing;
						TransmitAcc(m_acc = m_defaultAcc);
						TransmitCrossingType(10);
						TransmitTurnSignalLeft(tTrue);
						m_turnSignalState = 1;
					} else if (getActionFromManeuverID(m_i16CurrentManeuverID) == eActionPullOutRight) {
						cout << "Parke nach rechts aus" << endl;
						m_eMuxAng = eMuxAngCrossing;
						TransmitAcc(m_acc = m_defaultAcc);
						TransmitCrossingType(9);
						TransmitTurnSignalRight(tTrue);
						m_turnSignalState = 2;
					}
				} else if (getActionFromManeuverID(m_i16CurrentManeuverID) == eActionPullOutRight) {
					cout << "Parke nach rechts aus" << endl;
					m_eMuxAng = eMuxAngLineChange;
					TransmitAcc(m_acc = m_defaultAcc);
					TransmitLineChangeType(1);
					TransmitTurnSignalLeft(tTrue);
					m_turnSignalState = 1;
				}
				m_parked = tFalse;
			}
			m_stopped = 0;
        }
    }
    
    return cFilter::Run(nActivationCode, pvUserData, szUserDataSize, __exception_ptr);
}

tResult cDHDriverModule::Stop(__exception)
{
    __synchronized_obj(m_oCriticalSectionTimerSetup);
    
    if(m_hTimer)
    	destroyTimer(__exception_ptr);

    return cFilter::Stop(__exception_ptr);
}

tResult cDHDriverModule::Shutdown(tInitStage eStage, __exception)
{     
    return cFilter::Shutdown(eStage, __exception_ptr);
}

tResult cDHDriverModule::OnPinEvent(IPin* pSource, tInt nEventCode, tInt nParam1, tInt nParam2, IMediaSample* pMediaSample)
{

    RETURN_IF_POINTER_NULL(pMediaSample);
    RETURN_IF_POINTER_NULL(pSource);

    if (nEventCode == IPinEventSink::PE_MediaSampleReceived)
    {      
       
        if (pSource == &iPin_juryStruct) {
            tInt8 i8ActionID = -2;
            tInt16 i16entry = -1;
            
            {   // focus for sample read lock
                __adtf_sample_read_lock_mediadescription(m_pCoderDescJuryStruct,pMediaSample,pCoder);

                pCoder->Get("i8ActionID", (tVoid*)&i8ActionID);
                pCoder->Get("i16ManeuverEntry", (tVoid*)&i16entry);              
            }
            //change the state depending on the input
            // action_GETREADY --> stateCar_READY
            // action_START --> stateCar_RUNNING
            // action_STOP --> stateCar_STARTUP
            switch (juryActions(i8ActionID))
            {
                case action_GETREADY:
                    if(m_bDebugModeEnabled)  LOG_INFO(cString::Format("State Controller: Received: Request Ready with maneuver ID %d",i16entry));
                    changeState(stateCar_READY);
                    setManeuverID(i16entry);
                    break;
                case action_START:
                	if(m_state != stateCar_RUNNING) {
		                if(m_bDebugModeEnabled)  LOG_INFO(cString::Format("State Controller: Received: Run with maneuver ID %d",i16entry));
		                if (i16entry == m_i16CurrentManeuverID) {
		                    changeState(stateCar_RUNNING);
		                } else {
		                    LOG_WARNING("StateController: The id of the action_START corresponds not with the id of the last action_GETREADY");
		                    setManeuverID(i16entry);
		                    changeState(stateCar_RUNNING);
		                }
		                
		                m_eMuxAng = eMuxAngLaneAssist;
  						m_eMuxAcc = eMuxAccDefault;
		                TransmitAcc(m_acc = m_defaultAcc);
		                
						if (getActionFromManeuverID(m_i16CurrentManeuverID) == eActionPullOutLeft) {
							m_eMuxAng = eMuxAngCrossing;
							TransmitCrossingType(10);
							TransmitTurnSignalLeft(tTrue);
							m_turnSignalState = 1;
						} else if (getActionFromManeuverID(m_i16CurrentManeuverID) == eActionPullOutRight) {
							if(m_i16CurrentManeuverID == 0) {
								m_eMuxAng = eMuxAngCrossing;
								TransmitCrossingType(3);
								TransmitTurnSignalRight(tTrue);
								m_turnSignalState = 2;
							} else if(getActionFromManeuverID(m_i16CurrentManeuverID - 1) == eActionCrossParking) {
								m_eMuxAng = eMuxAngCrossing;
								TransmitCrossingType(3);
								TransmitTurnSignalRight(tTrue);
								m_turnSignalState = 2;
							} else {
								m_eMuxAng = eMuxAngLineChange;
								TransmitLineChangeType(1);
								TransmitTurnSignalRight(tTrue);
								m_turnSignalState = 2;
							}
						}
					} else LOG_INFO(cString::Format("State Running already reached. Signal ignored!",i16entry));
	                break;
	            case action_STOP:
	                if(m_bDebugModeEnabled)  LOG_INFO(cString::Format("State Controller: Received: Stop with maneuver ID %d",i16entry));
	                changeState(stateCar_STARTUP);
           			TransmitAng(m_ang = 0);
					TransmitAcc(m_acc = 0);
	                TransmitReset(tTrue);
                    m_stopped = 0;
					m_stopped_left = tFalse;
					m_stopped_straight = tFalse;
					m_stopped_right = tFalse;
					m_parked = tFalse;
					if(m_turnSignalState != 0) {
		        		switch(m_turnSignalState){
		        			case 1: TransmitTurnSignalLeft(tFalse); break;
		        			case 2: TransmitTurnSignalRight(tFalse); break;
		        			case 3: TransmitTurnSignalBoth(tFalse); break;
		        			default: break;
		        		}
		        	}
	                break;
            }
        } else if (m_state == stateCar_RUNNING) {
        	if (pSource == &iPin_laneAssist_ang && m_pCoderDescSignalValue != NULL) {
		    	tUInt32 ui32ArduinoTimestamp = 0;
				tFloat32 f32Value = 0;
		        
		        {   // focus for sample read lock
		            __adtf_sample_read_lock_mediadescription(m_pCoderDescSignalValue,pMediaSample,pCoder);

					pCoder->Get("ui32ArduinoTimestamp", (tVoid*)&ui32ArduinoTimestamp);
		            pCoder->Get("f32Value", (tVoid*)&f32Value);
		        }
		        
		        if (m_eMuxAng == eMuxAngLaneAssist) TransmitAng(m_ang = f32Value);
		    } else if (pSource == &iPin_stuck && m_pCoderDescBoolSignalValue != NULL) {
		    	tUInt32 ui32ArduinoTimestamp = 0;
				tBool bValue = 0;
		        
		        {   // focus for sample read lock
		            __adtf_sample_read_lock_mediadescription(m_pCoderDescBoolSignalValue,pMediaSample,pCoder);
					
		            pCoder->Get("bValue", (tVoid*)&bValue);
					pCoder->Get("ui32ArduinoTimestamp", (tVoid*)&ui32ArduinoTimestamp);
		        }
		        
		        if(bValue){
		        	TransmitLineChangeType(3);
		        	m_eMuxAng = eMuxAngLineChange;
		        	TransmitAcc(m_acc = m_defaultAcc);
		        }
   

		    } else if (pSource == &iPin_crossing_ang && m_pCoderDescSignalValue != NULL) {
		    	tUInt32 ui32ArduinoTimestamp = 0;
				tFloat32 f32Value = 0;
		        
		        {   // focus for sample read lock
		            __adtf_sample_read_lock_mediadescription(m_pCoderDescSignalValue,pMediaSample,pCoder);

					pCoder->Get("ui32ArduinoTimestamp", (tVoid*)&ui32ArduinoTimestamp);
		            pCoder->Get("f32Value", (tVoid*)&f32Value);
		        }
		        
		        if (f32Value > 200) {
		        	std::cout << "RESET" << std::endl;
		        	
		        	m_eMuxAng = eMuxAngLaneAssist;
		        	m_eMuxAcc = eMuxAccDefault;
		        	incrementManeuverID();
		        	
		        	TransmitAcc(m_acc = m_defaultAcc);
		        	
		        	if(m_turnSignalState != 0) {
		        		switch(m_turnSignalState){
		        			case 1: TransmitTurnSignalLeft(tFalse); break;
		        			case 2: TransmitTurnSignalRight(tFalse); break;
		        			case 3: TransmitTurnSignalBoth(tFalse); break;
		        			default: break;
		        		}
		        	}
		        }
		    
		    	if (m_eMuxAng == eMuxAngCrossing) TransmitAng(m_ang = f32Value);
		    } else if (pSource == &iPin_crossing_acc && m_pCoderDescSignalValue != NULL) {
		    	tUInt32 ui32ArduinoTimestamp = 0;
				tFloat32 f32Value = 0;
		        
		        {   // focus for sample read lock
		            __adtf_sample_read_lock_mediadescription(m_pCoderDescSignalValue,pMediaSample,pCoder);

					pCoder->Get("ui32ArduinoTimestamp", (tVoid*)&ui32ArduinoTimestamp);
		            pCoder->Get("f32Value", (tVoid*)&f32Value);
		        }
		    
		    	if (m_eMuxAcc == eMuxAccCrossing) TransmitAcc(m_acc = f32Value);
		    } else if (pSource == &iPin_parking_acc && m_pCoderDescSignalValue != NULL) {
		    	tUInt32 ui32ArduinoTimestamp = 0;
				tFloat32 f32Value = 0;
		        
		        {   // focus for sample read lock
		            __adtf_sample_read_lock_mediadescription(m_pCoderDescSignalValue,pMediaSample,pCoder);

					pCoder->Get("ui32ArduinoTimestamp", (tVoid*)&ui32ArduinoTimestamp);
		            pCoder->Get("f32Value", (tVoid*)&f32Value);
		        }
		    
		    	if (m_eMuxAcc == eMuxAccParking) TransmitAcc(m_acc = f32Value);
		    	if ((f32Value < 0) && (m_eMuxAng != eMuxAngParking)) m_eMuxAng = eMuxAngParking;
		    } else if (pSource == &iPin_parking_ang && m_pCoderDescSignalValue != NULL) {
		    	tUInt32 ui32ArduinoTimestamp = 0;
				tFloat32 f32Value = 0;
		        
		        {   // focus for sample read lock
		            __adtf_sample_read_lock_mediadescription(m_pCoderDescSignalValue,pMediaSample,pCoder);

					pCoder->Get("ui32ArduinoTimestamp", (tVoid*)&ui32ArduinoTimestamp);
		            pCoder->Get("f32Value", (tVoid*)&f32Value);
		        }
		        
		        if (f32Value > 200) {
		        	std::cout << "Geparkt" << std::endl << "Warte kurz....";
		        	
		        	if(m_turnSignalState == 2) {
		        		TransmitTurnSignalRight(tFalse);
		        		m_turnSignalState = 0;
		        		}
		        	m_eMuxAng = eMuxAngLaneAssist;
		        	m_eMuxAcc = eMuxAccDefault;
		        	incrementManeuverID();
		        	
		        	m_stopped = 1;
		        	m_parked = tTrue;		        	
		        	TransmitAcc(m_acc = 0);
		        	TransmitTurnSignalBoth(tTrue);
		        	m_turnSignalState = 3;
		        	
		        }
		    
		    	if (m_eMuxAng == eMuxAngParking) TransmitAng(m_ang = f32Value);
			} else if (pSource == &iPin_lineChange_ang && m_pCoderDescSignalValue != NULL) {
		    	tUInt32 ui32ArduinoTimestamp = 0;
				tFloat32 f32Value = 0;
		        
		        {   // focus for sample read lock
		            __adtf_sample_read_lock_mediadescription(m_pCoderDescSignalValue,pMediaSample,pCoder);

					pCoder->Get("ui32ArduinoTimestamp", (tVoid*)&ui32ArduinoTimestamp);
		            pCoder->Get("f32Value", (tVoid*)&f32Value);
		        }
		        
		        if (f32Value > 200) {
		        	std::cout << "RESET" << std::endl;
		        	
		        	m_eMuxAng = eMuxAngLaneAssist;
		        	m_eMuxAcc = eMuxAccDefault;
		        	
		        	TransmitAcc(m_acc = m_defaultAcc);
		        	
		        	if((getActionFromManeuverID(m_i16CurrentManeuverID - 1) == eActionPullOutRight))
		        		incrementManeuverID();
		        		
			    	if(m_turnSignalState != 0) {
			    		switch(m_turnSignalState){
			    			case 1: TransmitTurnSignalLeft(tFalse); break;
			    			case 2: TransmitTurnSignalRight(tFalse); break;
			    			case 3: TransmitTurnSignalBoth(tFalse); break;
			    			default: break;
			    		}
			    		m_turnSignalState = 0;
			    	}
		        }
		    
		    	if (m_eMuxAng == eMuxAngLineChange) TransmitAng(m_ang = f32Value);
		    } else if (pSource == &iPin_roadSign && m_pCoderDescRoadSign != NULL && (m_eMuxAng == eMuxAngLaneAssist) && (m_eMuxAcc == eMuxAccDefault)) {
		    	tInt8 i8Identifier = 0;
				tFloat32 fl32Imagesize = 0;
		        
		        {   // focus for sample read lock
		            __adtf_sample_read_lock_mediadescription(m_pCoderDescRoadSign,pMediaSample,pCoder);

					pCoder->Get("i8Identifier", (tVoid*)&i8Identifier);
		            pCoder->Get("fl32Imagesize", (tVoid*)&fl32Imagesize);
		        }
		        
		       	eAction action;
		        
		        if (((float)fl32Imagesize > m_minRoadSignSize) && ((float)fl32Imagesize < m_maxRoadSignSize)) {
		        			
		        			action = getActionFromManeuverID(m_i16CurrentManeuverID);
							
							switch((tInt32)i8Identifier) {
							case 1:		// Vorfahrt gewaehren
								std::cout << "Vorfahrt gewaehren Schild erkannt" << std::endl;
								
								if (action == eActionLeft) {
									TransmitAcc(m_acc = 0);
									m_eMuxAng = eMuxAngCrossing;
									m_eMuxAcc = eMuxAccCrossing;
									TransmitCrossingType(1);
									TransmitTurnSignalLeft(tTrue);
									m_turnSignalState = 1;
								} else if (action == eActionStraight) {
									TransmitAcc(m_acc = 0);
									m_eMuxAng = eMuxAngCrossing;
									m_eMuxAcc = eMuxAccCrossing;
									TransmitCrossingType(2);
								} else if (action == eActionRight) {
									TransmitAcc(m_acc = 0);
									m_eMuxAng = eMuxAngCrossing;
									m_eMuxAcc = eMuxAccCrossing;
									TransmitCrossingType(3);
									TransmitTurnSignalRight(tTrue);
									m_turnSignalState = 2;
								}
								
								break;
							case 2:		// Vorfahrt an nächster Kreuzung
								std::cout << "Vorfahrt an nächster Kreuzung Schild erkannt" << std::endl;
								
								if (action == eActionLeft) {
									m_eMuxAng = eMuxAngCrossing;
									m_eMuxAcc = eMuxAccCrossing;
									TransmitCrossingType(7);
									TransmitTurnSignalLeft(tTrue);
									m_turnSignalState = 1;
								} else if (action == eActionStraight) {
									m_eMuxAng = eMuxAngCrossing;
									m_eMuxAcc = eMuxAccCrossing;
									TransmitCrossingType(8);
								} else if (action == eActionRight) {
									m_eMuxAng = eMuxAngCrossing;
									m_eMuxAcc = eMuxAccCrossing;
									TransmitCrossingType(9);
									TransmitTurnSignalRight(tTrue);
									m_turnSignalState = 2;
								}
								
								break;
							case 3:		// Stopp
								std::cout << "Stoppschild erkannt" << std::endl;
								
								if (action == eActionLeft) {
									TransmitAcc(m_acc = 0);
									m_stopped_left = tTrue;
									m_stopped = 1;
									
									cout << "Warte kurz...";
									
									m_eMuxAng = eMuxAngCrossing;
									m_eMuxAcc = eMuxAccCrossing;
									TransmitTurnSignalLeft(tTrue);
									m_turnSignalState = 1;
								} else if (action == eActionStraight) {
									TransmitAcc(m_acc = 0);
									m_stopped_straight = tTrue;
									m_stopped = 1;
									
									cout << "Warte kurz...";
									
									m_eMuxAng = eMuxAngCrossing;
									m_eMuxAcc = eMuxAccCrossing;
								} else if (action == eActionRight) {
									TransmitAcc(m_acc = 0);
									m_stopped_right = tTrue;
									m_stopped = 1;
									
									cout << "Warte kurz...";
									
									m_eMuxAng = eMuxAngCrossing;
									m_eMuxAcc = eMuxAccCrossing;
									TransmitTurnSignalRight(tTrue);
									m_turnSignalState = 2;
								}
								
								break;
							case 4:		// Parken
								break;
							case 5: 	// vorgeschriebene Fahrtrichtung gerade aus
								std::cout << "(5)" << std::endl;
								TransmitAcc(m_acc = 0);
								m_eMuxAng = eMuxAngCrossing;
								m_eMuxAcc = eMuxAccCrossing;
								TransmitCrossingType(5);
								break;
							case 6: 	// Kreuzung
								std::cout << "Kreuzungsschild erkannt" << std::endl;
								
								if (action == eActionLeft) {
									TransmitAcc(m_acc = 0);
									m_eMuxAng = eMuxAngCrossing;
									m_eMuxAcc = eMuxAccCrossing;
									TransmitCrossingType(4);
									TransmitTurnSignalLeft(tTrue);
									m_turnSignalState = 1;
								} else if (action == eActionStraight) {
									TransmitAcc(m_acc = 0);
									m_eMuxAng = eMuxAngCrossing;
									m_eMuxAcc = eMuxAccCrossing;
									TransmitCrossingType(5);
								} else if (action == eActionRight) {
									TransmitAcc(m_acc = 0);
									m_eMuxAng = eMuxAngCrossing;
									m_eMuxAcc = eMuxAccCrossing;
									TransmitCrossingType(6);
									TransmitTurnSignalRight(tTrue);
									m_turnSignalState = 2;
								}
								
								break;
							case 7:		// Fussgaenger ueberweg
								std::cout << "(7)" << std::endl;
								break;
							case 8:		// kreisverkehr
								std::cout << "(8)" << std::endl;
								break;
					   		case 9:		// ueberholverbot
						   		std::cout << "(9)" << std::endl;
					   			break;
					   		case 10:	// Einfahrt verboten
						   		std::cout << "(10)" << std::endl;
					   			break;
					   		case 11:	// Einbahnstraße
						   		std::cout << "(11)" << std::endl;
					   			break;
					   		default:
					   			break;
							}
		        		
		        		//}
		        	//
		        }
		        //cout << "ImageSize: " << m_maxParkSignSize <<" > " << (float)fl32Imagesize << " > " << m_minParkSignSize << " Identifier: " << (int)i8Identifier << " Mux Ang: " << m_eMuxAng << " Mux Acc: " << m_eMuxAcc << endl;
		        if (((float)fl32Imagesize > m_minParkSignSize) && ((float)fl32Imagesize < m_maxParkSignSize)) {
    				if ((int)i8Identifier == 4) {
						std::cout << "Parkschild erkannt" << std::endl;
						
						action = getActionFromManeuverID(m_i16CurrentManeuverID);
						
						if(action == eActionCrossParking){
							TransmitTurnSignalRight(tTrue);
							m_turnSignalState = 2;
							TransmitCrossParking(tTrue);
							m_eMuxAcc = eMuxAccParking;
						} else if(action == eActionParallelParking) {
							TransmitTurnSignalRight(tTrue);
							m_turnSignalState = 2;
							TransmitParallelParking(tTrue);
							cout << "parallel parking start:" << endl;
							m_eMuxAcc = eMuxAccParking;
						}
					}
		        }
		        //std::cout << "cDHDriverModule::OnPinEvent: Zeichen-ID = " << (tInt32)i8Identifier << ", Groesse = " << (float)fl32Imagesize << std::endl;
		    }
			//TransmitAng(m_ang);
			//TransmitAcc(m_acc);
        }
    }
    RETURN_NOERROR;

}

eAction cDHDriverModule::getActionFromManeuverID(tInt maneuverId) {
	std::cout << "maneuverId = " << maneuverId << ", action = " << m_sectorList[m_i16SectionListIndex].maneuverList[m_i16ManeuverListIndex].action << std::endl;

	if (strcmp(m_sectorList[m_i16SectionListIndex].maneuverList[m_i16ManeuverListIndex].action, "left") == 0) {
		return eActionLeft;
	} else if (strcmp(m_sectorList[m_i16SectionListIndex].maneuverList[m_i16ManeuverListIndex].action, "straight") == 0) {
		return eActionStraight;
	} else if (strcmp(m_sectorList[m_i16SectionListIndex].maneuverList[m_i16ManeuverListIndex].action, "right") == 0) {
		return eActionRight;
	} else if (strcmp(m_sectorList[m_i16SectionListIndex].maneuverList[m_i16ManeuverListIndex].action, "cross_parking") == 0) {
		return eActionCrossParking;
	} else if (strcmp(m_sectorList[m_i16SectionListIndex].maneuverList[m_i16ManeuverListIndex].action, "parallel_parking") == 0) {
		return eActionParallelParking;
	} else if (strcmp(m_sectorList[m_i16SectionListIndex].maneuverList[m_i16ManeuverListIndex].action, "pull_out_left") == 0) {
		return eActionPullOutLeft;
	} else if (strcmp(m_sectorList[m_i16SectionListIndex].maneuverList[m_i16ManeuverListIndex].action, "pull_out_right") == 0) {
		return eActionPullOutRight;
	}
	
	return eActionUndefined;
}

tResult cDHDriverModule::TransmitDriverStruct(stateCar state, tInt16 i16ManeuverEntry)
{            
    __synchronized_obj(m_oCriticalSectionTransmit);
    
    cObjectPtr<IMediaSample> pMediaSample;
    RETURN_IF_FAILED(AllocMediaSample((tVoid**)&pMediaSample));

    cObjectPtr<IMediaSerializer> pSerializer;
    m_pCoderDescDriverStruct->GetMediaSampleSerializer(&pSerializer);
    tInt nSize = pSerializer->GetDeserializedSize();

    tInt8 bValue = tInt8(state);

    RETURN_IF_FAILED(pMediaSample->AllocBuffer(nSize));
    {   // focus for sample write lock
        __adtf_sample_write_lock_mediadescription(m_pCoderDescDriverStruct,pMediaSample,pCoder);


        pCoder->Set("i8StateID", (tVoid*)&bValue);
        pCoder->Set("i16ManeuverEntry", (tVoid*)&i16ManeuverEntry);
    }      
        
    pMediaSample->SetTime(_clock->GetStreamTime());
    oPin_driverStruct.Transmit(pMediaSample);
        
    //debug output to console
    if(m_bDebugModeEnabled)  
    {
        switch (state)
            {
            case stateCar_ERROR:
                if(m_bDebugModeEnabled) LOG_INFO(cString::Format("State Controller Module: Send state: ERROR, Maneuver ID %d",i16ManeuverEntry));
                break;
            case stateCar_READY:
                if(m_bDebugModeEnabled) LOG_INFO(cString::Format("State Controller Module: Send state: READY, Maneuver ID %d",i16ManeuverEntry));
                break;
            case stateCar_RUNNING:
                if(m_bDebugModeEnabled) LOG_INFO(cString::Format("State Controller Module: Send state: RUNNING, Maneuver ID %d",i16ManeuverEntry));
                break;
            case stateCar_COMPLETE:
                if(m_bDebugModeEnabled) LOG_INFO(cString::Format("State Controller Module: Send state: COMPLETE, Maneuver ID %d",i16ManeuverEntry));                   
                break;
            case stateCar_STARTUP:
                break;
            }
    }
        
    RETURN_NOERROR;
}

tResult cDHDriverModule::TransmitAcc(const tFloat32 acc) {    
    //create new media sample
    cObjectPtr<IMediaSample> pMediaSample;
    AllocMediaSample((tVoid**)&pMediaSample);

    //allocate memory with the size given by the descriptor
    cObjectPtr<IMediaSerializer> pSerializer;
    m_pCoderDescSignalValue->GetMediaSampleSerializer(&pSerializer);
    tInt nSize = pSerializer->GetDeserializedSize();
    pMediaSample->AllocBuffer(nSize);
      
    tUInt32 timeStampValue = 0;
    //tFloat32 acc_out = ((m_distFront < 10) && (acc > 0)) ? 0 : acc;
    
      
    //write date to the media sample with the coder of the descriptor    
    {   // focus for sample write lock

        __adtf_sample_write_lock_mediadescription(m_pCoderDescSignalValue,pMediaSample,pCoderOutput);
        pCoderOutput->Set("f32Value", (tVoid*)&(acc));    
        pCoderOutput->Set("ui32ArduinoTimestamp", (tVoid*)&timeStampValue);
    }    
    //transmit media sample over output pin
    pMediaSample->SetTime(_clock->GetStreamTime());
    oPin_acc.Transmit(pMediaSample);
    
    RETURN_NOERROR;
}

tResult cDHDriverModule::TransmitAng(const tFloat32 ang) {    
    //create new media sample
    cObjectPtr<IMediaSample> pMediaSample;
    AllocMediaSample((tVoid**)&pMediaSample);

    //allocate memory with the size given by the descriptor
    cObjectPtr<IMediaSerializer> pSerializer;
    m_pCoderDescSignalValue->GetMediaSampleSerializer(&pSerializer);
    tInt nSize = pSerializer->GetDeserializedSize();
    pMediaSample->AllocBuffer(nSize);
      
    tUInt32 timeStampValue = 0;
      
    //write date to the media sample with the coder of the descriptor    
    {   // focus for sample write lock

        __adtf_sample_write_lock_mediadescription(m_pCoderDescSignalValue,pMediaSample,pCoderOutput);
        pCoderOutput->Set("f32Value", (tVoid*)&(ang));    
        pCoderOutput->Set("ui32ArduinoTimestamp", (tVoid*)&timeStampValue);
    }    
    //transmit media sample over output pin
    pMediaSample->SetTime(_clock->GetStreamTime());
    oPin_ang.Transmit(pMediaSample);
    
    RETURN_NOERROR;
}

tResult cDHDriverModule::TransmitCrossingType(const tInt8 crossingType) {    
    //create new media sample
    cObjectPtr<IMediaSample> pMediaSample;
    AllocMediaSample((tVoid**)&pMediaSample);

    //allocate memory with the size given by the descriptor
    cObjectPtr<IMediaSerializer> pSerializer;
    m_pCoderDescCrossingType->GetMediaSampleSerializer(&pSerializer);
    tInt nSize = pSerializer->GetDeserializedSize();
    pMediaSample->AllocBuffer(nSize);
    
    //write date to the media sample with the coder of the descriptor    
    {   // focus for sample write lock

        __adtf_sample_write_lock_mediadescription(m_pCoderDescCrossingType,pMediaSample,pCoderOutput);
        pCoderOutput->Set("i8Identifier", (tVoid*)&(crossingType));    
    }    
    
    //transmit media sample over output pin
    pMediaSample->SetTime(_clock->GetStreamTime());
    oPin_crossingTrigger.Transmit(pMediaSample);
    
    RETURN_NOERROR;
}

tResult cDHDriverModule::TransmitLineChangeType(const tInt8 crossingType) {    
    //create new media sample
    cObjectPtr<IMediaSample> pMediaSample;
    AllocMediaSample((tVoid**)&pMediaSample);

    //allocate memory with the size given by the descriptor
    cObjectPtr<IMediaSerializer> pSerializer;
    m_pCoderDescCrossingType->GetMediaSampleSerializer(&pSerializer);
    tInt nSize = pSerializer->GetDeserializedSize();
    pMediaSample->AllocBuffer(nSize);
    
    //write date to the media sample with the coder of the descriptor    
    {   // focus for sample write lock

        __adtf_sample_write_lock_mediadescription(m_pCoderDescCrossingType,pMediaSample,pCoderOutput);
        pCoderOutput->Set("i8Identifier", (tVoid*)&(crossingType));    
    }    
    
    //transmit media sample over output pin
    pMediaSample->SetTime(_clock->GetStreamTime());
    oPin_lineChangeTrigger.Transmit(pMediaSample);
    
    RETURN_NOERROR;
}

tResult cDHDriverModule::TransmitTurnSignalRight(tBool state) {
    //write values with zero
    tUInt32 timeStamp = 0;
    
    //create new media sample
    cObjectPtr<IMediaSample> pMediaSample;
    AllocMediaSample((tVoid**)&pMediaSample);

    //allocate memory with the size given by the descriptor
    cObjectPtr<IMediaSerializer> pSerializer;
    m_pCoderDescBoolSignalValue->GetMediaSampleSerializer(&pSerializer);
    tInt nSize = pSerializer->GetDeserializedSize();
    pMediaSample->AllocBuffer(nSize);
       
    //write date to the media sample with the coder of the descriptor
    {
        __adtf_sample_read_lock_mediadescription(m_pCoderDescBoolSignalValue, pMediaSample, pCoderOutput);

        pCoderOutput->Set("bValue", (tVoid*)&(state));    
        pCoderOutput->Set("ui32ArduinoTimestamp", (tVoid*)&timeStamp);    
    }

    //transmit media sample over output pin
    pMediaSample->SetTime(_clock->GetStreamTime());
	oPin_turnSignalRight.Transmit(pMediaSample);

    RETURN_NOERROR;
}

tResult cDHDriverModule::TransmitTurnSignalBoth(tBool state) {
    //write values with zero
    tUInt32 timeStamp = 0;
    
    //create new media sample
    cObjectPtr<IMediaSample> pMediaSample;
    AllocMediaSample((tVoid**)&pMediaSample);

    //allocate memory with the size given by the descriptor
    cObjectPtr<IMediaSerializer> pSerializer;
    m_pCoderDescBoolSignalValue->GetMediaSampleSerializer(&pSerializer);
    tInt nSize = pSerializer->GetDeserializedSize();
    pMediaSample->AllocBuffer(nSize);
       
    //write date to the media sample with the coder of the descriptor
    {
        __adtf_sample_read_lock_mediadescription(m_pCoderDescBoolSignalValue, pMediaSample, pCoderOutput);

        pCoderOutput->Set("bValue", (tVoid*)&(state));    
        pCoderOutput->Set("ui32ArduinoTimestamp", (tVoid*)&timeStamp);    
    }

    //transmit media sample over output pin
    pMediaSample->SetTime(_clock->GetStreamTime());
	oPin_turnSignalBoth.Transmit(pMediaSample);

    RETURN_NOERROR;
}

tResult cDHDriverModule::TransmitTurnSignalLeft(tBool state) {
    //write values with zero
    tUInt32 timeStamp = 0;
    
    //create new media sample
    cObjectPtr<IMediaSample> pMediaSample;
    AllocMediaSample((tVoid**)&pMediaSample);

    //allocate memory with the size given by the descriptor
    cObjectPtr<IMediaSerializer> pSerializer;
    m_pCoderDescBoolSignalValue->GetMediaSampleSerializer(&pSerializer);
    tInt nSize = pSerializer->GetDeserializedSize();
    pMediaSample->AllocBuffer(nSize);
       
    //write date to the media sample with the coder of the descriptor
    {
        __adtf_sample_read_lock_mediadescription(m_pCoderDescBoolSignalValue, pMediaSample, pCoderOutput);

        pCoderOutput->Set("bValue", (tVoid*)&(state));    
        pCoderOutput->Set("ui32ArduinoTimestamp", (tVoid*)&timeStamp);    
    }

    //transmit media sample over output pin
    pMediaSample->SetTime(_clock->GetStreamTime());
	oPin_turnSignalLeft.Transmit(pMediaSample);

    RETURN_NOERROR;
}

tResult cDHDriverModule::TransmitParallelParking(tBool state) {
    //write values with zero
    tUInt32 timeStamp = 0;
    
    //create new media sample
    cObjectPtr<IMediaSample> pMediaSample;
    AllocMediaSample((tVoid**)&pMediaSample);

    //allocate memory with the size given by the descriptor
    cObjectPtr<IMediaSerializer> pSerializer;
    m_pCoderDescBoolSignalValue->GetMediaSampleSerializer(&pSerializer);
    tInt nSize = pSerializer->GetDeserializedSize();
    pMediaSample->AllocBuffer(nSize);
       
    //write date to the media sample with the coder of the descriptor
    {
        __adtf_sample_read_lock_mediadescription(m_pCoderDescBoolSignalValue, pMediaSample, pCoderOutput);

        pCoderOutput->Set("bValue", (tVoid*)&(state));    
        pCoderOutput->Set("ui32ArduinoTimestamp", (tVoid*)&timeStamp);    
    }

    //transmit media sample over output pin
    pMediaSample->SetTime(_clock->GetStreamTime());
	oPin_parallelParking.Transmit(pMediaSample);

    RETURN_NOERROR;
}

tResult cDHDriverModule::TransmitCrossParking(tBool state) {
    //write values with zero
    tUInt32 timeStamp = 0;
    
    //create new media sample
    cObjectPtr<IMediaSample> pMediaSample;
    AllocMediaSample((tVoid**)&pMediaSample);

    //allocate memory with the size given by the descriptor
    cObjectPtr<IMediaSerializer> pSerializer;
    m_pCoderDescBoolSignalValue->GetMediaSampleSerializer(&pSerializer);
    tInt nSize = pSerializer->GetDeserializedSize();
    pMediaSample->AllocBuffer(nSize);
       
    //write date to the media sample with the coder of the descriptor
    {
        __adtf_sample_read_lock_mediadescription(m_pCoderDescBoolSignalValue, pMediaSample, pCoderOutput);

        pCoderOutput->Set("bValue", (tVoid*)&(state));    
        pCoderOutput->Set("ui32ArduinoTimestamp", (tVoid*)&timeStamp);    
    }

    //transmit media sample over output pin
    pMediaSample->SetTime(_clock->GetStreamTime());
	oPin_crossParking.Transmit(pMediaSample);

    RETURN_NOERROR;
}

tResult cDHDriverModule::TransmitReset(tBool state) {
    //write values with zero
    tUInt32 timeStamp = 0;
    
    //create new media sample
    cObjectPtr<IMediaSample> pMediaSample;
    AllocMediaSample((tVoid**)&pMediaSample);

    //allocate memory with the size given by the descriptor
    cObjectPtr<IMediaSerializer> pSerializer;
    m_pCoderDescBoolSignalValue->GetMediaSampleSerializer(&pSerializer);
    tInt nSize = pSerializer->GetDeserializedSize();
    pMediaSample->AllocBuffer(nSize);
       
    //write date to the media sample with the coder of the descriptor
    {
        __adtf_sample_read_lock_mediadescription(m_pCoderDescBoolSignalValue, pMediaSample, pCoderOutput);

        pCoderOutput->Set("bValue", (tVoid*)&(state));    
        pCoderOutput->Set("ui32ArduinoTimestamp", (tVoid*)&timeStamp);    
    }

    //transmit media sample over output pin
    pMediaSample->SetTime(_clock->GetStreamTime());
	oPin_reset.Transmit(pMediaSample);

    RETURN_NOERROR;
}



tResult cDHDriverModule::loadManeuverList()
{

    m_maneuverListFile = GetPropertyStr(SC_PROP_FILENAME);
    
    if (m_maneuverListFile.IsEmpty())
    {
        LOG_ERROR("StateController: Maneuver file not found");
        RETURN_ERROR(ERR_INVALID_FILE);
    }    
    
    ADTF_GET_CONFIG_FILENAME(m_maneuverListFile);
    
    m_maneuverListFile = m_maneuverListFile.CreateAbsolutePath(".");

    //Load file, parse configuration, print the data
   
    if (cFileSystem::Exists(m_maneuverListFile))
    {
        cDOM oDOM;
        oDOM.Load(m_maneuverListFile);        
        cDOMElementRefList oSectorElems;
        cDOMElementRefList oManeuverElems;

        //read first Sector Elem
        if(IS_OK(oDOM.FindNodes("AADC-Maneuver-List/AADC-Sector", oSectorElems)))
        {                
            //iterate through sectors
            for (cDOMElementRefList::iterator itSectorElem = oSectorElems.begin(); itSectorElem != oSectorElems.end(); ++itSectorElem)
            {
                //if sector found
                tSector sector;
                sector.id = (*itSectorElem)->GetAttributeUInt32("id");
                
                if(IS_OK((*itSectorElem)->FindNodes("AADC-Maneuver", oManeuverElems)))
                {
                    //iterate through maneuvers
                    for(cDOMElementRefList::iterator itManeuverElem = oManeuverElems.begin(); itManeuverElem != oManeuverElems.end(); ++itManeuverElem)
                    {
                        tAADC_Maneuver man;
                        man.id = (*itManeuverElem)->GetAttributeUInt32("id");
                        man.action = (*itManeuverElem)->GetAttribute("action");
                        sector.maneuverList.push_back(man);
                    }
                }
    
                m_sectorList.push_back(sector);
            }
        }
        if (oSectorElems.size() > 0)
        {
            LOG_INFO("StateController: Loaded Maneuver file successfully.");
        }
        else
        {
            LOG_ERROR("StateController: no valid Maneuver Data found!");
            RETURN_ERROR(ERR_INVALID_FILE);
        }
    }
    else
    {
        LOG_ERROR("DriverFilter: no valid Maneuver File found!");
        RETURN_ERROR(ERR_INVALID_FILE);
    }
   
    RETURN_NOERROR;
}

tResult cDHDriverModule::changeState(stateCar newState)
{
    // if state is the same do nothing
    if (m_state == newState) RETURN_NOERROR;
    
    //to secure the state is sent at least one time
    TransmitDriverStruct(newState, m_i16CurrentManeuverID);
   
    //handle the timer depending on the state
    switch (newState)
    {
    case stateCar_ERROR:
        destroyTimer();
        LOG_INFO("State ERROR reached");
        cout <<  "State ERROR reached" << endl;
        break;
    case stateCar_STARTUP:
        destroyTimer();
        LOG_INFO("State STARTUP reached");
        cout <<  "State STARTUP reached" << endl;
        break;
    case stateCar_READY:
        createTimer();
        LOG_INFO(adtf_util::cString::Format("State READY reached (ID %d)",m_i16CurrentManeuverID));
        cout <<  "State READY reached" << endl;
        break;
    case stateCar_RUNNING:
        if (m_state!=stateCar_READY)
            LOG_WARNING("Invalid state change to Car_RUNNING. Car_READY was not reached before");
        LOG_INFO("State RUNNING reached");
        cout <<  "State RUNNING reached" << endl;
        break;
    case stateCar_COMPLETE:
        destroyTimer();
        LOG_INFO("State COMPLETE reached");
        cout <<  "State COMPLETE reached" << endl;
        break;
    }
    
    m_state = newState;
    RETURN_NOERROR;
}

tResult cDHDriverModule::createTimer()
{
     // creates timer with 0.5 sec
     __synchronized_obj(m_oCriticalSectionTimerSetup);
     // additional check necessary because input jury structs can be mixed up because every signal is sent three times
     if (m_hTimer == NULL)
     {
            m_hTimer = _kernel->TimerCreate(0.5*1000000, 0, static_cast<IRunnable*>(this),
                                        NULL, NULL, 0, 0, adtf_util::cString::Format("%s.timer", OIGetInstanceName()));
     }
     else
     {
        LOG_ERROR("Timer is already running. Unable to create a new one.");
     }
     RETURN_NOERROR;
}

tResult cDHDriverModule::destroyTimer(__exception)
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

tResult cDHDriverModule::incrementManeuverID()
{
    //check if list was sucessfully loaded in init    
    if (m_i16ManeuverListIndex!=-1 && m_i16SectionListIndex!=-1)
    {
        //check if end of section is reached
        if (m_sectorList[m_i16SectionListIndex].maneuverList.size()>tUInt(m_i16ManeuverListIndex+1))
        {
            //increment only maneuver index
            m_i16ManeuverListIndex++;
            m_i16CurrentManeuverID++;
        }
        else
        {
            //end of section was reached and another section is in list
            if (m_sectorList.size() >tUInt(m_i16SectionListIndex+1))
            {
                //reset maneuver index to zero and increment section list index
                m_i16SectionListIndex++;
                m_i16ManeuverListIndex=0;
                m_i16CurrentManeuverID++;
                if (m_sectorList[m_i16SectionListIndex].maneuverList[m_i16ManeuverListIndex].id !=m_i16CurrentManeuverID)
                {
                    LOG_ERROR("State Controller: inconsistancy in maneuverfile detected. Please check the file ");
                }
            }
            else
            {
                LOG_INFO("State Controller: end of maneuverlist reached, cannot increment any more");
                changeState(stateCar_COMPLETE);
            }
        }
    }
    else
    {
        LOG_ERROR("State Controller: could not set new maneuver id because no maneuver list was loaded");
    }
    
    if(m_bDebugModeEnabled) LOG_INFO(adtf_util::cString::Format("Increment Manevuer ID: Sectionindex is %d, Maneuverindex is %d, ID is %d",m_i16SectionListIndex,m_i16ManeuverListIndex,m_i16CurrentManeuverID));

    RETURN_NOERROR;
}

tResult cDHDriverModule::resetSection()
{
    //maneuver list index to zero, and current maneuver id to first element in list 
    m_i16ManeuverListIndex=0;
    m_i16CurrentManeuverID = m_sectorList[m_i16SectionListIndex].maneuverList[m_i16ManeuverListIndex].id;
   
    if(m_bDebugModeEnabled) LOG_INFO(adtf_util::cString::Format("Reset section: Sectionindex is %d, Maneuverindex is %d, ID is %d",m_i16SectionListIndex,m_i16ManeuverListIndex,m_i16CurrentManeuverID));

    RETURN_NOERROR;
}

tResult cDHDriverModule::setManeuverID(tInt maneuverId)
{   
    //look for the right section id and write it to section combobox
    for(unsigned int i = 0; i < m_sectorList.size(); i++)
    {
        for(unsigned int j = 0; j < m_sectorList[i].maneuverList.size(); j++)
        {
            if(m_i16CurrentManeuverID == m_sectorList[i].maneuverList[j].id)
            {            
                m_i16SectionListIndex = i;
                m_i16ManeuverListIndex = j;
                m_i16CurrentManeuverID = maneuverId;
                if(m_bDebugModeEnabled) LOG_INFO(adtf_util::cString::Format("Sectionindex is %d, Maneuverindex is %d, ID is %d",m_i16SectionListIndex,m_i16ManeuverListIndex,m_i16CurrentManeuverID));
                break;
            }
        }
    }    
    RETURN_NOERROR;
}
