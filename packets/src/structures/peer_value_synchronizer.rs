use serde::{Deserialize, Serialize};
use structures::collections::{VecMap, VecMapEntry};

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
pub enum PeerSyncMessage<T> {
    Begin,
    CurrentValue(T),
    RequestSync,
    Sync(T),
    Disconnect,
    Ready,
}

#[derive(Clone, Debug, PartialEq)]
pub enum PeerSyncResponse<PeerId, T> {
    Send(PeerId, PeerSyncMessage<T>),
    /// Send this message to everyone else
    Broadcast(PeerSyncMessage<T>),
    Complete,
}

pub trait PeerSyncConnectionStates<PeerId> {
    fn total_fully_connected(&self) -> usize {
        self.iter_fully_connected().count()
    }

    /// This should include the local id, not just remote ids
    fn iter_fully_connected(&self) -> impl Iterator<Item = PeerId>;
    fn peer_fully_connected(&self, id: PeerId) -> bool;
}

struct PeerValueState<T> {
    value: T,
    version: usize,
    ready: bool,
}

impl<T> PeerValueState<T> {
    fn new(value: T) -> Self {
        Self {
            value,
            version: 0,
            ready: false,
        }
    }
}

pub struct PeerSynchronizer<PeerId, T> {
    next_sync: usize,
    syncing_with: Option<PeerId>,
    peer_values: VecMap<PeerId, PeerValueState<T>>,
}

