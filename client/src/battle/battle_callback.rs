use super::{BattleScriptContext, SharedBattleResources};
use crate::battle::BattleSimulation;
use crate::lua_api::VM_INDEX_REGISTRY_KEY;
use crate::resources::Globals;
use framework::prelude::GameIO;
use std::cell::RefCell;
use std::sync::Arc;

#[derive(Clone)]
pub struct BattleCallback<P = (), R = ()>
where
    P: for<'lua> rollback_mlua::IntoLuaMulti<'lua>,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua>,
{
    callback:
        Arc<dyn Fn(&GameIO, &SharedBattleResources, &mut BattleSimulation, P) -> R + Sync + Send>,
}

impl<P, R> std::fmt::Debug for BattleCallback<P, R>
where
    P: for<'lua> rollback_mlua::IntoLuaMulti<'lua>,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua>,
{
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("BattleCallback").finish()
    }
}

impl<P, R> BattleCallback<P, R>
where
    P: for<'lua> rollback_mlua::IntoLuaMulti<'lua>,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua>,
{
    pub fn new(
        callback: impl Fn(&GameIO, &SharedBattleResources, &mut BattleSimulation, P) -> R
        + Sync
        + Send
        + 'static,
    ) -> Self {
        Self {
            callback: Arc::new(callback),
        }
    }

    pub fn call(
        &self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        params: P,
    ) -> R {
        (self.callback)(game_io, resources, simulation, params)
    }
}

impl<P, R> BattleCallback<P, R>
where
    P: for<'lua> rollback_mlua::IntoLuaMulti<'lua> + Send + Sync + Copy + 'static,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua> + 'static,
{
    pub fn bind(self, params: P) -> BattleCallback<(), R> {
        BattleCallback::new(move |game_io, resources, simulation, _| {
            self.call(game_io, resources, simulation, params)
        })
    }
}

impl<P, R> BattleCallback<P, R>
where
    P: for<'lua> rollback_mlua::IntoLuaMulti<'lua>,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua> + Default,
{
    pub fn new_transformed_lua_callback<'lua, F>(
        lua: &'lua rollback_mlua::Lua,
        vm_index: usize,
        function: rollback_mlua::Function<'lua>,
        param_callback: F,
    ) -> rollback_mlua::Result<Self>
    where
        F: for<'p_lua> Fn(
                &RefCell<BattleScriptContext>,
                &'p_lua rollback_mlua::Lua,
                P,
            ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'p_lua>>
            + Send
            + Sync
            + 'static,
    {
        let key = lua.create_registry_value(function)?;

        Ok(BattleCallback::new(
            move |game_io, resources, simulation, params| {
                let lua = &resources.vm_manager.vms()[vm_index].lua;
                let lua_callback: rollback_mlua::Function = lua.registry_value(&key).unwrap();

                let api_ctx = RefCell::new(BattleScriptContext {
                    vm_index,
                    resources,
                    game_io,
                    simulation,
                });

                let lua_api = &Globals::from_resources(game_io).battle_api;

                let mut result = Default::default();

                lua_api.inject_dynamic(lua, &api_ctx, |lua| {
                    let params = param_callback(&api_ctx, lua, params)?;
                    result = lua_callback.call(params)?;

                    Ok(())
                });

                result
            },
        ))
    }

    pub fn new_lua_callback<'lua>(
        lua: &'lua rollback_mlua::Lua,
        vm_index: usize,
        function: rollback_mlua::Function<'lua>,
    ) -> rollback_mlua::Result<Self> {
        Self::new_transformed_lua_callback(lua, vm_index, function, |_, lua, p| lua.pack_multi(p))
    }
}

impl<P, R> Default for BattleCallback<P, R>
where
    P: for<'lua> rollback_mlua::IntoLuaMulti<'lua>,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua> + Default + Send + Sync + Clone + 'static,
{
    fn default() -> Self {
        Self::stub(R::default())
    }
}

impl<P, R> BattleCallback<P, R>
where
    P: for<'lua> rollback_mlua::IntoLuaMulti<'lua>,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua> + Default + Send + Sync + Clone + 'static,
{
    pub fn stub(value: R) -> Self {
        Self {
            callback: Arc::new(move |_, _, _, _| value.clone()),
        }
    }
}

impl<'lua, P, R> rollback_mlua::FromLua<'lua> for BattleCallback<P, R>
where
    P: for<'a> rollback_mlua::IntoLuaMulti<'a>,
    R: for<'a> rollback_mlua::FromLuaMulti<'a> + Default + Send + Sync + Clone + 'static,
{
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        let function = rollback_mlua::Function::from_lua(lua_value, lua)?;
        let vm_index = lua.named_registry_value(VM_INDEX_REGISTRY_KEY)?;
        Self::new_lua_callback(lua, vm_index, function)
    }
}
