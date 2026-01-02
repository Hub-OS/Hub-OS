use super::{BattleCallback, SharedBattleResources};
use crate::bindable::CardProperties;
use crate::packages::{CardPackage, PackageNamespace};
use crate::render::SpriteColorQueue;
use crate::resources::{AssetManager, Globals, ResourcePaths};
use crate::saves::Card;
use framework::prelude::{GameIO, Texture, Vec2};
use packets::structures::PackageId;
use std::collections::VecDeque;
use std::sync::Arc;

#[derive(Clone)]
pub enum StagedItemData {
    Deck(usize),
    Discard(usize),
    Card(CardProperties),
    Form((usize, Option<Arc<Texture>>, Option<Box<str>>)),
    Icon((Arc<Texture>, Box<str>)),
}

#[derive(Clone)]
pub struct StagedItem {
    pub data: StagedItemData,
    pub undo_callback: Option<BattleCallback>,
}

#[derive(Default, Clone)]
pub struct StagedItems {
    confirmed: bool,
    updated: bool,
    items: VecDeque<StagedItem>,
}

impl StagedItems {
    pub fn confirmed(&self) -> bool {
        self.confirmed
    }

    pub fn set_confirmed(&mut self, confirmed: bool) {
        self.confirmed = confirmed
    }

    pub fn take_updated(&mut self) -> bool {
        let updated = self.updated;
        self.updated = false;
        updated
    }

    pub fn stage_form(
        &mut self,
        form_index: usize,
        texture: Option<Arc<Texture>>,
        texture_path: Option<Box<str>>,
    ) {
        let item = StagedItem {
            data: StagedItemData::Form((form_index, texture, texture_path)),
            undo_callback: None,
        };

        if self.stored_form_index().is_some() {
            self.items[0] = item;
        } else {
            self.items.push_front(item);
        }

        self.updated = true
    }

    pub fn stage_item(&mut self, item: StagedItem) {
        if let StagedItemData::Form((index, texture, texture_path)) = item.data {
            self.stage_form(index, texture, texture_path);
            self.items[0].undo_callback = item.undo_callback;
            return;
        }

        if let StagedItemData::Deck(i) | StagedItemData::Discard(i) = &item.data {
            let index = *i;

            self.items.retain(|item|{
                !matches!(item.data, StagedItemData::Deck(i) | StagedItemData::Discard(i) if i == index)
            })
        }

        self.items.push_back(item);
        self.updated = true
    }

    pub fn stored_form_index(&self) -> Option<usize> {
        self.items.front().and_then(|item| match &item.data {
            StagedItemData::Form((i, _, _)) => Some(*i),
            _ => None,
        })
    }

    /// Returns the undo callback for the form
    #[must_use]
    pub fn drop_form_selection(&mut self) -> Option<(usize, Option<BattleCallback>)> {
        let index = self.stored_form_index()?;
        let item = self.items.pop_front()?;
        self.updated = true;
        Some((index, item.undo_callback))
    }

    pub fn has_deck_index(&self, index: usize) -> bool {
        self.deck_card_indices().any(|i| i == index)
    }

