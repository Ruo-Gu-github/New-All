#version 420

in vec3 Color;
layout(location = 0) out vec4 FragColor;

void main()
{
	FragColor = vec4(Color, 1.0);
	FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}