/**
 * bt_player.cpp
 * =============================================================================
 * Copyright 2021-2024 Serhii Snitsaruk
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */

#include "bt_player.h"

#include "../util/limbo_compat.h"
#include "../util/limbo_string_names.h"

#ifdef LIMBOAI_MODULE
#include "core/config/engine.h"
#include "core/debugger/engine_debugger.h"
#include "core/error/error_macros.h"
#include "core/io/resource_loader.h"
#include "core/object/class_db.h"
#include "core/os/memory.h"
#include "core/string/string_name.h"
#include "core/variant/variant.h"
#include "main/performance.h"

#define IS_DEBUGGER_ACTIVE() (EngineDebugger::is_active())
#define GET_TICKS_USEC() (OS::get_singleton()->get_ticks_usec())

#endif // ! LIMBOAI_MODULE

#ifdef LIMBOAI_GDEXTENSION
#include <godot_cpp/classes/engine_debugger.hpp>
#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/classes/time.hpp>

#define IS_DEBUGGER_ACTIVE() (EngineDebugger::get_singleton()->is_active())
#define GET_TICKS_USEC() (Time::get_singleton()->get_ticks_usec())

#endif // ! LIMBOAI_GDEXTENSION

VARIANT_ENUM_CAST(BTPlayer::UpdateMode);

void BTPlayer::_load_tree() {
	bt_instance.unref();
	ERR_FAIL_COND_MSG(!behavior_tree.is_valid(), "BTPlayer: Initialization failed - needs a valid behavior tree.");
	ERR_FAIL_COND_MSG(!behavior_tree->get_root_task().is_valid(), "BTPlayer: Initialization failed - behavior tree has no valid root task.");
	Node *agent = GET_NODE(this, agent_node);
	ERR_FAIL_NULL_MSG(agent, vformat("BTPlayer: Initialization failed - can't get agent with path '%s'.", agent_node));
	Node *scene_root = get_owner();
	ERR_FAIL_NULL_MSG(scene_root, "BTPlayer: Initialization failed - can't get scene root (make sure the BTPlayer's owner property is set).");
	bt_instance = behavior_tree->instantiate(agent, blackboard, this);
	ERR_FAIL_COND_MSG(bt_instance.is_null(), "BTPlayer: Failed to instantiate behavior tree.");
#ifdef DEBUG_ENABLED
	bt_instance->set_monitor_performance(monitor_performance);
	bt_instance->register_with_debugger();
#endif // DEBUG_ENABLED
}

void BTPlayer::_update_blackboard_plan() {
	if (blackboard_plan.is_null()) {
		blackboard_plan = Ref<BlackboardPlan>(memnew(BlackboardPlan));
	} else if (!RESOURCE_IS_BUILT_IN(blackboard_plan)) {
		WARN_PRINT_ED("BTPlayer: Using external resource for derived blackboard plan is not supported. Converted to built-in resource.");
		blackboard_plan = blackboard_plan->duplicate();
	}

	blackboard_plan->set_base_plan(behavior_tree.is_valid() ? behavior_tree->get_blackboard_plan() : nullptr);
}

void BTPlayer::set_behavior_tree(const Ref<BehaviorTree> &p_tree) {
	if (Engine::get_singleton()->is_editor_hint()) {
		if (behavior_tree.is_valid() && behavior_tree->is_connected(LW_NAME(plan_changed), callable_mp(this, &BTPlayer::_update_blackboard_plan))) {
			behavior_tree->disconnect(LW_NAME(plan_changed), callable_mp(this, &BTPlayer::_update_blackboard_plan));
		}
		if (p_tree.is_valid()) {
			p_tree->connect(LW_NAME(plan_changed), callable_mp(this, &BTPlayer::_update_blackboard_plan));
		}
		behavior_tree = p_tree;
		_update_blackboard_plan();
	} else {
		behavior_tree = p_tree;
		if (get_owner() && is_inside_tree()) {
			_load_tree();
		}
	}
}

void BTPlayer::set_agent_node(const NodePath &p_agent_node) {
	agent_node = p_agent_node;
	if (bt_instance.is_valid()) {
		ERR_PRINT("BTPlayer: Agent node cannot be set after the behavior tree is instantiated. This change will not affect the behavior tree instance.");
	}
}

void BTPlayer::set_blackboard_plan(const Ref<BlackboardPlan> &p_plan) {
	blackboard_plan = p_plan;
	_update_blackboard_plan();
}

void BTPlayer::set_update_mode(UpdateMode p_mode) {
	update_mode = p_mode;
	set_active(active);
}

void BTPlayer::set_active(bool p_active) {
	active = p_active;
	bool is_not_editor = !Engine::get_singleton()->is_editor_hint();
	set_process(update_mode == UpdateMode::IDLE && active && is_not_editor);
	set_physics_process(update_mode == UpdateMode::PHYSICS && active && is_not_editor);
	set_process_input(active && is_not_editor);
}

void BTPlayer::update(double p_delta) {
	if (!bt_instance.is_valid()) {
		ERR_PRINT_ONCE(vformat("BTPlayer doesn't have a behavior tree with a valid root task to execute (owner: %s)", get_owner()));
		return;
	}

	if (active) {
		BT::Status status = bt_instance->update(p_delta);
		emit_signal(LW_NAME(updated), status);
#ifndef DISABLE_DEPRECATED
		if (status == BTTask::SUCCESS || status == BTTask::FAILURE) {
			emit_signal(LW_NAME(behavior_tree_finished), status);
		}
#endif // DISABLE_DEPRECATED
	}
}

void BTPlayer::restart() {
	ERR_FAIL_COND_MSG(bt_instance.is_null(), "BTPlayer: Restart failed - no valid tree instance. Make sure the BTPlayer has a valid behavior tree with a valid root task.");
	bt_instance->get_root_task()->abort();
	set_active(true);
}

