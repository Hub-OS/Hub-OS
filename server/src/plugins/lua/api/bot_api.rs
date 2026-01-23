use super::LuaApi;
use super::lua_errors::{create_area_error, create_bot_error};
use crate::net::{Actor, Direction};
use packets::structures::ActorId;

pub fn inject_dynamic(lua_api: &mut LuaApi) {
    lua_api.add_dynamic_function("Net", "list_bots", |api_ctx, lua, params| {
        let area_id: mlua::String = lua.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(area) = net.get_area_mut(area_id_str) {
            let connected_players_iter = area
                .connected_bots()
                .iter()
                .enumerate()
                .map(|(i, id)| (i + 1, *id));

            let result = lua.create_table_from(connected_players_iter);

            lua.pack_multi(result?)
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "create_bot", |api_ctx, lua, params| {
        use std::time::Instant;

        let table: mlua::Table = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let name: Option<String> = table.get("name")?;
        let area_id: Option<String> = table.get("area_id")?;
        let warp_in: Option<bool> = table.get("warp_in")?;
        let texture_path: Option<String> = table.get("texture_path")?;
        let animation_path: Option<String> = table.get("animation_path")?;
        let animation: Option<String> = table.get("animation")?;
        let loop_animation: Option<bool> = table.get("loop_animation")?;
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

            let bot_id = net.create_actor_id();

            let bot = Actor {
                id: bot_id,
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
                loop_animation: loop_animation.unwrap_or(false),
                solid: solid.unwrap_or_default(),
                child_sprites: Vec::new(),
            };

            net.add_bot(bot, warp_in.unwrap_or(true));

            lua.pack_multi(bot_id)
        } else {
            Err(create_area_error(&area_id))
        }
    });

    lua_api.add_dynamic_function("Net", "is_bot", |api_ctx, lua, params| {
        let bot_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        let bot_exists = net.get_bot(bot_id).is_some();

        lua.pack_multi(bot_exists)
    });

    lua_api.add_dynamic_function("Net", "remove_bot", |api_ctx, lua, params| {
        let (bot_id, warp_out): (ActorId, Option<bool>) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.remove_bot(bot_id, warp_out.unwrap_or_default());

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_bot_area", |api_ctx, lua, params| {
        let bot_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow_mut();

        if let Some(bot) = net.get_bot(bot_id) {
            lua.pack_multi(bot.area_id.as_str())
        } else {
            Err(create_bot_error(bot_id))
        }
    });

    lua_api.add_dynamic_function("Net", "get_bot_name", |api_ctx, lua, params| {
        let bot_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow_mut();

        if let Some(bot) = net.get_bot(bot_id) {
            lua.pack_multi(bot.name.as_str())
        } else {
            Err(create_bot_error(bot_id))
        }
    });

    lua_api.add_dynamic_function("Net", "set_bot_name", |api_ctx, lua, params| {
        let (bot_id, name): (ActorId, mlua::String) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_bot_name(bot_id, name.to_str()?);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_bot_direction", |api_ctx, lua, params| {
        let bot_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        if let Some(bot) = net.get_bot(bot_id) {
            let direction_str: &str = bot.direction.into();

            lua.pack_multi(direction_str)
        } else {
            Err(create_bot_error(bot_id))
        }
    });

    lua_api.add_dynamic_function("Net", "set_bot_direction", |api_ctx, lua, params| {
        let (bot_id, direction_string): (ActorId, String) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_bot_direction(bot_id, Direction::from(&direction_string));

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_bot_position", |api_ctx, lua, params| {
        let bot_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        if let Some(bot) = net.get_bot(bot_id) {
            let table = lua.create_table()?;
            table.set("x", bot.x)?;
            table.set("y", bot.y)?;
            table.set("z", bot.z)?;

            lua.pack_multi(table)
        } else {
            Err(create_bot_error(bot_id))
        }
    });

    lua_api.add_dynamic_function("Net", "get_bot_position_multi", |api_ctx, lua, params| {
        let bot_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        if let Some(bot) = net.get_bot(bot_id) {
            lua.pack_multi((bot.x, bot.y, bot.z))
        } else {
            Err(create_bot_error(bot_id))
        }
    });

    lua_api.add_dynamic_function("Net", "move_bot", |api_ctx, lua, params| {
        let (bot_id, x, y, z): (ActorId, f32, f32, f32) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.move_bot(bot_id, x, y, z);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "animate_bot", |api_ctx, lua, params| {
        let (bot_id, name, loop_option): (ActorId, mlua::String, Option<bool>) =
            lua.unpack_multi(params)?;
        let name_str = name.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let loop_animation = loop_option.unwrap_or_default();

        net.animate_bot(bot_id, name_str, loop_animation);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "animate_bot_properties", |api_ctx, lua, params| {
        use super::actor_property_animation::parse_animation;

        let (bot_id, keyframe_tables): (ActorId, Vec<mlua::Table>) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let animation = parse_animation(keyframe_tables)?;
        net.animate_bot_properties(bot_id, animation);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "set_bot_avatar", |api_ctx, lua, params| {
        let (bot_id, texture_path, animation_path): (ActorId, mlua::String, mlua::String) =
            lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_bot_avatar(bot_id, texture_path.to_str()?, animation_path.to_str()?);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "set_bot_emote", |api_ctx, lua, params| {
        let (bot_id, emote_id): (ActorId, String) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_bot_emote(bot_id, emote_id);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "set_bot_map_color", |api_ctx, lua, params| {
        let (bot_id, color_table): (ActorId, mlua::Table) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let color = (
            color_table.get("r")?,
            color_table.get("g")?,
            color_table.get("b")?,
            color_table.get("a").unwrap_or(255),
        );

        net.set_bot_map_color(bot_id, color);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "transfer_bot", |api_ctx, lua, params| {
        let (bot_id, area_id, warp_in_option, x_option, y_option, z_option): (
            ActorId,
            String,
            Option<bool>,
            Option<f32>,
            Option<f32>,
            Option<f32>,
        ) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();
        let warp_in = warp_in_option.unwrap_or(true);
        let x;
        let y;
        let z;

        if let Some(bot) = net.get_bot(bot_id) {
            x = x_option.unwrap_or(bot.x);
            y = y_option.unwrap_or(bot.y);
            z = z_option.unwrap_or(bot.z);
        } else {
            return Err(create_bot_error(bot_id));
        }

        net.transfer_bot(bot_id, &area_id, warp_in, x, y, z);

        lua.pack_multi(())
    });
}
