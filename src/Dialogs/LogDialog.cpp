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

#include "Dialogs/LogDialog.hpp"
#include "Dialogs/Dialogs.h"
#include "Look/DialogLook.hpp"
#include "Form/Form.hpp"
#include "Form/List.hpp"
#include "Form/ButtonPanel.hpp"
#include "Screen/Layout.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/AnyCanvas.hpp"
#include "Language/Language.hpp"
#include "Log/Log.hpp"
#include "Log/Message.hpp"
#include "Log/TimedMessage.hpp"
#include "Log/TakeoffMessage.hpp"
#include "Log/LandingMessage.hpp"
#include "Units/UnitsFormatter.hpp"
#include "LocalTime.hpp"

#include <tchar.h>

static WndForm *dialog;
static WndListFrame *list;

static unsigned margin;
static unsigned time_column_width;

static void
RefreshList()
{
  list->SetLength(Log::Size());
  list->invalidate();
}

static void
PaintTime(Canvas &canvas, const PixelRect rc, const Log::TimedMessage &message)
{
  TCHAR buffer[100];
  Units::TimeToTextHHMMSigned(buffer, TimeLocal(message.time));
  canvas.text(rc.left + margin, rc.top + margin, buffer);
}

static void
PaintTakeoff(Canvas &canvas, const PixelRect rc,
             const Log::TakeoffMessage &message)
{
  canvas.text(rc.left + time_column_width + margin * 3, rc.top + margin,
              _("Takeoff"));
}

static void
PaintLanding(Canvas &canvas, const PixelRect rc,
             const Log::LandingMessage &message)
{
  unsigned width = canvas.text_width(_("Landing"));
  canvas.text(rc.left + time_column_width + margin * 3, rc.top + margin,
              _("Landing"));

  canvas.set_text_color(COLOR_GRAY);

  TCHAR buffer[100];
  _tcscpy(buffer, _("Time of flight: "));
  Units::TimeToTextHHMMSigned(buffer + _tcslen(buffer), message.flight_time);
  canvas.text(rc.left + time_column_width + margin * 5 + width, rc.top + margin,
              buffer);
}

static void
PaintMessage(Canvas &canvas, const PixelRect rc, unsigned i)
{
  canvas.set_text_color(COLOR_BLACK);

  const Log::Message &message = Log::GetMessage(i);

  switch (message.type) {
  case Log::Message::LANDING:
    PaintTime(canvas, rc, (const Log::TimedMessage &)message);
    PaintLanding(canvas, rc, (const Log::LandingMessage &)message);
    break;
  case Log::Message::TAKEOFF:
    PaintTime(canvas, rc, (const Log::TimedMessage &)message);
    PaintTakeoff(canvas, rc, (const Log::TakeoffMessage &)message);
    break;
  default:
    break;
  }
}

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  dialog->SetModalResult(mrOK);
}

void
ShowLog(SingleWindow &parent, const DialogLook &look)
{
  margin = Layout::Scale(5);

  /* create the dialog */

  WindowStyle dialog_style;
  dialog_style.hide();
  dialog_style.control_parent();

  PixelSize size = parent.get_size();
  dialog = new WndForm(parent, look, 0, 0, size.cx, size.cy,
                       _("Log"), dialog_style);

  ContainerWindow &client_area = dialog->GetClientAreaWindow();

  ButtonPanel buttons(client_area, look);
  buttons.Add(_("Close"), OnCloseClicked);

  const PixelRect rc = buttons.GetRemainingRect();

  /* create the list */

  unsigned font_height = look.list.font->get_height();

  AnyCanvas canvas;
  canvas.select(*look.list.font);
  time_column_width = canvas.text_size(_T("22:55")).cx;

  WindowStyle list_style;
  list_style.tab_stop();
  list_style.border();

  list = new WndListFrame(client_area, look,
                          rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                          list_style, 2 * margin + font_height);
  list->SetPaintItemCallback(PaintMessage);

  RefreshList();

  /* run it */

  dialog->ShowModal();

  delete list;
  delete dialog;
}