impl<PeerId, T> PeerSynchronizer<PeerId, T>
where
    PeerId: PartialEq + Copy,
    T: PartialEq + Copy + PartialOrd + Ord,
{
    pub fn new(local_id: PeerId, initial_value: T) -> Self {
        let mut synchronizer = Self {
            next_sync: 0,
            syncing_with: None,
            peer_values: Default::default(),
        };

        let state = PeerValueState::new(initial_value);
        synchronizer.peer_values.insert(local_id, state);

        synchronizer
    }

    pub fn handle_message(
        &mut self,
        connection_states: &impl PeerSyncConnectionStates<PeerId>,
        local_id: PeerId,
        sender_id: PeerId,
        message: PeerSyncMessage<T>,
        mut response: impl FnMut(PeerSyncResponse<PeerId, T>),
    ) {
        let total_connected = connection_states.total_fully_connected();

        if total_connected == 1 {
            response(PeerSyncResponse::Complete);
            return;
        }

        match message {
            PeerSyncMessage::Begin => {
                let stored = self.peer_values.get(&local_id).unwrap();

                let message = PeerSyncMessage::CurrentValue(stored.value);
                response(PeerSyncResponse::Broadcast(message));
            }
            PeerSyncMessage::CurrentValue(value) => {
                match self.peer_values.entry(sender_id) {
                    VecMapEntry::Occupied(mut entry) => {
                        let stored = entry.get_mut();
                        stored.value = value;
                        stored.version += 1;
                        stored.ready = false;
                    }
                    VecMapEntry::Vacant(entry) => entry.insert(PeerValueState::new(value)),
                }

                self.try_settle(connection_states, total_connected, local_id, response);
            }
            PeerSyncMessage::RequestSync => {
                let stored = self.peer_values.get(&local_id).unwrap();

                let message = PeerSyncMessage::Sync(stored.value);
                response(PeerSyncResponse::Send(sender_id, message));
            }
            PeerSyncMessage::Sync(value) => {
                self.syncing_with = None;

                let stored = self.peer_values.get_mut(&local_id).unwrap();
                stored.value = value;
                stored.version += 1;

                let message = PeerSyncMessage::CurrentValue(value);
                response(PeerSyncResponse::Broadcast(message));

                // test if we're synced now
                if self.total_synced(connection_states) == total_connected {
                    let stored = self.peer_values.get_mut(&local_id).unwrap();
                    stored.ready = true;

                    let message = PeerSyncMessage::Ready;
                    response(PeerSyncResponse::Broadcast(message));

                    if self.total_ready(connection_states) == total_connected {
                        response(PeerSyncResponse::Complete);
                    }
                }
            }
            PeerSyncMessage::Disconnect => {
                if self.syncing_with == Some(sender_id) {
                    self.syncing_with = None;

                    let stored = self.peer_values.get_mut(&local_id).unwrap();
                    stored.version += 1;
                    stored.ready = false;

                    let message = PeerSyncMessage::CurrentValue(stored.value);
                    response(PeerSyncResponse::Broadcast(message));
                }

                self.try_settle(connection_states, total_connected, local_id, response);
            }
            PeerSyncMessage::Ready => {
                if let Some(stored) = self.peer_values.get_mut(&sender_id) {
                    stored.ready = true;
                }

                if self.total_ready(connection_states) == total_connected {
                    response(PeerSyncResponse::Complete);
                }
            }
        }
    }

    fn count(
        &self,
        connection_states: &impl PeerSyncConnectionStates<PeerId>,
        filter: impl Fn(&PeerValueState<T>, &PeerValueState<T>) -> bool,
    ) -> usize {
        let mut connected_iter = self
            .peer_values
            .iter()
            .filter(|(id, ..)| connection_states.peer_fully_connected(*id))
            .peekable();

        let Some((_, a)) = connected_iter.peek() else {
            return 0;
        };

        connected_iter.filter(|(_, b)| filter(a, b)).count()
    }

    fn total_synced(&self, connection_states: &impl PeerSyncConnectionStates<PeerId>) -> usize {
        self.count(connection_states, |a, b| {
            a.version == b.version && a.value == b.value
        })
    }

    fn total_ready(&self, connection_states: &impl PeerSyncConnectionStates<PeerId>) -> usize {
        self.count(connection_states, |a, b| b.ready && a.version == b.version)
    }

    fn count_peer_values(&self, f: impl FnMut(&&(PeerId, PeerValueState<T>)) -> bool) -> usize {
        self.peer_values.iter().filter(f).count()
    }

    fn try_settle(
        &mut self,
        connection_states: &impl PeerSyncConnectionStates<PeerId>,
        total_connected: usize,
        local_id: PeerId,
        mut response: impl FnMut(PeerSyncResponse<PeerId, T>),
    ) {
        let synced_values = self.count_peer_values(|(id, stored)| {
            connection_states.peer_fully_connected(*id) && stored.version >= self.next_sync
        });

        // total_connected includes us, and received values contains our value, so we check for an exact match
        let versions_synced = total_connected == synced_values;

        if !versions_synced {
            return;
        }

        self.next_sync += 1;

        self.syncing_with = self.resolve_syncer(local_id, connection_states);

        if let Some(id) = self.syncing_with {
            let message = PeerSyncMessage::RequestSync;
            response(PeerSyncResponse::Send(id, message));
        } else if self.total_synced(connection_states) == total_connected {
            let stored = self.peer_values.get_mut(&local_id).unwrap();
            stored.ready = true;

            let message = PeerSyncMessage::Ready;
            response(PeerSyncResponse::Broadcast(message));

            if self.total_ready(connection_states) == total_connected {
                response(PeerSyncResponse::Complete);
            }
        } else {
            let stored = self.peer_values.get_mut(&local_id).unwrap();
            stored.version += 1;
            stored.ready = false;

            let message = PeerSyncMessage::CurrentValue(stored.value);
            response(PeerSyncResponse::Broadcast(message));
        }
    }

    fn resolve_syncer(
        &self,
        local_id: PeerId,
        connection_states: &impl PeerSyncConnectionStates<PeerId>,
    ) -> Option<PeerId> {
        let (max_index, max_stored) = self
            .peer_values
            .iter()
            .filter(|(id, _)| connection_states.peer_fully_connected(*id))
            .max_by_key(|(_, stored)| stored.value)?;

        let local_value = self.peer_values.get(&local_id)?.value;

        if local_value >= max_stored.value {
            return None;
        }

        Some(*max_index)
    }
}

#[cfg(test)]
mod test {
    use super::*;

