use super::{BattleLuaApi, BUSTER_TABLE, HITBOX_TABLE, SHARED_HITBOX_TABLE, VIRUS_DEFENSE_TABLE};
use crate::battle::{AttackBox, BattleCallback, Component, Entity, Spell};
use crate::bindable::{ComponentLifetime, EntityId};
use crate::lua_api::{create_entity_table, AUX_MATH_TABLE, AUX_PROP_TABLE};
use crate::render::FrameTime;

// lazy loader for built in tables
macro_rules! built_in_table {
    ($lua_api:expr, $file_name:literal, $table_name:expr) => {{
        $lua_api.add_static_injector(|lua| {
            let meta_table = lua.create_table()?;

            fn load_table(lua: &rollback_mlua::Lua) -> rollback_mlua::Result<rollback_mlua::Table> {
                let function = lua
                    .load(include_str!(concat!("built_in/", $file_name, ".lua")))
                    .set_name(concat!("built_in/", $file_name, ".lua"))
                    .into_function()?;

                // overwrite the table to speed up future calls
                let table: rollback_mlua::Table = function.call(())?;
                lua.globals().set($table_name, table.clone())?;

                Ok(table)
            }

            meta_table.set(
                "__index",
                lua.create_function(
                    |lua, (_, key): (rollback_mlua::Table, rollback_mlua::String)| {
                        let table = load_table(lua)?;
                        let value: rollback_mlua::Value = table.get(key)?;
                        Ok(value)
                    },
                )?,
            )?;

            meta_table.set(
                "__newindex",
                lua.create_function(
                    |lua,
                     (_, key, value): (
                        rollback_mlua::Table,
                        rollback_mlua::String,
                        rollback_mlua::Value,
                    )| {
                        let table = load_table(lua)?;
                        table.set(key, value)?;
                        Ok(())
                    },
                )?,
            )?;

            let table = lua.create_table()?;
            table.set_metatable(Some(meta_table));
            lua.globals().set($table_name, table)?;

            Ok(())
        });
    }};
}

pub fn inject_built_in_api(lua_api: &mut BattleLuaApi) {
    built_in_table!(lua_api, "buster", BUSTER_TABLE);
    built_in_table!(lua_api, "defense_virus_body", VIRUS_DEFENSE_TABLE);
    built_in_table!(lua_api, "hitbox", HITBOX_TABLE);
    built_in_table!(lua_api, "aux_prop", AUX_PROP_TABLE);
    built_in_table!(lua_api, "aux_math", AUX_MATH_TABLE);

    lua_api.add_dynamic_function(SHARED_HITBOX_TABLE, "new", |api_ctx, lua, params| {
        let (entity_table, lifetime): (rollback_mlua::Table, Option<FrameTime>) =
            lua.unpack_multi(params)?;

        let parent_id: EntityId = entity_table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        let hitbox_id = Spell::create(api_ctx.game_io, simulation);

        let (hitbox_entity, hitbox_spell) = simulation
            .entities
            .query_one_mut::<(&mut Entity, &mut Spell)>(hitbox_id.into())
            .unwrap();

        if let Some(lifetime) = lifetime {
            hitbox_entity.spawn_callback = BattleCallback::new(move |_, _, simulation, _| {
                Component::create_delayed_deleter(
                    simulation,
                    hitbox_id,
                    ComponentLifetime::Local,
                    lifetime,
                );
            });
        }

        hitbox_entity.update_callback =
            BattleCallback::new(move |game_io, resources, simulation, _| {
                // get properties from the parent
                let entities = &mut simulation.entities;
                let Ok((entity, spell)) =
                    entities.query_one_mut::<(&Entity, &Spell)>(parent_id.into())
                else {
                    // delete if the parent is deleted
                    simulation.delete_entity(game_io, resources, hitbox_id);
                    return;
                };

                if entity.deleted {
                    // delete if the parent is deleted
                    simulation.delete_entity(game_io, resources, hitbox_id);
                    return;
                }

                let team = entity.team;
                let hit_props = spell.hit_props.clone();

                // apply to the shared hitbox
                let entities = &mut simulation.entities;
                let Ok((entity, spell)) =
                    entities.query_one_mut::<(&mut Entity, &mut Spell)>(hitbox_id.into())
                else {
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
        hitbox_spell.attack_callback =
            BattleCallback::new(move |game_io, resources, simulation, params| {
                let entities = &mut simulation.entities;
                let Ok(spell) = entities.query_one_mut::<&Spell>(parent_id.into()) else {
                    return;
                };

                let callback = spell.attack_callback.clone();
                callback.call(game_io, resources, simulation, params);
            });

        // forward collision callback
        hitbox_spell.collision_callback =
            BattleCallback::new(move |game_io, resources, simulation, params| {
                let entities = &mut simulation.entities;
                let Ok(spell) = entities.query_one_mut::<&Spell>(parent_id.into()) else {
                    return;
                };

                let callback = spell.collision_callback.clone();
                callback.call(game_io, resources, simulation, params);
            });

        let hitbox_table = create_entity_table(lua, hitbox_id);

        lua.pack_multi(hitbox_table)
    });
}
