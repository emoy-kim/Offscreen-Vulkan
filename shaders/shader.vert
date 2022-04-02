#version 460

layout (binding = 0) uniform MVP
{
    mat4 WorldMatrix;
    mat4 ViewMatrix;
    mat4 ProjectionMatrix;
} mvp;

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec2 v_tex_coord;

layout (location = 0) out vec3 position_in_ec;
layout (location = 1) out vec3 normal_in_ec;
layout (location = 2) out vec2 tex_coord;

void main()
{
    vec4 e_position = mvp.ViewMatrix * mvp.WorldMatrix * vec4(v_position, 1.0f);
    vec4 e_normal = transpose( inverse( mvp.ViewMatrix * mvp.WorldMatrix ) ) * vec4(v_normal, 1.0f);
    position_in_ec = e_position.xyz;
    normal_in_ec = normalize( e_normal.xyz );

    tex_coord = v_tex_coord;

    gl_Position = mvp.ProjectionMatrix * mvp.ViewMatrix * mvp.WorldMatrix * vec4(v_position, 1.0);
}