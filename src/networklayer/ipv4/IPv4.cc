//
// Copyright (C) 2004 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//


#include <stdlib.h>
#include <string.h>

#include "IPv4.h"

#include "ARPPacket_m.h"
#include "ICMPMessage_m.h"
#include "InterfaceTableAccess.h"
#include "IPv4ControlInfo.h"
#include "IPv4Datagram.h"
#include "IPv4InterfaceData.h"
#include "IIPv4RoutingTable.h"
#include "Ieee802Ctrl_m.h"
#include "NodeOperations.h"
#include "NodeStatus.h"
#include "NotificationBoard.h"
#include "IPSocket.h"
#include "IARPCache.h"
#include "Ieee802Ctrl_m.h"

Define_Module(IPv4);

//TODO TRANSLATE
// a multicast cimek eseten hianyoznak bizonyos NetFilter hook-ok
// a local interface-k hasznalata eseten szinten hianyozhatnak bizonyos NetFilter hook-ok

void IPv4::initialize(int stage)
{
    if (stage == 0)
    {
        QueueBase::initialize();

        ift = InterfaceTableAccess().get();
        rt = check_and_cast<IIPv4RoutingTable *>(getModuleByPath(par("routingTableModule")));
        nb = NotificationBoardAccess().getIfExists(); // needed only for multicast forwarding
        arp = ARPCacheAccess().get();

        arpDgramOutGate = gate("arpDgramOut");
        arpInGate = gate("arpIn");
        arpOutGate = gate("arpOut");
        transportInGateBaseId = gateBaseId("transportIn");
        queueOutGateBaseId = gateBaseId("queueOut");

        defaultTimeToLive = par("timeToLive");
        defaultMCTimeToLive = par("multicastTimeToLive");
        fragmentTimeoutTime = par("fragmentTimeout");
        forceBroadcast = par("forceBroadcast");
        useProxyARP = par("useProxyARP");

        curFragmentId = 0;
        lastCheckTime = 0;
        fragbuf.init(icmpAccess.get());

        numMulticast = numLocalDeliver = numDropped = numUnroutable = numForwarded = 0;

        // NetFilter:
        hooks.clear();
        queuedDatagramsForHooks.clear();

        WATCH(numMulticast);
        WATCH(numLocalDeliver);
        WATCH(numDropped);
        WATCH(numUnroutable);
        WATCH(numForwarded);
    }
    else if (stage == 1)
    {
        isUp = isNodeUp();
    }
}

void IPv4::updateDisplayString()
{
    char buf[80] = "";
    if (numForwarded>0) sprintf(buf+strlen(buf), "fwd:%d ", numForwarded);
    if (numLocalDeliver>0) sprintf(buf+strlen(buf), "up:%d ", numLocalDeliver);
    if (numMulticast>0) sprintf(buf+strlen(buf), "mcast:%d ", numMulticast);
    if (numDropped>0) sprintf(buf+strlen(buf), "DROP:%d ", numDropped);
    if (numUnroutable>0) sprintf(buf+strlen(buf), "UNROUTABLE:%d ", numUnroutable);
    getDisplayString().setTagArg("t", 0, buf);
}

void IPv4::handleMessage(cMessage *msg)
{
    if (msg->getKind() == IP_C_REGISTER_PROTOCOL) {
        IPRegisterProtocolCommand * command = check_and_cast<IPRegisterProtocolCommand *>(msg->getControlInfo());
        mapping.addProtocolMapping(command->getProtocol(), msg->getArrivalGate()->getIndex());
        delete msg;
    }
    else if (!msg->isSelfMessage() && msg->getArrivalGate()->isName("arpIn"))
        endService(PK(msg));
    else
        QueueBase::handleMessage(msg);
}

void IPv4::endService(cPacket *packet)
{
    if (!isUp) {
        EV << "IPv4 is down -- discarding message\n";
        delete packet;
        return;
    }
    if (packet->getArrivalGate()->isName("transportIn")) //TODO packet->getArrivalGate()->getBaseId() == transportInGateBaseId
    {
        handlePacketFromHL(packet);
    }
    else if (packet->getArrivalGate() == arpInGate)
    {
        handlePacketFromARP(packet);
    }
    else // from network
    {
        const InterfaceEntry *fromIE = getSourceInterfaceFrom(packet);
        if (dynamic_cast<ARPPacket *>(packet))
            handleIncomingARPPacket((ARPPacket *)packet, fromIE);
        else if (dynamic_cast<IPv4Datagram *>(packet))
            handleIncomingDatagram((IPv4Datagram *)packet, fromIE);
        else
            throw cRuntimeError(packet, "Unexpected packet type");
    }

    if (ev.isGUI())
        updateDisplayString();
}

const InterfaceEntry *IPv4::getSourceInterfaceFrom(cPacket *packet)
{
    cGate *g = packet->getArrivalGate();
    return g ? ift->getInterfaceByNetworkLayerGateIndex(g->getIndex()) : NULL;
}

