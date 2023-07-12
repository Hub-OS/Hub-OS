use super::lua_errors::create_player_error;
use super::LuaApi;
use crate::net::ItemDefinition;
use packets::structures::{BlockColor, Emotion, PackageId};
use std::borrow::Cow;

pub fn inject_dynamic(lua_api: &mut LuaApi) {
    lua_api.add_dynamic_function("Net", "get_player_secret", |api_ctx, lua, params| {
        let player_id: mlua::String = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id_str) {
            let identity_string = lua.create_string(&player_data.identity)?;

            lua.pack_multi(identity_string)
        } else {
            Err(create_player_error(player_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "get_player_element", |api_ctx, lua, params| {
        let player_id: mlua::String = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id_str) {
            lua.pack_multi(player_data.element.as_str())
        } else {
            Err(create_player_error(player_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "get_player_health", |api_ctx, lua, params| {
        let player_id: mlua::String = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id_str) {
            lua.pack_multi(player_data.health)
        } else {
            Err(create_player_error(player_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "set_player_health", |api_ctx, lua, params| {
        let (player_id, health): (mlua::String, i32) = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_player_health(player_id_str, health);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_player_base_health", |api_ctx, lua, params| {
        let player_id: mlua::String = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id_str) {
            lua.pack_multi(player_data.base_health)
        } else {
            Err(create_player_error(player_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "set_player_base_health", |api_ctx, lua, params| {
        let (player_id, base_health): (mlua::String, i32) = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_player_base_health(player_id_str, base_health);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_player_max_health", |api_ctx, lua, params| {
        let player_id: mlua::String = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id_str) {
            lua.pack_multi(player_data.max_health().max(0))
        } else {
            Err(create_player_error(player_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "set_player_max_health", |api_ctx, lua, params| {
        log::warn!("Net.set_max_health() is deprecated, use Net.set_base_health()");

        let (player_id, max_health): (mlua::String, i32) = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_player_base_health(player_id_str, max_health.max(0));

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_player_emotion", |api_ctx, lua, params| {
        let player_id: mlua::String = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id_str) {
            lua.pack_multi(player_data.emotion.as_str())
        } else {
            Err(create_player_error(player_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "set_player_emotion", |api_ctx, lua, params| {
        let (player_id, emotion): (mlua::String, String) = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_player_emotion(player_id_str, Emotion::from(emotion));

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_player_money", |api_ctx, lua, params| {
        let player_id: mlua::String = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id_str) {
            lua.pack_multi(player_data.money)
        } else {
            Err(create_player_error(player_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "set_player_money", |api_ctx, lua, params| {
        let (player_id, money): (mlua::String, u32) = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_player_money(player_id_str, money);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_player_items", |api_ctx, lua, params| {
        let player_id: mlua::String = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id_str) {
            let items: Vec<_> = player_data
                .inventory
                .items()
                .map(|(id, _)| id.clone())
                .collect();

            lua.pack_multi(items)
        } else {
            Err(create_player_error(player_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "give_player_item", |api_ctx, lua, params| {
        let (player_id, item_id, count): (mlua::String, String, Option<isize>) =
            lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.give_player_item(player_id_str, item_id, count.unwrap_or(1));

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_player_item_count", |api_ctx, lua, params| {
        let (player_id, item_id): (mlua::String, mlua::String) = lua.unpack_multi(params)?;
        let (player_id_str, item_id_str) = (player_id.to_str()?, item_id.to_str()?);

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id_str) {
            lua.pack_multi(player_data.inventory.count_item(item_id_str))
        } else {
            Err(create_player_error(player_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "player_has_item", |api_ctx, lua, params| {
        let (player_id, item_id): (mlua::String, mlua::String) = lua.unpack_multi(params)?;
        let (player_id_str, item_id_str) = (player_id.to_str()?, item_id.to_str()?);

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id_str) {
            lua.pack_multi(player_data.inventory.count_item(item_id_str) > 0)
        } else {
            Err(create_player_error(player_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "register_item", |api_ctx, lua, params| {
        let (item_id, item_table): (String, mlua::Table) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let item = ItemDefinition {
            name: item_table.get("name")?,
            description: item_table.get("description")?,
            consumable: item_table.get("consumable").unwrap_or_default(),
        };

        net.set_item(item_id, item);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_item_name", |api_ctx, lua, params| {
        let item_id: mlua::String = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let item = net.get_item(item_id.to_str()?);
        let name = item.map(|item| item.name.clone());

        lua.pack_multi(name)
    });

    lua_api.add_dynamic_function("Net", "get_item_description", |api_ctx, lua, params| {
        let item_id: mlua::String = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let item = net.get_item(item_id.to_str()?);
        let description = item.map(|item| item.description.clone());

        lua.pack_multi(description)
    });

    lua_api.add_dynamic_function("Net", "get_player_card_count", |api_ctx, lua, params| {
        let (player_id, card_id, code): (mlua::String, mlua::String, mlua::String) =
            lua.unpack_multi(params)?;

        let (player_id_str, card_str, code_str) =
            (player_id.to_str()?, card_id.to_str()?, code.to_str()?);

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id_str) {
            let key = (Cow::Borrowed(card_str), Cow::Borrowed(code_str));
            let stored_count = player_data.owned_cards.get(&key);
            let count = stored_count.cloned().unwrap_or_default();

            lua.pack_multi(count)
        } else {
            Err(create_player_error(player_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "give_player_card", |api_ctx, lua, params| {
        let (player_id, package_id, code, count): (
            mlua::String,
            mlua::String,
            mlua::String,
            Option<isize>,
        ) = lua.unpack_multi(params)?;

        let (player_id_str, package_id_str, code_str) =
            (player_id.to_str()?, package_id.to_str()?, code.to_str()?);

        let mut net = api_ctx.net_ref.borrow_mut();
        net.give_player_card(
            player_id_str,
            PackageId::from(package_id_str),
            code_str.to_string(),
            count.unwrap_or(1),
        );

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_player_block_count", |api_ctx, lua, params| {
        let (player_id, package_id, color_string): (mlua::String, mlua::String, mlua::String) =
            lua.unpack_multi(params)?;

        let (player_id_str, package_id_str, color_str) = (
            player_id.to_str()?,
            package_id.to_str()?,
            color_string.to_str()?,
        );

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id_str) {
            let color = BlockColor::from(color_str);

            let key = (Cow::Borrowed(package_id_str), color);
            let stored_count = player_data.owned_blocks.get(&key);
            let count = stored_count.cloned().unwrap_or_default();

            lua.pack_multi(count)
        } else {
            Err(create_player_error(player_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "give_player_block", |api_ctx, lua, params| {
        let (player_id, package_id, color_string, count): (
            mlua::String,
            mlua::String,
            mlua::String,
            Option<isize>,
        ) = lua.unpack_multi(params)?;

        let (player_id_str, package_id_str, color_str) = (
            player_id.to_str()?,
            package_id.to_str()?,
            color_string.to_str()?,
        );

        let color = BlockColor::from(color_str);

        let mut net = api_ctx.net_ref.borrow_mut();
        net.give_player_block(
            player_id_str,
            PackageId::from(package_id_str),
            color,
            count.unwrap_or(1),
        );

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "player_character_enabled", |api_ctx, lua, params| {
        let (player_id, package_id): (mlua::String, mlua::String) = lua.unpack_multi(params)?;
        let (player_id_str, package_id_str) = (player_id.to_str()?, package_id.to_str()?);

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id_str) {
            let owns_character = player_data.owned_players.contains(package_id_str);

            lua.pack_multi(owns_character)
        } else {
            Err(create_player_error(player_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "enable_player_character", |api_ctx, lua, params| {
        let (player_id, package_id, enabled): (mlua::String, mlua::String, Option<bool>) =
            lua.unpack_multi(params)?;

        let (player_id_str, package_id_str) = (player_id.to_str()?, package_id.to_str()?);

        let mut net = api_ctx.net_ref.borrow_mut();
        net.enable_player_character(
            player_id_str,
            PackageId::from(package_id_str),
            enabled.unwrap_or(true),
        );

        lua.pack_multi(())
    });
}
