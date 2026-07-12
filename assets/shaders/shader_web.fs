#version 100

precision mediump float;

// Interpolated from the vertex shader
varying vec2 fragTexCoord;
varying vec4 fragColor;

// Uniforms
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Screen dimensions
const float renderWidth = 720.0;
const float renderHeight = 720.0;

#define PI 3.14159265359

void main()
{
    // Convert UV from [0,1] to [-1,1]
    vec2 p = fragTexCoord * 2.0 - 1.0;

    // Fisheye distortion
    float curvature = 0.02;
    p *= 1.0 + curvature * dot(p, p);

    // Convert back to texture coordinates
    vec2 uv = p * 0.485 + 0.5;

    // Outside screen -> transparent
    if (uv.x < 0.0 || uv.x > 1.0 ||
        uv.y < 0.0 || uv.y > 1.0)
    {
        gl_FragColor = vec4(0.0);
        return;
    }

    // Sample texture
    vec4 texelColor = texture2D(texture0, uv);

    // Slight CRT tint
    texelColor *= vec4(0.85, 0.80, 0.95, 1.0);

    // Scanlines
    float frequency = renderHeight * 0.5;
    float scanline = 1.0 + 0.15 * cos(uv.y * frequency * PI);

    texelColor *= scanline;

    // Final output
    gl_FragColor = texelColor * colDiffuse * fragColor;
}