use super::{BattleStatistics, Net};
use crate::plugins::PluginInterface;
use packets::structures::{ActorId, BattleId, PackageId};

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

    fn handle_player_request(&mut self, net: &mut Net, player_id: ActorId, data: &str) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_player_request(net, player_id, data)
        });
    }

    fn handle_player_connect(&mut self, net: &mut Net, player_id: ActorId) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_player_connect(net, player_id)
        });
    }

    fn handle_player_join(&mut self, net: &mut Net, player_id: ActorId) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_player_join(net, player_id)
        });
    }

    fn handle_player_transfer(&mut self, net: &mut Net, player_id: ActorId) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_player_transfer(net, player_id)
        });
    }

    fn handle_player_disconnect(&mut self, net: &mut Net, player_id: ActorId) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_player_disconnect(net, player_id)
        });
    }

    fn handle_player_move(&mut self, net: &mut Net, player_id: ActorId, x: f32, y: f32, z: f32) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_player_move(net, player_id, x, y, z)
        });
    }

    fn handle_player_augment(&mut self, net: &mut Net, player_id: ActorId, augments: &[PackageId]) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_player_augment(net, player_id, augments)
        });
    }

    fn handle_player_avatar_change(
        &mut self,
        net: &mut Net,
        player_id: ActorId,
        texture_path: &str,
        animation_path: &str,
    ) -> bool {
        let mut prevent_default = false;

        self.wrap_calls(net, |plugin_interface, net| {
            prevent_default |= plugin_interface.handle_player_avatar_change(
                net,
                player_id,
                texture_path,
                animation_path,
            )
        });

        prevent_default
    }

    fn handle_player_emote(&mut self, net: &mut Net, player_id: ActorId, emote_id: &str) -> bool {
        let mut prevent_default = false;

        self.wrap_calls(net, |plugin_interface, net| {
            prevent_default |= plugin_interface.handle_player_emote(net, player_id, emote_id)
        });

        prevent_default
    }

    fn handle_custom_warp(&mut self, net: &mut Net, player_id: ActorId, tile_object_id: u32) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_custom_warp(net, player_id, tile_object_id)
        });
    }

    fn handle_object_interaction(
        &mut self,
        net: &mut Net,
        player_id: ActorId,
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
        player_id: ActorId,
        actor_id: ActorId,
        button: u8,
    ) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_actor_interaction(net, player_id, actor_id, button)
        });
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
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_tile_interaction(net, player_id, x, y, z, button)
        });
    }

    fn handle_textbox_response(&mut self, net: &mut Net, player_id: ActorId, response: u8) {
        if let Some(client) = net.get_client_mut(player_id)
            && let Some(i) = client.widget_tracker.pop_textbox()
        {
            self.wrap_call(i, net, |plugin_interface, net| {
                plugin_interface.handle_textbox_response(net, player_id, response)
            });
        }
    }

    fn handle_prompt_response(&mut self, net: &mut Net, player_id: ActorId, response: String) {
        if let Some(client) = net.get_client_mut(player_id)
            && let Some(i) = client.widget_tracker.pop_textbox()
        {
            self.wrap_call(i, net, |plugin_interface, net| {
                plugin_interface.handle_prompt_response(net, player_id, response.clone())
            });
        }
    }

    fn handle_board_open(&mut self, net: &mut Net, player_id: ActorId) {
        if let Some(client) = net.get_client_mut(player_id) {
            client.widget_tracker.open_board();

            if let Some(i) = client.widget_tracker.current_board() {
                self.wrap_call(*i, net, |plugin_interface, net| {
                    plugin_interface.handle_board_open(net, player_id)
                });
            }
        }
    }

    fn handle_board_close(&mut self, net: &mut Net, player_id: ActorId) {
        if let Some(client) = net.get_client_mut(player_id)
            && let Some(i) = client.widget_tracker.close_board()
        {
            self.wrap_call(i, net, |plugin_interface, net| {
                plugin_interface.handle_board_close(net, player_id)
            });
        }
    }

    fn handle_post_request(&mut self, net: &mut Net, player_id: ActorId) {
        if let Some(client) = net.get_client_mut(player_id)
            && let Some(i) = client.widget_tracker.current_board()
        {
            self.wrap_call(*i, net, |plugin_interface, net| {
                plugin_interface.handle_post_request(net, player_id)
            });
        }
    }

    fn handle_post_selection(&mut self, net: &mut Net, player_id: ActorId, post_id: &str) {
        if let Some(client) = net.get_client_mut(player_id)
            && let Some(i) = client.widget_tracker.current_board()
        {
            self.wrap_call(*i, net, |plugin_interface, net| {
                plugin_interface.handle_post_selection(net, player_id, post_id)
            });
        }
    }

    fn handle_shop_open(&mut self, net: &mut Net, player_id: ActorId) {
        if let Some(client) = net.get_client_mut(player_id) {
            client.widget_tracker.open_shop();

            if let Some(i) = client.widget_tracker.current_shop() {
                self.wrap_call(*i, net, |plugin_interface, net| {
                    plugin_interface.handle_shop_open(net, player_id)
                });
            }
        }
    }

    fn handle_shop_leave(&mut self, net: &mut Net, player_id: ActorId) {
        if let Some(client) = net.get_client_mut(player_id) {
            client.widget_tracker.open_shop();

            if let Some(i) = client.widget_tracker.current_shop() {
                self.wrap_call(*i, net, |plugin_interface, net| {
                    plugin_interface.handle_shop_leave(net, player_id)
                });
            }
        }
    }

    fn handle_shop_close(&mut self, net: &mut Net, player_id: ActorId) {
        if let Some(client) = net.get_client_mut(player_id)
            && let Some(i) = client.widget_tracker.close_shop()
        {
            self.wrap_call(i, net, |plugin_interface, net| {
                plugin_interface.handle_shop_close(net, player_id);
            });
        }
    }

    fn handle_shop_purchase(&mut self, net: &mut Net, player_id: ActorId, item_id: &str) {
        if let Some(client) = net.get_client_mut(player_id)
            && let Some(i) = client.widget_tracker.current_shop()
        {
            self.wrap_call(*i, net, |plugin_interface, net| {
                plugin_interface.handle_shop_purchase(net, player_id, item_id)
            });
        }
    }

    fn handle_shop_description_request(
        &mut self,
        net: &mut Net,
        player_id: ActorId,
        item_id: &str,
    ) {
        if let Some(client) = net.get_client_mut(player_id)
            && let Some(i) = client.widget_tracker.current_shop()
        {
            self.wrap_call(*i, net, |plugin_interface, net| {
                plugin_interface.handle_shop_description_request(net, player_id, item_id)
            });
        }
    }

    fn handle_item_use(&mut self, net: &mut Net, player_id: ActorId, item_id: &str) {
        self.wrap_calls(net, |plugin_interface, net| {
            plugin_interface.handle_item_use(net, player_id, item_id)
        });
    }

    fn handle_battle_message(
        &mut self,
        net: &mut Net,
        player_id: ActorId,
        _: BattleId,
        message: &str,
    ) {
        if let Some(client) = net.get_client_mut(player_id)
            && let Some(info) = client.battle_tracker.front()
        {
            let i = info.plugin_index;
            let battle_id = info.battle_id;

            self.wrap_call(i, net, |plugin_interface, net| {
                plugin_interface.handle_battle_message(net, player_id, battle_id, message)
            });
        }
    }

    fn handle_battle_results(
        &mut self,
        net: &mut Net,
        player_id: ActorId,
        battle_stats: &BattleStatistics,
    ) {
        if let Some(client) = net.get_client_mut(player_id)
            && let Some(info) = client.battle_tracker.pop_front()
        {
            net.end_battle_for_player(info.battle_id, player_id);

            let i = info.plugin_index;

            self.wrap_call(i, net, |plugin_interface, net| {
                plugin_interface.handle_battle_results(net, player_id, battle_stats)
            });
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
