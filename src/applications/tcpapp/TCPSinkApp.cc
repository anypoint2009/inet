//
// Copyright 2004 Andras Varga
//
// This library is free software, you can redistribute it and/or modify
// it under  the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation;
// either version 2 of the License, or any later version.
// The library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//


#include "TCPSinkApp.h"

#include "AddressResolver.h"
#include "ModuleAccess.h"
#include "NodeStatus.h"
#include "TCPSocket.h"


Define_Module(TCPSinkApp);

simsignal_t TCPSinkApp::rcvdPkSignal = SIMSIGNAL_NULL;

int TCPSinkApp::numInitStages() const
{
    return INITSTAGE_APPLICATION_LAYER + 1;
}

void TCPSinkApp::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_LOCAL)
    {

        bytesRcvd = 0;
        WATCH(bytesRcvd);
        rcvdPkSignal = registerSignal("rcvdPk");
    }
    if (stage == INITSTAGE_APPLICATION_LAYER)
    {
        bool isOperational;
        NodeStatus *nodeStatus = dynamic_cast<NodeStatus *>(findContainingNode(this)->getSubmodule("status"));
        isOperational = (!nodeStatus) || nodeStatus->getState() == NodeStatus::UP;
        if (!isOperational)
            throw cRuntimeError("This module doesn't support starting in node DOWN state");

        // ASSERT(stage >= STAGE:TRANSPORT_LAYER_AVAILABLE);
        // ASSERT(stage >= STAGE:IP_ADDRESS_AVAILABLE);

        const char *localAddress = par("localAddress");
        int localPort = par("localPort");
        TCPSocket socket;
        socket.setOutputGate(gate("tcpOut"));
        socket.readDataTransferModePar(*this);
        socket.bind(localAddress[0] ? AddressResolver().resolve(localAddress) : Address(), localPort);
        socket.listen();
    }
}

void TCPSinkApp::handleMessage(cMessage *msg)
{
    if (msg->getKind() == TCP_I_PEER_CLOSED)
    {
        // we close too
        msg->setName("close");
        msg->setKind(TCP_C_CLOSE);
        send(msg, "tcpOut");
    }
    else if (msg->getKind() == TCP_I_DATA || msg->getKind() == TCP_I_URGENT_DATA)
    {
        cPacket *pk = PK(msg);
        long packetLength = pk->getByteLength();
        bytesRcvd += packetLength;
        emit(rcvdPkSignal, pk);
        delete msg;

        if (ev.isGUI())
        {
            char buf[32];
            sprintf(buf, "rcvd: %ld bytes", bytesRcvd);
            getDisplayString().setTagArg("t", 0, buf);
        }
    }
    else
    {
        // must be data or some kind of indication -- can be dropped
        delete msg;
    }
}

void TCPSinkApp::finish()
{
}

