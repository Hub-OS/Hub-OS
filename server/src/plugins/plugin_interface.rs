use crate::net::{BattleStatistics, Net};
use packets::structures::PackageId;

pub trait PluginInterface {
    fn init(&mut self, net: &mut Net);
    fn tick(&mut self, net: &mut Net, delta_time: f32);
    fn handle_authorization(&mut self, net: &mut Net, identity: &[u8], host: &str, data: &[u8]);
    fn handle_player_request(&mut self, net: &mut Net, player_id: &str, data: &str);
    fn handle_player_connect(&mut self, net: &mut Net, player_id: &str);
    fn handle_player_join(&mut self, net: &mut Net, player_id: &str);
    fn handle_player_transfer(&mut self, net: &mut Net, player_id: &str);
    fn handle_player_disconnect(&mut self, net: &mut Net, player_id: &str);
    fn handle_player_move(&mut self, net: &mut Net, player_id: &str, x: f32, y: f32, z: f32);
    fn handle_player_boost(&mut self, net: &mut Net, player_id: &str, blocks: &[PackageId]);
    fn handle_player_avatar_change(
        &mut self,
        net: &mut Net,
        player_id: &str,
        texture_path: &str,
        animation_path: &str,
    ) -> bool;
    fn handle_player_emote(&mut self, net: &mut Net, player_id: &str, emote_id: u8) -> bool;
    fn handle_custom_warp(&mut self, net: &mut Net, player_id: &str, tile_object_id: u32);
    fn handle_object_interaction(
        &mut self,
        net: &mut Net,
        player_id: &str,
        tile_object_id: u32,
        button: u8,
    );
    fn handle_actor_interaction(
        &mut self,
        net: &mut Net,
        player_id: &str,
        actor_id: &str,
        button: u8,
    );
    fn handle_tile_interaction(
        &mut self,
        net: &mut Net,
        player_id: &str,
        x: f32,
        y: f32,
        z: f32,
        button: u8,
    );
    fn handle_textbox_response(&mut self, net: &mut Net, player_id: &str, response: u8);
    fn handle_prompt_response(&mut self, net: &mut Net, player_id: &str, response: String);
    fn handle_board_open(&mut self, net: &mut Net, player_id: &str);
    fn handle_board_close(&mut self, net: &mut Net, player_id: &str);
    fn handle_post_request(&mut self, net: &mut Net, player_id: &str);
    fn handle_post_selection(&mut self, net: &mut Net, player_id: &str, post_id: &str);
    fn handle_shop_close(&mut self, net: &mut Net, player_id: &str);
    fn handle_shop_purchase(&mut self, net: &mut Net, player_id: &str, post_id: &str);
    fn handle_battle_results(
        &mut self,
        net: &mut Net,
        player_id: &str,
        battle_stats: &BattleStatistics,
    );
    fn handle_server_message(
        &mut self,
        net: &mut Net,
        socket_address: std::net::SocketAddr,
        data: &[u8],
    );
}
