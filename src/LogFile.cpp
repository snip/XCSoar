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

#include "LogFile.hpp"
#include "LocalPath.hpp"
#include "Asset.hpp"
#include "IO/TextWriter.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "OS/Clock.hpp"
#include "Util/StaticString.hpp"

#include <stdio.h>
#include <stdarg.h>
#include <windef.h> // for MAX_PATH

#ifdef ANDROID
#include <android/log.h>
#endif


void
LogStartUp(const TCHAR *Str, ...)
{
  static bool initialised = false;
  static TCHAR szFileName[MAX_PATH];

  if (!initialised) {
    LocalPath(szFileName, _T("xcsoar-startup.log"));
  }

  TCHAR buf[MAX_PATH];
  va_list ap;

  va_start(ap, Str);
  _vstprintf(buf, Str, ap);
  va_end(ap);

  TCHAR time_buffer[32];
  FormatTimeLong(time_buffer, fixed(MonotonicClockMS()) / 1000);

#ifdef ANDROID
  __android_log_print(ANDROID_LOG_INFO, "XCSoar", "%s", buf);
#endif

#if defined(HAVE_POSIX) && !defined(ANDROID) && !defined(NDEBUG)
  fprintf(stderr, "[%s] %s\n", time_buffer, buf);
#endif

  TextWriter writer(szFileName, initialised);
  if (writer.error())
    return;

  StaticString<MAX_PATH> output_buffer;
  output_buffer.Format(_T("[%s] %s"), time_buffer, buf);
  writer.writeln(output_buffer);

  if (!initialised)
    initialised = true;
}
