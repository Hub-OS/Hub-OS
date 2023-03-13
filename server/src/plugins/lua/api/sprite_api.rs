use super::LuaApi;
use packets::structures::SpriteDefinition;
use packets::structures::SpriteParent;

pub fn inject_dynamic(lua_api: &mut LuaApi) {
    lua_api.add_dynamic_function("Net", "create_sprite", |api_ctx, lua_ctx, params| {
        let table: mlua::Table = lua_ctx.unpack_multi(params)?;

        let client_id_restriction = table.get("player_id")?;
        let parent_name: String = table.get("parent_id")?;
        let parent = match parent_name.as_str() {
            "widget" => SpriteParent::Widget,
            "hud" => SpriteParent::Hud,
            _ => SpriteParent::Actor(parent_name),
        };

        let sprite_definition = SpriteDefinition {
            texture_path: table.get("texture_path")?,
            animation_path: table.get("animation_path").unwrap_or_default(),
            animation_state: table.get("animation").unwrap_or_default(),
            animation_loops: true,
            parent,
            parent_point: table.get("parent_point")?,
            x: table.get("x").unwrap_or_default(),
            y: table.get("y").unwrap_or_default(),
            layer: table.get("layer").unwrap_or_default(),
        };

        let mut net = api_ctx.net_ref.borrow_mut();
        net.create_sprite(client_id_restriction, sprite_definition);

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "animate_sprite", |api_ctx, lua_ctx, params| {
        let (sprite_id, animation_state, loops): (String, String, bool) =
            lua_ctx.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();
        net.animate_sprite(sprite_id, animation_state, loops);

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "delete_sprite", |api_ctx, lua_ctx, params| {
        let sprite_id: String = lua_ctx.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();
        net.delete_sprite(sprite_id);

        lua_ctx.pack_multi(())
    });
}
