use super::{BattleLuaApi, DEFENSE_RULE_TABLE, DEFENSE_TABLE};
use crate::{
    bindable::{DefensePriority, IntangibleRule},
    lua_api::INTANGIBLE_RULE_TABLE,
};

pub fn inject_defense_rule_api(lua_api: &mut BattleLuaApi) {
    inject_defense_api(lua_api);
    inject_intangible_api(lua_api);

    lua_api.add_dynamic_function(DEFENSE_RULE_TABLE, "new", |_, lua, params| {
        let (priority, collision_only): (DefensePriority, bool) = lua.unpack_multi(params)?;

        let table = lua.create_table()?;
        table.set("#priority", priority)?;
        table.set("#collision_only", collision_only)?;
        table.set(
            "replaced",
            lua.create_function(|_, table: rollback_mlua::Table| {
                Ok(table.get::<_, bool>("#replaced").unwrap_or_default())
            })?,
        )?;

        lua.pack_multi(table)
    });
}

fn inject_defense_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(DEFENSE_TABLE, "block_damage", |api_ctx, lua, _| {
        let mut api_ctx = api_ctx.borrow_mut();
        api_ctx.simulation.defense.damage_blocked = true;

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(DEFENSE_TABLE, "block_impact", |api_ctx, lua, _| {
        let mut api_ctx = api_ctx.borrow_mut();
        api_ctx.simulation.defense.impact_blocked = true;

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(DEFENSE_TABLE, "damage_blocked", |api_ctx, lua, _| {
        let api_ctx = api_ctx.borrow();

        lua.pack_multi(api_ctx.simulation.defense.damage_blocked)
    });

    lua_api.add_dynamic_function(DEFENSE_TABLE, "impact_blocked", |api_ctx, lua, _| {
        let api_ctx = api_ctx.borrow();
        lua.pack_multi(api_ctx.simulation.defense.impact_blocked)
    });
}

fn inject_intangible_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(INTANGIBLE_RULE_TABLE, "new", |_, lua, _| {
        lua.pack_multi(IntangibleRule::default())
    });
}
