#version 100

precision mediump float;

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

const float renderWidth = 720.0;
const float renderHeight = 720.0;

#define PI 3.14159265359

void main()
{
    vec2 p = fragTexCoord * 2.0 - 1.0;
    float curvature = 0.02;
    p *= 1.0 + curvature * dot(p, p);
    vec2 uv = p * 0.485 + 0.5;
    if (uv.x < 0.0 || uv.x > 1.0 ||
        uv.y < 0.0 || uv.y > 1.0)
    {
        gl_FragColor = vec4(0.0);
        return;
    }

    vec4 texelColor = texture2D(texture0, uv);
    texelColor *= vec4(0.85, 0.80, 0.95, 1.0);
    float frequency = renderHeight * 0.5;
    float scanline = 1.0 + 0.15 * cos(uv.y * frequency * PI);

    texelColor *= scanline;

    gl_FragColor = texelColor * colDiffuse * fragColor;
}