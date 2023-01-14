@group(1) @binding(0)
var txture: texture_2d<f32>;
@group(1) @binding(1)
var smplr: sampler;

struct Config {
    brightness: f32,
    saturation: f32,
}

@group(0) @binding(0)
var<uniform> config: Config;

@fragment
fn fs_main(@location(0) uv: vec2<f32>) -> @location(0) vec4<f32> {
    let sample = textureSample(txture, smplr, uv);
    let value = (sample.x + sample.y + sample.z) * 0.3333;

    return vec4<f32>(
        clamp(
            config.brightness * (sample.xyz * config.saturation + value * (1.0 - config.saturation)),
            vec3<f32>(),
            vec3<f32>(1.0)
        ),
        1.0
    );
}
