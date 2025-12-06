use super::BattleLuaApi;
use crate::lua_api::battle_api::errors::get_source_name;
use indexmap::IndexSet;
use rand::Rng;
use rollback_mlua::prelude::{LuaError, LuaNil};
use std::rc::Rc;

pub fn inject_desync_patch_api(lua_api: &mut BattleLuaApi) {
    patch_basic_api(lua_api);
    patch_math_api(lua_api);
}

pub fn patch_basic_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_static_injector(|lua| {
        // "The order in which the indices are enumerated is not specified, even for numeric indices. (To traverse a table in numerical order, use a numerical for.)"
        // so we're removing `next()` and using a custom `pairs()` function to avoid desyncs

        lua.globals().set("next", LuaNil)?;

        lua.globals().set(
            "pairs",
            lua.create_function(|lua, table: rollback_mlua::Table| {
                #[derive(PartialEq, Eq, PartialOrd, Ord, Hash, Clone, Copy)]
                enum NumberKey {
                    Boolean(bool),
                    Integer(i64),
                    Float([u8; 8]),
                }

                let mut number_set = IndexSet::<NumberKey>::new();
                let mut string_set = IndexSet::<Rc<[u8]>>::new();

                let mut first_key = None;

                for pair in table.clone().pairs() {
                    let (key, _): (rollback_mlua::Value, rollback_mlua::Value) = pair?;

                    match &key {
                        rollback_mlua::Value::Boolean(b) => {
                            number_set.insert(NumberKey::Boolean(*b));
                        }
                        rollback_mlua::Value::Integer(i) => {
                            number_set.insert(NumberKey::Integer(*i));
                        }
                        rollback_mlua::Value::Number(n) => {
                            number_set.insert(NumberKey::Float(n.to_le_bytes()));
                        }
                        rollback_mlua::Value::String(s) => {
                            string_set.insert(s.as_bytes().into());
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
                number_set.sort();
                string_set.sort();

                let next = lua.create_function(
                    move |lua, (table, key): (rollback_mlua::Table, rollback_mlua::Value)| {
                        let (mut map_index, index) = match &key {
                            rollback_mlua::Nil => (0, Some(0)),
                            rollback_mlua::Value::Boolean(b) => {
                                (0, number_set.get_index_of(&NumberKey::Boolean(*b)))
                            }
                            rollback_mlua::Value::Integer(i) => {
                                (0, number_set.get_index_of(&NumberKey::Integer(*i)))
                            }
                            rollback_mlua::Value::Number(n) => {
                                let f = n.to_le_bytes();
                                (0, number_set.get_index_of(&NumberKey::Float(f)))
                            }
                            rollback_mlua::Value::String(s) => {
                                (1, string_set.get_index_of(s.as_bytes()))
                            }
                            _ => {
                                return Ok((rollback_mlua::Nil, rollback_mlua::Nil));
                            }
                        };

                        let Some(mut index) = index else {
                            // this shouldn't occur unless someone calls this iterator directly with an odd value
                            return Ok((rollback_mlua::Nil, rollback_mlua::Nil));
                        };

                        if !key.is_nil() {
                            // for nil we want the first value
                            // otherwise we want to know the next value
                            index += 1;
                        }

                        loop {
                            let key = match map_index {
                                0 => {
                                    let Some(next_value) = number_set.get_index(index) else {
                                        map_index += 1;
                                        index = 0;
                                        continue;
                                    };

                                    match *next_value {
                                        NumberKey::Boolean(b) => rollback_mlua::Value::Boolean(b),
                                        NumberKey::Integer(i) => rollback_mlua::Value::Integer(i),
                                        NumberKey::Float(u) => {
                                            rollback_mlua::Value::Number(f64::from_le_bytes(u))
                                        }
                                    }
                                }
                                1 => {
                                    let Some(next_value) = string_set.get_index(index) else {
                                        map_index += 1;
                                        index = 0;
                                        continue;
                                    };

                                    let string = lua.create_string(next_value)?;
                                    rollback_mlua::Value::String(string)
                                }
                                // out of values
                                _ => break,
                            };

                            let value: rollback_mlua::Value = table.raw_get(key.clone())?;

                            if value.is_nil() {
                                // set to nil during iteration, try the next value
                                index += 1;
                                continue;
                            }

                            return Ok((key, value));
                        }

                        Ok((rollback_mlua::Nil, rollback_mlua::Nil))
                    },
                )?;

                Ok((next, table))
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
