use super::LuaApi;
use crate::net::{Asset, AssetData};
use packets::structures::ActorId;

pub fn inject_dynamic(lua_api: &mut LuaApi) {
    lua_api.add_dynamic_function("Net", "update_asset", |api_ctx, lua, params| {
        let (path, data): (String, mlua::String) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let path_buf = std::path::PathBuf::from(path.to_string());
        let asset = Asset::load_from_memory(&path_buf, data.as_bytes().to_vec());

        net.set_asset(path, asset);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "remove_asset", |api_ctx, lua, params| {
        let path: mlua::String = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.remove_asset(path.to_str()?);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "has_asset", |api_ctx, lua, params| {
        let path: mlua::String = lua.unpack_multi(params)?;
        let path_str = path.to_str()?;
        let net = api_ctx.net_ref.borrow();

        let has_asset = net.get_asset(path_str).is_some();

        lua.pack_multi(has_asset)
    });

    lua_api.add_dynamic_function("Net", "get_asset_type", |api_ctx, lua, params| {
        let path: mlua::String = lua.unpack_multi(params)?;
        let path_str = path.to_str()?;
        let net = api_ctx.net_ref.borrow();

        let asset_type = net.get_asset(path_str).map(|asset| match asset.data {
            AssetData::Text(_) => Some("text"),
            AssetData::CompressedText(_) => Some("text"),
            AssetData::Texture(_) => Some("texture"),
            AssetData::Audio(_) => Some("audio"),
            AssetData::Data(_) => Some("data"),
        });

        lua.pack_multi(asset_type)
    });

    lua_api.add_dynamic_function("Net", "get_asset_size", |api_ctx, lua, params| {
        let path: mlua::String = lua.unpack_multi(params)?;
        let path_str = path.to_str()?;
        let net = api_ctx.net_ref.borrow();

        if let Some(asset) = net.get_asset(path_str) {
            lua.pack_multi(asset.len())
        } else {
            lua.pack_multi(0)
        }
    });

    lua_api.add_dynamic_function("Net", "provide_asset_for_player", |api_ctx, lua, params| {
        let (player_id, asset_path): (ActorId, mlua::String) = lua.unpack_multi(params)?;
        let asset_path_str = asset_path.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.preload_asset_for_player(player_id, asset_path_str);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(
        "Net",
        "provide_package_for_player",
        |api_ctx, lua, params| {
            let (player_id, asset_path): (ActorId, mlua::String) = lua.unpack_multi(params)?;
            let asset_path_str = asset_path.to_str()?;

            let mut net = api_ctx.net_ref.borrow_mut();

            net.preload_package(&[player_id], asset_path_str);

            lua.pack_multi(())
        },
    );
}
