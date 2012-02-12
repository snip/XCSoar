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

#include "Dialogs/XML.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "xmlParser.hpp"
#include "DataField/Boolean.hpp"
#include "DataField/Enum.hpp"
#include "DataField/FileReader.hpp"
#include "DataField/Float.hpp"
#include "DataField/Integer.hpp"
#include "DataField/String.hpp"
#include "DataField/Time.hpp"
#include "Screen/Layout.hpp"
#include "Screen/SingleWindow.hpp"
#include "Interface.hpp"
#include "Form/Form.hpp"
#include "Form/Frame.hpp"
#include "Form/Edit.hpp"
#include "Form/SymbolButton.hpp"
#include "Form/Draw.hpp"
#include "Form/List.hpp"
#include "Form/Tabbed.hpp"
#include "Form/TabBar.hpp"
#include "Form/TabMenu.hpp"
#include "Form/Panel.hpp"
#include "Form/Keyboard.hpp"
#include "Form/CheckBox.hpp"
#include "Form/DockWindow.hpp"
#include "StringUtil.hpp"
#include "ResourceLoader.hpp"
#include "Look/DialogLook.hpp"
#include "Inflate.hpp"

#include <zlib/zlib.h>

#include <stdio.h>    // for _stprintf
#include <assert.h>
#include <tchar.h>
#include <limits.h>

static const DialogLook *xml_dialog_look;

void
SetXMLDialogLook(const DialogLook &_dialog_look)
{
  xml_dialog_look = &_dialog_look;
}

// used when stretching dialog and components
static int dialog_width_scale = 1024;

struct ControlSize: public PixelSize
{
  bool no_scaling;
};

struct ControlPosition: public RasterPoint
{
  bool no_scaling;
};

/**
 * Callback type for the "Custom" element, attribute "OnCreate".
 */
typedef Window *(*CreateWindowCallback_t)(ContainerWindow &parent,
                                          PixelScalar left, PixelScalar top,
                                          UPixelScalar width,
                                          UPixelScalar height,
                                          const WindowStyle style);

static Window *
LoadChild(SubForm &form, ContainerWindow &parent,
          const CallBackTableEntry *lookup_table, XMLNode node,
          int bottom_most = 0,
          WindowStyle style=WindowStyle());

static void
LoadChildrenFromXML(SubForm &form, ContainerWindow &parent,
                    const CallBackTableEntry *lookup_table,
                    const XMLNode *node);

/**
 * Converts a String into an Integer and returns
 * the default value if String = NULL
 * @param String The String to parse
 * @param Default The default return value
 * @return The parsed Integer value
 */
static long
StringToIntDflt(const TCHAR *string, long _default)
{
  if (string == NULL || StringIsEmpty(string))
    return _default;
  return _tcstol(string, NULL, 0);
}

/**
 * Converts a String into a Float and returns
 * the default value if String = NULL
 * @param String The String to parse
 * @param Default The default return value
 * @return The parsed Float value
 */
static double
StringToFloatDflt(const TCHAR *string, double _default)
{
  if (string == NULL || StringIsEmpty(string))
    return _default;
  return _tcstod(string, NULL);
}

/**
 * Returns the default value if String = NULL
 * @param String The String to parse
 * @param Default The default return value
 * @return The output String
 */
static const TCHAR *
StringToStringDflt(const TCHAR *string, const TCHAR *_default)
{
  if (string == NULL || StringIsEmpty(string))
    return _default;
  return string;
}

/**
 * Converts a String into a Color and sets
 * a default value if String = NULL
 * @param String The String to parse
 * @param color The color (output)
 */
static bool
StringToColor(const TCHAR *string, Color &color)
{
  long value = StringToIntDflt(string, -1);
  if (value & ~0xffffff)
    return false;

  color = Color((value >> 16) & 0xff, (value >> 8) & 0xff, value & 0xff);
  return true;
}

