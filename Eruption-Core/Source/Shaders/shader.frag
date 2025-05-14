#version 450
layout(location = 0) in vec3 v_FragmentColor;
layout(location = 1) in vec2 v_TexCoord;

layout(location = 0) out vec4 o_Color;

layout(binding = 1) uniform sampler2D u_TextureSample;

void main()
{
    o_Color = texture(u_TextureSample, v_TexCoord);
}
