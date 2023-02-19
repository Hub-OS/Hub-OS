use super::{BattleStatistics, Net};
use crate::plugins::PluginInterface;

pub(super) struct PluginWrapper {
    plugin_interfaces: Vec<Box<dyn PluginInterface>>,
}

impl PluginWrapper {
    pub(super) fn new() -> PluginWrapper {
        PluginWrapper {
            plugin_interfaces: Vec::new(),
        }
    }

    pub(super) fn add_plugin_interface(&mut self, plugin_interface: Box<dyn PluginInterface>) {
        self.plugin_interfaces.push(plugin_interface);
    }

    fn wrap_call<C>(&mut self, i: usize, net: &mut Net, call: C)
    where
        C: FnMut(&mut Box<dyn PluginInterface>, &mut Net),
    {
        let mut call = call;

        net.set_active_plugin(i);
        call(&mut self.plugin_interfaces[i], net);
    }

    fn wrap_calls<C>(&mut self, net: &mut Net, call: C)
    where
        C: FnMut(&mut Box<dyn PluginInterface>, &mut Net),
    {
        let mut call = call;

        for (i, plugin_interface) in self.plugin_interfaces.iter_mut().enumerate() {
            net.set_active_plugin(i);
            call(plugin_interface, net);
        }
    }
}

impl PluginInterface for PluginWrapper {
    fn init(&mut self, net: &mut Net) {
        self.wrap_calls(net, |plugin_interface, net| plugin_interface.init(net));
    }

