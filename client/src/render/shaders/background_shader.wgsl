struct VertexInput {
    @location(0) vertex: vec2<f32>,
    @location(1) scale: vec2<f32>,
    @location(2) offset: vec2<f32>,
    @location(3) bounds: vec4<f32>,
};

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
    @location(1) bounds: vec4<f32>,
};

@vertex
fn vs_main(v_in: VertexInput) -> VertexOutput {
    var v_out : VertexOutput;

    var position : vec2<f32> = v_in.vertex;
    position.x = position.x * 2.0 - 1.0;
    position.y = 1.0 - position.y * 2.0;

    v_out.position = vec4<f32>(position.x, position.y, 0.0, 1.0);
    v_out.uv = (v_in.vertex + v_in.offset) * v_in.scale;
    v_out.bounds = v_in.bounds;

    return v_out;
}


@group(1) @binding(0)
var txture: texture_2d<f32>;
@group(1) @binding(1)
var smplr: sampler;

@fragment
fn fs_main(v_out: VertexOutput) -> @location(0) vec4<f32> {
    var uv: vec2<f32> = v_out.uv;

    uv.x %= v_out.bounds.z;
    uv.y %= v_out.bounds.w;
    uv.x += v_out.bounds.x;
    uv.y += v_out.bounds.y;

    return textureSample(txture, smplr, uv);
}
