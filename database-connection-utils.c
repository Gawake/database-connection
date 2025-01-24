/* database-connection-utils.c
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
#include "debugger.h"

static sqlite3 *db = NULL;
static char sql[SQL_SIZE];

int
utils_run_sql (void)
{
  int rc;
  char *err_msg = 0;

  if (db == NULL)
    {
      fprintf (stderr, "Database not connected\n");
      return EXIT_FAILURE;
    }

  DEBUG_PRINT (("Generated SQL:\n\t%s", sql));

  rc = sqlite3_exec(db, sql, NULL, 0, &err_msg);
  if (rc != SQLITE_OK)
    fprintf (stderr, "Failed to run SQL: %s\n", sqlite3_errmsg(db));

  sqlite3_free (err_msg);

  if (rc == SQLITE_OK)
    {
      // TODO
      /* trigger_update_database (); */
      return EXIT_SUCCESS;
    }
  else
    return EXIT_FAILURE;
}

char *
utils_get_sql (void)
{
  return sql;
}

sqlite3 * utils_get_pdb (void)
{
  return db;
}

sqlite3 ** utils_get_ppdb (void)
{
  return &db;
}
