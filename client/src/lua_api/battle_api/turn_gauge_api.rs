use super::{BattleLuaApi, TURN_GAUGE_TABLE};
use crate::battle::TurnGauge;
use crate::render::*;

pub fn inject_turn_gauge_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(TURN_GAUGE_TABLE, "enabled", |api_ctx, lua, _| {
        let api_ctx = api_ctx.borrow();
        lua.pack_multi(api_ctx.simulation.turn_gauge.enabled())
    });

    lua_api.add_dynamic_function(TURN_GAUGE_TABLE, "set_enabled", |api_ctx, lua, params| {
        let enabled = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        api_ctx.simulation.turn_gauge.set_enabled(enabled);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(TURN_GAUGE_TABLE, "frozen", |api_ctx, lua, _| {
        let api_ctx = api_ctx.borrow();
        let time_freeze_tracker = &api_ctx.simulation.time_freeze_tracker;

        lua.pack_multi(time_freeze_tracker.time_is_frozen())
    });

    lua_api.add_dynamic_function(TURN_GAUGE_TABLE, "progress", |api_ctx, lua, _| {
        lua.pack_multi(api_ctx.borrow().simulation.turn_gauge.progress())
    });

    lua_api.add_dynamic_function(TURN_GAUGE_TABLE, "time", |api_ctx, lua, _| {
        lua.pack_multi(api_ctx.borrow().simulation.turn_gauge.time())
    });

    lua_api.add_dynamic_function(TURN_GAUGE_TABLE, "max_time", |api_ctx, lua, _| {
        lua.pack_multi(api_ctx.borrow().simulation.turn_gauge.max_time())
    });

    lua_api.add_dynamic_function(TURN_GAUGE_TABLE, "default_max_time", |_, lua, _| {
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

    lua_api.add_dynamic_function(TURN_GAUGE_TABLE, "current_turn", |api_ctx, lua, _| {
        let api_ctx = api_ctx.borrow();
        lua.pack_multi(api_ctx.simulation.statistics.turns)
    });

    lua_api.add_dynamic_function(TURN_GAUGE_TABLE, "complete_turn", |api_ctx, lua, _| {
        let mut api_ctx = api_ctx.borrow_mut();
        api_ctx.simulation.turn_gauge.set_completed_turn(true);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(TURN_GAUGE_TABLE, "turn_limit", |api_ctx, lua, _| {
        let api_ctx = api_ctx.borrow();
        lua.pack_multi(api_ctx.resources.config.borrow().turn_limit)
    });
}
