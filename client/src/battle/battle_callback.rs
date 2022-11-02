use super::BattleScriptContext;
use crate::battle::{BattleSimulation, RollbackVM};
use crate::resources::Globals;
use framework::prelude::GameIO;
use std::cell::RefCell;
use std::sync::Arc;

#[derive(Clone)]
pub struct BattleCallback<P = (), R = ()>
where
    P: for<'lua> rollback_mlua::ToLuaMulti<'lua>,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua> + Default,
{
    callback:
        Arc<dyn Fn(&GameIO<Globals>, &mut BattleSimulation, &[RollbackVM], P) -> R + Sync + Send>,
}

impl<P, R> BattleCallback<P, R>
where
    P: for<'lua> rollback_mlua::ToLuaMulti<'lua>,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua> + Default,
{
    pub fn new(
        callback: impl Fn(&GameIO<Globals>, &mut BattleSimulation, &[RollbackVM], P) -> R
            + Sync
            + Send
            + 'static,
    ) -> Self {
        Self {
            callback: Arc::new(callback),
        }
    }

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
            move |game_io, simulation, vms, params| {
                let lua = &vms[vm_index].lua;
                let lua_callback: rollback_mlua::Function = lua.registry_value(&key).unwrap();

                let api_ctx = RefCell::new(BattleScriptContext {
                    vm_index,
                    vms,
                    game_io,
                    simulation,
                });

                let lua_api = &game_io.globals().battle_api;

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

    pub fn call(
        &self,
        game_io: &GameIO<Globals>,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
        params: P,
    ) -> R {
        (self.callback)(game_io, simulation, vms, params)
    }
}

impl<P, R> Default for BattleCallback<P, R>
where
    P: for<'lua> rollback_mlua::ToLuaMulti<'lua>,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua> + Default + Send + Sync + Clone + 'static,
{
    fn default() -> Self {
        Self::stub(R::default())
    }
}

impl<P, R> BattleCallback<P, R>
where
    P: for<'lua> rollback_mlua::ToLuaMulti<'lua>,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua> + Default + Send + Sync + Clone + 'static,
{
    pub fn stub(value: R) -> Self {
        Self {
            callback: Arc::new(move |_, _, _, _| value.clone()),
        }
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for BattleCallback {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        let function = rollback_mlua::Function::from_lua(lua_value, lua)?;
        let vm_index = lua.named_registry_value("vm_index")?;
        Self::new_lua_callback(lua, vm_index, function)
    }
}
