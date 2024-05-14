use crate::battle::*;
use crate::bindable::*;
use crate::render::*;
use crate::structures::SlotMap;
use framework::prelude::*;
use std::collections::VecDeque;

#[derive(Clone, Copy)]
pub struct FullEntityPosition {
    pub tile_position: IVec2,
    pub offset: Vec2,
    pub movement_offset: Vec2,
}

#[derive(Clone)]
pub struct Entity {
    pub updated: bool,
    pub pending_spawn: bool,
    pub spawned: bool,
    pub on_field: bool,
    pub id: EntityId,
    pub name: String,
    pub team: Team,
    pub element: Element,
    pub deleted: bool,
    pub erased: bool,
    pub facing: Direction,
    pub share_tile: bool,
    pub auto_reserves_tiles: bool, // should be set and never change after spawning
    pub x: i32,
    pub y: i32,
    pub height: f32,
    pub elevation: f32,
    pub animator_index: GenerationalIndex,
    pub sprite_tree_index: GenerationalIndex,
    pub offset: Vec2,          // does not flip with teams, only perspective
    pub movement_offset: Vec2, // resets every frame, does not flip with teams, only perspective
    pub hit_context: HitContext,
    pub time_frozen: bool,
    pub ignore_hole_tiles: bool,
    pub ignore_negative_tile_effects: bool,
    pub movement: Option<Movement>,
    pub action_queue: VecDeque<GenerationalIndex>,
    pub action_index: Option<GenerationalIndex>,
    pub local_components: Vec<GenerationalIndex>,
    pub can_move_to_callback: BattleCallback<(i32, i32), bool>,
    pub spawn_callback: BattleCallback,
    pub update_callback: BattleCallback,
    pub idle_callback: BattleCallback,
    pub counter_callback: BattleCallback<EntityId>,
    pub battle_start_callback: BattleCallback,
    pub battle_end_callback: BattleCallback,
    pub delete_callback: BattleCallback,
    pub delete_callbacks: Vec<BattleCallback>,
}

impl Entity {
    pub fn create(game_io: &GameIO, simulation: &mut BattleSimulation) -> EntityId {
        let id: EntityId = Self::reserve(simulation);

        // create sprite tree
        let sprite_tree = Tree::new(SpriteNode::new(game_io, SpriteColorMode::Add));
        let sprite_tree_index = simulation.sprite_trees.insert(sprite_tree);

        // create animator
        let mut animator = BattleAnimator::new();
        animator.set_target(sprite_tree_index, GenerationalIndex::tree_root());
        animator.disable();

        let animator_index = simulation.animators.insert(animator);

        let entity = Self {
            updated: false,
            pending_spawn: false,
            spawned: false,
            on_field: false,
            name: String::new(),
            id,
            team: Team::Unset,
            element: Element::None,
            deleted: false,
            erased: false,
            facing: Direction::None,
            share_tile: true,
            auto_reserves_tiles: false,
            x: 0,
            y: 0,
            height: 0.0,
            elevation: 0.0,
            animator_index,
            sprite_tree_index,
            offset: Vec2::ZERO,
            movement_offset: Vec2::ZERO,
            hit_context: HitContext {
                aggressor: id,
                flags: HitFlag::NONE,
            },
            time_frozen: false,
            ignore_hole_tiles: false,
            ignore_negative_tile_effects: false,
            movement: None,
            action_queue: VecDeque::new(),
            action_index: None,
            local_components: Vec::new(),
            can_move_to_callback: BattleCallback::stub(false),
            update_callback: BattleCallback::stub(()),
            idle_callback: BattleCallback::stub(()),
            spawn_callback: BattleCallback::stub(()),
            battle_start_callback: BattleCallback::stub(()),
            battle_end_callback: BattleCallback::stub(()), // todo:
            counter_callback: BattleCallback::stub(()),
            delete_callback: BattleCallback::new(move |game_io, resources, simulation, _| {
                // default behavior, just erase
                Entity::mark_erased(game_io, resources, simulation, id);
            }),
            delete_callbacks: Vec::new(),
        };

        simulation.entities.spawn_at(id.into(), (entity,));

        id
    }

