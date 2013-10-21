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
    INITSTAGE_LOCAL = 0,

    /**
     * Physical environment (mobility, obstacles, battery, annotations, etc); Traci-launch
     */
    INITSTAGE_PHYSICAL_ENVIRONMENT = 2,

    /**
     * INITIALIZE_AND_PUBLISH_LOCATION
     */
    INITSTAGE_PHYSICAL_ENVIRONMENT_2 = 3,

    /**
     * PUBLISH_RADIOSTATE, REGISTER_RADIO in ChannelController
     */
    INITSTAGE_PHYSICAL_LAYER = 4,

    /**
     * REGISTER_INTERFACE, GENERATE_MACADDRESS = 5,
     */
    INITSTAGE_LINK_LAYER = 5,

    /**
     * TODO
     */
    INITSTAGE_LINK_LAYER_2 = 6,

    /**
     * IPV6ROUTINGTABLE_XMIPV6_SETTINGS, ADD_IP_PROTOCOLDATA_TO_INTERFACEENTRY, COMPUTE_IP_AUTOCONFIGURATION,
     * isOperational, IPv4::registerHook
     */
    INITSTAGE_NETWORK_LAYER = 7,

    /**
     * CONFIGURE_IP_ADDRESSES
     */
    INITSTAGE_NETWORK_LAYER_2 = 8,

    /**
     * ADD_STATIC_ROUTES, SET_INTERFACEENTRY_RTR_ADV_INTERVAL, ASSIGN_ROUTERID
     */
    INITSTAGE_NETWORK_LAYER_3 = 9,

    /**
     * TODO (LIBTable reads routerID)
     */
    INITSTAGE_NETWORK_LAYER_4 = 10,

    /**
     * REGISTER_TRANSPORTPROTOCOLID_IN_IP
     */
    INITSTAGE_TRANSPORT_LAYER = 11,   //FIXME used many times in *routing* protocols, replace those with STAGE_ROUTING!!

    /**
     * TODO
     */
    INITSTAGE_TRANSPORT_LAYER_2 = 12,

    /**
     * INIT_ROUTING_PROTOCOLS
     */
    INITSTAGE_ROUTING_PROTOCOLS = 13,

    /**
     * INIT_APPLICATION
     */
    INITSTAGE_APPLICATION_LAYER = 14,

    INITSTAGE_APPLICATION_LAYER_2 = 15,

    NUM_STAGES,
};

#endif // __INET_INITSTAGES

