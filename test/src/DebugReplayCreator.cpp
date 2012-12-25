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

#include "DebugReplay.hpp"
#include "DebugReplayIGC.hpp"
#include "DebugReplayNMEA.hpp"
#include "OS/Args.hpp"
#include "IO/FileLineReader.hpp"
#include "OS/PathName.hpp"
#include "Device/Register.hpp"

DebugReplay *
CreateDebugReplay(Args &args)
{
  if (!args.IsEmpty() && MatchesExtension(args.PeekNext(), ".igc")) {
    const char *input_file = args.ExpectNext();

    FileLineReaderA *reader = new FileLineReaderA(input_file);
    if (reader->error()) {
      delete reader;
      fprintf(stderr, "Failed to open %s\n", input_file);
      return NULL;
    }

    return new DebugReplayIGC(reader);
  }

  const tstring driver_name = args.ExpectNextT();

  const struct DeviceRegister *driver = FindDriverByName(driver_name.c_str());
  if (driver == NULL) {
    _ftprintf(stderr, _T("No such driver: %s\n"), driver_name.c_str());
    return NULL;
  }

  const char *input_file = args.ExpectNext();

  FileLineReaderA *reader = new FileLineReaderA(input_file);
  if (reader->error()) {
    delete reader;
    fprintf(stderr, "Failed to open %s\n", input_file);
    return NULL;
  }

  return new DebugReplayNMEA(reader, driver);
}
