use super::{BattleLuaApi, RESOURCES_TABLE};
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
        let globals = api_ctx.game_io.resource::<Globals>().unwrap();
        globals.assets.audio(&path);

        lua.pack_multi(path)
    });

    lua_api.add_dynamic_function(RESOURCES_TABLE, "play_audio", |api_ctx, lua, params| {
        let path: String = lua.unpack_multi(params)?; // todo: (path, AudioPriority)?
        let path = absolute_path(lua, path)?;

        let api_ctx = api_ctx.borrow();
        let game_io = &api_ctx.game_io;
        let simulation = &api_ctx.simulation;

        let sound_buffer = game_io.resource::<Globals>().unwrap().assets.audio(&path);
        simulation.play_sound(game_io, &sound_buffer);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(RESOURCES_TABLE, "play_music", |api_ctx, lua, params| {
        // todo: loop points
        let (path, loops, _start_ms, _end_ms): (String, Option<bool>, Option<u64>, Option<u64>) =
            lua.unpack_multi(params)?;

        let loops = loops.unwrap_or(true);

        let api_ctx = api_ctx.borrow();
        let globals = api_ctx.game_io.resource::<Globals>().unwrap();
        let sound_buffer = globals.assets.audio(&path);
        globals.audio.play_music(&sound_buffer, loops);

        lua.pack_multi(())
    });
}
