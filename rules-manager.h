/* rules-manager.h
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

#ifndef RULES_MANAGER_H_
#define RULES_MANAGER_H_

#include "gawake-types.h"

uint16_t rule_add (const Rule *rule);
int rule_delete (const uint16_t id, const Table table);
int rule_enable_disable (const uint16_t id, const Table table, const bool active);
uint16_t rule_edit (const Rule *rule);
int rule_custom_schedule (const RtcwakeArgs *rtcwake_args);

#endif /* RULES_MANAGER_H_ */
