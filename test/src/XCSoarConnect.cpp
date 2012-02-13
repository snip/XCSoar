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

#include "UIGlobals.hpp"
#include "Form/Form.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Init.hpp"
#include "Look/DialogLook.hpp"
#include "Fonts.hpp"
#include "Device/Register.hpp"
#include "Device/Driver.hpp"
#include "Device/Descriptor.hpp"
#include "Device/Parser.hpp"
#include "Profile/DeviceConfig.hpp"
#include "OS/PathName.hpp"
#include "Util/Args.hpp"
#include "Operation/VerboseOperationEnvironment.hpp"

#ifdef HAVE_POSIX
#include "Device/Port/TTYPort.hpp"
#else
#include "Device/Port/SerialPort.hpp"
#endif

static SingleWindow *main_window;
static DialogLook *dialog_look;

SingleWindow &
UIGlobals::GetMainWindow() {
  return *main_window;
}

const DialogLook &
UIGlobals::GetDialogLook() {
  return *dialog_look;
}

bool
NMEAParser::ReadGeoPoint(NMEAInputLine &line, GeoPoint &value_r)
{
  return false;
}

#ifndef WIN32
int main(int argc, char **argv)
#else
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
#ifdef _WIN32_WCE
        LPWSTR lpCmdLine,
#else
        LPSTR lpCmdLine2,
#endif
        int nCmdShow)
#endif
{
  // Prepare the usage message
  NarrowString<1024> usage;
  usage.Format("DRIVER PORT BAUD\n\n"
               "Where DRIVER is one of:", argv[0]);
  {
    const DeviceRegister *driver;
    for (unsigned i = 0; (driver = GetDriverByIndex(i)) != NULL; ++i) {
      if (driver->IsLogger()) {
        NarrowPathName driver_name(driver->name);
        usage.AppendFormat("\n\t%s", driver_name);
      }
    }
  }

  // Parse the command line arguments
#ifndef WIN32
  Args args(argc, argv, usage);
#else
  Args args(GetCommandLine(), usage);
#endif

  PathName driver_name(args.ExpectNext());
  PathName port_name(args.ExpectNext());

  DeviceDescriptor device;
  DeviceConfig &config = device.SetConfig();

  config.Clear();
  config.driver_name = driver_name;
  config.port_type = DeviceConfig::PortType::SERIAL;
  config.path = port_name;
  config.baud_rate = atoi(args.ExpectNext());

  args.ExpectEnd();


  // Open the port
  VerboseOperationEnvironment env;
  device.Open(env, true);

  /*
  if (!port.Open()) {
    fprintf(stderr, "Failed to open COM port\n");
    return EXIT_FAILURE;
  }

  const struct DeviceRegister *driver = FindDriverByName(driver_name);
  if (driver == NULL) {
    fprintf(stderr, "No such driver: %s\n", argv[1]);
    args.UsageError();
  }

  if (!driver->IsLogger()) {
    fprintf(stderr, "Not a logger driver: %s\n", argv[1]);
    args.UsageError();
  }
*/

  // Setup the main window
  PixelRect screen_rc{0, 0, 640, 480};

  ScreenGlobalInit screen_init;
  Layout::Initialize(screen_rc.right - screen_rc.left,
                     screen_rc.bottom - screen_rc.top);

  main_window = new SingleWindow();
  main_window->set(_T("STATIC"), _T("XCSoar Connect"), screen_rc);
  main_window->show();

  InitialiseFonts();

  dialog_look = new DialogLook();
  dialog_look->Initialise(bold_font, normal_font, small_font,
                          bold_font, bold_font);

  WndForm form(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
               UIGlobals::GetMainWindow().get_client_rect());

  WindowStyle style;
  style.TabStop();

  form.ShowModal();

  delete dialog_look;
  delete main_window;
  DeinitialiseFonts();

  return 0;
}
