/*
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Copyright (c) 1999-2003 Apple Computer, Inc.  All Rights Reserved.
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 *
 */
/*
    File:       RTPStream.cpp

    Contains:   Implementation of RTPStream class. 
                    
    

*/



#include <stdlib.h>
#include "SafeStdLib.h"
#include "RTPStream.h"

#include "QTSSModuleUtils.h"
#include "QTSServerInterface.h"
#include "OS.h"

#include "RTCPPacket.h"
#include "RTCPAPPPacket.h"
#include "RTCPAckPacket.h"
#include "RTCPSRPacket.h"
#include "SocketUtils.h"
#include <errno.h>

#if DEBUG
#define RTP_TCP_STREAM_DEBUG 1
#else
#define RTP_TCP_STREAM_DEBUG 0
#endif

QTSSAttrInfoDict::AttrInfo  RTPStream::sAttributes[] = 
{   /*fields:   fAttrName, fFuncPtr, fAttrDataType, fAttrPermission */
    /* 0  */ { "qtssRTPStrTrackID",                 NULL,   qtssAttrDataTypeUInt32, qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite },
    /* 1  */ { "qtssRTPStrSSRC",                    NULL,   qtssAttrDataTypeUInt32, qtssAttrModeRead | qtssAttrModePreempSafe }, 
    /* 2  */ { "qtssRTPStrPayloadName",             NULL,   qtssAttrDataTypeCharArray,  qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite   },
    /* 3  */ { "qtssRTPStrPayloadType",             NULL,   qtssAttrDataTypeUInt32, qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite   },
    /* 4  */ { "qtssRTPStrFirstSeqNumber",          NULL,   qtssAttrDataTypeSInt16, qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite   },
    /* 5  */ { "qtssRTPStrFirstTimestamp",          NULL,   qtssAttrDataTypeSInt32, qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite   },
    /* 6  */ { "qtssRTPStrTimescale",               NULL,   qtssAttrDataTypeSInt32, qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite   },
    /* 7  */ { "qtssRTPStrQualityLevel",            NULL,   qtssAttrDataTypeUInt32, qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite   },
    /* 8  */ { "qtssRTPStrNumQualityLevels",        NULL,   qtssAttrDataTypeUInt32, qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite   },
    /* 9  */ { "qtssRTPStrBufferDelayInSecs",       NULL,   qtssAttrDataTypeFloat32,    qtssAttrModeRead | qtssAttrModePreempSafe | qtssAttrModeWrite   },
    
    /* 10 */ { "qtssRTPStrFractionLostPackets",     NULL,   qtssAttrDataTypeUInt32, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 11 */ { "qtssRTPStrTotalLostPackets",        NULL,   qtssAttrDataTypeUInt32, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 12 */ { "qtssRTPStrJitter",                  NULL,   qtssAttrDataTypeUInt32, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 13 */ { "qtssRTPStrRecvBitRate",             NULL,   qtssAttrDataTypeUInt32, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 14 */ { "qtssRTPStrAvgLateMilliseconds",     NULL,   qtssAttrDataTypeUInt16, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 15 */ { "qtssRTPStrPercentPacketsLost",      NULL,   qtssAttrDataTypeUInt16, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 16 */ { "qtssRTPStrAvgBufDelayInMsec",       NULL,   qtssAttrDataTypeUInt16, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 17 */ { "qtssRTPStrGettingBetter",           NULL,   qtssAttrDataTypeUInt16, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 18 */ { "qtssRTPStrGettingWorse",            NULL,   qtssAttrDataTypeUInt16, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 19 */ { "qtssRTPStrNumEyes",                 NULL,   qtssAttrDataTypeUInt32, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 20 */ { "qtssRTPStrNumEyesActive",           NULL,   qtssAttrDataTypeUInt32, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 21 */ { "qtssRTPStrNumEyesPaused",           NULL,   qtssAttrDataTypeUInt32, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 22 */ { "qtssRTPStrTotPacketsRecv",          NULL,   qtssAttrDataTypeUInt32, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 23 */ { "qtssRTPStrTotPacketsDropped",       NULL,   qtssAttrDataTypeUInt16, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 24 */ { "qtssRTPStrTotPacketsLost",          NULL,   qtssAttrDataTypeUInt16, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 25 */ { "qtssRTPStrClientBufFill",           NULL,   qtssAttrDataTypeUInt16, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 26 */ { "qtssRTPStrFrameRate",               NULL,   qtssAttrDataTypeUInt16, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 27 */ { "qtssRTPStrExpFrameRate",            NULL,   qtssAttrDataTypeUInt16, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 28 */ { "qtssRTPStrAudioDryCount",           NULL,   qtssAttrDataTypeUInt16, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 29 */ { "qtssRTPStrIsTCP",                   NULL,   qtssAttrDataTypeBool16, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 30 */ { "qtssRTPStrStreamRef",               NULL,   qtssAttrDataTypeQTSS_StreamRef, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 31 */ { "qtssRTPStrTransportType",           NULL,   qtssAttrDataTypeUInt32, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 32 */ { "qtssRTPStrStalePacketsDropped",     NULL,   qtssAttrDataTypeUInt32, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 33 */ { "qtssRTPStrCurrentAckTimeout",       NULL,   qtssAttrDataTypeUInt32, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 34 */ { "qtssRTPStrCurPacketsLostInRTCPInterval",    NULL,   qtssAttrDataTypeUInt32, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 35 */ { "qtssRTPStrPacketCountInRTCPInterval",       NULL,   qtssAttrDataTypeUInt32, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 36 */ { "qtssRTPStrSvrRTPPort",              NULL,   qtssAttrDataTypeUInt16, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 37 */ { "qtssRTPStrClientRTPPort",           NULL,   qtssAttrDataTypeUInt16, qtssAttrModeRead | qtssAttrModePreempSafe  },
    /* 38 */ { "qtssRTPStrNetworkMode",             NULL,   qtssAttrDataTypeUInt32, qtssAttrModeRead | qtssAttrModePreempSafe  }

};

StrPtrLen   RTPStream::sChannelNums[] =
{
    StrPtrLen("0"),
    StrPtrLen("1"),
    StrPtrLen("2"),
    StrPtrLen("3"),
    StrPtrLen("4"),
    StrPtrLen("5"),
    StrPtrLen("6"),
    StrPtrLen("7"),
    StrPtrLen("8"),
    StrPtrLen("9")
};

char *RTPStream::noType = "no-type";
char *RTPStream::UDP = "UDP";
char *RTPStream::RUDP = "RUDP";
char *RTPStream::TCP = "TCP";

QTSS_ModuleState RTPStream::sRTCPProcessModuleState = { NULL, 0, NULL, false };

void    RTPStream::Initialize()
{
    for (int x = 0; x < qtssRTPStrNumParams; x++)
        QTSSDictionaryMap::GetMap(QTSSDictionaryMap::kRTPStreamDictIndex)->
            SetAttribute(x, sAttributes[x].fAttrName, sAttributes[x].fFuncPtr,
                sAttributes[x].fAttrDataType, sAttributes[x].fAttrPermission);
}

RTPStream::RTPStream(UInt32 inSSRC, RTPSessionInterface* inSession)
:   QTSSDictionary(QTSSDictionaryMap::GetMap(QTSSDictionaryMap::kRTPStreamDictIndex), NULL),
    fLastQualityChange(0),
    fSockets(NULL),
    fSession(inSession),
    fBytesSentThisInterval(0),
    fDisplayCount(0),
    fSawFirstPacket(false),
    fTracker(NULL),
    fRemoteAddr(0),
    fRemoteRTPPort(0),
    fRemoteRTCPPort(0),
    fLocalRTPPort(0),
    fLastSenderReportTime(0),
    fPacketCount(0),
    fLastPacketCount(0),
    fPacketCountInRTCPInterval(0),
    fByteCount(0),
    fTrackID(0),
    fSsrc(inSSRC),
    fSsrcStringPtr(fSsrcString, 0),
    fEnableSSRC(false),
    fPayloadType(qtssUnknownPayloadType),
    fFirstSeqNumber(0),
    fFirstTimeStamp(0),
    fTimescale(0),
    fStreamURLPtr(fStreamURL, 0),
    fQualityLevel(0),
    fNumQualityLevels(0),
    fLastRTPTimestamp(0),
    
    fFractionLostPackets(0),
    fTotalLostPackets(0),
    //fPriorTotalLostPackets(0),
    fJitter(0),
    fReceiverBitRate(0),
    fAvgLateMsec(0),
    fPercentPacketsLost(0),
    fAvgBufDelayMsec(0),
    fIsGettingBetter(0),
    fIsGettingWorse(0),
    fNumEyes(0),
    fNumEyesActive(0),
    fNumEyesPaused(0),
    fTotalPacketsRecv(0),
    fTotalPacketsDropped(0),
    fTotalPacketsLost(0),
    fCurPacketsLostInRTCPInterval(0),
    fClientBufferFill(0),
    fFrameRate(0),
    fExpectedFrameRate(0),
    fAudioDryCount(0),
    fClientSSRC(0),
    fIsTCP(false),
    fTransportType(qtssRTPTransportTypeUDP),
    fTurnThinningOffDelay_TCP(0),
    fIncreaseThinningDelay_TCP(0),
    fDropAllPacketsForThisStreamDelay_TCP(0),
    fStalePacketsDropped_TCP(0),
    fTimeStreamCaughtUp_TCP(0),
    fLastQualityLevelIncreaseTime_TCP(0),

    fThinAllTheWayDelay(0),
    fAlwaysThinDelay(0),
    fStartThinningDelay(0),
    fStartThickingDelay(0),
    fThickAllTheWayDelay(0),
    fQualityCheckInterval(0),
    fDropAllPacketsForThisStreamDelay(0),
    fStalePacketsDropped(0),
    fLastCurrentPacketDelay(0),
    fWaitOnLevelAdjustment(true),
    fBufferDelay(3.0),
    fLateToleranceInSec(0),
    fCurrentAckTimeout(0),
    fMaxSendAheadTimeMSec(0),
    fRTPChannel(0),
    fRTCPChannel(0),
    fNetworkMode(qtssRTPNetworkModeDefault),
    fStreamStartTimeOSms(OS::Milliseconds())
{

    fStreamRef = this;
#if DEBUG
    fNumPacketsDroppedOnTCPFlowControl = 0;
    fFlowControlStartedMsec = 0;
    fFlowControlDurationMsec = 0;
#endif
    //format the ssrc as a string
    qtss_sprintf(fSsrcString, "%lu", fSsrc);
    fSsrcStringPtr.Len = ::strlen(fSsrcString);
    Assert(fSsrcStringPtr.Len < kMaxSsrcSizeInBytes);

    // SETUP DICTIONARY ATTRIBUTES
    
    this->SetVal(qtssRTPStrTrackID,             &fTrackID,              sizeof(fTrackID));
    this->SetVal(qtssRTPStrSSRC,                &fSsrc,                 sizeof(fSsrc));
    this->SetEmptyVal(qtssRTPStrPayloadName,    &fPayloadNameBuf,       kDefaultPayloadBufSize);
    this->SetVal(qtssRTPStrPayloadType,         &fPayloadType,          sizeof(fPayloadType));
    this->SetVal(qtssRTPStrFirstSeqNumber,      &fFirstSeqNumber,       sizeof(fFirstSeqNumber));
    this->SetVal(qtssRTPStrFirstTimestamp,      &fFirstTimeStamp,       sizeof(fFirstTimeStamp));
    this->SetVal(qtssRTPStrTimescale,           &fTimescale,            sizeof(fTimescale));
    this->SetVal(qtssRTPStrQualityLevel,        &fQualityLevel,         sizeof(fQualityLevel));
    this->SetVal(qtssRTPStrNumQualityLevels,    &fNumQualityLevels,     sizeof(fNumQualityLevels));
    this->SetVal(qtssRTPStrBufferDelayInSecs,   &fBufferDelay,          sizeof(fBufferDelay));
    this->SetVal(qtssRTPStrFractionLostPackets, &fFractionLostPackets,  sizeof(fFractionLostPackets));
    this->SetVal(qtssRTPStrTotalLostPackets,    &fTotalLostPackets,     sizeof(fTotalLostPackets));
    this->SetVal(qtssRTPStrJitter,              &fJitter,               sizeof(fJitter));
    this->SetVal(qtssRTPStrRecvBitRate,         &fReceiverBitRate,      sizeof(fReceiverBitRate));
    this->SetVal(qtssRTPStrAvgLateMilliseconds, &fAvgLateMsec,          sizeof(fAvgLateMsec));
    this->SetVal(qtssRTPStrPercentPacketsLost,  &fPercentPacketsLost,   sizeof(fPercentPacketsLost));
    this->SetVal(qtssRTPStrAvgBufDelayInMsec,   &fAvgBufDelayMsec,      sizeof(fAvgBufDelayMsec));
    this->SetVal(qtssRTPStrGettingBetter,       &fIsGettingBetter,      sizeof(fIsGettingBetter));
    this->SetVal(qtssRTPStrGettingWorse,        &fIsGettingWorse,       sizeof(fIsGettingWorse));
    this->SetVal(qtssRTPStrNumEyes,             &fNumEyes,              sizeof(fNumEyes));
    this->SetVal(qtssRTPStrNumEyesActive,       &fNumEyesActive,        sizeof(fNumEyesActive));
    this->SetVal(qtssRTPStrNumEyesPaused,       &fNumEyesPaused,        sizeof(fNumEyesPaused));
    this->SetVal(qtssRTPStrTotPacketsRecv,      &fTotalPacketsRecv,     sizeof(fTotalPacketsRecv));
    this->SetVal(qtssRTPStrTotPacketsDropped,   &fTotalPacketsDropped,  sizeof(fTotalPacketsDropped));
    this->SetVal(qtssRTPStrTotPacketsLost,      &fTotalPacketsLost,     sizeof(fTotalPacketsLost));
    this->SetVal(qtssRTPStrClientBufFill,       &fClientBufferFill,     sizeof(fClientBufferFill));
    this->SetVal(qtssRTPStrFrameRate,           &fFrameRate,            sizeof(fFrameRate));
    this->SetVal(qtssRTPStrExpFrameRate,        &fExpectedFrameRate,    sizeof(fExpectedFrameRate));
    this->SetVal(qtssRTPStrAudioDryCount,       &fAudioDryCount,        sizeof(fAudioDryCount));
    this->SetVal(qtssRTPStrIsTCP,               &fIsTCP,                sizeof(fIsTCP));
    this->SetVal(qtssRTPStrStreamRef,           &fStreamRef,            sizeof(fStreamRef));
    this->SetVal(qtssRTPStrTransportType,       &fTransportType,        sizeof(fTransportType));
    this->SetVal(qtssRTPStrStalePacketsDropped, &fStalePacketsDropped,  sizeof(fStalePacketsDropped));
    this->SetVal(qtssRTPStrCurrentAckTimeout,   &fCurrentAckTimeout,    sizeof(fCurrentAckTimeout));
    this->SetVal(qtssRTPStrCurPacketsLostInRTCPInterval ,       &fCurPacketsLostInRTCPInterval ,        sizeof(fPacketCountInRTCPInterval));
    this->SetVal(qtssRTPStrPacketCountInRTCPInterval,       &fPacketCountInRTCPInterval,        sizeof(fPacketCountInRTCPInterval));
    this->SetVal(qtssRTPStrSvrRTPPort,          &fLocalRTPPort,         sizeof(fLocalRTPPort));
    this->SetVal(qtssRTPStrClientRTPPort,       &fRemoteRTPPort,        sizeof(fRemoteRTPPort));
    this->SetVal(qtssRTPStrNetworkMode,         &fNetworkMode,          sizeof(fNetworkMode));
    
    
}

RTPStream::~RTPStream()
{
    QTSS_Error err = QTSS_NoErr;
    if (fSockets != NULL)
    {
        // If there is an UDP socket pair associated with this stream, make sure to free it up
        Assert(fSockets->GetSocketB()->GetDemuxer() != NULL);
        fSockets->GetSocketB()->GetDemuxer()->
            UnregisterTask(fRemoteAddr, fRemoteRTCPPort, this);
        Assert(err == QTSS_NoErr);
    
        QTSServerInterface::GetServer()->GetSocketPool()->ReleaseUDPSocketPair(fSockets);
    }
    
#if RTP_PACKET_RESENDER_DEBUGGING
    //fResender.LogClose(fFlowControlDurationMsec);
    //qtss_printf("Flow control duration msec: %I64d. Max outstanding packets: %d\n", fFlowControlDurationMsec, fResender.GetMaxPacketsInList());
#endif

#if RTP_TCP_STREAM_DEBUG
    if ( fIsTCP )
        qtss_printf( "DEBUG: ~RTPStream %li sends got EAGAIN'd.\n", (long)fNumPacketsDroppedOnTCPFlowControl );
#endif
}

SInt32 RTPStream::GetQualityLevel()
{
    if (fTransportType == qtssRTPTransportTypeUDP)
        return fQualityLevel;
    else
        return fSession->GetQualityLevel();
}

void RTPStream::SetQualityLevel(SInt32 level)
{
    if (level <  0 ) // invalid level
    {  Assert(0);
       level = 0;
    }
    if (QTSServerInterface::GetServer()->GetPrefs()->DisableThinning())
        level = 0;
        
    if (fTransportType == qtssRTPTransportTypeUDP)
        fQualityLevel = level;
    else
        fSession->SetQualityLevel(level);
}

void  RTPStream::SetOverBufferState(RTSPRequestInterface* request)
{
    SInt32 requestedOverBufferState = request->GetDynamicRateState();
    Bool16 enableOverBuffer = false;
    
    switch (fTransportType)
    {
        case qtssRTPTransportTypeReliableUDP:
        {
            enableOverBuffer = true; // default is on
            if (requestedOverBufferState == 0) // client specifically set to false
                enableOverBuffer = false;
        }
        break;
        
        case qtssRTPTransportTypeUDP:
        {   
            enableOverBuffer = false; // always off
        }
        break;
        
        
        case qtssRTPTransportTypeTCP:
        {
            
             enableOverBuffer = true; // default is on same as 4.0 and earlier. Allows tcp to compensate for falling behind from congestion or slow-start. 
            if (requestedOverBufferState == 0) // client specifically set to false
                enableOverBuffer = false;
        }
        break;
        
    }
    
    //over buffering is enabled for the session by default
    //if any stream turns it off then it is off for all streams
    //a disable is from either the stream type default or a specific rtsp command to disable
    if (!enableOverBuffer)
        fSession->GetOverbufferWindow()->TurnOffOverbuffering();
}

QTSS_Error RTPStream::Setup(RTSPRequestInterface* request, QTSS_AddStreamFlags inFlags)
{
    //Get the URL for this track
    fStreamURLPtr.Len = kMaxStreamURLSizeInBytes;
    if (request->GetValue(qtssRTSPReqFileName, 0, fStreamURLPtr.Ptr, &fStreamURLPtr.Len) != QTSS_NoErr)
        return QTSSModuleUtils::SendErrorResponse(request, qtssClientBadRequest, qtssMsgFileNameTooLong);
    fStreamURL[fStreamURLPtr.Len] = '\0';//just in case someone wants to use string routines
    
    //
    // Store the late-tolerance value that came out of hte x-RTP-Options header,
    // so that when it comes time to determine our thinning params (when we PLAY),
    // we will know this
    fLateToleranceInSec = request->GetLateToleranceInSec();
    if (fLateToleranceInSec == -1.0)
        fLateToleranceInSec = 1.5;

    //
    // Setup the transport type
    fTransportType = request->GetTransportType();
    fNetworkMode = request->GetNetworkMode();
    //
    // Only allow reliable UDP if it is enabled
    if ((fTransportType == qtssRTPTransportTypeReliableUDP) && (!QTSServerInterface::GetServer()->GetPrefs()->IsReliableUDPEnabled()))
        fTransportType = qtssRTPTransportTypeUDP;

        //
        // Check to see if we are inside a valid reliable UDP directory
        if ((fTransportType == qtssRTPTransportTypeReliableUDP) && (!QTSServerInterface::GetServer()->GetPrefs()->IsPathInsideReliableUDPDir(request->GetValue(qtssRTSPReqFilePath))))
            fTransportType = qtssRTPTransportTypeUDP;

    //
    // Check to see if caller is forcing raw UDP transport
    if ((fTransportType == qtssRTPTransportTypeReliableUDP) && (inFlags & qtssASFlagsForceUDPTransport))
        fTransportType = qtssRTPTransportTypeUDP;
        
	//
	// decide whether to overbuffer
	this->SetOverBufferState(request);
        
    // Check to see if this RTP stream should be sent over TCP.
    if (fTransportType == qtssRTPTransportTypeTCP)
    {
        fIsTCP = true;
        fSession->GetOverbufferWindow()->SetWindowSize(kUInt32_Max);
        
        // If it is, get 2 channel numbers from the RTSP session.
        fRTPChannel = request->GetSession()->GetTwoChannelNumbers(fSession->GetValue(qtssCliSesRTSPSessionID));
        fRTCPChannel = fRTPChannel+1;
        
        // If we are interleaving, this is all we need to do to setup.
        return QTSS_NoErr;
    }
    
    //
    // This track is not interleaved, so let the session know that all
    // tracks are not interleaved. This affects our scheduling of packets
    fSession->SetAllTracksInterleaved(false);
    
    //Get and store the remote addresses provided by the client. The remote addr is the
    //same as the RTSP client's IP address, unless an alternate was specified in the
    //transport header.
    fRemoteAddr = request->GetSession()->GetSocket()->GetRemoteAddr();
    if (request->GetDestAddr() != INADDR_ANY)
    {
        // Sending data to other addresses could be used in malicious ways, therefore
        // it is up to the module as to whether this sort of request might be allowed
        if (!(inFlags & qtssASFlagsAllowDestination))
            return QTSSModuleUtils::SendErrorResponse(request, qtssClientBadRequest, qtssMsgAltDestNotAllowed);
        fRemoteAddr = request->GetDestAddr();
    }
    fRemoteRTPPort = request->GetClientPortA();
    fRemoteRTCPPort = request->GetClientPortB();

    if ((fRemoteRTPPort == 0) || (fRemoteRTCPPort == 0))
        return QTSSModuleUtils::SendErrorResponse(request, qtssClientBadRequest, qtssMsgNoClientPortInTransport);       
    
    //make sure that the client is advertising an even-numbered RTP port,
    //and that the RTCP port is actually one greater than the RTP port
    if ((fRemoteRTPPort & 1) != 0)
        return QTSSModuleUtils::SendErrorResponse(request, qtssClientBadRequest, qtssMsgRTPPortMustBeEven);     
 
 // comment out check below. This allows the rtcp port to be non-contiguous with the rtp port.
 //   if (fRemoteRTCPPort != (fRemoteRTPPort + 1))
 //       return QTSSModuleUtils::SendErrorResponse(request, qtssClientBadRequest, qtssMsgRTCPPortMustBeOneBigger);       
    
    // Find the right source address for this stream. If it isn't specified in the
    // RTSP request, assume it is the same interface as for the RTSP request.
    UInt32 sourceAddr = request->GetSession()->GetSocket()->GetLocalAddr();
    if ((request->GetSourceAddr() != INADDR_ANY) && (SocketUtils::IsLocalIPAddr(request->GetSourceAddr())))
        sourceAddr = request->GetSourceAddr();

    // if the transport is TCP or RUDP, then we only want one session quality level instead of a per stream one
    if (fTransportType != qtssRTPTransportTypeUDP)
    {
//        this->SetVal(qtssRTPStrQualityLevel, fSession->GetQualityLevelPtr(), sizeof(fQualityLevel));
        this->SetQualityLevel(*(fSession->GetQualityLevelPtr()));
    }
    
    
    // If the destination address is multicast, we need to setup multicast socket options
    // on the sockets. Because these options may be different for each stream, we need
    // a dedicated set of sockets
    if (SocketUtils::IsMulticastIPAddr(fRemoteAddr))
    {
        fSockets = QTSServerInterface::GetServer()->GetSocketPool()->CreateUDPSocketPair(sourceAddr, 0);
        
        if (fSockets != NULL)
        {
            //Set options on both sockets. Not really sure why we need to specify an
            //outgoing interface, because these sockets are already bound to an interface!
            QTSS_Error err = fSockets->GetSocketA()->SetTtl(request->GetTtl());
            if (err == QTSS_NoErr)
                err = fSockets->GetSocketB()->SetTtl(request->GetTtl());
            if (err == QTSS_NoErr)
                err = fSockets->GetSocketA()->SetMulticastInterface(fSockets->GetSocketA()->GetLocalAddr());
            if (err == QTSS_NoErr)
                err = fSockets->GetSocketB()->SetMulticastInterface(fSockets->GetSocketB()->GetLocalAddr());
            if (err != QTSS_NoErr)
                return QTSSModuleUtils::SendErrorResponse(request, qtssServerInternal, qtssMsgCantSetupMulticast);      
        }
    }
    else
        fSockets = QTSServerInterface::GetServer()->GetSocketPool()->GetUDPSocketPair(sourceAddr, 0, fRemoteAddr, 
                                                                                        fRemoteRTCPPort);

    if (fSockets == NULL)
        return QTSSModuleUtils::SendErrorResponse(request, qtssServerInternal, qtssMsgOutOfPorts);      
    
    else if (fTransportType == qtssRTPTransportTypeReliableUDP)
    {
        //
        // FIXME - we probably want to get rid of this slow start flag in the API
        Bool16 useSlowStart = !(inFlags & qtssASFlagsDontUseSlowStart);
        if (!QTSServerInterface::GetServer()->GetPrefs()->IsSlowStartEnabled())
            useSlowStart = false;
        
        fTracker = fSession->GetBandwidthTracker();
            
        fResender.SetBandwidthTracker( fTracker );
        fResender.SetDestination( fSockets->GetSocketA(), fRemoteAddr, fRemoteRTPPort );

#if RTP_PACKET_RESENDER_DEBUGGING
        if (QTSServerInterface::GetServer()->GetPrefs()->IsAckLoggingEnabled())
        {
            char        url[256];
            char        logfile[256];
            qtss_sprintf(logfile, "resend_log_%lu", fSession->GetRTSPSession()->GetSessionID());
            StrPtrLen   logName(logfile);
            fResender.SetLog(&logName);
        
            StrPtrLen   *presoURL = fSession->GetValue(qtssCliSesPresentationURL);
            UInt32      clientAddr = request->GetSession()->GetSocket()->GetRemoteAddr();
            memcpy( url, presoURL->Ptr, presoURL->Len );
            url[presoURL->Len] = 0;
            qtss_printf( "RTPStream::Setup for %s will use ACKS, ip addr: %li.%li.%li.%li\n", url, (clientAddr & 0xff000000) >> 24
                                                                                                 , (clientAddr & 0x00ff0000) >> 16
                                                                                                 , (clientAddr & 0x0000ff00) >> 8
                                                                                                 , (clientAddr & 0x000000ff)
                                                                                                  );
        }
#endif
    }
    
    //
    // Record the Server RTP port
    fLocalRTPPort = fSockets->GetSocketA()->GetLocalPort();

    //finally, register with the demuxer to get RTCP packets from the proper address
    Assert(fSockets->GetSocketB()->GetDemuxer() != NULL);
    QTSS_Error err = fSockets->GetSocketB()->GetDemuxer()->RegisterTask(fRemoteAddr, fRemoteRTCPPort, this);
    //errors should only be returned if there is a routing problem, there should be none
    Assert(err == QTSS_NoErr);
    return QTSS_NoErr;
}

void RTPStream::SendSetupResponse( RTSPRequestInterface* inRequest )
{
    if (fSession->DoSessionSetupResponse(inRequest) != QTSS_NoErr)
        return;
        
    inRequest->AppendDateAndExpires();
    this->AppendTransport(inRequest);
    
    //
    // Append the x-RTP-Options header if there was a late-tolerance field
    if (inRequest->GetLateToleranceStr()->Len > 0)
        inRequest->AppendHeader(qtssXTransportOptionsHeader, inRequest->GetLateToleranceStr());
    
    //
    // Append the retransmit header if the client sent it
    StrPtrLen* theRetrHdr = inRequest->GetHeaderDictionary()->GetValue(qtssXRetransmitHeader);
    if ((theRetrHdr->Len > 0) && (fTransportType == qtssRTPTransportTypeReliableUDP))
        inRequest->AppendHeader(qtssXRetransmitHeader, theRetrHdr);

	// Append the dynamic rate header if the client sent it
	SInt32 theRequestedRate =inRequest->GetDynamicRateState();
	static StrPtrLen sHeaderOn("1",1);
	static StrPtrLen sHeaderOff("0",1);
	if (theRequestedRate > 0)	// the client sent the header and wants a dynamic rate
	{	
		if(*(fSession->GetOverbufferWindow()->OverbufferingEnabledPtr()))
			inRequest->AppendHeader(qtssXDynamicRateHeader, &sHeaderOn); // send 1 if overbuffering is turned on
		else
			inRequest->AppendHeader(qtssXDynamicRateHeader, &sHeaderOff); // send 0 if overbuffering is turned off
	}
    else if (theRequestedRate == 0) // the client sent the header but doesn't want a dynamic rate
        inRequest->AppendHeader(qtssXDynamicRateHeader, &sHeaderOff);        
    //else the client didn't send a header so do nothing 
	        
    inRequest->SendHeader();
}

void RTPStream::AppendTransport(RTSPRequestInterface* request)
{

    StrPtrLen* ssrcPtr = NULL;
    if (fEnableSSRC)
        ssrcPtr = &fSsrcStringPtr;

    // We are either going to append the RTP / RTCP port numbers (UDP),
    // or the channel numbers (TCP, interleaved)
    if (!fIsTCP)
    {
        //
        // With UDP retransmits its important the client starts sending RTCPs
        // to the right address right away. The sure-firest way to get the client
        // to do this is to put the src address in the transport. So now we do that always.
        //
        char srcIPAddrBuf[20];
        StrPtrLen theSrcIPAddress(srcIPAddrBuf, 20);
        QTSServerInterface::GetServer()->GetPrefs()->GetTransportSrcAddr(&theSrcIPAddress);
        if (theSrcIPAddress.Len == 0)       
            theSrcIPAddress = *fSockets->GetSocketA()->GetLocalAddrStr();


        if(request->IsPushRequest())
        {
            char rtpPortStr[10];
            char rtcpPortStr[10];
            qtss_sprintf(rtpPortStr, "%u", request->GetSetUpServerPort());     
            qtss_sprintf(rtcpPortStr, "%u", request->GetSetUpServerPort()+1);
            //qtss_printf(" RTPStream::AppendTransport rtpPort=%u rtcpPort=%u \n",request->GetSetUpServerPort(),request->GetSetUpServerPort()+1);
            StrPtrLen rtpSPL(rtpPortStr);
            StrPtrLen rtcpSPL(rtcpPortStr);
            // Append UDP socket port numbers.
            request->AppendTransportHeader(&rtpSPL, &rtcpSPL, NULL, NULL, &theSrcIPAddress,ssrcPtr);
        }       
        else
        {
            // Append UDP socket port numbers.
            UDPSocket* theRTPSocket = fSockets->GetSocketA();
            UDPSocket* theRTCPSocket = fSockets->GetSocketB();
            request->AppendTransportHeader(theRTPSocket->GetLocalPortStr(), theRTCPSocket->GetLocalPortStr(), NULL, NULL, &theSrcIPAddress,ssrcPtr);
        }
    }
    else if (fRTCPChannel < kNumPrebuiltChNums)
        // We keep a certain number of channel number strings prebuilt, so most of the time
        // we won't have to call qtss_sprintf
        request->AppendTransportHeader(NULL, NULL, &sChannelNums[fRTPChannel],  &sChannelNums[fRTCPChannel],NULL,ssrcPtr);
    else
    {
        // If these channel numbers fall outside prebuilt range, we will have to call qtss_sprintf.
        char rtpChannelBuf[10];
        char rtcpChannelBuf[10];
        qtss_sprintf(rtpChannelBuf, "%d", fRTPChannel);        
        qtss_sprintf(rtcpChannelBuf, "%d", fRTCPChannel);
        
        StrPtrLen rtpChannel(rtpChannelBuf);
        StrPtrLen rtcpChannel(rtcpChannelBuf);

        request->AppendTransportHeader(NULL, NULL, &rtpChannel, &rtcpChannel,NULL,ssrcPtr);
    }
}

void    RTPStream::AppendRTPInfo(QTSS_RTSPHeader inHeader, RTSPRequestInterface* request, UInt32 inFlags, Bool16 lastInfo)
{
    //format strings for the various numbers we need to send back to the client
    char rtpTimeBuf[20];
    StrPtrLen rtpTimeBufPtr;
    if (inFlags & qtssPlayRespWriteTrackInfo)
    {
        qtss_sprintf(rtpTimeBuf, "%lu", fFirstTimeStamp);
        rtpTimeBufPtr.Set(rtpTimeBuf, ::strlen(rtpTimeBuf));
        Assert(rtpTimeBufPtr.Len < 20);
    }   
    
    char seqNumberBuf[20];
    StrPtrLen seqNumberBufPtr;
    if (inFlags & qtssPlayRespWriteTrackInfo)
    {
        qtss_sprintf(seqNumberBuf, "%u", fFirstSeqNumber);
        seqNumberBufPtr.Set(seqNumberBuf, ::strlen(seqNumberBuf));
        Assert(seqNumberBufPtr.Len < 20);
    }

    StrPtrLen *nullSSRCPtr = NULL; // There is no SSRC in RTP-Info header, it goes in the transport header.
    request->AppendRTPInfoHeader(inHeader, &fStreamURLPtr, &seqNumberBufPtr, nullSSRCPtr, &rtpTimeBufPtr,lastInfo);

}

/*********************************
/
/   InterleavedWrite
/
/   Write the given RTP packet out on the RTSP channel in interleaved format.
/   update quality levels and statistics
/   on success refresh the RTP session timeout to keep it alive
/
*/

//ReliableRTPWrite must be called from a fSession mutex protected caller
QTSS_Error  RTPStream::InterleavedWrite(void* inBuffer, UInt32 inLen, UInt32* outLenWritten, unsigned char channel)
{
    
    if (fSession->GetRTSPSession() == NULL) // RTSPSession required for interleaved write
    {
        return EAGAIN;
    }

    //char blahblah[2048];
    
    QTSS_Error err = fSession->GetRTSPSession()->InterleavedWrite( inBuffer, inLen, outLenWritten, channel);
    //QTSS_Error err = fSession->GetRTSPSession()->InterleavedWrite( blahblah, 2044, outLenWritten, channel);
#if DEBUG
    //if (outLenWritten != NULL)
    //{
    //  Assert((*outLenWritten == 0) || (*outLenWritten == 2044));
    //}
#endif
    

#if DEBUG
    if ( err == EAGAIN )
    {
        fNumPacketsDroppedOnTCPFlowControl++;
    }
#endif      

    // reset the timeouts when the connection is still alive
    // wehn transmitting over HTTP, we're not going to get
    // RTCPs that would normally Refresh the session time.
    if ( err == QTSS_NoErr )
        fSession->RefreshTimeout(); // RTSP session gets refreshed internally in WriteV

    #if RTP_TCP_STREAM_DEBUG
    //qtss_printf( "DEBUG: RTPStream fCurrentPacketDelay %li, fQualityLevel %i\n", (long)fCurrentPacketDelay, (int)fQualityLevel );
    #endif

    return err;
}

//SendRetransmits must be called from a fSession mutex protected caller
void    RTPStream::SendRetransmits()
{

    if ( fTransportType == qtssRTPTransportTypeReliableUDP )
        fResender.ResendDueEntries(); 
        
    
}

//ReliableRTPWrite must be called from a fSession mutex protected caller
QTSS_Error RTPStream::ReliableRTPWrite(void* inBuffer, UInt32 inLen, const SInt64& curPacketDelay)
{
    QTSS_Error err = QTSS_NoErr;

    // this must ALSO be called in response to a packet timeout
    // event that can be resecheduled as necessary by the fResender
    // for -hacking- purposes we'l do it just as we write packets,
    // but we won't be able to play low bit-rate movies ( like MIDI )
    // until this is a schedulable task
    
    // Send retransmits for all streams on this session
    RTPStream** retransStream = NULL;
    UInt32 retransStreamLen = 0;

    //
    // Send retransmits if we need to
    for (int streamIter = 0; fSession->GetValuePtr(qtssCliSesStreamObjects, streamIter, (void**)&retransStream, &retransStreamLen) == QTSS_NoErr; streamIter++)
    {
        //qtss_printf("Resending packets for stream: %d\n",(*retransStream)->fTrackID);
        //qtss_printf("RTPStream::ReliableRTPWrite. Calling ResendDueEntries\n");
        if (retransStream != NULL && *retransStream != NULL)
            (*retransStream)->fResender.ResendDueEntries();
    }
    
    if ( !fSawFirstPacket )
    {
        fSawFirstPacket = true;
        fStreamCumDuration = 0;
        fStreamCumDuration = OS::Milliseconds() - fSession->GetPlayTime();
        //fInfoDisplayTimer.ResetToDuration( 1000 - fStreamCumDuration % 1000 );
    }
    
#if RTP_PACKET_RESENDER_DEBUGGING
    fResender.SetDebugInfo(fTrackID, fRemoteRTCPPort, curPacketDelay);
    fBytesSentThisInterval = fResender.SpillGuts(fBytesSentThisInterval);
#endif

    if ( fResender.IsFlowControlled() )
    {   
//      qtss_printf("Flow controlled\n");
#if DEBUG
        if (fFlowControlStartedMsec == 0)
        {
            //qtss_printf("Flow control start\n");
            fFlowControlStartedMsec = OS::Milliseconds();
        }
#endif
        err = QTSS_WouldBlock;
    }
    else
    {
#if DEBUG   
        if (fFlowControlStartedMsec != 0)
        {
            fFlowControlDurationMsec += OS::Milliseconds() - fFlowControlStartedMsec;
            fFlowControlStartedMsec = 0;
        }
#endif
        
        //
        // Assign a lifetime to the packet using the current delay of the packet and
        // the time until this packet becomes stale.
        fBytesSentThisInterval += inLen;
        fResender.AddPacket( inBuffer, inLen, (SInt32) (fDropAllPacketsForThisStreamDelay - curPacketDelay) );

        (void)fSockets->GetSocketA()->SendTo(fRemoteAddr, fRemoteRTPPort, inBuffer, inLen);
    }


    return err;
}

void RTPStream::SetThinningParams()
{
    SInt32 toleranceAdjust = 1500 - (SInt32(fLateToleranceInSec * 1000));
    
    QTSServerPrefs* thePrefs = QTSServerInterface::GetServer()->GetPrefs();
    
    if (fPayloadType == qtssVideoPayloadType)
        fDropAllPacketsForThisStreamDelay = thePrefs->GetDropAllVideoPacketsTimeInMsec() - toleranceAdjust;
    else
        fDropAllPacketsForThisStreamDelay = thePrefs->GetDropAllPacketsTimeInMsec() - toleranceAdjust;

    fThinAllTheWayDelay = thePrefs->GetThinAllTheWayTimeInMsec() - toleranceAdjust;
    fAlwaysThinDelay = thePrefs->GetAlwaysThinTimeInMsec() - toleranceAdjust;
    fStartThinningDelay = thePrefs->GetStartThinningTimeInMsec() - toleranceAdjust;
    fStartThickingDelay = thePrefs->GetStartThickingTimeInMsec() - toleranceAdjust;
    fThickAllTheWayDelay = thePrefs->GetThickAllTheWayTimeInMsec();
    fQualityCheckInterval = thePrefs->GetQualityCheckIntervalInMsec();
    fSession->fLastQualityCheckTime = 0;
	fSession->fLastQualityCheckMediaTime = 0;
	fSession->fStartedThinning = false;
}

Bool16 RTPStream::UpdateQualityLevel(const SInt64& inTransmitTime, const SInt64& inCurrentPacketDelay,
                                        const SInt64& inCurrentTime, UInt32 inPacketSize)
{
    Assert(fNumQualityLevels > 0);
    
    if (inTransmitTime <= fSession->GetPlayTime())
        return true;
    if (fTransportType == qtssRTPTransportTypeUDP)
        return true;
        
//  fQualityLevel = 2;
//  return true;
    
	if (fSession->fLastQualityCheckTime == 0)
	{
		// Reset the interval for checking quality levels
		fSession->fLastQualityCheckTime = inCurrentTime;
		fSession->fLastQualityCheckMediaTime = inTransmitTime;
		fLastCurrentPacketDelay = inCurrentPacketDelay;
		return true;
	}
	
	if (!fSession->fStartedThinning)
	{
		// if we're still behind but not falling further behind, then don't thin
		if ((inCurrentPacketDelay > fStartThinningDelay) && (inCurrentPacketDelay - fLastCurrentPacketDelay < 250))
		{
			if (inCurrentPacketDelay < fLastCurrentPacketDelay)
				fLastCurrentPacketDelay = inCurrentPacketDelay;
			return true;
		}
		else
		{	fSession->fStartedThinning = true;
		}
	}
	
	if ((fSession->fLastQualityCheckTime == 0) || (inCurrentPacketDelay > fThinAllTheWayDelay))
	{
		//
		// Reset the interval for checking quality levels
		fSession->fLastQualityCheckTime = inCurrentTime;
		fSession->fLastQualityCheckMediaTime = inTransmitTime;
		fLastCurrentPacketDelay = inCurrentPacketDelay;

		if (inCurrentPacketDelay > fThinAllTheWayDelay ) 
        {
            //
            // If we have fallen behind enough such that we risk trasmitting
            // stale packets to the client, AGGRESSIVELY thin the stream
            SetQualityLevel(fNumQualityLevels);
//          if (fPayloadType == qtssVideoPayloadType)
//              qtss_printf("Q=%d, delay = %qd\n", GetQualityLevel(), inCurrentPacketDelay);
            if (inCurrentPacketDelay > fDropAllPacketsForThisStreamDelay)
            {
                fStalePacketsDropped++;
                return false; // We should not send this packet
            }
        }
    }
    
    if (fNumQualityLevels <= 2)
    {
        if ((inCurrentPacketDelay < fStartThickingDelay) && (GetQualityLevel() > 0))
            SetQualityLevel(0);
            
        return true;        // not enough quality levels to do fine tuning
    }
    
	if (((inCurrentTime - fSession->fLastQualityCheckTime) > fQualityCheckInterval) || 
		((inTransmitTime - fSession->fLastQualityCheckMediaTime) > fQualityCheckInterval))
	{
        if ((inCurrentPacketDelay > fAlwaysThinDelay) && (GetQualityLevel() <  (SInt32) fNumQualityLevels))
            SetQualityLevel(GetQualityLevel() + 1);
        else if ((inCurrentPacketDelay > fStartThinningDelay) && (inCurrentPacketDelay > fLastCurrentPacketDelay))
        {
            if (!fWaitOnLevelAdjustment && (GetQualityLevel() < (SInt32) fNumQualityLevels))
            {
                SetQualityLevel(GetQualityLevel() + 1);
                fWaitOnLevelAdjustment = true;
            }
            else
                fWaitOnLevelAdjustment = false;
        }
        
        if ((inCurrentPacketDelay < fStartThickingDelay) && (GetQualityLevel() > 0) && (inCurrentPacketDelay < fLastCurrentPacketDelay))
        {
            SetQualityLevel(GetQualityLevel() - 1);
            fWaitOnLevelAdjustment = true;
        }
            
        if (inCurrentPacketDelay < fThickAllTheWayDelay)
        {
            SetQualityLevel(0);
            fWaitOnLevelAdjustment = false;
        }

//		if (fPayloadType == qtssVideoPayloadType)
//			printf("Q=%d, delay = %qd\n", GetQualityLevel(), inCurrentPacketDelay);
		fLastCurrentPacketDelay = inCurrentPacketDelay;
		fSession->fLastQualityCheckTime = inCurrentTime;
		fSession->fLastQualityCheckMediaTime = inTransmitTime;
    }

    return true; // We should send this packet
}

QTSS_Error  RTPStream::Write(void* inBuffer, UInt32 inLen, UInt32* outLenWritten, UInt32 inFlags)
{
    Assert(fSession != NULL);
    if (!fSession->GetSessionMutex()->TryLock())
        return EAGAIN;


    QTSS_Error err = QTSS_NoErr;
    SInt64 theTime = OS::Milliseconds();
    
    //
    // Data passed into this version of write must be a QTSS_PacketStruct
    QTSS_PacketStruct* thePacket = (QTSS_PacketStruct*)inBuffer;
    thePacket->suggestedWakeupTime = -1;
    SInt64 theCurrentPacketDelay = theTime - thePacket->packetTransmitTime;
    
#if RTP_PACKET_RESENDER_DEBUGGING
    UInt16* theSeqNum = (UInt16*)thePacket->packetData;
#endif

    //
    // Empty the overbuffer window
    fSession->GetOverbufferWindow()->EmptyOutWindow(theTime);

    //
    // Update the bit rate value
    fSession->UpdateCurrentBitRate(theTime);
    
    //
    // Is this the first write in a write burst?
    if (inFlags & qtssWriteFlagsWriteBurstBegin)
        fSession->GetOverbufferWindow()->MarkBeginningOfWriteBurst();
    
    if (inFlags & qtssWriteFlagsIsRTCP)
    {   
		//
		// Check to see if this packet is ready to send
		if (false == *(fSession->GetOverbufferWindow()->OverbufferingEnabledPtr())) // only force rtcps on time if overbuffering is off
		{
            thePacket->suggestedWakeupTime = fSession->GetOverbufferWindow()->CheckTransmitTime(thePacket->packetTransmitTime, theTime, inLen);
            if (thePacket->suggestedWakeupTime > theTime)
            {
                Assert(thePacket->suggestedWakeupTime >= fSession->GetOverbufferWindow()->GetSendInterval());			
                fSession->GetSessionMutex()->Unlock();// Make sure to unlock the mutex
                return QTSS_WouldBlock;
            }
        }

        if ( fTransportType == qtssRTPTransportTypeTCP )// write out in interleave format on the RTSP TCP channel
        {
            err = this->InterleavedWrite( thePacket->packetData, inLen, outLenWritten, fRTCPChannel );
        }
        else if ( inLen > 0 )
        {
            (void)fSockets->GetSocketB()->SendTo(fRemoteAddr, fRemoteRTCPPort, thePacket->packetData, inLen);
        }
        
        if (err == QTSS_NoErr)
            PrintPacketPrefEnabled( (char*) thePacket->packetData, inLen, (SInt32) RTPStream::rtcpSR);
    }
    else if (inFlags & qtssWriteFlagsIsRTP)
    {
        //
        // Check to see if this packet fits in the overbuffer window
        thePacket->suggestedWakeupTime = fSession->GetOverbufferWindow()->CheckTransmitTime(thePacket->packetTransmitTime, theTime, inLen);
        if (thePacket->suggestedWakeupTime > theTime)
        {
            Assert(thePacket->suggestedWakeupTime >= fSession->GetOverbufferWindow()->GetSendInterval());
#if RTP_PACKET_RESENDER_DEBUGGING
            fResender.logprintf("Overbuffer window full. Num bytes in overbuffer: %d. Wakeup time: %qd\n",fSession->GetOverbufferWindow()->AvailableSpaceInWindow(), thePacket->packetTransmitTime);
#endif
            //qtss_printf("Overbuffer window full. Returning: %qd\n", thePacket->suggestedWakeupTime - theTime);
            
            fSession->GetSessionMutex()->Unlock();// Make sure to unlock the mutex
            return QTSS_WouldBlock;
        }

        //
        // Check to make sure our quality level is correct. This function
        // also tells us whether this packet is just too old to send
        if (this->UpdateQualityLevel(thePacket->packetTransmitTime, theCurrentPacketDelay, theTime, inLen))
        {
            if ( fTransportType == qtssRTPTransportTypeTCP )    // write out in interleave format on the RTSP TCP channel
                err = this->InterleavedWrite( thePacket->packetData, inLen, outLenWritten, fRTPChannel );       
            else if ( fTransportType == qtssRTPTransportTypeReliableUDP )
                err = this->ReliableRTPWrite( thePacket->packetData, inLen, theCurrentPacketDelay );
            else if ( inLen > 0 )
                (void)fSockets->GetSocketA()->SendTo(fRemoteAddr, fRemoteRTPPort, thePacket->packetData, inLen);
            
            if (err == QTSS_NoErr)
                PrintPacketPrefEnabled( (char*) thePacket->packetData, inLen, (SInt32) RTPStream::rtp);
        
            if (err == 0)
            {
                static SInt64 time = -1;
                static int byteCount = 0;
                static SInt64 startTime = -1;
                static int totalBytes = 0;
                static int numPackets = 0;
                static SInt64 firstTime;
                
                if (theTime - time > 1000)
                {
                    if (time != -1)
                    {
//                      qtss_printf("   %qd KBit (%d in %qd secs)", byteCount * 8 * 1000 / (theTime - time) / 1024, totalBytes, (theTime - startTime) / 1000);
//                      if (fTracker)
//                          qtss_printf(" Window = %d\n", fTracker->CongestionWindow());
//                      else
//                          qtss_printf("\n");
//                      qtss_printf("Packet #%d xmit time = %qd\n", numPackets, (thePacket->packetTransmitTime - firstTime) / 1000);
                    }
                    else
                    {
                        startTime = theTime;
                        firstTime = thePacket->packetTransmitTime;
                    }
                    
                    byteCount = 0;
                    time = theTime;
                }

                byteCount += inLen;
                totalBytes += inLen;
                numPackets++;
                
//              UInt16* theSeqNumP = (UInt16*)thePacket->packetData;
//              UInt16 theSeqNum = ntohs(theSeqNumP[1]);
//				qtss_printf("Packet %d for time %qd sent at %qd (%d bytes)\n", theSeqNum, thePacket->packetTransmitTime - fSession->GetPlayTime(), theTime - fSession->GetPlayTime(), inLen);
            }
        }   
            
#if RTP_PACKET_RESENDER_DEBUGGING
        if (err != QTSS_NoErr)
            fResender.logprintf("Flow controlled: %qd Overbuffer window: %d. Cur time %qd\n", theCurrentPacketDelay, fSession->GetOverbufferWindow()->AvailableSpaceInWindow(), theTime);
        else
            fResender.logprintf("Sent packet: %d. Overbuffer window: %d Transmit time %qd. Cur time %qd\n", ntohs(theSeqNum[1]), fSession->GetOverbufferWindow()->AvailableSpaceInWindow(), thePacket->packetTransmitTime, theTime);
#endif
        //if (err != QTSS_NoErr)
        //  qtss_printf("flow controlled\n");
        if ( err == QTSS_NoErr && inLen > 0 )
        {
            // Update statistics if we were actually able to send the data (don't
            // update if the socket is flow controlled or some such thing)
            
            fSession->GetOverbufferWindow()->AddPacketToWindow(inLen);
            fSession->UpdatePacketsSent(1);
            fSession->UpdateBytesSent(inLen);
            QTSServerInterface::GetServer()->IncrementTotalRTPBytes(inLen);
            QTSServerInterface::GetServer()->IncrementTotalPackets();
            
            QTSServerInterface::GetServer()->IncrementTotalLate(theCurrentPacketDelay);
            QTSServerInterface::GetServer()->IncrementTotalQuality(this->GetQualityLevel());

            // Record the RTP timestamp for RTCPs
            UInt32* timeStampP = (UInt32*)(thePacket->packetData);
            fLastRTPTimestamp = ntohl(timeStampP[1]);
            
            //stream statistics
            fPacketCount++;
            fByteCount += inLen;

            // Send an RTCP sender report if it's time. Again, we only want to send an
            // RTCP if the RTP packet was sent sucessfully
            if ((fSession->GetPlayFlags() & qtssPlayFlagsSendRTCP) &&
                (theTime > (fLastSenderReportTime + (kSenderReportIntervalInSecs * 1000))))
            {
                fLastSenderReportTime = theTime;
                // CISCO comments
                // thePacket->packetTransmissionTime is
                // the expected transmission time, which
                // is what we should report in RTCP for
                // synchronization purposes, not theTime,
                // which is the actual transmission time.
                this->SendRTCPSR(thePacket->packetTransmitTime);
            }
            
        }
    }
    else
    {   fSession->GetSessionMutex()->Unlock();// Make sure to unlock the mutex
        return QTSS_BadArgument;//qtssWriteFlagsIsRTCP or qtssWriteFlagsIsRTP wasn't specified
    }
    
    if (outLenWritten != NULL)
        *outLenWritten = inLen;
        
    fSession->GetSessionMutex()->Unlock();// Make sure to unlock the mutex
    return err;
}



// SendRTCPSR is called by the session as well as the strem
// SendRTCPSR must be called from a fSession mutex protected caller
void RTPStream::SendRTCPSR(const SInt64& inTime, Bool16 inAppendBye)
{
        //
        // This will roll over, after which payloadByteCount will be all messed up.
        // But because it is a 32 bit number, that is bound to happen eventually,
        // and we are limited by the RTCP packet format in that respect, so this is
        // pretty much ok.
    UInt32 payloadByteCount = fByteCount - (12 * fPacketCount);
        
    RTCPSRPacket* theSR = fSession->GetSRPacket();
    theSR->SetSSRC(fSsrc);
    theSR->SetClientSSRC(fClientSSRC);
    theSR->SetNTPTimestamp(fSession->GetNTPPlayTime() +
                            OS::TimeMilli_To_Fixed64Secs(inTime - fSession->GetPlayTime()));
    theSR->SetRTPTimestamp(fLastRTPTimestamp);
    theSR->SetPacketCount(fPacketCount);
    theSR->SetByteCount(payloadByteCount);
#if RTP_PACKET_RESENDER_DEBUGGING
    fResender.logprintf("Recommending ack timeout of: %d\n",fSession->GetBandwidthTracker()->RecommendedClientAckTimeout());
#endif
    theSR->SetAckTimeout(fSession->GetBandwidthTracker()->RecommendedClientAckTimeout());
    
    UInt32 thePacketLen = theSR->GetSRPacketLen();
    if (inAppendBye)
        thePacketLen = theSR->GetSRWithByePacketLen();
        
    QTSS_Error err = QTSS_NoErr;
    if ( fTransportType == qtssRTPTransportTypeTCP )    // write out in interleave format on the RTSP TCP channel
    {
       UInt32  wasWritten; 
       err = this->InterleavedWrite( theSR->GetSRPacket(), thePacketLen, &wasWritten, fRTCPChannel );
    }
    else
    {
       err = fSockets->GetSocketB()->SendTo(fRemoteAddr, fRemoteRTCPPort, theSR->GetSRPacket(), thePacketLen);
    }
    
    if (err == QTSS_NoErr)
        PrintPacketPrefEnabled((char *) theSR->GetSRPacket(), thePacketLen, (SInt32) RTPStream::rtcpSR); // if we are flow controlled this packet is not sent
}


void RTPStream::ProcessIncomingInterleavedData(UInt8 inChannelNum, RTSPSessionInterface* inRTSPSession, StrPtrLen* inPacket)
{
    if (inChannelNum == fRTPChannel)
    {
        //
        // Currently we don't do anything with incoming RTP packets. Eventually,
        // we might need to make a role to deal with these
    }
    else if (inChannelNum == fRTCPChannel)
        this->ProcessIncomingRTCPPacket(inPacket);
}

void RTPStream::ProcessIncomingRTCPPacket(StrPtrLen* inPacket)
{
    StrPtrLen currentPtr(*inPacket);
    SInt64 curTime = OS::Milliseconds();


    // Modules are guarenteed atomic access to the session. Also, the RTSP Session accessed
    // below could go away at any time. So we need to lock the RTP session mutex.
    // *BUT*, when this function is called the caller already has the UDP socket pool &
    // UDP Demuxer mutexes. Blocking on grabbing this mutex could cause a deadlock.
    // So, dump this RTCP packet if we can't get the mutex.
    if (!fSession->GetSessionMutex()->TryLock())
        return;
    
    //no matter what happens (whether or not this is a valid packet) reset the timeouts
    fSession->RefreshTimeout();
    if (fSession->GetRTSPSession() != NULL)
        fSession->GetRTSPSession()->RefreshTimeout();
        
    while ( currentPtr.Len > 0 )
    {
        /*
            Due to the variable-type nature of RTCP packets, this is a bit unusual...
            We initially treat the packet as a generic RTCPPacket in order to determine its'
            actual packet type.  Once that is figgered out, we treat it as its' actual packet type
        */
        RTCPPacket rtcpPacket;
        if (!rtcpPacket.ParsePacket((UInt8*)currentPtr.Ptr, currentPtr.Len))
        {   fSession->GetSessionMutex()->Unlock();
            return;//abort if we discover a malformed RTCP packet
        }
        // Increment our RTCP Packet and byte counters for the session.
        
        fSession->IncrTotalRTCPPacketsRecv();
        fSession->IncrTotalRTCPBytesRecv( (SInt16) currentPtr.Len);

        switch (rtcpPacket.GetPacketType())
        {
            case RTCPPacket::kReceiverPacketType:
            {
                RTCPReceiverPacket receiverPacket;
                if (!receiverPacket.ParseReceiverReport((UInt8*)currentPtr.Ptr, currentPtr.Len))
                {   fSession->GetSessionMutex()->Unlock();
                    return;//abort if we discover a malformed receiver report
                }

                this->PrintPacketPrefEnabled(currentPtr.Ptr,  currentPtr.Len, RTPStream::rtcpRR);

                //
                // Set the Client SSRC based on latest RTCP
                fClientSSRC = rtcpPacket.GetPacketSSRC();

                fFractionLostPackets = receiverPacket.GetCumulativeFractionLostPackets();
                fJitter = receiverPacket.GetCumulativeJitter();
                
                UInt32 curTotalLostPackets = receiverPacket.GetCumulativeTotalLostPackets();
                
                // Workaround for client problem.  Sometimes it appears to report a bogus lost packet count.
                // Since we can't have lost more packets than we sent, ignore the packet if that seems to be the case.
                if (curTotalLostPackets - fTotalLostPackets <= fPacketCount - fLastPacketCount)
                {
                    // if current value is less than the old value, that means that the packets are out of order
                    //  just wait for another packet that arrives in the right order later and for now, do nothing
                    if (curTotalLostPackets > fTotalLostPackets)
                    {   
                        //increment the server total by the new delta
                        QTSServerInterface::GetServer()->IncrementTotalRTPPacketsLost(curTotalLostPackets - fTotalLostPackets);
                        fCurPacketsLostInRTCPInterval = curTotalLostPackets - fTotalLostPackets;
    //                  qtss_printf("fCurPacketsLostInRTCPInterval = %d\n", fCurPacketsLostInRTCPInterval);
                        fTotalLostPackets = curTotalLostPackets;
                    }
                    else if(curTotalLostPackets == fTotalLostPackets)
                    {
                        fCurPacketsLostInRTCPInterval = 0;
    //                  qtss_printf("fCurPacketsLostInRTCPInterval set to 0\n");
                    }
                    
                                    
                    fPacketCountInRTCPInterval = fPacketCount - fLastPacketCount;
                    fLastPacketCount = fPacketCount;
                }

#ifdef DEBUG_RTCP_PACKETS
                receiverPacket.Dump();
#endif
            }
            break;
            
            case RTCPPacket::kAPPPacketType:
            {   
                //
                // Check and see if this is an Ack packet. If it is, update the UDP Resender
                RTCPAckPacket theAckPacket;
                UInt8* packetBuffer = rtcpPacket.GetPacketBuffer();
                UInt32 packetLen = (rtcpPacket.GetPacketLength() * 4) + RTCPPacket::kRTCPHeaderSizeInBytes;
                
                
                if (theAckPacket.ParseAckPacket(packetBuffer, packetLen))
                {
                    if (NULL != fTracker && false == fTracker->ReadyForAckProcessing()) // this stream must be ready to receive acks.  Between RTSP setup and sending of first packet on stream we must protect against a bad ack.
                    {   fSession->GetSessionMutex()->Unlock();
                        return;//abort if we receive an ack when we haven't sent anything.
                    }
                        
                    this->PrintPacketPrefEnabled( (char*)packetBuffer,  packetLen, RTPStream::rtcpACK);
                    // Only check for ack packets if we are using Reliable UDP
                    if (fTransportType == qtssRTPTransportTypeReliableUDP)
                    {
                        UInt16 theSeqNum = theAckPacket.GetAckSeqNum();
                        fResender.AckPacket(theSeqNum, curTime);
                        //qtss_printf("Got ack: %d\n",theSeqNum);
                        
                        for (UInt16 maskCount = 0; maskCount < theAckPacket.GetAckMaskSizeInBits(); maskCount++)
                        {
                            if (theAckPacket.IsNthBitEnabled(maskCount))
                            {
                                fResender.AckPacket( theSeqNum + maskCount + 1, curTime);
                                //qtss_printf("Got ack in mask: %d\n",theSeqNum + maskCount + 1);
                            }
                        }
                        
                    }
                }
                else
                {   
                   this->PrintPacketPrefEnabled( (char*) packetBuffer, packetLen, RTPStream::rtcpAPP);
                   //
                    // If it isn't an ACK, assume its the qtss APP packet
                    RTCPCompressedQTSSPacket compressedQTSSPacket;
                    if (!compressedQTSSPacket.ParseCompressedQTSSPacket((UInt8*)currentPtr.Ptr, currentPtr.Len))
                    {   fSession->GetSessionMutex()->Unlock();
                        return;//abort if we discover a malformed app packet
                    }
                    
                    fReceiverBitRate =      compressedQTSSPacket.GetReceiverBitRate();
                    fAvgLateMsec =          compressedQTSSPacket.GetAverageLateMilliseconds();
                    
                    fPercentPacketsLost =   compressedQTSSPacket.GetPercentPacketsLost();
                    fAvgBufDelayMsec =      compressedQTSSPacket.GetAverageBufferDelayMilliseconds();
                    fIsGettingBetter = (UInt16)compressedQTSSPacket.GetIsGettingBetter();
                    fIsGettingWorse = (UInt16)compressedQTSSPacket.GetIsGettingWorse();
                    fNumEyes =              compressedQTSSPacket.GetNumEyes();
                    fNumEyesActive =        compressedQTSSPacket.GetNumEyesActive();
                    fNumEyesPaused =        compressedQTSSPacket.GetNumEyesPaused();
                    fTotalPacketsRecv =     compressedQTSSPacket.GetTotalPacketReceived();
                    fTotalPacketsDropped =  compressedQTSSPacket.GetTotalPacketsDropped();
                    fTotalPacketsLost =     compressedQTSSPacket.GetTotalPacketsLost();
                    fClientBufferFill =     compressedQTSSPacket.GetClientBufferFill();
                    fFrameRate =            compressedQTSSPacket.GetFrameRate();
                    fExpectedFrameRate =    compressedQTSSPacket.GetExpectedFrameRate();
                    fAudioDryCount =        compressedQTSSPacket.GetAudioDryCount();
                    
//                  if (fPercentPacketsLost == 0)
//                  {
//                      qtss_printf("***\n");
//                      fCurPacketsLostInRTCPInterval = 0;
//                  }
                    //
                    // Update our overbuffer window size to match what the client is telling us
                    if (fTransportType != qtssRTPTransportTypeUDP)
                    {
//                      qtss_printf("Setting over buffer to %d\n", compressedQTSSPacket.GetOverbufferWindowSize());
                        fSession->GetOverbufferWindow()->SetWindowSize(compressedQTSSPacket.GetOverbufferWindowSize());
                    }
                    
#ifdef DEBUG_RTCP_PACKETS
                compressedQTSSPacket.Dump();
#endif
                }
            }
            break;
            
            case RTCPPacket::kSDESPacketType:
            {
#ifdef DEBUG_RTCP_PACKETS
                SourceDescriptionPacket sedsPacket;
                if (!sedsPacket.ParsePacket((UInt8*)currentPtr.Ptr, currentPtr.Len))
                {   fSession->GetSessionMutex()->Unlock();
                    return;//abort if we discover a malformed app packet
                }

                sedsPacket.Dump();
#endif
            }
            break;
            
            //default:
            //  WarnV(false, "Unknown RTCP Packet Type");
            break;
        
        }
        
        
        currentPtr.Ptr += (rtcpPacket.GetPacketLength() * 4 ) + 4;
        currentPtr.Len -= (rtcpPacket.GetPacketLength() * 4 ) + 4;
    }

    // Invoke the RTCP modules, allowing them to process this packet
    QTSS_RoleParams theParams;
    theParams.rtcpProcessParams.inRTPStream = this;
    theParams.rtcpProcessParams.inClientSession = fSession;
    theParams.rtcpProcessParams.inRTCPPacketData = inPacket->Ptr;
    theParams.rtcpProcessParams.inRTCPPacketDataLen = inPacket->Len;
    
    // We don't allow async events from this role, so just set an empty module state.
    OSThreadDataSetter theSetter(&sRTCPProcessModuleState, NULL);
    
    // Invoke RTCP processing modules
    for (UInt32 x = 0; x < QTSServerInterface::GetNumModulesInRole(QTSSModule::kRTCPProcessRole); x++)
        (void)QTSServerInterface::GetModule(QTSSModule::kRTCPProcessRole, x)->CallDispatch(QTSS_RTCPProcess_Role, &theParams);

    fSession->GetSessionMutex()->Unlock();
}

char* RTPStream::GetStreamTypeStr()
{
    char *streamType = NULL;
     
    switch (fTransportType)
     {
        case qtssRTPTransportTypeUDP:   streamType = RTPStream::UDP;
        break;
        
        case qtssRTPTransportTypeReliableUDP: streamType = RTPStream::RUDP;
        break;
        
        case qtssRTPTransportTypeTCP: streamType = RTPStream::TCP;
        break;
        
        default: 
        streamType = RTPStream::noType;
     };
  
    return streamType;
}

void RTPStream::PrintRTP(char* packetBuff, UInt32 inLen)
{
  
    UInt16 sequence = ntohs( ((UInt16*)packetBuff)[1]);
    UInt32 timestamp = ntohl( ((UInt32*)packetBuff)[1]);
    UInt32 ssrc = ntohl( ((UInt32*)packetBuff)[2]);
    
     
       
    if (fFirstTimeStamp == 0)
        fFirstTimeStamp = timestamp;
        
    Float32 rtpTimeInSecs = 0.0; 
    if (fTimescale > 0 && fFirstTimeStamp < timestamp)
        rtpTimeInSecs = (Float32) (timestamp - fFirstTimeStamp) /  (Float32) fTimescale;
      
     
    StrPtrLen   *payloadStr = this->GetValue(qtssRTPStrPayloadName);
    if (payloadStr && payloadStr->Len > 0)
        payloadStr->PrintStr();
    else
        qtss_printf("?");

    
     qtss_printf(" H_ssrc=%ld H_seq=%u H_ts=%lu seq_count=%lu ts_secs=%.3f \n", ssrc, sequence, timestamp, fPacketCount +1, rtpTimeInSecs );

}


void RTPStream::PrintRTCPSenderReport(char* packetBuff, UInt32 inLen)
{

    char timebuffer[kTimeStrSize];    
     UInt32* theReport = (UInt32*)packetBuff;
    
    theReport++;
    UInt32 ssrc = htonl(*theReport);
    
    theReport++;
    SInt64 ntp = 0;
    ::memcpy(&ntp, theReport, sizeof(SInt64));
    ntp = OS::NetworkToHostSInt64(ntp);
    time_t theTime = OS::Time1900Fixed64Secs_To_UnixTimeSecs(ntp);
                    
    theReport += 2;
    UInt32 timestamp = ntohl(*theReport);
    Float32 theTimeInSecs = 0.0;

    if (fFirstTimeStamp == 0)
        fFirstTimeStamp = timestamp;

    if (fTimescale > 0 && fFirstTimeStamp < timestamp )
        theTimeInSecs = (Float32) (timestamp - fFirstTimeStamp) /  (Float32) fTimescale;
    
    theReport++;        
    UInt32 packetcount = ntohl(*theReport);
    
    theReport++;
    UInt32 bytecount = ntohl(*theReport);          
    
    StrPtrLen   *payloadStr = this->GetValue(qtssRTPStrPayloadName);
    if (payloadStr && payloadStr->Len > 0)
        payloadStr->PrintStr();
    else
        qtss_printf("?");

    qtss_printf(" H_ssrc=%lu H_bytes=%lu H_ts=%lu H_pckts=%lu ts_secs=%.3f H_ntp=%s", ssrc,bytecount, timestamp, packetcount, theTimeInSecs, ::qtss_ctime( &theTime,timebuffer,sizeof(timebuffer)) );
 }

void RTPStream::PrintPacket(char *inBuffer, UInt32 inLen, SInt32 inType)
{
    static char* rr="RR";
    static char* ack="ACK";
    static char* app="APP";
    static char* sTypeAudio=" type=audio";
    static char* sTypeVideo=" type=video";
    static char* sUnknownTypeStr = "?";
    char* theType = sUnknownTypeStr;
    
    if (fPayloadType == qtssVideoPayloadType)
        theType = sTypeVideo;
    else if (fPayloadType == qtssAudioPayloadType)
        theType = sTypeAudio;

    switch (inType)
    {
        case RTPStream::rtp:
           if (QTSServerInterface::GetServer()->GetPrefs()->PrintRTPHeaders())
           {
                qtss_printf("<send sess=%lu: RTP %s xmit_sec=%.3f %s size=%lu ", this->fSession->GetUniqueID(), this->GetStreamTypeStr(), this->GetStreamStartTimeSecs(), theType, inLen);
                PrintRTP(inBuffer, inLen);
           }
        break;
         
        case RTPStream::rtcpSR:
            if (QTSServerInterface::GetServer()->GetPrefs()->PrintSRHeaders())
            {
                qtss_printf("<send sess=%lu: SR %s xmit_sec=%.3f %s size=%lu ", this->fSession->GetUniqueID(), this->GetStreamTypeStr(), this->GetStreamStartTimeSecs(), theType, inLen);
                PrintRTCPSenderReport(inBuffer, inLen);
            }
        break;
        
        case RTPStream::rtcpRR:
           if (QTSServerInterface::GetServer()->GetPrefs()->PrintRRHeaders())
           {   
                RTCPReceiverPacket rtcpRR;
                if (rtcpRR.ParseReceiverReport( (UInt8*) inBuffer, inLen))
                {
                    qtss_printf(">recv sess=%lu: RTCP %s recv_sec=%.3f %s size=%lu ",this->fSession->GetUniqueID(), rr, this->GetStreamStartTimeSecs(), theType, inLen);
                    rtcpRR.Dump();
                }
           }
        break;
        
        case RTPStream::rtcpAPP:
            if (QTSServerInterface::GetServer()->GetPrefs()->PrintAPPHeaders())
            {   
                Bool16 debug = true;
                RTCPCompressedQTSSPacket compressedQTSSPacket(debug);
                if (compressedQTSSPacket.ParseCompressedQTSSPacket((UInt8*)inBuffer, inLen))
                {
                    qtss_printf(">recv sess=%lu: RTCP %s recv_sec=%.3f %s size=%lu ",this->fSession->GetUniqueID(), app, this->GetStreamStartTimeSecs(), theType, inLen);
                    compressedQTSSPacket.Dump();
                }
            }
        break;
        
        case RTPStream::rtcpACK:
            if (QTSServerInterface::GetServer()->GetPrefs()->PrintACKHeaders())
            {                
                RTCPAckPacket rtcpAck;
                if (rtcpAck.ParseAckPacket((UInt8*)inBuffer,inLen))
                {
                    qtss_printf(">recv sess=%lu: RTCP %s recv_sec=%.3f %s size=%lu ",this->fSession->GetUniqueID(), ack, this->GetStreamStartTimeSecs(), theType, inLen);
                    rtcpAck.Dump();
                }
            }
        break;
                
    }
}
