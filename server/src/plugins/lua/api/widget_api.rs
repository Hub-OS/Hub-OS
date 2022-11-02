use super::lua_helpers::*;
use super::LuaApi;

#[allow(clippy::type_complexity)]
pub fn inject_dynamic(lua_api: &mut LuaApi) {
    lua_api.add_dynamic_function("Net", "is_player_in_widget", |api_ctx, lua_ctx, params| {
        let player_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        let is_in_widget = net.is_player_in_widget(player_id_str);

        lua_ctx.pack_multi(is_in_widget)
    });

    lua_api.add_dynamic_function("Net", "is_player_shopping", |api_ctx, lua_ctx, params| {
        let player_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        let is_shopping = net.is_player_shopping(player_id_str);

        lua_ctx.pack_multi(is_shopping)
    });

    lua_api.add_dynamic_function("Net", "_message_player", |api_ctx, lua_ctx, params| {
        let (player_id, message, mug_texture_path, mug_animation_path): (
            mlua::String,
            mlua::String,
            Option<mlua::String>,
            Option<mlua::String>,
        ) = lua_ctx.unpack_multi(params)?;
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

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "_question_player", |api_ctx, lua_ctx, params| {
        let (player_id, message, mug_texture_path, mug_animation_path): (
            mlua::String,
            mlua::String,
            Option<mlua::String>,
            Option<mlua::String>,
        ) = lua_ctx.unpack_multi(params)?;
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

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "_quiz_player", |api_ctx, lua_ctx, params| {
        let (player_id, option_a, option_b, option_c, mug_texture_path, mug_animation_path): (
            mlua::String,
            Option<mlua::String>,
            Option<mlua::String>,
            Option<mlua::String>,
            Option<mlua::String>,
            Option<mlua::String>,
        ) = lua_ctx.unpack_multi(params)?;
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

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "_prompt_player", |api_ctx, lua_ctx, params| {
        let (player_id, character_limit, message): (
            mlua::String,
            Option<u16>,
            Option<mlua::String>,
        ) = lua_ctx.unpack_multi(params)?;
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

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "_open_board", |api_ctx, lua_ctx, params| {
        use crate::net::BbsPost;

        let (player_id, name, color_table, post_tables, open_instantly): (
            mlua::String,
            mlua::String,
            mlua::Table,
            Vec<mlua::Table>,
            Option<bool>,
        ) = lua_ctx.unpack_multi(params)?;
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

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "prepend_posts", |api_ctx, lua_ctx, params| {
        use crate::net::BbsPost;

        let (player_id, post_tables, reference): (
            mlua::String,
            Vec<mlua::Table>,
            Option<mlua::String>,
        ) = lua_ctx.unpack_multi(params)?;
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

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "append_posts", |api_ctx, lua_ctx, params| {
        use crate::net::BbsPost;

        let (player_id, post_tables, reference): (
            mlua::String,
            Vec<mlua::Table>,
            Option<mlua::String>,
        ) = lua_ctx.unpack_multi(params)?;
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

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "remove_post", |api_ctx, lua_ctx, params| {
        let (player_id, post_id): (mlua::String, mlua::String) = lua_ctx.unpack_multi(params)?;
        let (player_id_str, post_id_str) = (player_id.to_str()?, post_id.to_str()?);

        let mut net = api_ctx.net_ref.borrow_mut();

        net.remove_post(player_id_str, post_id_str);

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "close_bbs", |api_ctx, lua_ctx, params| {
        let player_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let player_id_str = player_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.close_bbs(player_id_str);

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "_open_shop", |api_ctx, lua_ctx, params| {
        use crate::net::ShopItem;

        let (player_id, item_tables, mug_texture_path, mug_animation_path): (
            mlua::String,
            Vec<mlua::Table>,
            Option<mlua::String>,
            Option<mlua::String>,
        ) = lua_ctx.unpack_multi(params)?;
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
                let name: String = item_table.get("name")?;
                let description: String = item_table.get("description")?;
                let price: u32 = item_table.get("price")?;

                items.push(ShopItem {
                    name,
                    description,
                    price,
                });
            }

            net.open_shop(
                player_id_str,
                items,
                optional_lua_string_to_str(&mug_texture_path)?,
                optional_lua_string_to_str(&mug_animation_path)?,
            );
        }

        lua_ctx.pack_multi(())
    });
}
