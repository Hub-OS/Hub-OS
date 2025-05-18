use crate::{render::FrameTime, resources::INPUT_BUFFER_LIMIT};
use serde::{Deserialize, Serialize};
use std::collections::{HashMap, HashSet};

#[derive(Serialize, Deserialize, Clone)]
pub enum ExternalEvent {
    ServerMessage(String),
}

pub struct ExternalEvents {
    pub server: EventSyncer<String>,
    events: Vec<(FrameTime, ExternalEvent)>,
}

impl ExternalEvents {
    pub fn new() -> Self {
        Self {
            server: EventSyncer::new(),
            events: Default::default(),
        }
    }

    pub fn load(&mut self, events: Vec<(FrameTime, ExternalEvent)>) {
        self.events.extend(events);
    }

    pub fn tick(&mut self, synced_time: FrameTime, total_players: usize) {
        // make synced events available at the end of our buffer limit
        // that way we don't need to roll back to handle the event
        let next_time = synced_time + INPUT_BUFFER_LIMIT as FrameTime;

        for message in self.server.tick(total_players) {
            self.events
                .push((next_time, ExternalEvent::ServerMessage(message)));
        }
    }

    pub fn events_for_time(&self, time: FrameTime) -> impl Iterator<Item = &ExternalEvent> {
        self.events
            .iter()
            .skip_while(move |(t, _)| *t < time)
            .take_while(move |(t, _)| *t == time)
            .map(|(_, event)| event)
    }

    pub fn dropped_player(&mut self, player_index: usize) {
        self.server.drop_player(player_index);
    }
}

struct PendingEvent<T> {
    data: Option<T>,
    players_acknowledged: HashSet<usize>,
}

impl<T> PendingEvent<T> {
    fn clear(&mut self) {
        self.data = None;
        self.players_acknowledged.clear();
    }
}

impl<T> Default for PendingEvent<T> {
    fn default() -> Self {
        Self {
            data: Default::default(),
            players_acknowledged: Default::default(),
        }
    }
}

pub struct EventSyncer<T> {
    event_limbo: HashMap<usize, PendingEvent<T>>,
    pending_sync: Vec<usize>,
    pending_release: Vec<(FrameTime, T)>,
    event_recycler: Vec<PendingEvent<T>>,
    total_players: usize,
}

impl<T> EventSyncer<T> {
    fn new() -> Self {
        Self {
            event_limbo: Default::default(),
            pending_sync: Default::default(),
            pending_release: Default::default(),
            event_recycler: Default::default(),
            total_players: usize::MAX,
        }
    }

    fn get_event_mut(&mut self, event_id: usize) -> &mut PendingEvent<T> {
        self.event_limbo
            .entry(event_id)
            .or_insert_with(|| self.event_recycler.pop().unwrap_or_default())
    }

    pub fn update_event(&mut self, event_id: usize, data: T) {
        let event = self.get_event_mut(event_id);
        event.data = Some(data);
    }

    pub fn acknowledge_event(&mut self, event_id: usize, player_index: usize) {
        let event = self.get_event_mut(event_id);
        event.players_acknowledged.insert(player_index);

        if event.players_acknowledged.len() >= self.total_players {
            self.pending_sync.push(event_id);
        }
    }

    fn drop_player(&mut self, player_index: usize) {
        for (_, event) in self.event_limbo.iter_mut() {
            event.players_acknowledged.remove(&player_index);
        }
    }

    fn tick(&mut self, total_players: usize) -> impl Iterator<Item = T> + '_ {
        if total_players != self.total_players {
            self.total_players = total_players;

            for (id, event) in self.event_limbo.iter_mut() {
                if event.players_acknowledged.len() >= total_players {
                    self.pending_sync.push(*id);
                }
            }
        }

        // sort the pending events to ensure a consistent order
        self.pending_sync.sort();

        self.pending_sync
            .drain(..)
            .flat_map(|id| self.event_limbo.remove(&id))
            .flat_map(|mut event| {
                let data = event.data.take();

                event.clear();
                self.event_recycler.push(event);

                data
            })
    }
}
