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
    File:       ClientSocket.cpp


这段代码是一个基于Objective-C的客户端套接字（ClientSocket）的实现，主要用于网络通信。它定义了几个类，包括ClientSocket、TCPClientSocket和HTTPClientSocket，用于处理TCP和HTTP协议的通信。下面是对代码的详细解释：

1. ClientSocket 类
ClientSocket 是一个基类，定义了客户端套接字的基本操作，如打开、连接、发送和接收数据等。

构造函数：初始化了一些成员变量，如fHostAddr、fHostPort、fEventMask、fSocketP、fSendBuffer和fSentLength。
Open 方法：尝试打开一个TCP套接字，并绑定到本地地址和端口。如果套接字已经绑定，则跳过这一步。
Connect 方法：尝试连接到指定的主机和端口。如果连接正在进行中，则设置事件掩码为读和写事件。
Send 方法：发送数据到套接字。数据被存储在发送缓冲区中，然后通过SendV方法发送。
SendSendBuffer 方法：从发送缓冲区发送数据。如果数据没有完全发送，则设置事件掩码为写事件。
2. TCPClientSocket 类
TCPClientSocket 继承自ClientSocket，专门用于处理TCP协议的通信。

构造函数：调用基类的构造函数，并立即打开套接字。
SetOptions 方法：设置套接字选项，如发送缓冲区大小、接收缓冲区大小等。
SendV 方法：将数据向量编码后发送。如果发送缓冲区为空，则将数据向量复制到发送缓冲区中，然后调用SendSendBuffer方法发送数据。
3. HTTPClientSocket 类
HTTPClientSocket 继承自ClientSocket，专门用于处理HTTP协议的通信。

构造函数：初始化一些成员变量，包括URL、Cookie、套接字类型等。同时分配内存用于存储URL。
析构函数：释放URL和POST套接字的内存。
Read 方法：读取数据。如果GET连接未建立，则发送GET请求。如果发送缓冲区中有数据，则发送这些数据。然后尝试读取数据，如果读取失败，则设置事件掩码为读事件。
SendV 方法：发送数据。如果POST连接未建立，则发送POST请求。如果发送缓冲区为空，则将数据向量编码后添加到发送缓冲区中，然后调用SendSendBuffer方法发送数据。
EncodeVec 方法：将数据向量编码为Base64格式，并添加到发送缓冲区中。
注意事项
代码中使用了条件编译指令#ifndef __Win32__，表明这部分代码主要在非Windows平台上运行。
使用了Assert宏进行断言，确保程序的正确性。
代码中包含了一些调试信息，可以通过定义CLIENT_SOCKET_DEBUG宏来启用或禁用调试输出。
这段代码主要用于网络编程中，处理TCP和HTTP协议的通信，适用于需要与服务器进行数据交换的应用程序。
    
*/
#ifndef __Win32__
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <sys/uio.h>
#endif


#include "ClientSocket.h"
#include "OSMemory.h"
#include "base64.h"
#include "MyAssert.h"

#define CLIENT_SOCKET_DEBUG 0


ClientSocket::ClientSocket()
:   fHostAddr(0),
    fHostPort(0),
    fEventMask(0),
    fSocketP(NULL),
    fSendBuffer(fSendBuf, 0),
    fSentLength(0)
{}

OS_Error ClientSocket::Open(TCPSocket* inSocket)
{
    OS_Error theErr = OS_NoErr;
    if (!inSocket->IsBound())
    {
        theErr = inSocket->Open();
        if (theErr == OS_NoErr)
            theErr = inSocket->Bind(0, 0);

        if (theErr != OS_NoErr)
            return theErr;
            
        inSocket->NoDelay();
#if __FreeBSD__ || __MacOSX__
    // no KeepAlive -- probably should be off for all platforms.
#else
        inSocket->KeepAlive();
#endif

    }
    return theErr;
}

OS_Error ClientSocket::Connect(TCPSocket* inSocket)
{
    OS_Error theErr = this->Open(inSocket);
    Assert(theErr == OS_NoErr);
    if (theErr != OS_NoErr)
        return theErr;

    if (!inSocket->IsConnected())
    {
        theErr = inSocket->Connect(fHostAddr, fHostPort);
        if ((theErr == EINPROGRESS) || (theErr == EAGAIN))
        {
            fSocketP = inSocket;
            fEventMask = EV_RE | EV_WR;
            return theErr;
        }
    }
    return theErr;
}

OS_Error ClientSocket::Send(char* inData, const UInt32 inLength)
{
    iovec theVec[1];
    theVec[0].iov_base = (char*)inData;
    theVec[0].iov_len = inLength;
    
    return this->SendV(theVec, 1);
}

