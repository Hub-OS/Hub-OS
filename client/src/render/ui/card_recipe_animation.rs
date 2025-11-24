use crate::battle::{
    BattleSimulation, Character, PlayerHand, SharedBattleResources, StagedItem, StagedItemData,
};
use crate::bindable::{CardProperties, EntityId};
use crate::packages::PackageNamespace;
use crate::render::{FrameTime, SpriteColorQueue};
use crate::{AssetManager, Globals, ResourcePaths};
use framework::common::GameIO;
use framework::graphics::Color;
use framework::math::Vec2;
use packets::structures::PackageId;
use std::ops::Range;
use std::rc::Rc;

use super::{FontName, TextStyle};

#[derive(Debug, Clone)]
enum State {
    FadeIn,
    DisplayLabel,
    DisplayDelay,
    DisplayCard,
    HidePendingDelay,
    HidePending,
    DisplayChanges,
    HideLabel,
    FadeOut,
    Complete,
}

impl State {
    fn next_state(&self) -> (FrameTime, Self) {
        match self {
            Self::FadeIn => (8, Self::DisplayLabel),
            Self::DisplayLabel => (8, Self::DisplayDelay),
            Self::DisplayDelay => (9, Self::DisplayCard),
            Self::DisplayCard => (16, Self::HidePendingDelay),
            Self::HidePendingDelay => (16, Self::HidePending),
            Self::HidePending => (8, Self::DisplayChanges),
            Self::DisplayChanges => (16 * 5, Self::HideLabel),
            Self::HideLabel => (8, Self::FadeOut),
            Self::FadeOut => (12, Self::Complete),
            Self::Complete => (99, Self::Complete),
        }
    }
}

#[derive(Clone)]
pub struct CardRecipeAnimation {
    total_time: FrameTime,
    time: FrameTime,
    state: State,
    visible_card_count: usize,
    cards: Rc<[CardProperties]>,
    changes: Rc<[(PackageId, Range<usize>)]>,
}

impl CardRecipeAnimation {
    pub fn try_new(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        namespace: PackageNamespace,
        hand: &mut PlayerHand,
    ) -> Option<Self> {
        let cards = hand
            .staged_items
            .resolve_card_properties(game_io, resources, &hand.deck)
            .collect::<Rc<_>>();

        let recipes = &resources.recipes;
        let mut changes = recipes.resolve_changes(namespace, &cards, &hand.used_recipes);

        if changes.is_empty() {
            return None;
        }

        // reset staged cards, preserve form changes and discards
        let form_index = hand.staged_items.stored_form_index();
        let discards: Vec<_> = hand.staged_items.deck_card_indices().collect();

        hand.staged_items.clear();

        for index in discards {
            hand.staged_items.stage_item(StagedItem {
                data: StagedItemData::Discard(index),
                undo_callback: None,
            });
        }

        if let Some(index) = form_index {
            hand.staged_items.stage_form(index, None, None);
        }

        // make sure the target has a VM / was properly marked as a dependency / was not banned by format
        changes
            .retain(|(package_id, _)| resources.vm_manager.find_vm(package_id, namespace).is_ok());

        // clear regular card if it was used
        if hand.has_regular_card && hand.staged_items.has_deck_index(0) {
            hand.has_regular_card = false;
        }

        Some(Self {
            total_time: 0,
            time: 0,
            state: State::FadeIn,
            visible_card_count: 0,
            cards,
            changes: changes.into(),
        })
    }

    pub fn completed(&self) -> bool {
        matches!(self.state, State::Complete)
    }

    pub fn apply(
        &self,
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        resources: &SharedBattleResources,
        entity_id: EntityId,
    ) {
        let entities = &mut simulation.entities;
        let Ok((hand, character, namespace)) =
            entities.query_one_mut::<(&mut PlayerHand, &mut Character, &PackageNamespace)>(
                entity_id.into(),
            )
        else {
            return;
        };

        // store pending cards
        character.cards = self.cards.to_vec();
        character.next_card_mutation = Some(0);

        // apply recipes
        let globals = game_io.resource::<Globals>().unwrap();
        let card_packages = &globals.card_packages;
        let status_registry = &resources.status_registry;

        let mut removed_count: usize = 0;

        for (package_id, range) in self.changes.iter() {
            let uses = hand.used_recipes.entry(package_id.clone()).or_default();
            *uses += 1;

            let Some(card_package) = card_packages.package_or_fallback(*namespace, package_id)
            else {
                continue;
            };

            let card = card_package.card_properties.to_bindable(status_registry);
            let range = range.start - removed_count..range.end - removed_count;
            removed_count += range.end - range.start - 1;

            character.cards.splice(range, std::iter::once(card));
        }

        // reverse cards to allow us to pop the top card on use
        character.cards.reverse();
    }

    pub fn update(
        &mut self,
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        resources: &SharedBattleResources,
        local: bool,
    ) {
        self.total_time += 1;
        self.time += 1;
        let (duration, mut next_state) = self.state.next_state();
        let should_advance = self.time > duration;

        let mut fade_progress = 1.0;

        match self.state {
            State::DisplayCard => {
                if self.time == 1 {
                    if local && self.changes_contain_index(self.visible_card_count) {
                        let globals = game_io.resource::<Globals>().unwrap();
                        simulation.play_sound(game_io, &globals.sfx.indicate);
                    }

                    self.visible_card_count += 1;
                } else if should_advance && self.visible_card_count < self.cards.len() {
                    next_state = State::DisplayCard;
                }
            }
            State::DisplayChanges => {
                if local && self.time == 1 {
                    let globals = game_io.resource::<Globals>().unwrap();
                    simulation.play_sound(game_io, &globals.sfx.craft);
                }
            }
            State::FadeIn => {
                fade_progress = self.time as f32 / duration as f32;
            }
            State::FadeOut => {
                fade_progress = 1.0 - self.time as f32 / duration as f32;
            }
            State::Complete => {
                fade_progress = 0.0;
            }
            _ => {}
        }

        if local {
            const FADE_STRENGTH: f32 = 0.5;
            let a = fade_progress * FADE_STRENGTH;
            let color = Color::BLACK.multiply_alpha(a);
            resources.battle_fade_color.set(color);
        }

        if should_advance {
            self.state = next_state;
            self.time = 0;
        }
    }

