/**
Copyright (c) 
Audi Autonomous Driving Cup. All rights reserved.
 
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3.  All advertising materials mentioning features or use of this software must display the following acknowledgement: "This product includes software developed by the Audi AG and its contributors for Audi Autonomous Driving Cup."
4.  Neither the name of Audi nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY AUDI AG AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL AUDI AG OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/

#include "stdafx.h"
#include "lineDetector.h"

using namespace std;
using namespace cv;

ADTF_FILTER_PLUGIN("DH laneAssistant", OID_ADTF_DHLINEDETECTOR, cLineDetector)

cLineDetector::cLineDetector(const tChar* __info) : cFilter(__info) {
	m_steeringAngle = 0;
	m_bFirstFrame = tTrue;
	m_laneCenterX = 322;
	m_carWidth = 50;
	m_carCenterY = 140;
	
	m_carLength = 35;

	m_laneBorderWidth = 0;
	m_straightZoneStartRow = 20;

	m_steeringAngleHistoryLength=1;
//	for(int a_idx = 0; a_idx<64; a_idx++)
//		m_steeringAngleHistory[a_idx]=0;

    m_distanceToOuterLine = 5;

	m_camAngle = 19;
	
	SetPropertyInt("Grenze V Links", 0);
	SetPropertyBool("Grenze V Links" NSSUBPROP_ISCHANGEABLE, tTrue);
	SetPropertyInt("Grenze V Links" NSSUBPROP_MINIMUM  , 0);
	SetPropertyInt("Grenze V Links" NSSUBPROP_MAXIMUM  , 640);

	SetPropertyInt("Grenze V Rechts", 640);
	SetPropertyBool("Grenze V Rechts" NSSUBPROP_ISCHANGEABLE, tTrue);
	SetPropertyInt("Grenze V Rechts" NSSUBPROP_MINIMUM  , 0);
	SetPropertyInt("Grenze V Rechts" NSSUBPROP_MAXIMUM  , 640);

	SetPropertyInt("Grenze H Oben", 100);
	SetPropertyBool("Grenze H Oben" NSSUBPROP_ISCHANGEABLE, tTrue);
	SetPropertyInt("Grenze H Oben" NSSUBPROP_MINIMUM  , 0);
	SetPropertyInt("Grenze H Oben" NSSUBPROP_MAXIMUM  , 480);

	SetPropertyInt("Grenze H Unten", 370);
	SetPropertyBool("Grenze H Unten" NSSUBPROP_ISCHANGEABLE, tTrue);
	SetPropertyInt("Grenze H Unten" NSSUBPROP_MINIMUM  , 0);
	SetPropertyInt("Grenze H Unten" NSSUBPROP_MAXIMUM  , 480);
	
	SetPropertyInt("Spurzentrum", 375);
	SetPropertyBool("Spurzentrum" NSSUBPROP_ISCHANGEABLE, tTrue);
	
	SetPropertyInt("Autobreite", 55);
	SetPropertyBool("Autobreite" NSSUBPROP_ISCHANGEABLE, tTrue);
	
	SetPropertyInt("Autocenter Y", 140);
	SetPropertyBool("Autocenter Y" NSSUBPROP_ISCHANGEABLE, tTrue);

	SetPropertyInt("Kamerawinkel", 19);
	SetPropertyBool("Kamerawinkel" NSSUBPROP_ISCHANGEABLE, tTrue);

	SetPropertyInt("Lenkwinkel Glaettung", 1);
        SetPropertyBool("Lenkwinkel Glaettung" NSSUBPROP_ISCHANGEABLE, tTrue);
        SetPropertyInt("Lenkwinkel Glaettung" NSSUBPROP_MINIMUM  , 1);
        SetPropertyInt("Lenkwinkel Glaettung" NSSUBPROP_MAXIMUM  , 64);

	SetPropertyInt("Seitenstreifenbreite", 0);
        SetPropertyBool("Seitenstreifenbreite" NSSUBPROP_ISCHANGEABLE, tTrue);
        SetPropertyInt("Seitenstreifenbreite" NSSUBPROP_MINIMUM  , 0);
        
    SetPropertyInt("Startreihe Geradezone", 20);
        SetPropertyBool("Startreihe Geradezone" NSSUBPROP_ISCHANGEABLE, tTrue);
        SetPropertyInt("Startreihe Geradezone" NSSUBPROP_MINIMUM  , 0);
        SetPropertyInt("Startreihe Geradezone" NSSUBPROP_MAXIMUM  , 480);
        
    SetPropertyBool("Imshow", tFalse);
	SetPropertyBool("Imshow" NSSUBPROP_ISCHANGEABLE, tTrue);
}

cLineDetector::~cLineDetector() {}

//****************************
tResult cLineDetector::CreateInputPins(__exception) {
	RETURN_IF_FAILED(iPin_vid.Create("video_in", IPin::PD_Input, static_cast<IPinEventSink*> (this)));
	RETURN_IF_FAILED(RegisterPin(&iPin_vid));

	RETURN_NOERROR;
}

//*****************************************************************************

tResult cLineDetector::CreateOutputPins(__exception) {
    cObjectPtr<IMediaDescriptionManager> pDescManager;
    RETURN_IF_FAILED(_runtime->GetObject(OID_ADTF_MEDIA_DESCRIPTION_MANAGER,IID_ADTF_MEDIA_DESCRIPTION_MANAGER,(tVoid**)&pDescManager,__exception_ptr));
    
    tChar const * strDescSignalValue = pDescManager->GetMediaDescription("tSignalValue");
    RETURN_IF_POINTER_NULL(strDescSignalValue);        
    cObjectPtr<IMediaType> pTypeSignalValue = new cMediaType(0, 0, 0, "tSignalValue", strDescSignalValue,IMediaDescription::MDF_DDL_DEFAULT_VERSION);    
    RETURN_IF_FAILED(pTypeSignalValue->GetInterface(IID_ADTF_MEDIA_TYPE_DESCRIPTION, (tVoid**)&m_pCoderDescSignal)); 
    
    RETURN_IF_FAILED(oPin_steeringAngle.Create("steering_angle", pTypeSignalValue, static_cast<IPinEventSink*> (this)));
    RETURN_IF_FAILED(RegisterPin(&oPin_steeringAngle));

    RETURN_NOERROR;
}
tResult cLineDetector::TransmitAng(const tFloat32 ang) {    
    //create new media sample
    cObjectPtr<IMediaSample> pMediaSample;
    AllocMediaSample((tVoid**)&pMediaSample);

    //allocate memory with the size given by the descriptor
    cObjectPtr<IMediaSerializer> pSerializer;
    m_pCoderDescSignal->GetMediaSampleSerializer(&pSerializer);
    tInt nSize = pSerializer->GetDeserializedSize();
    pMediaSample->AllocBuffer(nSize);
      
    tUInt32 timeStampValue = 0;
      
    //write date to the media sample with the coder of the descriptor    
    {   // focus for sample write lock

        __adtf_sample_write_lock_mediadescription(m_pCoderDescSignal,pMediaSample,pCoderOutput);
        pCoderOutput->Set("f32Value", (tVoid*)&(ang));    
        pCoderOutput->Set("ui32ArduinoTimestamp", (tVoid*)&timeStampValue);
    }    
    //transmit media sample over output pin
    pMediaSample->SetTime(_clock->GetStreamTime());
    oPin_steeringAngle.Transmit(pMediaSample);
    
    RETURN_NOERROR;
}


/*tResult cLineDetector::TransmitSteeringAngle(tTimeStamp sampleTimeStamp, const tTimeStamp timeStampValue, const tFloat32 ang) {    
    //create new media sample
    cObjectPtr<IMediaSample> pMediaSampleSteeringAngle;
    AllocMediaSample((tVoid**)&pMediaSampleSteeringAngle);

    //allocate memory with the size given by the descriptor
    cObjectPtr<IMediaSerializer> pSerializer;
    m_pCoderDescSignal->GetMediaSampleSerializer(&pSerializer);
    tInt nSize = pSerializer->GetDeserializedSize();
    pMediaSampleSteeringAngle->AllocBuffer(nSize);
      
    //write date to the media sample with the coder of the descriptor    
    {   // focus for sample write lock
        __adtf_sample_write_lock_mediadescription(m_pCoderDescSignal,pMediaSampleSteeringAngle,pCoderOutputSteeringAngle);
        pCoderOutputSteeringAngle->Set("f32Value", (tVoid*)&(ang));    
        pCoderOutputSteeringAngle->Set("ui32ArduinoTimestamp", (tVoid*)&timeStampValue);
    }    
    //transmit media sample over output pin
    pMediaSampleSteeringAngle->SetTime(sampleTimeStamp);
    oPin_steeringAngle.Transmit(pMediaSampleSteeringAngle);
    
    RETURN_NOERROR;
}*/

