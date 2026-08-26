use crate::battle::PlayerSetup;
use crate::resources::{ClientPacketSender, NetplayPacketReceiver, NetplayPacketSender};
use packets::{NetplayPacket, NetplayPacketData, NetplaySignal, structures::BattleId};
use structures::collections::VecSet;

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum ConnectionState {
    Connected,
    Disconnected,
}

#[derive(Default)]
pub struct ConnectionStates {
    states: Vec<ConnectionState>,
    connected_count: usize,
}

impl ConnectionStates {
    fn load_setups(&mut self, setups: &[PlayerSetup]) {
        debug_assert!(
            setups.array_windows::<2>().all(|[a, b]| a.index < b.index),
            "setups should be sorted"
        );

        self.states.clear();
        self.states.extend(setups.iter().map(|setup| {
            if setup.connected {
                ConnectionState::Connected
            } else {
                ConnectionState::Disconnected
            }
        }));

        let state_iter = self.states.iter();
        self.connected_count = state_iter
            .filter(|&&s| s != ConnectionState::Disconnected)
            .count();
    }

    pub fn connected_count(&self) -> usize {
        self.connected_count
    }

    pub fn total_states(&self) -> usize {
        self.states.len()
    }

    pub fn count(&self, f: impl Fn(ConnectionState) -> bool) -> usize {
        self.states.iter().filter(|&&state| f(state)).count()
    }

    pub fn get(&self, index: usize) -> ConnectionState {
        self.states
            .get(index)
            .cloned()
            .unwrap_or(ConnectionState::Disconnected)
    }

    fn set(&mut self, index: usize, state: ConnectionState) {
        if let Some(stored_state) = self.states.get_mut(index) {
            if state == ConnectionState::Disconnected && *stored_state != state {
                self.connected_count -= 1;
            }

            *stored_state = state;
        }
    }

    fn clear(&mut self) {
        self.states.clear();
        self.connected_count = 0;
    }
}

#[derive(Default)]
pub struct BattleComms {
    pub senders: Vec<(Option<usize>, NetplayPacketSender)>,
    pub receivers: Vec<(Option<usize>, NetplayPacketReceiver)>,
    pub remote_id: BattleId,
    pub server: Option<(ClientPacketSender, flume::Receiver<(BattleId, String)>)>,
    pub rtts: Vec<f32>,
    // resolved late in BattleScene
    pub connection_states: ConnectionStates,
    // this will be 0 in replays, avoid using it for anything outside of packets
    pub local_index: usize,
    // recycled output
    pub pending_packets: Vec<NetplayPacket>,
}

impl BattleComms {
    pub fn load_setups(&mut self, setups: &[PlayerSetup]) {
        self.local_index = setups
            .iter()
            .position(|setup| setup.local)
            .unwrap_or_default();

        self.connection_states.load_setups(setups);
    }

    pub fn clear_connection(&mut self) {
        self.connection_states.clear();
        self.senders.clear();
        self.receivers.clear();
        self.server = None;
    }

    pub fn receive_packets(&mut self) {
        let mut pending_removal = VecSet::new();

        // take receivers so we can call mutable methods on self while iterating
        let receivers = std::mem::take(&mut self.receivers);

        'main_loop: for (i, (index, receiver)) in receivers.iter().enumerate() {
            let is_fallback = index.is_none();

            while let Ok(packet) = receiver.try_recv() {
                if !is_fallback && Some(packet.index) != *index {
                    // ignore obvious impersonation cheat
                    continue;
                }

                let index = packet.index;

                if self.connection_states.get(index) == ConnectionState::Disconnected {
                    // ignore packets from players that have already disconnected
                    continue;
                }

                let is_disconnect = matches!(
                    &packet.data,
                    NetplayPacketData::Buffer { data, .. } if data.signals.contains(&NetplaySignal::Disconnect)
                );

                self.pending_packets.push(packet);

                if is_disconnect {
                    self.connection_states
                        .set(index, ConnectionState::Disconnected);

                    if self.connection_states.connected_count() <= 1 {
                        // break to prevent receiving extra packets from the fallback receiver
                        // these extra packets are likely for future scenes
                        // the 1 represents the ConnectionState::Connected we have with ourself
                        break 'main_loop;
                    }
                }
            }

            if receiver.is_disconnected() {
                pending_removal.insert(i);
            }
        }

