use super::State;
use crate::bindable::{ActionLockout, ComponentLifetime, EntityId, GenerationalIndex};
use crate::render::{FrameTime, SpriteShaderEffect};
use crate::resources::{Globals, SoundBuffer};
use crate::transitions::BATTLE_FADE_DURATION;
use crate::{battle::*, BATTLE_CAMERA_OFFSET};
use framework::prelude::*;

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
    // (entity_id, action_index)
    tracked_intros: Vec<(EntityId, GenerationalIndex)>,
}

impl State for IntroState {
    fn clone_box(&self) -> Box<dyn State> {
        Box::new(self.clone())
    }

    fn allows_animation_updates(&self) -> bool {
        true
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
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        // first frame setup
        if simulation.time == 0 {
            let entities = &mut simulation.entities;
            let mut entity_ids = Vec::new();

            for (id, (entity, _)) in entities.query_mut::<(&mut Entity, &Character)>() {
                if !entity.spawned {
                    continue;
                }

                entity_ids.push(id);
            }

            // get intros for entities
            // built in reverse to allow us to pop from the end to get the next value
            self.tracked_intros = entity_ids
                .into_iter()
                .rev()
                .flat_map(|id| {
                    let entity_id = id.into();
                    let action_index =
                        self.resolve_intro_action(game_io, resources, simulation, entity_id)?;

                    Some((entity_id, action_index))
                })
                .collect();
        }

        let scale = simulation.field.best_fitting_scale();
        simulation.camera.set_scale(scale);
        simulation.camera.snap(BATTLE_CAMERA_OFFSET);

        // wait for the transition to end
        let transition_frames =
            BATTLE_FADE_DURATION.as_secs_f32() / game_io.target_duration().as_secs_f32();

        match simulation
            .time
            .cmp(&(transition_frames.round() as FrameTime))
        {
            std::cmp::Ordering::Less => return,
            std::cmp::Ordering::Equal => {
                // start music
                if let Some(init_music) = resources.config.borrow().battle_init_music.clone() {
                    simulation.play_music(game_io, resources, &init_music.buffer, init_music.loops);
                }

                self.queue_next_intro(simulation);
            }
            std::cmp::Ordering::Greater => {}
        }

        let entities = &mut simulation.entities;

        // see if the active entity's action queue is empty in order to remove it from our queue
        if let Some((id, _)) = self.tracked_intros.last().cloned() {
            let pop = entities
                .query_one_mut::<&mut ActionQueue>(id.into())
                .map(|queue| queue.active.is_none() && queue.pending.is_empty())
                .unwrap_or(true);

            if pop {
                self.tracked_intros.pop();
                self.queue_next_intro(simulation);
            }
        }

        // mark completion if there's no more entities to introduce
        if self.tracked_intros.is_empty() {
            self.completed = true;

            if simulation.progress < BattleProgress::CompletedIntro {
                simulation.progress = BattleProgress::CompletedIntro;
            }
        }

        Action::process_queues(game_io, resources, simulation);
        Action::process_actions(game_io, resources, simulation);
    }
}

impl IntroState {
    pub fn new() -> Self {
        Self {
            completed: false,
            tracked_intros: Default::default(),
        }
    }

    fn queue_next_intro(&self, simulation: &mut BattleSimulation) {
        let Some(&(entity_id, index)) = self.tracked_intros.last() else {
            return;
        };

        ActionQueue::ensure(&mut simulation.entities, entity_id);
        Action::queue_action(simulation, entity_id, index);
    }

    fn resolve_intro_action(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
    ) -> Option<GenerationalIndex> {
        let entities = &mut simulation.entities;
        let Ok((character, player)) =
            entities.query_one_mut::<(&mut Character, Option<&Player>)>(entity_id.into())
        else {
            return None;
        };

        let is_player = player.is_some();

        // see if the entity provides an intro action
        let intro_callback = character.intro_callback.clone();
        let action_index = intro_callback.call(game_io, resources, simulation, ());

        if let Some(index) = action_index {
            return Some(index);
        }

        if is_player {
            // players have no intro by default
            return None;
        }

        // create the default intro action
        let action_index = Action::create(game_io, simulation, String::new(), entity_id)?;

        // grab the entity to build the intro
        let entities = &mut simulation.entities;
        let Ok(entity) = entities.query_one_mut::<&mut Entity>(entity_id.into()) else {
            return None;
        };

        // hide the entity
        if let Some(sprite_tree) = simulation.sprite_trees.get_mut(entity.sprite_tree_index) {
            let root_node = sprite_tree.root_mut();
            root_node.set_alpha(0.0);
        }

        // pause sprite animations
        let animator_index = entity.animator_index;

        if let Some(animator) = simulation.animators.get_mut(animator_index) {
            animator.disable();
        };

        // build the action
        let action = &mut simulation.actions[action_index];
        action.lockout_type = ActionLockout::Sequence;

        action.execute_callback = Some(BattleCallback::new(move |game_io, _, simulation, _| {
            // play a sound to start
            let sfx = &game_io.resource::<Globals>().unwrap().sfx;
            simulation.play_sound(game_io, &sfx.appear);

            // create action step now that we know our start time
            let start_time = simulation.time;
            let action = &mut simulation.actions[action_index];

            let mut action_step = ActionStep::default();
            action_step.callback = BattleCallback::new(move |_, _, simulation, ()| {
                let Some(action) = &mut simulation.actions.get_mut(action_index) else {
                    return;
                };
                let Some(action_step) = action.steps.get_mut(0) else {
                    return;
                };
                let Ok(entity) = simulation
                    .entities
                    .query_one_mut::<&mut Entity>(action.entity.into())
                else {
                    action_step.completed = true;
                    return;
                };
                let Some(sprite_tree) = simulation.sprite_trees.get_mut(entity.sprite_tree_index)
                else {
                    action_step.completed = true;
                    return;
                };

                let elapsed = simulation.time - start_time;

                let max_time = MAX_ANIMATION_TIME / 2;
                let time = (elapsed + 1) / 2;

                let alpha = time as f32 / max_time as f32;

                let root_sprite = sprite_tree.root_mut();

                if alpha < 1.0 {
                    root_sprite.set_alpha(alpha);
                    root_sprite.set_shader_effect(SpriteShaderEffect::Pixelate)
                } else {
                    root_sprite.set_alpha(1.0);
                    root_sprite.set_shader_effect(SpriteShaderEffect::Default);
                    action_step.completed = true;
                }
            });

            action.steps.push(action_step);
        }));

        // resume sprite animations when battle begins
        let mut component = Component::new(entity_id, ComponentLifetime::Battle);
        component.update_callback = BattleCallback::new(move |_, _, simulation, _| {
            if let Some(animator) = simulation.animators.get_mut(animator_index) {
                animator.enable();
            };
        });

        simulation.components.insert(component);

        Some(action_index)
    }
}