tResult cLineDetector::Init(tInitStage eStage, __exception) {
    RETURN_IF_FAILED(cFilter::Init(eStage, __exception_ptr))

    if (eStage == StageFirst) {
        CreateInputPins(__exception_ptr);
        CreateOutputPins(__exception_ptr);
    } else if (eStage == StageNormal) {
    	m_roiVL = GetPropertyInt("Grenze V Links");
		m_roiVR = GetPropertyInt("Grenze V Rechts");
		m_roiHO = GetPropertyInt("Grenze H Oben");
		m_roiHU = GetPropertyInt("Grenze H Unten");
		m_laneCenterX = GetPropertyInt("Spurzentrum");
		m_carWidth = GetPropertyInt("Autobreite");
		m_carCenterY = GetPropertyInt("Autocenter Y");
		m_camAngle = GetPropertyInt("Kamerawinkel");
		m_imshow = GetPropertyBool("Imshow");
    } else if (eStage == StageGraphReady) {
		cObjectPtr<IMediaType> pType;
		RETURN_IF_FAILED(iPin_vid.GetMediaType(&pType));

		cObjectPtr<IMediaTypeVideo> pTypeVideo;
		RETURN_IF_FAILED(pType->GetInterface(IID_ADTF_MEDIA_TYPE_VIDEO, (tVoid**)&pTypeVideo));
		UpdateImageFormat(pTypeVideo->GetFormat());
		iPin_vid.SetFormat(&m_sInputFormat, NULL);  
	}
        
    RETURN_NOERROR;
}

tResult cLineDetector::Start(__exception) {
    return cFilter::Start(__exception_ptr);
}

tResult cLineDetector::Stop(__exception) {
    return cFilter::Stop(__exception_ptr);
}

tResult cLineDetector::Shutdown(tInitStage eStage, __exception) {
    return cFilter::Shutdown(eStage,__exception_ptr);
}

tResult cLineDetector::OnPinEvent(IPin* pSource, tInt nEventCode, tInt nParam1, tInt nParam2, IMediaSample* pMediaSample) {
	switch (nEventCode)	{
		case IPinEventSink::PE_MediaSampleReceived:
			if (pSource == &iPin_vid) {
				if (m_bFirstFrame) {
					double alpha = ((double)m_camAngle-90)/180.0*M_PI;
					double f = 400;
					double dist = 200;

					double w = m_sInputFormat.nWidth;
					double h = m_sInputFormat.nHeight;

					// Projection 2D -> 3D matrix
					Mat A1 = (Mat_<double>(4,3) <<
						1, 0, -w/2,
						0, 1, -h/2,
						0, 0,    0,
						0, 0,    1);

					// Rotation matrices around the X,Y,Z axis
					Mat R = (Mat_<double>(4, 4) <<
						1,          0,           0, 0,
						0, cos(alpha), -sin(alpha), 0,
						0, sin(alpha),  cos(alpha), 0,
						0,          0,           0, 1);

					// Translation matrix on the Z axis change dist will change the height
					Mat T = (Mat_<double>(4, 4) <<
						1, 0, 0, 0,
						0, 1, 0, 0,
						0, 0, 1, dist,
						0, 0, 0, 1);

					// Camera Intrisecs matrix 3D -> 2D
					Mat A2 = (Mat_<double>(3,4) <<
						f, 0, w/2, 0,
						0, f, h/2, 0,
						0, 0,   1, 0);

					// Final and overall transformation matrix
					m_H = A2 * (T * (R * A1));
				}
				ProcessVideo(pMediaSample);		
			}
			break;
		case IPinEventSink::PE_MediaTypeChanged:
			if (pSource == &iPin_vid) {
				cObjectPtr<IMediaType> pType;
				RETURN_IF_FAILED(iPin_vid.GetMediaType(&pType));
				cObjectPtr<IMediaTypeVideo> pTypeVideo;
				RETURN_IF_FAILED(pType->GetInterface(IID_ADTF_MEDIA_TYPE_VIDEO, (tVoid**)&pTypeVideo));
				UpdateImageFormat(iPin_vid.GetFormat());	
			}			
			break;
		default:
			break;
	}	
	RETURN_NOERROR;
}

tResult cLineDetector::PropertyChanged(const char* strProperty) {
	m_roiVL = GetPropertyInt("Grenze V Links");
	m_roiVR = GetPropertyInt("Grenze V Rechts");
	m_roiHO = GetPropertyInt("Grenze H Oben");
	m_roiHU = GetPropertyInt("Grenze H Unten");
	
	m_laneCenterX = GetPropertyInt("Spurzentrum");
	m_carWidth = GetPropertyInt("Autobreite");
	m_carCenterY = GetPropertyInt("Autocenter Y");
	m_laneBorderWidth = GetPropertyInt("Seitenstreifenbreite");
	m_camAngle = GetPropertyInt("Kamerawinkel");
	m_steeringAngleHistoryLength = GetPropertyInt("Lenkwinkel Glaettung");
	m_straightZoneStartRow = GetPropertyInt("Startreihe Geradezone");
	m_imshow = GetPropertyBool("Imshow");
	
	RETURN_NOERROR;
}

