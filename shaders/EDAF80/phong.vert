#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform vec3 light_position;

out VS_OUT {
	vec2 texcoord;
	vec3 vertex;
	vec3 normal;
	mat3 TBN;
} vs_out;


void main()
{
	vs_out.texcoord = vec2(texcoord.x, texcoord.y);
	vs_out.vertex = vec3(vertex_model_to_world * vec4(vertex, 1.0));

	vec3 normal = normalize(vec3(normal_model_to_world * vec4(normal, 0.0)));
	vec3 tangent = normalize(vec3(normal_model_to_world * vec4(tangent, 0.0)));
	vec3 binormal = normalize(vec3(normal_model_to_world * vec4(binormal, 0.0)));
	vs_out.normal = normal;

	vs_out.TBN = mat3(tangent, binormal, normal);

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}



