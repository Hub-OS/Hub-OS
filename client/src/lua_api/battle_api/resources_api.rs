use super::{BattleLuaApi, GAME_FOLDER_KEY, RESOURCES_TABLE};
use crate::bindable::AudioBehavior;
use crate::lua_api::helpers::absolute_path;
use crate::resources::{AssetManager, Globals};

pub fn inject_engine_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(RESOURCES_TABLE, "load_texture", |api_ctx, lua, params| {
        let path = absolute_path(lua, lua.unpack_multi(params)?)?;

        // cache the texture
        let api_ctx = api_ctx.borrow();
        let game_io = api_ctx.game_io;
        let globals = game_io.resource::<Globals>().unwrap();
        globals.assets.texture(game_io, &path);

        // pass the string right back since passing pointers is not safe
        lua.pack_multi(path)
    });

    lua_api.add_dynamic_function(RESOURCES_TABLE, "load_audio", |api_ctx, lua, params| {
        let path = absolute_path(lua, lua.unpack_multi(params)?)?;

        // cache the sound
        let api_ctx = api_ctx.borrow();
        let game_io = &api_ctx.game_io;
        let globals = game_io.resource::<Globals>().unwrap();
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

        if !simulation.is_resimulation {
            let globals = game_io.resource::<Globals>().unwrap();
            let sound_buffer = globals.assets.audio(game_io, &path);
            let audio = &globals.audio;

            audio.play_sound_with_behavior(&sound_buffer, behavior);
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(RESOURCES_TABLE, "play_music", |api_ctx, lua, params| {
        let (path, loops, start_ms, end_ms): (String, Option<bool>, Option<u64>, Option<u64>) =
            lua.unpack_multi(params)?;

        let path = absolute_path(lua, path)?;
        let loops = loops.unwrap_or(true);

        let api_ctx = api_ctx.borrow();
        let simulation = &api_ctx.simulation;
        let game_io = api_ctx.game_io;
        let globals = game_io.resource::<Globals>().unwrap();

        let sound_buffer = globals.assets.audio(game_io, &path);
        simulation.play_music(game_io, &sound_buffer, loops, start_ms, end_ms);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(RESOURCES_TABLE, "game_folder", |_, lua, _| {
        let path_str: rollback_mlua::String = lua.named_registry_value(GAME_FOLDER_KEY)?;
        lua.pack_multi(path_str)
    });
}
