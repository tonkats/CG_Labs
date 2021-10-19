#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;
layout (location = 5) in float speed;
layout (location = 6) in vec3 direction;

uniform mat4 vertex_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform float minSpeed;
uniform float maxSpeed;
uniform float t;

out VS_OUT {
	vec2 texcoord;
	float speed;
	float directionDot;
} vs_out;


void main()
{
	vec3 displacedVertex = vertex;

	vs_out.texcoord = vec2(texcoord.x, texcoord.y);
	vs_out.speed = speed;

	float speedNorm = (speed - minSpeed) / (maxSpeed - minSpeed);
	float directionDot = dot(normalize(direction), normalize(vertex));
	vs_out.directionDot = directionDot;

	float displacement = 1.0;
	if (directionDot < 0) {
		displacedVertex = displacedVertex - pow(abs(directionDot), 3) * direction * 10;
	}
	displacedVertex = displacedVertex + normal * sin(t * 30 * speedNorm + directionDot * 10) * 0.2;
	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(displacedVertex, 1.0);
}
