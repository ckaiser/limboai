/**
 * limbo_utility.cpp
 * =============================================================================
 * Copyright 2021-2023 Serhii Snitsaruk
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */

#include "limbo_utility.h"

#ifdef LIMBOAI_MODULE

#include "modules/limboai/bt/tasks/bt_task.h"

#include "core/error/error_macros.h"
#include "core/object/script_language.h"
#include "core/variant/variant.h"
#include "scene/resources/texture.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_node.h"
#endif // TOOLS_ENABLED

#endif // LIMBOAI_MODULE

#ifdef LIMBOAI_GDEXTENSION
#include "bt/tasks/bt_task.h"

#include "godot_cpp/classes/input_event_key.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/core/error_macros.hpp>

#endif // LIMBOAI_GDEXTENSION

LimboUtility *LimboUtility::singleton = nullptr;

LimboUtility *LimboUtility::get_singleton() {
	return singleton;
}

String LimboUtility::decorate_var(String p_variable) const {
	String var = p_variable.trim_prefix("$").trim_prefix("\"").trim_suffix("\"");
	if (var.find(" ") == -1 && !var.is_empty()) {
		return vformat("$%s", var);
	} else {
		return vformat("$\"%s\"", var);
	}
}

String LimboUtility::get_status_name(int p_status) const {
	switch (p_status) {
		case BTTask::FRESH:
			return "FRESH";
		case BTTask::RUNNING:
			return "RUNNING";
		case BTTask::FAILURE:
			return "FAILURE";
		case BTTask::SUCCESS:
			return "SUCCESS";
		default:
			return "";
	}
}

Ref<Texture2D> LimboUtility::get_task_icon(String p_class_or_script_path) const {
#if defined(TOOLS_ENABLED) && defined(LIMBOAI_MODULE)
	ERR_FAIL_COND_V_MSG(p_class_or_script_path.is_empty(), Variant(), "BTTask: script path or class cannot be empty.");

	Ref<Theme> theme = EditorNode::get_singleton()->get_editor_theme();
	ERR_FAIL_COND_V(theme.is_null(), nullptr);

	if (p_class_or_script_path.begins_with("res:")) {
		Ref<Script> s = ResourceLoader::load(p_class_or_script_path, "Script");
		if (s.is_null()) {
			return theme->get_icon(SNAME("FileBroken"), SNAME("EditorIcons"));
		}

		EditorData &ed = EditorNode::get_editor_data();
		Ref<Texture2D> script_icon = ed.get_script_icon(s);
		if (script_icon.is_valid()) {
			return script_icon;
		}

		StringName base_type = s->get_instance_base_type();
		if (theme->has_icon(base_type, SNAME("EditorIcons"))) {
			return theme->get_icon(base_type, SNAME("EditorIcons"));
		}
	}

	if (theme->has_icon(p_class_or_script_path, SNAME("EditorIcons"))) {
		return theme->get_icon(p_class_or_script_path, SNAME("EditorIcons"));
	}

	// Use an icon of one of the base classes: look up max 3 parents.
	StringName class_name = p_class_or_script_path;
	for (int i = 0; i < 3; i++) {
		class_name = ClassDB::get_parent_class(class_name);
		if (theme->has_icon(class_name, SNAME("EditorIcons"))) {
			return theme->get_icon(class_name, SNAME("EditorIcons"));
		}
	}
	// Return generic resource icon as a fallback.
	return theme->get_icon(SNAME("Resource"), SNAME("EditorIcons"));
#endif // TOOLS_ENABLED

	// TODO: GDExtension needs the icons too.

	// * Class icons are not available at runtime as they are part of the editor theme.
	return nullptr;
}

String LimboUtility::get_check_operator_string(CheckType p_check_type) const {
	switch (p_check_type) {
		case LimboUtility::CheckType::CHECK_EQUAL: {
			return "==";
		} break;
		case LimboUtility::CheckType::CHECK_LESS_THAN: {
			return "<";
		} break;
		case LimboUtility::CheckType::CHECK_LESS_THAN_OR_EQUAL: {
			return "<=";
		} break;
		case LimboUtility::CheckType::CHECK_GREATER_THAN: {
			return ">";
		} break;
		case LimboUtility::CheckType::CHECK_GREATER_THAN_OR_EQUAL: {
			return ">=";
		} break;
		case LimboUtility::CheckType::CHECK_NOT_EQUAL: {
			return "!=";
		} break;
		default: {
			return "?";
		} break;
	}
}

