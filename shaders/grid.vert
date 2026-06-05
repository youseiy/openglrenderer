#version 330 core

out vec3 vNearPoint;
out vec3 vFarPoint;

uniform mat4 uView;
uniform mat4 uProjection;

const vec2 Positions[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0,  1.0)
);

vec3 Unproject(vec2 position, float depth)
{
    // Reconstruct the world-space ray represented by this screen-space vertex.
    vec4 point = inverse(uView) *
                 inverse(uProjection) *
                 vec4(position, depth, 1.0);
    return point.xyz / point.w;
}

void main()
{
    // A full-screen quad is generated without a vertex buffer.
    vec2 position = Positions[gl_VertexID];
    vNearPoint = Unproject(position, -1.0);
    vFarPoint = Unproject(position, 1.0);
    gl_Position = vec4(position, 0.0, 1.0);
}
