use crate::prelude::*;

/// RenderQueues only render when consumed by a RenderPass
pub struct SpriteQueue<'a, InstanceData: self::InstanceData> {
    sprite_pipeline: &'a SpritePipeline<InstanceData>,
    render_queue: RenderQueue<'a, SpriteVertex, InstanceData>,
}

impl<'a, InstanceData: self::InstanceData> SpriteQueue<'a, InstanceData> {
    pub fn new<'b, Globals, I>(
        game_io: &'a GameIO<Globals>,
        sprite_pipeline: &'a SpritePipeline<InstanceData>,
        uniform_resources: I,
    ) -> Self
    where
        I: IntoIterator<Item = BindingResource<'b>>,
    {
        Self {
            sprite_pipeline,
            render_queue: RenderQueue::new(game_io, sprite_pipeline, uniform_resources),
        }
    }

    pub fn set_uniforms<'b, I>(&mut self, uniform_resources: I)
    where
        I: IntoIterator<Item = BindingResource<'b>>,
    {
        self.render_queue.set_uniforms(uniform_resources);
    }

    pub fn set_scissor(&mut self, rect: Rect) {
        self.render_queue.set_scissor(rect);
    }

    pub fn draw_sprite<Instance: self::Instance<InstanceData>>(&mut self, sprite: &Instance) {
        let mesh = self.sprite_pipeline.mesh();
        self.render_queue.draw_instance(mesh, sprite);
    }
}

impl<'a, InstanceData: self::InstanceData> RenderQueueTrait for SpriteQueue<'a, InstanceData> {
    fn into_operation_vec(self) -> Vec<RenderOperation> {
        self.render_queue.into_operation_vec()
    }
}
