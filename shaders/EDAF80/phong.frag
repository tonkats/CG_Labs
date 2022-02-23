#version 410

in VS_OUT {
	vec2 texcoord;
	vec3 vertex;
	vec3 normal;
	mat3 TBN;
} fs_in;

out vec4 frag_color;

uniform sampler2D my_texture;
uniform sampler2D my_texture_normal;
uniform sampler2D my_texture_rough;

uniform int use_normal_mapping;
uniform vec3 light_position;
uniform vec3 camera_position;
uniform vec3 ambient_colour;
uniform vec3 diffuse_colour;
uniform vec3 specular_colour;
uniform float shininess_value;

uniform int has_my_texture_normal;
uniform int has_my_texture;
uniform int has_my_texture_rough;


void main()
{
	vec3 view_position = camera_position - fs_in.vertex;
	vec3 normal = fs_in.normal;
	if (has_my_texture_normal == 1) {
		normal = texture(my_texture_normal, fs_in.texcoord).rgb;
		normal = normal * 2.0 - 1.0;
		normal = normalize(fs_in.TBN * normal);
	}
	vec3 L = normalize(light_position - fs_in.vertex);

	// Set to specular_colour if not included
	vec3 specular_rough = texture(my_texture_rough, fs_in.texcoord).rgb;

	vec3 diffuse = diffuse_colour;
	vec3 diffuse_light = vec3(1.0) * clamp(dot(normal, L), 0, 1);
	if (has_my_texture == 1) {
		diffuse = texture(my_texture, fs_in.texcoord).xyz;
	}
	frag_color = vec4(ambient_colour * diffuse, 1.0)
					+ vec4(diffuse*diffuse_light, 1.0)
					+ vec4(specular_rough * pow(clamp(dot(normalize(view_position), reflect(-L, normal)), 0, 1), shininess_value), 1);

}
