@group(0) @binding(1)
var palette: texture_2d<f32>;

@group(1) @binding(0)
var txture: texture_2d<f32>;
@group(1) @binding(1)
var smplr: sampler;

fn to_srgb(v: f32) -> f32 {
    if v <= 0.0031308 {
        return v * 12.92;
    } else {
        return (1.055 * pow(v, 0.416)) - 0.055;
    }
}

fn sample_palette(uv: vec2<f32>) -> vec4<f32> {
    let sample = textureSample(txture, smplr, uv);
    let i = to_srgb(sample.x);
    return textureSample(palette, smplr, vec2<f32>(i, 0.5));
}

@fragment
fn multiply_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>) -> @location(0) vec4<f32> {
    return color * sample_palette(uv);
}

@fragment
fn add_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>) -> @location(0) vec4<f32> {
    let palette_sample = sample_palette(uv);
    var out: vec4<f32> = clamp(color + palette_sample, vec4<f32>(), vec4<f32>(1.0));
    out.w = palette_sample.w * color.w;

    return out;
}

fn resolve_pixelated_uv(uv: vec2<f32>, color: vec4<f32>, frame_size: vec2<f32>) -> vec2<f32> {
    let pixelation = mix(frame_size * 0.5, vec2<f32>(0.0), color.a);

    if pixelation.x != 0.0 {
        return trunc(uv / pixelation) * pixelation;
    } else {
        return uv;
    }
}

@fragment
fn pixelate_multiply_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>, @location(2) frame_size: vec2<f32>) -> @location(0) vec4<f32> {
    let updated_uv = resolve_pixelated_uv(uv, color, frame_size);

    return color * sample_palette(updated_uv);
}

@fragment
fn pixelate_add_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>, @location(2) frame_size: vec2<f32>) -> @location(0) vec4<f32> {
    let updated_uv = resolve_pixelated_uv(uv, color, frame_size);

    let sample = sample_palette(updated_uv);
    var out: vec4<f32> = clamp(color + sample, vec4<f32>(), vec4<f32>(1.0));
    out.w = sample.w * color.w;

    return out;
}