tResult cLineDetector::ProcessVideo(IMediaSample* pISample) {
	RETURN_IF_POINTER_NULL(pISample);

	//creating new media sample for output
	cObjectPtr<IMediaSample> pNewSample;
	RETURN_IF_FAILED(_runtime->CreateInstance(OID_ADTF_MEDIA_SAMPLE, IID_ADTF_MEDIA_SAMPLE, (tVoid**) &pNewSample));
	RETURN_IF_FAILED(pNewSample->AllocBuffer(m_sInputFormat.nSize));	
	
	//creating new pointer for input data
	const tVoid* l_pSrcBuffer;
	Mat imgSrc;
	//receiving data from inputsample, and saving to inputFrame
	if (IS_OK(pISample->Lock(&l_pSrcBuffer))) {
		//convert to mat
		imgSrc = Mat(m_sInputFormat.nHeight,m_sInputFormat.nWidth,CV_8UC3,(tVoid*)l_pSrcBuffer,m_sInputFormat.nBytesPerLine);	
		
		Mat imgTmp;
		cvtColor(imgSrc, imgTmp, CV_BGR2GRAY);

		// Apply matrix transformation
		warpPerspective(imgTmp, imgTmp, m_H, imgSrc.size(), INTER_CUBIC | WARP_INVERSE_MAP);

		// extract ROI
		Rect roi = Rect(m_roiVL, m_roiHO, m_roiVR-m_roiVL, m_roiHU-m_roiHO);	//0,50,640,325);	// TODO
		imgTmp = imgTmp(roi);
	
		Mat imgTh;
		detectWhiteRegions(imgTmp, imgTh, 30);
		std::vector<ConnectedComponent> binCC;
		determineConnectedComponents(imgTh, binCC);
		removeSmallRegions(imgTh, binCC,20);
		
		
		Mat imgLines;
		extractLines(imgTmp, imgLines);
		
		markLinesAsCentralOrRightByMajorityVote(imgTh, binCC, imgLines);
		
		Mat imgCenterLineProps;
		imgCenterLineProps.create(imgTh.size(), CV_8UC1);
		imgCenterLineProps.setTo(0);
		
		Mat imgRightLineProps;
		imgRightLineProps.create(imgTh.size(), CV_8UC1);
		imgRightLineProps.setTo(0);

		
		unsigned char *pTh;
		bool foundCenterLine=0;
		for (int i = 0; i < imgTh.rows; i+=5) {
			pTh = (unsigned char*)imgTh.data+i*imgTh.cols;
			int start_col = -1;
			for (int j = 0; j < imgTh.cols; j++) {
				if (pTh[j]==100){
					start_col = j;
					break;
				}
			}
			if(start_col>=0){
				int j;
				for (j = start_col+30; j < start_col+200 && j < imgTh.cols; j+=2) {
				 	if (pTh[j]!=0){
			 			break;
					}

				}
				if(j > start_col+100){
					foundCenterLine=1;
					cv::line(imgCenterLineProps, cv::Point(start_col+30, i), cv::Point(j-30, i), cv::Scalar(200), 2);
				}
			}
		}
		if(foundCenterLine==0){
			for (int i = 0; i < imgTh.rows; i+=5) {
				pTh = (unsigned char*)imgTh.data+i*imgTh.cols;
				int start_col = imgTh.cols;
				for (int j = imgTh.cols; j> 0; j--) {
					if (pTh[j]==200){
						start_col = j;
						break;
					}
				}
				if(start_col<imgTh.cols){
					int j;
					for (j = start_col-30; j > start_col-150 && j > 0 ; --j) {
					 	if (pTh[j]!=0){
							break;
						}
					}
					if(j < start_col-100)
			 		{
					 	cv::line(imgRightLineProps, cv::Point(start_col-30, i), cv::Point(j+30, i), cv::Scalar(200), 2);
					}
				}
			}
		}
		
		addWeighted(imgCenterLineProps, 1.0, imgRightLineProps, 1.0, 0, imgTh);
		
		

//		line(imgTh, Point(0,90), Point(imgTh.size().width/2 -70, imgTh.size().height+40) , Scalar(255,255,255), 20);
//		line(imgTh, Point(imgTh.size().width,90), Point(imgTh.size().width/2 +70, imgTh.size().height+40) , Scalar(255,255,255), 20);

		std::vector<cv::Point> binCCPoints;
		pTh = (unsigned char*)imgTh.data;
		for (int i = 0; i < imgTh.rows; i++) {
			for (int j = 0; j < imgTh.cols; j++, pTh++) {
				if (*pTh==200) binCCPoints.push_back(cv::Point(j, i));
			}
		}
		
		tUInt32 bonusMax = 0;//std::numeric_limits<tUInt32>::max();
		for (int i = m_straightZoneStartRow; i < imgTh.rows; ++i) {
			unsigned char *pTh = (unsigned char*)imgTh.data+i*imgTh.cols;
			for (tInt32 j = (tInt32)m_laneCenterX-(tInt32)m_carWidth; j < (tInt32)m_laneCenterX+(tInt32)m_carWidth; ++j) {
				if (pTh[j]==255) ++bonusMax;
			}
		}
		
		tFloat32 gammaOpt = 0;
	    
	    Mat imgPos = Mat(imgTh.rows, imgTh.cols, CV_32FC3);
	    
        for (tFloat32 gamma = -M_PI/48; gamma > -M_PI/6; gamma -= 0.02) {
            tFloat32 tanGamma = tan(gamma);
            tFloat32 rho = 3.3*sqrt(pow(m_carLength,2.0)/(tanGamma*tanGamma)+pow(m_carLength/2.0,2.0));

            tUInt32 bonusL = 0, bonusR = 0;

            for (size_t i = 0; i < binCCPoints.size(); i++) {
                tInt32 dx = m_laneCenterX-rho-binCCPoints[i].x;
                tInt32 dy = imgTh.rows+m_carCenterY-binCCPoints[i].y;
                tFloat32 dist = sqrt(dx*dx+dy*dy);
                if ((dist < rho+m_carWidth) && (dist > rho-m_carWidth)) ++bonusL;

                dx = m_laneCenterX+rho-binCCPoints[i].x;
                dist = sqrt(dx*dx+dy*dy);
                if ((dist < rho+m_carWidth) && (dist > rho-m_carWidth)) ++bonusR;
            }
		
	/*		Scalar colorRight = Scalar(0,255-penR, penR);
		Scalar colorLeft = Scalar(0,255-penL, penL);
		
		Point circleCenterPoint = Point(m_laneCenterX-rho, imgPos.rows+m_carCenterY);
		circ(imgPos, circeCenterPoint, rho-m_carWidth, colorLeft,  2);
		circ(imgPos, circeCenterPoint, rho+m_carWidth, colorLeft,  2);
		
		circleCenterPoint = Point(m_laneCenterX+rho, imgPos.rows+m_carCenterY);
		circ(imgPos, circeCenterPoint, rho-m_carWidth, colorRight,  2);
		circ(imgPos, circeCenterPoint, rho+m_carWidth, colorRight,  2);*/

            if (bonusL > bonusMax) {
                bonusMax = bonusL;
                gammaOpt = gamma;
            }
            if (bonusR > bonusMax) {
                bonusMax = bonusR;
                gammaOpt = -gamma;
            }
        }
        
		m_steeringAngle = 0.75*gammaOpt * 180/M_PI;
		// Geisterfahrer Mode		
		//if (penOL > penOR) m_steeringAngle = 50;
		
		Mat imgCrc(imgTh.size(), CV_8UC1);
		imgCrc.setTo(0);
		if (gammaOpt != 0) {
			tFloat32 tanGammaOpt = tan(gammaOpt);
			tFloat32 rhoOpt = 3.3*sqrt(35*35/(tanGammaOpt*tanGammaOpt)+17*17);
			circle(imgCrc, cv::Point((gammaOpt < 0) ? m_laneCenterX-rhoOpt : m_laneCenterX+rhoOpt, imgCrc.rows+m_carCenterY), rhoOpt, Scalar(255), 2*m_carWidth);
		} else {
			rectangle(imgCrc, Rect(m_laneCenterX-m_carWidth, m_straightZoneStartRow, 2*m_carWidth, imgTh.rows), cv::Scalar(255));		
		}
		if(m_imshow){
			// display best road
			Mat tmp;
			Mat tmp2;
			Mat tmp3;
			addWeighted(imgCrc, 0.5, imgTh, 0.5, 0, tmp2);
			addWeighted(imgCenterLineProps, 1.0, imgRightLineProps, 1.0, 0, tmp);		
		
			addWeighted(tmp, 0.5, tmp2, 0.5, 0, tmp3);
		
			cv::imshow("opt", tmp3	);		
			cv::waitKey(1);
		}
		
		
		
		
		/*for (int rho = 160; rho < 800; rho *= 1.2) {
			Mat imgP(m_roiHU-m_roiHO, m_roiVR-m_roiVL, CV_8UC1);
			imgP.setTo(0);
			
			circle(imgP, cv::Point(322+rho, imgP.rows-1), rho, Scalar(255), 130);

			cv::imshow("tmp", imgP);
			cv::waitKey(0);
		}*/
		
		/*tUInt32 mX = 0, mY = 0;
		tUInt32 n  = 0;
		
		unsigned char *pTh = (unsigned char*)imgTh.data;
		for (int i = 0; i < imgTh.rows; i++) {
			for (int j = 0; j < imgTh.cols; j++, ++pTh) {
				if (*pTh > 0) {
					mX += j;
					mY += i;
					++n;
				}
			}
		}
		
		mX /= n;
		
		std::cout << "mX = " << mX << std::endl;
		
		m_steeringAngle = (tFloat32)(((tUInt32)(mX - 315))>>2);*/
	
		/*namedWindow("cLineDetector::ProcessVideo: Top view");
		imshow("cLineDetector::ProcessVideo: Top view", imgTmp);
		namedWindow("cLineDetector::ProcessVideo: Thresholded Image");
		imshow("cLineDetector::ProcessVideo: Thresholded Image", imgTh);
		waitKey(1);*/
		
		
	}
	pISample->Unlock(l_pSrcBuffer);
	//cout << m_steeringAngle << endl;	
	tUInt32 timeStamp = 0;
	TransmitAng(m_steeringAngle);
//	TransmitSteeringAngle(pISample->GetTime(),timeStamp,m_steeringAngle);
	
	RETURN_NOERROR;
}

