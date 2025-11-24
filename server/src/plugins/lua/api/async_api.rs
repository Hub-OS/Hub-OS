use super::LuaApi;
use crate::jobs::{JobPromise, JobPromiseManager, PromiseValue};
use std::cell::RefCell;

pub fn inject_static(lua_api: &mut LuaApi) {
    lua_api.add_global_table("Async");

    lua_api.add_static_injector(|lua| {
        lua.load(include_str!("async_api.lua"))
            .set_name("internal: async_api.lua")
            .exec()?;

        Ok(())
    });
}

pub fn inject_dynamic(lua_api: &mut LuaApi) {
    lua_api.add_dynamic_function(
        "Async",
        "_is_promise_pending",
        move |api_ctx, lua, params| {
            let id: usize = lua.unpack_multi(params)?;
            let promise_manager = api_ctx.promise_manager_ref.borrow();

            let mut pending = true;

            if let Some(promise) = promise_manager.get_promise(id) {
                pending = promise.is_pending();
            }

            lua.pack_multi(pending)
        },
    );

    lua_api.add_dynamic_function("Async", "_get_promise_value", |api_ctx, lua, params| {
        let id: usize = lua.unpack_multi(params)?;
        let mut promise_manager = api_ctx.promise_manager_ref.borrow_mut();

        let mut value = None;

        if let Some(promise) = promise_manager.get_promise_mut(id)
            && let Some(promise_value) = promise.get_value()
        {
            value = match promise_value {
                PromiseValue::HttpResponse(response_data) => {
                    let table = lua.create_table()?;

                    table.set("status", response_data.status)?;

                    let headers_table = lua.create_table()?;

                    for (key, value) in &response_data.headers {
                        headers_table.set(key.as_str(), value.as_str())?;
                    }

                    table.set("headers", headers_table)?;

                    let body = lua.create_string(&response_data.body)?;
                    table.set("body", body)?;

                    Some(mlua::Value::Table(table))
                }
                PromiseValue::Bytes(bytes) => {
                    let lua_string = lua.create_string(&bytes)?;

                    Some(mlua::Value::String(lua_string))
                }
                PromiseValue::Success(success) => Some(mlua::Value::Boolean(success)),
                PromiseValue::ServerPolled {} => {
                    let table = lua.create_table()?;

                    Some(mlua::Value::Table(table))
                }
                PromiseValue::None => None,
            }
        }

        promise_manager.remove_promise(id);

        lua.pack_multi(value)
    });

    lua_api.add_dynamic_function("Async", "request", |api_ctx, lua, params| {
        use crate::jobs::web_request::web_request;

        let (url, options): (String, Option<mlua::Table>) = lua.unpack_multi(params)?;

        let method: String;
        let body: Option<Vec<u8>>;
        let headers: Vec<(String, String)>;

        if let Some(options) = options {
            method = options
                .get("method")
                .ok()
                .unwrap_or_else(|| String::from("GET"));

            body = options
                .get("body")
                .ok()
                .map(|lua_string: mlua::String| lua_string.as_bytes().to_vec());

            headers = options
                .get("headers")
                .ok()
                .map(|table: mlua::Table| {
                    table
                        .pairs()
                        .filter_map(|result| {
                            let (key, value): (String, String) = result.ok()?;
                            Some((key, value))
                        })
                        .collect()
                })
                .unwrap_or_default();
        } else {
            method = String::from("GET");
            body = None;
            headers = Vec::new();
        }

        let promise = web_request(url, method, headers, body);

        let lua_promise = create_lua_promise(lua, api_ctx.promise_manager_ref, promise);
        lua.pack_multi(lua_promise)
    });

    lua_api.add_dynamic_function("Async", "download", |api_ctx, lua, params| {
        use crate::jobs::web_download::web_download;

        let (path, url, options): (String, String, Option<mlua::Table>) =
            lua.unpack_multi(params)?;

        let method: String;
        let body: Option<Vec<u8>>;
        let headers: Vec<(String, String)>;

        if let Some(options) = options {
            method = options
                .get("method")
                .ok()
                .unwrap_or_else(|| String::from("GET"));

            body = options
                .get("body")
                .ok()
                .map(|lua_string: mlua::String| lua_string.as_bytes().to_vec());

            headers = options
                .get("headers")
                .ok()
                .map(|table: mlua::Table| {
                    table
                        .pairs()
                        .filter_map(|result| {
                            let (key, value): (String, String) = result.ok()?;
                            Some((key, value))
                        })
                        .collect()
                })
                .unwrap_or_default();
        } else {
            method = String::from("GET");
            body = None;
            headers = Vec::new();
        }

        let promise = web_download(path, url, method, headers, body);

        let lua_promise = create_lua_promise(lua, api_ctx.promise_manager_ref, promise);

        lua.pack_multi(lua_promise)
    });

    lua_api.add_dynamic_function("Async", "read_file", |api_ctx, lua, params| {
        use crate::jobs::files::read_file;

        let path: String = lua.unpack_multi(params)?;

        let promise = read_file(path);

        let lua_promise = create_lua_promise(lua, api_ctx.promise_manager_ref, promise);

        lua.pack_multi(lua_promise)
    });

    lua_api.add_dynamic_function("Async", "write_file", |api_ctx, lua, params| {
        let (path, content): (String, mlua::String) = lua.unpack_multi(params)?;

        use crate::jobs::files::write_file;

        let promise = write_file(path, content.as_bytes());

        let lua_promise = create_lua_promise(lua, api_ctx.promise_manager_ref, promise);

        lua.pack_multi(lua_promise)
    });

    lua_api.add_dynamic_function("Async", "ensure_dir", |api_ctx, lua, params| {
        let path: String = lua.unpack_multi(params)?;

        use crate::jobs::files::ensure_folder;

        let promise = ensure_folder(path);

        let lua_promise = create_lua_promise(lua, api_ctx.promise_manager_ref, promise);

        lua.pack_multi(lua_promise)
    });

    lua_api.add_dynamic_function("Async", "poll_server", |api_ctx, lua, params| {
        let address: String = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let promise = net.poll_server(address);

        let lua_promise = create_lua_promise(lua, api_ctx.promise_manager_ref, promise);

        lua.pack_multi(lua_promise)
    });

    lua_api.add_dynamic_function("Async", "message_server", |api_ctx, lua, params| {
        let (address, data): (String, mlua::String) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.message_server(address, data.as_bytes().to_vec());

        lua.pack_multi(())
    });
}

fn create_lua_promise<'a>(
    lua: &'a mlua::Lua,
    promise_manager_ref: &RefCell<&mut JobPromiseManager>,
    promise: JobPromise,
) -> mlua::Result<mlua::Table<'a>> {
    let mut promise_manager = promise_manager_ref.borrow_mut();
    let id = promise_manager.add_promise(promise);

    let async_api: mlua::Table = lua.globals().get("Async")?;
    let create_promise: mlua::Function = async_api.get("_promise_from_id")?;
    let lua_promise: mlua::Table = create_promise.call(id)?;

    Ok(lua_promise)
}