static int 
ScaleWidth(const int x)
{
  const DialogStyle dialog_style = UIGlobals::GetDialogSettings().dialog_style;
  if (dialog_style == dsFullWidth || dialog_style == dsScaledBottom)
    // stretch width to fill screen horizontally
    return x * dialog_width_scale / 1024;
  else
    return x;
}

static const TCHAR*
GetName(const XMLNode &node)
{
  return StringToStringDflt(node.getAttribute(_T("Name")), _T(""));
}

static const TCHAR*
GetCaption(const XMLNode &node)
{
  const TCHAR* tmp =
      StringToStringDflt(node.getAttribute(_T("Caption")), _T(""));

  // don't translate empty strings, it would query gettext metadata
  if (tmp[0] == _T('\0'))
    return tmp;

  return gettext(tmp);
}

static ControlPosition
GetPosition(const XMLNode &node, const PixelRect rc, int bottom_most = -1)
{
  ControlPosition pt;

  // Calculate x- and y-Coordinate
  pt.x = StringToIntDflt(node.getAttribute(_T("X")), 0);
  pt.y = StringToIntDflt(node.getAttribute(_T("Y")), -1);
  pt.no_scaling = false;

  if (Layout::ScaleSupported()) {
    pt.x = Layout::Scale(pt.x);
    if (pt.y != -1)
      pt.y = Layout::Scale(pt.y);
  }

  if (pt.y == -1)
    pt.y = bottom_most;

  if (pt.x < -1) {
    pt.x += rc.right;
    pt.no_scaling = true;
  }
  if (pt.y < -1)
    pt.y += rc.bottom;

  // Move inside target rc (e.g. if parent != targetRect -> usually, rc.left == rc.top == 0, so no moving takes place).
  pt.x += rc.left;
  pt.y += rc.top;

  return pt;
}

static ControlPosition
SetPositionCentered(const ControlPosition original, const PixelRect rc,
                    const ControlSize size)
{
  ControlPosition pt = original;
  // center horizontally in parent PixelRect
  pt.x = (rc.right + rc.left - size.cx) / 2;
  return pt;
}

static ControlSize
GetSize(const XMLNode &node, const PixelRect rc, const RasterPoint &pos)
{
  ControlSize sz;

  // Calculate width and height
  sz.cx = StringToIntDflt(node.getAttribute(_T("Width")), 0);
  sz.cy = StringToIntDflt(node.getAttribute(_T("Height")), 0);
  sz.no_scaling = false;

  if (Layout::ScaleSupported()) {
    sz.cx = Layout::Scale(sz.cx);
    sz.cy = Layout::Scale(sz.cy);
  }

  if (sz.cx <= 0) {
    sz.cx += rc.right - pos.x;
    sz.no_scaling = true;
  }
  if (sz.cy <= 0)
    sz.cy += rc.bottom - pos.y;

  assert(sz.cx > 0);
  assert(sz.cy > 0);

  return sz;
}

static void *
CallBackLookup(const CallBackTableEntry *lookup_table, const TCHAR *name)
{
  assert(name != NULL);
  assert(!StringIsEmpty(name));
  assert(lookup_table != NULL);

  for (const CallBackTableEntry *p = lookup_table;; ++p) {
    assert(p->name != NULL);
    assert(p->callback != NULL);

    if (StringIsEqual(p->name, name))
      return p->callback;
  }
}

static void *
GetCallBack(const CallBackTableEntry *lookup_table,
            const XMLNode &node, const TCHAR* attribute)
{
  const TCHAR *name = node.getAttribute(attribute);
  if (name == NULL)
    return NULL;

  assert(!StringIsEmpty(name));

  return CallBackLookup(lookup_table, name);
}

static XMLNode *
LoadXMLFromResource(const TCHAR* resource, XMLResults *xml_results)
{
  ResourceLoader::Data data = ResourceLoader::Load(resource, _T("XMLDialog"));
  assert(data.first != NULL);

  char *buffer = InflateToString(data.first, data.second);

#ifdef _UNICODE
  int length = strlen(buffer);
  TCHAR *buffer2 = new TCHAR[length + 1];
  length = MultiByteToWideChar(CP_UTF8, 0, buffer, length,
                               buffer2, length);
  buffer2[length] = _T('\0');
  delete[] buffer;
#else
  const char *buffer2 = buffer;
#endif

  XMLNode *x = XMLNode::parseString(buffer2, xml_results);

#ifdef _UNICODE
  delete[] buffer2;
#else
  delete[] buffer;
#endif

  return x;
}

