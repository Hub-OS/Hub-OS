use super::BattleLuaApi;
use crate::lua_api::battle_api::errors::get_source_name;
use nom::AsBytes;
use rand::Rng;
use rollback_mlua::prelude::{LuaError, LuaNil};

pub fn inject_desync_patch_api(lua_api: &mut BattleLuaApi) {
    patch_basic_api(lua_api);
    patch_math_api(lua_api);
}

pub fn patch_basic_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_static_injector(|lua| {
        // "The order in which the indices are enumerated is not specified, even for numeric indices. (To traverse a table in numerical order, use a numerical for.)"
        // so we're removing `next()` and using a custom `pairs()` function to avoid desyncs

        lua.globals().set("next", LuaNil)?;

        let next = lua.create_function(
            |_, (state, key): (rollback_mlua::Table, rollback_mlua::Value)| {
                let index_map: rollback_mlua::Table = state.raw_get("index_map")?;

                let next_index = if key.is_nil() {
                    1
                } else {
                    let index: usize = index_map.raw_get(key)?;
                    index + 1
                };

                let index_list: rollback_mlua::Table = state.raw_get("index_list")?;
                let next_key: rollback_mlua::Value = index_list.raw_get(next_index)?;

                if next_key.is_nil() {
                    return Ok((LuaNil, LuaNil));
                }

                let table: rollback_mlua::Table = state.raw_get("table")?;
                let value: rollback_mlua::Value = table.raw_get(next_key.clone())?;

                Ok((next_key, value))
            },
        )?;

        lua.set_named_registry_value("next()", next)?;

        lua.globals().set(
            "pairs",
            lua.create_function(|lua, table: rollback_mlua::Table| {
                let mut bools = Vec::<bool>::new();
                let mut integers = Vec::<i64>::new();
                let mut floats = Vec::<[u8; 8]>::new();
                let mut strings = Vec::<Vec<u8>>::new();

                let mut first_key = None;

                for pair in table.clone().pairs() {
                    let (key, _): (rollback_mlua::Value, rollback_mlua::Value) = pair?;

                    match &key {
                        rollback_mlua::Value::Boolean(b) => {
                            bools.push(*b);
                        }
                        rollback_mlua::Value::Integer(i) => {
                            integers.push(*i);
                        }
                        rollback_mlua::Value::Number(n) => {
                            floats.push(n.to_le_bytes());
                        }
                        rollback_mlua::Value::String(s) => {
                            strings.push(s.as_bytes().into());
                        }
                        key => {
                            let source_name = get_source_name(lua);

                            log::warn!(
                                "pairs() encountered unsupported type: {} in {source_name}",
                                key.type_name()
                            );
                            continue;
                        }
                    };

                    if first_key.is_none() {
                        first_key = Some(key);
                    }
                }

                // sort keys for a consistent sort order
                bools.sort();
                integers.sort();
                floats.sort();
                strings.sort();

                // create state
                let next_state = lua.create_table()?;
                let index_map = lua.create_table()?;
                let index_list = lua.create_table()?;

                let mut i = 1;

                for value in bools {
                    index_list.raw_push(value)?;
                    index_map.raw_set(value, i)?;
                    i += 1;
                }

                for value in integers {
                    index_list.raw_push(value)?;
                    index_map.raw_set(value, i)?;
                    i += 1;
                }

                for value in floats {
                    let value = f64::from_le_bytes(value);
                    index_list.raw_push(value)?;
                    index_map.raw_set(value, i)?;
                    i += 1;
                }

                for value in strings {
                    let value = lua.create_string(value.as_bytes())?;
                    index_list.raw_push(value.clone())?;
                    index_map.raw_set(value, i)?;
                    i += 1;
                }

                next_state.raw_set("table", table)?;
                next_state.raw_set("index_map", index_map)?;
                next_state.raw_set("index_list", index_list)?;

                let next_fn: rollback_mlua::Value = lua.named_registry_value("next()")?;
                Ok((next_fn, next_state))
            })?,
        )?;

        Ok(())
    });
}

pub fn patch_math_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_static_injector(|lua| {
        let math_table: rollback_mlua::Table = lua.globals().get("math")?;
        math_table.set("randomseed", LuaNil)?;
        math_table.set("random", LuaNil)?;
        Ok(())
    });

    lua_api.add_dynamic_function("math", "randomseed", |_, lua, _| {
        log::warn!("Calling math.randomseed is not necessary, the engine handles it for you");
        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("math", "random", |api_ctx, lua, params| {
        let (n, m): (Option<i64>, Option<i64>) = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let rng = &mut api_ctx.simulation.rng;

        let Some(mut n) = n else {
            return lua.pack_multi(rng.random::<f32>());
        };

        let Some(mut m) = m else {
            if n <= 0 {
                return Err(LuaError::RuntimeError(String::from(
                    "n must be larger than 0",
                )));
            }

            return lua.pack_multi(rng.random_range(1..=n));
        };

        if m < n {
            std::mem::swap(&mut n, &mut m);
        }

        lua.pack_multi(rng.random_range(n..=m))
    });
}
