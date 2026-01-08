use crate::battle::{BattleSimulation, Entity, Field, Living};
use crate::bindable::{EntityId, HitFlag};
use crate::render::ui::{FontName, TextStyle};
use crate::render::{FrameTime, SpriteColorQueue};
use crate::resources::{CONTEXT_TEXT_SHADOW_COLOR, Globals};
use framework::prelude::*;
use rand::Rng;

const HIT_COLOR: Color = Color::from_rgb_u32(0xF89F1F);
const HEAL_COLOR: Color = Color::from_rgb_u32(0x71FF4A);
const DRAIN_COLOR: Color = Color::from_rgb_u8s(128, 20, 255);

const HOLD_FRAME: usize = 3;
const SCALE_KEY_FRAMES: [(FrameTime, f32); 5] =
    [(0, 0.0), (7, 1.2), (10, 1.0), (10, 1.0), (14, 0.0)];

#[derive(Default, Clone)]
pub struct HpChanges {
    pub hit: i32,
    pub heal: i32,
    pub drain: i32,
}

impl HpChanges {
    pub fn track_direct_change(simulation: &mut BattleSimulation, id: EntityId, hp_diff: i32) {
        let source = match hp_diff.cmp(&0) {
            std::cmp::Ordering::Greater => HpChangeSource::Heal,
            std::cmp::Ordering::Less => HpChangeSource::Drain,
            std::cmp::Ordering::Equal => return,
        };

        Self::track_source(simulation, id, source, hp_diff);
    }

    pub fn track_source(
        simulation: &mut BattleSimulation,
        id: EntityId,
        source: HpChangeSource,
        hp_diff: i32,
    ) {
        if hp_diff == 0 {
            return;
        }

        let entities = &mut simulation.entities;

        if let Ok(hp_changes) = entities.query_one_mut::<&mut HpChanges>(id.into()) {
            hp_changes.track_source_change_inner(source, hp_diff);
        } else {
            let mut changes = HpChanges::default();
            changes.track_source_change_inner(source, hp_diff);
            let _ = entities.insert_one(id.into(), changes);
        }
    }

    fn track_source_change_inner(&mut self, source: HpChangeSource, change: i32) {
        let abs_change = change.abs();

        match source {
            HpChangeSource::Hit => self.hit += abs_change,
            HpChangeSource::Heal => self.heal += abs_change,
            HpChangeSource::Drain => self.drain += abs_change,
        }
    }
}

#[derive(Clone, Copy)]
pub enum HpChangeSource {
    Hit,
    Heal,
    Drain,
}

impl HpChangeSource {
    fn render_priority(self) -> u8 {
        match self {
            HpChangeSource::Hit => 2,
            HpChangeSource::Heal => 1,
            HpChangeSource::Drain => 0,
        }
    }
}

#[derive(Clone)]
pub struct HpParticle {
    creation_time: FrameTime,
    source: HpChangeSource,
    abs_change: i32,
    sort_key: (i32, i32),
    position: Vec2,
}

impl HpParticle {
    pub fn new(
        field: &Field,
        entity: &Entity,
        time: FrameTime,
        source: HpChangeSource,
        abs_change: i32,
    ) -> Self {
        // use external rng since mods shouldn't be able to read this value
        // and we want to avoid calling the simulation's rng differently for each player
        let mut rng = rand::rng();

        let mut position = field.calc_tile_center((entity.x, entity.y), false);
        position += entity.movement_offset;

        // a little bigger than the default tile size
        let mut randomized_x_radius = 24.0;

        randomized_x_radius *= 1.0 - abs_change as f32 / 200.0 * 0.5;

        position.x += rng.random_range(-1.0..1.0) * randomized_x_radius;

        // half height to just above the player's head
        let height = entity.height.max(0.0);
        position.y -= rng.random_range(height * 0.0..height + 10.0);

        Self {
            creation_time: time,
            source,
            abs_change,
            sort_key: (entity.y, entity.movement_offset.y as _),
            position,
        }
    }

    fn hold_duration(&self) -> FrameTime {
        (8 + self.abs_change as FrameTime / 2).min(120)
    }

