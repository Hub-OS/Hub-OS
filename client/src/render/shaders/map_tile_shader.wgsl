struct MapUniforms {
    center: vec2<f32>,
    tile_size: vec2<f32>,
    trim: i32,
    padding: i32,
};

@group(0) @binding(1)
var<uniform> map_uniforms: MapUniforms;

@group(1) @binding(0)
var txture: texture_2d<f32>;
@group(1) @binding(1)
var smplr: sampler;

@fragment
fn fs_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>) -> @location(0) vec4<f32> {
    let sample = textureSample(txture, smplr, uv);

    let diff = abs(map_uniforms.center - uv);
    let tile_scaled_diff = diff / map_uniforms.tile_size;

    if map_uniforms.trim != 0 && tile_scaled_diff.x + tile_scaled_diff.y >= 0.5 {
        discard;
    }

    return ceil(sample.a) * color;
}
