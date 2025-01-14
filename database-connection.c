/* database-connection.c
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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>

#include "database-connection.h"
#include "database-connection-utils.h"
#include "debugger.h"

// This function connect to the database
// Should be called once
int
connect_database (bool read_only)
{
  int rc = 0;

  if (utils_get_pdb () != NULL)
    {
      printf ("Warning: Database already connected.\n");
      return SQLITE_OK;
    }

  // Open the SQLite database
  if (!read_only)
    rc = sqlite3_open_v2 (DB_PATH, utils_get_ppdb (), SQLITE_OPEN_READWRITE, NULL);
  else
    rc = sqlite3_open_v2 (DB_PATH, utils_get_ppdb (), SQLITE_OPEN_READONLY, NULL);

  if (rc != SQLITE_OK)
    {
      DEBUG_PRINT_CONTEX;
      fprintf (stderr, "Can't open database: %s\n", sqlite3_errmsg (utils_get_pdb ()));
      sqlite3_close (utils_get_pdb ());
      return rc;
    }
  else
    {
      // Enable security options
      sqlite3_db_config (utils_get_pdb (), SQLITE_DBCONFIG_DEFENSIVE, 0, 0);
      sqlite3_db_config (utils_get_pdb (), SQLITE_DBCONFIG_ENABLE_TRIGGER, 0, 0);
      sqlite3_db_config (utils_get_pdb (), SQLITE_DBCONFIG_ENABLE_VIEW, 0, 0);
      sqlite3_db_config (utils_get_pdb (), SQLITE_DBCONFIG_TRUSTED_SCHEMA, 0, 0);
    }

  return rc;
}

int
disconnect_database (void)
{
  sqlite3 **db = utils_get_ppdb ();
  int rc = sqlite3_close (utils_get_pdb ());
  *db = NULL;
  return rc;
}

bool
check_user_group (void)
{
  int ngroups = 0;
  gid_t *groups = NULL;
  uid_t uid;
  struct passwd *pw = NULL;

  // Get the UID
  uid = getuid ();

  // Get struct pw
  pw = getpwuid (uid);
  if (pw == NULL)
    {
      DEBUG_PRINT_CONTEX;
      fprintf (stderr, "Error on getpwuid\n");
      return true;
    }

  // Get ngroups
  getgrouplist (pw->pw_name, pw->pw_gid, NULL, &ngroups);

  // Allocate memory for groups
  groups = malloc (ngroups * sizeof (gid_t));
  if (groups == NULL)
    {
      DEBUG_PRINT_CONTEX;
      fprintf (stderr, "Failed to allocate memory for groups\n");
      return true;
    }

  // Get groups
  getgrouplist (pw->pw_name, pw->pw_gid, groups, &ngroups);

  // Check if user is in group gawake
  for (int i = 0; i < ngroups; i++)
    {
      struct group* gr = getgrgid (groups[i]);

      if (gr == NULL)
        {
          DEBUG_PRINT_CONTEX;
          fprintf (stderr, "Failed to getgrgid\n");
          free (groups);
          return true;
        }

      if (strcmp ("gawake", gr->gr_name) == 0)
        {
          printf ("Info: user is in group \"gawake\"\n");
          free (groups);
          return false;
        }
    }

  free (groups);
  groups = NULL;
  return true;
}
