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

#include "ProgressBar.hpp"
#include "Thread/Debug.hpp"

#ifdef USE_GDI
#include <commctrl.h>
#else
#include "Screen/Canvas.hpp"
#endif

void
ProgressBarStyle::vertical()
{
#ifdef USE_GDI
  style |= PBS_VERTICAL;
#endif
}

void
ProgressBarStyle::smooth()
{
#ifdef USE_GDI
  style |= PBS_SMOOTH;
#endif
}

void
ProgressBar::set(ContainerWindow &parent,
                 PixelScalar left, PixelScalar top,
                 UPixelScalar width, UPixelScalar height,
                 const ProgressBarStyle style)
{
  Window::set(&parent,
#ifdef USE_GDI
              PROGRESS_CLASS, NULL,
#endif
              left, top, width, height,
              style);
}

void
ProgressBar::set_range(unsigned min_value, unsigned max_value)
{
  assert_none_locked();
  AssertThread();

#ifndef USE_GDI
  this->min_value = min_value;
  this->max_value = max_value;
  value = 0;
  step_size = 1;
  expose();
#else
  ::SendMessage(hWnd, PBM_SETRANGE, (WPARAM)0,
                (LPARAM)MAKELPARAM(min_value, max_value));
#endif
}

void
ProgressBar::set_position(unsigned value)
{
  assert_none_locked();
  AssertThread();

#ifndef USE_GDI
  this->value = value;
  expose();
#else
  ::SendMessage(hWnd, PBM_SETPOS, value, 0);
#endif
}

void
ProgressBar::set_step(unsigned size)
{
  assert_none_locked();
  AssertThread();

#ifndef USE_GDI
  step_size = size;
  expose();
#else
  ::SendMessage(hWnd, PBM_SETSTEP, (WPARAM)size, (LPARAM)0);
#endif
}

void
ProgressBar::step()
{
  assert_none_locked();
  AssertThread();

#ifndef USE_GDI
  value += step_size;
  expose();
#else
  ::SendMessage(hWnd, PBM_STEPIT, (WPARAM)0, (LPARAM)0);
#endif
}

#ifndef USE_GDI
void
ProgressBar::OnPaint(Canvas &canvas)
{
  unsigned position = 0;
  if (min_value < max_value) {
    unsigned value = this->value;
    if (value < min_value)
      value = min_value;
    else if (value > max_value)
      value = max_value;
#ifdef EYE_CANDY
    position = (value - min_value) * (get_width() - get_height()) /
               (max_value - min_value);
#else
    position = (value - min_value) * get_width() / (max_value - min_value);
#endif
  }

#ifdef EYE_CANDY
  unsigned margin = get_height() / 9;

  canvas.SelectNullPen();
  canvas.SelectWhiteBrush();
  canvas.DrawRoundRectangle(0, 0, get_width(), get_height(),
                            get_height(), get_height());

  Brush progress_brush(COLOR_XCSOAR_LIGHT);
  canvas.Select(progress_brush);
  canvas.DrawRoundRectangle(margin, margin, margin + position,
                            get_height() - margin, get_height(), get_height());
#else
  canvas.DrawFilledRectangle(0, 0, position, get_height(), COLOR_GREEN);
  canvas.DrawFilledRectangle(position, 0, get_width(), get_height(), COLOR_WHITE);
#endif
}
#endif
