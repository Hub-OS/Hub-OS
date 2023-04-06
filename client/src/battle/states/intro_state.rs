use super::State;
use crate::battle::*;
use crate::bindable::EntityId;
use crate::render::FrameTime;
use crate::resources::Globals;
use crate::transitions::DEFAULT_FADE_DURATION;
use framework::prelude::*;
use std::collections::VecDeque;

// max time per entity
const MAX_ANIMATION_TIME: FrameTime = 32;

#[derive(Clone)]
pub struct IntroState {
    completed: bool,
    animation_delay: FrameTime,
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
        _shared_assets: &mut SharedBattleAssets,
        simulation: &mut BattleSimulation,
        _vms: &[RollbackVM],
    ) {
        let entities = &mut simulation.entities;

        // first frame setup
        if simulation.time == 0 {
            use hecs::Without;

            for (_, (entity, _)) in
                entities.query_mut::<Without<(&mut Entity, &Character), &Player>>()
            {
                if !entity.spawned {
                    continue;
                }

                self.tracked_entities.push_front(entity.id);

                let root_node = entity.sprite_tree.root_mut();
                root_node.set_alpha(0.0);
            }
        }

        for (i, id) in self.tracked_entities.iter().cloned().enumerate() {
            let Ok(entity) = entities.query_one_mut::<&mut Entity>(id.into()) else {
                continue;
            };

            let root_node = entity.sprite_tree.root_mut();

            let alpha = if i == 0 {
                let max_time = MAX_ANIMATION_TIME / 2;
                let time = (self.animation_time + 1) / 2;

                time as f32 / max_time as f32
            } else {
                0.0
            };

            root_node.set_pixelate_with_alpha(alpha < 1.0);
            root_node.set_alpha(alpha);
        }

        if simulation.time > self.animation_delay {
            if self.animation_time == 0 {
                let appear_sfx = &game_io.resource::<Globals>().unwrap().appear_sfx;
                simulation.play_sound(game_io, appear_sfx);
            }

            self.animation_time += 1;

            if self.animation_time >= MAX_ANIMATION_TIME {
                self.animation_time = 0;
                self.tracked_entities.pop_front();
            }

            if self.tracked_entities.is_empty() {
                self.completed = true;
                simulation.intro_complete = true;
            }
        }
    }
}

impl IntroState {
    pub fn new() -> Self {
        Self {
            completed: false,
            animation_delay: (DEFAULT_FADE_DURATION.as_secs_f32() * 60.0) as FrameTime,
            animation_time: 0,
            tracked_entities: VecDeque::new(),
        }
    }
}
