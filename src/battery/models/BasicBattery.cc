/***************************************************************************
 * Simple battery model for inetmanet framework
 * Author:  Isabel Dietrich
 *
 * This software is provided `as is' and without any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose.
 *
 ***************************************************************************/

/**
 * @short Implementation of a simple battery model
 *      "real" battery models should subclass this!
 *      The basic class publishes the remaining energy on the notification board,
 *      but does not decrement the energy!
 *      It can therefore be used for hosts having an infinite energy supply
 *      i.e. a power plug
 * @author Isabel Dietrich
*/

#include "BasicBattery.h"


/////////////////////////////// PUBLIC ///////////////////////////////////////

//============================= LIFECYCLE ===================================
/**
 * Initialization routine
 */
int BasicBattery::numInitStages() const
{
    return STAGE_DO_LOCAL + 1;
}

void BasicBattery::initialize(int stage)
{
    InetSimpleModule::initialize(stage); //DO NOT DELETE!!

    if (stage == STAGE_DO_LOCAL)
    {
        mpNb = NotificationBoardAccess().get();
    }
}

void BasicBattery::finish()
{

}


/**
 * Dispatches self-messages to handleSelfMsg()
 */

