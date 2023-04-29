use super::lua_helpers::*;
use super::LuaApi;
use crate::net::ShopItem;

#[allow(clippy::type_complexity)]
pub fn inject_dynamic(lua_api: &mut LuaApi) {
    lua_api.add_dynamic_function("Net", "is_player_in_widget", |api_ctx, lua, params| {
        let player_id: mlua::String = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        let is_in_widget = net.is_player_in_widget(player_id_str);

        lua.pack_multi(is_in_widget)
    });

    lua_api.add_dynamic_function("Net", "is_player_shopping", |api_ctx, lua, params| {
        let player_id: mlua::String = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        let is_shopping = net.is_player_shopping(player_id_str);

        lua.pack_multi(is_shopping)
    });

    lua_api.add_dynamic_function("Net", "_message_player", |api_ctx, lua, params| {
        let (player_id, message, mug_texture_path, mug_animation_path): (
            mlua::String,
            mlua::String,
            Option<mlua::String>,
            Option<mlua::String>,
        ) = lua.unpack_multi(params)?;
        let (player_id_str, message_str) = (player_id.to_str()?, message.to_str()?);

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(tracker) = api_ctx
            .widget_tracker_ref
            .borrow_mut()
            .get_mut(player_id_str)
        {
            tracker.track_textbox(api_ctx.script_index);

            net.message_player(
                player_id_str,
                message_str,
                optional_lua_string_to_str(&mug_texture_path)?,
                optional_lua_string_to_str(&mug_animation_path)?,
            );
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "_question_player", |api_ctx, lua, params| {
        let (player_id, message, mug_texture_path, mug_animation_path): (
            mlua::String,
            mlua::String,
            Option<mlua::String>,
            Option<mlua::String>,
        ) = lua.unpack_multi(params)?;
        let (player_id_str, message_str) = (player_id.to_str()?, message.to_str()?);

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(tracker) = api_ctx
            .widget_tracker_ref
            .borrow_mut()
            .get_mut(player_id_str)
        {
            tracker.track_textbox(api_ctx.script_index);

            net.question_player(
                player_id_str,
                message_str,
                optional_lua_string_to_str(&mug_texture_path)?,
                optional_lua_string_to_str(&mug_animation_path)?,
            );
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "_quiz_player", |api_ctx, lua, params| {
        let (player_id, option_a, option_b, option_c, mug_texture_path, mug_animation_path): (
            mlua::String,
            Option<mlua::String>,
            Option<mlua::String>,
            Option<mlua::String>,
            Option<mlua::String>,
            Option<mlua::String>,
        ) = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(tracker) = api_ctx
            .widget_tracker_ref
            .borrow_mut()
            .get_mut(player_id_str)
        {
            tracker.track_textbox(api_ctx.script_index);

            net.quiz_player(
                player_id_str,
                optional_lua_string_to_str(&option_a)?,
                optional_lua_string_to_str(&option_b)?,
                optional_lua_string_to_str(&option_c)?,
                optional_lua_string_to_str(&mug_texture_path)?,
                optional_lua_string_to_str(&mug_animation_path)?,
            );
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "_prompt_player", |api_ctx, lua, params| {
        let (player_id, character_limit, message): (
            mlua::String,
            Option<u16>,
            Option<mlua::String>,
        ) = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(tracker) = api_ctx
            .widget_tracker_ref
            .borrow_mut()
            .get_mut(player_id_str)
        {
            tracker.track_textbox(api_ctx.script_index);

            net.prompt_player(
                player_id_str,
                character_limit.unwrap_or(u16::MAX),
                optional_lua_string_to_optional_str(&message)?,
            );
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "_open_board", |api_ctx, lua, params| {
        use crate::net::BbsPost;

        let (player_id, name, color_table, post_tables, open_instantly): (
            mlua::String,
            mlua::String,
            mlua::Table,
            Vec<mlua::Table>,
            Option<bool>,
        ) = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(tracker) = api_ctx
            .widget_tracker_ref
            .borrow_mut()
            .get_mut(player_id_str)
        {
            tracker.track_board(api_ctx.script_index);

            let color = (
                color_table.get("r")?,
                color_table.get("g")?,
                color_table.get("b")?,
            );

            let mut posts = Vec::new();
            posts.reserve(post_tables.len());

            for post_table in post_tables {
                let read: Option<bool> = post_table.get("read")?;
                let title: Option<String> = post_table.get("title")?;
                let author: Option<String> = post_table.get("author")?;

                posts.push(BbsPost {
                    id: post_table.get("id")?,
                    read: read.unwrap_or_default(),
                    title: title.unwrap_or_default(),
                    author: author.unwrap_or_default(),
                });
            }

            net.open_board(
                player_id_str,
                name.to_str()?,
                color,
                posts,
                open_instantly.unwrap_or_default(),
            );
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "prepend_posts", |api_ctx, lua, params| {
        use crate::net::BbsPost;

        let (player_id, post_tables, reference): (
            mlua::String,
            Vec<mlua::Table>,
            Option<mlua::String>,
        ) = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let mut posts = Vec::new();
        posts.reserve(post_tables.len());

        for post_table in post_tables {
            let read: Option<bool> = post_table.get("read")?;
            let title: Option<String> = post_table.get("title")?;
            let author: Option<String> = post_table.get("author")?;

            posts.push(BbsPost {
                id: post_table.get("id")?,
                read: read.unwrap_or_default(),
                title: title.unwrap_or_default(),
                author: author.unwrap_or_default(),
            });
        }

        net.prepend_posts(
            player_id_str,
            optional_lua_string_to_optional_str(&reference)?,
            posts,
        );

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "append_posts", |api_ctx, lua, params| {
        use crate::net::BbsPost;

        let (player_id, post_tables, reference): (
            mlua::String,
            Vec<mlua::Table>,
            Option<mlua::String>,
        ) = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let mut posts = Vec::new();
        posts.reserve(post_tables.len());

        for post_table in post_tables {
            let read: Option<bool> = post_table.get("read")?;
            let title: Option<String> = post_table.get("title")?;
            let author: Option<String> = post_table.get("author")?;

            posts.push(BbsPost {
                id: post_table.get("id")?,
                read: read.unwrap_or_default(),
                title: title.unwrap_or_default(),
                author: author.unwrap_or_default(),
            });
        }

        net.append_posts(
            player_id_str,
            optional_lua_string_to_optional_str(&reference)?,
            posts,
        );

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "remove_post", |api_ctx, lua, params| {
        let (player_id, post_id): (mlua::String, mlua::String) = lua.unpack_multi(params)?;
        let (player_id_str, post_id_str) = (player_id.to_str()?, post_id.to_str()?);

        let mut net = api_ctx.net_ref.borrow_mut();

        net.remove_post(player_id_str, post_id_str);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "close_bbs", |api_ctx, lua, params| {
        log::warn!("Net.close_bbs() is deprecated, use Net.close_board()");

        let player_id: mlua::String = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.close_board(player_id_str);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "close_board", |api_ctx, lua, params| {
        let player_id: mlua::String = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.close_board(player_id_str);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "_open_shop", |api_ctx, lua, params| {
        let (player_id, item_tables, mug_texture_path, mug_animation_path): (
            mlua::String,
            Vec<mlua::Table>,
            Option<mlua::String>,
            Option<mlua::String>,
        ) = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        if let Some(tracker) = api_ctx
            .widget_tracker_ref
            .borrow_mut()
            .get_mut(player_id_str)
        {
            tracker.track_shop(api_ctx.script_index);
            let mut net = api_ctx.net_ref.borrow_mut();

            let mut items = Vec::new();
            items.reserve(item_tables.len());

            for item_table in item_tables {
                items.push(table_to_shop_item(item_table)?);
            }

            net.open_shop(
                player_id_str,
                items,
                optional_lua_string_to_str(&mug_texture_path)?,
                optional_lua_string_to_str(&mug_animation_path)?,
            );
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "set_shop_message", |api_ctx, lua, params| {
        let (player_id, message): (mlua::String, String) = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();
        net.set_shop_message(player_id_str, message);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "update_shop_item", |api_ctx, lua, params| {
        let (player_id, table): (mlua::String, mlua::Table) = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();
        net.update_shop_item(player_id_str, table_to_shop_item(table)?);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "remove_shop_item", |api_ctx, lua, params| {
        let (player_id, id): (mlua::String, String) = lua.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();
        net.remove_shop_item(player_id_str, id);

        lua.pack_multi(())
    });
}

fn table_to_shop_item(item_table: mlua::Table) -> mlua::Result<ShopItem> {
    let id: Option<String> = item_table.get("id")?;
    let name: String = item_table.get("name")?;
    let price: mlua::Value = item_table.get("price")?;

    let price_text = match price {
        mlua::Value::Integer(n) => n.to_string(),
        mlua::Value::Number(n) => n.to_string(),
        mlua::Value::String(s) => String::from(s.to_str()?),
        _ => String::new(),
    };

    Ok(ShopItem {
        id,
        name,
        price_text,
    })
}
