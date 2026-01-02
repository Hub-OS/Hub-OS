use super::BattleScriptContext;
use super::{BattleSimulation, Entity, Living, SharedBattleResources};
use crate::battle::EmotionWindow;
use crate::bindable::{DefensePriority, EntityId, HitFlag, HitProperties, Team};
use crate::lua_api::{
    DEFENSE_FN, DEFENSE_TABLE, FILTER_HIT_PROPS_FN, REPLACE_FN, create_entity_table,
};
use crate::render::SpriteColorQueue;
use crate::render::ui::{FontName, TextStyle};
use crate::resources::{BATTLE_INFO_SHADOW_COLOR, Globals, RESOLUTION_F};
use framework::prelude::GameIO;
use rollback_mlua::prelude::{LuaFunction, LuaRegistryKey, LuaResult, LuaTable};
use std::cell::RefCell;
use std::sync::Arc;

#[derive(Clone)]
pub struct DefenseRule {
    pub collision_only: bool,
    pub priority: DefensePriority,
    pub vm_index: usize,
    pub table: Arc<LuaRegistryKey>,
}

impl DefenseRule {
    pub fn add(
        api_ctx: &mut BattleScriptContext,
        lua: &rollback_mlua::Lua,
        defense_table: LuaTable,
        entity_id: EntityId,
    ) -> LuaResult<()> {
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let Ok(living) = entities.query_one_mut::<&mut Living>(entity_id.into()) else {
            return Ok(());
        };

        let key = lua.create_registry_value(defense_table.clone())?;

        let mut rule = DefenseRule {
            collision_only: defense_table.get("#collision_only")?,
            priority: defense_table.get("#priority")?,
            vm_index: api_ctx.vm_index,
            table: Arc::new(key),
        };

        if rule.priority == DefensePriority::Last {
            living.defense_rules.push(rule);
            return Ok(());
        }

        let priority = rule.priority;

        if let Some(index) = living
            .defense_rules
            .iter()
            .position(|r| r.priority >= priority)
        {
            // there's a rule with the same or greater priority
            let existing_rule = &mut living.defense_rules[index];

            if existing_rule.priority > rule.priority {
                // greater priority, we'll insert just before
                living.defense_rules.insert(index, rule);
            } else {
                // same priority, we'll replace
                std::mem::swap(existing_rule, &mut rule);

                // call the on_replace_func on the old rule
                rule.call_on_replace(api_ctx.game_io, api_ctx.resources, simulation);
            }
        } else {
            // nothing should exist after this rule, just append
            living.defense_rules.push(rule);
        }

        Ok(())
    }

    pub fn remove(
        api_ctx: &mut BattleScriptContext,
        lua: &rollback_mlua::Lua,
        defense_table: LuaTable,
        entity_id: EntityId,
    ) -> LuaResult<()> {
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let Ok(living) = entities.query_one_mut::<&mut Living>(entity_id.into()) else {
            return Ok(());
        };

        let priority = defense_table.get("#priority")?;

        let similar_rule_index = living
            .defense_rules
            .iter()
            .position(|rule| rule.vm_index == api_ctx.vm_index && rule.priority == priority);

        if let Some(index) = similar_rule_index {
            let existing_rule = &living.defense_rules[index];

            if lua.registry_value::<LuaTable>(&existing_rule.table)? == defense_table {
                living.defense_rules.remove(index);
            }
        }

        Ok(())
    }

    pub fn remove_priority(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        priority: DefensePriority,
    ) {
        let entities = &mut simulation.entities;

        let Ok(living) = entities.query_one_mut::<&mut Living>(entity_id.into()) else {
            return;
        };

        let rule_index = living
            .defense_rules
            .iter()
            .position(|rule| rule.priority == priority);

        if let Some(index) = rule_index {
            let rule = living.defense_rules.remove(index);

            // call the on_replace_func on the old rule
            rule.call_on_replace(game_io, resources, simulation);
        }
    }

    pub fn has_priority(
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        priority: DefensePriority,
    ) -> bool {
        let entities = &mut simulation.entities;

        let Ok(living) = entities.query_one_mut::<&Living>(entity_id.into()) else {
            return false;
        };

        living
            .defense_rules
            .iter()
            .any(|rule| rule.priority == priority)
    }

    fn call_on_replace(
        &self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let lua_api = &Globals::from_resources(game_io).battle_api;

        let context = RefCell::new(BattleScriptContext {
            vm_index: self.vm_index,
            resources,
            game_io,
            simulation,
        });

        let vms = resources.vm_manager.vms();
        let lua = &vms[self.vm_index].lua;

        let table: LuaTable = lua.registry_value(&self.table).unwrap();

        lua_api.inject_dynamic(lua, &context, |_| {
            table.raw_set("#replaced", true)?;

            if let Ok(callback) = table.get::<_, LuaFunction>(REPLACE_FN) {
                callback.call::<_, ()>(())?;
            };

            Ok(())
        });
    }

