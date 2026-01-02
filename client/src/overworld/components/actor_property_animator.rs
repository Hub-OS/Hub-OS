use crate::overworld::components::{MovementAnimator, MovementState};
use crate::render::{Animator, AnimatorLoopMode, FrameTime};
use crate::resources::{AssetManager, AudioBehavior, Globals, OVERWORLD_RUN_THRESHOLD};
use enum_map::EnumMap;
use framework::prelude::{GameIO, Sprite, Vec3, Vec3Swizzles};
use packets::structures::{ActorKeyFrame, ActorProperty, ActorPropertyId, Direction, Ease};

struct PropertyKeyFrame {
    ease: Ease,
    property: ActorProperty,
    previous_property: ActorProperty,
    previous_time_point: f32,
    time_point: f32,
}

impl PropertyKeyFrame {
    fn progress_from_time_point(&self, time_point: f32) -> f32 {
        let time_since_last_frame = time_point - self.previous_time_point;
        let time_between_frames = self.time_point - self.previous_time_point;

        if time_between_frames == 0.0 {
            1.0
        } else {
            (time_since_last_frame / time_between_frames).min(1.0)
        }
    }
}

#[derive(Default)]
struct PropertyState {
    done: bool,
    current_key_frame: usize,
    key_frames: Vec<PropertyKeyFrame>,
}

pub struct ActorPropertyAnimator {
    time: f32,
    total_time: f32,
    sprite_animation_base_time: FrameTime,
    sprite_animation_time: FrameTime,
    sprite_animation_speed: f32,
    movement_animator_moved: bool,
    animating_sprite: bool,
    animating_position: bool,
    animating_direction: bool,
    actor_direction: Direction,
    audio_enabled: bool,
    property_states: EnumMap<ActorPropertyId, PropertyState>,
    relevant_properties: Vec<ActorPropertyId>,
}

impl ActorPropertyAnimator {
    pub fn new() -> Self {
        Self {
            time: 0.0,
            total_time: 0.0,
            sprite_animation_base_time: 0,
            sprite_animation_time: 0,
            sprite_animation_speed: 1.0,
            movement_animator_moved: false,
            animating_sprite: false,
            animating_position: false,
            animating_direction: false,
            actor_direction: Direction::None,
            audio_enabled: true,
            property_states: EnumMap::default(),
            relevant_properties: Vec::new(),
        }
    }

    pub fn is_animating_position(&self) -> bool {
        self.animating_position
    }

    pub fn set_audio_enabled(&mut self, enable: bool) {
        self.audio_enabled = enable;
    }

    pub fn actor_direction(&self) -> Direction {
        self.actor_direction
    }

    pub fn add_key_frame(&mut self, keyframe: ActorKeyFrame) {
        self.total_time += keyframe.duration;

        for (property, ease) in keyframe.property_steps {
            let property_id = property.id();

            if !self.relevant_properties.contains(&property_id) {
                self.relevant_properties.push(property_id);
            }

            let property_key_frame = PropertyKeyFrame {
                ease,
                property: property.clone(),
                previous_property: property,
                time_point: self.total_time,
                previous_time_point: 0.0,
            };

            let state = &mut self.property_states[property_id];
            state.key_frames.push(property_key_frame);
        }
    }

    pub fn start(
        game_io: &GameIO,
        assets: &impl AssetManager,
        entities: &mut hecs::World,
        entity: hecs::Entity,
    ) {
        type Query<'a> = (
            &'a mut ActorPropertyAnimator,
            &'a Vec3,
            &'a Direction,
            &'a Sprite,
            &'a Animator,
            &'a mut MovementAnimator,
        );

        let Ok((property_animator, position, direction, sprite, animator, movement_animator)) =
            entities.query_one_mut::<Query>(entity)
        else {
            return;
        };

        property_animator.movement_animator_moved = movement_animator.movement_enabled();
        property_animator.sprite_animation_base_time = animator.calculate_time();

