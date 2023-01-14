use crate::saves::Config;

#[repr(C)]
#[derive(Clone, Copy, PartialEq, bytemuck::Pod, bytemuck::Zeroable)]
pub struct PostProcessAdjustConfig {
    pub brightness: f32,
    pub saturation: f32,
}

impl PostProcessAdjustConfig {
    pub fn from_config(config: &Config) -> Self {
        Self {
            brightness: (config.brightness as f32 * 0.01).powf(2.2),
            saturation: config.saturation as f32 * 0.01,
        }
    }

    pub fn should_enable(&self) -> bool {
        self.brightness != 1.0 || self.saturation != 1.0
    }
}
