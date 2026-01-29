#include FRAME_UNIFORMS
// struct FrameUniforms {
//     // scene params (only set in ScenePass, otherwise 0)
//     projection: mat4x4f,
//     view: mat4x4f,
//     projection_view_inverse_no_translation: mat4x4f,
//     camera_pos: vec3f,
//     ambient_light: vec3f,
//     num_lights: i32,
//     background_color: vec4f,

//     // general params (include in all passes except ComputePass)
//     resolution: vec3i,      // window viewport resolution 
//     time: f32,              // time in seconds since the graphics window was opened
//     delta_time: f32,        // time since last frame (in seconds)
//     frame_count: i32,       // frames since window was opened
//     mouse: vec2f,           // normalized mouse coords (range 0-1)
//     mouse_click: vec2i,     // mouse click state
//     sample_rate: f32        // chuck VM sound sample rate (e.g. 44100)
// };
// @group(0) @binding(0) var<uniform> u_frame: FrameUniforms;

@group(1) @binding(0) var u_texture: texture_2d<f32>;
// @group(1) @binding(1) var u_sampler: sampler;

struct VertexOutput {
    @builtin(position) position : vec4<f32>,
    @location(0) v_uv : vec2<f32>,
};


@vertex 
fn vs_main(@builtin(vertex_index) vertexIndex : u32) -> VertexOutput {
    var output : VertexOutput;

    // a triangle which covers the screen
    output.v_uv = vec2f(f32((vertexIndex << 1u) & 2u), f32(vertexIndex & 2u));
    output.position = vec4f(output.v_uv * 2.0 - 1.0, 0.0, 1.0);
    
    return output;
}

@fragment 
fn fs_main(in : VertexOutput) -> @location(0) vec4f {
    let t0 = u_frame;
    var uv = in.position.xy / vec2f(u_frame.resolution.xy); // interesting. fragCoord doesn't change based on viewport
    uv.y = 1.0 - uv.y;

    let tex_dims = textureDimensions(u_texture);
    let samp_x = u32(uv.x * f32(tex_dims.x));
    let samp_y = u32(uv.y * f32(tex_dims.y));
    let samp = u32(255.0 * ((textureLoad(u_texture, vec2u(samp_x, samp_y), 0).r + 1.0) * 0.5));

    let bit = u32(fract(uv.y * f32(tex_dims.y)) * 8);
    let bit_bucket = (samp >> bit) & 1;


    
    // return vec4(uv, 0.0, 1.0);
    return vec4(f32(bit_bucket));
}
