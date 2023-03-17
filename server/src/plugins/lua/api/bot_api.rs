use super::lua_errors::{create_area_error, create_bot_error};
use super::LuaApi;
use crate::net::{Actor, Direction};

pub fn inject_dynamic(lua_api: &mut LuaApi) {
    lua_api.add_dynamic_function("Net", "list_bots", |api_ctx, lua_ctx, params| {
        let area_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(area) = net.get_area_mut(area_id_str) {
            let connected_bots_iter = area.connected_bots().iter();

            let result: mlua::Result<Vec<mlua::String>> = connected_bots_iter
                .map(|bot_id| lua_ctx.create_string(&bot_id))
                .collect();

            lua_ctx.pack_multi(result?)
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "create_bot", |api_ctx, lua_ctx, params| {
        use std::time::Instant;
        use uuid::Uuid;

        // (bot_id, table) or (table, nil)
        let (bot_id_or_table, optional_table): (mlua::Value, mlua::Value) =
            lua_ctx.unpack_multi(params)?;

        let bot_id;
        let table: mlua::Table;

        if let mlua::Value::Table(bot_table) = bot_id_or_table {
            // (table, nil)
            bot_id = Uuid::new_v4().to_string();
            table = bot_table;
        } else {
            // (bot_id, table)
            bot_id = lua_ctx.unpack(bot_id_or_table)?;
            table = lua_ctx.unpack(optional_table)?;
        }

        let mut net = api_ctx.net_ref.borrow_mut();

        let name: Option<String> = table.get("name")?;
        let area_id: Option<String> = table.get("area_id")?;
        let warp_in: Option<bool> = table.get("warp_in")?;
        let texture_path: Option<String> = table.get("texture_path")?;
        let animation_path: Option<String> = table.get("animation_path")?;
        let animation: Option<String> = table.get("animation")?;
        let x: Option<f32> = table.get("x")?;
        let y: Option<f32> = table.get("y")?;
        let z: Option<f32> = table.get("z")?;
        let direction: Option<String> = table.get("direction")?;
        let solid: Option<bool> = table.get("solid")?;

        let area_id = area_id.unwrap_or_else(|| String::from("default"));

        if let Some(area) = net.get_area(&area_id) {
            let map = area.map();
            let spawn = map.spawn_position();
            let spawn_direction = map.spawn_direction();

            let direction = direction
                .map(|string| Direction::from(&string))
                .unwrap_or(spawn_direction);

            let bot = Actor {
                id: bot_id.clone(),
                name: name.unwrap_or_default(),
                area_id,
                texture_path: texture_path.unwrap_or_default(),
                animation_path: animation_path.unwrap_or_default(),
                mugshot_texture_path: String::default(),
                mugshot_animation_path: String::default(),
                direction,
                x: x.unwrap_or(spawn.0),
                y: y.unwrap_or(spawn.1),
                z: z.unwrap_or(spawn.2),
                last_movement_time: Instant::now(),
                scale_x: 1.0,
                scale_y: 1.0,
                rotation: 0.0,
                map_color: (0, 0, 0, 0),
                current_animation: animation,
                solid: solid.unwrap_or_default(),
                child_sprites: Vec::new(),
            };

            net.add_bot(bot, warp_in.unwrap_or(true));

            lua_ctx.pack_multi(bot_id)
        } else {
            Err(create_area_error(&area_id))
        }
    });

    lua_api.add_dynamic_function("Net", "is_bot", |api_ctx, lua_ctx, params| {
        let bot_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let bot_id_str = bot_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        let bot_exists = net.get_bot(bot_id_str).is_some();

        lua_ctx.pack_multi(bot_exists)
    });

    lua_api.add_dynamic_function("Net", "remove_bot", |api_ctx, lua_ctx, params| {
        let (bot_id, warp_out): (mlua::String, Option<bool>) = lua_ctx.unpack_multi(params)?;
        let bot_id_str = bot_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.remove_bot(bot_id_str, warp_out.unwrap_or_default());

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_bot_area", |api_ctx, lua_ctx, params| {
        let bot_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let bot_id_str = bot_id.to_str()?;

        let net = api_ctx.net_ref.borrow_mut();

        if let Some(bot) = net.get_bot(bot_id_str) {
            lua_ctx.pack_multi(bot.area_id.as_str())
        } else {
            Err(create_bot_error(bot_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "get_bot_name", |api_ctx, lua_ctx, params| {
        let bot_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let bot_id_str = bot_id.to_str()?;

        let net = api_ctx.net_ref.borrow_mut();

        if let Some(bot) = net.get_bot(bot_id_str) {
            lua_ctx.pack_multi(bot.name.as_str())
        } else {
            Err(create_bot_error(bot_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "set_bot_name", |api_ctx, lua_ctx, params| {
        let (bot_id, name): (mlua::String, mlua::String) = lua_ctx.unpack_multi(params)?;
        let bot_id_str = bot_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_bot_name(bot_id_str, name.to_str()?);

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_bot_direction", |api_ctx, lua_ctx, params| {
        let bot_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let bot_id_str = bot_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(bot) = net.get_bot(bot_id_str) {
            let direction_str: &str = bot.direction.into();

            lua_ctx.pack_multi(direction_str)
        } else {
            Err(create_bot_error(bot_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "set_bot_direction", |api_ctx, lua_ctx, params| {
        let (bot_id, direction_string): (mlua::String, String) = lua_ctx.unpack_multi(params)?;
        let bot_id_str = bot_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_bot_direction(bot_id_str, Direction::from(&direction_string));

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_bot_position", |api_ctx, lua_ctx, params| {
        let bot_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let bot_id_str = bot_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(bot) = net.get_bot(bot_id_str) {
            let table = lua_ctx.create_table()?;
            table.set("x", bot.x)?;
            table.set("y", bot.y)?;
            table.set("z", bot.z)?;

            lua_ctx.pack_multi(table)
        } else {
            Err(create_bot_error(bot_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "move_bot", |api_ctx, lua_ctx, params| {
        let (bot_id, x, y, z): (mlua::String, f32, f32, f32) = lua_ctx.unpack_multi(params)?;
        let bot_id_str = bot_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.move_bot(bot_id_str, x, y, z);

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "animate_bot", |api_ctx, lua_ctx, params| {
        let (bot_id, name, loop_option): (mlua::String, mlua::String, Option<bool>) =
            lua_ctx.unpack_multi(params)?;
        let (bot_id_str, name_str) = (bot_id.to_str()?, name.to_str()?);

        let mut net = api_ctx.net_ref.borrow_mut();

        let loop_animation = loop_option.unwrap_or_default();

        net.animate_bot(bot_id_str, name_str, loop_animation);

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function(
        "Net",
        "animate_bot_properties",
        |api_ctx, lua_ctx, params| {
            use super::actor_property_animation::parse_animation;

            let (player_id, keyframe_tables): (mlua::String, Vec<mlua::Table>) =
                lua_ctx.unpack_multi(params)?;
            let player_id_str = player_id.to_str()?;

            let mut net = api_ctx.net_ref.borrow_mut();

            let animation = parse_animation(keyframe_tables)?;
            net.animate_bot_properties(player_id_str, animation);

            lua_ctx.pack_multi(())
        },
    );

    lua_api.add_dynamic_function("Net", "set_bot_avatar", |api_ctx, lua_ctx, params| {
        let (bot_id, texture_path, animation_path): (mlua::String, mlua::String, mlua::String) =
            lua_ctx.unpack_multi(params)?;
        let bot_id_str = bot_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_bot_avatar(bot_id_str, texture_path.to_str()?, animation_path.to_str()?);

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "set_bot_emote", |api_ctx, lua_ctx, params| {
        let (bot_id, emote_id, use_custom_emotes): (mlua::String, u8, Option<bool>) =
            lua_ctx.unpack_multi(params)?;
        let bot_id_str = bot_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_bot_emote(bot_id_str, emote_id, use_custom_emotes.unwrap_or_default());

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "set_bot_map_color", |api_ctx, lua_ctx, params| {
        let (bot_id, color_table): (mlua::String, mlua::Table) = lua_ctx.unpack_multi(params)?;
        let bot_id_str = bot_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let color = (
            color_table.get("r")?,
            color_table.get("g")?,
            color_table.get("b")?,
            color_table.get("a").unwrap_or(255),
        );

        net.set_bot_map_color(bot_id_str, color);

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "transfer_bot", |api_ctx, lua_ctx, params| {
        let (bot_id, area_id, warp_in_option, x_option, y_option, z_option): (
            mlua::String,
            String,
            Option<bool>,
            Option<f32>,
            Option<f32>,
            Option<f32>,
        ) = lua_ctx.unpack_multi(params)?;
        let bot_id_str = bot_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();
        let warp_in = warp_in_option.unwrap_or(true);
        let x;
        let y;
        let z;

        if let Some(bot) = net.get_bot(bot_id_str) {
            x = x_option.unwrap_or(bot.x);
            y = y_option.unwrap_or(bot.y);
            z = z_option.unwrap_or(bot.z);
        } else {
            return Err(create_bot_error(bot_id_str));
        }

        net.transfer_bot(bot_id_str, &area_id, warp_in, x, y, z);

        lua_ctx.pack_multi(())
    });
}
