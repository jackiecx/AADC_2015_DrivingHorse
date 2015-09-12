/**
Copyright (c) 
Audi Autonomous Driving Cup. All rights reserved.
 
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3.  All advertising materials mentioning features or use of this software must display the following acknowledgement: “This product includes software developed by the Audi AG and its contributors for Audi Autonomous Driving Cup.”
4.  Neither the name of Audi nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY AUDI AG AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL AUDI AG OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#ifndef _DH_LINE_DETECTOR_HPP_
#define _DH_LINE_DETECTOR_HPP_

#define OID_ADTF_DHLINEDETECTOR  "adtf.dh.laneAssistant"

#include "line_detection.h"

class cLineDetector : public adtf::cFilter {
	ADTF_DECLARE_FILTER_VERSION(OID_ADTF_DHLINEDETECTOR, "DH lineDetector", adtf::OBJCAT_Tool, "DH lineDetector", 0, 0, 1, "Alpha Version");
	
	public:
		cLineDetector(const tChar*);
		virtual ~cLineDetector();
		
	protected:
		tResult Init(tInitStage eStage, __exception=NULL);
		tResult Start(__exception = NULL);
        tResult Stop(__exception = NULL);
		tResult Shutdown(tInitStage eStage, ucom::IException** __exception_ptr=NULL);
		tResult OnPinEvent(IPin* pSource, tInt nEventCode, tInt nParam1, tInt nParam2, IMediaSample* pMediaSample);
		tResult PropertyChanged(const char* strProperty);
		
	private:
        tResult CreateOutputPins(__exception = NULL);	// creates all the output pins
        tResult CreateInputPins(__exception = NULL);	// creates all the input pins
        //tResult TransmitGCL();							// transmit gcl output
        tResult TransmitAng(const tFloat32 ang);
        tResult TransmitSteeringAngle(tTimeStamp sampleTimeStamp, const tTimeStamp timeStampValue, const tFloat32 ang);
        tResult ProcessVideo(IMediaSample* pSample);	// process input image
   		tResult UpdateImageFormat(const tBitmapFormat* pFormat);
        
		cVideoPin 	iPin_vid;				// the input pin for the video
		//cOutputPin 	oPin_gcl;			// the output pin for the glc data
		cOutputPin	oPin_steeringAngle;

		// Coder Descriptor for the output pins
		cObjectPtr<IMediaTypeDescription> m_pCoderDescSignalOutput;
		cObjectPtr<IMediaTypeDescription> m_pCoderDescBoolSignalOutput;
		
		// Coder Descriptor for the input pins
		cObjectPtr<IMediaTypeDescription> m_pCoderDescSignal;
		
		cObjectPtr<IMediaTypeDescription> m_pCoderDescSignalInput;
		cObjectPtr<IMediaTypeDescription> m_pCoderDescBoolSignalInput;
		
		tBool m_bFirstFrame;				// flag for the first frame
		tBitmapFormat m_sInputFormat;		// bitmap format of the input image
		
		tFloat32 m_steeringAngle;
		
		tInt m_roiVL;			// the vertical limit on the left side
		tInt m_roiVR;			// the vertical limit on the right side
		tInt m_roiHO;			// the horizontal limit on the upper side
		tInt m_roiHU;			// the horizontal limit on the lower side
		
		cv::Mat m_H;
	
	
		tUInt32 m_laneCenterX;
		tUInt32 m_carWidth;
		tUInt32 m_carCenterY;
		tUInt32 m_camAngle;
		tFloat32 m_carLength;
		tFloat32 m_steeringAngleHistoryLength;
//		tFloat32 m_steeringAngleHistory[64];
	 	tUInt32 m_laneBorderWidth;
	 	tBool m_imshow;

    	tUInt32 m_distanceToOuterLine;
    	tUInt32 m_straightZoneStartRow;
    	
    	tResult extractLines(cv::Mat &imgSrc, cv::Mat &imgLines, float fThGrd= 20.f);
};

#endif 
