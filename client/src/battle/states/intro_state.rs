use super::State;
use crate::bindable::{ActionLockout, ComponentLifetime, EntityId, GenerationalIndex};
use crate::render::{FrameTime, SpriteShaderEffect};
use crate::resources::{Globals, SoundBuffer};
use crate::transitions::BATTLE_FADE_DURATION;
use crate::{BATTLE_CAMERA_OFFSET, battle::*};
use framework::prelude::*;
use rand::seq::SliceRandom;

#[derive(Clone)]
pub struct BattleInitMusic {
    pub buffer: SoundBuffer,
    pub loops: bool,
}

const PLAYER_ANIMATION_OFFSET: FrameTime = 16;

// default time per entity
const DEFAULT_ANIMATION_TIME: FrameTime = 32;

#[derive(Clone)]
pub struct IntroState {
    completed: bool,
    // (entity_id, action_index)
    pending_intros: Vec<(EntityId, GenerationalIndex)>,
    blocking_entities: Vec<EntityId>,
    // npcs animate in sequence, players animate concurrently
    npcs_remaining: usize,
    next_player_anim: FrameTime,
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
            let mut player_count = 0;

            for (id, (entity, _, player)) in
                entities.query_mut::<(&mut Entity, &Character, Option<&Player>)>()
            {
                if !entity.spawned {
                    continue;
                }

                entity_ids.push((id, player.is_some()));
            }

            // move players to the end to animate last
            entity_ids.sort_by_cached_key(|&(_, is_player)| is_player);

            // get intros for entities
            // built in reverse to allow us to pop from the end to get the next value
            self.pending_intros = entity_ids
                .into_iter()
                .rev()
                .flat_map(|(id, is_player)| {
                    let entity_id = id.into();
                    let action_index =
                        self.resolve_intro_action(game_io, resources, simulation, entity_id)?;

                    if is_player {
                        player_count += 1;
                    }

                    Some((entity_id, action_index))
                })
                .collect();

            self.npcs_remaining = self.pending_intros.len() - player_count;

            // shuffle players
            self.pending_intros[self.npcs_remaining..].shuffle(&mut simulation.rng);
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

                self.queue_next_intro(game_io, resources, simulation);

                if self.npcs_remaining == 0 {
                    self.next_player_anim = PLAYER_ANIMATION_OFFSET;
                }
            }
            std::cmp::Ordering::Greater => {}
        }

        let entities = &mut simulation.entities;

        // resolve which entities are still blocking intros from advancing
        self.blocking_entities.retain(|&id| {
            entities
                .query_one_mut::<&mut ActionQueue>(id.into())
                .map(|queue| queue.active.is_some() || !queue.pending.is_empty())
                .unwrap_or(false)
        });

        // try to queue the next intro if we need to
        if self.npcs_remaining > 0 {
            if self.blocking_entities.is_empty() {
                self.npcs_remaining -= 1;
                self.queue_next_intro(game_io, resources, simulation);
            }
        } else {
            self.next_player_anim -= 1;

            if self.next_player_anim <= 0 {
                self.queue_next_intro(game_io, resources, simulation);
                self.next_player_anim = PLAYER_ANIMATION_OFFSET;
            }
        }

        // mark completion if there's no more entities to introduce
        if self.blocking_entities.is_empty() && self.pending_intros.is_empty() {
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
            pending_intros: Default::default(),
            blocking_entities: Default::default(),
            npcs_remaining: 0,
            next_player_anim: 0,
        }
    }

    fn queue_next_intro(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let Some((entity_id, index)) = self.pending_intros.pop() else {
            return;
        };

        self.blocking_entities.push(entity_id);
        ActionQueue::ensure(&mut simulation.entities, entity_id);
        Action::queue_action(game_io, resources, simulation, entity_id, index);
    }

    fn resolve_intro_action(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
    ) -> Option<GenerationalIndex> {
        let entities = &mut simulation.entities;
        let Ok((player, intro_callback)) =
            entities.query_one_mut::<(Option<&Player>, Option<&IntroCallback>)>(entity_id.into())
        else {
            return None;
        };

        let is_player = player.is_some();

        // see if the entity provides an intro action
        if let Some(callback) = intro_callback {
            let callback = callback.0.clone();
            let action_index = callback.call(game_io, resources, simulation, ());

            if let Some(index) = action_index {
                return Some(index);
            }
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
            let sfx = &Globals::from_resources(game_io).sfx;
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

                let max_time = DEFAULT_ANIMATION_TIME / 2;
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
