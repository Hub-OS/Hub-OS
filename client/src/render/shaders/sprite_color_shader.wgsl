struct CameraUniform {
    view_proj: mat4x4<f32>,
};

@group(0) @binding(0)
var<uniform> camera: CameraUniform;

struct VertexInput {
    @location(0) vertex: vec2<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) transform0: vec3<f32>,
    @location(3) transform1: vec3<f32>,
    @location(4) transform2: vec3<f32>,
    @location(5) bounds: vec4<f32>,
    @location(6) color: vec4<f32>,
};

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
    @location(1) color: vec4<f32>,
};


struct PixelateVertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
    @location(1) color: vec4<f32>,
    @location(2) frame: vec4<f32>,
};

fn plain_vs_main(v_in: VertexInput) -> VertexOutput {
    var v_out: VertexOutput;

    let transform = mat3x3<f32>(
        v_in.transform0,
        v_in.transform1,
        v_in.transform2,
    );

    let transformed_position = transform * vec3<f32>(v_in.vertex.x, v_in.vertex.y, 1.0);

    v_out.position = camera.view_proj * vec4<f32>(transformed_position.x, transformed_position.y, 0.0, 1.0);

    v_out.uv = v_in.bounds.xy + v_in.uv.xy * v_in.bounds.zw;
    v_out.color = v_in.color;

    return v_out;
}

@vertex
fn vs_main(v_in: VertexInput) -> VertexOutput {
    return plain_vs_main(v_in);
}

@vertex
fn pixelate_vs_main(v_in: VertexInput) -> PixelateVertexOutput {
    let plain_v_out = plain_vs_main(v_in);

    var v_out: PixelateVertexOutput;
    v_out.position = plain_v_out.position;
    v_out.uv = plain_v_out.uv;
    v_out.color = plain_v_out.color;
    v_out.frame = v_in.bounds;

    return v_out;
}

@group(1) @binding(0)
var txture: texture_2d<f32>;
@group(1) @binding(1)
var smplr: sampler;

@fragment
fn adopt_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>) -> @location(0) vec4<f32> {
    return vec4<f32>(color.rgb, textureSample(txture, smplr, uv).a);
}

@fragment
fn multiply_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>) -> @location(0) vec4<f32> {
    return color * textureSample(txture, smplr, uv);
}

@fragment
fn add_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>) -> @location(0) vec4<f32> {
    let sample = textureSample(txture, smplr, uv);
    var out: vec4<f32> = clamp(color + sample, vec4<f32>(), vec4<f32>(1.0));
    out.a = sample.a * color.a;

    return out;
}

// no grascale_adopt_main implementation, adopt_main is used instead

@fragment
fn grayscale_multiply_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>) -> @location(0) vec4<f32> {
    let sample = textureSample(txture, smplr, uv);
    let average = (sample.r + sample.g + sample.b) * 0.33333;
    return color * vec4<f32>(vec3<f32>(average), sample.a);
}

@fragment
fn grayscale_add_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>) -> @location(0) vec4<f32> {
    let sample = textureSample(txture, smplr, uv);
    let average = (sample.r + sample.g + sample.b) * 0.33333;

    let grayscale = vec4<f32>(vec3<f32>(average), sample.a);
    var out: vec4<f32> = clamp(color + grayscale, vec4<f32>(), vec4<f32>(1.0));
    out.a = sample.a * color.a;

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

    return vec4<f32>(color.rgb, textureSample(txture, smplr, updated_uv).a);
}

@fragment
fn pixelate_multiply_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>, @location(2) frame: vec4<f32>) -> @location(0) vec4<f32> {
    let updated_uv = resolve_pixelated_uv(uv, color, frame);

    return color * textureSample(txture, smplr, updated_uv);
}

@fragment
fn pixelate_add_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>, @location(2) frame: vec4<f32>) -> @location(0) vec4<f32> {
    let updated_uv = resolve_pixelated_uv(uv, color, frame);

    let sample = textureSample(txture, smplr, updated_uv);
    var out: vec4<f32> = clamp(color + sample, vec4<f32>(), vec4<f32>(1.0));
    out.a = sample.a * color.a;

    return out;
}
