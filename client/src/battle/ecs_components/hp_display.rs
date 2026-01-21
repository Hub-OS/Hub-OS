use super::{Character, Entity, Living, Player};
use crate::battle::{ActionQueue, BattleSimulation, EmotionWindow, TileState};
use crate::bindable::{AuxEffect, AuxPropBodyParams};

#[derive(Default, Clone)]
pub struct HpDisplay {
    pub value: i32,
    pub last_value: i32,
    pub initialized: bool,
}

impl HpDisplay {
    pub fn update(simulation: &mut BattleSimulation) {
        let entities = &mut simulation.entities;

        type Query<'a> = (
            &'a mut HpDisplay,
            &'a Living,
            &'a Entity,
            Option<&'a Character>,
            Option<&'a Player>,
            Option<&'a EmotionWindow>,
            Option<&'a ActionQueue>,
        );

        for (_, (hp_display, living, entity, character, player, emotion_window, action_queue)) in
            entities.query_mut::<Query>()
        {
            if !hp_display.initialized {
                hp_display.value = living.health;
                hp_display.last_value = living.health;
                hp_display.initialized = true;
                continue;
            }

            let diff = living.health - hp_display.value;

            // diff + change: https://www.desmos.com/calculator/nd0vqxf6ye
            // raw data: https://www.desmos.com/calculator/odwgooh1bl
            // x = diff, y = change
            let y = match diff.abs() {
                x @ 2.. => x / 8 + 2,
                x => x,
            };

            hp_display.last_value = hp_display.value;
            hp_display.value += y * diff.signum();

            if hp_display.last_value != hp_display.value {
                continue;
            }

            // special test to avoid flickering with poison like effects
            let body_params = AuxPropBodyParams {
                emotion_window,
                status_director: &living.status_director,
                player,
                character,
                entity,
                action_queue,
                tile_state: simulation
                    .field
                    .tile_at_mut((entity.x, entity.y))
                    .map(|tile| tile.state_index())
                    .unwrap_or(TileState::VOID),
            };

            let has_drain = living
                .aux_props
                .iter()
                .filter(|(_, aux_prop)| matches!(aux_prop.effect(), AuxEffect::DrainHP(_)))
                .any(|(_, aux_prop)| {
                    // clone to avoid modifying the original
                    let mut aux_prop = aux_prop.clone();
                    aux_prop.process_body(&body_params);
                    aux_prop.process_health_calculations(living.health, living.max_health, 0);

                    aux_prop.passed_non_time_tests()
                });

            if has_drain {
                hp_display.last_value = hp_display.value + 1;
            }
        }
    }
}
