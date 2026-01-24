use super::LuaApi;
use crate::plugins::lua::api::widget_api::parse_text_style;
use packets::structures::{
    SpriteAttachment, SpriteDefinition, SpriteId, SpriteParent, TextAxisAlignment,
};

pub fn inject_dynamic(lua_api: &mut LuaApi) {
    lua_api.add_dynamic_function("Net", "create_sprite", |api_ctx, lua, params| {
        let table: mlua::Table = lua.unpack_multi(params)?;

        let client_id_restriction = table.get("player_id")?;
        let parent: mlua::Value = table.get("parent_id")?;
        let parent = match parent.as_str() {
            Some("widget") => SpriteParent::Widget,
            Some("hud") => SpriteParent::Hud,
            _ => SpriteParent::Actor(table.get("parent_id")?),
        };

        let attachment = SpriteAttachment {
            parent,
            parent_point: table.get("parent_point")?,
            x: table.get("x").unwrap_or_default(),
            y: table.get("y").unwrap_or_default(),
            layer: table.get("layer").unwrap_or_default(),
        };

        let definition = SpriteDefinition::Sprite {
            texture_path: table.get("texture_path")?,
            animation_path: table.get("animation_path").unwrap_or_default(),
            animation_state: table.get("animation").unwrap_or_default(),
            animation_loops: table.get("loop_animation").unwrap_or(false),
        };

        let mut net = api_ctx.net_ref.borrow_mut();
        let sprite_id = net.create_sprite(client_id_restriction, attachment, definition);

        lua.pack_multi(sprite_id)
    });

    lua_api.add_dynamic_function("Net", "create_text_sprite", |api_ctx, lua, params| {
        let table: mlua::Table = lua.unpack_multi(params)?;

        let client_id_restriction = table.get("player_id")?;
        let parent: mlua::Value = table.get("parent_id")?;
        let parent = match parent.as_str() {
            Some("widget") => SpriteParent::Widget,
            Some("hud") => SpriteParent::Hud,
            _ => SpriteParent::Actor(table.get("parent_id")?),
        };

        let attachment = SpriteAttachment {
            parent,
            parent_point: table.get("parent_point")?,
            x: table.get("x").unwrap_or_default(),
            y: table.get("y").unwrap_or_default(),
            layer: table.get("layer").unwrap_or_default(),
        };

        let mut h_align_string = table
            .get::<_, Option<String>>("h_align")?
            .unwrap_or_default();
        h_align_string.make_ascii_lowercase();

        let mut v_align_string = table
            .get::<_, Option<String>>("v_align")?
            .unwrap_or_default();
        v_align_string.make_ascii_lowercase();

        let definition = SpriteDefinition::Text {
            text: table.get("text")?,
            text_style: parse_text_style(table.get("text_style")?)?,
            h_align: match h_align_string.as_str() {
                "center" => TextAxisAlignment::Middle,
                "right" => TextAxisAlignment::End,
                _ => TextAxisAlignment::Start,
            },
            v_align: match v_align_string.as_str() {
                "center" => TextAxisAlignment::Middle,
                "bottom" => TextAxisAlignment::End,
                _ => TextAxisAlignment::Start,
            },
        };

        let mut net = api_ctx.net_ref.borrow_mut();
        let sprite_id = net.create_sprite(client_id_restriction, attachment, definition);

        lua.pack_multi(sprite_id)
    });

    lua_api.add_dynamic_function("Net", "animate_sprite", |api_ctx, lua, params| {
        let (sprite_id, animation_state, loops): (SpriteId, String, bool) =
            lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();
        net.animate_sprite(sprite_id, animation_state, loops);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "delete_sprite", |api_ctx, lua, params| {
        let sprite_id: SpriteId = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();
        net.delete_sprite(sprite_id);

        lua.pack_multi(())
    });
}