tResult cLineDetector::UpdateImageFormat(const tBitmapFormat* pFormat) {
	if (pFormat != NULL) {
		m_sInputFormat = (*pFormat);

		LOG_INFO(adtf_util::cString::Format("cLineDetector::UpdateImageFormat: Size %d x %d ; BPL %d ; Size %d , PixelFormat; %d", m_sInputFormat.nWidth,m_sInputFormat.nHeight,m_sInputFormat.nBytesPerLine, m_sInputFormat.nSize, m_sInputFormat.nPixelFormat));		
	}

	RETURN_NOERROR;
}


tResult cLineDetector::extractLines(cv::Mat &imgSrc, cv::Mat &imgLines, float fThGrd) {

Mat imgGrdX;
Mat imgGrdY;
Mat imgGrdM;

imgLines.create(imgSrc.size(), CV_8UC1);
imgLines.setTo(0);
imgGrdX.create(imgSrc.size(), CV_32FC1);
imgGrdY.create(imgSrc.size(), CV_32FC1);
imgGrdM.create(imgSrc.size(), CV_32FC1);
// calculate gradients (5 tap gaussian blur and sobel), normalize gradients and threshold edges
cv::Mat KGrd = (cv::Mat_<float>(5,5) << 0.004255, 0.012765, 0.0, -0.012765, -0.004255,
0.0241, 0.0723, 0.0, -0.0723, -0.0241,
0.04329, 0.12987, 0.0, -0.12987, -0.04329,
0.0241, 0.0723, 0.0, -0.0723, -0.0241,
0.004255, 0.012765, 0.0, -0.012765, -0.004255);
float* pGrdX = (float*)imgGrdX.data;
float* pGrdY = (float*)imgGrdY.data;
float* pGrdM = (float*)imgGrdM.data;	
float* pKGrd = (float*)KGrd.data;	
unsigned char* pSrc = (unsigned char*)imgSrc.data;
for (int y = 2; y < imgGrdM.rows-2; y++) {
unsigned int stride = y * imgGrdM.cols;
for (int x = 2; x < imgGrdM.cols-2; x++, stride++) {
// convolution
float gx = 0, gy = 0;
for (int yK = 0; yK < KGrd.rows; yK++) {
for (int xK = 0; xK < KGrd.cols; xK++) {
gx += pSrc[(y+yK-2)*imgSrc.cols + x+xK-2] * pKGrd[yK*5 + xK];
gy += pSrc[(y+yK-2)*imgSrc.cols + x+xK-2] * pKGrd[xK*5 + yK];
}
}
// write back
float mag = std::sqrt(gx*gx + gy*gy) + FLT_EPSILON;
pGrdX[stride] = gx / mag;
pGrdY[stride] = gy / mag;
pGrdM[stride] = (mag >= fThGrd) ? mag : 0.0f;
}
}
// determine lines with sepcified size
for (int y = 0; y < imgGrdM.rows; y++) {
	pGrdX = (float*)imgGrdX.data + y*imgGrdX.cols;
	pGrdY = (float*)imgGrdY.data + y*imgGrdY.cols;
	pGrdM = (float*)imgGrdM.data + y*imgGrdM.cols;
	for (int x = 0; x < imgGrdM.cols; x++) {
		if (pGrdM[x] > 0) {
			int t = 0;
			int dx = 0, dy = 0;
			float grdX = -pGrdX[x];
			float grdY = -pGrdY[x];
			
			for (; (t < 200) && (x+dx > 0) && (y+dy > 0) && (x+dx < imgSrc.cols-1) && (y+dy < imgSrc.rows-1); t++) {
				dx = (int)((float)t*grdX);
				dy = (int)((float)t*grdY);
				if (pGrdM[x+dx+dy*imgGrdM.cols] == 0) break;
			}
			for (; (t < 200) && (x+dx > 0) && (y+dy > 0) && (x+dx < imgSrc.cols-1) && (y+dy < imgSrc.rows-1); t++) {
				dx = (int)((float)t*grdX);
				dy = (int)((float)t*grdY);
				if (pGrdM[x+dx+dy*imgGrdM.cols] > 0) break;
			}
			if (t >= 6 && t <= 7) {	// CENTER LINE // TODO make line width changeable
				//cv::line(imgBGR, cv::Point(x,y), cv::Point(x+dx, y+dy), cv::Scalar(255,0,0), 2);
				//cv::line(imgLineC, cv::Point(x,y), cv::Point(x+dx, y+dy), cv::Scalar(255), 2);
				//cv::line(imgLineS, cv::Point(x,y), cv::Point(x+dx, y+dy), cv::Scalar(255), 2);
				cv::line(imgLines, cv::Point(x,y), cv::Point(x+dx, y+dy), cv::Scalar(100), 2);
			} else if (t >= 9 && t <= 10) {	// BORDER LINE
				//cv::line(imgBGR, cv::Point(x,y), cv::Point(x+dx, y+dy), cv::Scalar(0,0,255), 2);
				//cv::line(imgLineB, cv::Point(x,y), cv::Point(x+dx, y+dy), cv::Scalar(255), 2);
				//cv::line(imgLineS, cv::Point(x,y), cv::Point(x+dx, y+dy), cv::Scalar(255), 2);
				cv::line(imgLines, cv::Point(x,y), cv::Point(x+dx, y+dy), cv::Scalar(200), 2);
			}
		}
	}
}


RETURN_NOERROR;
}


































	/*SetPropertyInt("Grenze V Links", 0);
	SetPropertyBool("Grenze V Links" NSSUBPROP_ISCHANGEABLE, tTrue);
	SetPropertyInt("Grenze V Links" NSSUBPROP_MINIMUM  , 0);
	SetPropertyInt("Grenze V Links" NSSUBPROP_MAXIMUM  , 640);

	SetPropertyInt("Grenze V Rechts", 640);
	SetPropertyBool("Grenze V Rechts" NSSUBPROP_ISCHANGEABLE, tTrue);
	SetPropertyInt("Grenze V Rechts" NSSUBPROP_MINIMUM  , 0);
	SetPropertyInt("Grenze V Rechts" NSSUBPROP_MAXIMUM  , 640);

	SetPropertyInt("Grenze H Oben", 240);
	SetPropertyBool("Grenze H Oben" NSSUBPROP_ISCHANGEABLE, tTrue);
	SetPropertyInt("Grenze H Oben" NSSUBPROP_MINIMUM  , 0);
	SetPropertyInt("Grenze H Oben" NSSUBPROP_MAXIMUM  , 480);

	SetPropertyInt("Grenze H Unten", 420);
	SetPropertyBool("Grenze H Unten" NSSUBPROP_ISCHANGEABLE, tTrue);
	SetPropertyInt("Grenze H Unten" NSSUBPROP_MINIMUM  , 0);
	SetPropertyInt("Grenze H Unten" NSSUBPROP_MAXIMUM  , 480);
	
	SetPropertyInt("Hoehe ROI", 30);
	SetPropertyBool("Hoehe ROI" NSSUBPROP_ISCHANGEABLE, tTrue);
	SetPropertyInt("Hoehe ROI" NSSUBPROP_MINIMUM  , 0);
	SetPropertyInt("Hoehe ROI" NSSUBPROP_MAXIMUM  , 480);
	
	SetPropertyInt("Abstand ROI", 0);
	SetPropertyBool("Abstand ROI" NSSUBPROP_ISCHANGEABLE, tTrue);
	SetPropertyInt("Abstand ROI" NSSUBPROP_MINIMUM  , 0);
	SetPropertyInt("Abstand ROI" NSSUBPROP_MAXIMUM  , 480);
	
	SetPropertyInt("CC Threshold Oben", 15);
	SetPropertyBool("CC Threshold Oben" NSSUBPROP_ISCHANGEABLE, tTrue);

	SetPropertyInt("CC Threshold Oben" NSSUBPROP_MINIMUM  , 0);
	SetPropertyInt("CC Threshold Oben" NSSUBPROP_MAXIMUM  , 1024);
	
	SetPropertyInt("CC Threshold Unten", 30);
	SetPropertyBool("CC Threshold Unten" NSSUBPROP_ISCHANGEABLE, tTrue);
	SetPropertyInt("CC Threshold Unten" NSSUBPROP_MINIMUM  , 0);

	SetPropertyInt("CC Threshold Unten" NSSUBPROP_MAXIMUM  , 1024);
	
	SetPropertyInt("Lane Offset", 0);
	SetPropertyBool("Lane Offset" NSSUBPROP_ISCHANGEABLE, tTrue);
	SetPropertyInt("Lane Offset" NSSUBPROP_MINIMUM  , -1024);

	SetPropertyInt("Lane Offset" NSSUBPROP_MAXIMUM  , 1024);
	
	SetPropertyBool("showSteps", tTrue);
	SetPropertyBool("showSteps" NSSUBPROP_ISCHANGEABLE, tTrue);


	SetPropertyInt("ThresholdValue", 220);
	SetPropertyBool("ThresholdValue" NSSUBPROP_ISCHANGEABLE, tTrue);*/


	/*RETURN_IF_POINTER_NULL(pMediaSample);
	RETURN_IF_POINTER_NULL(pSource);
	if(nEventCode == IPinEventSink::PE_MediaSampleReceived)
	{
	if(pSource == &iPin_RGBin)
	{
	//Videoformat
	if (firstFrame)
	{		
	cObjectPtr<IMediaType> pType;
	RETURN_IF_FAILED(iPin_RGBin.GetMediaType(&pType));
	cObjectPtr<IMediaTypeVideo> pTypeVideo;
	RETURN_IF_FAILED(pType->GetInterface(IID_ADTF_MEDIA_TYPE_VIDEO, (tVoid**)&pTypeVideo));
	const tBitmapFormat* pFormat = pTypeVideo->GetFormat();								
	if (pFormat == NULL)
	{
	LOG_ERROR("dhLineDetector: No Bitmap information found on pin \"input\"");
	RETURN_ERROR(ERR_NOT_SUPPORTED);
	}
	m_sInputFormat.nPixelFormat = pFormat->nPixelFormat;
	m_sInputFormat.nWidth = pFormat->nWidth;
	m_sInputFormat.nHeight =  pFormat->nHeight;
	m_sInputFormat.nBitsPerPixel = pFormat->nBitsPerPixel;
	m_sInputFormat.nBytesPerLine = pFormat->nBytesPerLine;
	m_sInputFormat.nSize = pFormat->nSize;
	m_sInputFormat.nPaletteSize = pFormat->nPaletteSize;
	firstFrame = tFalse;
	}

	ProcessInput(pMediaSample);

	m_imgCount++;
	} 
	}
	if (nEventCode== IPinEventSink::PE_MediaTypeChanged)
	{

	if (pSource == &iPin_RGBin)

	{
	cObjectPtr<IMediaType> pType;
	RETURN_IF_FAILED(iPin_RGBin.GetMediaType(&pType));
	cObjectPtr<IMediaTypeVideo> pTypeVideo;
	RETURN_IF_FAILED(pType->GetInterface(IID_ADTF_MEDIA_TYPE_VIDEO, (tVoid**)&pTypeVideo));

	UpdateImageFormat(iPin_RGBin.GetFormat());
	}
	}

	RETURN_NOERROR;*/





