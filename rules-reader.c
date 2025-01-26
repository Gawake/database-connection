/* rules-reader.c
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
#include <sqlite3.h>

#include "database-connection-utils.h"
#include "rule-validation.h"
#include "debugger.h"
#include "rules-reader.h"
#include "get-time.h"

#define ALLOC 256
#define BUFFER_ALLOC 5

int
rule_get_single (const uint16_t id,
                 const Table table,
                 Rule *rule)
{
  // Database related variables
  int rc;
  struct sqlite3_stmt *stmt;
  // Temporary variables to receive the hour and minutes, and then pass to the structure
  int hour, minutes;
  char timestamp[9]; // HH:MM:SS'\0' = 9 characters


  if (rule_validate_table (table))
    return EXIT_FAILURE;

  // Generate SQL
  // SELECT length(<table>.rule_name), * FROM <table>;
  sqlite3_snprintf (SQL_SIZE, utils_get_sql (),
                    "SELECT * FROM %s WHERE id=%d;",
                    TABLE[table], id);

  DEBUG_PRINT (("Generated SQL:\n\t%s", utils_get_sql ()));

  // Prepare statement
  if (sqlite3_prepare_v2 (utils_get_pdb (), utils_get_sql (), -1, &stmt, NULL) != SQLITE_OK)
    {
      DEBUG_PRINT_CONTEX;
      fprintf (stderr, "ERROR: Failed to query rule\n");
      sqlite3_finalize (stmt);
      return EXIT_FAILURE;
    }

  // Query data
  /* ATTENTION columns numbers:
   *    0     1             2       3       (...)       9       10        11
   *    id    rule_name     time    sun     (...)       sat     active    mode
   *                                                                      ^~~~
   *                                                                         |
   *                                                     only for turn off rules
   */
  while ((rc = sqlite3_step (stmt)) == SQLITE_ROW)
    {
      // ID
      rule->id = (uint16_t) sqlite3_column_int (stmt, 0);

      // NAME
      snprintf (rule->name,                     // string pointer
                RULE_NAME_LENGTH,               // size
                "%s",                           // format
                sqlite3_column_text (stmt, 1)); // arguments

      // MINUTES AND HOUR
      sqlite3_snprintf (9, timestamp, "%s", sqlite3_column_text (stmt, 2));
      sscanf (timestamp, "%02d:%02d", &hour, &minutes);
      rule->hour =  (uint8_t) hour;
      rule->minutes = (uint8_t) minutes;

      // DAYS
      for (int i = 0; i <= 6; i++)
        {
          // days range: [0,6]                  column range: [3,9]
          rule->days[i] = (bool) sqlite3_column_int (stmt, (i+3));
        }

      // ACTIVE
      rule->active = (bool) sqlite3_column_int (stmt, 10);

      // MODE (for turn on rules it isn't used, assigning 0):
      rule->mode = (Mode) ((table == TABLE_OFF) ? sqlite3_column_int (stmt, 11) : 0);

      // TABLE
      rule->table = (Table) table;
    }

  if (rc != SQLITE_DONE)
    {
      DEBUG_PRINT_CONTEX;
      /* TODO */
      /* fprintf (stderr, "ERROR (failed to query rule): %s\n"), sqlite3_errmsg (utils_get_pdb ()); */
      sqlite3_finalize (stmt);
      return EXIT_FAILURE;
    }

  sqlite3_finalize(stmt);

  DEBUG_PRINT (("rule_get_single:\n"\
                "\tId: %d\n"
                "\tName: %s\n"\
                "\tTime: %02d:%02d\n"\
                "\tDays: [%d, %d, %d,  %d, %d, %d, %d]\n"\
                "\tActive: %d\n"\
                "\tMode: %d\n",
                rule->id,
                rule->name,
                rule->hour, rule->minutes,
                rule->days[0], rule->days[1], rule->days[2], rule->days[3], rule->days[4], rule->days[5], rule->days[6],
                rule->active,
                rule->mode));

  return EXIT_SUCCESS;
}

