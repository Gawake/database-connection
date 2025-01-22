/* time-converter.h
 *
 * Copyright 2021-2025 Kelvin Novais
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TIME_CONVERTER_H_
#define TIME_CONVERTER_H_

#include <inttypes.h>

typedef enum
{
  TIME_FORMAT_TWENTYFOUR,
  TIME_FORMAT_TWELVE
} TimeFormat;

typedef enum
{
  PERIOD_AM = 0,
  PERIOD_PM
} Period;

TimeFormat time_converter_get_format (void);

int time_converter_to_twelve_format (uint8_t  hour24,
                                     uint8_t *hour12,
                                     Period  *period);

int time_converter_to_twentyfour_format (uint8_t  hour12,
                                         uint8_t *hour24,
                                         Period   period);

#endif /* TIME_CONVERTER_H_ */
