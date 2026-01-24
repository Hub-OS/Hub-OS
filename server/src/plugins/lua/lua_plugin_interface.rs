use super::api::{ApiContext, LuaApi};
use crate::jobs::JobPromiseManager;
use crate::net::{BattleStatistics, Net, WidgetTracker};
use crate::plugins::PluginInterface;
use mlua::Lua;
use packets::structures::{ActorId, BattleId, PackageId};
use std::cell::RefCell;
use std::collections::HashMap;
use std::collections::VecDeque;

pub struct LuaPluginInterface {
    scripts: Vec<Lua>,
    all_scripts: Vec<usize>,
    widget_trackers: HashMap<ActorId, WidgetTracker<usize>>,
    battle_trackers: HashMap<ActorId, VecDeque<usize>>,
    promise_manager: JobPromiseManager,
    lua_api: LuaApi,
}

impl LuaPluginInterface {
    pub fn new() -> LuaPluginInterface {
        LuaPluginInterface {
            scripts: Vec::new(),
            all_scripts: Vec::new(),
            widget_trackers: HashMap::new(),
            battle_trackers: HashMap::new(),
            promise_manager: JobPromiseManager::new(),
            lua_api: LuaApi::new(),
        }
    }

    fn load_scripts(&mut self, net_ref: &mut Net) -> std::io::Result<()> {
        use std::fs::read_dir;

        for wrapped_dir_entry in read_dir("./scripts")? {
            let dir_path = wrapped_dir_entry?.path();
            let mut script_path = dir_path;

            let extension = script_path.extension().unwrap_or_default().to_str();

            if !matches!(extension, Some("lua")) {
                script_path = script_path.join("main.lua")
            }

            if !script_path.exists() {
                continue;
            }

            if let Err(err) = self.load_script(net_ref, script_path.to_path_buf()) {
                log::error!("{err}")
            }
        }

        Ok(())
    }

    fn load_script(
        &mut self,
        net_ref: &mut Net,
        script_path: std::path::PathBuf,
    ) -> mlua::Result<()> {
        let net_ref = RefCell::new(net_ref);

        let script_index = self.scripts.len();
        // we provide access to the debug libraries
        // and allow server creators to decide which scripts are safe to use
        self.scripts.push(unsafe {
            Lua::unsafe_new_with(
                mlua::StdLib::ALL_SAFE | mlua::StdLib::DEBUG,
                Default::default(),
            )
        });
        self.all_scripts.push(script_index);

        let lua = self.scripts.last_mut().unwrap();

        let widget_tracker_ref = RefCell::new(&mut self.widget_trackers);
        let battle_tracker_ref = RefCell::new(&mut self.battle_trackers);
        let promise_manager_ref = RefCell::new(&mut self.promise_manager);

        let api_ctx = ApiContext {
            script_index,
            net_ref: &net_ref,
            widget_tracker_ref: &widget_tracker_ref,
            battle_tracker_ref: &battle_tracker_ref,
            promise_manager_ref: &promise_manager_ref,
        };

        let globals = lua.globals();

        self.lua_api.inject_static(lua)?;

        lua.load(include_str!("api/deprecated_functions.lua"))
            .set_name("internal: deprecated_functions.lua")
            .exec()?;

        self.lua_api.inject_dynamic(lua, api_ctx, |_| {
            let parent_path = script_path
                .parent()
                .unwrap_or_else(|| std::path::Path::new(""));
            let stem = script_path.file_stem().unwrap_or_default();
            let path = parent_path.join(stem);
            let path_str = path.to_str().unwrap_or_default();

            let final_path = &path_str[2..]; // chop off the ./

            // using require to load the script for better error messages (logs the path of the file)
            let require: mlua::Function = globals.get("require")?;
            require.call::<&str, ()>(final_path)?;

            Ok(())
        })?;

        lua.load(include_str!("api/deprecated_callbacks.lua"))
            .set_name("internal: deprecated_callbacks.lua")
            .exec()?;

        Ok(())
    }
}

impl PluginInterface for LuaPluginInterface {
    fn init(&mut self, net: &mut Net) {
        if let Err(err) = self.load_scripts(net) {
            log::error!("Failed to load lua scripts: {err}");
        }
    }

