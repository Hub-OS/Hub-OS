use super::tile_api::create_tile_table;
use super::{
    BattleLuaApi, BUSTER_TABLE, HITBOX_TABLE, SHARED_HITBOX_TABLE, STANDARD_ENEMY_AUX_TABLE,
};
use crate::battle::{AttackBox, BattleCallback, Component, Entity, Spell};
use crate::bindable::{ComponentLifetime, EntityId};
use crate::lua_api::{
    create_entity_table, BattleVmManager, AUGMENT_TABLE, AUX_PROP_TABLE, ENTITY_TABLE,
    PLAYER_FORM_TABLE,
};
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

macro_rules! internal_script {
    ($lua:expr, $file_name:literal) => {
        $lua.load(include_str!(concat!("built_in/", $file_name, ".lua")))
            .set_name(concat!("built_in/", $file_name, ".lua"))
            .into_function()
    };
}

macro_rules! built_in_method {
    ($lua_api:expr, $file_name:literal, $table_names:expr) => {{
        $lua_api.add_static_injector(|lua| {
            let function = internal_script!(lua, $file_name)?;

            let globals = lua.globals();

            for name in $table_names {
                let table: rollback_mlua::Table = globals.get(name)?;
                table.raw_set($file_name, function.clone())?;
            }

            Ok(())
        });
    }};
}

pub fn inject_internal_scripts(vm_manager: &mut BattleVmManager) -> rollback_mlua::Result<()> {
    let lua = &vm_manager.vms[0].lua;

    let delete_player_fn = internal_script!(lua, "default_player_delete")?;
    vm_manager.scripts.default_player_delete =
        BattleCallback::new_transformed_lua_callback(lua, 0, delete_player_fn, |_, lua, id| {
            lua.pack_multi(create_entity_table(lua, id)?)
        })?;

    let delete_character_fn = internal_script!(lua, "default_character_delete")?;
    vm_manager.scripts.default_character_delete = BattleCallback::new_transformed_lua_callback(
        lua,
        0,
        delete_character_fn,
        |_, lua, (id, explosion_count)| {
            lua.pack_multi((create_entity_table(lua, id)?, explosion_count))
        },
    )?;

    let queue_movement_fn = internal_script!(lua, "queue_default_player_movement")?;
    vm_manager.scripts.queue_default_player_movement =
        BattleCallback::new_transformed_lua_callback(
            lua,
            0,
            queue_movement_fn,
            |_, lua, (id, tile_position)| {
                lua.pack_multi((
                    create_entity_table(lua, id)?,
                    create_tile_table(lua, tile_position)?,
                ))
            },
        )?;

    Ok(())
}

pub fn inject_built_in_api(lua_api: &mut BattleLuaApi) {
    built_in_table!(lua_api, "buster", BUSTER_TABLE);
    built_in_table!(lua_api, "standard_enemy_aux", STANDARD_ENEMY_AUX_TABLE);
    built_in_table!(lua_api, "hitbox", HITBOX_TABLE);
    built_in_table!(lua_api, "aux_prop", AUX_PROP_TABLE);

    built_in_method!(
        lua_api,
        "set_fixed_card",
        [ENTITY_TABLE, PLAYER_FORM_TABLE, AUGMENT_TABLE]
    );

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
                    Entity::delete(game_io, resources, simulation, hitbox_id);
                    return;
                };

                if entity.deleted {
                    // delete if the parent is deleted
                    Entity::delete(game_io, resources, simulation, hitbox_id);
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

                let attack_box = AttackBox::new_from((x, y), hitbox_id, entity, spell);
                simulation.queued_attacks.push(attack_box);
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
