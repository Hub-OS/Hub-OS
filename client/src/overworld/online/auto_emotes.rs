use crate::{
    overworld::OverworldArea,
    render::{Animator, FrameTime},
    resources::ClientPacketSender,
};
use framework::{common::GameIO, math::Vec3};
use packets::{ClientPacket, Reliability};

const MIN_AFK_TIME: FrameTime = 60 * 60;

#[derive(PartialEq, Eq)]
pub enum AutoEmote {
    None,
    Battle,
    Menu,
    Afk,
}

pub struct AutoEmotes {
    time: FrameTime,
    last_position: Vec3,
    last_move_time: FrameTime,
    set_last_emote: bool,
    auto_emote: AutoEmote,
    afk_state: Option<String>,
    busy_state: Option<String>,
}

impl AutoEmotes {
    pub fn new(emote_animator: &Animator) -> Self {
        let mut s = Self {
            time: 0,
            last_position: Vec3::ZERO,
            last_move_time: 0,
            set_last_emote: false,
            auto_emote: AutoEmote::None,
            afk_state: None,
            busy_state: None,
        };

        s.read_animator(emote_animator);

        s
    }

    pub fn read_animator(&mut self, emote_animator: &Animator) {
        self.afk_state = emote_animator
            .iter_states()
            .find(|(state, _)| state.contains("AFK"))
            .map(|(state, _)| state.to_string());

        self.busy_state = emote_animator
            .iter_states()
            .find(|(state, _)| state.contains("BUSY") && state.contains("MENUS"))
            .map(|(state, _)| state.to_string());
    }

    pub fn set_auto_emote(&mut self, emote: AutoEmote) {
        self.auto_emote = emote;
    }

    pub fn update(
        &mut self,
        game_io: &GameIO,
        area: &mut OverworldArea,
        send: &ClientPacketSender,
    ) {
        self.time += 1;

        // afk detection
        let input = game_io.input();

        let has_input = input.latest_key().is_some()
            || input.latest_button().is_some()
            || !input.touches().is_empty();

        if has_input {
            self.last_move_time = self.time;
        } else if let Ok(&player_position) = area
            .entities
            .query_one_mut::<&Vec3>(area.player_data.entity)
        {
            if self.last_position != player_position {
                self.last_move_time = self.time;
                self.last_position = player_position;
            }
        }

        // set emote with a packet once a second
        if self.time % 60 != 0 {
            return;
        }

        // update afk status
        let is_afk = area.world_time - self.last_move_time > MIN_AFK_TIME;

        match self.auto_emote {
            AutoEmote::None => {
                if is_afk {
                    self.auto_emote = AutoEmote::Afk;
                }
            }
            AutoEmote::Afk => {
                if !is_afk {
                    self.auto_emote = AutoEmote::None
                }
            }
            _ => self.last_move_time = self.time,
        }

        // resolve which emote to send, if any
        let emote_id = match self.auto_emote {
            AutoEmote::None | AutoEmote::Battle => {
                if self.set_last_emote {
                    // clear emote
                    self.set_last_emote = false;
                    Some(String::default())
                } else {
                    None
                }
            }
            AutoEmote::Menu => self.busy_state.clone(),
            AutoEmote::Afk => self.afk_state.clone(),
        };

        if let Some(emote_id) = emote_id {
            self.set_last_emote = !emote_id.is_empty();
            send(Reliability::Reliable, ClientPacket::Emote { emote_id });
        }
    }
}
