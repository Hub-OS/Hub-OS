use framework::math::{Duration, Instant};
use packets::RecoveryPhase;
use std::collections::HashMap;

const HEARTBEAT_INTERVAL: Duration = Duration::from_millis(250);
const HEARTBEAT_TIMEOUT: Duration = Duration::from_secs(2);
const COUNTDOWN: Duration = Duration::from_secs(3);

/// Recovery is controlled by explicit peer messages, not gameplay inputs:
/// deliberately paused clients do not generate gameplay inputs.
pub struct BattleRecovery {
    episode: u64,
    phase: RecoveryPhase,
    active: bool,
    participants: Option<Vec<usize>>,
    peers: HashMap<usize, (RecoveryPhase, Instant, usize)>,
    waiting_since: Option<Instant>,
    countdown_end: Option<Instant>,
    go_displayed: bool,
    last_broadcast: Option<Instant>,
}

impl Default for BattleRecovery {
    fn default() -> Self {
        Self {
            episode: 0,
            phase: RecoveryPhase::Resumed,
            active: false,
            participants: None,
            peers: HashMap::new(),
            waiting_since: None,
            countdown_end: None,
            go_displayed: false,
            last_broadcast: None,
        }
    }
}

impl BattleRecovery {
    pub fn has_started(&self) -> bool {
        self.episode != 0
    }

    pub fn active(&self) -> bool {
        self.active
    }

    pub fn waiting(&self) -> bool {
        self.active && self.phase == RecoveryPhase::Waiting
    }

    pub fn begin(&mut self, now: Instant) {
        self.adopt(self.episode.saturating_add(1), now);
    }

    fn adopt(&mut self, episode: u64, now: Instant) {
        self.episode = episode;
        self.phase = RecoveryPhase::Waiting;
        self.active = true;
        self.participants = None;
        self.peers.clear();
        self.waiting_since = Some(now);
        self.countdown_end = None;
        self.go_displayed = false;
        self.last_broadcast = None;
    }

    pub fn receive(
        &mut self,
        peer: usize,
        episode: u64,
        phase: RecoveryPhase,
        input_count: usize,
        now: Instant,
    ) {
        if episode == 0 || episode < self.episode {
            return;
        }
        if episode > self.episode {
            self.adopt(episode, now);
        }
        // Delayed/retransmitted messages cannot move an episode backwards or
        // reopen one that completed. Still record Resumed to finish the barrier.
        self.peers
            .entry(peer)
            .and_modify(|(stored, received, expected_inputs)| {
                *stored = (*stored).max(phase);
                *received = now;
                *expected_inputs = (*expected_inputs).max(input_count);
            })
            .or_insert((phase, now, input_count));
    }

    pub fn cancel(&mut self) {
        self.active = false;
        self.phase = RecoveryPhase::Resumed;
        self.countdown_end = None;
        self.go_displayed = false;
    }

    pub fn tick(&mut self, now: Instant, participants: &[usize], received_inputs: &[usize]) {
        if !self.active {
            return;
        }
        if participants.is_empty()
            || self
                .participants
                .as_deref()
                .is_some_and(|previous| previous != participants)
        {
            // A player leaving is not a successful reconnection.
            self.cancel();
            return;
        }
        self.participants = Some(participants.to_vec());

        let all_heard = participants.iter().all(|peer| {
            self.peers
                .get(peer)
                .is_some_and(|(_, received, _)| now - *received < HEARTBEAT_TIMEOUT)
        });
        if self.phase == RecoveryPhase::Waiting {
            let all_inputs_delivered = participants.iter().all(|&peer| {
                self.peers.get(&peer).is_some_and(|(_, _, expected)| {
                    received_inputs
                        .get(peer)
                        .is_some_and(|received| received >= expected)
                })
            });
            if all_heard && all_inputs_delivered {
                self.phase = RecoveryPhase::Countdown;
                self.countdown_end = Some(now + COUNTDOWN);
                self.last_broadcast = None;
            }
            return;
        }
        if !all_heard {
            // A genuine new interruption invalidates this countdown. A new
            // episode prevents old Ready packets from resuming the battle.
            self.begin(now);
            return;
        }
        if self.phase == RecoveryPhase::Countdown && self.go_displayed {
            self.phase = RecoveryPhase::Ready;
            self.last_broadcast = None;
        }
        if self.phase == RecoveryPhase::Ready
            && participants
                .iter()
                .all(|peer| self.peers[peer].0 >= RecoveryPhase::Ready)
        {
            self.phase = RecoveryPhase::Resumed;
            self.active = false;
            self.last_broadcast = None;
        }
    }

    pub fn outgoing(
        &mut self,
        now: Instant,
        participants: &[usize],
    ) -> Option<(u64, RecoveryPhase)> {
        if self.episode == 0 {
            return None;
        }
        if let Some(last) = self.last_broadcast {
            if (!self.active
                && participants.iter().all(|peer| {
                    self.peers
                        .get(peer)
                        .is_some_and(|(phase, _, _)| *phase == RecoveryPhase::Resumed)
                }))
                || now - last < HEARTBEAT_INTERVAL
            {
                return None;
            }
        }
        self.last_broadcast = Some(now);
        Some((self.episode, self.phase))
    }

    pub fn countdown_remaining(&self, now: Instant) -> Option<u64> {
        if !self.active || self.phase == RecoveryPhase::Waiting {
            return None;
        }
        let remaining = self.countdown_end?.saturating_duration_since(now);
        Some(remaining.as_secs() + u64::from(remaining.subsec_nanos() != 0))
    }

