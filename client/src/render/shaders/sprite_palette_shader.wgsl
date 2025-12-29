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
    let i = to_srgb(sample.x) + 0.5 / f32(textureDimensions(palette).x);
    return textureSample(palette, smplr, vec2<f32>(i, 0.5));
}

@fragment
fn adopt_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>) -> @location(0) vec4<f32> {
    return vec4<f32>(color.rgb, sample_palette(uv).a * color.a);
}

@fragment
fn multiply_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>) -> @location(0) vec4<f32> {
    return color * sample_palette(uv);
}

@fragment
fn add_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>) -> @location(0) vec4<f32> {
    let palette_sample = sample_palette(uv);
    var out: vec4<f32> = clamp(color + palette_sample, vec4<f32>(), vec4<f32>(1.0));
    out.a = palette_sample.a * color.a;

    return out;
}

// no grascale_adopt_main implementation, adopt_main is used instead

@fragment
fn grayscale_multiply_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>) -> @location(0) vec4<f32> {
    let sample = sample_palette(uv);
    let average = (sample.r + sample.g + sample.b) * 0.33333;
    return color * vec4<f32>(vec3<f32>(average), sample.a);
}

@fragment
fn grayscale_add_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>) -> @location(0) vec4<f32> {
    let sample = sample_palette(uv);
    let average = (sample.r + sample.g + sample.b) * 0.33333;

    let grayscale = vec4<f32>(vec3<f32>(average), sample.a);
    var out: vec4<f32> = clamp(color + grayscale, vec4<f32>(), vec4<f32>(1.0));
    out.w = sample.w * color.w;

    return out;
}

fn resolve_pixelated_uv(uv: vec2<f32>, color: vec4<f32>, frame: vec4<f32>) -> vec2<f32> {
    let block_size = mix(frame.zw * 0.5, vec2<f32>(0.0), color.a);

    if block_size.x == 0.0 || block_size.y == 0.0 {
        // avoid dividing by zero
        return uv;
    }

    // this value seems to work best for battle
    let scale_origin = vec2<f32>(frame.z * 0.5, 0.0);
    let centered_uv = uv - frame.xy - scale_origin;
    return trunc(centered_uv / block_size) * block_size + frame.xy + scale_origin;
}

@fragment
fn pixelate_adopt_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>, @location(2) frame: vec4<f32>) -> @location(0) vec4<f32> {
    let updated_uv = resolve_pixelated_uv(uv, color, frame);

    return vec4<f32>(color.rgb, sample_palette(updated_uv).a * color.a);
}

@fragment
fn pixelate_multiply_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>, @location(2) frame: vec4<f32>) -> @location(0) vec4<f32> {
    let updated_uv = resolve_pixelated_uv(uv, color, frame);

    return color * sample_palette(updated_uv);
}

@fragment
fn pixelate_add_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>, @location(2) frame: vec4<f32>) -> @location(0) vec4<f32> {
    let updated_uv = resolve_pixelated_uv(uv, color, frame);

    let sample = sample_palette(updated_uv);
    var out: vec4<f32> = clamp(color + sample, vec4<f32>(), vec4<f32>(1.0));
    out.a = sample.a * color.a;

    return out;
}
