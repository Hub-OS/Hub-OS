use super::lua_errors::create_player_error;
use super::LuaApi;
use crate::net::Item;

pub fn inject_dynamic(lua_api: &mut LuaApi) {
    lua_api.add_dynamic_function("Net", "get_player_secret", |api_ctx, lua_ctx, params| {
        let player_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id_str) {
            let identity_string = lua_ctx.create_string(&player_data.identity)?;

            lua_ctx.pack_multi(identity_string)
        } else {
            Err(create_player_error(player_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "get_player_element", |api_ctx, lua_ctx, params| {
        let player_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id_str) {
            lua_ctx.pack_multi(player_data.element.as_str())
        } else {
            Err(create_player_error(player_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "get_player_health", |api_ctx, lua_ctx, params| {
        let player_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id_str) {
            lua_ctx.pack_multi(player_data.health)
        } else {
            Err(create_player_error(player_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "set_player_health", |api_ctx, lua_ctx, params| {
        let (player_id, health): (mlua::String, u32) = lua_ctx.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_player_health(player_id_str, health);

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function(
        "Net",
        "get_player_max_health",
        |api_ctx, lua_ctx, params| {
            let player_id: mlua::String = lua_ctx.unpack_multi(params)?;
            let player_id_str = player_id.to_str()?;

            let net = api_ctx.net_ref.borrow();

            if let Some(player_data) = &net.get_player_data(player_id_str) {
                lua_ctx.pack_multi(player_data.max_health)
            } else {
                Err(create_player_error(player_id_str))
            }
        },
    );

    lua_api.add_dynamic_function(
        "Net",
        "set_player_max_health",
        |api_ctx, lua_ctx, params| {
            let (player_id, max_health): (mlua::String, u32) = lua_ctx.unpack_multi(params)?;
            let player_id_str = player_id.to_str()?;

            let mut net = api_ctx.net_ref.borrow_mut();

            net.set_player_max_health(player_id_str, max_health);

            lua_ctx.pack_multi(())
        },
    );

    lua_api.add_dynamic_function("Net", "get_player_emotion", |api_ctx, lua_ctx, params| {
        let player_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id_str) {
            lua_ctx.pack_multi(player_data.emotion)
        } else {
            Err(create_player_error(player_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "set_player_emotion", |api_ctx, lua_ctx, params| {
        let (player_id, emotion): (mlua::String, u8) = lua_ctx.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_player_emotion(player_id_str, emotion);

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_player_money", |api_ctx, lua_ctx, params| {
        let player_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id_str) {
            lua_ctx.pack_multi(player_data.money)
        } else {
            Err(create_player_error(player_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "set_player_money", |api_ctx, lua_ctx, params| {
        let (player_id, money): (mlua::String, u32) = lua_ctx.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_player_money(player_id_str, money);

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_player_items", |api_ctx, lua_ctx, params| {
        let player_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id_str) {
            lua_ctx.pack_multi(player_data.items.clone())
        } else {
            Err(create_player_error(player_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "give_player_item", |api_ctx, lua_ctx, params| {
        let (player_id, item_id): (mlua::String, String) = lua_ctx.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.give_player_item(player_id_str, item_id);

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "remove_player_item", |api_ctx, lua_ctx, params| {
        let (player_id, item_id): (mlua::String, mlua::String) = lua_ctx.unpack_multi(params)?;
        let (player_id_str, item_id_str) = (player_id.to_str()?, item_id.to_str()?);

        let mut net = api_ctx.net_ref.borrow_mut();

        net.remove_player_item(player_id_str, item_id_str);

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "player_has_item", |api_ctx, lua_ctx, params| {
        let (player_id, item_id): (mlua::String, String) = lua_ctx.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id_str) {
            lua_ctx.pack_multi(player_data.items.contains(&item_id))
        } else {
            Err(create_player_error(player_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "create_item", |api_ctx, lua_ctx, params| {
        let (item_id, item_table): (String, mlua::Table) = lua_ctx.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let item = Item {
            name: item_table.get("name")?,
            description: item_table.get("description")?,
        };

        net.set_item(item_id, item);

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_item_name", |api_ctx, lua_ctx, params| {
        let item_id: mlua::String = lua_ctx.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let item = net.get_item(item_id.to_str()?);
        let name = item.map(|item| item.name.clone());

        lua_ctx.pack_multi(name)
    });

    lua_api.add_dynamic_function("Net", "get_item_description", |api_ctx, lua_ctx, params| {
        let item_id: mlua::String = lua_ctx.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let item = net.get_item(item_id.to_str()?);
        let description = item.map(|item| item.description.clone());

        lua_ctx.pack_multi(description)
    });
}
