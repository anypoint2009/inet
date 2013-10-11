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
#define STAGE_DO_LOCAL                                                   0
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

    // stage 0
    STAGE_DO_LOCAL = 0,

    // stage 1
    STAGE_DO_ASSIGN_MOBILITY_COORDINATOR = 1,
    STAGE_DO_IPV6ROUTINGTABLE_XMIPV6_SETTINGS = 1,
    STAGE_DO_TRACI_LAUNCH = 1,
    STAGE_DO_REGISTER_INTERFACE = 1,
    STAGE_DO_REGISTER_TRANSPORTPROTOCOLID_IN_IP = 1,
    STAGE_DO_GENERATE_MACADDRESS = 1,
    STAGE_DO_SUBSCRIBE_TO_RADIOSTATE_NOTIFICATIONS = 1,

    // stage 2
    STAGE_DO_PUBLISH_RADIOSTATE = 2,
    STAGE_DO_ADD_IP_PROTOCOLDATA_TO_INTERFACEENTRY = 2,
    STAGE_DO_INITIALIZE_AND_PUBLISH_LOCATION = 2,
    STAGE_DO_COMPUTE_IP_AUTOCONFIGURATION = 2,

    // stage 3
    STAGE_DO_REGISTER_RADIO = 3,
    STAGE_DO_CONFIGURE_IP_ADDRESSES = 3,

    // stage 4
    STAGE_DO_ADD_STATIC_ROUTES = 4,
    STAGE_DO_SET_INTERFACEENTRY_RTR_ADV_INTERVAL = 4,
    STAGE_DO_ASSIGN_ROUTERID = 4,

    // stage 5
    STAGE_DO_INIT_ROUTING_PROTOCOLS = 5,

    // stage 6
    STAGE_DO_INIT_APPLICATION = 6,

    NUM_STAGES,

    //---------------
    STAGE_CHANNEL_AVAILABLE = 0,    // gate path bejarhato, transmission channel available on gate path
    STAGE_NODESTATUS_AVAILABLE = STAGE_DO_LOCAL + 1,        // NodeStatus module knows the initial status of the node
    STAGE_IP_LAYER_READY_FOR_HOOK_REGISTRATION = STAGE_DO_LOCAL + 1,      // The IP layer ready for calling the registerHook() function
    STAGE_IP_LAYER_READY_FOR_PROTOCOL_REGISTRATION = STAGE_DO_LOCAL + 1,      // The IP layer ready for receive the registerProtocol message
    STAGE_NOTIFICATIONBOARD_AVAILABLE = STAGE_DO_LOCAL + 1,      // The NotificationBoard ready for calling the subscribe() and fireChangeNotification() functions
    STAGE_ANNOTATIONMANAGER_AVAILABLE = STAGE_DO_LOCAL + 1,
    STAGE_BATTERY_READY_FOR_DEVICE_REGISTRATION = STAGE_DO_LOCAL + 1,
    STAGE_CHANNELCONTROL_AVAILABLE = STAGE_DO_LOCAL + 1,      // registerRadio(), setChannel(), etc. available in IChannelControl module
    STAGE_INTERFACETABLE_READY_FOR_INTERFACE_REGISTRATION = STAGE_DO_LOCAL + 1,
    STAGE_CHANNELCONTROL_NUMCHANNELS_AVAILABLE = STAGE_DO_LOCAL + 1,
    STAGE_IPASSIVEQUEUE_AVAILABLE = STAGE_DO_LOCAL + 1,       // IPassiveQueue::memberFunctions() work correctly
    STAGE_TRANSPORTPROTOCOLID_REGISTERED_IN_IP = STAGE_DO_REGISTER_TRANSPORTPROTOCOLID_IN_IP + 1,
    STAGE_RADIOSTATE_SUBSCRIPTIONS_DONE = STAGE_DO_SUBSCRIBE_TO_RADIOSTATE_NOTIFICATIONS + 1,
    STAGE_MACADDRESS_AVAILABLE = STAGE_DO_GENERATE_MACADDRESS + 1,
    STAGE_INTERFACEENTRY_REGISTERED = STAGE_DO_REGISTER_INTERFACE + 1,
    STAGE_MOBILITY_COORDINATOR_ASSIGNED = STAGE_DO_ASSIGN_MOBILITY_COORDINATOR + 1,
    STAGE_TRANSPORT_LAYER_AVAILABLE = _max_(STAGE_NODESTATUS_AVAILABLE + 1, STAGE_TRANSPORTPROTOCOLID_REGISTERED_IN_IP),
    STAGE_LOCATION_AVAILABLE = STAGE_DO_INITIALIZE_AND_PUBLISH_LOCATION + 1,
    STAGE_INTERFACEENTRY_IP_PROTOCOLDATA_AVAILABLE = STAGE_DO_ADD_IP_PROTOCOLDATA_TO_INTERFACEENTRY + 1,
    STAGE_IP_ADDRESS_AVAILABLE = STAGE_DO_CONFIGURE_IP_ADDRESSES + 1,
    STAGE_ROUTERID_AVAILABLE = STAGE_DO_ASSIGN_ROUTERID + 1,
    STAGE_ROUTING_PROTOCOLS_INITIALIZED = STAGE_DO_INIT_ROUTING_PROTOCOLS + 1,

};

#undef _max_

#endif // __INET_INITSTAGES

