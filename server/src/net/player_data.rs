use packets::structures::{BlockColor, Emotion, Inventory, PackageId};
use std::borrow::Cow;
use std::collections::{HashMap, HashSet};

pub struct PlayerData {
    pub identity: Vec<u8>,
    pub avatar_name: String,
    pub element: String,
    pub health: i32,
    pub base_health: i32,
    pub health_boost: i32,
    pub emotion: Emotion,
    pub money: u32,
    pub inventory: Inventory,
    pub owned_cards: HashMap<(Cow<'static, str>, Cow<'static, str>), usize>,
    pub owned_blocks: HashMap<(Cow<'static, str>, BlockColor), usize>,
    pub owned_players: HashSet<String>,
}

impl PlayerData {
    pub fn new(identity: Vec<u8>) -> PlayerData {
        PlayerData {
            identity,
            avatar_name: String::new(),
            element: String::new(),
            health: 0,
            base_health: 0,
            health_boost: 0,
            emotion: Emotion::default(),
            money: 0,
            inventory: Inventory::new(),
            owned_cards: HashMap::new(),
            owned_blocks: HashMap::new(),
            owned_players: HashSet::new(),
        }
    }

    pub fn add_card(&mut self, package_id: &PackageId, code: &str, count: isize) {
        let key = (
            Cow::Owned(package_id.to_string()),
            Cow::Owned(code.to_string()),
        );

        self.owned_cards
            .entry(key)
            .and_modify(|existing_count| {
                *existing_count = (*existing_count as isize + count).max(0) as usize;
            })
            .or_insert(if count > 0 { count as usize } else { 0 });
    }

    pub fn add_block(&mut self, package_id: &PackageId, color: BlockColor, count: isize) {
        let key = (Cow::Owned(package_id.to_string()), color);

        self.owned_blocks
            .entry(key)
            .and_modify(|existing_count| {
                *existing_count = (*existing_count as isize + count).max(0) as usize;
            })
            .or_insert(if count > 0 { count as usize } else { 0 });
    }

    pub fn enable_player_character(&mut self, package_id: &PackageId, enabled: bool) {
        if enabled {
            self.owned_players.insert(package_id.to_string());
        } else {
            self.owned_players.remove(package_id.as_str());
        }
    }

    pub fn max_health(&self) -> i32 {
        self.base_health + self.health_boost
    }
}
