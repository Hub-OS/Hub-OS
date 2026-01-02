use crate::bindable::Element;
use crate::packages::PackageNamespace;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use indexmap::IndexMap;
use packets::structures::PackageId;
use std::collections::HashSet;
use std::sync::Arc;
use strum::EnumCount;
use strum::IntoEnumIterator;

const ITEM_W: f32 = 14.0;
const ITEM_H: f32 = 14.0;
const PADDING: Vec2 = Vec2::new(0.0, 3.0);
const COLS: usize = Element::COUNT;
const ROWS: usize = 5;

#[derive(Clone, PartialEq, Eq, Hash)]
pub enum CardPropertyFilter {
    Element(Element),
    HitFlag {
        id: PackageId,
        flag_name: Arc<str>,
        name: Arc<str>,
    },
    Chargeable,
    Boostable,
    Recovers,
    TimeFreeze,
    Conceal,
}

const STATIC_PROPERTIES: &[CardPropertyFilter] = &[
    CardPropertyFilter::Chargeable,
    CardPropertyFilter::Boostable,
    CardPropertyFilter::Recovers,
    CardPropertyFilter::TimeFreeze,
    CardPropertyFilter::Conceal,
];

struct StatusItem {
    id: PackageId,
    flag_name: Arc<str>,
    name: Arc<str>,
    icon: Sprite,
}

pub struct CardPropertiesMenu {
    frame: ContextFrame,
    static_property_sprite: Sprite,
    static_property_animator: Animator,
    selection_frame_sprite: Sprite,
    cursor_sprite: Sprite,
    cursor_animator: Animator,
    h_scroll_tracker: ScrollTracker,
    v_scroll_tracker: ScrollTracker,
    hovered_item: Option<CardPropertyFilter>,
    hovered_text: String,
    applied_filters: HashSet<CardPropertyFilter>,
    statuses: Vec<StatusItem>,
    open: bool,
}

impl CardPropertiesMenu {
    pub fn new(game_io: &GameIO) -> Self {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        let mut statuses = IndexMap::new();

        const CONSISTENT: &[(&str, &str)] = &[
            ("Drain", "deck-editor-properties-drain"),
            ("Drag", "deck-editor-properties-drag"),
            ("Flinch", "deck-editor-properties-flinch"),
            ("Flash", "deck-editor-properties-flash"),
            ("PierceInvis", "deck-editor-properties-pierce-invis"),
            ("PierceGuard", "deck-editor-properties-pierce-guard"),
            ("PierceGround", "deck-editor-properties-pierce-ground"),
            ("Blind", "deck-editor-properties-blind"),
            ("Confuse", "deck-editor-properties-confuse"),
            ("Paralyze", "deck-editor-properties-paralyze"),
        ];

        let mut builtin_status_animator =
            Animator::load_new(assets, ResourcePaths::FULL_CARD_STATUSES_ANIMATION);

        for (flag_name, translation_key) in CONSISTENT {
            let mut icon = assets.new_sprite(game_io, ResourcePaths::FULL_CARD);
            builtin_status_animator.set_state(flag_name);
            builtin_status_animator.apply(&mut icon);

            statuses.insert(
                *flag_name,
                StatusItem {
                    id: PackageId::new_blank(),
                    flag_name: (*flag_name).into(),
                    name: globals.translate(translation_key).into(),
                    icon,
                },
            );
        }

        for namespace in [PackageNamespace::BuiltIn, PackageNamespace::Local] {
            let mut temp_list = Vec::new();

            for package in globals.status_packages.packages(namespace) {
                let Some(icon_path) = &package.icon_texture_path else {
                    // if we have no icon, we're going to assume the author doesn't want this status listed
                    continue;
                };

                let icon = if builtin_status_animator.has_state(&package.flag_name) {
                    // use built in sprite, prioritized for resource packs
                    let mut icon = assets.new_sprite(game_io, ResourcePaths::FULL_CARD);
                    builtin_status_animator.set_state(&package.flag_name);
                    builtin_status_animator.apply(&mut icon);
                    icon
                } else {
                    assets.new_sprite(game_io, icon_path)
                };

                // push status
                temp_list.push((
                    &*package.flag_name,
                    StatusItem {
                        id: package.package_info.id.clone(),
                        flag_name: package.flag_name.clone(),
                        name: package.name.clone(),
                        icon,
                    },
                ));
            }

            // sort for a consistent list
            temp_list.sort_by_key(|(key, _)| *key);

            // extend
            statuses.extend(temp_list);
        }

        let status_rows = statuses.len().div_ceil(COLS);

        // cursor and selection frame
        let mut cursor_animator = Animator::load_new(assets, ResourcePaths::ICON_CURSOR_ANIMATION);
        let cursor_sprite = assets.new_sprite(game_io, ResourcePaths::ICON_CURSOR);

        let mut selection_frame_sprite = cursor_sprite.clone();
        cursor_animator.set_state("SELECTED_FRAME");
        cursor_animator.apply(&mut selection_frame_sprite);

        Self {
            frame: ContextFrame::new(
                game_io,
                globals.translate("deck-editor-properties-menu-label"),
            )
            .with_inner_size(
                Vec2::new(COLS as f32 * ITEM_W, ROWS as f32 * ITEM_H)
                    + PADDING * 2.0
                    + Vec2::new(
                        0.0,
                        TextStyle::new(game_io, FontName::Context).line_height() + 2.0,
                    ),
            ),
            static_property_sprite: assets.new_sprite(game_io, ResourcePaths::FULL_CARD),
            static_property_animator: Animator::load_new(
                assets,
                ResourcePaths::FULL_CARD_ANIMATION,
            ),
            selection_frame_sprite,
            cursor_sprite,
            cursor_animator: cursor_animator
                .with_state("DEFAULT")
                .with_loop_mode(AnimatorLoopMode::Loop),
            h_scroll_tracker: ScrollTracker::new(game_io, Element::COUNT).with_wrap(true),
            v_scroll_tracker: ScrollTracker::new(game_io, ROWS).with_total_items(2 + status_rows),
            hovered_item: None,
            hovered_text: Default::default(),
            applied_filters: Default::default(),
            statuses: statuses.into_values().collect(),
            open: false,
        }
    }