bool LimboUtility::perform_check(CheckType p_check_type, const Variant &left_value, const Variant &right_value) {
	Variant ret;
	switch (p_check_type) {
		case LimboUtility::CheckType::CHECK_EQUAL: {
			VARIANT_EVALUATE(Variant::OP_EQUAL, left_value, right_value, ret);
		} break;
		case LimboUtility::CheckType::CHECK_LESS_THAN: {
			VARIANT_EVALUATE(Variant::OP_LESS, left_value, right_value, ret);
		} break;
		case LimboUtility::CheckType::CHECK_LESS_THAN_OR_EQUAL: {
			VARIANT_EVALUATE(Variant::OP_LESS_EQUAL, left_value, right_value, ret);
		} break;
		case LimboUtility::CheckType::CHECK_GREATER_THAN: {
			VARIANT_EVALUATE(Variant::OP_GREATER, left_value, right_value, ret);
		} break;
		case LimboUtility::CheckType::CHECK_GREATER_THAN_OR_EQUAL: {
			VARIANT_EVALUATE(Variant::OP_GREATER_EQUAL, left_value, right_value, ret);
		} break;
		case LimboUtility::CheckType::CHECK_NOT_EQUAL: {
			VARIANT_EVALUATE(Variant::OP_NOT_EQUAL, left_value, right_value, ret);
		} break;
		default: {
			return false;
		} break;
	}

	return ret;
}

String LimboUtility::get_operation_string(Operation p_operation) const {
	switch (p_operation) {
		case OPERATION_NONE: {
			return "";
		} break;
		case OPERATION_ADDITION: {
			return "+";
		} break;
		case OPERATION_SUBTRACTION: {
			return "-";
		} break;
		case OPERATION_MULTIPLICATION: {
			return "*";
		} break;
		case OPERATION_DIVISION: {
			return "/";
		} break;
		case OPERATION_MODULO: {
			return "%";
		} break;
		case OPERATION_POWER: {
			return "**";
		} break;
		case OPERATION_BIT_SHIFT_LEFT: {
			return "<<";
		} break;
		case OPERATION_BIT_SHIFT_RIGHT: {
			return ">>";
		} break;
		case OPERATION_BIT_AND: {
			return "&";
		} break;
		case OPERATION_BIT_OR: {
			return "|";
		} break;
		case OPERATION_BIT_XOR: {
			return "^";
		} break;
	}
	return "";
}

Variant LimboUtility::perform_operation(Operation p_operation, const Variant &left_value, const Variant &right_value) {
	Variant ret;
	switch (p_operation) {
		case OPERATION_NONE: {
			ret = right_value;
		} break;
		case OPERATION_ADDITION: {
			VARIANT_EVALUATE(Variant::OP_ADD, left_value, right_value, ret);
		} break;
		case OPERATION_SUBTRACTION: {
			VARIANT_EVALUATE(Variant::OP_SUBTRACT, left_value, right_value, ret);
		} break;
		case OPERATION_MULTIPLICATION: {
			VARIANT_EVALUATE(Variant::OP_MULTIPLY, left_value, right_value, ret);
		} break;
		case OPERATION_DIVISION: {
			VARIANT_EVALUATE(Variant::OP_DIVIDE, left_value, right_value, ret);
		} break;
		case OPERATION_MODULO: {
			VARIANT_EVALUATE(Variant::OP_MODULE, left_value, right_value, ret);
		} break;
		case OPERATION_POWER: {
			// TODO: Fix when godot-cpp https://github.com/godotengine/godot-cpp/issues/1348 is resolved.
			// 	Variant::evaluate(Variant::OP_POWER, left_value, right_value, ret, valid);
			ERR_PRINT("LimboUtility: Operation POWER is not available due to https://github.com/godotengine/godot-cpp/issues/1348");
		} break;
		case OPERATION_BIT_SHIFT_LEFT: {
			VARIANT_EVALUATE(Variant::OP_SHIFT_LEFT, left_value, right_value, ret);
		} break;
		case OPERATION_BIT_SHIFT_RIGHT: {
			VARIANT_EVALUATE(Variant::OP_SHIFT_RIGHT, left_value, right_value, ret);
		} break;
		case OPERATION_BIT_AND: {
			VARIANT_EVALUATE(Variant::OP_BIT_AND, left_value, right_value, ret);
		} break;
		case OPERATION_BIT_OR: {
			VARIANT_EVALUATE(Variant::OP_BIT_OR, left_value, right_value, ret);
		} break;
		case OPERATION_BIT_XOR: {
			VARIANT_EVALUATE(Variant::OP_BIT_XOR, left_value, right_value, ret);
		} break;
	}
	return Variant();
}

