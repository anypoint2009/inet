//
// Copyright (C) 2013 OpenSim Ltd.
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
// author: Zoltan Bojthe
//

#ifndef __INET_INITSTAGES
#define __INET_INITSTAGES


//NOTE: to make naming consistent with lifecycle stage constants, rename these constants to be similar to these:
// STAGE_LOCAL, STAGE_PHYSICAL_LAYER, STAGE_LINK_LAYER, STAGE_NETWORK_LAYER, STAGE_TRANSPORT_LAYER, STAGE_ROUTING_PROTOCOLS, STAGE_APPLICATION_LAYER
enum InetInitStages
{
    /**
     * Local initialization. MAC assigns auto addresses and adjusts the "address" module parameter;
     * NodeStatus, NotificationBoard, IPassiveQueue, etc are available for other modules.
     */
    NEWSTAGE_LOCAL_INITIALIZATION = 0,

    /**
     * TODO: this could be done in stage 0...
     */
    STAGE_DO_SUBSCRIBE_TO_RADIOSTATE_NOTIFICATIONS = 1,  //FIXME this is only used in 802.11MAC, and could be done in stage 0 when signals are used

    /**
     * Physical environment (mobility, obstacles, battery, annotations, etc); Traci-launch
     */
    NEWSTAGE_PHYSICALENV_INITIALIZATION = 2,

    /**
     * INITIALIZE_AND_PUBLISH_LOCATION
     */
    NEWSTAGE_PHYSICALENV_SECOND = 3,

    /**
     * PUBLISH_RADIOSTATE, REGISTER_RADIO in ChannelController
     */
    NEWSTAGE_L1_INITIALIZATION = 4,

    /**
     * REGISTER_INTERFACE, GENERATE_MACADDRESS = 5,
     */
    NEWSTAGE_L2_INITIALIZATION = 5,

    /**
     * IPV6ROUTINGTABLE_XMIPV6_SETTINGS, ADD_IP_PROTOCOLDATA_TO_INTERFACEENTRY, COMPUTE_IP_AUTOCONFIGURATION
     */
    NEWSTAGE_L3_INITIALIZATION = 6,

    /**
     * CONFIGURE_IP_ADDRESSES
     */
    NEWSTAGE_L3_IPADDRESSES = 7,

    /**
     * ADD_STATIC_ROUTES, SET_INTERFACEENTRY_RTR_ADV_INTERVAL, ASSIGN_ROUTERID
     */
    NEWSTAGE_L3_STATICROUTES = 8,

    /**
     * REGISTER_TRANSPORTPROTOCOLID_IN_IP
     */
    NEWSTAGE_TRANSPORT = 9,

    /**
     * INIT_ROUTING_PROTOCOLS
     */
    NEWSTAGE_ROUTING = 10,

    /**
     * INIT_APPLICATION
     */
    NEWSTAGE_APPLICATIONS = 11,

    NUM_STAGES,

    //---------------
    //EZ MIND LOCAL+1:
    STAGE_LOCAL_PLUS_1 = NEWSTAGE_LOCAL_INITIALIZATION + 1,        // NodeStatus module knows the initial status of the node

    //EZEK KESOBBIEK:
    STAGE_MACADDRESS_AVAILABLE = NEWSTAGE_L2_INITIALIZATION + 1,
    STAGE_INTERFACEENTRY_REGISTERED = NEWSTAGE_L2_INITIALIZATION + 1,
    STAGE_TRANSPORT_LAYER_AVAILABLE = NEWSTAGE_TRANSPORT + 1, //_max_(STAGE_LOCAL_PLUS_1 + 1, NEWSTAGE_TRANSPORT + 1),
    STAGE_LOCATION_AVAILABLE = NEWSTAGE_PHYSICALENV_SECOND + 1,
    STAGE_IP_ADDRESS_AVAILABLE = NEWSTAGE_L3_IPADDRESSES + 1,
    STAGE_ROUTERID_AVAILABLE = NEWSTAGE_L3_STATICROUTES + 1,
};

#endif // __INET_INITSTAGES

