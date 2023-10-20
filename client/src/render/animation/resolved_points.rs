use super::Animator;
use enum_map::{EnumArray, EnumMap};
use framework::prelude::Vec2;
use std::ops::Index;

#[derive(Debug)]
pub struct ResolvedPoints<K: EnumArray<Vec2>> {
    position_map: EnumMap<K, Vec2>,
}

impl<K: EnumArray<Vec2>> ResolvedPoints<K> {
    pub fn offset(&mut self, offset: Vec2) {
        for position in self.position_map.values_mut() {
            *position += offset;
        }
    }
}

impl<K: EnumArray<Vec2>> Index<K> for ResolvedPoints<K> {
    type Output = Vec2;

    fn index(&self, index: K) -> &Self::Output {
        &self.position_map[index]
    }
}

impl<K: EnumArray<Vec2> + EnumArray<&'static str>> ResolvedPoints<K> {
    pub fn new(animator: &Animator, point_map: EnumMap<K, &'static str>) -> Self {
        let origin = animator.origin();
        let position_map =
            point_map.map(|_, name| animator.point(name).unwrap_or_default() - origin);

        Self { position_map }
    }
}

struct StateParent<K> {
    state: &'static str,
    point: Option<K>,
    children: Vec<K>,
    depth: usize,
}

impl<K: Copy + PartialEq> StateParent<K> {
    fn find_point_parent(parents: &[Self], k: K) -> Option<&Self> {
        parents.iter().find(|s| s.children.contains(&k))
    }

    fn resolve_depth(parents: &[Self], index: usize) -> usize {
        let root = &parents[index];
        let mut parent = root.point;
        let mut counter = 0;

        #[cfg(debug_assertions)]
        let mut seen = Vec::new();

        while let Some(k) = parent {
            counter += 1;

            parent = Self::find_point_parent(parents, k).unwrap().point;

            #[cfg(debug_assertions)]
            if seen.contains(&k) {
                panic!("{:?} has a recursive parent", root.state);
            } else {
                seen.push(k);
            }
        }

        counter
    }
}

impl<K> ResolvedPoints<K>
where
    K: EnumArray<Vec2> + EnumArray<(&'static str, &'static str)> + Copy + PartialEq,
{
    pub fn new_parented(
        animator: &mut Animator,
        state_and_point_map: EnumMap<K, (&'static str, &'static str)>,
        parent_point_callback: impl Fn(&'static str) -> Option<K>,
    ) -> Self {
        let mut parents: Vec<StateParent<K>> = Vec::new();

        let mut position_map = state_and_point_map.map(|k, (state, name)| {
            // store parent info for later offset
            if let Some(parent) = parents.iter_mut().find(|p| p.state == state) {
                parent.children.push(k);
            } else {
                parents.push(StateParent {
                    state,
                    point: parent_point_callback(state),
                    children: vec![k],
                    depth: 0,
                });
            }

            // store initial position value
            animator.set_state(state);
            animator.point(name).unwrap_or_default() - animator.origin()
        });

        // resolve parent depth for sorting
        for i in 0..parents.len() {
            let depth = StateParent::resolve_depth(&parents, i);
            parents[i].depth = depth;
        }

        // we sort by depth to avoid using incorrect WIP data from unresolved points
        parents.sort_by_key(|p| p.depth);

        // update positions using parent points
        for parent in parents {
            let Some(point_key) = parent.point else {
                continue;
            };

            let offset = position_map[point_key];

            for k in parent.children {
                position_map[k] += offset;
            }
        }

        Self { position_map }
    }
}

#[cfg(test)]
mod test {
    use crate::render::animation::*;
    use enum_map::{enum_map, Enum};

    #[derive(Enum, PartialEq, Eq, Clone, Copy)]
    enum Points {
        A,
        B,
        C,
        D,
    }

    const ANIMATION_STR: &str = r#"
        animation state="0"
        frame
        point label="A" x="1"

        animation state="1"
        frame
        point label="B" x="10"

        animation state="2"
        frame
        point label="C" x="100"
        point label="D" x="1000"
    "#;

    #[test]
    fn resolves_current() {
        let mut animator = Animator::new();

        animator.load_from_str(ANIMATION_STR);
        animator.set_state("2");

        let points = ResolvedPoints::new(
            &animator,
            enum_map! {
                Points::A => "A",
                Points::B => "B",
                Points::C => "C",
                Points::D => "D",
            },
        );

        assert_eq!(points[Points::A].x, 0.0);
        assert_eq!(points[Points::B].x, 0.0);
        assert_eq!(points[Points::C].x, 100.0);
        assert_eq!(points[Points::D].x, 1000.0);
    }

    #[test]
    fn resolves_parents() {
        let mut animator = Animator::new();

        animator.load_from_str(ANIMATION_STR);

        let points = ResolvedPoints::new_parented(
            &mut animator,
            enum_map! {
                Points::A => ("0", "A"),
                Points::B => ("1", "B"),
                Points::C => ("2", "C"),
                Points::D => ("2", "D"),
            },
            |state| match state {
                "0" => None,
                "1" => Some(Points::A),
                "2" => Some(Points::B),
                _ => unreachable!(),
            },
        );

        assert_eq!(points[Points::A].x, 1.0);
        assert_eq!(points[Points::B].x, 11.0);
        assert_eq!(points[Points::C].x, 111.0);
        assert_eq!(points[Points::D].x, 1011.0);
    }
}
