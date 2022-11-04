use super::BattleLuaApi;
use rand::Rng;
use rollback_mlua::prelude::{LuaError, LuaNil};

pub fn inject_math_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_static_injector(|lua| {
        let math_table: rollback_mlua::Table = lua.globals().get("math")?;
        math_table.set("randomseed", LuaNil)?;
        math_table.set("random", LuaNil)?;
        Ok(())
    });

    lua_api.add_dynamic_function("math", "randomseed", |_, lua, _| {
        log::warn!("calling math.randomseed is not necessary, the engine handles it for you");
        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("math", "random", |api_ctx, lua, params| {
        let (n, m): (Option<i32>, Option<i32>) = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let rng = &mut api_ctx.simulation.rng;

        let n = match n {
            Some(n) => n,
            None => return lua.pack_multi(rng.gen::<f32>()),
        };

        if n < 0 {
            return Err(LuaError::RuntimeError(String::from(
                "n must be larger than 0",
            )));
        }

        let m = match m {
            Some(m) => m,
            None => return lua.pack_multi(rng.gen_range(1..=n)),
        };

        if m < n {
            return Err(LuaError::RuntimeError(String::from(
                "m must be larger or equal to n",
            )));
        }

        lua.pack_multi(rng.gen_range(n..=m))
    });
}
