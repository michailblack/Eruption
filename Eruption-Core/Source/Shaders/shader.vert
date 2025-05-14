#version 460
layout(location = 0) in vec2  a_Position;
layout(location = 1) in vec2  a_TexCoord;
layout(location = 2) in vec3  a_Color;

layout(location = 0) out vec3 v_FragmentColor;
layout(location = 1) out vec2 v_TexCoord;

layout(binding = 0) uniform PerView
{
  mat4 Model;
  mat4 View;
  mat4 Projection;
} u_PerView;

void main()
{
	gl_Position = u_PerView.Projection * u_PerView.View * u_PerView.Model * vec4(a_Position, 0.0, 1.0);
	v_FragmentColor = a_Color;
    v_TexCoord = a_TexCoord;
}