void IPv4::handleIncomingDatagram(IPv4Datagram *datagram, const InterfaceEntry *fromIE)
{
    ASSERT(datagram);
    ASSERT(fromIE);

    //
    // "Prerouting"
    //

    // check for header biterror
    if (datagram->hasBitError())
    {
        // probability of bit error in header = size of header / size of total message
        // (ignore bit error if in payload)
        double relativeHeaderLength = datagram->getHeaderLength() / (double)datagram->getByteLength();
        if (dblrand() <= relativeHeaderLength)
        {
            EV << "bit error found, sending ICMP_PARAMETER_PROBLEM\n";
            icmpAccess.get()->sendErrorMessage(datagram, ICMP_PARAMETER_PROBLEM, 0);
            return;
        }
    }

    EV << "Received datagram `" << datagram->getName() << "' with dest=" << datagram->getDestAddress() << "\n";

    const InterfaceEntry *destIE = NULL;
    Address nextHop(IPv4Address::UNSPECIFIED_ADDRESS);
    if (datagramPreRoutingHook(datagram, fromIE, destIE, nextHop) == INetfilter::IHook::ACCEPT)
        preroutingFinish(datagram, fromIE, destIE, nextHop.toIPv4());
}

void IPv4::preroutingFinish(IPv4Datagram *datagram, const InterfaceEntry *fromIE, const InterfaceEntry *destIE, IPv4Address nextHopAddr)
{
    IPv4Address &destAddr = datagram->getDestAddress();

    // remove control info
    delete datagram->removeControlInfo();

    // route packet

    if (fromIE->isLoopback())
    {
        reassembleAndDeliver(datagram);
    }
    else if (destAddr.isMulticast())
    {
        // check for local delivery
        // Note: multicast routers will receive IGMP datagrams even if their interface is not joined to the group
        if (fromIE->ipv4Data()->isMemberOfMulticastGroup(destAddr) ||
                (rt->isMulticastForwardingEnabled() && datagram->getTransportProtocol() == IP_PROT_IGMP))
            reassembleAndDeliver(datagram->dup());
        else
            EV << "Skip local delivery of multicast datagram (input interface not in multicast group)\n";

        // don't forward if IP forwarding is off, or if dest address is link-scope
        if (!rt->isForwardingEnabled() || destAddr.isLinkLocalMulticast())
        {
            EV << "Skip forwarding of multicast datagram (packet is link-local or forwarding disabled)\n";
            delete datagram;
        }
        else if (datagram->getTimeToLive() == 0)
        {
            EV << "Skip forwarding of multicast datagram (TTL reached 0)\n";
            delete datagram;
        }
        else
            forwardMulticastPacket(datagram, fromIE);
    }
    else
    {
        const InterfaceEntry *broadcastIE = NULL;

        // check for local delivery; we must accept also packets coming from the interfaces that
        // do not yet have an IP address assigned. This happens during DHCP requests.
        if (rt->isLocalAddress(destAddr) || fromIE->ipv4Data()->getIPAddress().isUnspecified())
        {
            reassembleAndDeliver(datagram);
        }
        else if (destAddr.isLimitedBroadcastAddress() || (broadcastIE=rt->findInterfaceByLocalBroadcastAddress(destAddr)))
        {
            // broadcast datagram on the target subnet if we are a router
            if (broadcastIE && fromIE != broadcastIE && rt->isForwardingEnabled())
                fragmentPostRouting(datagram->dup(), broadcastIE, IPv4Address::ALLONES_ADDRESS);

            EV << "Broadcast received\n";
            reassembleAndDeliver(datagram);
        }
        else if (!rt->isForwardingEnabled())
        {
            EV << "forwarding off, dropping packet\n";
            numDropped++;
            delete datagram;
        }
        else
            routeUnicastPacket(datagram, fromIE, destIE, nextHopAddr);
    }
}

void IPv4::handleIncomingARPPacket(ARPPacket *packet, const InterfaceEntry *fromIE)
{
    // give it to the ARP module
    Ieee802Ctrl *ctrl = check_and_cast<Ieee802Ctrl*>(packet->getControlInfo());
    ctrl->setInterfaceId(fromIE->getInterfaceId());
    send(packet, arpOutGate);
}

void IPv4::handleIncomingICMP(ICMPMessage *packet)
{
    switch (packet->getType())
    {
        case ICMP_REDIRECT: // TODO implement redirect handling
        case ICMP_DESTINATION_UNREACHABLE:
        case ICMP_TIME_EXCEEDED:
        case ICMP_PARAMETER_PROBLEM: {
            // ICMP errors are delivered to the appropriate higher layer protocol
            IPv4Datagram *bogusPacket = check_and_cast<IPv4Datagram *>(packet->getEncapsulatedPacket());
            int protocol = bogusPacket->getTransportProtocol();
            int gateindex = mapping.getOutputGateForProtocol(protocol);
            send(packet, "transportOut", gateindex);
            break;
        }
        default: {
            // all others are delivered to ICMP: ICMP_ECHO_REQUEST, ICMP_ECHO_REPLY,
            // ICMP_TIMESTAMP_REQUEST, ICMP_TIMESTAMP_REPLY, etc.
            int gateindex = mapping.getOutputGateForProtocol(IP_PROT_ICMP);
            send(packet, "transportOut", gateindex);
            break;
        }
    }
}