/**
 * Tries to load an XML file from the resources
 * @param lpszXML The resource name
 * @return The parsed XMLNode
 */
static XMLNode *
LoadXMLFromResource(const TCHAR *resource)
{
  XMLResults xml_results;

  // Reset errors
  xml_results.error = eXMLErrorNone;
  XMLNode::GlobalError = false;

  // Load and parse the resource
  XMLNode *node = LoadXMLFromResource(resource, &xml_results);

  // Show errors if they exist
  assert(xml_results.error == eXMLErrorNone);

  return node;
}

static void
InitScaleWidth(const PixelSize size, const PixelRect rc)
{
  // No need to calculate the scale factor on platforms that don't scale
  if (!Layout::ScaleSupported())
    return;

  const DialogStyle dialog_style = UIGlobals::GetDialogSettings().dialog_style;
  if (dialog_style == dsFullWidth || dialog_style == dsScaledBottom)
    dialog_width_scale = (rc.right - rc.left) * 1024 / size.cx;
  else
    dialog_width_scale = 1024;
}

/**
 * Loads a stand-alone XML file as a single top-level XML node
 * into an existing SubForm object and sets its parent to the parent parameter
 * Ignores additional top-level XML nodes.
 * Scales based on the DialogStyle of the last XML form loaded by XCSoar.
 * The Window is destroyed by its Form's destructor
 *
 * @param LookUpTable The CallBackTable
 * @param form The WndForm into which the Window is added
 * @param parent The parent window of the control being created
 *    set parent to "form-get_client_rect()" to make top level control
 *    or to a PanelControl to add it to a tab window
 * @param FileName The XML filename
 * @return the pointer to the Window added to the form
 */
Window *
LoadWindow(const CallBackTableEntry *lookup_table, SubForm *form,
           ContainerWindow &parent, const TCHAR *resource,
           WindowStyle style)
{
  if (!form)
    return NULL;

  XMLNode *node = LoadXMLFromResource(resource);
  assert(node != NULL);

  // load only one top-level control.
  Window *window = LoadChild(*form, parent, lookup_table, *node, 0, style);
  delete node;

  assert(!XMLNode::GlobalError);

  return window;
}

/**
 * This function returns a WndForm created either from the ressources or
 * from the XML file in XCSoarData(if found)
 * @param LookUpTable The CallBackTable
 * @param FileName The XML filename to search for in XCSoarData
 * @param Parent The parent window (e.g. XCSoarInterface::main_window)
 * @param resource The resource to look for
 * @param targetRect The area where to move the dialog if not parent
 * @return The WndForm object
 */
