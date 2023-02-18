#[derive(Default)]
pub struct Excluded {
    locks: usize,
}

impl Excluded {
    pub fn increment(entities: &mut hecs::World, entity: hecs::Entity) {
        if let Ok(component) = entities.query_one_mut::<&mut Excluded>(entity) {
            component.locks += 1;
        } else {
            let _ = entities.insert_one(entity, Excluded { locks: 1 });
        }
    }

    pub fn decrement(entities: &mut hecs::World, entity: hecs::Entity) {
        let Ok(component) = entities.query_one_mut::<&mut Excluded>(entity) else {
            return;
        };

        component.locks -= 1;

        if component.locks == 0 {
            let _ = entities.remove_one::<Self>(entity);
        }
    }
}
