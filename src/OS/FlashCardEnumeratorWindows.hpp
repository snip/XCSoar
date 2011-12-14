/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_FLASH_CARD_ENUMERATOR_WINDOWS_HPP
#define XCSOAR_FLASH_CARD_ENUMERATOR_WINDOWS_HPP

#include <windows.h>

/**
 * This class will enumerate all removable drives of the Windows PC.
 */
class FlashCardEnumerator
{
  DWORD drive_mask;
  TCHAR drive;
  TCHAR root_path[MAX_PATH];

public:
  FlashCardEnumerator()
    :drive_mask(::GetLogicalDrives()), drive(_T('A'))
  {
    _tcscpy(root_path, _T("A:\\"));
  }

  /**
   * Returns the next removable drive or NULL if at the end
   * @return e.g. "E:\"
   */
  const TCHAR *Next() {
    if (drive_mask == 0 || drive > _T('Z'))
      return NULL;

    do {
      root_path[0] = drive;

      unsigned bit_index = drive - _T('A');
      bool drive_exists = (drive_mask & (1 << bit_index)) != 0;
      if (!drive_exists)
        continue;

      unsigned drive_type = ::GetDriveType(root_path);
      if (drive_type != DRIVE_REMOVABLE)
        continue;

      drive++;
      return root_path;

    } while (++drive <= _T('Z'));

    return NULL;
  }
};

#endif