Ref<Shortcut> LimboUtility::add_shortcut(const String &p_path, const String &p_name, Key p_keycode) {
	Ref<Shortcut> sc = memnew(Shortcut);
	sc->set_name(p_name);

	Array events;
	Ref<InputEventKey> ev = memnew(InputEventKey);
	ev->set_keycode(p_keycode);
	events.append(ev);
	sc->set_events(events);

	shortcuts[p_path] = sc;

	return sc;
}

bool LimboUtility::is_shortcut(const String &p_path, const Ref<InputEvent> &p_event) const {
	HashMap<String, Ref<Shortcut>>::ConstIterator E = shortcuts.find(p_path);
	ERR_FAIL_COND_V_MSG(!E, false, vformat("LimboUtility: Shortcut not found: %s.", p_path));
	return E->value->matches_event(p_event);
}

Ref<Shortcut> LimboUtility::get_shortcut(const String &p_path) const {
	HashMap<String, Ref<Shortcut>>::ConstIterator SC = shortcuts.find(p_path);
	if (SC) {
		return SC->value;
	}
	return nullptr;
}

void LimboUtility::_bind_methods() {
	ClassDB::bind_method(D_METHOD("decorate_var", "p_variable"), &LimboUtility::decorate_var);
	ClassDB::bind_method(D_METHOD("get_status_name", "p_status"), &LimboUtility::get_status_name);
	ClassDB::bind_method(D_METHOD("get_task_icon", "p_class_or_script_path"), &LimboUtility::get_task_icon);

	BIND_ENUM_CONSTANT(CHECK_EQUAL);
	BIND_ENUM_CONSTANT(CHECK_LESS_THAN);
	BIND_ENUM_CONSTANT(CHECK_LESS_THAN_OR_EQUAL);
	BIND_ENUM_CONSTANT(CHECK_GREATER_THAN);
	BIND_ENUM_CONSTANT(CHECK_GREATER_THAN_OR_EQUAL);
	BIND_ENUM_CONSTANT(CHECK_NOT_EQUAL);

	BIND_ENUM_CONSTANT(OPERATION_NONE);
	BIND_ENUM_CONSTANT(OPERATION_ADDITION);
	BIND_ENUM_CONSTANT(OPERATION_SUBTRACTION);
	BIND_ENUM_CONSTANT(OPERATION_MULTIPLICATION);
	BIND_ENUM_CONSTANT(OPERATION_DIVISION);
	BIND_ENUM_CONSTANT(OPERATION_MODULO);
	BIND_ENUM_CONSTANT(OPERATION_POWER);
	BIND_ENUM_CONSTANT(OPERATION_BIT_SHIFT_LEFT);
	BIND_ENUM_CONSTANT(OPERATION_BIT_SHIFT_RIGHT);
	BIND_ENUM_CONSTANT(OPERATION_BIT_AND);
	BIND_ENUM_CONSTANT(OPERATION_BIT_OR);
	BIND_ENUM_CONSTANT(OPERATION_BIT_XOR);
}

LimboUtility::LimboUtility() {
	singleton = this;
}

LimboUtility::~LimboUtility() {
	singleton = nullptr;
}
