@group(1) @binding(0)
var txture: texture_2d<f32>;
@group(1) @binding(1)
var smplr: sampler;

@group(0) @binding(0)
var<uniform> ghosting: f32;

@fragment
fn fs_main(@location(0) uv: vec2<f32>) -> @location(0) vec4<f32> {
    return vec4<f32>(textureSample(txture, smplr, uv).xyz, 1.0 - ghosting);
}
