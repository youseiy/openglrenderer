#version 330 core

in vec3 vNearPoint;
in vec3 vFarPoint;

uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uCameraPosition;

out vec4 FragColor;

vec4 Grid(vec3 position, float scale)
{
    vec2 coordinate = position.xz / scale;
    // Screen-space derivatives keep line thickness stable while zooming.
    vec2 derivative = fwidth(coordinate);
    vec2 distanceToLine = abs(fract(coordinate - 0.5) - 0.5) / derivative;
    float line = min(distanceToLine.x, distanceToLine.y);
    float alpha = 1.0 - min(line, 1.0);

    return vec4(vec3(0.48, 0.56, 0.65), alpha);
}

void main()
{
    float denominator = vFarPoint.y - vNearPoint.y;
    if (abs(denominator) < 0.0001)
    {
        discard;
    }

    // Intersect the camera ray with the world-space XZ plane at y = 0.
    float rayDistance = -vNearPoint.y / denominator;
    if (rayDistance <= 0.0 || rayDistance >= 1.0)
    {
        discard;
    }

    vec3 worldPosition = vNearPoint + rayDistance * (vFarPoint - vNearPoint);
    vec4 fineGrid = Grid(worldPosition, 1.0);
    vec4 coarseGrid = Grid(worldPosition, 10.0);
    vec3 color = mix(fineGrid.rgb, coarseGrid.rgb, coarseGrid.a);
    float alpha = max(fineGrid.a * 0.32, coarseGrid.a * 0.58);

    float axisWidth = max(fwidth(worldPosition.x), fwidth(worldPosition.z));
    if (abs(worldPosition.x) < axisWidth)
    {
        color = vec3(0.18, 0.42, 0.90);
        alpha = 0.9;
    }
    if (abs(worldPosition.z) < axisWidth)
    {
        color = vec3(0.90, 0.22, 0.16);
        alpha = 0.9;
    }

    float distanceFromCamera = length(worldPosition.xz - uCameraPosition.xz);
    alpha *= 1.0 - smoothstep(100.0, 500.0, distanceFromCamera);
    if (alpha <= 0.001)
    {
        discard;
    }

    // Write the plane depth so scene geometry can occlude the procedural grid.
    vec4 clipPosition = uProjection * uView * vec4(worldPosition, 1.0);
    float normalizedDepth = clipPosition.z / clipPosition.w;
    gl_FragDepth = normalizedDepth * 0.5 + 0.5;
    FragColor = vec4(color, alpha);
}