OS_Error ClientSocket::SendSendBuffer(TCPSocket* inSocket)
{
    OS_Error theErr = OS_NoErr;
    UInt32 theLengthSent = 0;
    
    if (fSendBuffer.Len == 0)
        return OS_NoErr;
    
    do
    {
        // theLengthSent should be reset to zero before passing its pointer to Send function
        // otherwise the old value will be used and it will go into an infinite loop sometimes
        theLengthSent = 0;
        //
        // Loop, trying to send the entire message.
        theErr = inSocket->Send(fSendBuffer.Ptr + fSentLength, fSendBuffer.Len - fSentLength, &theLengthSent);
        fSentLength += theLengthSent;
        
    } while (theLengthSent > 0);
    
    if (theErr == OS_NoErr)
        fSendBuffer.Len = fSentLength = 0; // Message was sent
    else
    {
        // Message wasn't entirely sent. Caller should wait for a read event on the POST socket
        fSocketP = inSocket;
        fEventMask = EV_WR;
    }
    return theErr;
}


TCPClientSocket::TCPClientSocket(UInt32 inSocketType)
 : fSocket(NULL, inSocketType)
{
    //
    // It is necessary to open the socket right when we construct the
    // object because the QTSSSplitterModule that uses this class uses
    // the socket file descriptor in the QTSS_CreateStreamFromSocket call.
    fSocketP = &fSocket;
    this->Open(&fSocket);
}

void TCPClientSocket::SetOptions(int sndBufSize,int rcvBufSize)
{   //set options on the socket

    //qtss_printf("TCPClientSocket::SetOptions sndBufSize=%d,rcvBuf=%d,keepAlive=%d,noDelay=%d\n",sndBufSize,rcvBufSize,(int)keepAlive,(int)noDelay);
    int err = 0;
    err = ::setsockopt(fSocket.GetSocketFD(), SOL_SOCKET, SO_SNDBUF, (char*)&sndBufSize, sizeof(int));
    AssertV(err == 0, OSThread::GetErrno());

    err = ::setsockopt(fSocket.GetSocketFD(), SOL_SOCKET, SO_RCVBUF, (char*)&rcvBufSize, sizeof(int));
    AssertV(err == 0, OSThread::GetErrno());

#if __FreeBSD__ || __MacOSX__
    struct timeval time;
    //int len = sizeof(time);
    time.tv_sec = 0;
    time.tv_usec = 0;

    err = ::setsockopt(fSocket.GetSocketFD(), SOL_SOCKET, SO_RCVTIMEO, (char*)&time, sizeof(time));
    AssertV(err == 0, OSThread::GetErrno());

    err = ::setsockopt(fSocket.GetSocketFD(), SOL_SOCKET, SO_SNDTIMEO, (char*)&time, sizeof(time));
    AssertV(err == 0, OSThread::GetErrno());
#endif

}

OS_Error TCPClientSocket::SendV(iovec* inVec, UInt32 inNumVecs)
{
    if (fSendBuffer.Len == 0)
    {
        for (UInt32 count = 0; count < inNumVecs; count++)
        {
            ::memcpy(fSendBuffer.Ptr + fSendBuffer.Len, inVec[count].iov_base, inVec[count].iov_len);
            fSendBuffer.Len += inVec[count].iov_len;
            Assert(fSendBuffer.Len < ClientSocket::kSendBufferLen);
        }
    }
    
    OS_Error theErr = this->Connect(&fSocket);
    if (theErr != OS_NoErr)
        return theErr;
        
    return this->SendSendBuffer(&fSocket);
}
            
OS_Error TCPClientSocket::Read(void* inBuffer, const UInt32 inLength, UInt32* outRcvLen)
{
    this->Connect(&fSocket);
    OS_Error theErr = fSocket.Read(inBuffer, inLength, outRcvLen);
    if (theErr != OS_NoErr)
        fEventMask = EV_RE;
    return theErr;
}


HTTPClientSocket::HTTPClientSocket(const StrPtrLen& inURL, UInt32 inCookie, UInt32 inSocketType)
:   fCookie(inCookie),
    fSocketType(inSocketType),
    fGetReceived(0),
    
    fGetSocket(NULL, inSocketType),
    fPostSocket(NULL)
{
    fURL.Ptr = NEW char[inURL.Len + 1];
    fURL.Len = inURL.Len;
    ::memcpy(fURL.Ptr, inURL.Ptr, inURL.Len);
    fURL.Ptr[fURL.Len] = '\0';
}

HTTPClientSocket::~HTTPClientSocket()
{
    delete [] fURL.Ptr;
    delete fPostSocket;
}

