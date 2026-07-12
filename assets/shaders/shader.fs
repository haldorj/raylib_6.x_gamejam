#version 330

// Input vertex attributes
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniforms
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// Screen dimensions
const float renderWidth = 720.0;
const float renderHeight = 720.0;

#define PI 3.14159265359

void main()
{
	// Fisheye
    vec2 p = fragTexCoord * 2.0 - 1.0;
    float curvature = 0.02;
    p *= 1.0 + curvature * dot(p, p);

    vec2 uv = p * 0.485 + 0.5;

    // Black outside the warped screen
    if (uv.x < 0.0 || uv.x > 1.0 ||
        uv.y < 0.0 || uv.y > 1.0)
    {
        finalColor = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }
    vec4 texelColor = texture(texture0, uv) * vec4(0.85, 0.8, 0.95, 1.0);

	// Scanlines
    float frequency = renderHeight / 2;
    float scanline = 1.0 + 0.15 * cos(uv.y * frequency * PI);

    texelColor *= scanline;

    finalColor = texelColor * colDiffuse;
}