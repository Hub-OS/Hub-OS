use super::BattleSimulation;
use super::{rollback_vm::RollbackVM, BattleScriptContext};
use crate::bindable::{DefensePriority, EntityID, HitFlag, HitProperties};
use crate::lua_api::{create_entity_table, DEFENSE_JUDGE_TABLE};
use crate::resources::Globals;
use framework::prelude::GameIO;
use std::cell::RefCell;
use std::sync::Arc;

#[derive(Clone)]
pub struct DefenseRule {
    pub collision_only: bool,
    pub defense_priority: DefensePriority,
    pub vm_index: usize,
    pub table: Arc<rollback_mlua::RegistryKey>,
}

#[derive(Clone, Copy)]
pub struct DefenseJudge {
    pub impact_blocked: bool,
    pub damage_blocked: bool,
}

impl DefenseJudge {
    pub fn new() -> Self {
        Self {
            impact_blocked: false,
            damage_blocked: false,
        }
    }

    pub fn judge(
        game_io: &GameIO<Globals>,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],

        defender_id: EntityID,
        attacker_id: EntityID,
        defense_rules: &[DefenseRule],
        collision_only: bool,
    ) {
        let lua_api = &game_io.globals().battle_api;

        for defense_rule in defense_rules {
            if defense_rule.collision_only != collision_only {
                continue;
            }

            let context = RefCell::new(BattleScriptContext {
                vm_index: defense_rule.vm_index,
                vms,
                game_io,
                simulation,
            });

            let lua = &vms[defense_rule.vm_index].lua;

            let table: rollback_mlua::Table = lua.registry_value(&defense_rule.table).unwrap();

            lua_api.inject_dynamic(lua, &context, |lua| {
                let callback: rollback_mlua::Function = table.get("can_block_func")?;

                let judge_table: rollback_mlua::Table = lua.globals().get(DEFENSE_JUDGE_TABLE)?;

                let attacker_table = create_entity_table(lua, attacker_id)?;
                let defender_table = create_entity_table(lua, defender_id)?;

                callback.call::<_, ()>((judge_table, attacker_table, defender_table))?;

                Ok(())
            });
        }
    }

    pub fn filter_statuses(
        game_io: &GameIO<Globals>,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
        props: &mut HitProperties,
        defense_rules: &[DefenseRule],
    ) {
        let lua_api = &game_io.globals().battle_api;
        let no_counter = props.flags & HitFlag::NO_COUNTER;

        for defense_rule in defense_rules {
            let context = RefCell::new(BattleScriptContext {
                vm_index: defense_rule.vm_index,
                vms,
                game_io,
                simulation,
            });

            let lua = &vms[defense_rule.vm_index].lua;

            let table: rollback_mlua::Table = lua.registry_value(&defense_rule.table).unwrap();

            lua_api.inject_dynamic(lua, &context, |_| {
                let callback: rollback_mlua::Function = table.get("filter_statuses_func")?;

                *props = callback.call(&*props)?;

                Ok(())
            });
        }

        // prevent accidental overwrite of this internal flag
        props.flags |= no_counter;
    }
}
