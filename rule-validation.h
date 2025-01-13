/* rule-validation.h
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

#ifndef RULE_VALIDATION_H_
#define RULE_VALIDATION_H_

#include "gawake-types.h"

typedef struct _RuleTimeValidator RuleTimeValidator;

int rule_validate_rule (const Rule *rule);
int rule_validate_table (const Table table);

RuleTimeValidator *rule_validate_time_init (const Table table);
// Returns 0 if the rule was validated
// Return an id != 0, if the time is invalid; the id is an existing rule with a conflicting time
uint16_t rule_validate_time (RuleTimeValidator *self,
                             const uint8_t hour,
                             const uint8_t minutes,
                             const bool days[7]);
void rule_validate_time_finalize (RuleTimeValidator **self);

#endif /* RULE_VALIDATION_H_ */