WndForm *
LoadDialog(const CallBackTableEntry *lookup_table, SingleWindow &parent,
           const TCHAR *resource, const PixelRect *target_rc)
{
  WndForm *form = NULL;

  // Find XML file or resource and load XML data out of it
  XMLNode *node = LoadXMLFromResource(resource);

  // TODO code: put in error checking here and get rid of exits in xmlParser
  // If XML error occurred -> Error messagebox + cancel
  assert(node != NULL);

  // If the main XMLNode is of type "Form"
  assert(StringIsEqual(node->getName(), _T("Form")));

  // Determine the dialog size
  const TCHAR* caption = GetCaption(*node);
  const PixelRect rc = target_rc ? *target_rc : parent.get_client_rect();
  ControlPosition pos = GetPosition(*node, rc, 0);
  ControlSize size = GetSize(*node, rc, pos);

  InitScaleWidth(size, rc);

  // Correct dialog size and position for dialog style
  switch (UIGlobals::GetDialogSettings().dialog_style) {
  case dsFullWidth:
    pos.x = rc.left;
    pos.y = rc.top;
    size.cx = rc.right - rc.left; // stretch form to full width of screen
    size.cy = rc.bottom - rc.top;
    break;
  case dsScaledCentered:
    pos = SetPositionCentered(pos, rc, size);
    break;

  case dsScaled:
  case dsFixed:
    break;

  case dsScaledBottom:
    size.cx = rc.right - rc.left; // stretch form to full width of screen
    pos.y = rc.bottom - size.cy; // position at bottom of screen
    break;
  }

  // Create the dialog
  WindowStyle style;
  style.Hide();
  style.ControlParent();

  PixelRect form_rc;
  form_rc.left = pos.x;
  form_rc.top = pos.y;
  form_rc.right = form_rc.left + size.cx;
  form_rc.bottom = form_rc.top + size.cy;

  form = new WndForm(parent, *xml_dialog_look, form_rc, caption, style);

  // Load the children controls
  LoadChildrenFromXML(*form, form->GetClientAreaWindow(),
                      lookup_table, node);
  delete node;

  // If XML error occurred -> Error messagebox + cancel
  assert(!XMLNode::GlobalError);

  // Return the created form
  return form;
}

static DataField *
LoadDataField(const XMLNode &node, const CallBackTableEntry *LookUpTable)
{
  TCHAR data_type[32];
  TCHAR display_format[32];
  TCHAR edit_format[32];
  double step;
  bool fine;

  _tcscpy(data_type,
          StringToStringDflt(node.getAttribute(_T("DataType")), _T("")));
  _tcscpy(display_format,
          StringToStringDflt(node. getAttribute(_T("DisplayFormat")), _T("")));
  _tcscpy(edit_format,
          StringToStringDflt(node.getAttribute(_T("EditFormat")), _T("")));

  fixed min = fixed(StringToFloatDflt(node.getAttribute(_T("Min")), INT_MIN));
  fixed max = fixed(StringToFloatDflt(node.getAttribute(_T("Max")), INT_MAX));
  step = StringToFloatDflt(node.getAttribute(_T("Step")), 1);
  fine = StringToIntDflt(node.getAttribute(_T("Fine")), false);

  DataField::DataAccessCallback_t callback = (DataField::DataAccessCallback_t)
    GetCallBack(LookUpTable, node, _T("OnDataAccess"));

  if (_tcsicmp(data_type, _T("enum")) == 0)
    return new DataFieldEnum(callback);

  if (_tcsicmp(data_type, _T("filereader")) == 0)
    return new DataFieldFileReader(callback);

  if (_tcsicmp(data_type, _T("boolean")) == 0)
    return new DataFieldBoolean(false, _("On"), _("Off"), callback);

  if (_tcsicmp(data_type, _T("double")) == 0)
    return new DataFieldFloat(edit_format, display_format, min, max,
                              fixed_zero, fixed(step), fine, callback);

  if (_tcsicmp(data_type, _T("time")) == 0) {
    DataFieldTime *df = new DataFieldTime((int)min, (int)max, 0,
                                          (unsigned)step, callback);
    unsigned max_token = StringToIntDflt(node.getAttribute(_T("MaxTokens")), 2);
    df->SetMaxTokenNumber(max_token);
    return df;
  }

  if (_tcsicmp(data_type, _T("integer")) == 0)
    return new DataFieldInteger(edit_format, display_format, (int)min, (int)max,
                                0, (int)step, callback);

  if (_tcsicmp(data_type, _T("string")) == 0)
    return new DataFieldString(_T(""), callback);

  return NULL;
}

/**
 * Creates a control from the given XMLNode as a child of the given
 * parent.
 *
 * @param form the SubForm object
 * @param LookUpTable The parent CallBackTable
 * @param node The XMLNode that represents the control
 */