#ifdef DEBUG_ENABLED

void BTPlayer::_set_monitor_performance(bool p_monitor_performance) {
	monitor_performance = p_monitor_performance;

	if (bt_instance.is_valid()) {
		bt_instance->set_monitor_performance(monitor_performance);
	}
}

#endif // ! DEBUG_ENABLED

void BTPlayer::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_PROCESS: {
			Variant time = get_process_delta_time();
			update(time);
		} break;
		case NOTIFICATION_PHYSICS_PROCESS: {
			Variant time = get_physics_process_delta_time();
			update(time);
		} break;
		case NOTIFICATION_READY: {
			if (!Engine::get_singleton()->is_editor_hint()) {
				if (blackboard.is_null()) {
					blackboard = Ref<Blackboard>(memnew(Blackboard));
				}
				if (blackboard_plan.is_valid()) {
					blackboard_plan->populate_blackboard(blackboard, false, this);
				}
				if (behavior_tree.is_valid()) {
					_load_tree();
				}
			} else {
				_update_blackboard_plan();
			}
			set_active(active);
		} break;
		case NOTIFICATION_ENTER_TREE: {
#ifdef DEBUG_ENABLED
			if (bt_instance.is_valid()) {
				bt_instance->set_monitor_performance(monitor_performance);
				bt_instance->register_with_debugger();
			}
#endif // DEBUG_ENABLED
		} break;
		case NOTIFICATION_EXIT_TREE: {
#ifdef DEBUG_ENABLED
			if (bt_instance.is_valid()) {
				bt_instance->set_monitor_performance(false);
				bt_instance->unregister_with_debugger();
			}
#endif // DEBUG_ENABLED

			if (Engine::get_singleton()->is_editor_hint()) {
				if (behavior_tree.is_valid() && behavior_tree->is_connected(LW_NAME(plan_changed), callable_mp(this, &BTPlayer::_update_blackboard_plan))) {
					behavior_tree->disconnect(LW_NAME(plan_changed), callable_mp(this, &BTPlayer::_update_blackboard_plan));
				}
			}
		} break;
	}
}

void BTPlayer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_behavior_tree", "behavior_tree"), &BTPlayer::set_behavior_tree);
	ClassDB::bind_method(D_METHOD("get_behavior_tree"), &BTPlayer::get_behavior_tree);
	ClassDB::bind_method(D_METHOD("set_agent_node", "agent_node"), &BTPlayer::set_agent_node);
	ClassDB::bind_method(D_METHOD("get_agent_node"), &BTPlayer::get_agent_node);
	ClassDB::bind_method(D_METHOD("set_update_mode", "update_mode"), &BTPlayer::set_update_mode);
	ClassDB::bind_method(D_METHOD("get_update_mode"), &BTPlayer::get_update_mode);
	ClassDB::bind_method(D_METHOD("set_active", "active"), &BTPlayer::set_active);
	ClassDB::bind_method(D_METHOD("get_active"), &BTPlayer::get_active);
	ClassDB::bind_method(D_METHOD("set_blackboard", "blackboard"), &BTPlayer::set_blackboard);
	ClassDB::bind_method(D_METHOD("get_blackboard"), &BTPlayer::get_blackboard);

	ClassDB::bind_method(D_METHOD("set_blackboard_plan", "plan"), &BTPlayer::set_blackboard_plan);
	ClassDB::bind_method(D_METHOD("get_blackboard_plan"), &BTPlayer::get_blackboard_plan);

	ClassDB::bind_method(D_METHOD("update", "delta"), &BTPlayer::update);
	ClassDB::bind_method(D_METHOD("restart"), &BTPlayer::restart);

	ClassDB::bind_method(D_METHOD("get_bt_instance"), &BTPlayer::get_bt_instance);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "behavior_tree", PROPERTY_HINT_RESOURCE_TYPE, "BehaviorTree"), "set_behavior_tree", "get_behavior_tree");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "agent_node"), "set_agent_node", "get_agent_node");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "update_mode", PROPERTY_HINT_ENUM, "Idle,Physics,Manual"), "set_update_mode", "get_update_mode");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "active"), "set_active", "get_active");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "blackboard", PROPERTY_HINT_NONE, "Blackboard", 0), "set_blackboard", "get_blackboard");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "blackboard_plan", PROPERTY_HINT_RESOURCE_TYPE, "BlackboardPlan", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT | PROPERTY_USAGE_ALWAYS_DUPLICATE), "set_blackboard_plan", "get_blackboard_plan");

	BIND_ENUM_CONSTANT(IDLE);
	BIND_ENUM_CONSTANT(PHYSICS);
	BIND_ENUM_CONSTANT(MANUAL);

	ADD_SIGNAL(MethodInfo("updated", PropertyInfo(Variant::INT, "status")));

#ifndef DISABLE_DEPRECATED
	ADD_SIGNAL(MethodInfo("behavior_tree_finished", PropertyInfo(Variant::INT, "status")));
#endif

#ifdef DEBUG_ENABLED
	ClassDB::bind_method(D_METHOD("_set_monitor_performance", "enable"), &BTPlayer::_set_monitor_performance);
	ClassDB::bind_method(D_METHOD("_get_monitor_performance"), &BTPlayer::_get_monitor_performance);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "monitor_performance"), "_set_monitor_performance", "_get_monitor_performance");
#endif // DEBUG_ENABLED
}

BTPlayer::BTPlayer() {
	blackboard = Ref<Blackboard>(memnew(Blackboard));
	agent_node = LW_NAME(node_pp);
}

BTPlayer::~BTPlayer() {
}
