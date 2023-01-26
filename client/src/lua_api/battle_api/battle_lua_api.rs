// I want to make this a generic: LuaApi<ApiContext>
// however there's some lifetime issues as the ApiContext usually stores references
// and rust does not allow the type parameter to store dynamic/anonymous lifetimes

const PRIMARY_TABLE: &str = "Engine";

use crate::battle::BattleScriptContext;
use std::cell::RefCell;
use std::collections::HashMap;

type LuaApiFn = dyn for<'lua> Fn(
    &RefCell<BattleScriptContext>,
    &'lua rollback_mlua::Lua,
    rollback_mlua::MultiValue<'lua>,
) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>;

struct LuaApiFunction {
    function: Box<LuaApiFn>,
    is_getter: bool,
}

impl LuaApiFunction {
    fn new<F>(func: F) -> Self
    where
        F: 'static
            + for<'lua> Fn(
                &RefCell<BattleScriptContext>,
                &'lua rollback_mlua::Lua,
                rollback_mlua::MultiValue<'lua>,
            ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>,
    {
        Self {
            function: Box::new(func),
            is_getter: false,
        }
    }

    fn new_getter<F>(func: F) -> Self
    where
        F: 'static
            + for<'lua> Fn(
                &RefCell<BattleScriptContext>,
                &'lua rollback_mlua::Lua,
                rollback_mlua::MultiValue<'lua>,
            ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>,
    {
        Self {
            function: Box::new(func),
            is_getter: true,
        }
    }
}

const INDEX_CALLBACK: u8 = 1;
const NEWINDEX_CALLBACK: u8 = 2;

const REGULAR_FUNCTION: u8 = 1;
const GETTER_FUNCTION: u8 = 2;

pub struct BattleLuaApi {
    static_function_injectors: Vec<Box<dyn Fn(&rollback_mlua::Lua) -> rollback_mlua::Result<()>>>,
    dynamic_functions: Vec<HashMap<(u8, String), LuaApiFunction>>,
    table_paths: Vec<String>,
}

impl BattleLuaApi {
    pub fn new() -> Self {
        let mut lua_api = Self {
            static_function_injectors: Vec::new(),
            dynamic_functions: Vec::new(),
            table_paths: Vec::new(),
        };

        lua_api.add_static_injector(super::global_api::inject_global_api);

        super::math_api::inject_math_api(&mut lua_api);
        super::include_api::inject_include_api(&mut lua_api);
        super::engine_api::inject_engine_api(&mut lua_api);
        super::entity_api::inject_entity_api(&mut lua_api);
        super::player_form_api::inject_player_form_api(&mut lua_api);
        super::component_api::inject_component_api(&mut lua_api);
        super::card_action_api::inject_card_action_api(&mut lua_api);
        super::ability_modifier_api::inject_ability_mod_api(&mut lua_api);
        super::field_api::inject_field_api(&mut lua_api);
        super::tile_api::inject_tile_api(&mut lua_api);
        super::sprite_api::inject_sprite_api(&mut lua_api);
        super::sync_node_api::inject_sync_node_api(&mut lua_api);
        super::animation_api::inject_animation_api(&mut lua_api);
        super::defense_rule_api::inject_defense_rule_api(&mut lua_api);
        super::battle_init::inject_battle_init_api(&mut lua_api);
        super::built_in_api::inject_built_in_api(&mut lua_api);

        lua_api
    }

    pub fn add_static_injector<F>(&mut self, injector: F)
    where
        F: 'static + Send + Fn(&rollback_mlua::Lua) -> rollback_mlua::Result<()>,
    {
        self.static_function_injectors.push(Box::new(injector));
    }

    pub fn add_dynamic_function<F>(&mut self, table_path: &str, function_name: &str, func: F)
    where
        F: 'static
            + for<'lua> Fn(
                &RefCell<BattleScriptContext>,
                &'lua rollback_mlua::Lua,
                rollback_mlua::MultiValue<'lua>,
            ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>,
    {
        let index = match self.table_paths.iter().position(|name| *name == table_path) {
            Some(index) => index,
            None => {
                self.table_paths.push(table_path.to_string());
                self.dynamic_functions.push(HashMap::new());
                self.table_paths.len() - 1
            }
        };

        let prev = self.dynamic_functions[index].insert(
            (INDEX_CALLBACK, function_name.to_string()),
            LuaApiFunction::new(func),
        );

        if prev.is_some() {
            log::error!("{}:{} defined more than once", table_path, function_name)
        }
    }

