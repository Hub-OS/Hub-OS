use crate::prelude::*;
use std::sync::Arc;

pub trait Model<Vertex: super::Vertex, InstanceData: super::InstanceData>:
    Instance<InstanceData>
{
    fn mesh(&self) -> &Arc<super::Mesh<Vertex>>;
}
