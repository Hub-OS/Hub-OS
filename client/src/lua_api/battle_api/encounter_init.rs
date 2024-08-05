use super::field_api::get_field_table;
use super::{create_entity_table, BattleLuaApi, ENCOUNTER_TABLE, MUTATOR_TABLE, SPAWNER_TABLE};
use crate::battle::{BattleInitMusic, BattleScriptContext, Character, Entity};
use crate::bindable::{CharacterRank, EntityId};
use crate::lua_api::helpers::{absolute_path, inherit_metatable};
use crate::packages::PackageId;
use crate::render::{Animator, Background};
use crate::resources::{AssetManager, Globals};
use framework::prelude::Vec2;
use std::cell::RefCell;

pub fn encounter_init(api_ctx: BattleScriptContext, data: Option<&str>) {
    let globals = api_ctx.game_io.resource::<Globals>().unwrap();
    let battle_api = &globals.battle_api;

    let vms = api_ctx.resources.vm_manager.vms();
    let vm = &vms[api_ctx.vm_index];
    let lua = &vm.lua;

    let encounter_init: rollback_mlua::Function = match lua.globals().get("encounter_init") {
        Ok(encounter_init) => encounter_init,
        _ => {
            log::error!("Missing encounter_init() in {:?}", vm.path);
            return;
        }
    };

    let chunk = data.and_then(|data| {
        let chunk: Option<rollback_mlua::Value> = lua.load(data).eval().ok();

        if chunk.is_none() {
            log::error!("Failed to read data from server:\n{data}");
        }

        chunk
    });

    let context = RefCell::new(api_ctx);

    battle_api.inject_dynamic(lua, &context, |lua| {
        let init_table = lua.create_table()?;
        inherit_metatable(lua, ENCOUNTER_TABLE, &init_table)?;

        encounter_init.call((init_table, chunk))
    });
}