        // put receivers back
        self.receivers = receivers;

        // remove disconnected receivers
        let mut possible_disconnect_desync = false;

        for i in pending_removal.into_iter().rev() {
            self.senders.remove(i);
            let (player_index, _) = self.receivers.remove(i);

            let Some(peer_index) = player_index else {
                debug_assert_eq!(self.receivers.len(), 0);

                // this is a fallback connection, disconnect all players
                self.disconnect_peers();
                continue;
            };

            if self.connection_states.get(peer_index) == ConnectionState::Disconnected {
                continue;
            }

            possible_disconnect_desync = true;

            if self.connection_states.connected_count() == 1 {
                // we're only connected to ourself?
                self.disconnect_peers();
                break;
            }

            let disconnect_packet = NetplayPacket::new_disconnect_signal(peer_index);
            self.pending_packets.push(disconnect_packet);
        }

        if possible_disconnect_desync {
            // possible desync when there's another player we need to sync a disconnect with
            log::error!("Possible desync from a player disconnect without a Disconnect signal");
        }

        if self.connection_states.connected_count() <= 1 {
            // no need to store these, helps prevent reading too many packets from the fallback receiver
            self.senders.clear();
            self.receivers.clear();
        }
    }

    pub fn disconnect_peers(&mut self) {
        for i in 0..self.connection_states.total_states() {
            if i == self.local_index {
                // avoid marking ourself as disconnected
                continue;
            }

            if self.connection_states.get(i) != ConnectionState::Disconnected {
                let disconnect_packet = NetplayPacket::new_disconnect_signal(i);
                self.pending_packets.push(disconnect_packet);
            }
        }

        self.senders.clear();
        self.receivers.clear();
        self.connection_states.clear();
    }

    pub fn disconnect_peer_with_no_signal(&mut self, peer_index: usize) {
        if peer_index == self.local_index {
            // avoid marking ourself as disconnected
            return;
        }

        // drop sender + receiver
        let mut sender_iter = self.senders.iter();

        if let Some(i) = sender_iter.position(|(player_i, _)| *player_i == Some(peer_index)) {
            let (player_index, _) = self.receivers.remove(i);

            debug_assert_eq!(player_index, Some(peer_index));
        }
    }

    pub fn update_rtt_with_new_value(&mut self, peer_index: usize, new_rtt: f32) -> f32 {
        if self.rtts.len() <= peer_index {
            self.rtts.resize(peer_index + 1, 0.0);
        }

        let rtt = &mut self.rtts[peer_index];

        if *rtt == 0.0 {
            *rtt = new_rtt;
        } else {
            *rtt = Self::smooth_average_f32(*rtt, new_rtt);
        }

        *rtt
    }

    pub fn send(&self, to_index: usize, data: NetplayPacketData) {
        if to_index == self.local_index {
            log::warn!("Attempted to send netplay packet to self");
            return;
        }

        let senders = &self.senders;
        let Some((_, send)) = senders
            .iter()
            .find(|(i, _)| *i == Some(to_index))
            .or(senders.last())
        else {
            return;
        };

        send(NetplayPacket {
            index: self.local_index,
            data,
        });
    }

    pub fn broadcast(&self, data: NetplayPacketData) {
        for (_, send) in &self.senders {
            send(NetplayPacket {
                index: self.local_index,
                data: data.clone(),
            });
        }
    }

    const SMOOTH_FACTOR: f32 = 0.125;
    const SMOOTH_FACTOR_FLIPPED: f32 = 1.0 - Self::SMOOTH_FACTOR;

    pub fn smooth_average_f32(old: f32, new: f32) -> f32 {
        old * Self::SMOOTH_FACTOR_FLIPPED + new * Self::SMOOTH_FACTOR
    }
}