void IPv4::handlePacketFromHL(cPacket *packet)
{
    // if no interface exists, do not send datagram
    if (ift->getNumInterfaces() == 0)
    {
        EV << "No interfaces exist, dropping packet\n";
        numDropped++;
        delete packet;
        return;
    }

    // encapsulate and send
    IPv4Datagram *datagram = dynamic_cast<IPv4Datagram *>(packet);
    IPv4ControlInfo *controlInfo = NULL;
    //FIXME dubious code, remove? how can the HL tell IP whether it wants tunneling or forwarding?? --Andras
    if (!datagram) // if HL sends an IPv4Datagram, route the packet
    {
        // encapsulate
        controlInfo = check_and_cast<IPv4ControlInfo*>(packet->removeControlInfo());
        datagram = encapsulate(packet, controlInfo);
    }

    // extract requested interface and next hop
    const InterfaceEntry *destIE = controlInfo ? const_cast<const InterfaceEntry *>(ift->getInterfaceById(controlInfo->getInterfaceId())) : NULL;

    if (controlInfo)
        datagram->setControlInfo(controlInfo);    //FIXME ne rakjuk bele a cntrInfot!!!!! de kell :( kulonben a hook queue-ban elveszik a multicastloop flag

    // TODO:
    Address nextHopAddr(IPv4Address::UNSPECIFIED_ADDRESS);
    if (datagramLocalOutHook(datagram, destIE, nextHopAddr) == INetfilter::IHook::ACCEPT)
        datagramLocalOut(datagram, destIE, nextHopAddr.toIPv4());
}

void IPv4::handlePacketFromARP(cPacket *packet)
{
    // send out packet on the appropriate interface
    Ieee802Ctrl *ctrl = check_and_cast<Ieee802Ctrl*>(packet->getControlInfo());
    InterfaceEntry *destIE = ift->getInterfaceById(ctrl->getInterfaceId());
    sendPacketToNIC(packet, destIE);
}

void IPv4::datagramLocalOut(IPv4Datagram* datagram, const InterfaceEntry* destIE, IPv4Address requestedNextHopAddress)
{
    IPv4ControlInfo *controlInfo = dynamic_cast<IPv4ControlInfo*>(datagram->removeControlInfo());
    bool multicastLoop = true;
    if (controlInfo != NULL)
    {
        multicastLoop = controlInfo->getMulticastLoop();
        delete controlInfo;
    }

    // send
    IPv4Address &destAddr = datagram->getDestAddress();

    EV << "Sending datagram `" << datagram->getName() << "' with dest=" << destAddr << "\n";

    if (datagram->getDestAddress().isMulticast())
    {
        destIE = determineOutgoingInterfaceForMulticastDatagram(datagram, destIE);

        // loop back a copy
        if (multicastLoop && (!destIE || !destIE->isLoopback()))
        {
            const InterfaceEntry *loopbackIF = ift->getFirstLoopbackInterface();
            if (loopbackIF)
                fragmentPostRouting(datagram->dup(), loopbackIF, destAddr);
        }

        if (destIE)
        {
            numMulticast++;
            fragmentPostRouting(datagram, destIE, destAddr);
        }
        else
        {
            EV << "No multicast interface, packet dropped\n";
            numUnroutable++;
            delete datagram;
        }
    }
    else // unicast and broadcast
    {
        // check for local delivery
        if (rt->isLocalAddress(destAddr))
        {
            EV << "local delivery\n";
            if (destIE && !destIE->isLoopback())
            {
                EV << "datagram destination address is local, ignoring destination interface specified in the control info\n";
                destIE = NULL;
            }
            if (!destIE)
                destIE = ift->getFirstLoopbackInterface();
            ASSERT(destIE);
            routeUnicastPacket(datagram, NULL, destIE, destAddr);
        }
        else if (destAddr.isLimitedBroadcastAddress() || rt->isLocalBroadcastAddress(destAddr))
            routeLocalBroadcastPacket(datagram, destIE);
        else
            routeUnicastPacket(datagram, NULL, destIE, requestedNextHopAddress);
    }
}

/* Choose the outgoing interface for the muticast datagram:
 *   1. use the interface specified by MULTICAST_IF socket option (received in the control info)
 *   2. lookup the destination address in the routing table
 *   3. if no route, choose the interface according to the source address
 *   4. or if the source address is unspecified, choose the first MULTICAST interface
 */
const InterfaceEntry *IPv4::determineOutgoingInterfaceForMulticastDatagram(IPv4Datagram *datagram, const InterfaceEntry *multicastIFOption)
{
    const InterfaceEntry *ie = NULL;
    if (multicastIFOption)
    {
        ie = multicastIFOption;
        EV << "multicast packet routed by socket option via output interface " << ie->getName() << "\n";
    }
    if (!ie)
    {
        IPv4Route *route = rt->findBestMatchingRoute(datagram->getDestAddress());
        if (route)
            ie = route->getInterface();
        if (ie)
            EV << "multicast packet routed by routing table via output interface " << ie->getName() << "\n";
    }
    if (!ie)
    {
        ie = rt->getInterfaceByAddress(datagram->getSrcAddress());
        if (ie)
            EV << "multicast packet routed by source address via output interface " << ie->getName() << "\n";
    }
    if (!ie)
    {
        ie = ift->getFirstMulticastInterface();
        if (ie)
            EV << "multicast packet routed via the first multicast interface " << ie->getName() << "\n";
    }
    return ie;
}