/*bool cmpCC (const ConnectedComponent& a, const ConnectedComponent& b) {
	return a.number_of_pixels < b.number_of_pixels;
}*/



	/*RETURN_IF_POINTER_NULL(pSample);

	cObjectPtr<IMediaSample> pNewRGBSample;
	const tVoid* l_pSrcBuffer;

	if (IS_OK(pSample->Lock(&l_pSrcBuffer))) {
		// read image into mat
		cv::Mat img(m_sInputFormat.nHeight, m_sInputFormat.nWidth, CV_8UC3, (char*)l_pSrcBuffer);
		
		
		pSample->Unlock(l_pSrcBuffer);
		// convert to grayscale
		cvtColor(img, gray, CV_RGB2GRAY);
		
		//char fileName[80];
		//sprintf(fileName, "/home/odroid/calibration_pattern/gray%04d.png", m_imgCount);
		//cv::imwrite(fileName, gray);

		
		for (int iR = 0; iR < N_ROI; iR++) imgRoi[iR] = gray(cv::Rect(m_roiVL, m_roiHO + iR*(m_roiHeight + m_roiDist), m_roiVR-m_roiVL, m_roiHeight));
		for (int iR = 0; iR < N_ROI; iR++) {
			detectWhiteRegions(imgRoi[iR], imgLines[iR], m_thCC_min + (float)(m_thCC_max-m_thCC_min)/(N_ROI-1)*iR);

			if (binCC[iR].size() > 0) binCC[iR].clear();
			determineConnectedComponents(imgLines[iR], binCC[iR]);
			removeSmallRegions(imgLines[iR], binCC[iR], m_thCC_min + (float)(m_thCC_max-m_thCC_min)/(N_ROI-1)*iR);

			std::sort(binCC[iR].begin(), binCC[iR].end(), cmpCC);
		}*/
		
		/*cv::imshow("(0)", imgLines[0]);
		cv::imshow("(1)", imgLines[1]);
		cv::imshow("(2)", imgLines[2]);
		cv::imshow("(3)", imgLines[3]);
		cv::imshow("(4)", imgLines[4]);
		cv::imshow("(5)", imgLines[5]);
		cv::waitKey(1);*/
		/*
		
		binCCLeft.clear();
		binCCRight.clear();
		for (int iR = 0; iR < N_ROI; iR++) {
			if (binCC[iR].size() == 2) {
				tInt x1 = ((binCC[iR][0].min_expansion_point.x+m_roiVL+binCC[iR][0].max_expansion_point.x+m_roiVL)>>1);
				tInt x2 = ((binCC[iR][1].min_expansion_point.x+m_roiVL+binCC[iR][1].max_expansion_point.x+m_roiVL)>>1);
				
				if (x1 < x2) {
					binCCLeft.push_back(binCC[iR][0]);
					binCCRight.push_back(binCC[iR][1]);
				} else {

					binCCLeft.push_back(binCC[iR][1]);
					binCCRight.push_back(binCC[iR][0]);
				}
				
				binCCLeft.back().min_expansion_point.x += m_roiVL;
				binCCLeft.back().min_expansion_point.y += m_roiHO + iR*(m_roiHeight + m_roiDist);
				binCCLeft.back().max_expansion_point.x += m_roiVL;

				binCCLeft.back().max_expansion_point.y += m_roiHO + iR*(m_roiHeight + m_roiDist);
				binCCRight.back().min_expansion_point.x += m_roiVL;
				binCCRight.back().min_expansion_point.y += m_roiHO + iR*(m_roiHeight + m_roiDist);
				binCCRight.back().max_expansion_point.x += m_roiVL;

				binCCRight.back().max_expansion_point.y += m_roiHO + iR*(m_roiHeight + m_roiDist);
			} else {
				for (size_t iCC = 0; iCC < binCC[iR].size(); iCC++) {
					tInt x = ((binCC[iR][iCC].min_expansion_point.x+m_roiVL+binCC[iR][iCC].max_expansion_point.x+m_roiVL)>>1);
					if (x < m_nFadenkreuzV) {
						binCCLeft.push_back(binCC[iR][iCC]);
						binCCLeft.back().min_expansion_point.x += m_roiVL;
						binCCLeft.back().min_expansion_point.y += m_roiHO + iR*(m_roiHeight + m_roiDist);
						binCCLeft.back().max_expansion_point.x += m_roiVL;
						binCCLeft.back().max_expansion_point.y += m_roiHO + iR*(m_roiHeight + m_roiDist);

					} else {
						binCCRight.push_back(binCC[iR][iCC]);
						binCCRight.back().min_expansion_point.x += m_roiVL;
						binCCRight.back().min_expansion_point.y += m_roiHO + iR*(m_roiHeight + m_roiDist);
						binCCRight.back().max_expansion_point.x += m_roiVL;

						binCCRight.back().max_expansion_point.y += m_roiHO + iR*(m_roiHeight + m_roiDist);
					}
				}
			}
		}

		
		
		
		
		// calculate lines
		cv::Mat AL(binCCLeft.size(), 2, CV_32FC1);

		cv::Mat BL(binCCLeft.size(), 1, CV_32FC1);
		cv::Mat AR(binCCRight.size(), 2, CV_32FC1);
		cv::Mat BR(binCCRight.size(), 1, CV_32FC1);
		
		float *pAL = (float*)AL.data;

		float *pBL = (float*)BL.data;
		for (size_t iCCL = 0; iCCL < binCCLeft.size(); iCCL++) {
			pAL[2*iCCL + 0] = (float)((binCCLeft[iCCL].min_expansion_point.x + binCCLeft[iCCL].max_expansion_point.x)>>1);
			pAL[2*iCCL + 1] = 1.0f;
			pBL[iCCL] = (float)((binCCLeft[iCCL].min_expansion_point.y + binCCLeft[iCCL].max_expansion_point.y)>>1);

		}
		
		float *pAR = (float*)AR.data;
		float *pBR = (float*)BR.data;
		for (size_t iCCR = 0; iCCR < binCCRight.size(); iCCR++) {

			pAR[2*iCCR + 0] = (float)((binCCRight[iCCR].min_expansion_point.x + binCCRight[iCCR].max_expansion_point.x)>>1);
			pAR[2*iCCR + 1] = 1.0f;
			pBR[iCCR] = (float)((binCCRight[iCCR].min_expansion_point.y + binCCRight[iCCR].max_expansion_point.y)>>1);
		}
		
		XL.create(2,1,CV_32FC1);

		XR.create(2,1,CV_32FC1);
		XL.setTo(0);
		XR.setTo(0);
		if (binCCLeft.size() >= 2) cv::solve(AL, BL, XL, cv::DECOMP_SVD);
		if (binCCRight.size() >= 2) cv::solve(AR, BR, XR, cv::DECOMP_SVD);

		
		XI.create(2,1,CV_32FC1);
		XI.setTo(0);
		cv::Mat AI(2, 2, CV_32FC1);
		cv::Mat BI(2, 1, CV_32FC1);

		float *pAI = (float*)AI.data;
		float *pBI = (float*)BI.data;
		float *pXL = (float*)XL.data;
		float *pXR = (float*)XR.data;
		pAI[0] = -pXL[0]; pAI[1] = 1.0f; pBI[0] = pXL[1];

		pAI[2] = -pXR[0]; pAI[3] = 1.0f; pBI[1] = pXR[1];
		cv::solve(AI, BI, XI, cv::DECOMP_SVD);
		
		float *pXI = (float*)XI.data;
		m_steeringAngle = (tInt)pXI[0] - (m_sInputFormat.nWidth >> 1) - m_laneOffset;

		transmitSteeringAngle();*/
		
		
		//std::cout << XL << std::endl;
		//std::cout << XR << std::endl;
		
		
		/*cGCLWriter::StoreCommand(pc, GCL_CMD_DRAWRECT, binCCRight[iCCL].min_expansion_point.x+m_roiVL, binCCRight[iCCL].min_expansion_point.y+m_roiHO + 0*(m_roiHeight + m_roiDist), binCCRight[iCCL].max_expansion_point.x+m_roiVL, binCCRight[iCCL].max_expansion_point.y+m_roiHO + 0*(m_roiHeight + m_roiDist));
		cGCLWriter::StoreCommand(pc, GCL_CMD_FILLCIRCLE, ((binCCRight[iCCL].min_expansion_point.x+m_roiVL+binCCRight[iCCL].max_expansion_point.x+m_roiVL)>>1), ((binCCRight[iCCL].min_expansion_point.y+m_roiHO+0*(m_roiHeight + m_roiDist)+binCCRight[iCCL].max_expansion_point.y+m_roiHO+0*(m_roiHeight + m_roiDist))>>1), 5);*/
		
		/*detectWhiteRegions(gray, white_regions);
		

		if (connected_components.size() > 0)
			connected_components.clear();
		determineConnectedComponents(white_regions, connected_components);
		
		removeSmallRegions(white_regions, connected_components, 30);
		
		std::sort(connected_components.begin(), connected_components.end(), cmpCC);
		
		// Display the image with resulting lines
		if (m_showSteps) {
			imshow("lineDetector: Gray image only", white_regions);
			cv::waitKey(1);
		}*/
		
		// TODO begin Nicolai
		/*cv::Mat dist;
		electroMagneticTransform(white_regions, dist);
		

		cv::normalize(dist, dist, 0.0f, 1.0f, cv::NORM_MINMAX);
		cv::imshow("dist map", dist);
		cv::waitKey(1);*/
		// TODO end Nicolai
		/*if (connected_components.size() > 0) {
			m_steeringAngle = m_roiVL + ((connected_components[0].min_expansion_point.x + connected_components[0].max_expansion_point.x) >> 1) - (m_sInputFormat.nWidth >> 1) - m_laneOffset;

		} else {
			m_steeringAngle = 30;
		}*/
		//std::cout << "m_steeringAngle = " << m_steeringAngle << ", stopLine = " << (m_stopLine ? "true" : "false") << std::endl;
		//LOG_INFO(cString::Format("cLineDetector::ProcessInput: m_steeringAngle = %f, m_stopLine = %s", m_steeringAngle, (m_stopLine ? "true" : "false")));
		//printf("cLineDetector::ProcessInput: m_steeringAngle = %f, m_stopLine = %s\n", m_steeringAngle, (m_stopLine ? "true" : "false"));
		
		//transmitGCL();