        for property_id in property_animator.relevant_properties.iter().cloned() {
            let state = &mut property_animator.property_states[property_id];

            // set previous_* data
            let first_frame = &mut state.key_frames[0];

            if first_frame.time_point != 0.0 {
                first_frame.previous_property =
                    Self::default_property_for(property_id, sprite, *position, *direction);
            } else {
                first_frame.previous_property = first_frame.property.clone();
            }

            for i in 1..state.key_frames.len() {
                let (a, b) = state.key_frames.split_at_mut(i);
                let previous = &mut a[i - 1];
                let current = &mut b[0];

                current.previous_property = previous.property.clone();
                current.previous_time_point = previous.time_point;
            }

            // special property handling
            match property_id {
                ActorPropertyId::X | ActorPropertyId::Y | ActorPropertyId::Z => {
                    property_animator.animating_position = true;
                    movement_animator.set_movement_enabled(false);
                }
                ActorPropertyId::Direction => {
                    property_animator.animating_direction = true;
                }
                ActorPropertyId::SoundEffect => {
                    let first_frame = &state.key_frames[0];

                    if property_animator.audio_enabled && first_frame.time_point == 0.0 {
                        let globals = Globals::from_resources(game_io);

                        if let ActorProperty::SoundEffect(path) = &first_frame.property {
                            let sfx = assets.audio(game_io, path);
                            globals.audio.play_sound(&sfx);
                        }
                    }
                }
                ActorPropertyId::SoundEffectLoop => {
                    let last_frame = state.key_frames.last().unwrap();

                    if last_frame.time_point < property_animator.total_time
                        && matches!(&last_frame.property, ActorProperty::SoundEffectLoop(path) if !path.is_empty())
                    {
                        // inject a KeyFrame to keep the last frame looping to the end of the animation
                        let injected_frame = PropertyKeyFrame {
                            ease: Ease::Floor,
                            property: ActorProperty::SoundEffectLoop(String::new()),
                            previous_property: last_frame.property.clone(),
                            time_point: property_animator.total_time,
                            previous_time_point: last_frame.time_point,
                        };

                        state.key_frames.push(injected_frame);
                    }
                }
                _ => {}
            }
        }
    }

    fn default_property_for(
        property_id: ActorPropertyId,
        sprite: &Sprite,
        position: Vec3,
        direction: Direction,
    ) -> ActorProperty {
        match property_id {
            ActorPropertyId::Animation => ActorProperty::Animation(String::new()),
            ActorPropertyId::AnimationSpeed => ActorProperty::AnimationSpeed(1.0),
            ActorPropertyId::X => ActorProperty::X(position.x),
            ActorPropertyId::Y => ActorProperty::Y(position.y),
            ActorPropertyId::Z => ActorProperty::Z(position.z),
            ActorPropertyId::ScaleX => ActorProperty::ScaleX(sprite.scale().x),
            ActorPropertyId::ScaleY => ActorProperty::ScaleY(sprite.scale().y),
            ActorPropertyId::Rotation => ActorProperty::Rotation(sprite.rotation()),
            ActorPropertyId::Direction => ActorProperty::Direction(direction),
            ActorPropertyId::SoundEffect => ActorProperty::SoundEffect(String::new()),
            ActorPropertyId::SoundEffectLoop => ActorProperty::SoundEffectLoop(String::new()),
        }
    }

    pub fn update(game_io: &GameIO, assets: &impl AssetManager, entities: &mut hecs::World) {
        let elapsed = game_io.target_duration().as_secs_f32();

        type Query<'a> = (
            &'a mut ActorPropertyAnimator,
            &'a mut Vec3,
            &'a mut Direction,
            &'a mut Sprite,
            &'a mut Animator,
            &'a mut MovementAnimator,
        );

        let mut animator_remove_list = Vec::new();

        for (
            entity,
            (property_animator, position, direction, sprite, animator, movement_animator),
        ) in entities.query_mut::<Query>()
        {
            property_animator.time += elapsed;

            let previous_position = *position;

            let mut property_remove_list = Vec::new();

            for property_id in property_animator.relevant_properties.iter().cloned() {
                let state = &mut property_animator.property_states[property_id];
                let mut key_frame = &state.key_frames[state.current_key_frame];
                let mut swapped_key_frame = false;

                // use the latest key frame
                while key_frame.time_point < property_animator.time
                    && state.current_key_frame < state.key_frames.len() - 1
                {
                    state.current_key_frame += 1;
                    key_frame = &state.key_frames[state.current_key_frame];
                    swapped_key_frame = true;
                }

                // use previous string, we should only use a string if the time point has been passed
                let active_string_value = if property_animator.time < key_frame.time_point {
                    key_frame.previous_property.get_str()
                } else {
                    key_frame.property.get_str()
                };

                // update property
                match property_id {
                    ActorPropertyId::Animation => {
                        property_animator.animating_sprite = !active_string_value.is_empty();

                        if animator.current_state() != Some(active_string_value) {
                            animator.set_state(active_string_value);
                            animator.set_loop_mode(AnimatorLoopMode::Loop);

                            property_animator.sprite_animation_time = 0;
                            property_animator.sprite_animation_base_time = 0;
                        }
                    }
                    ActorPropertyId::AnimationSpeed => {
                        property_animator.sprite_animation_speed = key_frame.ease.interpolate(
                            key_frame.previous_property.get_f32(),
                            key_frame.property.get_f32(),
                            key_frame.progress_from_time_point(property_animator.time),
                        );

                        property_animator.sprite_animation_time = 0;
                        property_animator.sprite_animation_base_time = animator.calculate_time();
                    }
                    ActorPropertyId::SoundEffect => {
                        if property_animator.audio_enabled
                            && swapped_key_frame
                            && !active_string_value.is_empty()
                        {
                            let sfx = assets.audio(game_io, active_string_value);

                            let globals = Globals::from_resources(game_io);
                            globals.audio.play_sound(&sfx);
                        }
                    }
                    ActorPropertyId::SoundEffectLoop => {
                        if property_animator.audio_enabled && !active_string_value.is_empty() {
                            let sfx = assets.audio(game_io, active_string_value);

                            let globals = Globals::from_resources(game_io);
                            let audio = &globals.audio;

                            audio.play_sound_with_behavior(&sfx, AudioBehavior::NoOverlap);
                        }
                    }
                    ActorPropertyId::Direction => {
                        if let ActorProperty::Direction(new_direction) = key_frame.property {
                            property_animator.actor_direction = new_direction;
                            *direction = new_direction;
                        }
                    }
                    _ => {
                        Self::apply_f32_property(
                            key_frame,
                            property_animator.time,
                            sprite,
                            position,
                        );
                    }
                }

                // mark deletion for this tracker if it has completed
                if property_animator.time > key_frame.time_point {
                    property_remove_list.push(property_id);
                }
            }

            // drop finished properties
            for property in property_remove_list {
                property_animator.property_states[property]
                    .key_frames
                    .clear();

                let index = property_animator
                    .relevant_properties
                    .iter()
                    .position(|p| *p == property);

                if let Some(index) = index {
                    property_animator.relevant_properties.remove(index);
                }
            }

            // handle animation speed
            if property_animator.sprite_animation_speed != 1.0 {
                let multiplier = property_animator.sprite_animation_speed;
                let start = property_animator.sprite_animation_base_time;
                let elapsed = property_animator.sprite_animation_time;

                let sync_time = start + (elapsed as f32 * multiplier) as FrameTime;
                animator.sync_time(sync_time);
            }

            // handle movement
            let position_difference = *position - previous_position;

            if property_animator.animating_position && !property_animator.animating_sprite {
                // resolve direction and player animation
                let new_direction = if property_animator.animating_direction {
                    property_animator.actor_direction
                } else {
                    Direction::from_offset(position_difference.xy().into())
                };

                if !new_direction.is_none() {
                    *direction = new_direction;
                }

                let distance = position_difference.length();

                let movement_state = if distance == 0.0 {
                    MovementState::Idle
                } else if distance > OVERWORLD_RUN_THRESHOLD * 60.0 * elapsed {
                    MovementState::Walking
                } else {
                    MovementState::Running
                };

                movement_animator.set_state(movement_state);
            }

            movement_animator.set_animation_enabled(!property_animator.animating_sprite);
            property_animator.sprite_animation_time += 1;

            if property_animator.relevant_properties.is_empty() {
                movement_animator.set_animation_enabled(true);
                movement_animator.set_movement_enabled(property_animator.movement_animator_moved);
                animator_remove_list.push(entity);
            }
        }

        // drop completed animators
        for entity in animator_remove_list {
            let _ = entities.remove_one::<ActorPropertyAnimator>(entity);
        }
    }

    fn apply_f32_property(
        key_frame: &PropertyKeyFrame,
        time_point: f32,
        sprite: &mut Sprite,
        position: &mut Vec3,
    ) {
        let value = key_frame.ease.interpolate(
            key_frame.previous_property.get_f32(),
            key_frame.property.get_f32(),
            key_frame.progress_from_time_point(time_point),
        );

        match key_frame.property.id() {
            ActorPropertyId::X => {
                position.x = value;
            }
            ActorPropertyId::Y => {
                position.y = value;
            }
            ActorPropertyId::Z => {
                position.z = value;
            }
            ActorPropertyId::ScaleX => {
                let mut scale = sprite.scale();
                scale.x = value;
                sprite.set_scale(scale);
            }
            ActorPropertyId::ScaleY => {
                let mut scale = sprite.scale();
                scale.y = value;
                sprite.set_scale(scale);
            }
            ActorPropertyId::Rotation => {
                sprite.set_rotation(value);
            }
            ActorPropertyId::Direction
            | ActorPropertyId::AnimationSpeed
            | ActorPropertyId::Animation
            | ActorPropertyId::SoundEffect
            | ActorPropertyId::SoundEffectLoop => {
                log::error!("Called apply_f32_property for invalid ActorPropertyId")
            }
        }
    }

    pub fn stop(entities: &mut hecs::World, entity: hecs::Entity) {
        let Ok(property_animator) = entities.remove_one::<ActorPropertyAnimator>(entity) else {
            return;
        };

        let Ok(movement_animator) = entities.query_one_mut::<&mut MovementAnimator>(entity) else {
            return;
        };

        movement_animator.set_movement_enabled(property_animator.movement_animator_moved);
    }
}
