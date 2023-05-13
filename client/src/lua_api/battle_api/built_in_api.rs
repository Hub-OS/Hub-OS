use crate::{
    battle::{AttackBox, BattleCallback, Entity, Spell, Component},
    bindable::{EntityId, ComponentLifetime},
    lua_api::create_entity_table,
    render::FrameTime,
};

use super::{BattleLuaApi, BUSTER_TABLE, HITBOX_TABLE, SHARED_HITBOX_TABLE, VIRUS_DEFENSE_TABLE};

macro_rules! built_in {
    ($lua_api:expr, $file_name:literal, $table_name:expr, $function_name:literal) => {{
        $lua_api.add_dynamic_function($table_name, $function_name, |_, lua, params| {
            let function = match lua.named_registry_value($file_name)? {
                rollback_mlua::Value::Function(function) => function,
                _ => {
                    let function = lua
                        .load(include_str!(concat!("built_in/", $file_name, ".lua")))
                        .set_name(concat!("built_in/", $file_name, ".lua"))?
                        .into_function()?;

                    lua.set_named_registry_value($file_name, function.clone())?;

                    function
                }
            };

            function.call(params)
        });
    }};
}

pub fn inject_built_in_api(lua_api: &mut BattleLuaApi) {
    built_in!(lua_api, "buster", BUSTER_TABLE, "new");
    built_in!(lua_api, "defense_virus_body", VIRUS_DEFENSE_TABLE, "new");
    built_in!(lua_api, "hitbox", HITBOX_TABLE, "new");

    lua_api.add_dynamic_function(SHARED_HITBOX_TABLE, "new", |api_ctx, lua, params| {
        let (entity_table, lifetime): (rollback_mlua::Table, Option<FrameTime>) =
            lua.unpack_multi(params)?;

        let parent_id: EntityId = entity_table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        let hitbox_id = simulation.create_spell(api_ctx.game_io);

        let (hitbox_entity, hitbox_spell) = simulation
            .entities
            .query_one_mut::<(&mut Entity, &mut Spell)>(hitbox_id.into())
            .unwrap();

        if let Some(lifetime) = lifetime {
            hitbox_entity.spawn_callback = BattleCallback::new(move |_, simulation, _, _| {
                Component::new_delayed_deleter(simulation, hitbox_id, ComponentLifetime::Local, lifetime);
            });
        }

        hitbox_entity.update_callback = BattleCallback::new(move |game_io, simulation, vms, _| {
            // get properties from the parent
            let Ok((entity, spell)) = simulation.entities.query_one_mut::<(&Entity, &Spell)>(parent_id.into()) else {
                // delete if the parent is deleted
                simulation.delete_entity(game_io, vms, hitbox_id);
                return;
            };

            if entity.deleted {
                // delete if the parent is deleted
                simulation.delete_entity(game_io, vms, hitbox_id);
                return;
            }

            let team = entity.team;
            let hit_props = spell.hit_props.clone();

            // apply to the shared hitbox
            let Ok((entity, spell)) = simulation.entities.query_one_mut::<(&mut Entity, &mut Spell)>(hitbox_id.into()) else {
                return;
            };

            entity.team = team;
            spell.hit_props = hit_props;
            
            // attack the current tile
            let (x, y) = (entity.x, entity.y);

            let Some(tile) = simulation.field.tile_at_mut((x, y)) else {
                return;
            };

            if !tile.ignoring_attacker(hitbox_id) {
                let attack_box = AttackBox::new_from((x, y), entity, spell);
                simulation.queued_attacks.push(attack_box);
            }

        });

        // forward attack callback
        hitbox_spell.attack_callback = BattleCallback::new(move |game_io, simulation, vms, params| {
            let Ok(spell) = simulation.entities.query_one_mut::<&Spell>(parent_id.into()) else {
                return;
            };

            spell.attack_callback.clone().call(game_io, simulation, vms, params);
        });

        // forward collision callback
        hitbox_spell.collision_callback = BattleCallback::new(move |game_io, simulation, vms, params| {
            let Ok(spell) = simulation.entities.query_one_mut::<&Spell>(parent_id.into()) else {
                return;
            };

            spell.collision_callback.clone().call(game_io, simulation, vms, params);
        });

        let hitbox_table = create_entity_table(lua, hitbox_id);

        lua.pack_multi(hitbox_table)
    });
}
