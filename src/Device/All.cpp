/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Device/All.hpp"
#include "Device/List.hpp"
#include "Device/Descriptor.hpp"
#include "Device/Devices.hpp"
#include "Thread/Mutex.hpp"
#include "../Simulator.hpp"

#include <assert.h>

void
Devices::Tick(const DerivedInfo &calculated)
{
  int i;

  for (i = 0; i < NUMDEV; i++) {
    DeviceDescriptor &device = *device_list[i];
    device.OnSysTicker(calculated);
  }
}

void
Devices::AutoReopen(OperationEnvironment &env)
{
  for (unsigned i = 0; i < NUMDEV; i++) {
    DeviceDescriptor &d = *device_list[i];
    d.AutoReopen(env);
  }
}

void
Devices::PutMacCready(fixed mac_cready, OperationEnvironment &env)
{
  if (is_simulator())
    return;

  for (unsigned i = 0; i < NUMDEV; ++i)
    device_list[i]->PutMacCready(mac_cready, env);
}

void
Devices::PutBugs(fixed bugs, OperationEnvironment &env)
{
  if (is_simulator())
    return;

  for (unsigned i = 0; i < NUMDEV; ++i)
    device_list[i]->PutBugs(bugs, env);
}

void
Devices::PutBallast(fixed fraction, fixed overload,
                     OperationEnvironment &env)
{
  if (is_simulator())
    return;

  for (unsigned i = 0; i < NUMDEV; ++i)
    device_list[i]->PutBallast(fraction, overload, env);
}

void
Devices::PutVolume(unsigned volume, OperationEnvironment &env)
{
  if (is_simulator())
    return;

  for (unsigned i = 0; i < NUMDEV; ++i)
    device_list[i]->PutVolume(volume, env);
}

void
Devices::PutActiveFrequency(RadioFrequency frequency,
                             OperationEnvironment &env)
{
  if (is_simulator())
    return;

  for (unsigned i = 0; i < NUMDEV; ++i)
    device_list[i]->PutActiveFrequency(frequency, env);
}

void
Devices::PutStandbyFrequency(RadioFrequency frequency,
                              OperationEnvironment &env)
{
  if (is_simulator())
    return;

  for (unsigned i = 0; i < NUMDEV; ++i)
    device_list[i]->PutStandbyFrequency(frequency, env);
}

void
Devices::PutQNH(const AtmosphericPressure &pres,
                 OperationEnvironment &env)
{
  if (is_simulator())
    return;

  for (unsigned i = 0; i < NUMDEV; ++i)
    device_list[i]->PutQNH(pres, env);
}

void
Devices::NotifySensorUpdate(const MoreData &basic)
{
  if (is_simulator())
    return;

  for (unsigned i = 0; i < NUMDEV; ++i)
    device_list[i]->OnSensorUpdate(basic);
}