    fn team_has_trap(
        simulation: &mut BattleSimulation,
        team_filter: impl Fn(Team) -> bool,
    ) -> bool {
        type Query<'a> = (&'a Entity, &'a Living);
        let entities = &mut simulation.entities;

        entities
            .query_mut::<Query>()
            .into_iter()
            .any(|(_, (entity, living))| {
                let mut defense_rule_iter = living.defense_rules.iter();

                team_filter(entity.team)
                    && defense_rule_iter.any(|rule| rule.priority == DefensePriority::Trap)
            })
    }

    pub fn draw_trap_ui(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        sprite_queue: &mut SpriteColorQueue,
    ) {
        // draw ???? to indicate a trap exists for each side with a trap
        let mut text_style = TextStyle::new(game_io, FontName::Thick);
        text_style.shadow_color = BATTLE_INFO_SHADOW_COLOR;

        let health_bounds = simulation.local_health_ui.bounds();
        text_style.bounds.y = health_bounds.bottom() + 3.0;

        const TEXT: &str = "????";
        let text_width = text_style.measure(TEXT).size.x;

        let local_team = simulation.local_team;
        let left_trap = Self::team_has_trap(simulation, |team| team == local_team);
        let right_trap = Self::team_has_trap(simulation, |team| team != local_team);

        if left_trap {
            // default position to the start of the health ui
            text_style.bounds.x = health_bounds.left();

            // move to the start of the turn gauge if there's an emotion displayed
            let player_id = simulation.local_player_id.into();
            let entities = &mut simulation.entities;

            if let Ok(emotion_window) = entities.query_one_mut::<&EmotionWindow>(player_id) {
                let emotion = emotion_window.emotion();

                if emotion_window.has_emotion(emotion) {
                    let gauge_bounds = simulation.turn_gauge.bounds();
                    text_style.bounds.x = gauge_bounds.left();
                }
            }

            text_style.draw(game_io, sprite_queue, TEXT);
        }

        if right_trap {
            text_style.bounds.x = RESOLUTION_F.x - text_width - health_bounds.left();
            text_style.draw(game_io, sprite_queue, TEXT);
        }
    }
}

#[derive(Clone, Copy)]
pub struct Defense {
    pub responded: bool,
    pub damage_blocked: bool,
}

impl Defense {
    pub fn new() -> Self {
        Self {
            responded: false,
            damage_blocked: false,
        }
    }

    #[allow(clippy::too_many_arguments)]
    pub fn defend(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        defender_id: EntityId,
        attacker_id: Option<EntityId>,
        hit_props: &HitProperties,
        defense_rules: &[DefenseRule],
        collision_only: bool,
    ) {
        let lua_api = &Globals::from_resources(game_io).battle_api;

        for defense_rule in defense_rules {
            if defense_rule.collision_only != collision_only {
                continue;
            }

            let context = RefCell::new(BattleScriptContext {
                vm_index: defense_rule.vm_index,
                resources,
                game_io,
                simulation,
            });

            let vms = resources.vm_manager.vms();
            let lua = &vms[defense_rule.vm_index].lua;

            let table: LuaTable = lua.registry_value(&defense_rule.table).unwrap();

            lua_api.inject_dynamic(lua, &context, |lua| {
                let Ok(callback): LuaResult<LuaFunction> = table.get(DEFENSE_FN) else {
                    return Ok(());
                };

                let defense_table: LuaTable = lua.globals().get(DEFENSE_TABLE)?;

                let attacker_table = if let Some(attacker_id) = attacker_id {
                    Some(create_entity_table(lua, attacker_id)?)
                } else {
                    None
                };

                let defender_table = create_entity_table(lua, defender_id)?;

                callback.call::<_, ()>((
                    defense_table,
                    attacker_table,
                    defender_table,
                    hit_props,
                ))?;

                Ok(())
            });
        }
    }

    pub fn filter_statuses(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        props: &mut HitProperties,
        defense_rules: &[DefenseRule],
    ) {
        let lua_api = &Globals::from_resources(game_io).battle_api;
        let no_counter = props.flags & HitFlag::NO_COUNTER;

        for defense_rule in defense_rules {
            let context = RefCell::new(BattleScriptContext {
                vm_index: defense_rule.vm_index,
                resources,
                game_io,
                simulation,
            });

            let vms = resources.vm_manager.vms();
            let lua = &vms[defense_rule.vm_index].lua;

            let table: LuaTable = lua.registry_value(&defense_rule.table).unwrap();

            lua_api.inject_dynamic(lua, &context, |_| {
                let Ok(callback): LuaResult<LuaFunction> = table.get(FILTER_HIT_PROPS_FN) else {
                    return Ok(());
                };

                *props = callback.call(&*props)?;

                // negative values are not allowed to prevent accidental healing and incorrect logic
                if props.damage < 0 {
                    log::warn!("{FILTER_HIT_PROPS_FN} returned hit props with negative damage");
                    props.damage = 0;
                }

                Ok(())
            });
        }

        // prevent accidental overwrite of this internal flag
        props.flags |= no_counter;
    }
}
