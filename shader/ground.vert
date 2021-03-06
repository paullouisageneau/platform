#version 130

uniform mat4 transform;
uniform mat4 modelview;
uniform vec3 lightPosition;

in vec3 position;
in vec3 normal;
in float environment;

out vec3 fragPosition;
out vec3 fragNormal;
out vec3 fragLight;
out float fragEnvironment;

void main() 
{
	fragPosition = position;
	fragNormal = normal;
	fragLight = normalize(position-lightPosition);
	fragEnvironment = environment;
	
	gl_Position = transform*vec4(position, 1.0);
}

