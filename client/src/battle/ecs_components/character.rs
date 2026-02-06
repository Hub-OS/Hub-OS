use super::{Artifact, HpDisplay, Living, Player};
use crate::battle::{
    Action, ActionQueue, ActionType, BattleCallback, BattleSimulation, BattleState,
    CanMoveToCallback, DeleteCallback, Entity, Movement, SharedBattleResources, TileState,
};
use crate::bindable::*;
use crate::lua_api::create_entity_table;
use crate::packages::PackageNamespace;
use crate::render::ui::{FontName, TextStyle};
use crate::render::{FrameTime, SpriteColorQueue};
use crate::resources::{BATTLE_INFO_SHADOW_COLOR, RESOLUTION_F};
use framework::prelude::{GameIO, Vec2};
use packets::structures::PackageId;

#[derive(Clone)]
pub struct Character {
    pub rank: CharacterRank,
    pub cards: Vec<CardProperties>, // stores cards reversed
    pub card_use_requested: bool,
    pub next_card_mutation: Option<usize>, // stores card index, invert to get a usable index
}

impl Character {
    fn new(rank: CharacterRank) -> Self {
        Self {
            rank,
            cards: Vec::new(),
            card_use_requested: false,
            next_card_mutation: None,
        }
    }

    pub fn create(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        rank: CharacterRank,
        namespace: PackageNamespace,
    ) -> rollback_mlua::Result<EntityId> {
        let id = Entity::create(game_io, simulation);

        let can_move_to_callback = BattleCallback::new(move |_, resources, simulation, dest| {
            let Some(tile) = simulation.field.tile_at_mut(dest) else {
                return false;
            };

            if tile.state_index() == TileState::VOID {
                // can't walk on void tiles, even with ignore_hole_tiles
                return false;
            }

            let entities = &mut simulation.entities;
            let Ok((entity, living)) = entities.query_one_mut::<(&Entity, &Living)>(id.into())
            else {
                return false;
            };

            if !entity.ignore_hole_tiles && simulation.tile_states[tile.state_index()].is_hole {
                // can't walk on holes
                return false;
            }

            if tile.team() != entity.team && tile.team() != Team::Other {
                // tile can't belong to the opponent team
                return false;
            }

            let status_registry = &resources.status_registry;

            if living.status_director.is_immobile(status_registry) {
                // can't move while immobilized, useful feedback for scripts
                return false;
            }

            if entity.share_tile {
                return true;
            }

            for entity_id in tile.reservations().iter().cloned() {
                if entity_id == id {
                    continue;
                }

                let entities = &mut simulation.entities;
                let Ok(other_entity) = entities.query_one_mut::<&Entity>(entity_id.into()) else {
                    // non entity or erased is reserving?
                    continue;
                };

                if !other_entity.share_tile {
                    // another entity is reserving this tile and refusing to share
                    return false;
                }
            }

            true
        });

        let scripts = &resources.vm_manager.scripts;
        let delete_callback = scripts.default_character_delete.clone().bind((id, None));

        let components = (
            Character::new(rank),
            Living::default(),
            HpDisplay::default(),
            CanMoveToCallback(can_move_to_callback),
            DeleteCallback(delete_callback),
            namespace,
        );

        simulation.entities.insert(id.into(), components).unwrap();

        let (entity, living) = simulation
            .entities
            .query_one_mut::<(&mut Entity, &mut Living)>(id.into())
            .unwrap();

        // characters should own their tiles by default
        entity.share_tile = false;
        entity.auto_reserves_tiles = true;

        let elemental_weakness_aux_prop = AuxProp::new()
            .with_requirement(AuxRequirement::HitDamage(Comparison::GT, 0))
            .with_requirement(AuxRequirement::HitElementIsWeakness)
            .with_requirement(AuxRequirement::HitFlagsAbsent(HitFlag::DRAIN))
            .with_callback(BattleCallback::new(move |game_io, _, simulation, _| {
                let entities = &mut simulation.entities;
                let entity = entities.query_one_mut::<&Entity>(id.into()).unwrap();

                // spawn alert artifact
                let mut alert_position = entity.full_position();
                alert_position.offset += Vec2::new(0.0, -entity.height);

                let alert_id = Artifact::create_alert(game_io, simulation);
                let alert_entity = simulation
                    .entities
                    .query_one_mut::<&mut Entity>(alert_id.into())
                    .unwrap();

                alert_entity.copy_full_position(alert_position);
                alert_entity.pending_spawn = true;
            }));

        living.add_aux_prop(elemental_weakness_aux_prop);

        Ok(id)
    }

