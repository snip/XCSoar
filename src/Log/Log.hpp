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

#ifndef XCSOAR_LOG_HPP
#define XCSOAR_LOG_HPP

#include "Thread/Mutex.hpp"

#include <vector>

namespace Log
{
  class Message;

  typedef std::vector<Message*> MessageVector;
  typedef MessageVector::const_iterator const_iterator;

  /** Adds the given message to the message log */
  void Add(Message *message);

  /**
   * Frees the memory occupied by the message log.
   * The message log can not be used anymore afterwards.
   */
  void Destroy();

  /** Returns whether the log is empty */
  bool IsEmpty();

  /** Returns the number of messages in the log */
  unsigned Size();

  /** Returns the message with the given index */
  const Message &GetMessage(unsigned index);

  class ProtectedList: public ScopeLock
  {
    ProtectedList();

    const_iterator begin() const;
    const_iterator end() const;
  };
}

#endif
