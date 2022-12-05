#version 430 core

layout (binding = 0) uniform sampler2D tex_main;

in vec2 texcoord;

layout (location = 0) out vec4 color;

void main(void)
{
	vec4 oColor = textureLod(tex_main, texcoord, 0);
	color = vec4(oColor);
}