    pub fn add_dynamic_getter<F>(&mut self, table_path: &str, function_name: &str, func: F)
    where
        F: 'static
            + for<'lua> Fn(
                &RefCell<BattleScriptContext>,
                &'lua rollback_mlua::Lua,
                rollback_mlua::MultiValue<'lua>,
            ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>,
    {
        let index = match self.table_paths.iter().position(|name| *name == table_path) {
            Some(index) => index,
            None => {
                self.table_paths.push(table_path.to_string());
                self.dynamic_functions.push(HashMap::new());
                self.table_paths.len() - 1
            }
        };

        let prev = self.dynamic_functions[index].insert(
            (INDEX_CALLBACK, function_name.to_string()),
            LuaApiFunction::new_getter(func),
        );

        if prev.is_some() {
            log::error!(
                "getter {}.{} defined more than once",
                table_path,
                function_name
            )
        }
    }

    pub fn add_dynamic_setter<F>(&mut self, table_path: &str, function_name: &str, func: F)
    where
        F: 'static
            + for<'lua> Fn(
                &RefCell<BattleScriptContext>,
                &'lua rollback_mlua::Lua,
                rollback_mlua::MultiValue<'lua>,
            ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>,
    {
        let index = match self.table_paths.iter().position(|name| *name == table_path) {
            Some(index) => index,
            None => {
                self.table_paths.push(table_path.to_string());
                self.dynamic_functions.push(HashMap::new());
                self.table_paths.len() - 1
            }
        };

        let prev = self.dynamic_functions[index].insert(
            (NEWINDEX_CALLBACK, function_name.to_string()),
            LuaApiFunction::new(func),
        );

        if prev.is_some() {
            log::error!(
                "setter {}.{} defined more than once",
                table_path,
                function_name
            )
        }
    }

    /// Should be called on lua vm creation after static functions are created on the api struct
    pub fn inject_static(&self, lua: &rollback_mlua::Lua) -> rollback_mlua::Result<()> {
        for table_path in &self.table_paths {
            let mut parent_table = lua.globals();

            for table_name in table_path.split('.') {
                let value: rollback_mlua::Value = parent_table.raw_get(table_name)?;

                match value {
                    rollback_mlua::Value::Table(table) => parent_table = table,
                    _ => {
                        let new_table = lua.create_table()?;
                        parent_table.raw_set(table_name, new_table.clone())?;
                        parent_table = new_table;
                    }
                }
            }

            lua.set_named_registry_value(table_path, parent_table)?;
        }

        for (table_index, table_path) in self.table_paths.iter().enumerate() {
            // find the table
            let mut table = lua.globals();

            for table_name in table_path.split('.') {
                table = table.raw_get(table_name)?;
            }

            // store the table in the registry to easily reference in closures
            let table_key = lua.create_registry_value(table.clone())?;

            let metatable = lua.create_table()?;

            metatable.set(
                "__index",
                lua.create_function(
                    move |lua, (self_table, key): (rollback_mlua::Table, rollback_mlua::String)| {
                        // try value on self
                        let value: rollback_mlua::Value = self_table.raw_get(key.clone())?;

                        if value != rollback_mlua::Nil {
                            return Ok(value);
                        }

                        // try value on table
                        let table: rollback_mlua::Table = lua.registry_value(&table_key)?;
                        let value: rollback_mlua::Value = table.raw_get(key.clone())?;

                        if value != rollback_mlua::Nil {
                            return Ok(value);
                        }

                        // try delegate
                        let type_check: Option<rollback_mlua::Function> =
                            lua.named_registry_value("delegate_type")?;

                        let delegate_type: u8 = type_check
                            .map(|type_check| {
                                type_check.call((table_index, INDEX_CALLBACK, key.clone()))
                            })
                            .transpose()?
                            .unwrap_or_default();

                        match delegate_type {
                            REGULAR_FUNCTION => {
                                let func: rollback_mlua::Function =
                                    lua.named_registry_value("delegate")?;
                                Ok(rollback_mlua::Value::Function(func.bind((
                                    table_index,
                                    INDEX_CALLBACK,
                                    key.clone(),
                                ))?))
                            }
                            GETTER_FUNCTION => {
                                let func: rollback_mlua::Function =
                                    lua.named_registry_value("delegate")?;
                                func.call((table_index, INDEX_CALLBACK, key, self_table))
                            }
                            _ => Ok(rollback_mlua::Nil),
                        }
                    },
                )?,
            )?;

            metatable.set(
                "__newindex",
                lua.create_function(
                    move |lua,
                          (self_table, key, value): (
                        rollback_mlua::Table,
                        rollback_mlua::String,
                        rollback_mlua::Value,
                    )| {
                        let type_check: Option<rollback_mlua::Function> =
                            lua.named_registry_value("delegate_type")?;

                        let delegate_type: u8 = type_check
                            .map(|type_check| {
                                type_check.call((table_index, NEWINDEX_CALLBACK, key.clone()))
                            })
                            .transpose()?
                            .unwrap_or_default();

                        if delegate_type != 0 {
                            let func: rollback_mlua::Function =
                                lua.named_registry_value("delegate")?;

                            func.call((table_index, NEWINDEX_CALLBACK, key, (self_table, value)))?;
                        } else {
                            // try value on self
                            self_table.raw_set(key, value)?;
                        }

                        Ok(())
                    },
                )?,
            )?;

            table.set_metatable(Some(metatable));
        }

        for static_function_injector in &self.static_function_injectors {
            static_function_injector(lua)?;
        }

        Ok(())
    }

