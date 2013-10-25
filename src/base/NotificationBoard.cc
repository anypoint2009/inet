//
// Copyright (C) 2005 Andras Varga
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


#include <algorithm>
#include "NotificationBoard.h"
#include "NotifierConsts.h"
#include "INotifiable.h"

Define_Module(NotificationBoard);

std::ostream& operator<<(std::ostream& os, const NotificationBoard::NotifiableVector& v)
{
    return os;
}


void NotificationBoard::initialize()
{
    WATCH_MAP(clientMap);
}

void NotificationBoard::handleMessage(cMessage *msg)
{
    error("NotificationBoard doesn't handle messages, it can be accessed via direct method calls");
}


void NotificationBoard::subscribe(INotifiable *client, int category)
{
}

void NotificationBoard::unsubscribe(INotifiable *client, int category)
{
}

bool NotificationBoard::hasSubscribers(int category)
{
    return true;
}

void NotificationBoard::fireChangeNotification(int category, const cObject *details)
{
}