void IPv4::routeUnicastPacket(IPv4Datagram *datagram, const InterfaceEntry *fromIE, const InterfaceEntry *destIE, IPv4Address requestedNextHopAddress)
{
    IPv4Address destAddr = datagram->getDestAddress();

    EV << "Routing datagram `" << datagram->getName() << "' with dest=" << destAddr << ": ";

    IPv4Address nextHopAddr;
    // if output port was explicitly requested, use that, otherwise use IPv4 routing
    if (destIE)
    {
        EV << "using manually specified output interface " << destIE->getName() << "\n";
        // and nextHopAddr remains unspecified
        if (!requestedNextHopAddress.isUnspecified())
            nextHopAddr = requestedNextHopAddress;
         // special case ICMP reply
        else if (destIE->isBroadcast())
        {
            // if the interface is broadcast we must search the next hop
            const IPv4Route *re = rt->findBestMatchingRoute(destAddr);
            if (re && re->getInterface() == destIE)
                nextHopAddr = re->getGateway();
        }
    }
    else
    {
        // use IPv4 routing (lookup in routing table)
        const IPv4Route *re = rt->findBestMatchingRoute(destAddr);
        if (re)
        {
            destIE = re->getInterface();
            nextHopAddr = re->getGateway();
        }
    }

    if (!destIE) // no route found
    {
        EV << "unroutable, sending ICMP_DESTINATION_UNREACHABLE\n";
        numUnroutable++;
        icmpAccess.get()->sendErrorMessage(datagram, ICMP_DESTINATION_UNREACHABLE, 0);
    }
    else // fragment and send
    {
        Address nextHop(nextHopAddr);
        if (fromIE != NULL)
        {
            if (datagramForwardHook(datagram, fromIE, destIE, nextHop) != INetfilter::IHook::ACCEPT)
                return;
            nextHopAddr = nextHop.toIPv4();
        }

        routeUnicastPacketFinish(datagram, fromIE, destIE, nextHopAddr);
    }
}

void IPv4::routeUnicastPacketFinish(IPv4Datagram *datagram, const InterfaceEntry *fromIE, const InterfaceEntry *destIE, IPv4Address nextHopAddr)
{
    EV << "output interface is " << destIE->getName() << ", next-hop address: " << nextHopAddr << "\n";
    numForwarded++;
    fragmentPostRouting(datagram, destIE, nextHopAddr);
}

void IPv4::routeLocalBroadcastPacket(IPv4Datagram *datagram, const InterfaceEntry *destIE)
{
    // The destination address is 255.255.255.255 or local subnet broadcast address.
    // We always use 255.255.255.255 as nextHopAddress, because it is recognized by ARP,
    // and mapped to the broadcast MAC address.
    if (destIE!=NULL)
    {
        fragmentPostRouting(datagram, destIE, IPv4Address::ALLONES_ADDRESS);
    }
    else if (forceBroadcast)
    {
        // forward to each interface including loopback
        for (int i = 0; i<ift->getNumInterfaces(); i++)
        {
            const InterfaceEntry *ie = ift->getInterface(i);
            fragmentPostRouting(datagram->dup(), ie, IPv4Address::ALLONES_ADDRESS);
        }
        delete datagram;
    }
    else
    {
        numDropped++;
        delete datagram;
    }
}

const InterfaceEntry *IPv4::getShortestPathInterfaceToSource(IPv4Datagram *datagram)
{
    return rt->getInterfaceForDestAddr(datagram->getSrcAddress());
}

void IPv4::forwardMulticastPacket(IPv4Datagram *datagram, const InterfaceEntry *fromIE)
{
    ASSERT(fromIE);
    const IPv4Address &srcAddr = datagram->getSrcAddress();
    const IPv4Address &destAddr = datagram->getDestAddress();
    ASSERT(destAddr.isMulticast());
    ASSERT(!destAddr.isLinkLocalMulticast());

    if (!nb)
        throw cRuntimeError("If multicast forwarding is enabled, then the node must contain a NotificationBoard.");

    EV << "Forwarding multicast datagram `" << datagram->getName() << "' with dest=" << destAddr << "\n";

    numMulticast++;

    const IPv4MulticastRoute *route = rt->findBestMatchingMulticastRoute(srcAddr, destAddr);
    if (!route)
    {
        EV << "Multicast route does not exist, try to add.\n";
        nb->fireChangeNotification(NF_IPv4_NEW_MULTICAST, datagram);

        // read new record
        route = rt->findBestMatchingMulticastRoute(srcAddr, destAddr);

        if (!route)
        {
            EV << "No route, packet dropped.\n";
            numUnroutable++;
            delete datagram;
            return;
        }
    }

    if (route->getInInterface() && fromIE != route->getInInterface()->getInterface())
    {
        EV << "Did not arrive on input interface, packet dropped.\n";
        nb->fireChangeNotification(NF_IPv4_DATA_ON_NONRPF, datagram);
        numDropped++;
        delete datagram;
    }
    // backward compatible: no parent means shortest path interface to source (RPB routing)
    else if (!route->getInInterface() && fromIE != getShortestPathInterfaceToSource(datagram))
    {
        EV << "Did not arrive on shortest path, packet dropped.\n";
        numDropped++;
        delete datagram;
    }
    else
    {
        nb->fireChangeNotification(NF_IPv4_DATA_ON_RPF, datagram); // forwarding hook

        numForwarded++;
        // copy original datagram for multiple destinations
        for (unsigned int i=0; i<route->getNumOutInterfaces(); i++)
        {
            IPv4MulticastRoute::OutInterface *outInterface = route->getOutInterface(i);
            const InterfaceEntry *destIE = outInterface->getInterface();
            if (destIE != fromIE && outInterface->isEnabled())
            {
                int ttlThreshold = destIE->ipv4Data()->getMulticastTtlThreshold();
                if (datagram->getTimeToLive() <= ttlThreshold)
                    EV << "Not forwarding to " << destIE->getName() << " (ttl treshold reached)\n";
                else if (outInterface->isLeaf() && !destIE->ipv4Data()->hasMulticastListener(destAddr))
                    EV << "Not forwarding to " << destIE->getName() << " (no listeners)\n";
                else
                {
                    EV << "Forwarding to " << destIE->getName() << "\n";
                    fragmentPostRouting(datagram->dup(), destIE, destAddr);
                }
            }
        }

        nb->fireChangeNotification(NF_IPv4_MDATA_REGISTER, datagram); // postRouting hook

        // only copies sent, delete original datagram
        delete datagram;
    }
}