OS_Error HTTPClientSocket::Read(void* inBuffer, const UInt32 inLength, UInt32* outRcvLen)
{
    //
    // Bring up the GET connection if we need to
    if (!fGetSocket.IsConnected())
    {
#if CLIENT_SOCKET_DEBUG
        qtss_printf("HTTPClientSocket::Read: Sending GET\n");
#endif
        qtss_sprintf(fSendBuffer.Ptr, "GET %s HTTP/1.0\r\nX-SessionCookie: %lu\r\nAccept: application/x-rtsp-rtp-interleaved\r\nUser-Agent: QTSS/2.0\r\n\r\n", fURL.Ptr, fCookie);
        fSendBuffer.Len = ::strlen(fSendBuffer.Ptr);
        Assert(fSentLength == 0);
    }

    OS_Error theErr = this->Connect(&fGetSocket);
    if (theErr != OS_NoErr)
        return theErr;
    
    if (fSendBuffer.Len > 0)
    {
        theErr = this->SendSendBuffer(&fGetSocket);
        if (theErr != OS_NoErr)
            return theErr;
        fSentLength = 1; // So we know to execute the receive code below.
    }
    
    // We are done sending the GET. If we need to receive the GET response, do that here
    if (fSentLength > 0)
    {
        *outRcvLen = 0;
        do
        {
            // Loop, trying to receive the entire response.
            theErr = fGetSocket.Read(&fSendBuffer.Ptr[fGetReceived], kSendBufferLen - fGetReceived, outRcvLen);
            fGetReceived += *outRcvLen;
            
            // Check to see if we've gotten a \r\n\r\n. If we have, then we've received
            // the entire GET
            fSendBuffer.Ptr[fGetReceived] = '\0';
            char* theGetEnd = ::strstr(fSendBuffer.Ptr, "\r\n\r\n");

            if (theGetEnd != NULL)
            {
                // We got the entire GET response, so we are ready to move onto
                // real RTSP response data. First skip past the \r\n\r\n
                theGetEnd += 4;

#if CLIENT_SOCKET_DEBUG
                qtss_printf("HTTPClientSocket::Read: Received GET response\n");
#endif
                
                // Whatever remains is part of an RTSP request, so move that to
                // the beginning of the buffer and blow away the GET
                *outRcvLen = fGetReceived - (theGetEnd - fSendBuffer.Ptr);
                ::memcpy(inBuffer, theGetEnd, *outRcvLen);
                fGetReceived = fSentLength = 0;
                return OS_NoErr;
            }
            
            Assert(fGetReceived < inLength);
        } while (*outRcvLen > 0);
        
#if CLIENT_SOCKET_DEBUG
        qtss_printf("HTTPClientSocket::Read: Waiting for GET response\n");
#endif
        // Message wasn't entirely received. Caller should wait for a read event on the GET socket
        Assert(theErr != OS_NoErr);
        fSocketP = &fGetSocket;
        fEventMask = EV_RE;
        return theErr;
    }
    
    theErr = fGetSocket.Read(&((char*)inBuffer)[fGetReceived], inLength - fGetReceived, outRcvLen);
    if (theErr != OS_NoErr)
    {
#if CLIENT_SOCKET_DEBUG
        //qtss_printf("HTTPClientSocket::Read: Waiting for data\n");
#endif
        fSocketP = &fGetSocket;
        fEventMask = EV_RE;
    }
#if CLIENT_SOCKET_DEBUG
    //else
        //qtss_printf("HTTPClientSocket::Read: Got some data\n");
#endif
    return theErr;
}

OS_Error HTTPClientSocket::SendV(iovec* inVec, UInt32 inNumVecs)
{
    //
    // Bring up the POST connection if we need to
    if (fPostSocket == NULL)
        fPostSocket = NEW TCPSocket(NULL, fSocketType);

    if (!fPostSocket->IsConnected())
    {
#if CLIENT_SOCKET_DEBUG
        qtss_printf("HTTPClientSocket::Send: Sending POST\n");
#endif
        qtss_sprintf(fSendBuffer.Ptr, "POST %s HTTP/1.0\r\nX-SessionCookie: %lu\r\nAccept: application/x-rtsp-rtp-interleaved\r\nUser-Agent: QTSS/2.0\r\n\r\n", fURL.Ptr, fCookie);
        fSendBuffer.Len = ::strlen(fSendBuffer.Ptr);
        this->EncodeVec(inVec, inNumVecs);
    }
    
    OS_Error theErr = this->Connect(fPostSocket);
    if (theErr != OS_NoErr)
        return theErr;

    //
    // If we have nothing to send currently, this should be a new message, in which case
    // we can encode it and send it
    if (fSendBuffer.Len == 0)
        this->EncodeVec(inVec, inNumVecs);
    
#if CLIENT_SOCKET_DEBUG
    //qtss_printf("HTTPClientSocket::Send: Sending data\n");
#endif
    return this->SendSendBuffer(fPostSocket);
}

void HTTPClientSocket::EncodeVec(iovec* inVec, UInt32 inNumVecs)
{
    for (UInt32 count = 0; count < inNumVecs; count++)
    {
        fSendBuffer.Len += ::Base64encode(fSendBuffer.Ptr + fSendBuffer.Len, (char*)inVec[count].iov_base, inVec[count].iov_len);
        Assert(fSendBuffer.Len < ClientSocket::kSendBufferLen);
        fSendBuffer.Len = ::strlen(fSendBuffer.Ptr); //Don't trust what the above function returns for a length
    }
}
