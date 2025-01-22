/* time-converter.c
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

#include "time-converter.h"

#include "debugger.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <gio/gio.h>

#define PORTAL_BUS_NAME             "org.freedesktop.portal.Desktop"
#define PORTAL_OBJECT_PATH          "/org/freedesktop/portal/desktop"
#define PORTAL_SETTINGS_INTERFACE   "org.freedesktop.portal.Settings"
#define PORTAL_METHOD_NAME          "Read"
#define CLOCK_FORMAT_SCHEMA         "org.gnome.desktop.interface"
#define CLOCK_FORMAT_PROPERTY_NAME  "clock-format"

// See https://gitlab.gnome.org/GNOME/gnome-clocks/-/blob/master/src/utils.vala?ref_type=heads#L223
static int
time_converter_get_format_using_dbus (TimeFormat *format)
{
  g_autoptr (GDBusConnection) connection = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (GVariant) result = NULL;
  g_autoptr (GVariant) child1 = NULL;
  g_autoptr (GVariant) child2 = NULL;
  const gchar *time_format = NULL;

  connection = g_bus_get_sync (G_BUS_TYPE_SESSION,
                               NULL,
                               &error);

  if (error != NULL)
    {
      DEBUG_PRINT (("[warning]: Failed to get time format using DBus - connection error: %s",
                    error->message));
      return EXIT_FAILURE;
    }

  result = g_dbus_connection_call_sync (connection,
                                        PORTAL_BUS_NAME,
                                        PORTAL_OBJECT_PATH,
                                        PORTAL_SETTINGS_INTERFACE,
                                        PORTAL_METHOD_NAME,
                                        g_variant_new ("(ss)",
                                                       CLOCK_FORMAT_SCHEMA,
                                                       CLOCK_FORMAT_PROPERTY_NAME),
                                        NULL,
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        &error);

  if (error != NULL)
    {
      DEBUG_PRINT (("[warning]: Failed to get time format using DBus - method call error: %s",
                    error->message));
      return EXIT_FAILURE;
    }

  g_variant_get (result, "(v)", &child1);
  g_variant_get (child1, "v", &child2);
  time_format = g_variant_get_string (child2, NULL);

  DEBUG_PRINT (("[info]: DBus Setting time format: %s", time_format));

  *format = (g_strcmp0 (time_format, "12h") == 0) ?
             TIME_FORMAT_TWELVE : TIME_FORMAT_TWENTYFOUR;

  return EXIT_SUCCESS;
}

static int
time_converter_get_format_using_c_calls (TimeFormat *format)
{
  time_t now;
  struct tm *tm_info;
  char buffer[256];

  // Set the locale to the user's default environment
  setlocale (LC_TIME, "");

  // Get the current time
  time (&now);
  tm_info = localtime (&now);

  // Format the time in 12-hour format
  strftime (buffer, sizeof (buffer), "%I:%M %p", tm_info);

  DEBUG_PRINT (("[info]: got time using c calls %s\n", buffer));

  // Check if the formatted time contains AM or PM
  if (strstr (buffer, "AM") || strstr (buffer, "PM"))
    *format = TIME_FORMAT_TWELVE;
  else
    *format = TIME_FORMAT_TWENTYFOUR;

  return EXIT_SUCCESS;
}

TimeFormat
time_converter_get_format (void)
{
  static int checked = 0;
  static TimeFormat format = TIME_FORMAT_TWENTYFOUR;

  // First, try to get the time format using a DBus call
  if (!checked)
    if (time_converter_get_format_using_dbus (&format) == EXIT_SUCCESS)
      return format;

  // If DBus call to get the time format fails, use c functions instead
  if (!checked)
    if (time_converter_get_format_using_c_calls (&format) == EXIT_SUCCESS)
      return format;

  return format;
}

int
time_converter_to_twelve_format (uint8_t hour24,
                                 uint8_t *hour12,
                                 Period  *period)
{
  // Validation
  if (hour24 > 23)
    {
      DEBUG_PRINT_CONTEX;
      fprintf (stderr, "ERROR: Invalid time input\n");
      return EXIT_FAILURE;
    }

  // Midnight case
  if (hour24 == 0)
    {
      *hour12 = 12;
      *period = PERIOD_AM;
    }

  // Morning case
  else if (hour24 < 12)
    {
      *hour12 = hour24;
      *period = PERIOD_AM;
    }

  // Noon case
  else if (hour24 == 12)
    {
      *hour12 = 12; // Noon case
      *period = PERIOD_PM;
    }

  // Afternoon/evening case
  else
    {
      *hour12 = hour24 - 12;
      *period = PERIOD_PM;
    }

  return EXIT_SUCCESS;
}

int
time_converter_to_twentyfour_format (uint8_t  hour12,
                                     uint8_t *hour24,
                                     Period   period)
{
  // Validation
  if (hour12 < 1 || hour12 > 12)
    {
      DEBUG_PRINT_CONTEX;
      fprintf (stderr, "ERROR: Invalid time input\n");
      return EXIT_FAILURE;
    }

  if (period == PERIOD_AM)
    {
      if (hour12 == 12)
        *hour24 = 0;
      else
        *hour24 = hour12;
    }
  else
    {
      if (hour12 == 12)
        *hour24 = 12;
      else
        *hour24 = hour12 + 12;
    }

  return EXIT_SUCCESS;
}
