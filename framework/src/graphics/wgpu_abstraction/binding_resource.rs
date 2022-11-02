pub type BindingResource<'a> = wgpu::BindingResource<'a>;

pub trait AsBinding {
    fn as_binding(&self) -> BindingResource;
}