/*
tResult cLineDetector::transmitGCL() {
	if (!oPin_GCLout.IsConnected()) {
        RETURN_NOERROR;
    }

    cObjectPtr<IMediaSample> pSample;
    RETURN_IF_FAILED(AllocMediaSample((tVoid**)&pSample));
    
    RETURN_IF_FAILED(pSample->AllocBuffer(8192));

    pSample->SetTime(_clock->GetStreamTime());

    tUInt32* aGCLProc;
    RETURN_IF_FAILED(pSample->WriteLock((tVoid**)&aGCLProc));
	
    tUInt32* pc = aGCLProc;
	// DRAWING
	// draw image box
    cGCLWriter::StoreCommand(pc, GCL_CMD_FGCOL, cColor(0, 0, 0).GetRGBA());	
	cGCLWriter::StoreCommand(pc, GCL_CMD_DRAWRECT, 0, 0, m_sInputFormat.nWidth, m_sInputFormat.nHeight);
	
	
	
	// draw ROI
	cGCLWriter::StoreCommand(pc, GCL_CMD_FGCOL, cColor(255, 0, 255).GetRGBA());
	//cGCLWriter::StoreCommand(pc, GCL_CMD_DRAWRECT, m_roiVL, m_roiHO, m_roiVR, m_roiHU);
	for (int iR = 0; iR < N_ROI; iR++) cGCLWriter::StoreCommand(pc, GCL_CMD_DRAWRECT, m_roiVL, m_roiHO + iR*(m_roiHeight + m_roiDist), m_roiVR, m_roiHO + iR*(m_roiHeight + m_roiDist) + m_roiHeight);
	
	// draw bounding boxes of connected_components
	cGCLWriter::StoreCommand(pc, GCL_CMD_FGCOL, cColor(255, 0, 0).GetRGBA());
	for (size_t iCCL = 0; iCCL < binCCLeft.size(); iCCL++) {
		cGCLWriter::StoreCommand(pc, GCL_CMD_DRAWRECT, binCCLeft[iCCL].min_expansion_point.x, binCCLeft[iCCL].min_expansion_point.y, binCCLeft[iCCL].max_expansion_point.x, binCCLeft[iCCL].max_expansion_point.y);
		cGCLWriter::StoreCommand(pc, GCL_CMD_FILLCIRCLE, ((binCCLeft[iCCL].min_expansion_point.x+binCCLeft[iCCL].max_expansion_point.x)>>1), ((binCCLeft[iCCL].min_expansion_point.y+binCCLeft[iCCL].max_expansion_point.y)>>1), 5);
		/*cGCLWriter::StoreCommand(pc, GCL_CMD_DRAWRECT, binCCLeft[iCCL].min_expansion_point.x+m_roiVL, binCCLeft[iCCL].min_expansion_point.y+m_roiHO + 0*(m_roiHeight + m_roiDist), binCCLeft[iCCL].max_expansion_point.x+m_roiVL, binCCLeft[iCCL].max_expansion_point.y+m_roiHO + 0*(m_roiHeight + m_roiDist));
		cGCLWriter::StoreCommand(pc, GCL_CMD_FILLCIRCLE, ((binCCLeft[iCCL].min_expansion_point.x+m_roiVL+binCCLeft[iCCL].max_expansion_point.x+m_roiVL)>>1), ((binCCLeft[iCCL].min_expansion_point.y+m_roiHO+0*(m_roiHeight + m_roiDist)+binCCLeft[iCCL].max_expansion_point.y+m_roiHO+0*(m_roiHeight + m_roiDist))>>1), 5);*/
	/*}
	cGCLWriter::StoreCommand(pc, GCL_CMD_FGCOL, cColor(0, 255, 0).GetRGBA());
	for (size_t iCCL = 0; iCCL < binCCRight.size(); iCCL++) {
		cGCLWriter::StoreCommand(pc, GCL_CMD_DRAWRECT, binCCRight[iCCL].min_expansion_point.x, binCCRight[iCCL].min_expansion_point.y, binCCRight[iCCL].max_expansion_point.x, binCCRight[iCCL].max_expansion_point.y);
		cGCLWriter::StoreCommand(pc, GCL_CMD_FILLCIRCLE, ((binCCRight[iCCL].min_expansion_point.x+binCCRight[iCCL].max_expansion_point.x)>>1), ((binCCRight[iCCL].min_expansion_point.y+binCCRight[iCCL].max_expansion_point.y)>>1), 5);
	}
	
	*/
	/*cGCLWriter::StoreCommand(pc, GCL_CMD_FGCOL, cColor(0, 255, 255).GetRGBA());
	for (int iR = 0; iR < N_ROI; iR++) {
		for(std::vector<ConnectedComponent>::iterator itCC = binCC[iR].begin(); itCC != binCC[iR].end(); itCC++) {
			cGCLWriter::StoreCommand(pc, GCL_CMD_DRAWRECT, itCC->min_expansion_point.x+m_roiVL, itCC->min_expansion_point.y+m_roiHO + iR*(m_roiHeight + m_roiDist), itCC->max_expansion_point.x+m_roiVL, itCC->max_expansion_point.y+m_roiHO + iR*(m_roiHeight + m_roiDist));

			cGCLWriter::StoreCommand(pc, GCL_CMD_FILLCIRCLE, ((itCC->min_expansion_point.x+m_roiVL+itCC->max_expansion_point.x+m_roiVL)>>1), ((itCC->min_expansion_point.y+m_roiHO+iR*(m_roiHeight + m_roiDist)+itCC->max_expansion_point.y+m_roiHO+iR*(m_roiHeight + m_roiDist))>>1), 5);
			//cGCLWriter::StoreCommand(pc, GCL_CMD_DRAWLINE, compo_iter->min_expansion_point.x+m_roiVL, compo_iter->min_expansion_point.y+m_roiHO, compo_iter->max_expansion_point.x+m_roiVL, compo_iter->max_expansion_point.y+m_roiHO);

		}
	}*/
	
		/*for(std::vector<ConnectedComponent>::iterator compo_iter = connected_components.begin(); compo_iter != connected_components.end(); compo_iter++) {
		cGCLWriter::StoreCommand(pc, GCL_CMD_DRAWRECT, compo_iter->min_expansion_point.x+m_roiVL, compo_iter->min_expansion_point.y+m_roiHO, compo_iter->max_expansion_point.x+m_roiVL, compo_iter->max_expansion_point.y+m_roiHO);

		cGCLWriter::StoreCommand(pc, GCL_CMD_DRAWLINE, compo_iter->min_expansion_point.x+m_roiVL, compo_iter->min_expansion_point.y+m_roiHO, compo_iter->max_expansion_point.x+m_roiVL, compo_iter->max_expansion_point.y+m_roiHO);
	}*/
	
	// draw calculated lines
