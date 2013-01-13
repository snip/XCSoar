/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Angle.hpp"
#include "ComboList.hpp"
#include "Util/NumberParser.hpp"
#include "Util/Macros.hpp"

#include <stdio.h>

static TCHAR buffer[16];

unsigned
AngleDataField::Import(int value)
{
  assert(value >= -int(MAX));
  if (value < 0)
    return value + MAX;

  return Import(unsigned(value));
}

void
AngleDataField::ModifyValue(unsigned _value)
{
  unsigned value2 = Import(_value);
  if (value2 == value)
    return;

  value = value2;
  Modified();
}

void
AngleDataField::ModifyValue(int _value)
{
  unsigned value2 = Import(_value);
  if (value2 == value)
    return;

  value = value2;
  Modified();
}

void
AngleDataField::ModifyValue(Angle _value)
{
  unsigned value2 = Import(_value);
  if (value2 == value)
    return;

  value = value2;
  Modified();
}

int
AngleDataField::GetAsInteger() const
{
  return GetIntegerValue();
}

const TCHAR *
AngleDataField::GetAsString() const
{
  _stprintf(buffer, _T("%u"), GetIntegerValue());
  return buffer;
}

const TCHAR *
AngleDataField::GetAsDisplayString() const
{
  _stprintf(buffer, _T("%u°"), GetIntegerValue());
  return buffer;
}

void
AngleDataField::SetAsInteger(int _value)
{
  ModifyValue(_value);
}

void
AngleDataField::SetAsString(const TCHAR *_value)
{
  ModifyValue(Angle::Degrees(ParseDouble(_value, nullptr)));
}

void
AngleDataField::Inc()
{
  ModifyValue(value + step);
}

void
AngleDataField::Dec()
{
  ModifyValue(MAX + value - step);
}
