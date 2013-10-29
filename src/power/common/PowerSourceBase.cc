//
// Copyright (C) 2013 Andras Varga
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

#include "PowerSourceBase.h"

int PowerSourceBase::getNumPowerConsumers()
{
    return powerConsumers.size();
}

IPowerConsumer * PowerSourceBase::getPowerConsumer(int index)
{
    return powerConsumers[index].powerConsumer;
}

int PowerSourceBase::addPowerConsumer(IPowerConsumer * powerConsumer)
{
    powerConsumers.push_back(PowerConsumerEntry(powerConsumer, 0));
    return powerConsumers.size() - 1;
}

void PowerSourceBase::removePowerConsumer(int powerConsumerId)
{
    consumedPower -= powerConsumers[powerConsumerId].consumedPower;
    powerConsumers.erase(powerConsumers.begin() + powerConsumerId);
}

void PowerSourceBase::setPowerConsumption(int powerConsumerId, double consumedPower)
{
    this->consumedPower += consumedPower - powerConsumers[powerConsumerId].consumedPower;
    powerConsumers[powerConsumerId].consumedPower = consumedPower;
}
