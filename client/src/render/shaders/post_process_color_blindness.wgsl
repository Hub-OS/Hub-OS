@group(1) @binding(0)
var txture: texture_2d<f32>;
@group(1) @binding(1)
var smplr: sampler;

@group(0) @binding(0)
var<uniform> simulation_matrix: mat3x3<f32>;

@fragment
fn fs_main(@location(0) uv: vec2<f32>) -> @location(0) vec4<f32> {
    let sample = textureSample(txture, smplr, uv);
    let simulated = simulation_matrix * sample.xyz;

    return vec4<f32>(saturate(simulated), sample.w);
}
