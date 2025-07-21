use crate::net::{ActorKeyFrame, ActorProperty, Direction, Ease};

pub fn parse_animation(keyframe_tables: Vec<mlua::Table>) -> mlua::Result<Vec<ActorKeyFrame>> {
    let mut animation = Vec::new();

    for keyframe_table in keyframe_tables {
        let duration_option: Option<f32> = keyframe_table.get("duration")?;

        let mut keyframe = ActorKeyFrame {
            property_steps: Vec::new(),
            duration: duration_option.unwrap_or_default(),
        };

        let property_tables: Vec<mlua::Table> = keyframe_table.get("properties")?;

        for property_table in property_tables {
            let property_name: mlua::String = property_table.get("property")?;
            let property_name_str = property_name.to_str()?;

            let property = match property_name_str {
                "Animation" => ActorProperty::Animation(property_table.get("value")?),
                "Animation Speed" => ActorProperty::AnimationSpeed(property_table.get("value")?),
                "X" => ActorProperty::X(property_table.get("value")?),
                "Y" => ActorProperty::Y(property_table.get("value")?),
                "Z" => ActorProperty::Z(property_table.get("value")?),
                "ScaleX" => ActorProperty::ScaleX(property_table.get("value")?),
                "ScaleY" => ActorProperty::ScaleY(property_table.get("value")?),
                "Rotation" => ActorProperty::Rotation(property_table.get("value")?),
                "Direction" => {
                    let value: mlua::String = property_table.get("value")?;

                    ActorProperty::Direction(Direction::from(value.to_str()?))
                }
                "Sound Effect" => ActorProperty::SoundEffect(property_table.get("value")?),
                "Sound Effect Loop" => ActorProperty::SoundEffectLoop(property_table.get("value")?),
                _ => {
                    let error_string = format!("Unknown Property: {property_name_str}");
                    return Err(mlua::Error::RuntimeError(error_string));
                }
            };

            let ease_name_option: Option<mlua::String> = property_table.get("ease")?;

            let ease = match ease_name_option {
                Some(ease_name) => {
                    let ease_name_str = ease_name.to_str()?;
                    match ease_name_str {
                        "Linear" => Ease::Linear,
                        "In" => Ease::In,
                        "Out" => Ease::Out,
                        "InOut" => Ease::InOut,
                        "Floor" => Ease::Floor,
                        _ => {
                            let error_string = format!("Unknown Ease: {ease_name_str}");
                            return Err(mlua::Error::RuntimeError(error_string));
                        }
                    }
                }
                None => Ease::Floor,
            };

            keyframe.property_steps.push((property, ease));
        }

        animation.push(keyframe);
    }

    Ok(animation)
}