    fn tick(&mut self, net: &mut Net, delta_time: f32) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.tick(net, delta_time)
        });
    }

    fn handle_authorization(&mut self, net: &mut Net, identity: &[u8], host: &str, data: &[u8]) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_authorization(net, identity, host, data);
        });
    }

    fn handle_player_request(&mut self, net: &mut Net, player_id: &str, data: &str) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_player_request(net, player_id, data)
        });
    }

    fn handle_player_connect(&mut self, net: &mut Net, player_id: &str) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_player_connect(net, player_id)
        });
    }

    fn handle_player_join(&mut self, net: &mut Net, player_id: &str) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_player_join(net, player_id)
        });
    }

    fn handle_player_transfer(&mut self, net: &mut Net, player_id: &str) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_player_transfer(net, player_id)
        });
    }

    fn handle_player_disconnect(&mut self, net: &mut Net, player_id: &str) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_player_disconnect(net, player_id)
        });
    }

    fn handle_player_move(&mut self, net: &mut Net, player_id: &str, x: f32, y: f32, z: f32) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_player_move(net, player_id, x, y, z)
        });
    }

    fn handle_player_avatar_change(
        &mut self,
        net: &mut Net,
        player_id: &str,
        texture_path: &str,
        animation_path: &str,
        name: &str,
        element: &str,
        max_health: u32,
    ) -> bool {
        let mut prevent_default = false;

        self.wrap_calls(net, |plugin_interface, net| {
            prevent_default |= plugin_interface.handle_player_avatar_change(
                net,
                player_id,
                texture_path,
                animation_path,
                name,
                element,
                max_health,
            )
        });

        prevent_default
    }

    fn handle_player_emote(&mut self, net: &mut Net, player_id: &str, emote_id: u8) -> bool {
        let mut prevent_default = false;

        self.wrap_calls(net, |plugin_interface, net| {
            prevent_default |= plugin_interface.handle_player_emote(net, player_id, emote_id)
        });

        prevent_default
    }

    fn handle_custom_warp(&mut self, net: &mut Net, player_id: &str, tile_object_id: u32) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_custom_warp(net, player_id, tile_object_id)
        });
    }

    fn handle_object_interaction(
        &mut self,
        net: &mut Net,
        player_id: &str,
        tile_object_id: u32,
        button: u8,
    ) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_object_interaction(net, player_id, tile_object_id, button)
        });
    }

    fn handle_actor_interaction(
        &mut self,
        net: &mut Net,
        player_id: &str,
        actor_id: &str,
        button: u8,
    ) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_actor_interaction(net, player_id, actor_id, button)
        });
    }

    fn handle_tile_interaction(
        &mut self,
        net: &mut Net,
        player_id: &str,
        x: f32,
        y: f32,
        z: f32,
        button: u8,
    ) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_tile_interaction(net, player_id, x, y, z, button)
        });
    }

    fn handle_textbox_response(&mut self, net: &mut Net, player_id: &str, response: u8) {
        if let Some(client) = net.get_client_mut(player_id) {
            if let Some(i) = client.widget_tracker.pop_textbox() {
                self.wrap_call(i, net, |plugin_interface, net| {
                    plugin_interface.handle_textbox_response(net, player_id, response)
                });
            }
        }
    }

    fn handle_prompt_response(&mut self, net: &mut Net, player_id: &str, response: String) {
        if let Some(client) = net.get_client_mut(player_id) {
            if let Some(i) = client.widget_tracker.pop_textbox() {
                self.wrap_call(i, net, |plugin_interface, net| {
                    plugin_interface.handle_prompt_response(net, player_id, response.clone())
                });
            }
        }
    }

    fn handle_board_open(&mut self, net: &mut Net, player_id: &str) {
        if let Some(client) = net.get_client_mut(player_id) {
            client.widget_tracker.open_board();

            if let Some(i) = client.widget_tracker.current_board() {
                self.wrap_call(*i, net, |plugin_interface, net| {
                    plugin_interface.handle_board_open(net, player_id)
                });
            }
        }
    }

    fn handle_board_close(&mut self, net: &mut Net, player_id: &str) {
        if let Some(client) = net.get_client_mut(player_id) {
            if let Some(i) = client.widget_tracker.close_board() {
                self.wrap_call(i, net, |plugin_interface, net| {
                    plugin_interface.handle_board_close(net, player_id)
                });
            }
        }
    }

    fn handle_post_request(&mut self, net: &mut Net, player_id: &str) {
        if let Some(client) = net.get_client_mut(player_id) {
            if let Some(i) = client.widget_tracker.current_board() {
                self.wrap_call(*i, net, |plugin_interface, net| {
                    plugin_interface.handle_post_request(net, player_id)
                });
            }
        }
    }

    fn handle_post_selection(&mut self, net: &mut Net, player_id: &str, post_id: &str) {
        if let Some(client) = net.get_client_mut(player_id) {
            if let Some(i) = client.widget_tracker.current_board() {
                self.wrap_call(*i, net, |plugin_interface, net| {
                    plugin_interface.handle_post_selection(net, player_id, post_id)
                });
            }
        }
    }

    fn handle_shop_close(&mut self, net: &mut Net, player_id: &str) {
        if let Some(client) = net.get_client_mut(player_id) {
            if let Some(i) = client.widget_tracker.close_shop() {
                self.wrap_call(i, net, |plugin_interface, net| {
                    plugin_interface.handle_shop_close(net, player_id);
                });
            }
        }
    }

    fn handle_shop_purchase(&mut self, net: &mut Net, player_id: &str, item_name: &str) {
        if let Some(client) = net.get_client_mut(player_id) {
            if let Some(i) = client.widget_tracker.current_shop() {
                self.wrap_call(*i, net, |plugin_interface, net| {
                    plugin_interface.handle_shop_purchase(net, player_id, item_name)
                });
            }
        }
    }

    fn handle_battle_results(
        &mut self,
        net: &mut Net,
        player_id: &str,
        battle_stats: &BattleStatistics,
    ) {
        if let Some(client) = net.get_client_mut(player_id) {
            if let Some(info) = client.battle_tracker.pop_front() {
                let i = info.plugin_index;

                self.wrap_call(i, net, |plugin_interface, net| {
                    plugin_interface.handle_battle_results(net, player_id, battle_stats)
                });
            }
        }
    }

    fn handle_server_message(
        &mut self,
        net: &mut Net,
        socket_address: std::net::SocketAddr,
        data: &[u8],
    ) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_server_message(net, socket_address, data)
        });
    }
}