    fn reserve(simulation: &mut BattleSimulation) -> EntityId {
        let id = simulation.entities.reserve_entity();

        let match_id = |stored: &&mut hecs::Entity| stored.id() == id.id();

        if let Some(stored) = simulation.generation_tracking.iter_mut().find(match_id) {
            *stored = id;
        } else {
            simulation.generation_tracking.push(id);
        }

        id.into()
    }

    pub fn can_move_to(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        position: (i32, i32),
    ) -> bool {
        let entities = &mut simulation.entities;
        let Ok(entity) = entities.query_one_mut::<&Entity>(entity_id.into()) else {
            return false;
        };

        let entity_callback = entity.can_move_to_callback.clone();
        let Some(index) = entity.action_index else {
            return entity_callback.call(game_io, resources, simulation, position);
        };

        let action = &simulation.actions[index];

        let Some(action_callback) = action.can_move_to_callback.clone() else {
            return entity_callback.call(game_io, resources, simulation, position);
        };

        action_callback.call(game_io, resources, simulation, position)
    }

    pub fn full_position(&self) -> FullEntityPosition {
        FullEntityPosition {
            tile_position: IVec2::new(self.x, self.y),
            offset: self.offset,
            movement_offset: self.movement_offset,
        }
    }

    pub fn copy_full_position(&mut self, full_position: FullEntityPosition) {
        self.x = full_position.tile_position.x;
        self.y = full_position.tile_position.y;
        self.offset = full_position.offset;
        self.movement_offset = full_position.movement_offset;
    }

    pub fn sort_key(&self, sprite_trees: &SlotMap<Tree<SpriteNode>>) -> (i32, i32, i32, i32) {
        let Some(sprite_tree) = sprite_trees.get(self.sprite_tree_index) else {
            return (self.y, self.movement_offset.y as _, 0, self.x);
        };

        (
            self.y,
            self.movement_offset.y as _,
            -sprite_tree.root().layer(),
            self.x,
        )
    }

    pub fn flipped(&self) -> bool {
        self.facing == Direction::Left
    }

    pub fn corrected_offset(&self, perspective_flipped: bool) -> Vec2 {
        let mut entity_offset = self.movement_offset + self.offset;

        if perspective_flipped {
            entity_offset.x *= -1.0;
        }

        entity_offset
    }

    pub fn screen_position(&self, field: &Field, perspective_flipped: bool) -> Vec2 {
        let offset = self.corrected_offset(perspective_flipped);
        let tile_center = field.calc_tile_center((self.x, self.y), perspective_flipped);
        tile_center + offset
    }

    pub fn delete(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        id: EntityId,
    ) {
        let Ok(entity) = simulation.entities.query_one_mut::<&mut Entity>(id.into()) else {
            return;
        };

        if entity.deleted {
            return;
        }

        let card_indices: Vec<_> = (simulation.actions)
            .iter()
            .filter(|(_, action)| action.entity == id && action.processed)
            .map(|(index, _)| index)
            .collect();

        entity.deleted = true;

        let listener_callbacks = std::mem::take(&mut entity.delete_callbacks);
        let delete_callback = entity.delete_callback.clone();

        // delete player augments
        if let Ok(player) = simulation.entities.query_one_mut::<&mut Player>(id.into()) {
            let augment_iter = player.augments.values();
            let augment_callbacks =
                augment_iter.flat_map(|augment| augment.delete_callback.clone());

            simulation.pending_callbacks.extend(augment_callbacks);
            simulation.call_pending_callbacks(game_io, resources);
        }

        // delete card actions
        Action::delete_multi(game_io, resources, simulation, card_indices);

        // call delete callbacks after
        simulation.pending_callbacks.extend(listener_callbacks);
        simulation.pending_callbacks.push(delete_callback);

        simulation.call_pending_callbacks(game_io, resources);
    }

    pub fn mark_erased(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        id: EntityId,
    ) {
        let Ok(entity) = simulation.entities.query_one_mut::<&mut Entity>(id.into()) else {
            return;
        };

        if entity.erased {
            return;
        }

        // clear the delete callback
        entity.delete_callback = BattleCallback::default();

        // mark as erased
        entity.erased = true;

        // delete
        Entity::delete(game_io, resources, simulation, id);
    }
}
