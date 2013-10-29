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

#ifndef __INET_IPowerSource_H
#define __INET_IPowerSource_H

#include "INETDefs.h"
#include "IPowerConsumer.h"

/**
 * This interface provides an abstraction for different power sources.
 *
 * @author Levente Meszaros
 */
class INET_API IPowerSource
{
  public:
    /** @brief A signal used to publish power source state changes. */
    static simsignal_t PowerSourceStateChangedSignal;

  public:
    virtual ~IPowerSource() {}

    /**
     * Returns the number of power consumers.
     */
    virtual int getNumPowerConsumers() = 0;

    /**
     * Returns the power consumer for the provided index.
     */
    virtual IPowerConsumer * getPowerConsumer(int index) = 0;

    /**
     * Registers a new power consumer and returns its index.
     */
    virtual int addPowerConsumer(IPowerConsumer * powerConsumer) = 0;

    /**
     * Removes a previously registered power consumer with the provided index.
     */
    virtual void removePowerConsumer(int powerConsumerId) = 0;

    /**
     * Returns the regulated voltage.
     */
    virtual double getVoltage() = 0;

    /**
     * Returns the nominal energy capacity in the range [0, +infinity].
     */
    virtual double getNominalEnergyCapacity() = 0;

    /**
     * Returns the residual energy capacity in the range [0, +infinity].
     */
    virtual double getResidualEnergyCapacity() = 0;

    /**
     * Returns the current total power consumption.
     */
    virtual double getPowerConsumption() = 0;

    /**
     * Changes the consumed power for the provided power consumer.
     */
    virtual void setPowerConsumption(int powerConsumerId, double consumedPower) = 0;

    /**
     * Immediately consumes some energy for the provided power consumer.
     */
    virtual void consumeEnergy(int powerConsumerId, double consumedEnergy) = 0;
};

#endif