    fn changes_contain_index(&self, i: usize) -> bool {
        self.changes.iter().any(|(_, r)| r.contains(&i))
    }

    pub fn draw(
        &self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        sprite_queue: &mut SpriteColorQueue,
        namespace: PackageNamespace,
    ) {
        const TEXT_SHADOW_COLOR: Color = Color::new(0.16, 0.16, 0.16, 1.0);
        const GREEN_TEXT_COLOR: Color = Color::new(0.51, 1.0, 0.51, 1.0);
        const RED_TEXT_COLOR: Color = Color::new(1.0, 0.51, 0.51, 1.0);
        const PLAIN_TEXT_COLOR: Color = Color::WHITE;

        let mut text_style =
            TextStyle::new_monospace(game_io, FontName::Thick).with_shadow_color(TEXT_SHADOW_COLOR);

        // resolve code offset
        let longest_name_len = self
            .cards
            .iter()
            .map(|card| card.short_name.len())
            .max()
            .unwrap_or_default();
        let space_width = text_style.measure(" ").size.x + text_style.letter_spacing;
        let code_x_offset = space_width * (longest_name_len.max(8) + 1) as f32;

        // apply + resolve initial offsets
        text_style.bounds += resources.recipe_animator.point_or_zero("LIST_START");
        let list_step = resources.recipe_animator.point_or_zero("LIST_STEP");

        // display based on state
        let (duration, _) = self.state.next_state();
        let mut label_scale = 1.0;

        match self.state {
            State::FadeIn | State::FadeOut | State::Complete => {
                label_scale = 0.0;
            }
            State::DisplayLabel => {
                label_scale = self.time as f32 / duration as f32;
            }
            State::HideLabel => {
                label_scale = 1.0 - self.time as f32 / duration as f32;
            }
            State::DisplayCard | State::HidePendingDelay => {
                for (i, card) in self.cards.iter().enumerate().take(self.visible_card_count) {
                    text_style.color = if self.changes_contain_index(i) {
                        GREEN_TEXT_COLOR
                    } else {
                        PLAIN_TEXT_COLOR
                    };

                    // draw name
                    text_style.draw(game_io, sprite_queue, &card.short_name);

                    // draw code
                    text_style.bounds.x += code_x_offset;
                    text_style.draw(game_io, sprite_queue, &card.code);
                    text_style.bounds.x -= code_x_offset;

                    text_style.bounds += list_step;
                }
            }
            State::HidePending => {
                for (i, card) in self.cards.iter().enumerate().take(self.visible_card_count) {
                    if !self.changes_contain_index(i) {
                        // draw name
                        text_style.draw(game_io, sprite_queue, &card.short_name);

                        // draw code
                        text_style.bounds.x += code_x_offset;
                        text_style.draw(game_io, sprite_queue, &card.code);
                        text_style.bounds.x -= code_x_offset;
                    }

                    text_style.bounds += list_step;
                }
            }
            State::DisplayChanges => {
                let globals = game_io.resource::<Globals>().unwrap();
                let card_packages = &globals.card_packages;

                let relevant_color = match self.time / 16 % 4 {
                    0 => RED_TEXT_COLOR,
                    1 => GREEN_TEXT_COLOR,
                    2 => PLAIN_TEXT_COLOR,
                    3 => GREEN_TEXT_COLOR,
                    _ => unreachable!(),
                };

                for (i, card) in self.cards.iter().enumerate().take(self.visible_card_count) {
                    if let Some((package_id, range)) =
                        self.changes.iter().find(|(_, r)| r.contains(&i))
                    {
                        // render the new card if we're drawing at the first input in the recipe
                        // otherwise render nothing
                        if range.start == i
                            && let Some(package) = card_packages.package_or_fallback(
                                card.namespace.unwrap_or(namespace),
                                package_id,
                            )
                        {
                            let card = &package.card_properties;
                            text_style.color = relevant_color;
                            text_style.draw(game_io, sprite_queue, &card.short_name);
                        }
                    } else {
                        // draw cards not part of any recipe
                        text_style.color = PLAIN_TEXT_COLOR;
                        text_style.draw(game_io, sprite_queue, &card.short_name);

                        // draw code
                        text_style.bounds.x += code_x_offset;
                        text_style.draw(game_io, sprite_queue, &card.code);
                        text_style.bounds.x -= code_x_offset;
                    }

                    text_style.bounds += list_step;
                }
            }
            _ => {}
        }

        // display label
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;
        let mut label_sprite = assets.new_sprite(game_io, ResourcePaths::BATTLE_RECIPE);
        resources.recipe_animator.apply(&mut label_sprite);
        label_sprite.set_position(resources.recipe_animator.point_or_zero("LABEL"));
        label_sprite.set_scale(Vec2::new(1.0, label_scale));
        sprite_queue.draw_sprite(&label_sprite);
    }
}
