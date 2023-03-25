use super::{BattleLuaApi, TURN_GAUGE_TABLE};
use crate::battle::TurnGauge;
use crate::render::*;

pub fn inject_turn_gauge_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(TURN_GAUGE_TABLE, "get_progress", |api_ctx, lua, _| {
        lua.pack_multi(api_ctx.borrow().simulation.turn_gauge.progress())
    });

    lua_api.add_dynamic_function(TURN_GAUGE_TABLE, "get_time", |api_ctx, lua, _| {
        lua.pack_multi(api_ctx.borrow().simulation.turn_gauge.time())
    });

    lua_api.add_dynamic_function(TURN_GAUGE_TABLE, "get_max_time", |api_ctx, lua, _| {
        lua.pack_multi(api_ctx.borrow().simulation.turn_gauge.max_time())
    });

    lua_api.add_dynamic_function(TURN_GAUGE_TABLE, "get_default_max_time", |_, lua, _| {
        lua.pack_multi(TurnGauge::DEFAULT_MAX_TIME)
    });

    lua_api.add_dynamic_function(TURN_GAUGE_TABLE, "set_time", |api_ctx, lua, params| {
        let time: FrameTime = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let turn_gauge = &mut api_ctx.simulation.turn_gauge;
        turn_gauge.set_time(time);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(TURN_GAUGE_TABLE, "set_max_time", |api_ctx, lua, params| {
        let time: FrameTime = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let turn_gauge = &mut api_ctx.simulation.turn_gauge;
        turn_gauge.set_max_time(time);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(TURN_GAUGE_TABLE, "reset_max_time", |api_ctx, lua, _| {
        let mut api_ctx = api_ctx.borrow_mut();
        let turn_gauge = &mut api_ctx.simulation.turn_gauge;
        turn_gauge.set_max_time(TurnGauge::DEFAULT_MAX_TIME);

        lua.pack_multi(())
    });
}
