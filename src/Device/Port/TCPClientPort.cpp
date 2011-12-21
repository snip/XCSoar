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

#include "TCPClientPort.hpp"

#include <unistd.h>
#include <assert.h>
#include <tchar.h>
#include <stdio.h>
#include <stdint.h>

#ifdef HAVE_POSIX
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif

TCPClientPort::TCPClientPort(const char *_host, unsigned _port,
                             Handler &_handler)
  :Port(_handler), host(_host), port(_port), socket_fd(-1) {}

TCPClientPort::~TCPClientPort()
{
  Close();
}

bool
TCPClientPort::Open()
{
  assert(socket_fd < 0);

  // Convert IP address to binary form
  struct sockaddr_in server_addr;
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  if (server_addr.sin_addr.s_addr == INADDR_NONE)
    return false;

  // Create socket for the outgoing connection
  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0)
    return false;

  // Prepare connect() parameter
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  memset(&(server_addr.sin_zero), 0, 8);

  // Connect to the specified server
  if (connect(socket_fd, (struct sockaddr *)&server_addr,
              sizeof(struct sockaddr)) == 0)
    // Connection established
    return true;

  // Connection refused
  close(socket_fd);
  socket_fd = -1;
  return false;
}

void
TCPClientPort::Flush()
{
}

void
TCPClientPort::Run()
{
  char buffer[1024];

  while (!CheckStopped()) {
    if (socket_fd < 0)
      break;

    int ret = GetSocketReadStatus();
    if (ret > 0) {
      ssize_t nbytes = recv(socket_fd, buffer, sizeof(buffer), 0);
      if (nbytes <= 0) {
        close(socket_fd);
        socket_fd = -1;
        continue;
      }

      handler.DataReceived(buffer, nbytes);
    } else if (ret < 0) {
      close(socket_fd);
      socket_fd = -1;
    }
  }
}

bool
TCPClientPort::Close()
{
  if (socket_fd < 0)
    return true;

  StopRxThread();

  close(socket_fd);
  socket_fd = -1;
  return true;
}

size_t
TCPClientPort::Write(const void *data, size_t length)
{
  if (socket_fd < 0)
    return 0;

  ssize_t nbytes = send(socket_fd, (const char *)data, length, 0);
  return nbytes < 0 ? 0 : nbytes;
}

bool
TCPClientPort::StopRxThread()
{
  // Make sure the thread isn't terminating itself
  assert(!Thread::IsInside());

  // Make sure the port is still open
  if (socket_fd < 0)
    return false;

  // If the thread is not running, cancel the rest of the function
  if (!Thread::IsDefined())
    return true;

  BeginStop();

  Thread::Join();

  return true;
}

bool
TCPClientPort::StartRxThread(void)
{
  // Make sure the thread isn't starting itself
  assert(!Thread::IsInside());

  // Make sure the port was opened correctly
  if (socket_fd < 0)
    return false;

  // Start the receive thread
  StoppableThread::Start();
  return true;
}

bool
TCPClientPort::SetRxTimeout(unsigned Timeout)
{
  return true;
}

unsigned
TCPClientPort::GetBaudrate() const
{
  return 0;
}

unsigned
TCPClientPort::SetBaudrate(unsigned BaudRate)
{
  return 0;
}

int
TCPClientPort::Read(void *buffer, size_t length)
{
  // Make sure the socket is open
  if (socket_fd < 0)
    return -1;

  // Read socket status information
  if (GetSocketReadStatus() != 1)
    // Socket has no readable data
    return -1;

  // Read data from socket
  return read(socket_fd, buffer, length);
}


int
TCPClientPort::GetSocketReadStatus()
{
  // Make sure the socket is open
  assert(socket_fd >= 0);

  // Prepare select() socket parameter
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(socket_fd, &rfds);

  // Prepare timeout structure
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 250000;

  // Read socket status information
  return select(socket_fd + 1, &rfds, NULL, NULL, &timeout);
}

