#version 410

// Remember how we enabled vertex attributes in assignment 2 and attached some
// data to each of them, here we retrieve that data. Attribute 0 pointed to the
// vertices inside the OpenGL buffer object, so if we say that our input
// variable `vertex` is at location 0, which corresponds to attribute 0 of our
// vertex array, vertex will be effectively filled with vertices from our
// buffer.
// Similarly, normal is set to location 1, which corresponds to attribute 1 of
// the vertex array, and therefore will be filled with normals taken out of our
// buffer.
layout (location = 0) in vec3 vertex;
layout (location = 2) in vec3 texcoord;
//layout (location = 1) in vec3 normal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform float t;

// This is the custom output of this shader. If you want to retrieve this data
// from another shader further down the pipeline, you need to declare the exact
// same structure as in (for input), with matching name for the structure
// members and matching structure type. Have a look at
// shaders/EDAF80/diffuse.frag.
out VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec2 texcoord;
	mat3 TBN;
} vs_out;

float wave(vec2 position, vec2 direction, float amplitude, float frequency, float phase, float sharpness, float time)
{
	 return amplitude * pow(sin((position.x * direction.x + position.y * direction.y) * frequency + time * phase) * 0.5 + 0.5, sharpness);
}

vec3 wave_normal(vec2 position, vec2 direction, float amplitude, float frequency, float phase, float sharpness, float time)
{
	vec3 normal = vec3(0.0, 1.0, 0.0);
	normal.x = -amplitude * sharpness * pow(sin((position.x * direction.x + position.y * direction.y) * frequency + time * phase) * 0.5, sharpness - 1.0)
				* cos((position.x * direction.x + position.y * direction.y) * frequency + time * phase) * 0.5
				* frequency * direction.x;
	normal.z = -amplitude * sharpness * pow(sin((position.x * direction.x + position.y * direction.y) * frequency + time * phase) * 0.5, sharpness - 1.0)
				* cos((position.x * direction.x + position.y * direction.y) * frequency + time * phase) * 0.5
				* frequency * direction.y;
	return normal;
} 

void main()
{
	vec3 displaced_vertex = vertex;
	vec3 displaced_vertex_normal = vec3(0.0);

	displaced_vertex.y += wave(vertex.xz, vec2(-1.0, 0.0), 1.0, 0.2, 0.5, 2.0, t);
	displaced_vertex.y += wave(vertex.xz, vec2(-0.7, 0.7), 0.5, 0.4, 1.3, 2.0, t);

	displaced_vertex_normal += wave_normal(vertex.xz, vec2(-1.0, 0.0), 1.0, 0.2, 0.5, 2.0, t);
	displaced_vertex_normal += wave_normal(vertex.xz, vec2(-0.7, 0.7), 0.5, 0.4, 1.3, 2.0, t);
	displaced_vertex_normal.y = 1.0;

	vs_out.normal = normalize(displaced_vertex_normal);
	vec3 tangent = normalize(vec3(1, -displaced_vertex_normal.x, 0));
	vec3 binormal = normalize(vec3(0, -displaced_vertex_normal.z, 1));
	vs_out.TBN = mat3(tangent, binormal, displaced_vertex_normal);

	// Vertex
	vs_out.vertex = vec3(vertex_model_to_world * vec4(displaced_vertex, 1.0));

	// Texture coordinates
	vs_out.texcoord = vec2(texcoord.x, texcoord.y);

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(displaced_vertex, 1.0);
}