    /// Should be called anytime a lua function must be called, wrap the call in the wrapped_fn
    /// Automatically logs errors
    pub fn inject_dynamic<'lua, F>(
        &self,
        lua: &'lua rollback_mlua::Lua,
        api_ctx: &RefCell<BattleScriptContext>,
        wrapped_fn: F,
    ) where
        F: FnOnce(&'lua rollback_mlua::Lua) -> rollback_mlua::Result<()>,
    {
        let res = lua.scope(move |scope| -> rollback_mlua::Result<()> {
            let old_delegate_type: rollback_mlua::Value =
                lua.named_registry_value("delegate_type")?;
            let old_delegate: rollback_mlua::Value = lua.named_registry_value("delegate")?;

            lua.set_named_registry_value(
                "delegate_type",
                scope.create_function(
                    move |_, (table_index, callback_type, function_name): (usize, u8, String)| {
                        // return value is REGULAR_FUNCTION, GETTER_FUNCTION, or 0
                        let function_type = self
                            .dynamic_functions
                            .get(table_index)
                            .and_then(|functions| functions.get(&(callback_type, function_name)))
                            .map(|function| 1 + function.is_getter as u8)
                            .unwrap_or_default();

                        Ok(function_type)
                    },
                )?,
            )?;

            lua.set_named_registry_value(
                "delegate",
                scope.create_function(
                    move |lua_ctx,
                          (table_index, callback_type, function_name, params): (
                        usize,
                        u8,
                        String,
                        rollback_mlua::MultiValue,
                    )| {
                        let func = self
                            .dynamic_functions
                            .get(table_index)
                            .ok_or_else(|| {
                                rollback_mlua::Error::RuntimeError(
                                    "invalid table index".to_string(),
                                )
                            })?
                            .get(&(callback_type, function_name));

                        if let Some(func) = func {
                            (func.function)(api_ctx, lua_ctx, params)
                                as rollback_mlua::Result<rollback_mlua::MultiValue>
                        } else {
                            Err(rollback_mlua::Error::RuntimeError(String::from(
                                "Function does not exist",
                            )))
                        }
                    },
                )?,
            )?;

            wrapped_fn(lua)?;

            lua.set_named_registry_value("delegate_type", old_delegate_type)?;
            lua.set_named_registry_value("delegate", old_delegate)?;

            Ok(())
        });

        if let Err(err) = res {
            log::error!("{err}");
        }
    }
}
