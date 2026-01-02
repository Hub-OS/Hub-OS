use crate::resources::Globals;
use framework::{prelude::*, wgpu};

// matrices taken from
// https://www.inf.ufrgs.br/~oliveira/pubs_files/CVD_Simulation/CVD_Simulation.html

const MATRICES: [Mat3; 3] = [
    // protanopia
    Mat3::from_cols(
        Vec3::new(0.152286, 1.052583, -0.204868),
        Vec3::new(0.114503, 0.786281, 0.099216),
        Vec3::new(-0.003882, -0.048116, 1.051998),
    ),
    // deuteranopia
    Mat3::from_cols(
        Vec3::new(0.367322, 0.860646, -0.227968),
        Vec3::new(0.280085, 0.672501, 0.047413),
        Vec3::new(-0.011820, 0.042940, 0.968881),
    ),
    // tritanopia
    Mat3::from_cols(
        Vec3::new(1.255528, -0.076749, -0.178779),
        Vec3::new(-0.078411, 0.930809, 0.147602),
        Vec3::new(0.004733, 0.691367, 0.303900),
    ),
];

const MAT3_LAYOUT: [VertexFormat; 3] = [
    VertexFormat::Float32x3,
    VertexFormat::Float32x3,
    VertexFormat::Float32x3,
];

pub struct PostProcessColorBlindness {
    selection: u8,
    matrix_resource: StructResource<Mat3>,
    pipeline: PostPipeline,
}

impl PostProcessColorBlindness {
    pub const TOTAL_OPTIONS: u8 = MATRICES.len() as u8;

    pub fn new(game_io: &GameIO) -> Self {
        let device = game_io.graphics().device();
        let shader =
            device.create_shader_module(include_wgsl!("post_process_color_blindness.wgsl"));

        let globals = Globals::from_resources(game_io);
        let selection = globals.post_process_color_blindness;

        Self {
            selection,
            matrix_resource: StructResource::new_with_layout(
                game_io,
                MATRICES[selection.min(Self::TOTAL_OPTIONS - 1) as usize].transpose(),
                &MAT3_LAYOUT,
            ),
            pipeline: PostPipeline::new(
                game_io,
                &shader,
                "fs_main",
                &[BindGroupLayoutEntry {
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    binding_type: StructResource::<()>::binding_type(),
                }],
            ),
        }
    }
}

impl PostProcess for PostProcessColorBlindness {
    fn render_pipeline(&self) -> &PostPipeline {
        &self.pipeline
    }

    fn uniform_resources(&self) -> Vec<BindingResource<'_>> {
        vec![self.matrix_resource.as_binding()]
    }

    fn update(&mut self, game_io: &GameIO) {
        let globals = Globals::from_resources(game_io);
        let selection = globals.post_process_color_blindness;

        if self.selection != selection {
            self.selection = selection;

            let matrix = MATRICES[selection.min(Self::TOTAL_OPTIONS - 1) as usize].transpose();
            self.matrix_resource = StructResource::new_with_layout(game_io, matrix, &MAT3_LAYOUT);
        }
    }
}
