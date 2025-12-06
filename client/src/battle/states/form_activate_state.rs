use super::{State, TurnStartState};
use crate::battle::{
    Action, Artifact, BattleSimulation, Entity, Living, Movement, Player, SharedBattleResources,
};
use crate::bindable::{EntityId, HitFlag, SpriteColorMode};
use crate::ease::inverse_lerp;
use crate::render::FrameTime;
use crate::resources::Globals;
use framework::prelude::{Color, GameIO, Vec2};

const FADE_TIME: FrameTime = 15;

#[derive(Clone)]
pub struct FormActivateState {
    time: FrameTime,
    target_complete_time: Option<FrameTime>,
    artifact_entities: Vec<EntityId>,
    completed: bool,
}

impl State for FormActivateState {
    fn clone_box(&self) -> Box<dyn State> {
        Box::new(self.clone())
    }

    fn next_state(&self, _game_io: &GameIO) -> Option<Box<dyn State>> {
        if self.completed {
            Some(Box::new(TurnStartState::new()))
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
        if self.time == 0 && !has_pending_activations(simulation) {
            self.completed = true;
            return;
        }

        if self.target_complete_time == Some(self.time) {
            self.mark_activated(simulation);
            self.completed = true;
            return;
        }

        // update fade
        let alpha = match self.target_complete_time {
            Some(time) => inverse_lerp!(time, time - FADE_TIME, self.time),
            None => inverse_lerp!(0, FADE_TIME, self.time),
        };

        let fade_color = Color::BLACK.multiply_alpha(alpha);
        resources.battle_fade_color.set(fade_color);

        // logic
        if self.target_complete_time.is_none() {
            match self.time {
                // flash white and activate forms
                15 => {
                    set_relevant_color(simulation, Color::WHITE);
                    self.activate_forms(game_io, resources, simulation);
                }
                // flash white for 9 more frames
                16..=25 => set_relevant_color(simulation, Color::WHITE),
                // reset color
                26 => set_relevant_color(simulation, Color::BLACK),
                // wait for the shine artifacts to complete animation
                27.. => self.detect_animation_end(game_io, resources, simulation),
                _ => {}
            }
        }

        self.time += 1;
    }
}

impl FormActivateState {
    pub fn new() -> Self {
        Self {
            time: 0,
            target_complete_time: None,
            artifact_entities: Vec::new(),
            completed: false,
        }
    }

    fn activate_forms(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let mut activated = Vec::new();

        type Query<'a> = (&'a mut Entity, &'a mut Living, &'a mut Player);

        // deactivate previous forms, and activate new forms
        for (id, (entity, living, player)) in simulation.entities.query_mut::<Query>() {
            let Some(index) = player.active_form else {
                continue;
            };

            if player.forms[index].activated {
                continue;
            }

            activated.push(id);

            // clear statuses
            living.status_director.clear_statuses(HitFlag::ALL);

            // deactivate previous forms
            for form in &player.forms {
                if !form.activated || form.deactivated {
                    continue;
                }

                if let Some(callback) = form.deactivate_callback.clone() {
                    simulation.pending_callbacks.push(callback);
                }
            }

            // activate new form
            let form = &player.forms[index];

            if let Some(callback) = form.activate_callback.clone() {
                simulation.pending_callbacks.push(callback);
            }

            // cancel charge
            player.cancel_charge();

            if let Some(sprite_tree) = simulation.sprite_trees.get_mut(entity.sprite_tree_index) {
                player.card_charge.update_sprite(sprite_tree);
                player.attack_charge.update_sprite(sprite_tree);
            }
        }

        // cancel movement and actions
        for id in activated {
            Movement::cancel(simulation, id.into());
            Action::cancel_all(game_io, resources, simulation, id.into());
        }

        simulation.call_pending_callbacks(game_io, resources);

        self.spawn_shine(game_io, simulation);
    }

    fn spawn_shine(&mut self, game_io: &GameIO, simulation: &mut BattleSimulation) {
        let mut relevant_ids = Vec::new();

        for (id, player) in simulation.entities.query_mut::<&mut Player>() {
            let Some(index) = player.active_form else {
                continue;
            };

            let form = &mut player.forms[index];

            // allow activated form to reactivate
            if form.deactivated {
                form.activated = false;
                form.deactivated = false;
            }

            if form.activated {
                continue;
            }

            relevant_ids.push(id);
        }

        for id in relevant_ids {
            let Ok(entity) = simulation.entities.query_one_mut::<&Entity>(id) else {
                continue;
            };

            let mut full_position = entity.full_position();
            full_position.offset += Vec2::new(0.0, -entity.height * 0.5);

            let shine_id = Artifact::create_transformation_shine(game_io, simulation);
            let shine_entity = simulation
                .entities
                .query_one_mut::<&mut Entity>(shine_id.into())
                .unwrap();

            shine_entity.copy_full_position(full_position);
            shine_entity.pending_spawn = true;

            self.artifact_entities.push(shine_id);
        }

        // play sfx
        let globals = game_io.resource::<Globals>().unwrap();
        simulation.play_sound(game_io, &globals.sfx.shine);
        simulation.play_sound(game_io, &globals.sfx.form_activate);
    }

    fn detect_animation_end(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        for id in self.artifact_entities.iter().cloned() {
            let Ok(entity) = simulation.entities.query_one_mut::<&mut Entity>(id.into()) else {
                continue;
            };

            // force update, since animations are only automatic in BattleState
            let animator = &mut simulation.animators[entity.animator_index];
            let callbacks = animator.update();
            simulation.pending_callbacks.extend(callbacks);
        }

        simulation.call_pending_callbacks(game_io, resources);

        let all_erased = self
            .artifact_entities
            .iter()
            .all(|id| !simulation.entities.contains((*id).into()));

        if !all_erased {
            return;
        }

        self.target_complete_time = Some(self.time + FADE_TIME);
    }

    fn mark_activated(&mut self, simulation: &mut BattleSimulation) {
        for (_, player) in simulation.entities.query_mut::<&mut Player>() {
            let Some(index) = player.active_form else {
                continue;
            };

            if player.forms[index].activated {
                continue;
            }

            for form in &mut player.forms {
                if form.activated {
                    form.deactivated = true;
                }
            }

            let form = &mut player.forms[index];
            form.activated = true;
            form.deactivated = false;
        }
    }
}

fn has_pending_activations(simulation: &mut BattleSimulation) -> bool {
    for (_, player) in simulation.entities.query_mut::<&Player>() {
        let Some(index) = player.active_form else {
            continue;
        };

        if !player.forms[index].activated {
            return true;
        }
    }

    false
}

fn set_relevant_color(simulation: &mut BattleSimulation, color: Color) {
    let entities = &mut simulation.entities;

    for (_, (entity, player)) in entities.query_mut::<(&mut Entity, &Player)>() {
        let Some(index) = player.active_form else {
            continue;
        };

        if player.forms[index].activated {
            continue;
        }

        if let Some(sprite_tree) = simulation.sprite_trees.get_mut(entity.sprite_tree_index) {
            let root_node = sprite_tree.root_mut();
            root_node.set_color_mode(SpriteColorMode::Add);
            root_node.set_color(color);
        }
    }
}