/*	cGCLWriter::StoreCommand(pc, GCL_CMD_FGCOL, cColor(0, 0, 255).GetRGBA());
	float *pXL = (float*)XL.data;
	float *pXR = (float*)XR.data;
	cGCLWriter::StoreCommand(pc, GCL_CMD_DRAWLINE, (tInt)(-pXL[1]/pXL[0]), 0, (tInt)((m_sInputFormat.nHeight-1-pXL[1])/pXL[0]), (tInt)(m_sInputFormat.nHeight-1));
	cGCLWriter::StoreCommand(pc, GCL_CMD_DRAWLINE, (tInt)(-pXR[1]/pXR[0]), 0, (tInt)((m_sInputFormat.nHeight-1-pXR[1])/pXR[0]), (tInt)(m_sInputFormat.nHeight-1));
	
	// draw Intersection
	float *pXI = (float*)XI.data;
	cGCLWriter::StoreCommand(pc, GCL_CMD_FILLCIRCLE, (tInt)(pXI[0]), (tInt)(pXI[1]), 10);
	
	// end of drawing
	cGCLWriter::StoreCommand(pc, GCL_CMD_END);

    pSample->Unlock(aGCLProc);

    RETURN_IF_FAILED(oPin_GCLout.Transmit(pSample));
	RETURN_NOERROR;
}

tResult cLineDetector::transmitStopLine() {
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
	cObjectPtr<IMediaCoder> pCoder;
	m_pCoderDescSignalOutput->WriteLock(pMediaSample, &pCoder);	

	pCoder->Set("bValue", (tVoid*)&(m_stopLine));	
	pCoder->Set("ui32ArduinoTimestamp", (tVoid*)&timeStamp);    
	m_pCoderDescSignalOutput->Unlock(pCoder);


	//transmit media sample over output pin
	pMediaSample->SetTime(_clock->GetStreamTime());
	oPin_stopLine.Transmit(pMediaSample);
	
	RETURN_NOERROR;
}

tResult cLineDetector::transmitSteeringAngle() {
    tFloat32 flValue= ((tFloat32)m_steeringAngle)*0.35f;

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
    cObjectPtr<IMediaCoder> pCoder;
    m_pCoderDescSignalOutput->WriteLock(pMediaSample, &pCoder);    
        
    pCoder->Set("f32Value", (tVoid*)&(flValue));    
    pCoder->Set("ui32ArduinoTimestamp", (tVoid*)&timeStamp);    
    m_pCoderDescSignalOutput->Unlock(pCoder);
    
    //transmit media sample over output pin
    pMediaSample->SetTime(_clock->GetStreamTime());
    oPin_steeringAngle.Transmit(pMediaSample);
    
    RETURN_NOERROR;
}*/





