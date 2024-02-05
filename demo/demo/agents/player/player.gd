#*
#* player.gd
#* =============================================================================
#* Copyright 2021-2024 Serhii Snitsaruk
#*
#* Use of this source code is governed by an MIT-style
#* license that can be found in the LICENSE file or at
#* https://opensource.org/licenses/MIT.
#* =============================================================================
#*
extends "res://demo/agents/scripts/agent_base.gd"

## Player.

@export var dodge_cooldown: float = 0.4

@onready var hsm: LimboHSM = $LimboHSM
@onready var idle_state: LimboState = $LimboHSM/IdleState
@onready var move_state: LimboState = $LimboHSM/MoveState
@onready var attack_state: LimboState = $LimboHSM/AttackState
@onready var dodge_state: LimboState = $LimboHSM/DodgeState

var can_dodge: bool = true
var attack_pressed: bool = false


func _ready() -> void:
	super._ready()
	can_dodge = true
	_init_state_machine()
	death.connect(func(): remove_from_group(&"player"))


func _unhandled_input(event: InputEvent) -> void:
	if event.is_echo():
		return
	if event.is_action_pressed("attack"):
		attack_pressed = true
		_process_attack_input()
	if event.is_action_pressed("dodge"):
		hsm.dispatch("dodge!")


func _process_attack_input() -> void:
	if not attack_pressed or hsm.get_active_state() == attack_state:
		return
	hsm.dispatch("attack!")
	attack_pressed = false


func _init_state_machine() -> void:
	hsm.add_transition(idle_state, move_state, idle_state.EVENT_FINISHED)
	hsm.add_transition(move_state, idle_state, move_state.EVENT_FINISHED)
	hsm.add_transition(idle_state, attack_state, "attack!")
	hsm.add_transition(move_state, attack_state, "attack!")
	hsm.add_transition(attack_state, move_state, attack_state.EVENT_FINISHED)
	hsm.add_transition(hsm.ANYSTATE, dodge_state, "dodge!")
	hsm.add_transition(dodge_state, move_state, dodge_state.EVENT_FINISHED)

	dodge_state.set_guard(_can_dodge)
	attack_state.set_guard(attack_state.can_enter)

	# Process attack input buffer when move_state is entered.
	# This way we can buffer the attack button presses and chain the attacks.
	move_state.call_on_enter(_process_attack_input)

	hsm.initialize(self)
	hsm.set_active(true)
	hsm.set_guard(_can_dodge)


func set_victorious() -> void:
	idle_state.idle_animation = &"dance"


func _can_dodge() -> bool:
	if can_dodge:
		can_dodge = false
		get_tree().create_timer(dodge_cooldown).timeout.connect(func(): can_dodge = true)
		return true
	return false