void IPv4::reassembleAndDeliver(IPv4Datagram *datagram)
{
    EV << "Local delivery\n";

    if (datagram->getSrcAddress().isUnspecified())
        EV << "Received datagram '%s' without source address filled in" << datagram->getName() << "\n";

    // reassemble the packet (if fragmented)
    if (datagram->getFragmentOffset()!=0 || datagram->getMoreFragments())
    {
        EV << "Datagram fragment: offset=" << datagram->getFragmentOffset()
           << ", MORE=" << (datagram->getMoreFragments() ? "true" : "false") << ".\n";

        // erase timed out fragments in fragmentation buffer; check every 10 seconds max
        if (simTime() >= lastCheckTime + 10)
        {
            lastCheckTime = simTime();
            fragbuf.purgeStaleFragments(simTime()-fragmentTimeoutTime);
        }

        datagram = fragbuf.addFragment(datagram, simTime());
        if (!datagram)
        {
            EV << "No complete datagram yet.\n";
            return;
        }
        EV << "This fragment completes the datagram.\n";
    }

    if (datagramLocalInHook(datagram, getSourceInterfaceFrom(datagram)) != INetfilter::IHook::ACCEPT)
{
        return;
    }

    reassembleAndDeliverFinish(datagram);
}

void IPv4::reassembleAndDeliverFinish(IPv4Datagram *datagram)
{
    // decapsulate and send on appropriate output gate
    int protocol = datagram->getTransportProtocol();

    if (protocol==IP_PROT_ICMP)
    {
        // incoming ICMP packets are handled specially
        handleIncomingICMP(check_and_cast<ICMPMessage *>(decapsulate(datagram)));
        numLocalDeliver++;
    }
    else if (protocol==IP_PROT_IP)
    {
        // tunnelled IP packets are handled separately
        send(decapsulate(datagram), "preRoutingOut");  //FIXME There is no "preRoutingOut" gate in the IPv4 module.
    }
    else
    {
        int gateindex = mapping.findOutputGateForProtocol(protocol);
        // check if the transportOut port are connected, otherwise discard the packet
        if (gateindex >= 0)
        {
            cGate* outGate = gate("transportOut", gateindex);
            if (outGate->isPathOK())
            {
                send(decapsulate(datagram), outGate);
                numLocalDeliver++;
                return;
            }
        }

        EV << "L3 Protocol not connected. discarding packet" << endl;
        icmpAccess.get()->sendErrorMessage(datagram, ICMP_DESTINATION_UNREACHABLE, ICMP_DU_PROTOCOL_UNREACHABLE);
    }
}

cPacket *IPv4::decapsulate(IPv4Datagram *datagram)
{
    // decapsulate transport packet
    const InterfaceEntry *fromIE = getSourceInterfaceFrom(datagram);
    cPacket *packet = datagram->decapsulate();

    // create and fill in control info
    IPv4ControlInfo *controlInfo = new IPv4ControlInfo();
    controlInfo->setProtocol(datagram->getTransportProtocol());
    controlInfo->setSrcAddr(datagram->getSrcAddress());
    controlInfo->setDestAddr(datagram->getDestAddress());
    controlInfo->setTypeOfService(datagram->getTypeOfService());
    controlInfo->setInterfaceId(fromIE ? fromIE->getInterfaceId() : -1);
    controlInfo->setTimeToLive(datagram->getTimeToLive());

    // original IPv4 datagram might be needed in upper layers to send back ICMP error message
    controlInfo->setOrigDatagram(datagram);

    // attach control info
    packet->setControlInfo(controlInfo);

    return packet;
}

void IPv4::fragmentPostRouting(IPv4Datagram *datagram, const InterfaceEntry *ie, IPv4Address nextHopAddr)
{
    Address nextHop(nextHopAddr);
    if (datagramPostRoutingHook(datagram, getSourceInterfaceFrom(datagram), ie, nextHop) == INetfilter::IHook::ACCEPT)
        fragmentAndSend(datagram, ie, nextHop.toIPv4());
}