    pub fn applied_filters(&self) -> impl Iterator<Item = &CardPropertyFilter> {
        self.applied_filters.iter()
    }

    pub fn is_open(&self) -> bool {
        self.open
    }

    pub fn open(&mut self, game_io: &GameIO) {
        self.open = true;
        self.h_scroll_tracker.set_selected_index(0);
        self.v_scroll_tracker.set_selected_index(0);
        self.h_scroll_tracker.set_total_items(Element::COUNT);
        self.resolve_hovered_item(game_io);
    }

    pub fn close(&mut self) {
        self.open = false;
    }

    pub fn update(&mut self, game_io: &mut GameIO, ui_input_tracker: &UiInputTracker) {
        if !self.open {
            return;
        }

        // update animations
        self.cursor_animator.update();

        // handle input
        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Cancel) {
            // closed
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_cancel);

            self.open = false;
            return;
        }

        // scroll
        let prev_v_index = self.v_scroll_tracker.selected_index();
        let prev_h_index = self.h_scroll_tracker.selected_index();

        self.v_scroll_tracker
            .handle_vertical_input(ui_input_tracker);

        if prev_v_index != self.v_scroll_tracker.selected_index() {
            let total_items = match self.v_scroll_tracker.selected_index() {
                0 => Element::COUNT,
                1 => STATIC_PROPERTIES.len(),
                row => (self.statuses.len() - (row - 2) * COLS).min(COLS),
            };

            self.h_scroll_tracker.set_total_items(total_items);
        }

        self.h_scroll_tracker
            .handle_horizontal_input(ui_input_tracker);

        // sfx and resolving selection
        if prev_v_index != self.v_scroll_tracker.selected_index()
            || prev_h_index != self.h_scroll_tracker.selected_index()
        {
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_move);

            self.resolve_hovered_item(game_io);
        }

        if input_util.was_just_pressed(Input::Confirm) {
            // selection
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_select);

            if let Some(filter) = &self.hovered_item {
                if self.applied_filters.contains(filter) {
                    self.applied_filters.remove(filter);
                } else {
                    self.applied_filters.insert(filter.clone());
                }
            }
        }
    }

    fn resolve_hovered_item(&mut self, game_io: &GameIO) {
        self.hovered_item = match self.v_scroll_tracker.selected_index() {
            0 => Element::iter()
                .nth(self.h_scroll_tracker.selected_index())
                .map(CardPropertyFilter::Element),
            1 => STATIC_PROPERTIES
                .get(self.h_scroll_tracker.selected_index())
                .cloned(),
            row => {
                let i = (row - 2) * COLS + self.h_scroll_tracker.selected_index();
                self.statuses.get(i).map(|s| CardPropertyFilter::HitFlag {
                    id: s.id.clone(),
                    name: s.name.clone(),
                    flag_name: s.flag_name.clone(),
                })
            }
        };

        let globals = Globals::from_resources(game_io);

        self.hovered_text = self
            .hovered_item
            .as_ref()
            .map(|item| match item {
                CardPropertyFilter::Element(element) => {
                    globals.translate(element.translation_key())
                }
                CardPropertyFilter::HitFlag { name, .. } => name.to_string(),
                CardPropertyFilter::Chargeable => {
                    globals.translate("deck-editor-properties-chargeable")
                }
                CardPropertyFilter::Boostable => {
                    globals.translate("deck-editor-properties-boostable")
                }
                CardPropertyFilter::Recovers => {
                    globals.translate("deck-editor-properties-recovers")
                }
                CardPropertyFilter::TimeFreeze => {
                    globals.translate("deck-editor-properties-time-freeze")
                }
                CardPropertyFilter::Conceal => {
                    globals.translate("deck-editor-properties-concealed")
                }
            })
            .unwrap_or_default();
    }

    pub fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        if !self.open {
            return;
        }

        self.frame.draw(game_io, sprite_queue);

        // draw options
        let top_left = self.frame.inner_top_left() + PADDING;
        let mut item_position = top_left;

        for row in self.v_scroll_tracker.view_range() {
            match row {
                0 => {
                    for element in Element::iter() {
                        let mut sprite = ElementSprite::new(game_io, element);
                        sprite.set_position(item_position);
                        sprite_queue.draw_sprite(&sprite);

                        // render selection frame
                        let key = CardPropertyFilter::Element(element);

                        if self.applied_filters.contains(&key) {
                            self.selection_frame_sprite.set_position(item_position);
                            sprite_queue.draw_sprite(&self.selection_frame_sprite);
                        }

                        item_position.x += ITEM_W;
                    }
                }
                1 => {
                    for filter in STATIC_PROPERTIES {
                        let state = match filter {
                            CardPropertyFilter::Chargeable => "CAN_CHARGE",
                            CardPropertyFilter::Boostable => "CAN_BOOST",
                            CardPropertyFilter::Recovers => "RECOVER",
                            CardPropertyFilter::Conceal => "CONCEAL",
                            CardPropertyFilter::TimeFreeze => "TIME_FREEZE",
                            _ => {
                                log::error!("No state defined for static property?");
                                continue;
                            }
                        };

                        self.static_property_animator.set_state(state);
                        self.static_property_animator
                            .apply(&mut self.static_property_sprite);
                        self.static_property_sprite.set_position(item_position);

                        // offset, our grid uses 14x14 cells, while our static statuses are 12x12
                        self.static_property_sprite.set_origin(-Vec2::ONE);

                        sprite_queue.draw_sprite(&self.static_property_sprite);

                        // render selection frame
                        if self.applied_filters.contains(filter) {
                            self.selection_frame_sprite.set_position(item_position);
                            sprite_queue.draw_sprite(&self.selection_frame_sprite);
                        }

                        item_position.x += ITEM_W;
                    }
                }
                _ => {
                    let start = (row - 2) * COLS;
                    let end = self.statuses.len().min(start + COLS);

                    for item in &mut self.statuses[start..end] {
                        // offset, our grid uses 14x14 cells, while our statuses are 12x12
                        item.icon.set_position(item_position + Vec2::ONE);
                        sprite_queue.draw_sprite(&item.icon);

                        // render selection frame
                        let key = CardPropertyFilter::HitFlag {
                            id: item.id.clone(),
                            flag_name: item.flag_name.clone(),
                            name: item.name.clone(),
                        };

                        if self.applied_filters.contains(&key) {
                            self.selection_frame_sprite.set_position(item_position);
                            sprite_queue.draw_sprite(&self.selection_frame_sprite);
                        }

                        item_position.x += ITEM_W;
                    }
                }
            }

            item_position.x = top_left.x;
            item_position.y += ITEM_H;
        }

        // render text
        let inner_size = self.frame.inner_size();

        let mut text_style = TextStyle::new(game_io, FontName::Context);
        let text_size = text_style.measure(&self.hovered_text).size;

        text_style.bounds.set_position(top_left);
        text_style.bounds.x += (inner_size.x - text_size.x) * 0.5;
        text_style.bounds.y += inner_size.y - text_style.line_height() - text_style.line_spacing;
        text_style.draw(game_io, sprite_queue, &self.hovered_text);

        // render cursor
        let mut cursor_position = top_left;
        cursor_position.x += self.h_scroll_tracker.selected_index() as f32 * ITEM_W;
        cursor_position.y += (self.v_scroll_tracker.selected_index()
            - self.v_scroll_tracker.top_index()) as f32
            * ITEM_H;
        self.cursor_sprite.set_position(cursor_position);

        self.cursor_animator.apply(&mut self.cursor_sprite);
        sprite_queue.draw_sprite(&self.cursor_sprite);

        // render scroll arrows
        self.frame
            .draw_scroll_arrows(game_io, sprite_queue, &self.v_scroll_tracker);
    }
}