    pub fn deck_card_indices(&self) -> impl Iterator<Item = usize> + '_ {
        self.items.iter().flat_map(|item| match &item.data {
            StagedItemData::Deck(i) | StagedItemData::Discard(i) => Some(*i),
            _ => None,
        })
    }

    pub fn selected_deck_card_indices(&self) -> Vec<usize> {
        self.items
            .iter()
            .flat_map(|item| match &item.data {
                StagedItemData::Deck(i) => Some(*i),
                _ => None,
            })
            .collect()
    }

    pub fn resolve_card_properties<'a>(
        &'a self,
        game_io: &'a GameIO,
        resources: &'a SharedBattleResources,
        deck: &'a [(Card, PackageNamespace)],
    ) -> impl DoubleEndedIterator<Item = CardProperties> + 'a {
        let globals = Globals::from_resources(game_io);
        let card_packages = &globals.card_packages;
        let status_registry = &resources.status_registry;

        self.items.iter().flat_map(move |card| match &card.data {
            StagedItemData::Deck(i) => {
                let (card, namespace) = deck.get(*i)?;
                let package = card_packages.package_or_fallback(*namespace, &card.package_id)?;
                let mut card_properties = package.card_properties.to_bindable(status_registry);

                card_properties.code.clone_from(&card.code);
                card_properties.namespace = Some(*namespace);

                Some(card_properties)
            }
            StagedItemData::Card(properties) => Some(properties.clone()),
            StagedItemData::Discard(_) | StagedItemData::Form(_) | StagedItemData::Icon(_) => None,
        })
    }

    /// Iterator of card id and code references, includes staged cards not in the deck.
    pub fn resolve_card_ids_and_codes<'a>(
        &'a self,
        deck: &'a [(Card, PackageNamespace)],
    ) -> impl DoubleEndedIterator<Item = (&'a PackageId, &'a str)> {
        self.items.iter().flat_map(move |card| match &card.data {
            StagedItemData::Deck(i) => {
                let (card, _) = deck.get(*i)?;

                Some((&card.package_id, card.code.as_str()))
            }
            StagedItemData::Card(properties) => {
                if !properties.code.is_empty() {
                    Some((&properties.package_id, properties.code.as_str()))
                } else {
                    None
                }
            }
            StagedItemData::Discard(_) | StagedItemData::Form(_) | StagedItemData::Icon(_) => None,
        })
    }

    pub fn iter(&self) -> impl Iterator<Item = &StagedItem> {
        self.items.iter()
    }

    fn visible_iter(&self) -> impl Iterator<Item = &StagedItem> {
        self.items.iter().filter(|item| {
            matches!(
                item.data,
                StagedItemData::Deck(_)
                    | StagedItemData::Card(_)
                    | StagedItemData::Form((_, Some(_), _))
                    | StagedItemData::Icon(_)
            )
        })
    }

    pub fn visible_count(&self) -> usize {
        self.visible_iter().count()
    }

    pub fn draw_icons(
        &self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        deck: &[(Card, PackageNamespace)],
        start: Vec2,
        step: Vec2,
    ) {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        let mut sprite = assets.new_sprite(game_io, ResourcePaths::BLANK);
        let mut position = start;

        for item in self.visible_iter() {
            let texture = match &item.data {
                StagedItemData::Deck(i) => {
                    if let Some((card, namespace)) = deck.get(*i) {
                        let (texture, _) =
                            CardPackage::icon_texture(game_io, *namespace, &card.package_id);
                        texture
                    } else {
                        assets.texture(game_io, ResourcePaths::CARD_ICON_MISSING)
                    }
                }
                StagedItemData::Card(props) => {
                    let (texture, _) = CardPackage::icon_texture(
                        game_io,
                        props.namespace.unwrap_or(PackageNamespace::Local),
                        &props.package_id,
                    );
                    texture
                }
                StagedItemData::Form((_, texture, _)) => texture.clone().unwrap(),
                StagedItemData::Icon((texture, _)) => texture.clone(),
                StagedItemData::Discard(_) => unreachable!(),
            };

            sprite.set_texture(texture);
            sprite.set_position(position);
            sprite_queue.draw_sprite(&sprite);

            position += step;
        }
    }

    pub fn pop(&mut self) -> Option<StagedItem> {
        self.updated = true;
        self.items.pop_back()
    }

    pub fn clear(&mut self) {
        self.items.clear();
        self.updated = true;
    }

    pub fn handle_deck_index_removed(&mut self, index: usize) {
        self.updated = true;
        self.items.retain_mut(|item| match &mut item.data {
            StagedItemData::Deck(i) | StagedItemData::Discard(i) => {
                if *i > index {
                    *i -= 1;
                }

                *i != index
            }
            StagedItemData::Card(_) | StagedItemData::Form(_) | StagedItemData::Icon(_) => true,
        })
    }

    pub fn handle_deck_index_inserted(&mut self, index: usize) {
        self.updated = true;
        for item in &mut self.items {
            match &mut item.data {
                StagedItemData::Deck(i) | StagedItemData::Discard(i) => {
                    if *i >= index {
                        *i += 1;
                    }
                }
                StagedItemData::Card(_) | StagedItemData::Form(_) | StagedItemData::Icon(_) => {}
            }
        }
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for &StagedItem {
    fn into_lua(
        self,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        let table = lua.create_table()?;
        match &self.data {
            StagedItemData::Deck(i) | StagedItemData::Discard(i) => {
                table.set("index", *i + 1)?;
            }
            StagedItemData::Form((i, _, _)) => {
                table.set("index", *i)?;
            }
            StagedItemData::Card(properties) => {
                table.set("card_properties", properties)?;
            }
            StagedItemData::Icon(_) => {}
        }

        let category = match &self.data {
            StagedItemData::Deck(_) => "deck_card",
            StagedItemData::Discard(_) => "deck_discard",
            StagedItemData::Card(_) => "card",
            StagedItemData::Form(_) => "form",
            StagedItemData::Icon(_) => "icon",
        };

        table.set("category", category)?;

        Ok(rollback_mlua::Value::Table(table))
    }
}