    pub fn load(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        package_id: &PackageId,
        namespace: PackageNamespace,
        rank: CharacterRank,
    ) -> rollback_mlua::Result<EntityId> {
        let id = Self::create(game_io, resources, simulation, rank, namespace)?;

        let vm_index = resources.vm_manager.find_vm(package_id, namespace)?;
        simulation.call_global(game_io, resources, vm_index, "character_init", move |lua| {
            create_entity_table(lua, id)
        })?;

        Ok(id)
    }

    fn apply_aux_card_boosts(simulation: &mut BattleSimulation, entity_id: EntityId) {
        let entities = &mut simulation.entities;
        let (character, living) = entities
            .query_one_mut::<(&mut Character, &mut Living)>(entity_id.into())
            .unwrap();

        let Some(card_properties) = character.cards.last_mut() else {
            return;
        };

        // update card with multipliers
        let callbacks = living.modify_used_card(card_properties);
        simulation.pending_callbacks.extend(callbacks);
    }

    pub fn use_card(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
    ) {
        Self::apply_aux_card_boosts(simulation, entity_id);

        let entities = &mut simulation.entities;
        let is_player = entities
            .satisfies::<&Player>(entity_id.into())
            .unwrap_or(false);

        if is_player && !simulation.time_freeze_tracker.time_is_frozen() {
            Player::use_card(game_io, resources, simulation, entity_id);
            return;
        }

        Living::update_action_context(game_io, resources, simulation, ActionType::CARD, entity_id);

        let entities = &mut simulation.entities;
        let (character, namespace) = entities
            .query_one_mut::<(&mut Character, &PackageNamespace)>(entity_id.into())
            .unwrap();

        // create card action
        let namespace = *namespace;
        let card_props = character.cards.pop().unwrap();
        let action_index = Action::create_from_card_properties(
            game_io, resources, simulation, entity_id, namespace, card_props,
        );

        // spawn a poof if there's no action
        let Some(index) = action_index else {
            Artifact::create_card_poof(game_io, simulation, entity_id);
            return;
        };

        Action::queue_action(game_io, resources, simulation, entity_id, index);
    }

    pub fn mutate_cards(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        loop {
            let entities = &mut simulation.entities;
            let mut character_iter = entities
                .query_mut::<(&mut Character, &PackageNamespace)>()
                .into_iter();

            let Some((id, (character, namespace))) =
                character_iter.find(|(_, (character, _))| character.next_card_mutation.is_some())
            else {
                break;
            };

            // get next card index
            let lua_card_index = character.next_card_mutation.unwrap();

            // update the card index
            character.next_card_mutation = Some(lua_card_index + 1);

            let card_index = character.invert_card_index(lua_card_index);

            // get the card or mark the mutate state as complete
            let Some(card) = &character.cards.get(card_index) else {
                character.next_card_mutation = None;
                continue;
            };

            // make sure the vm + callback exists before calling
            // avoids an error message from simulation.call_global()
            // function card_mutate is optional to implement
            let vm_manager = &resources.vm_manager;
            let Ok(vm_index) = vm_manager.find_vm(&card.package_id, *namespace) else {
                continue;
            };

            let lua = &vm_manager.vms()[vm_index].lua;
            let callback_exists = lua.globals().contains_key("card_mutate").unwrap_or(false);

            if !callback_exists {
                continue;
            }

            let _ = simulation.call_global(game_io, resources, vm_index, "card_mutate", |lua| {
                Ok((create_entity_table(lua, id.into())?, lua_card_index + 1))
            });
        }
    }

    pub fn invert_card_index(&self, index: usize) -> usize {
        self.cards.len().wrapping_sub(index).wrapping_sub(1)
    }

