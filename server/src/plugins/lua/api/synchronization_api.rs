use super::LuaApi;

pub fn inject_dynamic(lua_api: &mut LuaApi) {
    lua_api.add_static_injector(|lua_ctx| {
        lua_ctx
            .load(include_str!("synchronization_api.lua"))
            .exec()?;

        Ok(())
    });

    lua_api.add_dynamic_function(
        "Net",
        "request_update_synchronization",
        |api_ctx, lua_ctx, _| {
            let mut net = api_ctx.net_ref.borrow_mut();

            net.request_update_synchronization();

            lua_ctx.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        "Net",
        "request_disable_update_synchronization",
        |api_ctx, lua_ctx, _| {
            let mut net = api_ctx.net_ref.borrow_mut();

            net.request_disable_update_synchronization();

            lua_ctx.pack_multi(())
        },
    );
}