int
rule_get_all (const Table table,
              Rule **rules,
              uint16_t *rowcount)
{
  int counter = 0;
  // Database related variables
  int rc;
  struct sqlite3_stmt *stmt;
  // Temporary variables to receive the hour and minutes, and then pass to the structure
  int hour, minutes;
  char timestamp[9]; // HH:MM:SS'\0' = 9 characters


  if (rule_validate_table (table))
    return EXIT_FAILURE;

  // Count the number of rows
  sqlite3_snprintf (SQL_SIZE, utils_get_sql (), "SELECT COUNT(*) FROM %s;", TABLE[table]);
  if (sqlite3_prepare_v2 (utils_get_pdb (), utils_get_sql (), -1, &stmt, NULL) == SQLITE_OK
      && sqlite3_step (stmt) != SQLITE_ROW)
    {
      DEBUG_PRINT_CONTEX;
      fprintf (stderr, "ERROR: Failed to query row count\n");
      sqlite3_finalize (stmt);
      return EXIT_FAILURE;
    }
  *rowcount = sqlite3_column_int (stmt, 0);

  DEBUG_PRINT (("Row count: %d", *rowcount));

  // Allocate structure array
  // https://www.youtube.com/watch?v=lq8tJS3g6tY
  // TODO should "sizeof (**rules)" be "sizeof (**Rules)"
  *rules = malloc (*rowcount * sizeof (**rules));
  if (*rules == NULL)
    {
      DEBUG_PRINT_CONTEX;
      fprintf (stderr, "ERROR: Failed to allocate memory\n");
      sqlite3_finalize (stmt);
      return EXIT_FAILURE;
    }

  // Generate SQL
  // SELECT length(<table>.rule_name), * FROM <table>;
  sqlite3_snprintf (SQL_SIZE, utils_get_sql (),
                    "SELECT length(%s.rule_name), * FROM %s;",
                    TABLE[table], TABLE[table]);

  DEBUG_PRINT (("Generated SQL:\n\t%s", utils_get_sql ()));

  // Prepare statement
  if (sqlite3_prepare_v2 (utils_get_pdb (), utils_get_sql (), -1, &stmt, NULL) != SQLITE_OK)
    {
      DEBUG_PRINT_CONTEX;
      fprintf (stderr, "ERROR: Failed to query rule\n");
      free (*rules);
      sqlite3_finalize (stmt);
      return EXIT_FAILURE;
    }

  // Query data
  /* ATTENTION columns numbers:
   *    0                     1     2             3       4       (...)       10      11        12
   *    rule_name length      id    rule_name     time    sun     (...)       sat     active    mode
   *                                                                                            ^~~~
   *                                                                                            |
   *                                                                      only for turn off rules
   */
  while ((rc = sqlite3_step (stmt)) == SQLITE_ROW)
    {
      // If the loop tries to assign on non SQL_SIZEated space, leave
      if (counter > *rowcount)
        break;

      // ID
      (*rules)[counter].id = (uint16_t) sqlite3_column_int (stmt, 1);

      // NAME
      snprintf ((*rules)[counter].name,           // string pointer
                  RULE_NAME_LENGTH,               // size
                  "%s",                           // format
                  sqlite3_column_text (stmt, 2)   // arguments
                  );

      // MINUTES AND HOUR
      sqlite3_snprintf (9, timestamp, "%s", sqlite3_column_text (stmt, 3));
      sscanf (timestamp, "%02d:%02d", &hour, &minutes);
      (*rules)[counter].hour =  (uint8_t) hour;
      (*rules)[counter].minutes = (uint8_t) minutes;

      // DAYS
      for (int i = 0; i <= 6; i++)
        {
          // days range: [0,6]                  column range: [4,10]
          (*rules)[counter].days[i] = (bool) sqlite3_column_int (stmt, (i+4));
        }

      // ACTIVE
      (*rules)[counter].active = (bool) sqlite3_column_int (stmt, 11);

      // MODE (for turn on rules it isn't used, assigning 0):
      (*rules)[counter].mode = (Mode) ((table == TABLE_OFF) ? sqlite3_column_int (stmt, 12) : 0);

      // TABLE
      (*rules)[counter].table = (Table) table;

      counter++;
    }

  if (rc != SQLITE_DONE)
    {
      DEBUG_PRINT_CONTEX;
      /* TODO */
      /* fprintf (stderr, "ERROR (failed to query rule): %s\n"), sqlite3_errmsg (utils_get_pdb ()); */
      free (*rules);
      sqlite3_finalize (stmt);
      return EXIT_FAILURE;
    }

  sqlite3_finalize(stmt);

  return EXIT_SUCCESS;
}

