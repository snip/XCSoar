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

#include "BasicMapWindow.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "Topography/TopographyStore.hpp"
#include "Topography/TopographyRenderer.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Terrain/RasterWeather.hpp"
#include "Look/TaskLook.hpp"
#include "Renderer/TaskRenderer.hpp"
#include "Renderer/RenderTaskPoint.hpp"
#include "Renderer/OZRenderer.hpp"
#include "Renderer/AircraftRenderer.hpp"
#include "Renderer/TrailRenderer.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Math/Earth.hpp"
#include "Units/Units.hpp"
#include "Interface.hpp"
#include "Computer/GlideComputer.hpp"
#include "Asset.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scissor.hpp"
#else
#include "Screen/WindowCanvas.hpp"
#endif

#include <tchar.h>

static const ComputerSettings &
GetComputerSettings()
{
  return CommonInterface::GetComputerSettings();
}

static const MapSettings &
GetMapSettings()
{
  return CommonInterface::GetMapSettings();
}

static const MoreData &
Basic()
{
  return CommonInterface::Basic();
}

static const DerivedInfo &
Calculated()
{
  return CommonInterface::Calculated();
}

/**
 * Constructor of the MapWindow class
 */
BasicMapWindow::BasicMapWindow(const WaypointLook &waypoint_look,
                                 const AirspaceLook &_airspace_look)
  :topography_renderer(NULL),
   airspace_renderer(_airspace_look),
   way_point_renderer(NULL, waypoint_look)
{
}

BasicMapWindow::~BasicMapWindow()
{
  delete topography_renderer;
}

void
BasicMapWindow::set(ContainerWindow &parent,
                     PixelScalar left, PixelScalar top,
                     UPixelScalar width, UPixelScalar height,
                     WindowStyle style)
{
  projection.SetFreeMapScale(fixed_int_constant(5000));

  BufferWindow::set(parent, left, top, width, height, style);
}

void
BasicMapWindow::RenderTerrain(Canvas &canvas)
{
  background.SetShadingAngle(projection, GetMapSettings().terrain,
                             Calculated());
  background.Draw(canvas, projection, GetMapSettings().terrain);
}

void
BasicMapWindow::RenderTopography(Canvas &canvas)
{
  if (topography_renderer != NULL && GetMapSettings().topography_enabled)
    topography_renderer->Draw(canvas, projection);
}

void
BasicMapWindow::RenderTopographyLabels(Canvas &canvas)
{
  if (topography_renderer != NULL && GetMapSettings().topography_enabled)
    topography_renderer->DrawLabels(canvas, projection, label_block);
}

void
BasicMapWindow::RenderAirspace(Canvas &canvas)
{
  if (GetMapSettings().airspace.enable)
    airspace_renderer.Draw(canvas,
#ifndef ENABLE_OPENGL
                           buffer_canvas, stencil_canvas,
#endif
                           projection,
                           Basic(), Calculated(),
                           GetComputerSettings().airspace,
                           GetMapSettings().airspace);
}

void
BasicMapWindow::DrawWaypoints(Canvas &canvas)
{
  const MapSettings &settings_map = GetMapSettings();
  WaypointRendererSettings settings = settings_map.waypoint;
  settings.display_text_type = DISPLAYNAME;

  way_point_renderer.render(canvas, label_block,
                            projection, settings,
                            GetComputerSettings().task,
                            NULL, NULL);
}

void
BasicMapWindow::OnPaintBuffer(Canvas &canvas)
{
#ifdef ENABLE_OPENGL
  /* enable clipping */
  GLCanvasScissor scissor(canvas);
#endif

  // reset label over-write preventer
  label_block.reset();

  // Render terrain, groundline and topography
  RenderTerrain(canvas);
  RenderTopography(canvas);

  // Render airspace
  RenderAirspace(canvas);

  // Render task, waypoints
  DrawWaypoints(canvas);

  // Render topography on top of airspace, to keep the text readable
  RenderTopographyLabels(canvas);
}

void
BasicMapWindow::SetTerrain(RasterTerrain *terrain)
{
  background.SetTerrain(terrain);
}

void
BasicMapWindow::SetTopograpgy(TopographyStore *topography)
{
  delete topography_renderer;
  topography_renderer = topography != NULL
    ? new TopographyRenderer(*topography)
    : NULL;
}

void
BasicMapWindow::SetTarget(GeoPoint location, fixed radius)
{
  projection.SetGeoLocation(location);
  projection.SetScaleFromRadius(radius);
  projection.SetScreenAngle(Angle::Zero());
  projection.UpdateScreenBounds();

  invalidate();
}

void
BasicMapWindow::OnResize(UPixelScalar width, UPixelScalar height)
{
  BufferWindow::OnResize(width, height);

#ifndef ENABLE_OPENGL
  buffer_canvas.grow(width, height);

  if (!IsAncientHardware())
    stencil_canvas.grow(width, height);
#endif

  projection.SetScreenSize(width, height);
  projection.SetScreenOrigin(width / 2, height / 2);
  projection.UpdateScreenBounds();
}

void
BasicMapWindow::OnCreate()
{
  BufferWindow::OnCreate();

#ifndef ENABLE_OPENGL
  WindowCanvas canvas(*this);
  buffer_canvas.set(canvas);

  if (!IsAncientHardware())
    stencil_canvas.set(canvas);
#endif
}

void
BasicMapWindow::OnDestroy()
{
  SetTerrain(NULL);
  SetTopograpgy(NULL);
  SetAirspaces(NULL);
  SetWaypoints(NULL);

#ifndef ENABLE_OPENGL
  buffer_canvas.reset();

  if (!IsAncientHardware())
    stencil_canvas.reset();
#endif

  BufferWindow::OnDestroy();
}
