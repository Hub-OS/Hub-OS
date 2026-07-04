use super::LuaApi;
use crate::net::CommandInfo;
use packets::structures::ActorId;

pub fn inject_dynamic(lua_api: &mut LuaApi) {
    lua_api.add_dynamic_function("Net", "register_command", |api_ctx, lua, params| {
        let (name, info): (String, mlua::Table) = lua.unpack_multi(params)?;

        if name.contains(|c: char| c.is_whitespace()) {
            return Err(mlua::Error::RuntimeError(String::from(
                "Commands can't contain spaces",
            )));
        }

        let mut net = api_ctx.net_ref.borrow_mut();

        let registry = net.command_registry_mut();
        registry.register_command(
            name,
            CommandInfo {
                description: info.get("description")?,
            },
        );

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "list_commands", |api_ctx, lua, params| {
        let (): () = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let registry = net.command_registry_mut();
        let list = lua.create_sequence_from(registry.commands().map(|(k, _)| k))?;

        lua.pack_multi(list)
    });

    lua_api.add_dynamic_function("Net", "get_command_description", |api_ctx, lua, params| {
        let name: mlua::String = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let registry = net.command_registry_mut();
        let description = registry
            .get_command_info(name.to_str()?)
            .map(|info| info.description.as_str());

        lua.pack_multi(description)
    });

    lua_api.add_dynamic_function("Net", "queue_command", |api_ctx, lua, params| {
        let (operator, command): (Option<ActorId>, String) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.queue_command(operator, command);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "print_to", |api_ctx, lua, params| {
        let (operator, message): (Option<ActorId>, mlua::Value) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();
        net.print_to(operator, &message.to_string()?);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "warn_to", |api_ctx, lua, params| {
        let (operator, message): (Option<ActorId>, mlua::Value) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();
        net.warn_to(operator, &message.to_string()?);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "error_to", |api_ctx, lua, params| {
        let (operator, message): (Option<ActorId>, mlua::Value) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();
        net.error_to(operator, &message.to_string()?);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "shutdown", |api_ctx, lua, _| {
        let mut net = api_ctx.net_ref.borrow_mut();
        net.shutdown();

        lua.pack_multi(())
    });
}
