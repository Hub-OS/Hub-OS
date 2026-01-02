use super::errors::{encounter_method_called_after_start, server_communication_closed};
use super::field_api::get_field_compat_table;
use super::{BattleLuaApi, ENCOUNTER_TABLE, MUTATOR_TABLE, SPAWNER_TABLE, create_entity_table};
use crate::battle::{
    BattleInitMusic, BattleProgress, BattleScriptContext, BattleSimulation, Character, Entity,
    SharedBattleResources, TileState, TileStateAnimationSupport,
};
use crate::bindable::{CharacterRank, EntityId, TimeFreezeChainLimit};
use crate::lua_api::helpers::{absolute_path, inherit_metatable, lua_value_to_string};
use crate::packages::PackageId;
use crate::render::{Animator, Background};
use crate::resources::{AssetManager, Globals};
use framework::common::GameIO;
use framework::prelude::Vec2;
use std::cell::RefCell;

const BATTLE_END_LISTENERS: &str = "#battle_end_listeners";
const EXTERNAL_LISTENERS: &str = "#external_listeners";

pub fn encounter_init(api_ctx: BattleScriptContext, data: Option<&str>) {
    let globals = Globals::from_resources(api_ctx.game_io);
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

        let api_ctx = api_ctx.borrow();

        if api_ctx.simulation.time > 0 {
            return Err(encounter_method_called_after_start());
        }

        if !api_ctx.simulation.field.in_bounds((x, y)) {
            return lua.pack_multi(());
        }

        let config = &mut *api_ctx.resources.config.borrow_mut();
        let spawn_pos = config.player_spawn_positions.get_mut(player_index);

        if let Some(spawn_pos) = spawn_pos {
            *spawn_pos = (x, y);
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENCOUNTER_TABLE, "mark_spectator", |api_ctx, lua, params| {
        let (_, player_index): (rollback_mlua::Table, usize) = lua.unpack_multi(params)?;

        let api_ctx = api_ctx.borrow();

        if api_ctx.simulation.time > 0 {
            return Err(encounter_method_called_after_start());
        }

        let config = &mut *api_ctx.resources.config.borrow_mut();
        config.spectators.insert(player_index);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(
        ENCOUNTER_TABLE,
        "set_spectate_on_delete",
        |api_ctx, lua, params| {
            let (_, spectate): (rollback_mlua::Table, Option<bool>) = lua.unpack_multi(params)?;

            let api_ctx = api_ctx.borrow();

            if api_ctx.simulation.time > 0 {
                return Err(encounter_method_called_after_start());
            }

            let config = &mut *api_ctx.resources.config.borrow_mut();
            config.spectate_on_delete = spectate.unwrap_or(true);

            lua.pack_multi(())
        },
    );

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
        let globals = Globals::from_resources(api_ctx.game_io);
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
        let game_io = api_ctx.game_io;
        let globals = Globals::from_resources(game_io);

        if api_ctx.simulation.time > 0 {
            return Err(encounter_method_called_after_start());
        }

        let config = &mut *api_ctx.resources.config.borrow_mut();
        config.battle_init_music = Some(BattleInitMusic {
            buffer: globals.assets.audio(game_io, &path),
            loops,
        });

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENCOUNTER_TABLE, "set_field_size", |api_ctx, lua, params| {
        let (_, width, height): (rollback_mlua::Table, u8, u8) = lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        simulation
            .field
            .resize(width.max(1) as _, height.max(1) as _);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENCOUNTER_TABLE, "field", |_, lua, _| {
        let field_table = get_field_compat_table(lua)?;

        lua.pack_multi(field_table)
    });

    lua_api.add_dynamic_function(ENCOUNTER_TABLE, "set_tile_size", |api_ctx, lua, params| {
        let (_, width, height): (rollback_mlua::Table, f32, f32) = lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        simulation.field.set_tile_size(Vec2::new(width, height));

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(
        ENCOUNTER_TABLE,
        "set_tile_state_resources",
        |api_ctx, lua, params| {
            let (_, state_index, texture_path, animation_path): (
                rollback_mlua::Table,
                usize,
                String,
                String,
            ) = lua.unpack_multi(params)?;
            let texture_path = absolute_path(lua, texture_path)?;
            let animation_path = absolute_path(lua, animation_path)?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;
            let game_io = &api_ctx.game_io;

            let globals = Globals::from_resources(game_io);
            let texture = globals.assets.texture(game_io, &texture_path);
            let animator = Animator::load_new(&globals.assets, &animation_path);

            if state_index == TileState::NORMAL {
                let field = &mut api_ctx.simulation.field;
                field.set_tile_filled_resources(texture, animator);
            } else if state_index == TileState::HOLE {
                let field = &mut api_ctx.simulation.field;
                field.set_tile_frame_resources(texture, animator);
            } else if let Some(state) = simulation.tile_states.get_mut(state_index) {
                state.texture = texture;
                state.animation_support = TileStateAnimationSupport::from_animator(&animator);
                state.animator = animator;
            }

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        ENCOUNTER_TABLE,
        "enable_automatic_turn_end",
        |api_ctx, lua, params| {
            let (_, enabled): (rollback_mlua::Table, Option<bool>) = lua.unpack_multi(params)?;

            let api_ctx = api_ctx.borrow();

            if api_ctx.simulation.time > 0 {
                return Err(encounter_method_called_after_start());
            }

            let config = &mut *api_ctx.resources.config.borrow_mut();
            config.automatic_turn_end = enabled.unwrap_or(true);

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(ENCOUNTER_TABLE, "set_turn_limit", |api_ctx, lua, params| {
        let (_, turn_limit): (rollback_mlua::Table, u32) = lua.unpack_multi(params)?;

        let api_ctx = api_ctx.borrow();

        if api_ctx.simulation.time > 0 {
            return Err(encounter_method_called_after_start());
        }

        let config = &mut *api_ctx.resources.config.borrow_mut();
        config.turn_limit = Some(turn_limit);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(
        ENCOUNTER_TABLE,
        "set_time_freeze_chain_limit",
        |api_ctx, lua, params| {
            let (_, chain_limit): (rollback_mlua::Table, TimeFreezeChainLimit) =
                lua.unpack_multi(params)?;

            let mut api_ctx = api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;
            simulation.time_freeze_tracker.set_chain_limit(chain_limit);

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        ENCOUNTER_TABLE,
        "set_team_owner_limit",
        |api_ctx, lua, params| {
            let (_, limit): (rollback_mlua::Table, u8) = lua.unpack_multi(params)?;

            let mut api_ctx = api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;
            simulation.ownership_tracking.team_owned_limit = limit;

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        ENCOUNTER_TABLE,
        "set_entity_owner_limit",
        |api_ctx, lua, params| {
            let (_, limit): (rollback_mlua::Table, u8) = lua.unpack_multi(params)?;

            let mut api_ctx = api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;
            simulation.ownership_tracking.entity_owned_limit = limit;

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        ENCOUNTER_TABLE,
        "set_entities_share_ownership",
        |api_ctx, lua, params| {
            let (_, share): (rollback_mlua::Table, Option<bool>) = lua.unpack_multi(params)?;

            let mut api_ctx = api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;
            simulation.ownership_tracking.share_entity_limit = share.unwrap_or(true);

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        ENCOUNTER_TABLE,
        "enable_flipping",
        |api_ctx, lua, params| {
            let (_, enable, player_index): (rollback_mlua::Table, Option<bool>, Option<usize>) =
                lua.unpack_multi(params)?;

            let enable = enable.unwrap_or(true);

            let api_ctx = api_ctx.borrow();

            if api_ctx.simulation.time > 0 {
                return Err(encounter_method_called_after_start());
            }

            let config = &mut *api_ctx.resources.config.borrow_mut();

            if let Some(index) = player_index {
                if let Some(flippable) = config.player_flippable.get_mut(index) {
                    *flippable = Some(enable);
                }
            } else {
                config.player_flippable.fill(Some(enable));
            }

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(ENCOUNTER_TABLE, "enable_boss_battle", |api_ctx, lua, _| {
        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        simulation.statistics.boss_battle = true;

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(
        ENCOUNTER_TABLE,
        "enable_scripted_result",
        |api_ctx, lua, _| {
            let api_ctx = &mut *api_ctx.borrow_mut();
            let mut config = api_ctx.resources.config.borrow_mut();
            config.automatic_battle_end = false;

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        ENCOUNTER_TABLE,
        "enable_scripted_scene_end",
        |api_ctx, lua, _| {
            let api_ctx = &mut *api_ctx.borrow_mut();
            let mut config = api_ctx.resources.config.borrow_mut();
            config.automatic_scene_end = false;

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(ENCOUNTER_TABLE, "end_scene", |api_ctx, lua, _| {
        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        simulation.mark_battle_end(api_ctx.game_io, api_ctx.resources, false);
        simulation.progress = BattleProgress::Exiting;

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENCOUNTER_TABLE, "win", |api_ctx, lua, _| {
        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        simulation.mark_battle_end(api_ctx.game_io, api_ctx.resources, true);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENCOUNTER_TABLE, "lose", |api_ctx, lua, _| {
        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        simulation.mark_battle_end(api_ctx.game_io, api_ctx.resources, false);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENCOUNTER_TABLE, "on_battle_end", |_, lua, params| {
        let (_, callback): (rollback_mlua::Table, rollback_mlua::Function) =
            lua.unpack_multi(params)?;

        let table: rollback_mlua::Table = lua.globals().get(ENCOUNTER_TABLE)?;

        let external_listeners = if let Ok(external_listeners) =
            table.get::<_, rollback_mlua::Table>(BATTLE_END_LISTENERS)
        {
            external_listeners
        } else {
            let external_listeners = lua.create_table()?;
            table.set(BATTLE_END_LISTENERS, external_listeners.clone())?;
            external_listeners
        };

        external_listeners.push(callback)?;

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENCOUNTER_TABLE, "send_to_server", |api_ctx, lua, params| {
        let (_, data): (rollback_mlua::Table, rollback_mlua::Value) = lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        if simulation.progress == BattleProgress::Exiting {
            return Err(server_communication_closed());
        }

        let string = lua_value_to_string(data, "", 0);
        simulation.external_outbox.push(string);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENCOUNTER_TABLE, "on_server_message", |_, lua, params| {
        let (_, callback): (rollback_mlua::Table, rollback_mlua::Function) =
            lua.unpack_multi(params)?;

        let table: rollback_mlua::Table = lua.globals().get(ENCOUNTER_TABLE)?;

        let external_listeners = if let Ok(external_listeners) =
            table.get::<_, rollback_mlua::Table>(EXTERNAL_LISTENERS)
        {
            external_listeners
        } else {
            let external_listeners = lua.create_table()?;
            table.set(EXTERNAL_LISTENERS, external_listeners.clone())?;
            external_listeners
        };

        external_listeners.push(callback)?;

        lua.pack_multi(())
    });
}

pub fn call_callbacks(
    game_io: &GameIO,
    resources: &SharedBattleResources,
    simulation: &mut BattleSimulation,
    callback_list_name: &str,
    load_params: impl for<'lua> FnOnce(
        &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>>,
) {
    let Some(vm_index) = resources.vm_manager.encounter_vm else {
        return;
    };

    let vm = &resources.vm_manager.vms[vm_index];
    let lua = &vm.lua;

    let api_ctx = RefCell::new(BattleScriptContext {
        vm_index,
        resources,
        game_io,
        simulation,
    });

    let lua_api = &Globals::from_resources(game_io).battle_api;

    lua_api.inject_dynamic(lua, &api_ctx, |lua| {
        let table: rollback_mlua::Table = lua.globals().get(ENCOUNTER_TABLE)?;
        let Ok(external_listeners) = table.get::<_, rollback_mlua::Table>(callback_list_name)
        else {
            // no listeners
            return Ok(());
        };

        let params = load_params(lua)?;

        for listener in external_listeners.sequence_values::<rollback_mlua::Function>() {
            let Ok(listener) = listener else {
                continue;
            };

            if let Err(err) = listener.call::<_, ()>(params.clone()) {
                log::error!("{err}");
            }
        }

        Ok(())
    });
}

pub fn pass_server_message(
    game_io: &GameIO,
    resources: &SharedBattleResources,
    simulation: &mut BattleSimulation,
    message: &str,
) {
    call_callbacks(game_io, resources, simulation, EXTERNAL_LISTENERS, |lua| {
        lua.load(message).eval()
    })
}

pub fn call_encounter_end_listeners(
    game_io: &GameIO,
    resources: &SharedBattleResources,
    simulation: &mut BattleSimulation,
    won: bool,
) {
    call_callbacks(
        game_io,
        resources,
        simulation,
        BATTLE_END_LISTENERS,
        |lua| lua.pack(won),
    )
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
) -> rollback_mlua::Result<rollback_mlua::Table<'_>> {
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
        callback.call::<_, ()>(table)?;

        lua.pack_multi(())
    });
}

fn create_mutator(
    lua: &rollback_mlua::Lua,
    id: EntityId,
) -> rollback_mlua::Result<rollback_mlua::Table<'_>> {
    let table = lua.create_table()?;
    table.set("#entity_id", id)?;
    inherit_metatable(lua, MUTATOR_TABLE, &table)?;

    Ok(table)
}
