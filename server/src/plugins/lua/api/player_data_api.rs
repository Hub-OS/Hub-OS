use super::LuaApi;
use super::lua_errors::create_player_error;
use crate::net::ItemDefinition;
use packets::structures::{ActorId, BlockColor, Emotion, PackageId};
use std::borrow::Cow;

pub fn inject_dynamic(lua_api: &mut LuaApi) {
    lua_api.add_dynamic_function("Net", "get_player_secret", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id) {
            let identity_string = lua.create_string(&player_data.identity)?;

            lua.pack_multi(identity_string)
        } else {
            Err(create_player_error(player_id))
        }
    });

    lua_api.add_dynamic_function("Net", "get_player_element", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id) {
            lua.pack_multi(player_data.element.as_str())
        } else {
            Err(create_player_error(player_id))
        }
    });

    lua_api.add_dynamic_function("Net", "get_player_health", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id) {
            lua.pack_multi(player_data.health)
        } else {
            Err(create_player_error(player_id))
        }
    });

    lua_api.add_dynamic_function("Net", "set_player_health", |api_ctx, lua, params| {
        let (player_id, health): (ActorId, i32) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_player_health(player_id, health);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_player_base_health", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id) {
            lua.pack_multi(player_data.base_health)
        } else {
            Err(create_player_error(player_id))
        }
    });

    lua_api.add_dynamic_function("Net", "set_player_base_health", |api_ctx, lua, params| {
        let (player_id, base_health): (ActorId, i32) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_player_base_health(player_id, base_health);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_player_max_health", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id) {
            lua.pack_multi(player_data.max_health().max(0))
        } else {
            Err(create_player_error(player_id))
        }
    });

    lua_api.add_dynamic_function("Net", "set_player_max_health", |api_ctx, lua, params| {
        log::warn!("Net.set_max_health() is deprecated, use Net.set_base_health()");

        let (player_id, max_health): (ActorId, i32) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_player_base_health(player_id, max_health.max(0));

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_player_emotion", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id) {
            lua.pack_multi(player_data.emotion.as_str())
        } else {
            Err(create_player_error(player_id))
        }
    });

    lua_api.add_dynamic_function("Net", "set_player_emotion", |api_ctx, lua, params| {
        let (player_id, emotion): (ActorId, String) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_player_emotion(player_id, Emotion::from(emotion));

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_player_money", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id) {
            lua.pack_multi(player_data.money)
        } else {
            Err(create_player_error(player_id))
        }
    });

    lua_api.add_dynamic_function("Net", "set_player_money", |api_ctx, lua, params| {
        let (player_id, money): (ActorId, u32) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_player_money(player_id, money);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_player_items", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id) {
            let item_iter = player_data
                .inventory
                .items()
                .filter(|&(_, &count)| count > 0)
                .enumerate()
                .map(|(i, (id, _))| (i + 1, id.as_str()));

            let item_table = lua.create_table_from(item_iter)?;

            lua.pack_multi(item_table)
        } else {
            Err(create_player_error(player_id))
        }
    });

    lua_api.add_dynamic_function("Net", "give_player_item", |api_ctx, lua, params| {
        let (player_id, item_id, count): (ActorId, String, Option<isize>) =
            lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.give_player_item(player_id, item_id, count.unwrap_or(1));

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_player_item_count", |api_ctx, lua, params| {
        let (player_id, item_id): (ActorId, mlua::String) = lua.unpack_multi(params)?;
        let item_id_str = item_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id) {
            lua.pack_multi(player_data.inventory.count_item(item_id_str))
        } else {
            Err(create_player_error(player_id))
        }
    });

    lua_api.add_dynamic_function("Net", "player_has_item", |api_ctx, lua, params| {
        let (player_id, item_id): (ActorId, mlua::String) = lua.unpack_multi(params)?;
        let item_id_str = item_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id) {
            lua.pack_multi(player_data.inventory.count_item(item_id_str) > 0)
        } else {
            Err(create_player_error(player_id))
        }
    });

    lua_api.add_dynamic_function("Net", "register_item", |api_ctx, lua, params| {
        let (item_id, item_table): (String, mlua::Table) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let item = ItemDefinition {
            name: item_table.get("name")?,
            description: item_table.get("description")?,
            consumable: item_table.get("consumable").unwrap_or_default(),
            sort_key: net.total_items(),
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
        let (player_id, card_id, code): (ActorId, mlua::String, mlua::String) =
            lua.unpack_multi(params)?;

        let (card_str, code_str) = (card_id.to_str()?, code.to_str()?);

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id) {
            let key = (Cow::Borrowed(card_str), Cow::Borrowed(code_str));
            let stored_count = player_data.owned_cards.get(&key);
            let count = stored_count.cloned().unwrap_or_default();

            lua.pack_multi(count)
        } else {
            Err(create_player_error(player_id))
        }
    });

    lua_api.add_dynamic_function("Net", "give_player_card", |api_ctx, lua, params| {
        let (player_id, package_id, code, count): (
            ActorId,
            mlua::String,
            mlua::String,
            Option<isize>,
        ) = lua.unpack_multi(params)?;

        let (package_id_str, code_str) = (package_id.to_str()?, code.to_str()?);

        let mut net = api_ctx.net_ref.borrow_mut();
        net.give_player_card(
            player_id,
            PackageId::from(package_id_str),
            code_str.to_string(),
            count.unwrap_or(1),
        );

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_player_block_count", |api_ctx, lua, params| {
        let (player_id, package_id, color_string): (ActorId, mlua::String, mlua::String) =
            lua.unpack_multi(params)?;

        let (package_id_str, color_str) = (package_id.to_str()?, color_string.to_str()?);

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id) {
            let color = BlockColor::from(color_str);

            let key = (Cow::Borrowed(package_id_str), color);
            let stored_count = player_data.owned_blocks.get(&key);
            let count = stored_count.cloned().unwrap_or_default();

            lua.pack_multi(count)
        } else {
            Err(create_player_error(player_id))
        }
    });

    lua_api.add_dynamic_function("Net", "give_player_block", |api_ctx, lua, params| {
        let (player_id, package_id, color_string, count): (
            ActorId,
            mlua::String,
            mlua::String,
            Option<isize>,
        ) = lua.unpack_multi(params)?;

        let (package_id_str, color_str) = (package_id.to_str()?, color_string.to_str()?);

        let color = BlockColor::from(color_str);

        let mut net = api_ctx.net_ref.borrow_mut();
        net.give_player_block(
            player_id,
            PackageId::from(package_id_str),
            color,
            count.unwrap_or(1),
        );

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "player_character_enabled", |api_ctx, lua, params| {
        let (player_id, package_id): (ActorId, mlua::String) = lua.unpack_multi(params)?;
        let package_id_str = package_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = &net.get_player_data(player_id) {
            let owns_character = player_data.owned_players.contains(package_id_str);

            lua.pack_multi(owns_character)
        } else {
            Err(create_player_error(player_id))
        }
    });

    lua_api.add_dynamic_function("Net", "enable_player_character", |api_ctx, lua, params| {
        let (player_id, package_id, enabled): (ActorId, mlua::String, Option<bool>) =
            lua.unpack_multi(params)?;

        let package_id_str = package_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();
        net.enable_player_character(
            player_id,
            PackageId::from(package_id_str),
            enabled.unwrap_or(true),
        );

        lua.pack_multi(())
    });
}
