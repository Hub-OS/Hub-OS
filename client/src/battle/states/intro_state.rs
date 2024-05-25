use super::State;
use crate::battle::*;
use crate::bindable::EntityId;
use crate::render::{FrameTime, SpriteShaderEffect};
use crate::resources::{Globals, SoundBuffer};
use crate::transitions::BATTLE_FADE_DURATION;
use framework::prelude::*;
use std::collections::VecDeque;

#[derive(Clone)]
pub struct BattleInitMusic {
    pub buffer: SoundBuffer,
    pub loops: bool,
}

// max time per entity
const MAX_ANIMATION_TIME: FrameTime = 32;

#[derive(Clone)]
pub struct IntroState {
    completed: bool,
    animation_time: FrameTime, // animation time for the current entity
    tracked_entities: VecDeque<EntityId>,
}

impl State for IntroState {
    fn clone_box(&self) -> Box<dyn State> {
        Box::new(self.clone())
    }

    fn next_state(&self, game_io: &GameIO) -> Option<Box<dyn State>> {
        if self.completed {
            Some(Box::new(CardSelectState::new(game_io)))
        } else {
            None
        }
    }

    fn update(
        &mut self,
        game_io: &GameIO,
        _resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        // first frame setup
        if simulation.time == 0 {
            use hecs::Without;

            let entities = &mut simulation.entities;
            for (_, (entity, _)) in
                entities.query_mut::<Without<(&mut Entity, &Character), &Player>>()
            {
                if !entity.spawned {
                    continue;
                }

                self.tracked_entities.push_back(entity.id);

                if let Some(sprite_tree) = simulation.sprite_trees.get_mut(entity.sprite_tree_index)
                {
                    let root_node = sprite_tree.root_mut();
                    root_node.set_alpha(0.0);
                }
            }
        }

        // wait for the transition to end
        let transition_frames =
            BATTLE_FADE_DURATION.as_secs_f32() / game_io.target_duration().as_secs_f32();

        if simulation.time < transition_frames.round() as FrameTime {
            return;
        }

        // start music
        if let Some(init_music) = simulation.config.battle_init_music.take() {
            simulation.play_music(game_io, &init_music.buffer, init_music.loops);
        }

        let entities = &mut simulation.entities;

        for (i, id) in self.tracked_entities.iter().cloned().enumerate() {
            let Ok(entity) = entities.query_one_mut::<&mut Entity>(id.into()) else {
                continue;
            };

            let Some(sprite_tree) = simulation.sprite_trees.get_mut(entity.sprite_tree_index)
            else {
                continue;
            };

            let root_node = sprite_tree.root_mut();

            let alpha = if i == 0 {
                let max_time = MAX_ANIMATION_TIME / 2;
                let time = (self.animation_time + 1) / 2;

                time as f32 / max_time as f32
            } else {
                0.0
            };

            root_node.set_shader_effect(if alpha < 1.0 {
                SpriteShaderEffect::Pixelate
            } else {
                SpriteShaderEffect::Default
            });

            root_node.set_alpha(alpha);
        }

        // play a sound if this is the first frame of the entity's introduction
        if self.animation_time == 0 {
            let sfx = &game_io.resource::<Globals>().unwrap().sfx;
            simulation.play_sound(game_io, &sfx.appear);
        }

        self.animation_time += 1;

        // move on to the next entity if we passed the max animation time
        if self.animation_time >= MAX_ANIMATION_TIME {
            self.animation_time = 0;
            self.tracked_entities.pop_front();
        }

        // mark completion if there's no more entities to introduce
        if self.tracked_entities.is_empty() {
            self.completed = true;
            simulation.intro_complete = true;
        }
    }
}

impl IntroState {
    pub fn new() -> Self {
        Self {
            completed: false,
            animation_time: 0,
            tracked_entities: VecDeque::new(),
        }
    }
}