static Window *
LoadChild(SubForm &form, ContainerWindow &parent,
          const CallBackTableEntry *lookup_table, XMLNode node,
          int bottom_most,
          WindowStyle style)
{
  Window *window = NULL;

  // Determine name, coordinates, width, height
  // and caption of the control
  const TCHAR* name = GetName(node);
  const TCHAR* caption = GetCaption(node);
  PixelRect rc = parent.get_client_rect();
  ControlPosition pos = GetPosition(node, rc, bottom_most);
  if (!pos.no_scaling)
    pos.x = ScaleWidth(pos.x);

  ControlSize size = GetSize(node, rc, pos);
  if (!size.no_scaling)
    size.cx = ScaleWidth(size.cx);

  if (!StringToIntDflt(node.getAttribute(_T("Visible")), 1))
    style.Hide();

  if (StringToIntDflt(node.getAttribute(_T("Border")), 0))
    style.Border();

  rc.left = pos.x;
  rc.top = pos.y;
  rc.right = rc.left + size.cx;
  rc.bottom = rc.top + size.cy;

  bool expert = (StringToIntDflt(node.getAttribute(_T("Expert")), 0) == 1);

  // PropertyControl (WndProperty)
  if (StringIsEqual(node.getName(), _T("Edit"))) {
    // Determine the width of the caption field
    int caption_width = StringToIntDflt(node.getAttribute(_T("CaptionWidth")), 0);

    if (Layout::ScaleSupported())
      caption_width = Layout::Scale(caption_width);

    caption_width = ScaleWidth(caption_width);

    // Determine the padding of the caption field
    unsigned caption_padding = Layout::Scale(StringToIntDflt(
        node.getAttribute(_T("CaptionPadding")), 3));

    // Determine whether the control is multiline or readonly
    bool multi_line = StringToIntDflt(node.getAttribute(_T("MultiLine")), 0);
    bool read_only = StringToIntDflt(node.getAttribute(_T("ReadOnly")), 0);

    // Load the event callback properties
    WndProperty::DataChangeCallback_t data_notify_callback =
      (WndProperty::DataChangeCallback_t)
      GetCallBack(lookup_table, node, _T("OnDataNotify"));

    WindowControl::OnHelpCallback_t help_callback =
      (WindowControl::OnHelpCallback_t)
      GetCallBack(lookup_table, node, _T("OnHelp"));

    // Create the Property Control
    style.ControlParent();

    EditWindowStyle edit_style;
    edit_style.vertical_center();
    if (read_only)
      edit_style.read_only();
    else
      edit_style.TabStop();

    if (IsEmbedded() || Layout::scale_1024 < 2048)
      /* sunken edge doesn't fit well on the tiny screen of an
         embedded device */
      edit_style.Border();
    else
      edit_style.SunkenEdge();

    if (multi_line) {
      edit_style.multiline();
      edit_style.VerticalScroll();
    }

    WndProperty *property;
    window = property = new WndProperty(parent, *xml_dialog_look, caption, rc,
                                        caption_width, caption_padding, style,
                                        edit_style, data_notify_callback);

    // Set the help function event callback
    property->SetOnHelpCallback(help_callback);

    // Load the help text
    property->SetHelpText(StringToStringDflt(node.getAttribute(_T("Help")),
                                             NULL));

    // If the control has (at least) one DataField child control
    const XMLNode *data_field_node = node.getChildNode(_T("DataField"));
    if (data_field_node != NULL) {
      // -> Load the first DataField control
      DataField *data_field =
        LoadDataField(*data_field_node, lookup_table);

      if (data_field != NULL)
        // Tell the Property control about the DataField control
        property->SetDataField(data_field);
    }

  } else if (StringIsEqual(node.getName(), _T("TextEdit"))) {
    // Determine whether the control is multiline or readonly
    bool multi_line = StringToIntDflt(node.getAttribute(_T("MultiLine")), 0);
    bool read_only = StringToIntDflt(node.getAttribute(_T("ReadOnly")), 0);

    EditWindowStyle edit_style(style);
    if (read_only)
      edit_style.read_only();
    else
      edit_style.TabStop();

    if (IsEmbedded() || Layout::scale_1024 < 2048)
      /* sunken edge doesn't fit well on the tiny screen of an
         embedded device */
      edit_style.Border();
    else
      edit_style.SunkenEdge();

    if (multi_line) {
      edit_style.multiline();
      edit_style.VerticalScroll();
    }

    EditWindow *edit;
    window = edit = new EditWindow();
    edit->set(parent, pos.x, pos.y, size.cx, size.cy, edit_style);
    edit->InstallWndProc();
    edit->set_font(*xml_dialog_look->text_font);

  // ButtonControl (WndButton)
  } else if (StringIsEqual(node.getName(), _T("Button"))) {
    // Determine ClickCallback function
    WndButton::ClickNotifyCallback click_callback =
      (WndButton::ClickNotifyCallback)
      GetCallBack(lookup_table, node, _T("OnClick"));

    // Create the ButtonControl

    ButtonWindowStyle button_style(style);
    button_style.TabStop();
    button_style.multiline();

    window = new WndButton(parent, *xml_dialog_look, caption,
                           rc,
                           button_style, click_callback);

  } else if (StringIsEqual(node.getName(), _T("CheckBox"))) {
    // Determine click_callback function
    CheckBoxControl::ClickNotifyCallback click_callback =
      (CheckBoxControl::ClickNotifyCallback)
      GetCallBack(lookup_table, node, _T("OnClick"));

    // Create the CheckBoxControl

    style.TabStop();

    window = new CheckBoxControl(parent, *xml_dialog_look, caption,
                                 rc,
                                 style, click_callback);

  // SymbolButtonControl (WndSymbolButton) not used yet
  } else if (StringIsEqual(node.getName(), _T("SymbolButton"))) {
    // Determine ClickCallback function
    WndButton::ClickNotifyCallback click_callback =
      (WndButton::ClickNotifyCallback)
      GetCallBack(lookup_table, node, _T("OnClick"));

    // Create the SymbolButtonControl

    style.TabStop();

    window = new WndSymbolButton(parent, *xml_dialog_look, caption,
                                 rc,
                                 style, click_callback);

  // PanelControl (WndPanel)
  } else if (StringIsEqual(node.getName(), _T("Panel"))) {
    // Create the PanelControl

    style.ControlParent();

    PanelControl *frame = new PanelControl(parent, *xml_dialog_look,
                                           rc,
                                           style);

    window = frame;

    // Load children controls from the XMLNode
    LoadChildrenFromXML(form, *frame,
                        lookup_table, &node);

  // KeyboardControl
  } else if (StringIsEqual(node.getName(), _T("Keyboard"))) {
    KeyboardControl::OnCharacterCallback_t character_callback =
      (KeyboardControl::OnCharacterCallback_t)
      GetCallBack(lookup_table, node, _T("OnCharacter"));

    // Create the KeyboardControl
    KeyboardControl *kb =
      new KeyboardControl(parent, *xml_dialog_look,
                          pos.x, pos.y, size.cx, size.cy,
                          character_callback, style);

    window = kb;
  // DrawControl (WndOwnerDrawFrame)
  } else if (StringIsEqual(node.getName(), _T("Canvas"))) {
    // Determine DrawCallback function
    WndOwnerDrawFrame::OnPaintCallback_t paint_callback =
      (WndOwnerDrawFrame::OnPaintCallback_t)
      GetCallBack(lookup_table, node, _T("OnPaint"));

    // Create the DrawControl
    WndOwnerDrawFrame* canvas =
      new WndOwnerDrawFrame(parent, pos.x, pos.y, size.cx, size.cy,
                            style, paint_callback);

    window = canvas;

  // FrameControl (WndFrame)
  } else if (StringIsEqual(node.getName(), _T("Label"))){
    // Create the FrameControl
    WndFrame* frame = new WndFrame(parent, *xml_dialog_look,
                                   pos.x, pos.y, size.cx, size.cy,
                                   style);

    // Set the caption
    frame->SetCaption(caption);
    // Set caption color
    Color color;
    if (StringToColor(node.getAttribute(_T("CaptionColor")), color))
      frame->SetCaptionColor(color);

    window = frame;

  // ListBoxControl (WndListFrame)
  } else if (StringIsEqual(node.getName(), _T("List"))){
    // Determine ItemHeight of the list items
    UPixelScalar item_height =
      Layout::Scale(StringToIntDflt(node.getAttribute(_T("ItemHeight")), 18));

    // Create the ListBoxControl

    style.TabStop();

    if (IsEmbedded() || Layout::scale_1024 < 2048)
      /* sunken edge doesn't fit well on the tiny screen of an
         embedded device */
      style.Border();
    else
      style.SunkenEdge();

    window = new WndListFrame(parent, *xml_dialog_look,
                              pos.x, pos.y, size.cx, size.cy,
                              style,
                              item_height);

  // TabControl (Tabbed)
  } else if (StringIsEqual(node.getName(), _T("Tabbed"))) {
    // Create the TabControl

    style.ControlParent();

    TabbedControl *tabbed = new TabbedControl(parent,
                                              pos.x, pos.y, size.cx, size.cy,
                                              style);

    window = tabbed;

    for (auto i = node.begin(), end = node.end(); i != end; ++i) {
      // Load each child control from the child nodes
      Window *child = LoadChild(form, *tabbed,
                                lookup_table,
                                *i);
      if (child != NULL)
        tabbed->AddClient(child);
    }
  // TabBarControl (TabBar)
  } else if (StringIsEqual(node.getName(), _T("TabBar"))) {
    // Create the TabBarControl

    bool flip_orientation = false;
    if ( (Layout::landscape && StringToIntDflt(node.getAttribute(_T("Horizontal")), 0)) ||
         (!Layout::landscape && StringToIntDflt(node.getAttribute(_T("Vertical")), 0) ) )
      flip_orientation = true;

    style.ControlParent();
    TabBarControl *tabbar = new TabBarControl(parent, *xml_dialog_look,
                                              pos.x, pos.y, size.cx, size.cy,
                                              style, flip_orientation);
    window = tabbar;

    // TabMenuControl (TabMenu)
  } else if (StringIsEqual(node.getName(), _T("TabMenu"))) {
    // Create the TabMenuControl

    style.ControlParent();
    TabMenuControl *tabmenu = new TabMenuControl(parent,
                                                 /* XXX this cast is
                                                    an ugly hack!
                                                    Please rewrite: */
                                                 (WndForm &)form,
                                                 lookup_table,
                                                 *xml_dialog_look, caption,
                                                 pos.x, pos.y, size.cx, size.cy,
                                                 style);
    window = tabmenu;

  } else if (StringIsEqual(node.getName(), _T("Custom"))) {
    // Create a custom Window object with a callback
    CreateWindowCallback_t create_callback =
        (CreateWindowCallback_t)GetCallBack(lookup_table, node, _T("OnCreate"));
    if (create_callback == NULL)
      return NULL;

    window = create_callback(parent, pos.x, pos.y, size.cx, size.cy, style);
  } else if (StringIsEqual(node.getName(), _T("Widget"))) {
    style.ControlParent();
    DockWindow *dock = new DockWindow();
    dock->set(parent, rc, style);
    window = dock;
  }

  if (window != NULL) {
    if (!StringIsEmpty(name))
      form.AddNamed(name, window);

    if (expert)
      form.AddExpert(window);

    form.AddDestruct(window);
  }

  return window;
}

/**
 * Loads the Parent's children Controls from the given XMLNode
 *
 * @param form the SubForm object
 * @param Parent The parent control
 * @param LookUpTable The parents CallBackTable
 * @param Node The XMLNode that represents the parent control
 */
static void
LoadChildrenFromXML(SubForm &form, ContainerWindow &parent,
                    const CallBackTableEntry *lookup_table,
                    const XMLNode *node)
{
  unsigned bottom_most = 0;

  // Iterate through the childnodes
  for (auto i = node->begin(), end = node->end(); i != end; ++i) {
    // Load each child control from the child nodes
    Window *window = LoadChild(form, parent, lookup_table,
                               *i,
                               bottom_most);
    if (window == NULL)
      continue;

    bottom_most = window->get_position().bottom;
  }
}