void IPv4::fragmentAndSend(IPv4Datagram *datagram, const InterfaceEntry *ie, IPv4Address nextHopAddr)
{
    // fill in source address
    if (datagram->getSrcAddress().isUnspecified())
        datagram->setSrcAddress(ie->ipv4Data()->getIPAddress());

    // hop counter decrement; but not if it will be locally delivered
    if (!ie->isLoopback())
        datagram->setTimeToLive(datagram->getTimeToLive()-1);

    // hop counter check
    if (datagram->getTimeToLive() < 0)
    {
        // drop datagram, destruction responsibility in ICMP
        EV << "datagram TTL reached zero, sending ICMP_TIME_EXCEEDED\n";
        icmpAccess.get()->sendErrorMessage(datagram, ICMP_TIME_EXCEEDED, 0);
        numDropped++;
        return;
    }

    int mtu = ie->getMTU();

    // send datagram straight out if it doesn't require fragmentation (note: mtu==0 means infinite mtu)
    if (mtu == 0 || datagram->getByteLength() <= mtu)
    {
        sendDatagramToOutput(datagram, ie, nextHopAddr);
        return;
    }

    // if "don't fragment" bit is set, throw datagram away and send ICMP error message
    if (datagram->getDontFragment())
    {
        EV << "datagram larger than MTU and don't fragment bit set, sending ICMP_DESTINATION_UNREACHABLE\n";
        icmpAccess.get()->sendErrorMessage(datagram, ICMP_DESTINATION_UNREACHABLE,
                                                     ICMP_FRAGMENTATION_ERROR_CODE);
        numDropped++;
        return;
    }

    // FIXME some IP options should not be copied into each fragment, check their COPY bit
    int headerLength = datagram->getHeaderLength();
    int payloadLength = datagram->getByteLength() - headerLength;
    int fragmentLength = ((mtu - headerLength) / 8) * 8; // payload only (without header)
    int offsetBase = datagram->getFragmentOffset();
    if (fragmentLength <= 0)
        throw cRuntimeError("Cannot fragment datagram: MTU=%d too small for header size (%d bytes)", mtu, headerLength); // exception and not ICMP because this is likely a simulation configuration error, not something one wants to simulate

    int noOfFragments = (payloadLength + fragmentLength - 1) / fragmentLength;
    EV << "Breaking datagram into " << noOfFragments << " fragments\n";

    // create and send fragments
    std::string fragMsgName = datagram->getName();
    fragMsgName += "-frag";

    for (int offset=0; offset < payloadLength; offset+=fragmentLength)
    {
        bool lastFragment = (offset+fragmentLength >= payloadLength);
        // length equal to fragmentLength, except for last fragment;
        int thisFragmentLength = lastFragment ? payloadLength - offset : fragmentLength;

        // FIXME is it ok that full encapsulated packet travels in every datagram fragment?
        // should better travel in the last fragment only. Cf. with reassembly code!
        IPv4Datagram *fragment = (IPv4Datagram *) datagram->dup();
        fragment->setName(fragMsgName.c_str());

        // "more fragments" bit is unchanged in the last fragment, otherwise true
        if (!lastFragment)
            fragment->setMoreFragments(true);

        fragment->setByteLength(headerLength + thisFragmentLength);
        fragment->setFragmentOffset(offsetBase + offset);

        sendDatagramToOutput(fragment, ie, nextHopAddr);
    }

    delete datagram;
}

IPv4Datagram *IPv4::encapsulate(cPacket *transportPacket, IPv4ControlInfo *controlInfo)
{
    IPv4Datagram *datagram = createIPv4Datagram(transportPacket->getName());
    datagram->setByteLength(IP_HEADER_BYTES);
    datagram->encapsulate(transportPacket);

    // set source and destination address
    IPv4Address dest = controlInfo->getDestAddr();
    datagram->setDestAddress(dest);

    IPv4Address src = controlInfo->getSrcAddr();

    // when source address was given, use it; otherwise it'll get the address
    // of the outgoing interface after routing
    if (!src.isUnspecified())
    {
        // if interface parameter does not match existing interface, do not send datagram
        if (rt->getInterfaceByAddress(src)==NULL)
            throw cRuntimeError("Wrong source address %s in (%s)%s: no interface with such address",
                      src.str().c_str(), transportPacket->getClassName(), transportPacket->getFullName());

        datagram->setSrcAddress(src);
    }

    // set other fields
    datagram->setTypeOfService(controlInfo->getTypeOfService());

    datagram->setIdentification(curFragmentId++);
    datagram->setMoreFragments(false);
    datagram->setDontFragment(controlInfo->getDontFragment());
    datagram->setFragmentOffset(0);

    short ttl;
    if (controlInfo->getTimeToLive() > 0)
        ttl = controlInfo->getTimeToLive();
    else if (datagram->getDestAddress().isLinkLocalMulticast())
        ttl = 1;
    else if (datagram->getDestAddress().isMulticast())
        ttl = defaultMCTimeToLive;
    else
        ttl = defaultTimeToLive;
    datagram->setTimeToLive(ttl);
    datagram->setTransportProtocol(controlInfo->getProtocol());

    // setting IPv4 options is currently not supported

    return datagram;
}

IPv4Datagram *IPv4::createIPv4Datagram(const char *name)
{
    return new IPv4Datagram(name);
}

void IPv4::sendDatagramToOutput(IPv4Datagram *datagram, const InterfaceEntry *ie, IPv4Address nextHopAddr)
{
    {
        bool isIeee802Lan = ie->isBroadcast() && !ie->getMacAddress().isUnspecified(); // we only need/can do ARP on IEEE 802 LANs
        if (!isIeee802Lan) {
            sendPacketToNIC(datagram, ie);
        }
        else {
            if (nextHopAddr.isUnspecified()) {
                IPv4InterfaceData* ipv4Data = ie->ipv4Data();
                IPv4Address destAddress = datagram->getDestAddress();
                if (IPv4Address::maskedAddrAreEqual(destAddress, ie->ipv4Data()->getIPAddress(), ipv4Data->getNetmask()))
                    nextHopAddr = destAddress;
                else if (useProxyARP) {
                    nextHopAddr = destAddress;
                    EV_WARN << "no next-hop address, using destination address " << nextHopAddr << " (proxy ARP)\n";
                }
                else {
                    throw cRuntimeError(datagram, "Cannot send datagram on broadcast interface: no next-hop address and Proxy ARP is disabled");
                }
            }

            MACAddress nextHopMacAddr = resolveNextHopMacAddress(datagram, nextHopAddr, ie);

            if (nextHopMacAddr.isUnspecified()) {
                IPv4RoutingDecision *routingDecision = new IPv4RoutingDecision();
                routingDecision->setInterfaceId(ie->getInterfaceId());
                routingDecision->setNextHopAddr(nextHopAddr);
                datagram->setControlInfo(routingDecision);

                send(datagram, arpDgramOutGate);  // send to ARP for resolution
            }
            else {
                sendPacketToIeee802NIC(datagram, ie, nextHopMacAddr, ETHERTYPE_IPv4);
            }
        }
    }
}

