use crate::battle::*;
use crate::bindable::*;
use crate::render::*;
use crate::structures::DenseSlotMap;
use framework::prelude::*;
use std::collections::VecDeque;

#[derive(Clone, Copy)]
pub struct FullEntityPosition {
    pub tile_position: IVec2,
    pub offset: Vec2,
    pub tile_offset: Vec2,
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
    pub sprite_tree: Tree<SpriteNode>,
    pub shadow_index: TreeIndex,
    pub offset: Vec2,      // does not flip with teams, only perspective
    pub tile_offset: Vec2, // resets every frame, does not flip with teams, only perspective
    pub hit_context: HitContext,
    pub time_frozen_count: usize,
    pub ignore_hole_tiles: bool,
    pub ignore_negative_tile_effects: bool,
    pub movement: Option<Movement>,
    pub move_anim_state: Option<String>,
    pub last_movement_time: FrameTime, // stores the simulation.battle_time for the last movement update, excluding endlag
    pub action_queue: VecDeque<GenerationalIndex>,
    pub action_index: Option<GenerationalIndex>,
    pub local_components: Vec<GenerationalIndex>,
    pub can_move_to_callback: BattleCallback<(i32, i32), bool>,
    pub spawn_callback: BattleCallback,
    pub update_callback: BattleCallback,
    pub counter_callback: BattleCallback<EntityId>,
    pub battle_start_callback: BattleCallback,
    pub battle_end_callback: BattleCallback,
    pub delete_callback: BattleCallback,
    pub delete_callbacks: Vec<BattleCallback>,
}

impl Entity {
    fn new(game_io: &GameIO, id: EntityId, animator_index: GenerationalIndex) -> Self {
        let mut sprite_tree = Tree::new(SpriteNode::new(game_io, SpriteColorMode::Add));

        let mut shadow_node = SpriteNode::new(game_io, SpriteColorMode::Add);
        shadow_node.set_visible(false);
        shadow_node.set_layer(i32::MAX);
        let shadow_index = sprite_tree.insert_root_child(shadow_node);

        Self {
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
            sprite_tree,
            shadow_index,
            offset: Vec2::ZERO,
            tile_offset: Vec2::ZERO,
            hit_context: HitContext {
                aggressor: id,
                flags: HitFlag::NONE,
            },
            time_frozen_count: 0,
            ignore_hole_tiles: false,
            ignore_negative_tile_effects: false,
            movement: None,
            move_anim_state: None,
            last_movement_time: 0,
            action_queue: VecDeque::new(),
            action_index: None,
            local_components: Vec::new(),
            can_move_to_callback: BattleCallback::stub(false),
            update_callback: BattleCallback::stub(()),
            spawn_callback: BattleCallback::stub(()),
            battle_start_callback: BattleCallback::stub(()),
            battle_end_callback: BattleCallback::stub(()), // todo:
            counter_callback: BattleCallback::stub(()),
            delete_callback: BattleCallback::new(move |game_io, resources, simulation, _| {
                // default behavior, just erase
                simulation.mark_entity_for_erasure(game_io, resources, id);
            }),
            delete_callbacks: Vec::new(),
        }
    }

    pub fn create(game_io: &GameIO, simulation: &mut BattleSimulation) -> EntityId {
        let id: EntityId = Self::reserve(simulation);

        let mut animator = BattleAnimator::new();
        animator.set_target(id, GenerationalIndex::tree_root());
        animator.disable();

        let animator_index = simulation.animators.insert(animator);

        simulation
            .entities
            .spawn_at(id.into(), (Entity::new(game_io, id, animator_index),));

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

    pub fn full_position(&self) -> FullEntityPosition {
        FullEntityPosition {
            tile_position: IVec2::new(self.x, self.y),
            offset: self.offset,
            tile_offset: self.tile_offset,
        }
    }

    pub fn copy_full_position(&mut self, full_position: FullEntityPosition) {
        self.x = full_position.tile_position.x;
        self.y = full_position.tile_position.y;
        self.offset = full_position.offset;
        self.tile_offset = full_position.tile_offset;
    }

    pub fn set_shadow(&mut self, game_io: &GameIO, path: String) {
        let sprite_node = &mut self.sprite_tree[self.shadow_index];
        sprite_node.set_texture(game_io, path);

        let shadow_size = sprite_node.size();
        sprite_node.set_origin(shadow_size * 0.5);
    }

    pub fn current_can_move_to_callback(
        &self,
        actions: &DenseSlotMap<Action>,
    ) -> BattleCallback<(i32, i32), bool> {
        if let Some(index) = self.action_index {
            // does the card action have a special can_move_to_callback?
            if let Some(callback) = actions[index].can_move_to_callback.clone() {
                return callback;
            }
        }

        self.can_move_to_callback.clone()
    }

    pub fn sort_key(&self) -> (i32, i32, i32) {
        (self.y, self.x, -self.sprite_tree.root().layer())
    }

    pub fn flipped(&self) -> bool {
        self.facing == Direction::Left
    }

    pub fn corrected_offset(&self, perspective_flipped: bool) -> Vec2 {
        let mut entity_offset = self.tile_offset + self.offset;

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
}
