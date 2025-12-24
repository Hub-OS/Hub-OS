use super::LuaApi;
use super::lua_helpers::*;
use crate::net::ShopItem;
use packets::ReferOptions;
use packets::structures::ActorId;
use packets::structures::{PackageId, TextStyleBlueprint, TextboxOptions, TextureAnimPathPair};

#[allow(clippy::type_complexity)]
pub fn inject_dynamic(lua_api: &mut LuaApi) {
    lua_api.add_dynamic_function("Net", "is_player_in_widget", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        let is_in_widget = net.is_player_in_widget(player_id);

        lua.pack_multi(is_in_widget)
    });

    lua_api.add_dynamic_function("Net", "is_player_shopping", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        let is_shopping = net.is_player_shopping(player_id);

        lua.pack_multi(is_shopping)
    });

    lua_api.add_dynamic_function("Net", "hide_hud", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();
        net.set_hud_visibility(player_id, false);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "show_hud", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();
        net.set_hud_visibility(player_id, true);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "_message_player", |api_ctx, lua, params| {
        let (player_id, message, rest): (ActorId, mlua::String, mlua::MultiValue) =
            lua.unpack_multi(params)?;
        let message_str = message.to_str()?;
        let textbox_options = parse_textbox_options(lua, rest)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(tracker) = api_ctx.widget_tracker_ref.borrow_mut().get_mut(&player_id) {
            tracker.track_textbox(api_ctx.script_index);

            net.message_player(player_id, message_str, textbox_options);
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "_message_player_auto", |api_ctx, lua, params| {
        let (player_id, message, close_delay, rest): (
            ActorId,
            mlua::String,
            f32,
            mlua::MultiValue,
        ) = lua.unpack_multi(params)?;
        let message_str = message.to_str()?;
        let textbox_options = parse_textbox_options(lua, rest)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(tracker) = api_ctx.widget_tracker_ref.borrow_mut().get_mut(&player_id) {
            tracker.track_textbox(api_ctx.script_index);

            net.message_player_auto(player_id, message_str, close_delay, textbox_options);
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "_question_player", |api_ctx, lua, params| {
        let (player_id, message, rest): (ActorId, mlua::String, mlua::MultiValue) =
            lua.unpack_multi(params)?;
        let message_str = message.to_str()?;
        let textbox_options = parse_textbox_options(lua, rest)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(tracker) = api_ctx.widget_tracker_ref.borrow_mut().get_mut(&player_id) {
            tracker.track_textbox(api_ctx.script_index);

            net.question_player(player_id, message_str, textbox_options);
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "_quiz_player", |api_ctx, lua, params| {
        let (player_id, option_a, option_b, option_c, rest): (
            ActorId,
            Option<mlua::String>,
            Option<mlua::String>,
            Option<mlua::String>,
            mlua::MultiValue,
        ) = lua.unpack_multi(params)?;

        let textbox_options = parse_textbox_options(lua, rest)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(tracker) = api_ctx.widget_tracker_ref.borrow_mut().get_mut(&player_id) {
            tracker.track_textbox(api_ctx.script_index);

            net.quiz_player(
                player_id,
                optional_lua_string_to_str(&option_a)?,
                optional_lua_string_to_str(&option_b)?,
                optional_lua_string_to_str(&option_c)?,
                textbox_options,
            );
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "_prompt_player", |api_ctx, lua, params| {
        let (player_id, character_limit, message): (ActorId, Option<u16>, Option<mlua::String>) =
            lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(tracker) = api_ctx.widget_tracker_ref.borrow_mut().get_mut(&player_id) {
            tracker.track_textbox(api_ctx.script_index);

            net.prompt_player(
                player_id,
                character_limit.unwrap_or(u16::MAX),
                optional_lua_string_to_optional_str(&message)?,
            );
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "_open_board", |api_ctx, lua, params| {
        use crate::net::BbsPost;

        let (player_id, name, color_table, post_tables, open_instantly): (
            ActorId,
            mlua::String,
            mlua::Table,
            Vec<mlua::Table>,
            Option<bool>,
        ) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(tracker) = api_ctx.widget_tracker_ref.borrow_mut().get_mut(&player_id) {
            tracker.track_board(api_ctx.script_index);

            let color = parse_rgb_table(color_table)?;

            let mut posts = Vec::with_capacity(post_tables.len());

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
                player_id,
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

        let (player_id, post_tables, reference): (ActorId, Vec<mlua::Table>, Option<mlua::String>) =
            lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let mut posts = Vec::with_capacity(post_tables.len());

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
            player_id,
            optional_lua_string_to_optional_str(&reference)?,
            posts,
        );

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "append_posts", |api_ctx, lua, params| {
        use crate::net::BbsPost;

        let (player_id, post_tables, reference): (ActorId, Vec<mlua::Table>, Option<mlua::String>) =
            lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let mut posts = Vec::with_capacity(post_tables.len());

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
            player_id,
            optional_lua_string_to_optional_str(&reference)?,
            posts,
        );

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "remove_post", |api_ctx, lua, params| {
        let (player_id, post_id): (ActorId, mlua::String) = lua.unpack_multi(params)?;
        let post_id_str = post_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.remove_post(player_id, post_id_str);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "close_board", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.close_board(player_id);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "_open_shop", |api_ctx, lua, params| {
        let (player_id, item_tables, rest): (ActorId, Vec<mlua::Table>, mlua::MultiValue) =
            lua.unpack_multi(params)?;

        let textbox_options = parse_textbox_options(lua, rest)?;

        if let Some(tracker) = api_ctx.widget_tracker_ref.borrow_mut().get_mut(&player_id) {
            tracker.track_shop(api_ctx.script_index);
            let mut net = api_ctx.net_ref.borrow_mut();

            let mut items = Vec::with_capacity(item_tables.len());

            for item_table in item_tables {
                items.push(table_to_shop_item(item_table)?);
            }

            net.open_shop(player_id, items, textbox_options);
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "set_shop_message", |api_ctx, lua, params| {
        let (player_id, message): (ActorId, String) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();
        net.set_shop_message(player_id, message);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "prepend_shop_items", |api_ctx, lua, params| {
        let (player_id, item_tables, reference): (ActorId, Vec<mlua::Table>, Option<mlua::String>) =
            lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let mut items = Vec::with_capacity(item_tables.len());

        for item_table in item_tables {
            items.push(table_to_shop_item(item_table)?);
        }

        net.prepend_shop_items(
            player_id,
            optional_lua_string_to_optional_str(&reference)?,
            items,
        );

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "append_shop_items", |api_ctx, lua, params| {
        let (player_id, item_tables, reference): (ActorId, Vec<mlua::Table>, Option<mlua::String>) =
            lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let mut items = Vec::with_capacity(item_tables.len());

        for item_table in item_tables {
            items.push(table_to_shop_item(item_table)?);
        }

        net.append_shop_items(
            player_id,
            optional_lua_string_to_optional_str(&reference)?,
            items,
        );

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "update_shop_item", |api_ctx, lua, params| {
        let (player_id, table): (ActorId, mlua::Table) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();
        net.update_shop_item(player_id, table_to_shop_item(table)?);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "remove_shop_item", |api_ctx, lua, params| {
        let (player_id, id): (ActorId, String) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();
        net.remove_shop_item(player_id, id);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "refer_link", |api_ctx, lua, params| {
        let (player_id, address): (ActorId, String) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.refer_link(player_id, address);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "refer_server", |api_ctx, lua, params| {
        let (player_id, name, address): (ActorId, String, String) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.refer_server(player_id, name, address);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "refer_package", |api_ctx, lua, params| {
        let (player_id, package_id_string, options): (ActorId, mlua::String, Option<mlua::Table>) =
            lua.unpack_multi(params)?;

        let package_id_str = package_id_string.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let package_id = PackageId::from(package_id_str);
        net.refer_package(
            player_id,
            package_id,
            options
                .map(|o| ReferOptions {
                    unless_installed: o.get::<_, bool>("unless_installed").is_ok_and(|b| b),
                })
                .unwrap_or_default(),
        );

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "offer_package", |api_ctx, lua, params| {
        let (player_id, package_id): (ActorId, mlua::String) = lua.unpack_multi(params)?;

        let package_id_str = package_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.offer_package(player_id, package_id_str);

        lua.pack_multi(())
    });
}

fn parse_textbox_options<'lua>(
    lua: &'lua mlua::Lua,
    mut rest: mlua::MultiValue<'lua>,
) -> mlua::Result<TextboxOptions> {
    let mut textbox_options = TextboxOptions::default();

    let Some(first_value) = rest.pop_front() else {
        // nil, return default
        return Ok(textbox_options);
    };

    match first_value {
        mlua::Value::Table(table) => {
            // parse mug
            textbox_options.mug = table
                .get::<_, Option<mlua::Table>>("mug")?
                .map(parse_texture_animation_pair)
                .transpose()?;

            // parse text style
            textbox_options.text_style = table
                .get::<_, Option<mlua::Table>>("text_style")?
                .map(parse_text_style)
                .transpose()?;
        }
        mlua::Value::String(mug_texture_path) => {
            // deprecation compatibility
            let mug_animation_path: String = lua.unpack_multi(rest)?;

            textbox_options.mug = Some(TextureAnimPathPair {
                texture: mug_texture_path.to_str()?.to_string().into(),
                animation: mug_animation_path.into(),
            });
        }
        _ => {}
    }

    Ok(textbox_options)
}

fn parse_texture_animation_pair(table: mlua::Table) -> mlua::Result<TextureAnimPathPair<'static>> {
    Ok(TextureAnimPathPair {
        texture: table.get::<_, String>("texture_path")?.into(),
        animation: table.get::<_, String>("animation_path")?.into(),
    })
}

pub fn parse_text_style(table: mlua::Table) -> mlua::Result<TextStyleBlueprint> {
    fn get_option<'lua, T: mlua::FromLua<'lua>>(
        table: &mlua::Table<'lua>,
        key: &str,
    ) -> mlua::Result<Option<T>> {
        table.get(key)
    }

    Ok(TextStyleBlueprint {
        font_name: get_option::<String>(&table, "font")?.unwrap_or_default(),
        monospace: get_option::<bool>(&table, "monospace")?.unwrap_or_default(),
        min_glyph_width: get_option::<f32>(&table, "min_glyph_width")?.unwrap_or(6.0),
        letter_spacing: get_option::<f32>(&table, "letter_spacing")?.unwrap_or(1.0),
        line_spacing: get_option::<f32>(&table, "line_spacing")?.unwrap_or(3.0),
        scale_x: get_option::<f32>(&table, "scale_x")?.unwrap_or(1.0),
        scale_y: get_option::<f32>(&table, "scale_y")?.unwrap_or(1.0),
        color: get_option::<mlua::Table>(&table, "color")?
            .map(parse_rgb_table)
            .transpose()?
            .unwrap_or((255, 255, 255)),
        shadow_color: get_option::<mlua::Table>(&table, "shadow_color")?
            .map(parse_rgba_table)
            .transpose()?
            .unwrap_or_default(),
        custom_atlas: parse_texture_animation_pair(table).ok(),
    })
}

fn parse_rgb_table(table: mlua::Table) -> mlua::Result<(u8, u8, u8)> {
    Ok((table.get("r")?, table.get("g")?, table.get("b")?))
}

fn parse_rgba_table(table: mlua::Table) -> mlua::Result<(u8, u8, u8, u8)> {
    Ok((
        table.get("r")?,
        table.get("g")?,
        table.get("b")?,
        table.get::<_, Option<u8>>("a")?.unwrap_or(255),
    ))
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