    fn tick(&mut self, net: &mut Net, delta_time: f32) {
        handle_event(
            &mut self.scripts,
            &self.all_scripts,
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("delta_time", delta_time)?;

                callback.call(("tick", event))
            },
        );
    }

    fn handle_authorization(&mut self, net: &mut Net, identity: &[u8], host: &str, data: &[u8]) {
        handle_event(
            &mut self.scripts,
            &self.all_scripts,
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let data_string = lua.create_string(data)?;

                let event = lua.create_table()?;
                event.set("identity", lua.create_string(identity)?)?;
                event.set("host", host)?;
                event.set("data", data_string)?;

                callback.call(("authorization", event))
            },
        );
    }

    fn handle_player_request(&mut self, net: &mut Net, player_id: ActorId, data: &str) {
        self.widget_trackers.insert(player_id, WidgetTracker::new());
        self.battle_trackers.insert(player_id, VecDeque::new());

        handle_event(
            &mut self.scripts,
            &self.all_scripts,
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;
                event.set("data", data)?;

                callback.call(("player_request", event))
            },
        );
    }

    fn handle_player_connect(&mut self, net: &mut Net, player_id: ActorId) {
        handle_event(
            &mut self.scripts,
            &self.all_scripts,
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;

                callback.call(("player_connect", event))
            },
        );
    }

    fn handle_player_join(&mut self, net: &mut Net, player_id: ActorId) {
        handle_event(
            &mut self.scripts,
            &self.all_scripts,
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;

                callback.call(("player_join", event))
            },
        );
    }

    fn handle_player_transfer(&mut self, net: &mut Net, player_id: ActorId) {
        handle_event(
            &mut self.scripts,
            &self.all_scripts,
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;

                callback.call(("player_area_transfer", event))
            },
        );
    }

    fn handle_player_disconnect(&mut self, net: &mut Net, player_id: ActorId) {
        handle_event(
            &mut self.scripts,
            &self.all_scripts,
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;

                callback.call(("player_disconnect", event))
            },
        );

        self.widget_trackers.remove(&player_id);
        self.battle_trackers.remove(&player_id);
    }

    fn handle_player_move(&mut self, net: &mut Net, player_id: ActorId, x: f32, y: f32, z: f32) {
        handle_event(
            &mut self.scripts,
            &self.all_scripts,
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;
                event.set("x", x)?;
                event.set("y", y)?;
                event.set("z", z)?;

                callback.call(("player_move", event))
            },
        );
    }

    fn handle_player_augment(&mut self, net: &mut Net, player_id: ActorId, augments: &[PackageId]) {
        handle_event(
            &mut self.scripts,
            &self.all_scripts,
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;

                let augments: Vec<_> = augments
                    .iter()
                    .flat_map(|id| -> mlua::Result<mlua::Table> {
                        let table = lua.create_table()?;
                        table.set("id", lua.create_string(id.as_str())?)?;

                        Ok(table)
                    })
                    .collect();

                event.set("augments", augments)?;

                callback.call(("player_augment", event))
            },
        );
    }

    fn handle_player_avatar_change(
        &mut self,
        net: &mut Net,
        player_id: ActorId,
        texture_path: &str,
        animation_path: &str,
    ) -> bool {
        use std::cell::Cell;
        use std::rc::Rc;

        let prevent_default = Rc::new(Cell::new(false));

        handle_event(
            &mut self.scripts,
            &self.all_scripts,
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let prevent_default_reference = prevent_default.clone();

                let event = lua.create_table()?;
                event.set("player_id", player_id)?;
                event.set("texture_path", texture_path)?;
                event.set("animation_path", animation_path)?;
                event.set(
                    "prevent_default",
                    lua.create_function(move |_, _: ()| {
                        prevent_default_reference.set(true);
                        Ok(())
                    })?,
                )?;

                callback.call(("player_avatar_change", event))
            },
        );

        prevent_default.get()
    }

    fn handle_player_emote(&mut self, net: &mut Net, player_id: ActorId, emote_id: &str) -> bool {
        use std::cell::Cell;
        use std::rc::Rc;

        let prevent_default = Rc::new(Cell::new(false));

        handle_event(
            &mut self.scripts,
            &self.all_scripts,
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let prevent_default_reference = prevent_default.clone();

                let event = lua.create_table()?;
                event.set("player_id", player_id)?;
                event.set("emote", emote_id)?;
                event.set(
                    "prevent_default",
                    lua.create_function(move |_, _: ()| {
                        prevent_default_reference.clone().set(true);
                        Ok(())
                    })?,
                )?;

                callback.call(("player_emote", event))
            },
        );

        prevent_default.get()
    }

    fn handle_custom_warp(&mut self, net: &mut Net, player_id: ActorId, tile_object_id: u32) {
        handle_event(
            &mut self.scripts,
            &self.all_scripts,
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;
                event.set("object_id", tile_object_id)?;

                callback.call(("custom_warp", event))
            },
        );
    }

    fn handle_object_interaction(
        &mut self,
        net: &mut Net,
        player_id: ActorId,
        tile_object_id: u32,
        button: u8,
    ) {
        handle_event(
            &mut self.scripts,
            &self.all_scripts,
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;
                event.set("object_id", tile_object_id)?;
                event.set("button", button)?;

                callback.call(("object_interaction", event))
            },
        );
    }

    fn handle_actor_interaction(
        &mut self,
        net: &mut Net,
        player_id: ActorId,
        actor_id: ActorId,
        button: u8,
    ) {
        handle_event(
            &mut self.scripts,
            &self.all_scripts,
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;
                event.set("actor_id", actor_id)?;
                event.set("button", button)?;

                callback.call(("actor_interaction", event))
            },
        );
    }

    fn handle_tile_interaction(
        &mut self,
        net: &mut Net,
        player_id: ActorId,
        x: f32,
        y: f32,
        z: f32,
        button: u8,
    ) {
        handle_event(
            &mut self.scripts,
            &self.all_scripts,
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;
                event.set("x", x)?;
                event.set("y", y)?;
                event.set("z", z)?;
                event.set("button", button)?;

                callback.call(("tile_interaction", event))
            },
        );
    }

    fn handle_textbox_response(&mut self, net: &mut Net, player_id: ActorId, response: u8) {
        let tracker = self.widget_trackers.get_mut(&player_id).unwrap();

        let Some(script_index) = tracker.pop_textbox() else {
            // protect against attackers
            return;
        };

        handle_event(
            &mut self.scripts,
            &[script_index],
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;
                event.set("response", response)?;

                callback.call(("textbox_response", event))
            },
        );
    }

    fn handle_prompt_response(&mut self, net: &mut Net, player_id: ActorId, response: String) {
        let tracker = self.widget_trackers.get_mut(&player_id).unwrap();

        let Some(script_index) = tracker.pop_textbox() else {
            // protect against attackers
            return;
        };

        handle_event(
            &mut self.scripts,
            &[script_index],
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;
                event.set("response", response.as_str())?;

                callback.call(("textbox_response", event))
            },
        );
    }

    fn handle_board_open(&mut self, net: &mut Net, player_id: ActorId) {
        let tracker = self.widget_trackers.get_mut(&player_id).unwrap();

        tracker.open_board();

        let Some(&script_index) = tracker.current_board() else {
            // protect against attackers
            return;
        };

        handle_event(
            &mut self.scripts,
            &[script_index],
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;

                callback.call(("board_open", event))
            },
        );
    }

    fn handle_board_close(&mut self, net: &mut Net, player_id: ActorId) {
        let tracker = self.widget_trackers.get_mut(&player_id).unwrap();

        let Some(script_index) = tracker.close_board() else {
            // protect against attackers
            return;
        };

        handle_event(
            &mut self.scripts,
            &[script_index],
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;

                callback.call(("board_close", event))
            },
        );
    }

    fn handle_post_request(&mut self, net: &mut Net, player_id: ActorId) {
        let tracker = self.widget_trackers.get_mut(&player_id).unwrap();

        let Some(&script_index) = tracker.current_board() else {
            // protect against attackers
            return;
        };

        handle_event(
            &mut self.scripts,
            &[script_index],
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;

                callback.call(("post_request", event))
            },
        );
    }

    fn handle_post_selection(&mut self, net: &mut Net, player_id: ActorId, post_id: &str) {
        let tracker = self.widget_trackers.get_mut(&player_id).unwrap();

        let Some(&script_index) = tracker.current_board() else {
            // protect against attackers
            return;
        };

        handle_event(
            &mut self.scripts,
            &[script_index],
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;
                event.set("post_id", post_id)?;

                callback.call(("post_selection", event))
            },
        );
    }

    fn handle_shop_open(&mut self, _: &mut Net, player_id: ActorId) {
        let tracker = self.widget_trackers.get_mut(&player_id).unwrap();

        tracker.open_shop();
    }

    fn handle_shop_leave(&mut self, net: &mut Net, player_id: ActorId) {
        let tracker = self.widget_trackers.get_mut(&player_id).unwrap();

        let Some(&script_index) = tracker.current_shop() else {
            // protect against attackers
            return;
        };

        handle_event(
            &mut self.scripts,
            &[script_index],
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;

                callback.call(("shop_leave", event))
            },
        );
    }

    fn handle_shop_close(&mut self, net: &mut Net, player_id: ActorId) {
        let tracker = self.widget_trackers.get_mut(&player_id).unwrap();

        let Some(script_index) = tracker.close_shop() else {
            // protect against attackers
            return;
        };

        handle_event(
            &mut self.scripts,
            &[script_index],
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;

                callback.call(("shop_close", event))
            },
        );
    }

    fn handle_shop_purchase(&mut self, net: &mut Net, player_id: ActorId, item_id: &str) {
        let tracker = self.widget_trackers.get_mut(&player_id).unwrap();

        let Some(&script_index) = tracker.current_shop() else {
            // protect against attackers
            return;
        };

        handle_event(
            &mut self.scripts,
            &[script_index],
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;
                event.set("item_id", item_id)?;

                callback.call(("shop_purchase", event))
            },
        );
    }

    fn handle_shop_description_request(
        &mut self,
        net: &mut Net,
        player_id: ActorId,
        item_id: &str,
    ) {
        let tracker = self.widget_trackers.get_mut(&player_id).unwrap();

        let Some(&script_index) = tracker.current_shop() else {
            // protect against attackers
            return;
        };

        handle_event(
            &mut self.scripts,
            &[script_index],
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;
                event.set("item_id", item_id)?;

                callback.call(("shop_description_request", event))
            },
        );
    }

    fn handle_item_use(&mut self, net: &mut Net, player_id: ActorId, item_id: &str) {
        handle_event(
            &mut self.scripts,
            &self.all_scripts,
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;
                event.set("item_id", item_id)?;

                callback.call(("item_use", event))
            },
        );
    }

    fn handle_battle_message(
        &mut self,
        net: &mut Net,
        player_id: ActorId,
        battle_id: BattleId,
        message: &str,
    ) {
        let tracker = self.battle_trackers.get_mut(&player_id).unwrap();

        let Some(script_index) = tracker.front() else {
            // protect against attackers
            return;
        };

        handle_event(
            &mut self.scripts,
            &[*script_index],
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;
                event.set("battle_id", battle_id)?;

                let chunk: Option<mlua::Value> = lua.load(message).eval().ok();
                event.set("data", chunk)?;

                callback.call(("battle_message", event))
            },
        );
    }

    fn handle_battle_results(
        &mut self,
        net: &mut Net,
        player_id: ActorId,
        battle_stats: &BattleStatistics,
    ) {
        let tracker = self.battle_trackers.get_mut(&player_id).unwrap();

        let Some(script_index) = tracker.pop_front() else {
            // protect against attackers
            return;
        };

        handle_event(
            &mut self.scripts,
            &[script_index],
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("player_id", player_id)?;
                event.set("won", battle_stats.won)?;
                event.set("health", battle_stats.health)?;
                event.set("score", battle_stats.score)?;
                event.set("time", battle_stats.time)?;
                event.set("ran", battle_stats.ran)?;
                event.set("emotion", battle_stats.emotion.as_str())?;
                event.set("turns", battle_stats.turns)?;

                callback.call(("battle_results", event))
            },
        );
    }

    fn handle_server_message(
        &mut self,
        net: &mut Net,
        socket_address: std::net::SocketAddr,
        data: &[u8],
    ) {
        let address_string = socket_address.to_string();

        handle_event(
            &mut self.scripts,
            &self.all_scripts,
            &mut self.widget_trackers,
            &mut self.battle_trackers,
            &mut self.promise_manager,
            &mut self.lua_api,
            net,
            |lua, callback| {
                let event = lua.create_table()?;
                event.set("address", address_string.as_str())?;
                event.set("data", lua.create_string(data)?)?;

                callback.call(("server_message", event))
            },
        );
    }
}

