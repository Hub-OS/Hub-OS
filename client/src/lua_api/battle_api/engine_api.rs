use super::{BattleLuaApi, ENGINE_TABLE};
use crate::battle::TurnGauge;
use crate::lua_api::helpers::absolute_path;
use crate::render::*;
use crate::resources::AssetManager;

pub fn inject_engine_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(ENGINE_TABLE, "load_texture", |api_ctx, lua, params| {
        let path = absolute_path(lua, lua.unpack_multi(params)?)?;

        // cache the texture
        let api_ctx = api_ctx.borrow();
        let game_io = api_ctx.game_io;
        game_io.globals().assets.texture(game_io, &path);

        // pass the string right back since passing pointers is not safe
        lua.pack_multi(path)
    });

    lua_api.add_dynamic_function(ENGINE_TABLE, "load_audio", |api_ctx, lua, params| {
        let path = absolute_path(lua, lua.unpack_multi(params)?)?;

        // cache the sound
        let api_ctx = api_ctx.borrow();
        let globals = api_ctx.game_io.globals();
        globals.assets.audio(&path);

        lua.pack_multi(path)
    });

    lua_api.add_dynamic_function(ENGINE_TABLE, "play_audio", |api_ctx, lua, params| {
        let path: String = lua.unpack_multi(params)?; // todo: (path, AudioPriority)?
        let path = absolute_path(lua, path)?;

        let api_ctx = api_ctx.borrow();
        let game_io = &api_ctx.game_io;
        let simulation = &api_ctx.simulation;

        let sound_buffer = game_io.globals().assets.audio(&path);
        simulation.play_sound(game_io, &sound_buffer);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENGINE_TABLE, "stream_music", |api_ctx, lua, params| {
        // todo: loop points
        let (path, loops, _start_ms, _end_ms): (String, Option<bool>, Option<u64>, Option<u64>) =
            lua.unpack_multi(params)?;

        let loops = loops.unwrap_or(true);

        let api_ctx = api_ctx.borrow();
        let globals = api_ctx.game_io.globals();
        let sound_buffer = globals.assets.audio(&path);
        globals.audio.play_music(sound_buffer, loops);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENGINE_TABLE, "get_turn_gauge_value", |api_ctx, lua, _| {
        lua.pack_multi(api_ctx.borrow().simulation.turn_guage.progress())
    });

    lua_api.add_dynamic_function(ENGINE_TABLE, "get_turn_gauge_time", |api_ctx, lua, _| {
        lua.pack_multi(api_ctx.borrow().simulation.turn_guage.time())
    });

    lua_api.add_dynamic_function(
        ENGINE_TABLE,
        "get_turn_gauge_max_time",
        |api_ctx, lua, _| lua.pack_multi(api_ctx.borrow().simulation.turn_guage.max_time()),
    );

    lua_api.add_dynamic_function(
        ENGINE_TABLE,
        "get_default_turn_gauge_max_time",
        |_, lua, _| lua.pack_multi(TurnGauge::DEFAULT_MAX_TIME),
    );

    lua_api.add_dynamic_function(
        ENGINE_TABLE,
        "set_turn_gauge_time",
        |api_ctx, lua, params| {
            let time: FrameTime = lua.unpack_multi(params)?;

            let mut api_ctx = api_ctx.borrow_mut();
            let turn_guage = &mut api_ctx.simulation.turn_guage;
            turn_guage.set_time(time);

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        ENGINE_TABLE,
        "set_turn_gauge_max_time",
        |api_ctx, lua, params| {
            let time: FrameTime = lua.unpack_multi(params)?;

            let mut api_ctx = api_ctx.borrow_mut();
            let turn_guage = &mut api_ctx.simulation.turn_guage;
            turn_guage.set_max_time(time);

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        ENGINE_TABLE,
        "reset_turn_gauge_to_default",
        |api_ctx, lua, _| {
            let mut api_ctx = api_ctx.borrow_mut();
            let turn_guage = &mut api_ctx.simulation.turn_guage;
            turn_guage.set_max_time(TurnGauge::DEFAULT_MAX_TIME);

            lua.pack_multi(())
        },
    );
}
