use super::field_api::get_field_table;
use super::{create_entity_table, BattleLuaApi, BATTLE_INIT_TABLE, MUTATOR_TABLE, SPAWNER_TABLE};
use crate::battle::{BattleScriptContext, Entity};
use crate::bindable::{CharacterRank, EntityId};
use crate::lua_api::helpers::{absolute_path, inherit_metatable};
use crate::packages::PackageId;
use crate::render::{Animator, Background};
use crate::resources::{AssetManager, Globals};
use framework::prelude::Vec2;
use std::cell::RefCell;

pub fn battle_init(context: BattleScriptContext, data: Option<String>) {
    let globals = context.game_io.resource::<Globals>().unwrap();
    let battle_api = &globals.battle_api;

    let vm = &context.vms[context.vm_index];
    let lua = &vm.lua;

    let battle_init: rollback_mlua::Function = match lua.globals().get("battle_init") {
        Ok(battle_init) => battle_init,
        _ => {
            log::error!("Missing battle_init() in {:?}", vm.path);
            return;
        }
    };

    let chunk = data.and_then(|data| {
        let chunk: Option<rollback_mlua::Value> = lua.load(&data).eval().ok();

        if chunk.is_none() {
            log::error!("Failed to read data from server:\n{data}");
        }

        chunk
    });

    let context = RefCell::new(context);

    battle_api.inject_dynamic(lua, &context, |lua| {
        lua.scope(|_| {
            let init_table = lua.create_table()?;
            inherit_metatable(lua, BATTLE_INIT_TABLE, &init_table)?;

            battle_init.call((init_table, chunk))
        })
    });
}

pub fn inject_battle_init_api(lua_api: &mut BattleLuaApi) {
    inject_spawner_api(lua_api);
    inject_mutator_api(lua_api);

    lua_api.add_dynamic_function(BATTLE_INIT_TABLE, "create_spawner", |_, lua, params| {
        let (_, package_id, rank): (rollback_mlua::Table, String, CharacterRank) =
            lua.unpack_multi(params)?;

        lua.pack_multi(create_spawner(lua, package_id, rank)?)
    });

    lua_api.add_dynamic_function(BATTLE_INIT_TABLE, "set_panels", |api_ctx, lua, params| {
        let (_, texture_paths, animation_path, spacing_x, spacing_y): (
            rollback_mlua::Table,
            [String; 3],
            String,
            f32,
            f32,
        ) = lua.unpack_multi(params)?;

        let [red_texture_path, blue_texture_path, other_texture_path] = texture_paths;

        let red_texture_path = absolute_path(lua, red_texture_path)?;
        let blue_texture_path = absolute_path(lua, blue_texture_path)?;
        let other_texture_path = absolute_path(lua, other_texture_path)?;
        let animation_path = absolute_path(lua, animation_path)?;
        let spacing = Vec2::new(spacing_x, spacing_y);

        let api_ctx = &mut *api_ctx.borrow_mut();
        let game_io = api_ctx.game_io;
        let field = &mut api_ctx.simulation.field;

        field.set_sprites(
            game_io,
            &red_texture_path,
            &blue_texture_path,
            &other_texture_path,
            &animation_path,
            spacing,
        );

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(
        BATTLE_INIT_TABLE,
        "set_background",
        |api_ctx, lua, params| {
            let (_, texture_path, animation_path, vel_x, vel_y): (
                rollback_mlua::Table,
                String,
                String,
                f32,
                f32,
            ) = lua.unpack_multi(params)?;

            let texture_path = absolute_path(lua, texture_path)?;
            let animation_path = absolute_path(lua, animation_path)?;

            let mut api_ctx = &mut *api_ctx.borrow_mut();
            let globals = api_ctx.game_io.resource::<Globals>().unwrap();
            let assets = &globals.assets;

            let animator = Animator::load_new(assets, &animation_path);
            let sprite = assets.new_sprite(api_ctx.game_io, &texture_path);

            let mut background = Background::new(animator, sprite);
            background.set_velocity(Vec2::new(vel_x, vel_y));
            api_ctx.simulation.background = background;

            lua.pack_multi(())
        },
    );

    //   "stream_music", [](ScriptedMob& mob, const std::string& path, std::optional<long long> startMs, std::optional<long long> endMs) {
    //     mob.StreamMusic(path, startMs.value_or(-1), endMs.value_or(-1));
    //   },

    lua_api.add_dynamic_function(BATTLE_INIT_TABLE, "get_field", |_, lua, _| {
        let field_table = get_field_table(lua)?;

        lua.pack_multi(field_table)
    });

    // "set_turn_limit", &ScriptedMob::EnableFreedomMission,

    lua_api.add_dynamic_function(BATTLE_INIT_TABLE, "spawn_player", |api_ctx, lua, params| {
        let (_, player_index, x, y): (rollback_mlua::Table, usize, i32, i32) =
            lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        if !simulation.field.in_bounds((x, y)) {
            return lua.pack_multi(());
        }

        let spawn_pos = simulation.player_spawn_positions.get_mut(player_index);

        if let Some(spawn_pos) = spawn_pos {
            *spawn_pos = (x, y);
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(
        BATTLE_INIT_TABLE,
        "enable_boss_battle",
        |api_ctx, lua, _| {
            let mut api_ctx = api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;

            simulation.statistics.boss_battle = true;

            lua.pack_multi(())
        },
    );
}

fn inject_spawner_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(SPAWNER_TABLE, "spawn_at", |api_ctx, lua, params| {
        let (table, x, y): (rollback_mlua::Table, i32, i32) = lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();

        let package_id: PackageId = table.get("#package_id")?;
        let rank = table.get("#rank")?;

        let namespace = api_ctx.vms[api_ctx.vm_index].namespace;

        let id = api_ctx.simulation.load_character(
            api_ctx.game_io,
            api_ctx.vms,
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
