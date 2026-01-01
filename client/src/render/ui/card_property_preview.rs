use crate::packages::*;
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use std::collections::HashMap;
use std::collections::HashSet;
use uncased::{Uncased, UncasedStr};

/// The back of a card
pub struct CardPropertyPreview {
    card_animator: Animator,
    status_sprite_map: HashMap<Uncased<'static>, Sprite>,
    status_animator: Animator,
    status_sprite: Sprite,
    status_step: Vec2,
    status_bounds: Rect,
    displayed_sprites: Vec<Sprite>,
}

impl CardPropertyPreview {
    pub fn new(game_io: &GameIO) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let status_sprite_map = globals
            .status_packages
            .packages(PackageNamespace::Local)
            .flat_map(|package| {
                Some((
                    Uncased::from(package.flag_name.to_string()),
                    assets.new_sprite(game_io, package.icon_texture_path.as_ref()?),
                ))
            })
            .collect::<HashMap<Uncased<'static>, Sprite>>();

        let mut status_animator =
            Animator::load_new(assets, ResourcePaths::FULL_CARD_STATUSES_ANIMATION);
        status_animator.set_state("DEFAULT");

        let mut card_animator = Animator::load_new(assets, ResourcePaths::FULL_CARD_ANIMATION);
        card_animator.set_state("STANDARD");

        let status_step = status_animator.point_or_zero("STEP");
        let status_bounds = Rect::from_corners(
            card_animator.point_or_zero("STATUS_START"),
            card_animator.point_or_zero("STATUS_END"),
        );

        Self {
            card_animator,
            status_sprite: assets.new_sprite(game_io, ResourcePaths::FULL_CARD),
            status_sprite_map,
            status_animator,
            status_step,
            status_bounds,
            displayed_sprites: Default::default(),
        }
    }

    fn resolve_status_sprite(&mut self, flag: &str) -> Option<Sprite> {
        if self.status_animator.has_state(flag) {
            // use built in sprite, prioritized for resource packs
            self.status_animator.set_state(flag);
            self.status_animator.apply(&mut self.status_sprite);
            return Some(self.status_sprite.clone());
        }

        // use package sprite
        let uncased_flag = <&UncasedStr>::from(flag);
        self.status_sprite_map.get(uncased_flag).cloned()
    }

    pub fn set_package(&mut self, game_io: &GameIO, id: Option<&PackageId>) {
        self.displayed_sprites.clear();

        let globals = game_io.resource::<Globals>().unwrap();

        let Some(package) =
            id.and_then(|id| globals.card_packages.package(PackageNamespace::Local, id))
        else {
            return;
        };

        let properties = &package.card_properties;

        let mut displayed_hit_props = HashSet::new();

        let status_bounds = self.status_bounds;
        let status_start = status_bounds.top_left();
        let mut status_offset = Vec2::ZERO;

        for flag in &properties.hit_flags {
            let Some(mut sprite) = self.resolve_status_sprite(flag) else {
                continue;
            };

            displayed_hit_props.insert(flag.as_str());

            // resolve wrap
            if status_offset.x + self.status_step.x > status_bounds.width {
                status_offset.x = 0.0;
                status_offset.y += self.status_step.y;
            }

            // render
            let position = status_start + status_offset;
            sprite.set_position(position);
            self.displayed_sprites.push(sprite);

            // update position for the next render
            status_offset.x += self.status_step.x;
        }

        // start a new line
        if status_offset.x > 0.0 {
            status_offset.x = 0.0;
            status_offset.y += self.status_step.y;
        }

        // render flags that aren't directly listed
        for (category, dep_id) in &package.package_info.requirements {
            if *category != PackageCategory::Status {
                continue;
            }

            let Some(status_package) = globals
                .status_packages
                .package(PackageNamespace::Local, dep_id)
            else {
                continue;
            };

            let flag = &*status_package.flag_name;

            if !displayed_hit_props.insert(flag) {
                continue;
            }

            let Some(mut sprite) = self.resolve_status_sprite(flag) else {
                continue;
            };

            // resolve wrap
            if status_offset.x + self.status_step.x > status_bounds.width {
                status_offset.x = 0.0;
                status_offset.y += self.status_step.y;
            }

            // render
            let position = status_start + status_offset;
            sprite.set_position(position);
            self.displayed_sprites.push(sprite);

            // update position for the next render
            status_offset.x += self.status_step.x;
        }

        // draw static properties
        self.status_sprite.set_position(Vec2::ZERO);

        let static_properties = [
            (properties.can_charge, "CAN_CHARGE"),
            (properties.can_boost, "CAN_BOOST"),
            (properties.recover != 0, "RECOVER"),
            (properties.conceal, "CONCEAL"),
            (properties.time_freeze, "TIME_FREEZE"),
        ];

        for (applies, state) in static_properties {
            if applies {
                self.card_animator.set_state(state);
                self.card_animator.apply(&mut self.status_sprite);
                self.displayed_sprites.push(self.status_sprite.clone());
            }
        }
    }

    pub fn draw(&mut self, sprite_queue: &mut SpriteColorQueue, position: Vec2) {
        for sprite in &mut self.displayed_sprites {
            let original_position = sprite.position();

            sprite.set_position(original_position + position);
            sprite_queue.draw_sprite(sprite);
            sprite.set_position(original_position);
        }
    }
}
