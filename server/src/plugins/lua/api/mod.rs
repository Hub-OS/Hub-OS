mod actor_property_animation;
mod area_api;
mod asset_api;
mod async_api;
mod bot_api;
mod logging_api;
mod lua_errors;
mod lua_helpers;
mod misc_api;
mod object_api;
mod player_api;
mod player_data_api;
mod sprite_api;
mod synchronization_api;
mod widget_api;

use crate::net::{Net, WidgetTracker};
use std::cell::RefCell;

use crate::jobs::JobPromiseManager;
use std::collections::HashMap;
use std::collections::VecDeque;

pub struct ApiContext<'lua_scope, 'a> {
    pub script_index: usize,
    pub net_ref: &'lua_scope RefCell<&'a mut Net>,
    pub widget_tracker_ref: &'lua_scope RefCell<&'a mut HashMap<String, WidgetTracker<usize>>>,
    pub battle_tracker_ref: &'lua_scope RefCell<&'a mut HashMap<String, VecDeque<usize>>>,
    pub promise_manager_ref: &'lua_scope RefCell<&'a mut JobPromiseManager>,
}

type StaticLuaInjector = dyn Fn(&mlua::Lua) -> mlua::Result<()>;

type RustLuaFunction = dyn for<'lua> FnMut(
    &ApiContext,
    &'lua mlua::Lua,
    mlua::MultiValue<'lua>,
) -> mlua::Result<mlua::MultiValue<'lua>>;

pub struct LuaApi {
    static_function_injectors: Vec<Box<StaticLuaInjector>>,
    dynamic_function_table_and_name_pairs: Vec<(String, String)>,
    dynamic_functions: HashMap<String, Box<RustLuaFunction>>,
    table_names: Vec<String>,
}

impl LuaApi {
    pub fn new() -> LuaApi {
        let mut lua_api = LuaApi {
            static_function_injectors: Vec::new(),
            dynamic_function_table_and_name_pairs: Vec::new(),
            dynamic_functions: HashMap::new(),
            table_names: Vec::new(),
        };

        lua_api.add_static_injector(|lua| {
            lua.load(include_str!("event_emitter.lua"))
                .set_name("internal: event_emitter.lua")
                .exec()
        });

        logging_api::inject_static(&mut lua_api);

        area_api::inject_dynamic(&mut lua_api);
        asset_api::inject_dynamic(&mut lua_api);
        object_api::inject_dynamic(&mut lua_api);
        player_api::inject_dynamic(&mut lua_api);
        player_data_api::inject_dynamic(&mut lua_api);
        widget_api::inject_dynamic(&mut lua_api);
        bot_api::inject_dynamic(&mut lua_api);
        sprite_api::inject_dynamic(&mut lua_api);
        synchronization_api::inject_dynamic(&mut lua_api);

        async_api::inject_static(&mut lua_api);
        async_api::inject_dynamic(&mut lua_api);

        misc_api::inject_static(&mut lua_api);

        lua_api
    }

    pub fn add_global_table(&mut self, name: &str) {
        self.table_names.push(name.to_string());
    }

    pub(self) fn add_static_injector<F>(&mut self, injector: F)
    where
        F: 'static + Send + Fn(&mlua::Lua) -> mlua::Result<()>,
    {
        self.static_function_injectors.push(Box::new(injector));
    }

    pub(self) fn add_dynamic_function<F>(&mut self, table_name: &str, function_name: &str, func: F)
    where
        F: 'static
            + for<'lua> Fn(
                &ApiContext,
                &'lua mlua::Lua,
                mlua::MultiValue<'lua>,
            ) -> mlua::Result<mlua::MultiValue<'lua>>,
    {
        let table_name = String::from(table_name);
        let function_name = String::from(function_name);
        let function_id = table_name.clone() + "." + function_name.as_str();

        self.dynamic_function_table_and_name_pairs
            .push((table_name, function_name));

        self.dynamic_functions.insert(function_id, Box::new(func));
    }

    pub fn inject_static(&self, lua: &mlua::Lua) -> mlua::Result<()> {
        let globals = lua.globals();

        for table_name in &self.table_names {
            globals.set(table_name.as_str(), lua.create_table()?)?;
        }

        for static_function_injector in &self.static_function_injectors {
            static_function_injector(lua)?;
        }

        for (table_name, function_name) in &self.dynamic_function_table_and_name_pairs {
            let table: mlua::Table = globals.get(table_name.as_str())?;

            let function_id = table_name.clone() + "." + function_name.as_str();

            table.set(
                function_name.as_str(),
                lua.create_function(move |lua, values: mlua::MultiValue| {
                    let globals = lua.globals();
                    let net_table: mlua::Table = globals.get("Net")?;
                    let func: mlua::Function = net_table.get("_delegate")?;

                    let value: mlua::Value = func.call((function_id.as_str(), values))?;

                    Ok(value)
                })?,
            )?;
        }

        Ok(())
    }

    pub fn inject_dynamic<'lua, F>(
        &mut self,
        lua: &'lua mlua::Lua,
        api_ctx: ApiContext,
        wrapped_fn: F,
    ) -> mlua::Result<()>
    where
        F: FnMut(&'lua mlua::Lua) -> mlua::Result<()>,
    {
        let mut wrapped_fn = wrapped_fn;

        lua.scope(move |scope| -> mlua::Result<()> {
            let globals = lua.globals();
            let table: mlua::Table = globals.get("Net")?;

            table.set(
                "_delegate",
                scope.create_function_mut(
                    move |lua, (function_id, params): (String, mlua::MultiValue)| {
                        let func = self.dynamic_functions.get_mut(&function_id);

                        if let Some(func) = func {
                            func(&api_ctx, lua, params)
                        } else {
                            Err(mlua::Error::RuntimeError(format!(
                                "Function {:?} does not exist",
                                function_id
                            )))
                        }
                    },
                )?,
            )?;

            wrapped_fn(lua)?;

            Ok(())
        })?;

        Ok(())
    }
}
