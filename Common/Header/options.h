/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>

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

  $Id$
}
*/

#ifndef OPTIONS_H
#define OPTIONS_H

#define   MONOCHROME_SCREEN     1             // optimize for monochrom screen
#define   EXPERIMENTAL          0             // ????

#define   LOGGDEVICEINSTREAM    0             // log device in stream
#define   LOGGDEVCOMMANDLINE    NULL          // device in-stream logger command line
                                              // ie TEXT("-logA=\\Speicherkarte\\logA.log ""-logB=\\SD Card\\logB.log""")
#define   AIRSPACEUSEBINFILE    0             // use and maintain binary airspace file

// define this to be true for windows PC port
#if !defined(WINDOWSPC)
#define   WINDOWSPC             0
#endif

#define   FONTQUALITY           NONANTIALIASED_QUALITY

#if (WINDOWSPC>0)
#if _DEBUG
// leak checking
#define CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
#endif

#define DISABLEAUDIOVARIO

#if defined(GNAV)
#define DISABLEAUDIOVARIO
// use exception handling
#ifndef ALTAIRPROTOTYPE
#ifndef __MINGW32__
#define HAVEEXCEPTIONS
#endif
#endif
// disable internally generated sounds
#define DISABLEAUDIO
#else
#ifndef BIGDISPLAY
#define BIGDISPLAY
#endif
#endif


#ifdef BIGDISPLAY
#define IBLSCALE(x) (   (InfoBoxLayout::IntScaleFlag) ? ((x)*InfoBoxLayout::scale) : ((int)((x)*InfoBoxLayout::dscale)))

#else
#define IBLSCALE(x) (x)
#endif

#ifdef __MINGW32__
#if (WINDOWSPC==0)
#define NEWFLARMDB
#endif
#endif

#ifdef PNA
#define NOLINETO
#endif

#endif