MACAddress IPv4::resolveNextHopMacAddress(cPacket *packet, IPv4Address nextHopAddr, const InterfaceEntry *destIE)
{
    if (nextHopAddr.isLimitedBroadcastAddress() || nextHopAddr == destIE->ipv4Data()->getNetworkBroadcastAddress())
    {
        EV << "destination address is broadcast, sending packet to broadcast MAC address\n";
        return MACAddress::BROADCAST_ADDRESS;
    }

    if (nextHopAddr.isMulticast())
    {
        MACAddress macAddr = MACAddress::makeMulticastAddress(nextHopAddr);
        EV << "destination address is multicast, sending packet to MAC address " << macAddr << "\n";
        return macAddr;
    }

    return arp->getDirectAddressResolution(nextHopAddr);
}

void IPv4::sendPacketToIeee802NIC(cPacket *packet, const InterfaceEntry *ie, const MACAddress& macAddress, int etherType)
{
    // remove old control info
    delete packet->removeControlInfo();

    // add control info with MAC address
    Ieee802Ctrl *controlInfo = new Ieee802Ctrl();
    controlInfo->setDest(macAddress);
    controlInfo->setEtherType(etherType);
    packet->setControlInfo(controlInfo);

    sendPacketToNIC(packet, ie);
}

void IPv4::sendPacketToNIC(cPacket *packet, const InterfaceEntry *ie)
{
    EV << "Sending out packet to interface " << ie->getName() << endl;
    send(packet, queueOutGateBaseId + ie->getNetworkLayerGateIndex());
}

// NetFilter:

void IPv4::registerHook(int priority, INetfilter::IHook* hook)
{
    Enter_Method("registerHook()");
    hooks.insert(std::pair<int, INetfilter::IHook*>(priority, hook));
}

void IPv4::unregisterHook(int priority, INetfilter::IHook* hook)
{
    Enter_Method("unregisterHook()");
    for (HookList::iterator iter = hooks.begin(); iter != hooks.end(); iter++) {
        if ((iter->first == priority) && (iter->second == hook)) {
            hooks.erase(iter);
            return;
        }
    }
}

void IPv4::dropQueuedDatagram(const INetworkDatagram* datagram)
{
    Enter_Method("dropQueuedDatagram()");
    for (DatagramQueueForHooks::iterator iter = queuedDatagramsForHooks.begin(); iter != queuedDatagramsForHooks.end(); iter++) {
        if (iter->datagram == datagram) {
            delete datagram;
            queuedDatagramsForHooks.erase(iter);
            return;
        }
    }
}

void IPv4::reinjectQueuedDatagram(const INetworkDatagram* datagram)
{
    Enter_Method("reinjectDatagram()");
    for (DatagramQueueForHooks::iterator iter = queuedDatagramsForHooks.begin(); iter != queuedDatagramsForHooks.end(); iter++) {
        if (iter->datagram == datagram) {
            IPv4Datagram* datagram = iter->datagram;
            take(datagram);
            switch (iter->hookType) {
                case INetfilter::IHook::LOCALOUT:
                    datagramLocalOut(datagram, iter->outIE, iter->nextHopAddr);
                    break;
                case INetfilter::IHook::PREROUTING:
                    preroutingFinish(datagram, iter->inIE, iter->outIE, iter->nextHopAddr);
                    break;
                case INetfilter::IHook::POSTROUTING:
                    fragmentAndSend(datagram, iter->outIE, iter->nextHopAddr);
                    break;
                case INetfilter::IHook::LOCALIN:
                    reassembleAndDeliverFinish(datagram);
                    break;
                case INetfilter::IHook::FORWARD:
                    routeUnicastPacketFinish(datagram, iter->inIE, iter->outIE, iter->nextHopAddr);
                    break;
                default:
                    throw cRuntimeError("Unknown hook ID: %d", (int)(iter->hookType));
                    break;
            }
            queuedDatagramsForHooks.erase(iter);
            return;
        }
    }
}

INetfilter::IHook::Result IPv4::datagramPreRoutingHook(INetworkDatagram* datagram, const InterfaceEntry* inIE, const InterfaceEntry*& outIE, Address& nextHopAddr)
{
    for (HookList::iterator iter = hooks.begin(); iter != hooks.end(); iter++) {
        IHook::Result r = iter->second->datagramPreRoutingHook(datagram, inIE, outIE, nextHopAddr);
        switch(r)
        {
            case INetfilter::IHook::ACCEPT: break;   // continue iteration
            case INetfilter::IHook::DROP:   delete datagram; return r;
            case INetfilter::IHook::QUEUE:  queuedDatagramsForHooks.push_back(QueuedDatagramForHook(dynamic_cast<IPv4Datagram *>(datagram), inIE, outIE, nextHopAddr.toIPv4(), INetfilter::IHook::PREROUTING)); return r;
            case INetfilter::IHook::STOLEN: return r;
            default: throw cRuntimeError("Unknown Hook::Result value: %d", (int)r);
        }
    }
    return INetfilter::IHook::ACCEPT;
}

