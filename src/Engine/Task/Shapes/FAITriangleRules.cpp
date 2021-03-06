/* Copyright_License {

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

#include "FAITriangleRules.hpp"
#include "Geo/Math.hpp"

using namespace FAITriangleRules;

gcc_const
static bool
IsSmallFAITriangle(const fixed d_total,
                   const fixed d1, const fixed d2, const fixed d3)
{
  const fixed d_min = d_total * SMALL_MIN_LEG;
  return d1 >= d_min && d2 >= d_min && d3 >= d_min;
}

gcc_const
static bool
IsLargeFAITriangle(const fixed d_total,
                   const fixed d1, const fixed d2, const fixed d3)
{
  if (d_total < LARGE_THRESHOLD)
    return false;

  const fixed d_min = LargeMinLeg(d_total);
  if (d1 < d_min || d2 < d_min || d3 < d_min)
    return false;

  const fixed d_max = d_total * LARGE_MAX_LEG;
  return d1 <= d_max && d2 <= d_max && d3 <= d_max;
}

bool
FAITriangleRules::TestDistances(const fixed d1, const fixed d2, const fixed d3)
{
  const fixed d_wp = d1 + d2 + d3;

  /*
   * A triangle is a valid FAI-triangle, if no side is less than
   * 28% of the total length (total length less than 750 km), or no
   * side is less than 25% or larger than 45% of the total length
   * (totallength >= 750km).
   */

  return IsSmallFAITriangle(d_wp, d1, d2, d3) ||
    IsLargeFAITriangle(d_wp, d1, d2, d3);
}

bool
FAITriangleRules::TestDistances(const GeoPoint &a, const GeoPoint &b,
                                const GeoPoint &c)
{
  return TestDistances(Distance(a, b), Distance(b, c), Distance(c, a));
}
