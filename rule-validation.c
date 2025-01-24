/* rule-validation.c
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

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "debugger.h"
#include "rules-reader.h"
#include "rule-validation.h"
#include "get-time.h"

struct _RuleTimeValidator
{
  uint16_t row_count;
  Rule *rules;
};

int
rule_validate_rule (const Rule *rule)
{
  bool mode = false;

  // name can't be bigger than the max value
  bool name = !(strlen (rule->name) >= RULE_NAME_LENGTH);     // (>=) do not include null terminator

  // hour [00,23]
  bool hour = rule->hour <= 23;     // Due to the data type, it is always >= 0

  // minutes [0, 59]
  bool minutes = rule->minutes <= 59;

  // table
  bool table = (rule->table == TABLE_ON || rule->table == TABLE_OFF);

  // mode according to enum predefined values
  if (rule->table == TABLE_OFF)
    mode = (rule->mode >= 0 && rule->mode <= MODE_LAST);
  else
    mode = true;

  DEBUG_PRINT (("VALIDATION:\nName: %d\n\tName length: %lu\n"\
                "Hour: %d\nMinutes: %d\nMode: %d\nTable: %d",
                name, strlen (rule->name), hour, minutes, mode, table));

  if (name && hour && minutes && mode && table)
    return EXIT_SUCCESS;
  else
    {
      fprintf (stderr, "Invalid rule values\n\n");
      return EXIT_FAILURE;
    }
}

int
rule_validate_table (const Table table)
{
  if (table == TABLE_ON || table == TABLE_OFF)
    return EXIT_SUCCESS;

  fprintf (stderr, "Invalid table\n\n");
  return EXIT_FAILURE;
}

RuleTimeValidator *
rule_validate_time_init (const Table table)
{
  RuleTimeValidator *time_validator = NULL;
  int status = 0;

  time_validator = malloc (sizeof (RuleTimeValidator));

  status = rule_get_all (table, &time_validator->rules, &time_validator->row_count);

  if (status == EXIT_FAILURE)
    {
      free (time_validator);
      return NULL;
    }

  return time_validator;
}

uint16_t
rule_validate_time (RuleTimeValidator *self,
                    const uint8_t hour,
                    const uint8_t minutes,
                    const bool days[7])
{
  if (self == NULL)
    {
      fprintf (stderr, "Rule not validate\n\n");
      // return as not validated rule
      return 1;
    }

  for (int idx = 0; idx < self->row_count; idx++)
    {
      for (int d = 0; d < 7; d++)
        {
          if ((self->rules[idx].days[d] && days[d])
              && self->rules[idx].hour == hour
              && self->rules[idx].minutes == minutes)
            return self->rules[idx].id;
        }
    }

  return 0;
}

void
rule_validate_time_finalize (RuleTimeValidator **self)
{
  free (*self);
  self = NULL;
}

int
rule_validade_rtcwake_args (const RtcwakeArgs *rtcwake_args)
{
  bool hour, minutes, date, year, mode;
  int ret;
  struct tm *timeinfo;
  struct tm input = {
    .tm_mday = rtcwake_args->day,
    .tm_mon = rtcwake_args->month - 1,
    .tm_year = rtcwake_args->year - 1900,
  };
  time_t generated_time = mktime (&input);

  DEBUG_PRINT (("Validating rtcwake_args..."));
  hour = minutes = date = year  = mode = false;

  // Hour
  if (rtcwake_args->hour >= 0 && rtcwake_args->hour <= 23)
    hour = true;

  // Minutes
  if (rtcwake_args->minutes >= 0 && rtcwake_args->minutes <= 59)
    minutes = true;

  // Date
  timeinfo = localtime (&generated_time);
  if (generated_time == -1
      || rtcwake_args->day != timeinfo->tm_mday
      || rtcwake_args->month != timeinfo->tm_mon + 1
      || rtcwake_args->year != timeinfo->tm_year + 1900)
    date = false;
  else
    date = true;

  // Year (must be this year or at most the next, only)
  get_time_tm (&timeinfo);
  if (rtcwake_args->year > (timeinfo->tm_year + 1900 + 1))
    year = false;
  else
    year = true;

  switch (rtcwake_args->mode)
    {
    case MODE_STANDBY:
    case MODE_FREEZE:
    case MODE_MEM:
    case MODE_DISK:
    case MODE_OFF:
      mode = true;
      break;

    case MODE_LAST:
    case MODE_NO:
    case MODE_ON:
    case MODE_DISABLE:
    case MODE_SHOW:
    default:
      mode = false;
    }

  if (hour && minutes && date && year && mode)
    ret = EXIT_SUCCESS;   // valid
  else
    ret = EXIT_FAILURE;   // invalid

  DEBUG_PRINT (("RtcwakeArgs validation:\n"\
                "\tHour: %d\n\tMinutes: %d\n\tDate: %d\n\tYear: %d\n"\
                "\tMode: %d\n\tthis_year: %d\n\t--> Status (1 if not passed): %d",
                hour, minutes, date, year, mode, timeinfo->tm_year + 1900, ret));

  return ret;
}