INetfilter::IHook::Result IPv4::datagramForwardHook(INetworkDatagram* datagram, const InterfaceEntry* inIE, const InterfaceEntry*& outIE, Address& nextHopAddr)
{
    for (HookList::iterator iter = hooks.begin(); iter != hooks.end(); iter++) {
        IHook::Result r = iter->second->datagramForwardHook(datagram, inIE, outIE, nextHopAddr);
        switch(r)
        {
            case INetfilter::IHook::ACCEPT: break;   // continue iteration
            case INetfilter::IHook::DROP:   delete datagram; return r;
            case INetfilter::IHook::QUEUE:  queuedDatagramsForHooks.push_back(QueuedDatagramForHook(dynamic_cast<IPv4Datagram *>(datagram), inIE, outIE, nextHopAddr.toIPv4(), INetfilter::IHook::FORWARD)); return r;
            case INetfilter::IHook::STOLEN: return r;
            default: throw cRuntimeError("Unknown Hook::Result value: %d", (int)r);
        }
    }
    return INetfilter::IHook::ACCEPT;
}

INetfilter::IHook::Result IPv4::datagramPostRoutingHook(INetworkDatagram* datagram, const InterfaceEntry* inIE, const InterfaceEntry*& outIE, Address& nextHopAddr)
{
    for (HookList::iterator iter = hooks.begin(); iter != hooks.end(); iter++) {
        IHook::Result r = iter->second->datagramPostRoutingHook(datagram, inIE, outIE, nextHopAddr);
        switch(r)
        {
            case INetfilter::IHook::ACCEPT: break;   // continue iteration
            case INetfilter::IHook::DROP:   delete datagram; return r;
            case INetfilter::IHook::QUEUE:  queuedDatagramsForHooks.push_back(QueuedDatagramForHook(dynamic_cast<IPv4Datagram *>(datagram), inIE, outIE, nextHopAddr.toIPv4(), INetfilter::IHook::POSTROUTING)); return r;
            case INetfilter::IHook::STOLEN: return r;
            default: throw cRuntimeError("Unknown Hook::Result value: %d", (int)r);
        }
    }
    return INetfilter::IHook::ACCEPT;
}

bool IPv4::handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback)
{
    Enter_Method_Silent();
    if (dynamic_cast<NodeStartOperation *>(operation)) {
        if (stage == NodeStartOperation::STAGE_NETWORK_LAYER)
            start();
    }
    else if (dynamic_cast<NodeShutdownOperation *>(operation)) {
        if (stage == NodeShutdownOperation::STAGE_NETWORK_LAYER)
            stop();
    }
    else if (dynamic_cast<NodeCrashOperation *>(operation)) {
        if (stage == NodeCrashOperation::STAGE_CRASH)
            stop();
    }
    return true;
}

void IPv4::start()
{
    ASSERT(queue.isEmpty());
    isUp = true;
}

void IPv4::stop()
{
    isUp = false;
    flush();
}

void IPv4::flush()
{
    delete cancelService();
    queue.clear();
}

bool IPv4::isNodeUp()
{
    NodeStatus *nodeStatus = dynamic_cast<NodeStatus *>(findContainingNode(this)->getSubmodule("status"));
    return !nodeStatus || nodeStatus->getState() == NodeStatus::UP;
}

INetfilter::IHook::Result IPv4::datagramLocalInHook(INetworkDatagram* datagram, const InterfaceEntry* inIE)
{
    for (HookList::iterator iter = hooks.begin(); iter != hooks.end(); iter++) {
        IHook::Result r = iter->second->datagramLocalInHook(datagram, inIE);
        switch(r)
        {
            case INetfilter::IHook::ACCEPT: break;   // continue iteration
            case INetfilter::IHook::DROP:   delete datagram; return r;
            case INetfilter::IHook::QUEUE:  queuedDatagramsForHooks.push_back(QueuedDatagramForHook(dynamic_cast<IPv4Datagram *>(datagram), inIE, NULL, IPv4Address::UNSPECIFIED_ADDRESS, INetfilter::IHook::LOCALIN)); return r;
            case INetfilter::IHook::STOLEN: return r;
            default: throw cRuntimeError("Unknown Hook::Result value: %d", (int)r);
        }
    }
    return INetfilter::IHook::ACCEPT;
}

INetfilter::IHook::Result IPv4::datagramLocalOutHook(INetworkDatagram* datagram, const InterfaceEntry*& outIE, Address& nextHopAddr)
{
    for (HookList::iterator iter = hooks.begin(); iter != hooks.end(); iter++) {
        IHook::Result r = iter->second->datagramLocalOutHook(datagram, outIE, nextHopAddr);
        switch(r)
        {
            case INetfilter::IHook::ACCEPT: break;   // continue iteration
            case INetfilter::IHook::DROP:   delete datagram; return r;
            case INetfilter::IHook::QUEUE:  queuedDatagramsForHooks.push_back(QueuedDatagramForHook(dynamic_cast<IPv4Datagram *>(datagram), NULL, outIE, nextHopAddr.toIPv4(), INetfilter::IHook::LOCALOUT)); return r;
            case INetfilter::IHook::STOLEN: return r;
            default: throw cRuntimeError("Unknown Hook::Result value: %d", (int)r);
        }
    }
    return INetfilter::IHook::ACCEPT;
}

void IPv4::sendOnTransPortOutGateByProtocolId(cPacket *packet, int protocolId)
{
    int gateindex = mapping.getOutputGateForProtocol(protocolId);
    cGate* outGate = gate("transportOut", gateindex);
    send(packet, outGate);
}

