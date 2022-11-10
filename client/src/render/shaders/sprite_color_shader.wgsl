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

@vertex
fn vs_main(v_in: VertexInput) -> VertexOutput {
    var v_out : VertexOutput;

    let transform = mat3x3<f32>(
        v_in.transform0,
        v_in.transform1,
        v_in.transform2,
    );

    let transformed_position = transform * vec3<f32>(v_in.vertex.x, v_in.vertex.y, 1.0);

    v_out.position =
        camera.view_proj * vec4<f32>(transformed_position.x, transformed_position.y, 0.0, 1.0);

    v_out.uv.x = v_in.bounds.x + v_in.uv.x * v_in.bounds.z;
    v_out.uv.y = v_in.bounds.y + v_in.uv.y * v_in.bounds.w;
    v_out.color = v_in.color;

    return v_out;
}


@group(1) @binding(0)
var txture: texture_2d<f32>;
@group(1) @binding(1)
var smplr: sampler;

@fragment
fn replace_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>) -> @location(0) vec4<f32> {
    return color * textureSample(txture, smplr, uv).w;
}

@fragment
fn multiply_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>) -> @location(0) vec4<f32> {
    return color * textureSample(txture, smplr, uv);
}

@fragment
fn add_main(@location(0) uv: vec2<f32>, @location(1) color: vec4<f32>) -> @location(0) vec4<f32> {
    let sampled = textureSample(txture, smplr, uv);
    var out: vec4<f32> = clamp(color + sampled, vec4<f32>(), vec4<f32>(1.0));
    out.w = sampled.w * color.w;

    return out;
}
