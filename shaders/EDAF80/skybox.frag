#version 410

in VS_OUT {
	vec3 texcoord;
} fs_in;

out vec4 frag_color;

uniform samplerCube my_cube_map;

void main()
{
	frag_color = texture(my_cube_map, fs_in.texcoord);
}
