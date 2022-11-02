use crate::bindable::DefensePriority;

use super::{BattleLuaApi, DEFENSE_JUDGE_TABLE, DEFENSE_RULE_TABLE};

pub fn inject_defense_rule_api(lua_api: &mut BattleLuaApi) {
    inject_judge_api(lua_api);

    lua_api.add_dynamic_function(DEFENSE_RULE_TABLE, "new", |_, lua, params| {
        let (priority, collision_only): (DefensePriority, bool) = lua.unpack_multi(params)?;

        let table = lua.create_table()?;
        table.set("_priority", priority)?;
        table.set("_collision_only", collision_only)?;
        table.set(
            "is_replaced",
            lua.create_function(|_, table: rollback_mlua::Table| {
                Ok(table.get::<_, bool>("_replaced").unwrap_or_default())
            })?,
        )?;

        lua.pack_multi(table)
    });
}

fn inject_judge_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(DEFENSE_JUDGE_TABLE, "block_damage", |api_ctx, lua, _| {
        let mut api_ctx = api_ctx.borrow_mut();
        api_ctx.simulation.defense_judge.damage_blocked = true;

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(DEFENSE_JUDGE_TABLE, "block_impact", |api_ctx, lua, _| {
        let mut api_ctx = api_ctx.borrow_mut();
        api_ctx.simulation.defense_judge.impact_blocked = true;

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(
        DEFENSE_JUDGE_TABLE,
        "is_damage_blocked",
        |api_ctx, lua, _| {
            let api_ctx = api_ctx.borrow();

            lua.pack_multi(api_ctx.simulation.defense_judge.damage_blocked)
        },
    );

    lua_api.add_dynamic_function(
        DEFENSE_JUDGE_TABLE,
        "is_impact_blocked",
        |api_ctx, lua, _| {
            let api_ctx = api_ctx.borrow();
            lua.pack_multi(api_ctx.simulation.defense_judge.impact_blocked)
        },
    );
}
