// I want to make this a generic: LuaApi<ApiContext>
// however there's some lifetime issues as the ApiContext usually stores references
// and rust does not allow the type parameter to store dynamic/anonymous lifetimes

use crate::battle::BattleScriptContext;
use std::borrow::Cow;
use std::cell::RefCell;
use std::collections::HashMap;
use std::rc::Rc;

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

type DynamicFunctionMap = HashMap<(u8, Cow<'static, str>), LuaApiFunction>;

pub struct BattleLuaApi {
    static_function_injectors: Vec<Box<dyn Fn(&rollback_mlua::Lua) -> rollback_mlua::Result<()>>>,
    dynamic_functions: Vec<DynamicFunctionMap>,
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
        super::require_api::inject_require_api(&mut lua_api);
        super::resources_api::inject_engine_api(&mut lua_api);
        super::turn_gauge_api::inject_turn_gauge_api(&mut lua_api);
        super::entity_api::inject_entity_api(&mut lua_api);
        super::player_form_api::inject_player_form_api(&mut lua_api);
        super::component_api::inject_component_api(&mut lua_api);
        super::action_api::inject_action_api(&mut lua_api);
        super::movement_api::inject_movement_api(&mut lua_api);
        super::augment_api::inject_augment_api(&mut lua_api);
        super::field_api::inject_field_api(&mut lua_api);
        super::tile_api::inject_tile_api(&mut lua_api);
        super::sprite_api::inject_sprite_api(&mut lua_api);
        super::sync_node_api::inject_sync_node_api(&mut lua_api);
        super::animation_api::inject_animation_api(&mut lua_api);
        super::defense_rule_api::inject_defense_rule_api(&mut lua_api);
        super::encounter_init::inject_encounter_init_api(&mut lua_api);
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
            (INDEX_CALLBACK, Cow::Owned(function_name.to_string())),
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
            (INDEX_CALLBACK, Cow::Owned(function_name.to_string())),
            LuaApiFunction::new_getter(func),
        );

        if prev.is_some() {
            log::error!(
                "Getter {}.{} defined more than once",
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
            (NEWINDEX_CALLBACK, Cow::Owned(function_name.to_string())),
            LuaApiFunction::new(func),
        );

        if prev.is_some() {
            log::error!(
                "Setter {}.{} defined more than once",
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
                            return lua.pack_multi(value);
                        }

                        // try value on table
                        let table: rollback_mlua::Table = lua.registry_value(&table_key)?;
                        let value: rollback_mlua::Value = table.raw_get(key.clone())?;

                        if value != rollback_mlua::Nil {
                            return lua.pack_multi(value);
                        }

                        // try dynamic function
                        let Some(dynamic_api_ctx): Option<Rc<DynamicApiCtx>> = lua.app_data() else {
                            return lua.pack_multi(());
                        };

                        let function_name = key.to_str()?;
                        let Some(dynamic_function) = dynamic_api_ctx.dynamic_function(table_index, INDEX_CALLBACK, function_name) else {
                            return lua.pack_multi(());
                        };

                        if dynamic_function.is_getter {
                            (dynamic_function.function)(dynamic_api_ctx.api_ctx, lua, lua.pack_multi(self_table)?)
                        } else {
                            // cache this function to reduce garbage
                            let owned_name = function_name.to_string();

                            let binded_func = lua.create_function(
                                move |lua, params: rollback_mlua::MultiValue| {
                                    let Some(dynamic_api_ctx): Option<Rc<DynamicApiCtx>> = lua.app_data() else {
                                        return lua.pack_multi(());
                                    };

                                    let Some(dynamic_function) = dynamic_api_ctx.dynamic_function(table_index, INDEX_CALLBACK, &owned_name) else {
                                        return lua.pack_multi(());
                                    };

                                    (dynamic_function.function)(dynamic_api_ctx.api_ctx, lua, params)
                                },
                            )?;

                            table.set(key, binded_func.clone())?;

                            lua.pack_multi(binded_func)
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
                        let Some(dynamic_api_ctx): Option<Rc<DynamicApiCtx>> = lua.app_data() else {
                            // try value on self
                            self_table.raw_set(key, value)?;
                            return Ok(());
                        };

                        let function_name = key.to_str()?;
                        let Some(dynamic_function) = dynamic_api_ctx.dynamic_function(table_index, NEWINDEX_CALLBACK, function_name) else {
                            // try value on self
                            self_table.raw_set(key, value)?;
                            return Ok(());
                        };

                        let params = lua.pack_multi((self_table, value))?;
                        (dynamic_function.function)(dynamic_api_ctx.api_ctx, lua, params)?;

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
        let dynamic_api = DynamicApiCtx::new(api_ctx, &self.dynamic_functions);
        let old_dynamic_api = lua.set_app_data(Rc::new(unsafe {
            // unsafe if remove_app_data is never called,
            // and if this data is stored outside of a function scope during usage (long term closure + struct storage)
            // usage should be limited to this file for verification
            std::mem::transmute::<DynamicApiCtx, DynamicApiCtx<'static, 'static>>(dynamic_api)
        }));

        // call the function
        if let Err(err) = wrapped_fn(lua) {
            log::error!("{err}");
        }

        // cleanup
        lua.remove_app_data::<DynamicApiCtx>();

        if let Some(dynamic_api) = old_dynamic_api {
            lua.set_app_data(dynamic_api);
        }
    }
}

struct DynamicApiCtx<'a, 'b> {
    api_ctx: &'a RefCell<BattleScriptContext<'b>>,
    dynamic_functions: &'a [DynamicFunctionMap],
}

impl<'a, 'b> DynamicApiCtx<'a, 'b> {
    fn new(
        api_ctx: &'a RefCell<BattleScriptContext<'b>>,
        dynamic_functions: &'a [DynamicFunctionMap],
    ) -> Self {
        Self {
            api_ctx,
            dynamic_functions,
        }
    }

    fn dynamic_function<'c>(
        &'c self,
        table_index: usize,
        callback_type: u8,
        function_name: &'c str,
    ) -> Option<&LuaApiFunction> {
        let table_functions = self.dynamic_functions.get(table_index)?;

        table_functions.get(&(callback_type, Cow::Borrowed(function_name)))
    }
}
