@group(1) @binding(0)
var txture: texture_2d<f32>;
@group(1) @binding(1)
var smplr: sampler;

@fragment
fn fs_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>) -> @location(0) vec4<f32> {
    let sample = textureSample(txture, smplr, uv);
    let texture_unit = 1.0 / vec2<f32>(textureDimensions(txture));

    let left = textureSample(txture, smplr, uv + vec2<f32>(-texture_unit.x, 0.0)).a;
    let right = textureSample(txture, smplr, uv + vec2<f32>(texture_unit.x, 0.0)).a;
    let up = textureSample(txture, smplr, uv + vec2<f32>(0.0, -texture_unit.y)).a;
    let down = textureSample(txture, smplr, uv + vec2<f32>(0.0, texture_unit.y)).a;

    let is_edge = ceil(sample.a) - clamp(ceil(left * right * up * down), 0.0, 1.0);

    return is_edge * color + (1.0 - is_edge) * sample;
}