    pub fn is_complete(&self, time: FrameTime) -> bool {
        time > self.creation_time + self.hold_duration() + SCALE_KEY_FRAMES.last().unwrap().0
    }

    pub fn update(game_io: &GameIO, simulation: &mut BattleSimulation) {
        let time = simulation.time;
        let hp_particles = &mut simulation.hp_particles;
        let entities = &mut simulation.entities;
        let globals = Globals::from_resources(game_io);

        // despawn
        hp_particles.retain_mut(|particle| !particle.is_complete(time));

        // resolve blind filter to avoid revealing position through particles
        let local_player_id = simulation.local_player_id;
        let blind_filter = entities
            .query_one_mut::<(&Entity, &Living)>(local_player_id.into())
            .ok()
            .and_then(|(entity, living)| {
                if living.status_director.remaining_status_time(HitFlag::BLIND) > 0 {
                    Some(entity.team)
                } else {
                    None
                }
            });

        // spawn
        let mut spawned = false;

        let mut try_spawn = |entity, source, value| {
            if value <= 0 {
                return;
            }

            let enabled = match source {
                HpChangeSource::Hit => globals.config.hit_numbers,
                HpChangeSource::Heal => globals.config.heal_numbers,
                HpChangeSource::Drain => globals.config.drain_numbers,
            };

            if !enabled {
                return;
            }

            let field = &simulation.field;

            let particle = HpParticle::new(field, entity, time, source, value);
            hp_particles.push(particle);
            spawned = true;
        };

        for (_, (entity, changes)) in entities.query_mut::<(&Entity, &mut HpChanges)>() {
            // only spawn particles for entities that are visible
            if entity.on_field && blind_filter.is_none_or(|team| entity.team == team) {
                try_spawn(entity, HpChangeSource::Hit, changes.hit);
                try_spawn(entity, HpChangeSource::Heal, changes.heal);
                try_spawn(entity, HpChangeSource::Drain, changes.drain);
            }

            *changes = HpChanges::default();
        }

        // sort for render
        if spawned {
            hp_particles
                .sort_by_key(|particle| (particle.sort_key, particle.source.render_priority()));
        }
    }

    pub fn draw(
        &self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        time: FrameTime,
        perspective_flipped: bool,
    ) {
        // resolve animation
        let mut scale_key_frames = SCALE_KEY_FRAMES;
        let hold_duration = self.hold_duration();

        for (start_time, _) in &mut scale_key_frames[HOLD_FRAME..] {
            *start_time += hold_duration;
        }

        // resolve base scale
        let base_scale = 1.0 + inverse_lerp!(10.0, 200.0, self.abs_change);

        // resolve animation scale
        let elapsed_time = time - self.creation_time;

        let Some((key_i, (key_start_time, key_start_value))) = scale_key_frames
            .iter()
            .enumerate()
            .take_while(|(_, (start_time, _))| *start_time < elapsed_time)
            .last()
        else {
            return;
        };

        let Some((key_end_time, key_end_value)) = scale_key_frames.get(key_i + 1) else {
            return;
        };

        let progress = inverse_lerp!(*key_start_time, *key_end_time, elapsed_time);
        let animation_scale = progress * (key_end_value - key_start_value) + key_start_value;

        // resolve text
        let text = self.abs_change.to_string();

        let mut text_style = TextStyle::new(game_io, FontName::ThinSmall);
        text_style.scale = Vec2::new(base_scale, base_scale * animation_scale);
        text_style.shadow_color = CONTEXT_TEXT_SHADOW_COLOR;
        text_style.color = match self.source {
            HpChangeSource::Hit => HIT_COLOR,
            HpChangeSource::Heal => HEAL_COLOR,
            HpChangeSource::Drain => DRAIN_COLOR,
        };

        // resolve text position
        let mut position = self.position;

        if perspective_flipped {
            position.x *= -1.0;
        }

        text_style.bounds += position - text_style.measure(&text).size * 0.5;

        // render
        text_style.draw(game_io, sprite_queue, &text);
    }
}