// Receives a week day from 0 to 13, and returns from 0 to 6 (Sunday to Saturday);
// in other words, two weeks must be represented from 0 to 6 instead of 0 to 13
static int
week_day (int num)
{
  switch (num)
  {
  case 0:
  case 7:
    return 0;

  case 1:
  case 8:
    return 1;

  case 2:
  case 9:
    return 2;

  case 3:
  case 10:
    return 3;

  case 4:
  case 11:
    return 4;

  case 5:
  case 12:
    return 5;

  case 6:
  case 13:
    return 6;

  default:
    return -1;
  }
}

RtcwakeArgsReturn
rule_get_upcoming_on (RtcwakeArgs *rtcwake_args,
                      Mode         mode)
{
  int rc, now, ruletime, id_match = -1;
  bool is_localtime = true;

  struct tm *timeinfo;
  struct sqlite3_stmt *stmt;

  // Index(0 to 6) matches tm_wday; these strings refer to SQLite columns name
  char query[ALLOC], buffer[BUFFER_ALLOC], date[9];

  rtcwake_args->found = false;

  // GET THE DATABASE CONFIG
  rc = sqlite3_prepare_v2 (utils_get_pdb (),
                           "SELECT localtime, default_mode, shutdown_fail "\
                           "FROM config WHERE id = 1;",
                           -1,
                           &stmt,
                           NULL);
  if (rc != SQLITE_OK)
    {
      DEBUG_PRINT_CONTEX;
      fprintf (stderr, "ERROR: Failed getting config information\n");
      return RTCWAKE_ARGS_RETURN_FAILURE;
    }
  while ((rc = sqlite3_step (stmt)) == SQLITE_ROW)
    {
      // Localtime
      is_localtime = (bool) sqlite3_column_int (stmt, 0);

      // Mode
      // Use default mode
      if (mode == MODE_LAST)
        rtcwake_args->mode = (Mode) sqlite3_column_int (stmt, 1);
      // Use passed mode
      else
        rtcwake_args->mode = mode;

      // Shutdown on failure
      rtcwake_args->shutdown_fail = sqlite3_column_int (stmt, 2);
    }
  if (rc != SQLITE_DONE)
    {
      DEBUG_PRINT_CONTEX;
      fprintf (stderr, "ERROR (failed getting config information): %s\n",
               sqlite3_errmsg (utils_get_pdb ()));
      return RTCWAKE_ARGS_RETURN_FAILURE;
    }
  sqlite3_finalize (stmt);

  // GET THE CURRENT TIME
  // hour, minutes and seconds as integer members
  get_time_tm (&timeinfo);
  // Concatenate: HHMM as a string
  snprintf (buffer, BUFFER_ALLOC, "%02d%02d", timeinfo->tm_hour, timeinfo->tm_min);
  // HHMM as an integer, leading zeros doesn't matter
  now = atoi (buffer);

  DEBUG_PRINT (("Trying to get schedule for today\n"));

  // Create an SQL statement to get today's active rules time; tm_wday = number of the week
  snprintf (query,
            ALLOC,
            "SELECT id, strftime('%%H%%M', rule_time), strftime('%%Y%%m%%d', 'now', '%s') "\
            "FROM rules_turnon "\
            "WHERE %s = 1 AND active = 1 "\
            "ORDER BY time(rule_time) ASC;",
            is_localtime ? "localtime" : "utc",
            DAYS[timeinfo->tm_wday]);

  rc = sqlite3_prepare_v2 (utils_get_pdb (), query, -1, &stmt, NULL);
  if (rc != SQLITE_OK)
    {
      DEBUG_PRINT_CONTEX;
      fprintf (stderr, "ERROR: Failed while querying rules to make schedule for today\n");
      return RTCWAKE_ARGS_RETURN_FAILURE;
    }

  // Get all rules today, ordered by time; the first rule that has a bigger time than now is a valid
  while ((rc = sqlite3_step (stmt)) == SQLITE_ROW)
    {
      int id = sqlite3_column_int (stmt, 0);
      ruletime = sqlite3_column_int (stmt, 1);
      if (now < ruletime)
        {
          id_match = id;
          snprintf (date, 9, "%s", sqlite3_column_text (stmt, 2));               // YYYYMMDD
          snprintf (buffer, BUFFER_ALLOC, "%s", sqlite3_column_text (stmt, 1));  // HHMM
          break;
        }
      }
    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
      {
        DEBUG_PRINT_CONTEX;
        fprintf (stderr, "ERROR (failed scheduling for today): %s\n",
                 sqlite3_errmsg (utils_get_pdb ()));
        return RTCWAKE_ARGS_RETURN_FAILURE;
      }
  sqlite3_finalize (stmt);

  // IF IT WASN'T POSSIBLE TO SCHEDULE FOR TODAY, TRY ON THE NEXT DAYS
  if (id_match < 0)
    {
      DEBUG_PRINT (("Any time matched. Trying to schedule for tomorrow or later\n"));
      // search for a matching rule within a week
      for (int i = 1; i <= 7; i++)
        {
          int wday_num = week_day (timeinfo->tm_wday + i);
          if (wday_num == -1)
            {
              DEBUG_PRINT_CONTEX;
              fprintf (stderr, "ERROR: Failed to get schedule for for tomorrow or later (on wday function)\n");
              return RTCWAKE_ARGS_RETURN_FAILURE;
            }

          /*
           * The first rule after today is a valid match;
           * also calculate the day: now + number of day until the matching rule,
           * represented by the index i of the loop
           */
          snprintf (query,
                    ALLOC,
                    "SELECT id, strftime('%%Y%%m%%d', 'now', '%s', '+%d day'), strftime('%%H%%M', rule_time) "\
                    "FROM rules_turnon "\
                    "WHERE %s = 1 AND active = 1 "\
                    "ORDER BY time(rule_time) ASC LIMIT 1;",
                    is_localtime ? "localtime" : "utc",
                    i,
                    DAYS[wday_num]);

          rc = sqlite3_prepare_v2 (utils_get_pdb (), query, -1, &stmt, NULL);
          if (rc != SQLITE_OK)
            {
              DEBUG_PRINT_CONTEX;
              fprintf (stderr, "ERROR: Failed scheduling for after\n");
              return RTCWAKE_ARGS_RETURN_FAILURE;
            }
          while ((rc = sqlite3_step (stmt)) == SQLITE_ROW)
            {
              id_match = sqlite3_column_int (stmt, 0);
              snprintf (date, 9, "%s", sqlite3_column_text (stmt, 1)); // YYYYMMDD
              snprintf (buffer,
                        BUFFER_ALLOC,
                        "%s",
                        sqlite3_column_text (stmt, 2)); // HHMM
            }
          if (rc != SQLITE_DONE)
            {
              DEBUG_PRINT_CONTEX;
              fprintf (stderr, "ERROR (failed scheduling for after): %s\n",
                       sqlite3_errmsg (utils_get_pdb ()));
              return RTCWAKE_ARGS_RETURN_FAILURE;
            }
          sqlite3_finalize (stmt);

          if (id_match >= 0)
            break;
        }
    }

  // IF ANY RULE WAS FOUND, SEND RETURN AS RULE NOT FOUND
  if (id_match < 0)
    {
      fprintf (stderr, "WARNING: Any turn on rule found.\n");
      return RTCWAKE_ARGS_RETURN_NOT_FOUND;
    }

  // ELSE, RETURN PARAMETERS
  rtcwake_args->found = true;

  sscanf (buffer, "%02d%02d",
          &(rtcwake_args->hour),
          &(rtcwake_args->minutes));

  sscanf (date, "%04d%02d%02d",
          &(rtcwake_args->year),
          &(rtcwake_args->month),
          &(rtcwake_args->day));

  DEBUG_PRINT (("RtcwakeArgs fields:\n"\
                "\tFound: %d\n\tShutdown: %d"\
                "\n\t[HH:MM] %02d:%02d\n\t[DD/MM/YYYY] %02d/%02d/%d"\
                "\n\tMode: %d",
                rtcwake_args->found, rtcwake_args->shutdown_fail,
                rtcwake_args->hour, rtcwake_args->minutes,
                rtcwake_args->day, rtcwake_args->month, rtcwake_args->year,
                rtcwake_args->mode));

  if (rule_validade_rtcwake_args (rtcwake_args) == EXIT_FAILURE)
    return RTCWAKE_ARGS_RETURN_INVALID;
  else
    return RTCWAKE_ARGS_RETURN_SUCESS;
}
