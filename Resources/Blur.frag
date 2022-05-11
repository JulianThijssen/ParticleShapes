#version 330 core

in vec3 pass_position;
in vec2 pass_texCoords;

out vec4 fragColor;

uniform vec2 windowSize;
uniform sampler2D tex;
uniform vec2 direction;
uniform int minLod;
uniform int numMipMaps;

vec3 blur(int lod, vec2 uv, vec2 resolution, vec2 direction) {
    vec3 color = vec3(0);
    vec2 off = vec2(1.0) * direction;
    color += textureLod(tex, uv, lod).rgb * 0.29411764705882354;
    color += textureLod(tex, uv + (off / resolution), lod).rgb * 0.35294117647058826;
    color += textureLod(tex, uv - (off / resolution), lod).rgb * 0.35294117647058826;
    return color;
}

void main()
{
    vec3 color = vec3(0);

    for (int i = minLod; i < minLod + numMipMaps; i++)
    {
        color += blur(i, pass_texCoords, windowSize / pow(2, i), direction);
    }
    color /= numMipMaps;

    fragColor = vec4(color, 1);
}
