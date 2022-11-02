use super::*;

pub enum NextScene<Globals> {
    Push {
        scene: Box<dyn Scene<Globals>>,
        transition: Option<Box<dyn Transition<Globals>>>,
    },
    Swap {
        scene: Box<dyn Scene<Globals>>,
        transition: Option<Box<dyn Transition<Globals>>>,
    },
    PopSwap {
        scene: Box<dyn Scene<Globals>>,
        transition: Option<Box<dyn Transition<Globals>>>,
    },
    Pop {
        transition: Option<Box<dyn Transition<Globals>>>,
    },
    // skips calling Scene::enter
    #[doc(hidden)]
    __InternalPush {
        scene: Box<dyn Scene<Globals>>,
        transition: Option<Box<dyn Transition<Globals>>>,
    },
    // skips calling Scene::enter
    #[doc(hidden)]
    __InternalSwap {
        scene: Box<dyn Scene<Globals>>,
        transition: Option<Box<dyn Transition<Globals>>>,
    },
    None,
}

impl<Globals> NextScene<Globals> {
    #[inline]
    pub fn new_push(scene: impl Scene<Globals> + 'static) -> Self {
        NextScene::Push {
            scene: Box::new(scene),
            transition: None,
        }
    }

    #[inline]
    pub fn new_swap(scene: impl Scene<Globals> + 'static) -> Self {
        NextScene::Swap {
            scene: Box::new(scene),
            transition: None,
        }
    }

    #[inline]
    pub fn new_pop_swap(scene: impl Scene<Globals> + 'static) -> Self {
        NextScene::PopSwap {
            scene: Box::new(scene),
            transition: None,
        }
    }

    #[inline]
    pub fn new_pop() -> Self {
        NextScene::Pop { transition: None }
    }

    pub fn with_transition(mut self, transition: impl Transition<Globals> + 'static) -> Self {
        match &mut self {
            NextScene::Push {
                transition: set_transition,
                ..
            }
            | NextScene::Swap {
                transition: set_transition,
                ..
            }
            | NextScene::PopSwap {
                transition: set_transition,
                ..
            }
            | NextScene::Pop {
                transition: set_transition,
            }
            | NextScene::__InternalPush {
                transition: set_transition,
                ..
            }
            | NextScene::__InternalSwap {
                transition: set_transition,
                ..
            } => {
                *set_transition = Some(Box::new(transition));
            }
            NextScene::None => {}
        }

        self
    }

    pub fn is_none(&self) -> bool {
        matches!(self, NextScene::None)
    }

    pub fn is_some(&self) -> bool {
        !matches!(self, NextScene::None)
    }

    pub fn take(&mut self) -> NextScene<Globals> {
        let mut next_scene = NextScene::None;
        std::mem::swap(&mut next_scene, self);
        next_scene
    }
}

impl<Globals> Default for NextScene<Globals> {
    fn default() -> Self {
        Self::None
    }
}