pub fn inject_encounter_init_api(lua_api: &mut BattleLuaApi) {
    inject_spawner_api(lua_api);
    inject_mutator_api(lua_api);

    lua_api.add_dynamic_function(ENCOUNTER_TABLE, "create_spawner", |_, lua, params| {
        let (_, package_id, rank): (rollback_mlua::Table, String, CharacterRank) =
            lua.unpack_multi(params)?;

        lua.pack_multi(create_spawner(lua, package_id, rank)?)
    });

    lua_api.add_dynamic_function(ENCOUNTER_TABLE, "player_count", |api_ctx, lua, params| {
        let _: rollback_mlua::Table = lua.unpack_multi(params)?;

        let api_ctx = api_ctx.borrow();
        lua.pack_multi(api_ctx.simulation.inputs.len())
    });

    lua_api.add_dynamic_function(ENCOUNTER_TABLE, "spawn_player", |api_ctx, lua, params| {
        let (_, player_index, x, y): (rollback_mlua::Table, usize, i32, i32) =
            lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        if !simulation.field.in_bounds((x, y)) {
            return lua.pack_multi(());
        }

        let config = &mut simulation.config;
        let spawn_pos = config.player_spawn_positions.get_mut(player_index);

        if let Some(spawn_pos) = spawn_pos {
            *spawn_pos = (x, y);
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENCOUNTER_TABLE, "set_background", |api_ctx, lua, params| {
        let (_, texture_path, animation_path, vel_x, vel_y): (
            rollback_mlua::Table,
            String,
            String,
            Option<f32>,
            Option<f32>,
        ) = lua.unpack_multi(params)?;

        let texture_path = absolute_path(lua, texture_path)?;
        let animation_path = absolute_path(lua, animation_path)?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let globals = api_ctx.game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let animator = Animator::load_new(assets, &animation_path);
        let sprite = assets.new_sprite(api_ctx.game_io, &texture_path);

        let mut background = Background::new(animator, sprite);

        if let (Some(vel_x), Some(vel_y)) = (vel_x, vel_y) {
            background.set_velocity(Vec2::new(vel_x, vel_y));
        }

        api_ctx.simulation.background = background;

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENCOUNTER_TABLE, "set_music", |api_ctx, lua, params| {
        let (_, path, loops): (rollback_mlua::Table, String, Option<bool>) =
            lua.unpack_multi(params)?;

        let path = absolute_path(lua, path)?;
        let loops = loops.unwrap_or(true);

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let game_io = api_ctx.game_io;
        let globals = game_io.resource::<Globals>().unwrap();

        simulation.config.battle_init_music = Some(BattleInitMusic {
            buffer: globals.assets.audio(game_io, &path),
            loops,
        });

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENCOUNTER_TABLE, "field", |_, lua, _| {
        let field_table = get_field_table(lua)?;

        lua.pack_multi(field_table)
    });

    lua_api.add_dynamic_function(
        ENCOUNTER_TABLE,
        "enable_automatic_turn_end",
        |api_ctx, lua, params| {
            let enabled: Option<bool> = lua.unpack_multi(params)?;

            let mut api_ctx = api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;
            simulation.config.automatic_turn_end = enabled.unwrap_or(true);

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(ENCOUNTER_TABLE, "set_turn_limit", |api_ctx, lua, params| {
        let (_, turn_limit): (rollback_mlua::Table, u32) = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        simulation.config.turn_limit = Some(turn_limit);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(
        ENCOUNTER_TABLE,
        "enable_flipping",
        |api_ctx, lua, params| {
            let (_, enable, player_index): (rollback_mlua::Table, Option<bool>, Option<usize>) =
                lua.unpack_multi(params)?;

            let enable = enable.unwrap_or(true);

            let mut api_ctx = api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;

            if let Some(index) = player_index {
                if let Some(flippable) = simulation.config.player_flippable.get_mut(index) {
                    *flippable = Some(enable);
                }
            } else {
                simulation.config.player_flippable.fill(Some(enable));
            }

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(ENCOUNTER_TABLE, "enable_boss_battle", |api_ctx, lua, _| {
        let mut api_ctx = api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        simulation.statistics.boss_battle = true;

        lua.pack_multi(())
    });
}

fn inject_spawner_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(SPAWNER_TABLE, "spawn_at", |api_ctx, lua, params| {
        let (table, x, y): (rollback_mlua::Table, i32, i32) = lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();

        let package_id: PackageId = table.get("#package_id")?;
        let rank = table.get("#rank")?;

        let vms = api_ctx.resources.vm_manager.vms();
        let vm = &vms[api_ctx.vm_index];
        let namespace = vm.preferred_namespace();

        let id = Character::load(
            api_ctx.game_io,
            api_ctx.resources,
            api_ctx.simulation,
            &package_id,
            namespace,
            rank,
        )?;

        let entities = &mut api_ctx.simulation.entities;
        let entity = entities.query_one_mut::<&mut Entity>(id.into()).unwrap();

        entity.x = x;
        entity.y = y;
        entity.pending_spawn = true;

        let mutator_table = create_mutator(lua, id)?;

        lua.pack_multi(mutator_table)
    });
}

fn create_spawner(
    lua: &rollback_mlua::Lua,
    package_id: String,
    rank: CharacterRank,
) -> rollback_mlua::Result<rollback_mlua::Table> {
    let table = lua.create_table()?;
    inherit_metatable(lua, SPAWNER_TABLE, &table)?;
    table.set("#package_id", package_id)?;
    table.set("#rank", rank)?;

    Ok(table)
}

pub fn inject_mutator_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(MUTATOR_TABLE, "mutate", |_, lua, params| {
        let (table, callback): (rollback_mlua::Table, rollback_mlua::Function) =
            lua.unpack_multi(params)?;

        let id: EntityId = table.get("#entity_id")?;

        let table = create_entity_table(lua, id)?;
        callback.call(table)?;

        lua.pack_multi(())
    });
}

fn create_mutator(
    lua: &rollback_mlua::Lua,
    id: EntityId,
) -> rollback_mlua::Result<rollback_mlua::Table> {
    let table = lua.create_table()?;
    table.set("#entity_id", id)?;
    inherit_metatable(lua, MUTATOR_TABLE, &table)?;

    Ok(table)
}
