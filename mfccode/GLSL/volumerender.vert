#version 420


layout (location = 0) in vec3 VerPos;
layout (location = 1) in vec3 VerClr;  

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 EntryPoint;
out vec3 FragPos; 
out mat4 View;
out mat4 Model;


void main()
{
    gl_Position = projection * view * model * vec4(VerPos,1.0);
    EntryPoint = VerClr;
	FragPos = vec3(model * vec4(VerPos, 1.0f));
	View = view;
	Model = model;
}
