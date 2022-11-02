use super::{BattleLuaApi, GLOBAL_TABLE};
use crate::resources::{AssetManager, ResourcePaths};

pub fn inject_include_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(GLOBAL_TABLE, "include", |api_ctx, lua, params| {
        let path: String = lua.unpack_multi(params)?;

        let env = lua.environment()?;
        let folder_path: String = env.get("_folder_path")?;

        let path = folder_path + &path;

        let source = {
            let api_ctx = api_ctx.borrow();
            api_ctx.game_io.globals().assets.text(&path)
        };

        let metatable = lua.globals().get_metatable();

        let env = lua.create_table()?;
        env.set_metatable(metatable);
        env.set("_folder_path", ResourcePaths::parent(&path))?;

        lua.load(&source)
            .set_name(ResourcePaths::shorten(&path))?
            .set_environment(env)?
            .call(())
    });
}