#[allow(clippy::too_many_arguments)]
fn handle_event<F>(
    scripts: &mut [Lua],
    event_listeners: &[usize],
    widget_tracker: &mut HashMap<ActorId, WidgetTracker<usize>>,
    battle_tracker: &mut HashMap<ActorId, VecDeque<usize>>,
    promise_manager: &mut JobPromiseManager,
    lua_api: &mut LuaApi,
    net: &mut Net,
    fn_caller: F,
) where
    F: for<'lua> FnMut(&'lua mlua::Lua, mlua::Function<'lua>) -> mlua::Result<()>,
{
    let mut fn_caller = fn_caller;

    let call_lua = || -> mlua::Result<()> {
        let net_ref = RefCell::new(net);
        let widget_tracker_ref = RefCell::new(widget_tracker);
        let battle_tracker_ref = RefCell::new(battle_tracker);
        let promise_manager_ref = RefCell::new(promise_manager);

        // loop over scripts
        for script_index in event_listeners {
            let lua = scripts.get_mut(*script_index).unwrap();

            let api_ctx = ApiContext {
                script_index: *script_index,
                net_ref: &net_ref,
                widget_tracker_ref: &widget_tracker_ref,
                battle_tracker_ref: &battle_tracker_ref,
                promise_manager_ref: &promise_manager_ref,
            };

            lua_api.inject_dynamic(lua, api_ctx, |lua| {
                let globals = lua.globals();
                let net_table: mlua::Table = globals.get("Net")?;

                if let Ok(func) = net_table.get::<_, mlua::Function>("emit") {
                    let binded_func = func.bind(net_table)?;

                    if let Err(err) = fn_caller(lua, binded_func) {
                        log::error!("{err}");
                    }
                }

                Ok(())
            })?;
        }
        Ok(())
    };

    if let Err(err) = call_lua() {
        log::error!("{err:#}");
    }
}