/*tResult cLineDetector::Init(tInitStage eStage, __exception )
{
    RETURN_IF_FAILED(cFilter::Init(eStage, __exception_ptr));

	if (eStage == StageFirst) {

		// get a media type for the input pin
		cObjectPtr<IMediaDescriptionManager> pDescManager;
		RETURN_IF_FAILED(_runtime->GetObject(OID_ADTF_MEDIA_DESCRIPTION_MANAGER,IID_ADTF_MEDIA_DESCRIPTION_MANAGER,(tVoid**)&pDescManager,__exception_ptr));

		// Video Input
		RETURN_IF_FAILED(iPin_RGBin.Create("videoInput", IPin::PD_Input, static_cast<IPinEventSink*>(this)));
		   RETURN_IF_FAILED(RegisterPin(&iPin_RGBin));

		//GLC Output
		cObjectPtr<IMediaType> pCmdType = NULL;
		RETURN_IF_FAILED(AllocMediaType(&pCmdType, MEDIA_TYPE_COMMAND, MEDIA_SUBTYPE_COMMAND_GCL, __exception_ptr));
		RETURN_IF_FAILED(oPin_GCLout.Create("gcl",pCmdType, static_cast<IPinEventSink*> (this)));
		RETURN_IF_FAILED(RegisterPin(&oPin_GCLout));

	 	// get a media type for the output pin
		tChar const * strDescSignalValueOutput = pDescManager->GetMediaDescription("tSignalValue");
		RETURN_IF_POINTER_NULL(strDescSignalValueOutput);        
		cObjectPtr<IMediaType> pTypeSignalValueOutput = new cMediaType(0, 0, 0, "tSignalValue", strDescSignalValueOutput,IMediaDescription::MDF_DDL_DEFAULT_VERSION);	
		RETURN_IF_FAILED(pTypeSignalValueOutput->GetInterface(IID_ADTF_MEDIA_TYPE_DESCRIPTION, (tVoid**)&m_pCoderDescSignalOutput)); 
		
		tChar const * strDescBoolSignalValue = pDescManager->GetMediaDescription("tBoolSignalValue");
		RETURN_IF_POINTER_NULL(strDescBoolSignalValue);        
		cObjectPtr<IMediaType> pTypeBoolSignalValueOutput = new cMediaType(0, 0, 0, "tBoolSignalValue", strDescBoolSignalValue,IMediaDescription::MDF_DDL_DEFAULT_VERSION);	
		RETURN_IF_FAILED(pTypeBoolSignalValueOutput->GetInterface(IID_ADTF_MEDIA_TYPE_DESCRIPTION, (tVoid**)&m_pCoderDescBoolSignalOutput));

		// Steer Angle Output
		RETURN_IF_FAILED(oPin_steeringAngle.Create("steeringAngle", pTypeSignalValueOutput, static_cast<IPinEventSink*> (this)));
		RETURN_IF_FAILED(RegisterPin(&oPin_steeringAngle));
		    
		RETURN_IF_FAILED(oPin_stopLine.Create("stopLine", pTypeSignalValueOutput, static_cast<IPinEventSink*> (this)));
		RETURN_IF_FAILED(RegisterPin(&oPin_stopLine));
	} else if (eStage == StageNormal) {
		firstFrame = tTrue;
		imagecount = 0;


		m_showSteps = GetPropertyBool("showSteps");
		m_nthresholdvalue = GetPropertyInt("ThresholdValue");
	
		m_nFadenkreuzH = GetPropertyInt("FadenkreuzH");
		m_nFadenkreuzV = GetPropertyInt("FadenkreuzV");

		m_roiVL = GetPropertyInt("Grenze V Links");
		m_roiVR = GetPropertyInt("Grenze V Rechts");
		m_roiHO = GetPropertyInt("Grenze H Oben");
		m_roiHU = GetPropertyInt("Grenze H Unten");
		

		m_laneOffset = GetPropertyInt("Lane Offset");
		
		m_roiHeight = GetPropertyInt("Hoehe ROI");
		m_roiDist = GetPropertyInt("Abstand ROI");
		m_thCC_min = GetPropertyInt("CC Threshold Oben");

		m_thCC_max = GetPropertyInt("CC Threshold Unten");
	
		//m_roiVL = std::min(std::max(0, m_roiVL), m_sInputFormat.nWidth-1);
		//m_roiVR = std::min(std::max(0, m_roiVR), m_sInputFormat.nWidth-1);
		//m_roiHO = std::min(std::max(0, m_roiHO), m_sInputFormat.nHeight-1);
		//m_roiHU = std::min(std::max(0, m_roiHU), m_sInputFormat.nHeight-1);
	}
	RETURN_NOERROR;
}*/
