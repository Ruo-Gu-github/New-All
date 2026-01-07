#version 420

layout(location = 0) in vec3 VerPos;
layout(location = 1) in vec3 VerClr;

out vec3 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
Color = VerClr;
	gl_Position = projection * view * model * vec4(VerPos, 1.0f);
	//gl_Position = view * vec4(VerPos, 1.0f);
}