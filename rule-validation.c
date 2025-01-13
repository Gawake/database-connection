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

#include "debugger.h"
#include "rules-reader.h"
#include "rule-validation.h"

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