    pub fn process_card_requests(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        state_time: FrameTime,
    ) {
        if simulation.time_freeze_tracker.time_is_frozen() {
            // TFCs are handled in the time freeze tracker

            // prevent awkward chip usage after time freeze by cancelling requests
            let entities = &mut simulation.entities;
            for (_, character) in entities.query_mut::<&mut Character>() {
                if character.card_use_requested {
                    character.card_use_requested = false;
                }
            }

            return;
        }

        // ensure initial test values for all card use aux props
        for (_, living) in simulation.entities.query_mut::<&mut Living>() {
            for aux_prop in living.aux_props.values_mut() {
                if aux_prop.effect().executes_on_card_use() {
                    aux_prop.process_card(None);
                }
            }
        }

        if state_time > BattleState::GRACE_TIME {
            let mut requesters = Vec::new();
            let mut time_freeze_requested = false;

            let entities = &mut simulation.entities;

            // wait until movement ends before adding a card action
            // this is to prevent time freeze cards from applying during movement
            // process_action_queues only prevents non time freeze actions from starting until movements end
            type Query<'a> = hecs::Without<&'a mut Character, &'a Movement>;

            for (id, character) in entities.query_mut::<Query>() {
                if !character.card_use_requested {
                    continue;
                }

                let entity_id = id.into();
                let action_processed = (simulation.actions)
                    .values()
                    .any(|action| action.processed && action.entity == entity_id);

                if action_processed {
                    continue;
                }

                if let Some(card) = character.cards.last() {
                    if time_freeze_requested && card.time_freeze {
                        // ignore request if someone else queued a time freeze chip
                        // we'll keep the request in mind in case something happens to the chip
                        // but if time freeze starts, the request will be dropped (along with all other requests)
                        continue;
                    }

                    time_freeze_requested |= card.time_freeze;
                }

                character.card_use_requested = false;
                requesters.push(entity_id);
            }

            for requester in requesters {
                Self::use_card(game_io, resources, simulation, requester);
            }
        }

        // clean up card use aux props
        Living::aux_prop_cleanup(simulation, |aux_prop| {
            aux_prop.effect().executes_on_card_use()
        });

        simulation.call_pending_callbacks(game_io, resources);
    }

    pub fn draw_top_card_ui(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        sprite_queue: &mut SpriteColorQueue,
    ) {
        type Query<'a> = (&'a mut Character, &'a Living, Option<&'a ActionQueue>);

        let entities = &mut simulation.entities;
        let Ok((character, living, action_queue)) =
            entities.query_one_mut::<Query>(entity_id.into())
        else {
            return;
        };

        // only render if there's no processed or pending actions
        let mut actions_iter = simulation.actions.iter();
        let action_processed =
            actions_iter.any(|(_, action)| action.entity == entity_id && action.processed);

        if action_processed || action_queue.is_some_and(|action| !action.pending.is_empty()) {
            return;
        }

        let Some(card_props) = character.cards.last_mut() else {
            return;
        };

        // render on the bottom left
        const MARGIN: Vec2 = Vec2::new(1.0, -1.0);

        let line_height = TextStyle::new(game_io, FontName::Thick).line_height();
        let position = Vec2::new(0.0, RESOLUTION_F.y - line_height) + MARGIN;

        // apply aux damage
        let aux_damage = living.predict_card_aux_damage(card_props);
        card_props.damage += aux_damage;
        card_props.boosted_damage += aux_damage;

        let mut text_style =
            card_props.draw_summary(game_io, sprite_queue, position, Vec2::ONE, false);

        // draw card multiplier
        let card_multiplier = (living.predict_card_multiplier(card_props) * 100.0).trunc() / 100.0;

        if card_multiplier != 1.0 {
            let multiplier_string = format!("\u{00D7}{card_multiplier}");

            text_style.shadow_color = BATTLE_INFO_SHADOW_COLOR;
            text_style.font = FontName::Thick;
            text_style.draw(game_io, sprite_queue, &multiplier_string);
        }

        // revert aux damage
        card_props.damage -= aux_damage;
        card_props.boosted_damage -= aux_damage;
    }
}
