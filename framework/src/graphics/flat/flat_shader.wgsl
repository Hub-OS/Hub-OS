struct CameraUniform {
    view_proj: mat4x4<f32>,
};

@group(0) @binding(0)
var<uniform> camera: CameraUniform;

struct VertexInput {
    @location(0) vertex: vec2<f32>,
    @location(1) transform0: vec3<f32>,
    @location(2) transform1: vec3<f32>,
    @location(3) transform2: vec3<f32>,
    @location(4) color: vec4<f32>,
};

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec4<f32>,
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
    v_out.position = camera.view_proj * vec4<f32>(transformed_position.x, transformed_position.y, 0.0, 1.0);

    v_out.color = v_in.color;

    return v_out;
}


@fragment
fn fs_main(@location(0) color: vec4<f32>) -> @location(0) vec4<f32> {
    return color;
}