    pub fn waiting_since(&self) -> Option<Instant> {
        self.waiting().then_some(self.waiting_since).flatten()
    }

    pub fn waiting_for(&self, peer: usize, now: Instant, received_inputs: usize) -> bool {
        self.peers.get(&peer).is_none_or(|(_, received, expected)| {
            now - *received >= HEARTBEAT_TIMEOUT || received_inputs < *expected
        })
    }

    pub fn mark_go_displayed(&mut self, now: Instant) {
        if self.countdown_remaining(now) == Some(0) {
            self.go_displayed = true;
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn simulate_recovery(count: usize, interrupt_countdown: bool) -> Vec<BattleRecovery> {
        let start = Instant::now();
        let mut nodes: Vec<_> = (0..count).map(|_| BattleRecovery::default()).collect();
        nodes[0].begin(start);
        let mut deliveries: Vec<(u64, usize, usize, u64, RecoveryPhase)> = Vec::new();
        let mut packet_number = 0;
        let mut go_times = vec![None; count];
        let mut resumed_times = vec![None; count];

        for step in 0..300 {
            let now = start + Duration::from_millis(step * 50);
            let blackout = interrupt_countdown && (50..110).contains(&step);
            deliveries.retain(|&(due, from, to, episode, phase)| {
                if due > step {
                    return true;
                }
                if !blackout {
                    nodes[to].receive(from, episode, phase, 100, now);
                }
                false
            });

            for i in 0..count {
                let peers: Vec<_> = (0..count).filter(|&peer| peer != i).collect();
                let was_active = nodes[i].active();
                // No new gameplay inputs are generated anywhere in this pause.
                nodes[i].tick(now, &peers, &vec![100; count]);
                if was_active && !nodes[i].active() {
                    resumed_times[i] = Some(step);
                }
                if nodes[i].countdown_remaining(now) == Some(0) {
                    nodes[i].mark_go_displayed(now);
                    go_times[i].get_or_insert(step);
                }
                if let Some((episode, phase)) = nodes[i].outgoing(now, &peers) {
                    for &peer in &peers {
                        packet_number += 1;
                        // Asymmetric latency and repeated loss; subsequent
                        // heartbeats stand in for reliable retries.
                        if packet_number % 5 != 0 && !blackout {
                            deliveries.push((
                                step + 1 + (i * 2 + peer) as u64,
                                i,
                                peer,
                                episode,
                                phase,
                            ));
                        }
                    }
                }
            }
        }

        assert!(nodes.iter().all(|node| !node.active()));
        let latest_go = go_times.into_iter().map(Option::unwrap).max().unwrap();
        assert!(
            resumed_times
                .into_iter()
                .all(|step| step.unwrap() >= latest_go)
        );
        let episode = nodes[0].episode;
        assert!(nodes.iter().all(|node| node.episode == episode));
        nodes
    }

    #[test]
    fn recovery_pause_finishes_without_gameplay_input_heartbeats() {
        for count in [2, 3] {
            let mut nodes = simulate_recovery(count, false);
            assert!(nodes.iter().all(|node| node.episode == 1));
            // A late initial Waiting message must not reopen the completed pause.
            for node in &mut nodes {
                node.receive(99, 1, RecoveryPhase::Waiting, 100, Instant::now());
                assert!(!node.active());
            }
        }
    }

    #[test]
    fn recovery_restarts_only_when_link_really_drops_during_countdown() {
        let nodes = simulate_recovery(3, true);
        assert!(nodes[0].episode > 1);
    }

    #[test]
    fn recovery_requires_missing_gameplay_packets_before_countdown() {
        let now = Instant::now();
        let mut recovery = BattleRecovery::default();
        recovery.begin(now);
        recovery.receive(1, 1, RecoveryPhase::Waiting, 100, now);
        recovery.tick(now, &[1], &[0, 99]);
        assert!(recovery.waiting());
        recovery.tick(now, &[1], &[0, 100]);
        assert_eq!(recovery.countdown_remaining(now), Some(3));
    }

    #[test]
    fn recovery_does_not_resume_until_every_peer_has_shown_go() {
        let start = Instant::now();
        let mut recovery = BattleRecovery::default();
        recovery.begin(start);
        recovery.receive(1, 1, RecoveryPhase::Waiting, 100, start);
        recovery.tick(start, &[1], &[0, 100]);
        let now = start + Duration::from_secs(3);
        recovery.receive(1, 1, RecoveryPhase::Countdown, 100, now);
        recovery.tick(now, &[1], &[0, 100]);
        assert!(recovery.active());
        assert_eq!(recovery.phase, RecoveryPhase::Countdown);
        recovery.mark_go_displayed(now);
        recovery.tick(now, &[1], &[0, 100]);
        assert!(recovery.active());
        assert_eq!(recovery.phase, RecoveryPhase::Ready);
        recovery.receive(1, 1, RecoveryPhase::Ready, 100, now);
        recovery.tick(now, &[1], &[0, 100]);
        assert!(!recovery.active());
    }

    #[test]
    fn recovery_cancels_when_a_participant_disconnects() {
        let now = Instant::now();
        let mut recovery = BattleRecovery::default();
        recovery.begin(now);
        recovery.tick(now, &[1, 2], &[0, 0, 0]);
        recovery.tick(now, &[1], &[0, 0, 0]);
        assert!(!recovery.active());
        assert_eq!(recovery.countdown_remaining(now), None);
    }
}
