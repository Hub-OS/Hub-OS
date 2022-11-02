struct VertexInput {
    @location(0) vertex: vec2<f32>,
    @location(1) color: vec4<f32>,
};

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec4<f32>,
};

@vertex
fn vs_main(v_in: VertexInput) -> VertexOutput {
    var v_out : VertexOutput;

    v_out.position = vec4<f32>(v_in.vertex.x, v_in.vertex.y, 0.0, 1.0);
    v_out.color = v_in.color;

    return v_out;
}

@fragment
fn fs_main(f_in: VertexOutput) -> @location(0) vec4<f32> {
    return f_in.color;
}
