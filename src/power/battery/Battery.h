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

#ifndef __INET_BATTERY_H
#define __INET_BATTERY_H

#include "INETDefs.h"
#include "PowerSourceBase.h"

/**
 * This class implements a battery.
 *
 * @author Levente Meszaros
 */
class INET_API Battery : public PowerSourceBase
{
  private:
    double voltage;
    double nominalEnergyCapacity;
    double residualEnergyCapacity;

  public:
    Battery() : voltage(0), nominalEnergyCapacity(0), residualEnergyCapacity(0) { }

    virtual double getVoltage() { return voltage; }

    virtual double getNominalEnergyCapacity() { return nominalEnergyCapacity; }

    virtual double getResidualEnergyCapacity() { return residualEnergyCapacity; }

    virtual void setPowerConsumption(int powerConsumerId, double consumedPower);

    virtual void consumeEnergy(int powerConsumerId, double consumedEnergy);

  protected:
    void initialize(int stage);
};

#endif
