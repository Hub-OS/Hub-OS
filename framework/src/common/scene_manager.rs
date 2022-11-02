use crate::prelude::*;
use log::error;

pub(crate) struct SceneManager<Globals: 'static> {
    scenes: Vec<Box<dyn Scene<Globals>>>,
}

impl<Globals> SceneManager<Globals> {
    pub(crate) fn new(initial_scene: Box<dyn Scene<Globals>>) -> Self {
        Self {
            scenes: vec![initial_scene],
        }
    }

    fn active_scene_mut(&mut self) -> &mut Box<dyn Scene<Globals>> {
        self.scenes.last_mut().unwrap()
    }

    pub(crate) fn update(&mut self, game_io: &mut GameIO<Globals>) {
        let active_scene = self.active_scene_mut();
        active_scene.update(game_io);

        while self.handle_scene_request(game_io) {}
    }

    fn handle_scene_request(&mut self, game_io: &mut GameIO<Globals>) -> bool {
        let active_scene = self.active_scene_mut();

        let to_scene_request = active_scene.next_scene().take();

        match to_scene_request {
            NextScene::None => false,
            NextScene::Push {
                mut scene,
                transition,
            } => {
                scene.enter(game_io);
                self.push_scene(scene, transition);
                true
            }
            NextScene::Swap {
                mut scene,
                transition,
            } => {
                scene.enter(game_io);
                self.swap_scene(scene, transition);
                true
            }
            NextScene::PopSwap {
                mut scene,
                transition,
            } => {
                scene.enter(game_io);

                // remove the scene under the current one
                self.scenes.remove(self.scenes.len() - 2);

                // swap with the top scene
                self.swap_scene(scene, transition);
                true
            }
            NextScene::__InternalPush { scene, transition } => {
                self.push_scene(scene, transition);
                true
            }
            NextScene::__InternalSwap { scene, transition } => {
                self.swap_scene(scene, transition);
                true
            }
            NextScene::Pop { transition } => {
                self.pop_scene(game_io, transition);
                true
            }
        }
    }

    fn push_scene(
        &mut self,
        scene: Box<dyn Scene<Globals>>,
        transition: Option<Box<dyn Transition<Globals>>>,
    ) {
        if let Some(transition) = transition {
            let current_scene = self.scenes.pop().unwrap();

            let transition_scene =
                Box::new(TransitionWrapper::new(transition, current_scene, scene));

            self.scenes.push(transition_scene);
        } else {
            self.scenes.push(scene);
        }
    }

    fn swap_scene(
        &mut self,
        scene: Box<dyn Scene<Globals>>,
        transition: Option<Box<dyn Transition<Globals>>>,
    ) {
        let current_scene = self.scenes.pop().unwrap();

        if let Some(transition) = transition {
            let mut transition_scene =
                Box::new(TransitionWrapper::new(transition, current_scene, scene));

            transition_scene.replaces = true;

            self.scenes.push(transition_scene);
        } else {
            self.scenes.push(scene);
        }
    }

    fn pop_scene(
        &mut self,
        game_io: &mut GameIO<Globals>,
        transition: Option<Box<dyn Transition<Globals>>>,
    ) {
        let current_scene = self.scenes.pop().unwrap();
        let to_scene = self.scenes.pop();

        if let Some(mut to_scene) = to_scene {
            to_scene.enter(game_io);

            if let Some(transition) = transition {
                let mut transition_scene =
                    Box::new(TransitionWrapper::new(transition, current_scene, to_scene));

                transition_scene.replaces = true;
                transition_scene.resumes = true;

                self.scenes.push(transition_scene);
            } else {
                to_scene.resume(game_io);
                self.scenes.push(to_scene);
            }
        } else {
            panic!("No scene to pop into");
        }
    }

    pub(crate) fn draw<'a: 'b, 'b>(
        &'a mut self,
        game_io: &'a mut GameIO<Globals>,
        render_pass: &'b mut RenderPass<'_>,
    ) {
        self.active_scene_mut().draw(game_io, render_pass);
    }
}

struct TransitionWrapper<Globals> {
    transition: Box<dyn Transition<Globals>>,
    from_scene: Option<Box<dyn Scene<Globals>>>,
    to_scene: Option<Box<dyn Scene<Globals>>>,
    replaces: bool,
    resumes: bool,
    popping: bool,
    next_scene: NextScene<Globals>,
}

impl<Globals> TransitionWrapper<Globals> {
    fn new(
        transition: Box<dyn Transition<Globals>>,
        from_scene: Box<dyn Scene<Globals>>,
        to_scene: Box<dyn Scene<Globals>>,
    ) -> Self {
        Self {
            transition,
            from_scene: Some(from_scene),
            to_scene: Some(to_scene),
            replaces: false,
            resumes: false,
            popping: false,
            next_scene: NextScene::None,
        }
    }
}

