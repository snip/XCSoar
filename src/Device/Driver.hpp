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

#ifndef XCSOAR_DEVICE_DRIVER_HPP
#define XCSOAR_DEVICE_DRIVER_HPP

#include <tchar.h>

struct DeviceConfig;
class Port;
class Device;

/**
 * This is the structure exported by a device driver.
 */
struct DeviceRegister {
  enum {
    /**
     * Makes XCSoar forward all NMEA input to this device.  This is
     * only used by the "NmeaOut" driver.
     */
    NMEA_OUT = 0x1,

    /**
     * Does this driver support task declaration with
     * Device::Declare()?
     */
    DECLARE = 0x2,

    /**
     * Does this device store flight logs which can be downloaded?
     * See Device::ReadFlightList(), Device::DownloadFlight().
     */
    LOGGER = 0x4,

    /**
     * Does this driver support switching to a "bulk" baud rate?
     */
    BULK_BAUD_RATE = 0x8,

    /**
     * Does this driver support additional configuration in form
     * of a "Manage" dialog?
     */
    MANAGE = 0x10,

    /**
     * Shall timeout and auto-restart be disabled for this driver?
     * This flag should be set for devices that are not expected to
     * send data every second.
     */
    NO_TIMEOUT = 0x20,

    /**
     * Is this device sending GPS data in binary form? The line-based
     * handler/parser will be disabled in this case.
     */
    RAW_GPS_DATA = 0x40,

    /**
     * Is this driver able to receive settings like MC value,
     * bugs or ballast from the device?
     */
    RECEIVE_SETTINGS = 0x80,

    /**
     * Is this driver able to send settings like MC value,
     * bugs or ballast to the device?
     */
    SEND_SETTINGS = 0x100,

    /**
     * Is this driver capable of passing through communication to
     * another device behind it?  This indicates that
     * EnablePassThrough() is implemented.
     */
    PASS_THROUGH = 0x200,
  };

  /**
   * The internal name of the driver, i.e. the one that is stored in
   * the profile.
   */
  const TCHAR *name;

  /**
   * The human-readable name of this driver.
   */
  const TCHAR *display_name;

  /**
   * A bit set describing the features of this driver.
   */
  unsigned int flags;

  /**
   * Create an instance of this driver for the given NMEA port.
   */
  Device *(*CreateOnPort)(const DeviceConfig &config, Port &com_port);

  /**
   * Is this driver able to receive settings like MC value,
   * bugs or ballast from the device?
   */
  bool CanReceiveSettings() const {
    return (flags & RECEIVE_SETTINGS) != 0;
  }

  /**
   * Is this driver able to send settings like MC value,
   * bugs or ballast to the device?
   */
  bool CanSendSettings() const {
    return (flags & SEND_SETTINGS) != 0;
  }

  /**
   * Is this the NMEA out driver?
   */
  bool IsNMEAOut() const {
    return (flags & NMEA_OUT) != 0;
  }

  /**
   * Does this driver support task declaration with Device::Declare()?
   */
  bool CanDeclare() const {
    return (flags & DECLARE) != 0;
  }

  /**
   * Does this device store flight logs which can be downloaded?
   * See Device::ReadFlightList(), Device::DownloadFlight().
   */
  bool IsLogger() const {
    return (flags & LOGGER) != 0;
  }

  /**
   * Does this device support additional configuration in form
   * of a "Manage" dialog?
   */
  bool IsManageable() const {
    return (flags & MANAGE) != 0;
  }

  /**
   * Does this driver support switching to a "bulk" baud rate?
   */
  bool SupportsBulkBaudRate() const {
    return (flags & BULK_BAUD_RATE) != 0;
  }

  /**
   * Shall devices be restarted automatically when they time out?
   */
  bool HasTimeout() const {
    return (flags & NO_TIMEOUT) == 0;
  }

  /**
   * Is this device sending GPS data in binary form? The line-based
   * handler/parser will be disabled in this case.
   */
  bool UsesRawData() const {
    return (flags & RAW_GPS_DATA) != 0;
  }

  /**
   * Does this driver implement EnablePassThrough()?
   */
  bool HasPassThrough() const {
    return (flags & PASS_THROUGH) != 0;
  }
};

#endif
