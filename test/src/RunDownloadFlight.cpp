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

#include "Device/Driver.hpp"
#include "Device/Driver/FLARM/Device.hpp"
#include "Device/Register.hpp"
#include "Device/Parser.hpp"
#include "Device/device.hpp"
#include "Engine/Navigation/GeoPoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "OS/PathName.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "Profile/DeviceConfig.hpp"
#include "Device/Port/DumpPort.hpp"
#include "LocalPath.hpp"
#include "Formatter/IGCFilenameFormatter.hpp"

#ifdef HAVE_POSIX
#include "Device/Port/TTYPort.hpp"
#else
#include "Device/Port/SerialPort.hpp"
#endif

#include <stdio.h>

bool
NMEAParser::ReadGeoPoint(NMEAInputLine &line, GeoPoint &value_r)
{
  return false;
}

static void
PrintFlightList(const RecordedFlightList &flight_list)
{
  for (auto i = flight_list.begin(); i != flight_list.end(); ++i) {
    const RecordedFlightInfo &flight = *i;
    printf("%04u/%02u/%02u %02u:%02u-%02u:%02u\n",
           flight.date.year, flight.date.month, flight.date.day,
           flight.start_time.hour, flight.start_time.minute,
           flight.end_time.hour, flight.end_time.minute);
  }
}

int main(int argc, char **argv)
{
  if (argc < 5) {
    fprintf(stderr, "Usage: %s DRIVER PORT BAUD FILE.igc [FLIGHT NR]\n"
            "Where DRIVER is one of:\n", argv[0]);

    const struct DeviceRegister *driver;
    for (unsigned i = 0; (driver = GetDriverByIndex(i)) != NULL; ++i)
      if (driver->IsLogger())
        _ftprintf(stderr, _T("\t%s\n"), driver->name);

    return EXIT_FAILURE;
  }

  PathName driver_name(argv[1]);
  PathName port_name(argv[2]);
  PathName path(argv[4]);

  DeviceConfig config;
  config.Clear();
  config.baud_rate = atoi(argv[3]);

#ifdef HAVE_POSIX
  TTYPort port(port_name, config.baud_rate, *(Port::Handler *)NULL);
#else
  SerialPort port(port_name, config.baud_rate, *(Port::Handler *)NULL);
#endif
  if (!port.Open()) {
    fprintf(stderr, "Failed to open COM port\n");
    return EXIT_FAILURE;
  }

  InitialiseDataPath();
  DumpPort dump_port(port);

  ConsoleOperationEnvironment env;
  const struct DeviceRegister *driver = FindDriverByName(driver_name);
  if (driver == NULL) {
    fprintf(stderr, "No such driver: %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  if (!driver->IsLogger()) {
    fprintf(stderr, "Not a logger driver: %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  assert(driver->CreateOnPort != NULL);
  Device *device = driver->CreateOnPort(config, dump_port);
  assert(device != NULL);

  if (!device->Open(env)) {
    delete device;
    fprintf(stderr, "Failed to open driver: %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  if (!device->EnableDownloadMode()) {
    delete device;
    fprintf(stderr, "Failed to enable download mode\n");
    return EXIT_FAILURE;
  }

  RecordedFlightList flight_list;
  if (!device->ReadFlightList(flight_list, env)) {
    device->DisableDownloadMode();
    delete device;
    fprintf(stderr, "Failed to download flight list\n");
    return EXIT_FAILURE;
  }

  if (flight_list.empty()) {
    device->DisableDownloadMode();
    delete device;
    fprintf(stderr, "Logger is empty\n");
    return EXIT_FAILURE;
  }

  PrintFlightList(flight_list);

  for (unsigned flight_id = 0; flight_id < flight_list.size(); flight_id++) {
    if (!device->DownloadFlight(flight_list[flight_id], path, env)) {
      device->DisableDownloadMode();
      delete device;
      fprintf(stderr, "Failed to download flight %u/%u\n",
              flight_id, flight_list.size());
      return EXIT_FAILURE;
    }
  }

  device->DisableDownloadMode();
  delete device;

  printf("Flight downloaded successfully\n");

  return EXIT_SUCCESS;
}