impl<Globals: 'static> Scene<Globals> for TransitionWrapper<Globals> {
    fn next_scene(&mut self) -> &mut NextScene<Globals> {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO<Globals>) {
        if let Some(scene) = self.to_scene.as_mut() {
            scene.enter(game_io);
        }

        if let Some(scene) = self.from_scene.as_mut() {
            scene.enter(game_io);
        }
    }

    fn resume(&mut self, game_io: &mut GameIO<Globals>) {
        if self.popping || !self.transition.is_complete() {
            return;
        }

        let resumed_scene = self
            .to_scene
            .as_mut()
            .unwrap_or_else(|| self.from_scene.as_mut().unwrap());

        resumed_scene.resume(game_io);
    }

    fn update(&mut self, game_io: &mut GameIO<Globals>) {
        if self.next_scene.is_none()
            && !self.popping
            && !game_io.is_in_transition()
            && self.transition.is_complete()
        {
            // unwrap as nothing else should be taking this value
            let mut scene = self
                .to_scene
                .take()
                .unwrap_or_else(|| self.from_scene.take().unwrap());

            if self.resumes {
                // resumes can only happen when replacing,
                // so we don't need to worry about resuming from_scene
                scene.resume(game_io);
            }

            scene.update(game_io);

            if self.replaces || self.from_scene.is_none() {
                self.next_scene = NextScene::__InternalSwap {
                    scene,
                    transition: None,
                };
            } else {
                self.next_scene = NextScene::__InternalPush {
                    scene,
                    transition: None,
                };
            }

            return;
        }

        let started_in_transition = game_io.is_in_transition();
        game_io.set_transitioning(true);

        if let Some(from_scene) = self.from_scene.as_mut() {
            from_scene.update(game_io);

            let next_scene = from_scene.next_scene().take();

            if next_scene.is_some() && self.to_scene.is_some() {
                error!("Closing scene should return None on updates until resume()");
            } else if self.next_scene.is_none() {
                self.next_scene = next_scene;
            }
        }

        if let Some(to_scene) = self.to_scene.as_mut() {
            to_scene.update(game_io);

            if !self.popping && self.next_scene.is_none() {
                self.next_scene = match to_scene.next_scene().take() {
                    NextScene::Pop { transition } => {
                        self.popping = false;

                        #[allow(clippy::manual_map)]
                        if self.replaces {
                            // removing previous scene anyway
                            NextScene::Pop { transition }
                        } else if let Some(scene) = self.from_scene.take() {
                            // we need to transition to the from_scene
                            // and activate resume() at the right time

                            let scene = Box::new(TransitionResumer::new(scene));
                            NextScene::__InternalSwap { scene, transition }
                        } else {
                            // must have already sent a NextScene::Swap for previous scene if it's empty
                            // doesn't matter what we transition to now
                            NextScene::None
                        }
                    }
                    next_scene => next_scene,
                };

                self.resumes = self.next_scene.is_some();
            }
        }

        game_io.set_transitioning(started_in_transition);
    }

    fn draw(&mut self, game_io: &mut GameIO<Globals>, render_pass: &mut RenderPass<'_>) {
        let started_in_transition = game_io.is_in_transition();
        game_io.set_transitioning(true);

        let transition = &mut self.transition;

        match (&mut self.from_scene, &mut self.to_scene) {
            (Some(from_scene), Some(to_scene)) => {
                transition.draw(game_io, render_pass, from_scene, to_scene)
            }
            (Some(from_scene), None) => from_scene.draw(game_io, render_pass),
            (None, Some(to_scene)) => to_scene.draw(game_io, render_pass),
            _ => {}
        }

        game_io.set_transitioning(started_in_transition);
    }
}

// resumes scenes stuck in a transition
struct TransitionResumer<Globals: 'static> {
    wrapped_scene: Option<Box<dyn Scene<Globals>>>,
    next_scene: NextScene<Globals>,
    resumed: bool,
}

impl<Globals> TransitionResumer<Globals> {
    fn new(scene: Box<dyn Scene<Globals>>) -> Self {
        Self {
            wrapped_scene: Some(scene),
            next_scene: NextScene::None,
            resumed: false,
        }
    }
}

impl<Globals> Scene<Globals> for TransitionResumer<Globals> {
    fn next_scene(&mut self) -> &mut NextScene<Globals> {
        self.wrapped_scene
            .as_mut()
            .map(|scene| scene.next_scene())
            .unwrap_or(&mut self.next_scene)
    }

    fn enter(&mut self, game_io: &mut GameIO<Globals>) {
        if let Some(scene) = self.wrapped_scene.as_mut() {
            scene.enter(game_io);
        }
    }

    fn resume(&mut self, game_io: &mut GameIO<Globals>) {
        if let Some(scene) = self.wrapped_scene.as_mut() {
            scene.resume(game_io);
        }
    }

    fn update(&mut self, game_io: &mut GameIO<Globals>) {
        let wrapped_scene = self.wrapped_scene.as_mut().unwrap();

        if !self.resumed && !game_io.is_in_transition() {
            wrapped_scene.resume(game_io);
            self.resumed = true;
        }

        wrapped_scene.update(game_io);

        if !game_io.is_in_transition() {
            let scene = self.wrapped_scene.take().unwrap();

            self.next_scene = NextScene::__InternalSwap {
                scene,
                transition: None,
            }
        }
    }

    fn draw(&mut self, game_io: &mut GameIO<Globals>, render_pass: &mut RenderPass) {
        match self.wrapped_scene.as_mut() {
            Some(scene) => scene.draw(game_io, render_pass),
            None => unreachable!(),
        }
    }
}
