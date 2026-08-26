struct Uniforms {
    view_proj: mat4x4<f32>,
}

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) color: vec3<f32>,
    @location(2) normal: vec3<f32>,
}

struct Instance {
    @location(3) model_matrix_x: vec4<f32>,
    @location(4) model_matrix_y: vec4<f32>,
    @location(5) model_matrix_z: vec4<f32>,
    @location(6) model_matrix_t: vec4<f32>,
    @location(7) color: vec4<f32>,
}

struct VertexOutput {
    @builtin(position) clip_position: vec4<f32>,
    @location(0) color: vec4<f32>,
    @location(1) world_normal: vec3<f32>,
}

@group(0) @binding(0)
var<uniform> uniforms: Uniforms;
override encode_output_srgb: bool = false;

fn linear_to_srgb_component(linear_value: f32) -> f32 {
    let value = clamp(linear_value, 0.0, 1.0);
    if (value <= 0.0031308) {
        return 12.92 * value;
    }

    return 1.055 * pow(value, 1.0/2.4) - 0.055;
}

fn encode_output_rgb(linear_rgb: vec3<f32>) -> vec3<f32> {
    if (!encode_output_srgb) {
        return linear_rgb;
    }

    return vec3<f32>(
        linear_to_srgb_component(linear_rgb.r),
        linear_to_srgb_component(linear_rgb.g),
        linear_to_srgb_component(linear_rgb.b),
    );
}

@vertex
fn vs_main(input: VertexInput, instance: Instance) -> VertexOutput {
    let model_matrix = mat4x4<f32>(
        instance.model_matrix_x,
        instance.model_matrix_y,
        instance.model_matrix_z,
        instance.model_matrix_t,
    );

    var output: VertexOutput;
    output.clip_position = uniforms.view_proj * model_matrix * vec4<f32>(input.position, 1.0);
    output.color = instance.color;
    output.world_normal = normalize((model_matrix * vec4<f32>(input.normal, 0.0)).xyz);
    return output;
}

// Fragment shader for outline render pass
@fragment
fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {
    return vec4<f32>(
        encode_output_rgb(input.color.rgb),
        input.color.a,
    );
}
