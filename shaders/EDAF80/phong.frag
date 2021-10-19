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
uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;


void main()
{
	vec3 view_position = camera_position - fs_in.vertex;
	vec3 normal = fs_in.normal;
	//if (use_normal_mapping == 1){
	normal = texture(my_texture_normal, fs_in.texcoord).rgb;
	normal = normal * 2.0 - 1.0;
	normal = normalize(fs_in.TBN * normal);
	//}
	vec3 L = normalize(light_position - fs_in.vertex);

	vec3 specular_rough = texture(my_texture_rough, fs_in.texcoord).rgb;

	vec3 diffuse_light = vec3(1.0) * clamp(dot(normal, L), 0, 1);
	frag_color = vec4(ambient, 1)
					+ texture(my_texture, fs_in.texcoord) * vec4(diffuse_light, 1)
					+ vec4(specular_rough * pow(clamp(dot(normalize(view_position), reflect(-L, normal)), 0, 1), shininess), 1);

}
