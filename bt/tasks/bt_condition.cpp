/**
 * bt_condition.cpp
 * =============================================================================
 * Copyright 2021-2023 Serhii Snitsaruk
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */

#include "bt_condition.h"

PackedStringArray BTCondition::get_configuration_warnings() const {
	PackedStringArray warnings = BTTask::get_configuration_warnings();
	if (get_child_count() != 0) {
		warnings.append("Condition task can't have child tasks.");
	}
	return warnings;
}