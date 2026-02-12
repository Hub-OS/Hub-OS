use super::{BattleLuaApi, ENTITY_TABLE, GAME_FOLDER_REGISTRY_KEY, RESOURCES_TABLE};
use crate::battle::Player;
use crate::bindable::{AudioBehavior, EntityId, InputQuery, LuaColor};
use crate::lua_api::helpers::absolute_path;
use crate::resources::{AssetManager, Globals};
use crate::transitions::flash_color;

pub fn inject_engine_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(RESOURCES_TABLE, "load_texture", |api_ctx, lua, params| {
        let path = absolute_path(lua, lua.unpack_multi(params)?)?;

        // cache the texture
        let api_ctx = api_ctx.borrow();
        let game_io = api_ctx.game_io;
        let globals = Globals::from_resources(game_io);
        globals.assets.texture(game_io, &path);

        // pass the string right back since passing pointers is not safe
        lua.pack_multi(path)
    });

    lua_api.add_dynamic_function(RESOURCES_TABLE, "load_audio", |api_ctx, lua, params| {
        let path = absolute_path(lua, lua.unpack_multi(params)?)?;

        // cache the sound
        let api_ctx = api_ctx.borrow();
        let game_io = &api_ctx.game_io;
        let globals = Globals::from_resources(game_io);
        globals.assets.audio(game_io, &path);

        lua.pack_multi(path)
    });

    lua_api.add_dynamic_function(RESOURCES_TABLE, "play_audio", |api_ctx, lua, params| {
        let (path, behavior): (String, Option<AudioBehavior>) = lua.unpack_multi(params)?;
        let path = absolute_path(lua, path)?;
        let behavior = behavior.unwrap_or_default();

        let api_ctx = api_ctx.borrow();
        let game_io = &api_ctx.game_io;
        let simulation = &api_ctx.simulation;

        if !simulation.is_resimulation || matches!(behavior, AudioBehavior::EndLoop) {
            let globals = Globals::from_resources(game_io);
            let sound_buffer = globals.assets.audio(game_io, &path);
            let audio = &globals.audio;

            audio.play_sound_with_behavior(&sound_buffer, behavior);
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "play_audio", |api_ctx, lua, params| {
        let (player_table, path, behavior): (rollback_mlua::Table, String, Option<AudioBehavior>) =
            lua.unpack_multi(params)?;
        let path = absolute_path(lua, path)?;
        let behavior = behavior.unwrap_or_default();

        let api_ctx = &mut *api_ctx.borrow_mut();
        let game_io = api_ctx.game_io;
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let entity_id: EntityId = player_table.raw_get("#id")?;

        let player = entities.query_one_mut::<&Player>(entity_id.into()).ok();

        if player.is_some_and(|player| player.index == simulation.local_player_index)
            && (!simulation.is_resimulation || matches!(behavior, AudioBehavior::EndLoop))
        {
            let globals = Globals::from_resources(game_io);
            let sound_buffer = globals.assets.audio(game_io, &path);
            let audio = &globals.audio;

            audio.play_sound_with_behavior(&sound_buffer, behavior);
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(RESOURCES_TABLE, "stop_music", |api_ctx, lua, _| {
        let api_ctx = api_ctx.borrow();
        let game_io = api_ctx.game_io;
        let resources = &api_ctx.resources;
        let simulation = &api_ctx.simulation;

        simulation.stop_music(game_io, resources);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(RESOURCES_TABLE, "play_music", |api_ctx, lua, params| {
        let (path, loops): (String, Option<bool>) = lua.unpack_multi(params)?;

        let path = absolute_path(lua, path)?;
        let loops = loops.unwrap_or(true);

        let api_ctx = api_ctx.borrow();
        let game_io = api_ctx.game_io;
        let resources = &api_ctx.resources;
        let simulation = &api_ctx.simulation;

        let globals = Globals::from_resources(game_io);
        let sound_buffer = globals.assets.audio(game_io, &path);
        simulation.play_music(game_io, resources, &sound_buffer, loops);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(RESOURCES_TABLE, "game_folder", |_, lua, _| {
        let path_str: rollback_mlua::String = lua.named_registry_value(GAME_FOLDER_REGISTRY_KEY)?;
        lua.pack_multi(path_str)
    });

    lua_api.add_dynamic_function(RESOURCES_TABLE, "white_flash_color", |api_ctx, lua, _| {
        let game_io = api_ctx.borrow().game_io;
        let color = flash_color(game_io);
        lua.pack_multi(LuaColor::from(color))
    });

    lua_api.add_dynamic_function(RESOURCES_TABLE, "is_local", |api_ctx, lua, params| {
        let index: usize = lua.unpack_multi(params)?;

        let api_ctx = api_ctx.borrow_mut();
        lua.pack_multi(api_ctx.simulation.local_player_index == index)
    });

    lua_api.add_dynamic_function(RESOURCES_TABLE, "local_index", |api_ctx, lua, _| {
        let api_ctx = api_ctx.borrow();
        lua.pack_multi(api_ctx.simulation.local_player_index)
    });

    lua_api.add_dynamic_function(RESOURCES_TABLE, "input_has", |api_ctx, lua, params| {
        let (index, input_query): (usize, InputQuery) = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        let Some(player_input) = &simulation.inputs.get(index) else {
            return lua.pack_multi(false);
        };

        let result = match input_query {
            InputQuery::JustPressed(input) => player_input.was_just_pressed(input),
            InputQuery::Pulsed(input) => player_input.pulsed(input),
            InputQuery::Held(input) => player_input.is_down(input),
        };

        lua.pack_multi(result)
    });

    lua_api.add_dynamic_function(RESOURCES_TABLE, "namespace", |api_ctx, lua, _| {
        let api_ctx = api_ctx.borrow();

        let vms = api_ctx.resources.vm_manager.vms();
        let vm = &vms[api_ctx.vm_index];
        let namespace = vm.preferred_namespace();

        lua.pack_multi(namespace)
    });
}
