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

//TODO: rewrite to like these:
/*
#define NEWSTAGE_LOCAL_INITIALIZATION                                                   0
#define STAGE_INTERFACE_TABLE_READY_TO_REGISTER_INTERFACES         STAGE_LOCAL + 1
#define STAGE_DO_REGISTER_ETH_INTERFACE                                  STAGE_INTERFACE_TABLE_IS_READY_TO_REGISTER_INTERFACES
#define STAGE_DO_REGISTER_PPP_INTERFACE                                  STAGE_INTERFACE_TABLE_IS_READY_TO_REGISTER_INTERFACES
#define STAGE_ETH_INTERFACE_REGISTERED                             STAGE_REGISTER_ETH_INTERFACE + 1
#define STAGE_PPP_INTERFACE_REGISTERED                             STAGE_REGISTER_PPP_INTERFACE + 1
#define STAGE_ALL_INTERFACES_REGISTERED                           max(STAGE_ETH_INTERFACE_IS_REGISTERED, STAGE_PPP_INTERFACE_IS_REGISTERED)
*/

/**
 * cModule::initialize stage naming rules
 *
 * Stage names are defined using C++ macro definitions. There are two kinds of
 * such macros, one that specifies some activity, and another one that specifies
 * some state.
 *
 * An activity describing stage name tells what does the corresponding else-if
 * branch in the implementor module's initialize function should do. These names
 * should start with the STAGE_DO prefix. The macro expressions on the right side
 * should only use state describing stage names.
 *
 * A state describing stage name should tell what state is available for all modules
 * in that stage. These names should start with the STAGE prefix. The macro expressions
 * on the right side should only use activity describing stage names.
 */

#define _max_(a, b)   ((a) > (b) ? (a) : (b))

enum InetInitStages
{
//NOTE:
//    STAGE_LOCAL, // for changes that don't depend on other modules
//    STAGE_PHYSICAL_LAYER,
//    STAGE_LINK_LAYER,
//    STAGE_NETWORK_LAYER,
//    STAGE_TRANSPORT_LAYER,
//    STAGE_ROUTING_PROTOCOLS,
//    STAGE_APPLICATION_LAYER,
//    STAGE_LAST
//e.g.
//    STAGE_NETWORK_LAYER_1
//    STAGE_NETWORK_LAYER_2
//    STAGE_NETWORK_LAYER_3

    /**
     * Local initialization
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
    STAGE_CHANNEL_AVAILABLE = 0,    // gate path bejarhato, transmission channel available on gate path
    STAGE_NODESTATUS_AVAILABLE = NEWSTAGE_LOCAL_INITIALIZATION + 1,        // NodeStatus module knows the initial status of the node
    STAGE_IP_LAYER_READY_FOR_HOOK_REGISTRATION = NEWSTAGE_LOCAL_INITIALIZATION + 1,      // The IP layer ready for calling the registerHook() function
    STAGE_IP_LAYER_READY_FOR_PROTOCOL_REGISTRATION = NEWSTAGE_LOCAL_INITIALIZATION + 1,      // The IP layer ready for receive the registerProtocol message
    STAGE_NOTIFICATIONBOARD_AVAILABLE = 0, // note: it is ready right after it constructor call... //NEWSTAGE_LOCAL_INITIALIZATION + 1,      // The NotificationBoard ready for calling the subscribe() and fireChangeNotification() functions
    STAGE_ANNOTATIONMANAGER_AVAILABLE = NEWSTAGE_LOCAL_INITIALIZATION + 1,
    STAGE_BATTERY_READY_FOR_DEVICE_REGISTRATION = NEWSTAGE_LOCAL_INITIALIZATION + 1,
    STAGE_CHANNELCONTROL_AVAILABLE = NEWSTAGE_LOCAL_INITIALIZATION + 1,      // registerRadio(), setChannel(), etc. available in IChannelControl module
    STAGE_INTERFACETABLE_READY_FOR_INTERFACE_REGISTRATION = NEWSTAGE_LOCAL_INITIALIZATION + 1,
    STAGE_CHANNELCONTROL_NUMCHANNELS_AVAILABLE = NEWSTAGE_LOCAL_INITIALIZATION + 1,
    STAGE_IPASSIVEQUEUE_AVAILABLE = NEWSTAGE_LOCAL_INITIALIZATION + 1,       // IPassiveQueue::memberFunctions() work correctly
    STAGE_TRANSPORTPROTOCOLID_REGISTERED_IN_IP = NEWSTAGE_TRANSPORT + 1,
    STAGE_RADIOSTATE_SUBSCRIPTIONS_DONE = STAGE_DO_SUBSCRIBE_TO_RADIOSTATE_NOTIFICATIONS + 1,
    STAGE_MACADDRESS_AVAILABLE = NEWSTAGE_L2_INITIALIZATION + 1,
    STAGE_INTERFACEENTRY_REGISTERED = NEWSTAGE_L2_INITIALIZATION + 1,
    STAGE_MOBILITY_COORDINATOR_ASSIGNED = NEWSTAGE_PHYSICALENV_INITIALIZATION + 1,
    STAGE_TRANSPORT_LAYER_AVAILABLE = _max_(STAGE_NODESTATUS_AVAILABLE + 1, STAGE_TRANSPORTPROTOCOLID_REGISTERED_IN_IP),
    STAGE_LOCATION_AVAILABLE = NEWSTAGE_PHYSICALENV_SECOND + 1,
    STAGE_INTERFACEENTRY_IP_PROTOCOLDATA_AVAILABLE = NEWSTAGE_L3_INITIALIZATION + 1,
    STAGE_IP_ADDRESS_AVAILABLE = NEWSTAGE_L3_IPADDRESSES + 1,
    STAGE_ROUTERID_AVAILABLE = NEWSTAGE_L3_STATICROUTES + 1,
    STAGE_ROUTING_PROTOCOLS_INITIALIZED = NEWSTAGE_ROUTING + 1,

};

#undef _max_

#endif // __INET_INITSTAGES