    struct ConnectionStates<'a> {
        states: &'a [bool],
    }

    impl<'a> PeerSyncConnectionStates<usize> for ConnectionStates<'a> {
        fn iter_fully_connected(&self) -> impl Iterator<Item = usize> {
            self.states
                .iter()
                .enumerate()
                .filter(|(_, connected)| **connected)
                .map(|(i, _)| i)
        }

        fn peer_fully_connected(&self, id: usize) -> bool {
            self.states.get(id).copied().unwrap_or_default()
        }
    }

    struct Peer {
        index: usize,
        connection_states: Vec<bool>,
        synchronizer: PeerSynchronizer<usize, usize>,
        synced: bool,
    }

    impl Peer {
        fn new(index: usize, total_peers: usize) -> Self {
            Self {
                index,
                connection_states: vec![true; total_peers],
                // initializing with a value matching our id for simplicity
                synchronizer: PeerSynchronizer::new(index, index),
                synced: false,
            }
        }

        fn connected(&self) -> bool {
            self.connection_states[self.index]
        }

        fn local_data(&self) -> &PeerValueState<usize> {
            self.synchronizer.peer_values.get(&self.index).unwrap()
        }

        fn disconnect(&mut self) {
            self.connection_states.fill(false);
        }

        fn tick(
            &mut self,
            mut send: impl FnMut(usize, PeerSyncMessage<usize>),
            mut recv: impl FnMut() -> Option<(usize, PeerSyncMessage<usize>)>,
        ) {
            while let Some((sender, message)) = recv() {
                if message == PeerSyncMessage::Disconnect {
                    self.connection_states[sender] = false;
                }

                let connection_states = ConnectionStates {
                    states: &self.connection_states,
                };

                self.synchronizer.handle_message(
                    &connection_states,
                    self.index,
                    sender,
                    message.clone(),
                    |response| match response {
                        PeerSyncResponse::Send(to_index, message) => send(to_index, message),
                        PeerSyncResponse::Broadcast(message) => {
                            for (i, connected) in self.connection_states.iter().enumerate() {
                                if i != self.index && *connected {
                                    send(i, message.clone());
                                }
                            }
                        }
                        PeerSyncResponse::Complete => {
                            println!("--! {} Complete !--", self.index);
                            self.synced = true;
                        }
                    },
                );

                // logging
                match message {
                    PeerSyncMessage::Ready => {
                        let iter = self.synchronizer.peer_values.iter();
                        let ready_states: Vec<_> = iter
                            .filter(|(id, _)| connection_states.peer_fully_connected(*id))
                            .map(|(id, stored)| (id, stored.ready))
                            .collect();

                        println!("{ready_states:?}");
                    }
                    PeerSyncMessage::CurrentValue(_) | PeerSyncMessage::Disconnect => {
                        let iter = self.synchronizer.peer_values.iter();
                        let connected_iter =
                            iter.filter(|(id, _)| connection_states.peer_fully_connected(*id));

                        let peer_values = connected_iter
                            .map(|(id, stored)| (*id, stored.value, stored.version))
                            .collect::<Vec<_>>();

                        println!("{} {peer_values:?}", self.index);
                    }
                    _ => {}
                }

                if self.synced {
                    break;
                }
            }
        }
    }

    struct TestOptions {
        test: &'static GroupTest,
        delay_range: std::ops::Range<usize>,
        random_seed: u64,
    }

    type Packet = (usize, usize, PeerSyncMessage<usize>);

    /// `disconnect_on` disconnects senders for emitting a message to a specific peer. Format: (sender, receiver, message)
    fn test_disconnects(options: TestOptions) {
        let GroupTest {
            name,
            expected_value,
            total_peers,
            disconnect_gates,
        } = options.test;
        let random_seed = options.random_seed;
        let total_peers = *total_peers;

        // start marker
        println!("\nStarting test #{random_seed} {name:?}");

        // create peers
        let mut peers = Vec::new();

        for i in 0..total_peers {
            peers.push(Peer::new(i, total_peers));
        }

        // create rng for random latency
        use rand::{RngExt, SeedableRng};
        let mut rng = rand::rngs::SmallRng::seed_from_u64(random_seed);

        // track last delay for keeping randomized latency ordered
        let mut sender_delay_lists = vec![vec![0usize; total_peers]; total_peers];

        // tracking for disconnect gates
        let mut gate_pass_counts = vec![0usize; disconnect_gates.len()];

        // message loop
        let mut outbox: Vec<(usize, Packet)> = Vec::new();
        let mut inbox: Vec<(usize, Packet)> = Vec::new();

        for i in 0..total_peers {
            let packet = (0, i, PeerSyncMessage::Begin);
            inbox.push((0, packet));
        }

        while !inbox.is_empty() {
            // log connection state for this tick
            for peer in &peers {
                let i = peer.index;
                let stored = peer.synchronizer.peer_values.get(&peer.index).unwrap();
                let value = stored.value;
                let connected = if peer.connected() {
                    "Connected"
                } else {
                    "Disconnected"
                };

                println!("Peer {i}, Value {value}, {connected}");
            }

            // drop packets sent to a disconnected or complete peer
            inbox.retain(|(_, (_, receiver, _))| {
                let peer = &peers[*receiver];
                peer.connected() && !peer.synced
            });

            // sort packets
            inbox.sort_by_key(|(delay, _)| *delay);

            for (peer_index, peer) in peers.iter_mut().enumerate() {
                if !peer.connected() || peer.synced {
                    // skip we're disconnected or marked as complete
                    continue;
                }

                let outbox = &mut outbox;
                let inbox = &mut inbox;

                let mut disconnected = false;
                let send_delays = &mut sender_delay_lists[peer_index];

                let send = |i, message: PeerSyncMessage<usize>| {
                    // simulate disconnect
                    if !disconnected {
                        for (gate, pass_count) in
                            disconnect_gates.iter().zip(gate_pass_counts.iter_mut())
                        {
                            if gate.sender != peer_index {
                                continue;
                            }

                            if !(gate.filter)(message.clone()) {
                                continue;
                            }

                            *pass_count += 1;

                            if *pass_count <= gate.limit {
                                continue;
                            }

                            println!("--- Disconnecting {peer_index} ---");
                            disconnected = true;
                        }
                    }

                    if disconnected {
                        println!(" --X {peer_index} failed to send {message:?} to {i} --X",);
                        return;
                    }

                    // resolve random delay
                    let delay = if options.delay_range.is_empty() {
                        0
                    } else {
                        let delay = rng.random_range(options.delay_range.clone());

                        // preserve order
                        let longest_delay = &mut send_delays[i];

                        *longest_delay += delay;

                        *longest_delay
                    };

                    println!(" --> {peer_index} sent {message:?} to {i} -->");

                    let packet = (peer_index, i, message);
                    outbox.push((delay, packet));
                };

                let recv = || {
                    let i = inbox
                        .iter()
                        .take_while(|(delay, _)| *delay == 0)
                        .position(|(_, (_, i, _))| *i == peer_index)?;

                    let (_, (sender, _, message)) = inbox.remove(i);

                    if !options.delay_range.is_empty() {
                        println!("<-- {peer_index} received {message:?} from {sender} <--");
                    }

                    Some((sender, message))
                };

                peer.tick(send, recv);

                if !disconnected {
                    continue;
                }

                peer.disconnect();

                // notify peers of any disconnects
                for (i, last_delay) in send_delays.iter().enumerate() {
                    // using the last delay to avoid sending new data after disconnect (this is a reliable ordered simulation)
                    let packet = (peer_index, i, PeerSyncMessage::Disconnect);
                    outbox.push((*last_delay, packet));
                }
            }

            // advance time
            for (delay, _) in &mut inbox {
                *delay = delay.saturating_sub(1);
            }

            // move outbox packets to inbox
            inbox.append(&mut outbox);
        }

        // end marker
        println!("\nEnding test #{random_seed} {name:?}");

        // assertions
        let first_value = peers.first().map(|peer| peer.local_data().value).unwrap();

        for peer in peers {
            if !peer.connected() {
                continue;
            }

            debug_assert!(peer.synced, "peer {} should be marked synced", peer.index);

            debug_assert_eq!(
                first_value,
                peer.local_data().value,
                "peer {} should have a value matching peers from sync",
                peer.index
            );
        }

        assert!(
            first_value == *expected_value,
            "synced value should match {expected_value}, received {first_value}"
        );
    }

    struct TestConnectionGate {
        sender: usize,
        limit: usize,
        filter: fn(PeerSyncMessage<usize>) -> bool,
    }

    struct GroupTest {
        name: &'static str,
        expected_value: usize,
        total_peers: usize,
        disconnect_gates: &'static [TestConnectionGate],
    }

    #[allow(clippy::type_complexity)]
    const GROUP_TESTS: &[GroupTest] = &[
        GroupTest {
            name: "clean run",
            expected_value: 4,
            total_peers: 5,
            disconnect_gates: &[],
        },
        GroupTest {
            name: "no peers",
            expected_value: 0,
            total_peers: 1,
            disconnect_gates: &[],
        },
        GroupTest {
            name: "all disconnect",
            expected_value: 0,
            total_peers: 2,
            disconnect_gates: &[TestConnectionGate {
                sender: 1,
                limit: 0,
                filter: |packet| matches!(packet, PeerSyncMessage::CurrentValue(_)),
            }],
        },
        GroupTest {
            name: "all but 1 peer receiving the CurrentValue message from 4",
            expected_value: 3,
            total_peers: 5,
            disconnect_gates: &[TestConnectionGate {
                sender: 4,
                limit: 3,
                filter: |packet| matches!(packet, PeerSyncMessage::CurrentValue(_)),
            }],
        },
        GroupTest {
            name: "peer 4 diconnecting before sending Sync",
            expected_value: 3,
            total_peers: 5,
            disconnect_gates: &[TestConnectionGate {
                sender: 4,
                limit: 0,
                filter: |packet| matches!(packet, PeerSyncMessage::Sync(_)),
            }],
        },
        GroupTest {
            name: "all but 1 peer receiving the Sync message from 4",
            expected_value: 4,
            total_peers: 5,
            disconnect_gates: &[TestConnectionGate {
                sender: 4,
                limit: 3,
                filter: |packet| matches!(packet, PeerSyncMessage::Sync(_)),
            }],
        },
    ];

    #[test]
    fn no_latency() {
        for test in GROUP_TESTS {
            test_disconnects(TestOptions {
                test,
                delay_range: 0..0,
                random_seed: 0,
            });
        }
    }

    #[test]
    fn random_latency() {
        for test in GROUP_TESTS {
            for random_seed in 0..50 {
                test_disconnects(TestOptions {
                    test,
                    delay_range: 0..3,
                    random_seed,
                });
            }
        }
    }

    struct DirectTest {
        index: usize,
        initial_value: usize,
        connection_states: &'static [bool],
        inbox: &'static [(usize, PeerSyncMessage<usize>)],
        outbox: &'static [PeerSyncResponse<usize, usize>],
    }

    const DIRECT_TESTS: &[DirectTest] = &[
        // this previously emitted Complete without sending Ready, while passing all other tests
        DirectTest {
            index: 2,
            initial_value: 242,
            connection_states: &[true, true, true],
            inbox: &[
                (0, PeerSyncMessage::Begin),
                (0, PeerSyncMessage::CurrentValue(242)),
                (0, PeerSyncMessage::Ready),
            ],
            outbox: &[PeerSyncResponse::Broadcast(PeerSyncMessage::CurrentValue(
                242,
            ))],
        },
        // should wait for the ready signal before completing
        DirectTest {
            index: 1,
            initial_value: 454,
            connection_states: &[false, true, true],
            inbox: &[
                (0, PeerSyncMessage::Begin),
                (2, PeerSyncMessage::CurrentValue(454)),
            ],
            outbox: &[
                PeerSyncResponse::Broadcast(PeerSyncMessage::CurrentValue(454)),
                PeerSyncResponse::Broadcast(PeerSyncMessage::Ready),
            ],
        },
    ];

    #[test]
    fn direct_tests() {
        for (i, test) in DIRECT_TESTS.iter().enumerate() {
            let index = test.index;
            let mut peer = PeerSynchronizer::new(index, test.initial_value);

            let connection_states = ConnectionStates {
                states: test.connection_states,
            };

            let mut responses = Vec::new();

            for (sender, message) in test.inbox {
                peer.handle_message(
                    &connection_states,
                    index,
                    *sender,
                    message.clone(),
                    |response| responses.push(response),
                );
            }

            assert_eq!(responses.as_slice(), test.outbox, "direct test {i}");
        }
    }
}
