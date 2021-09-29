#version 410

uniform vec3 light_position;
uniform float t;
uniform vec3 camera_position;

uniform int use_reflection_mapping;
uniform int use_animated_normal_mapping;
uniform int use_fresnel_factor;
uniform int use_refraction_mapping;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec2 texcoord;
	mat3 TBN;
} fs_in;

out vec4 frag_color;

uniform samplerCube my_cube_map;
uniform sampler2D my_texture_normal;

void main()
{
	vec3 normal = fs_in.normal;

	// Creating view and light vector
	vec3 V = normalize(camera_position - fs_in.vertex);
	vec3 L = normalize(light_position - fs_in.vertex);

	// Animated normal mapping
	vec2 texScale = vec2(8.0, 4.0);
	float normalTime = mod(t, 100.0);
	vec2 normalSpeed = vec2(-0.05, 0);

	// Fresnel configuration
	float r0 = 0.02037;

	// Normal mapping variables
	vec2 normalCoord0;
	vec2 normalCoord1;
	vec2 normalCoord2;
	
	normalCoord0.xy =
	fs_in.texcoord.xy*texScale + normalTime*normalSpeed;
	normalCoord1.xy =
	fs_in.texcoord.xy*texScale*2 + normalTime*normalSpeed*4;
	normalCoord2.xy =
	fs_in.texcoord.xy*texScale*4 + normalTime*normalSpeed*8;

	// Base colors
	vec4 color_deep = vec4(0.0, 0.0, 0.1, 1.0);
	vec4 color_shallow = vec4(0.0, 0.5, 0.5, 1.0);


	// Animated normal mapping
	if (use_animated_normal_mapping == 1){
		vec3 normal0 = texture(my_texture_normal, normalCoord0).rgb * 2.0 - 1.0;
		vec3 normal1 = texture(my_texture_normal, normalCoord1).rgb * 2.0 - 1.0;
		vec3 normal2 = texture(my_texture_normal, normalCoord2).rgb * 2.0 - 1.0;

		normal = normalize(normal0 + normal1 + normal2);
		normal = fs_in.TBN * normal;
	}

	frag_color = mix(color_deep, color_shallow, 1.0-max(dot(V, normal), 0));

	float fresnel;

	// Reflection mapping
	if (use_reflection_mapping == 1) {
		vec4 reflection;
		reflection = texture(my_cube_map, reflect(-V, normal));
		if (use_fresnel_factor == 1) {
			fresnel = r0 + (1 - r0) * pow(1.0 - dot(V, normal), 5);
			reflection = reflection * fresnel;
		}
		frag_color += reflection;
	}
	if (use_refraction_mapping == 1) {
		vec4 refraction = texture(my_cube_map, refract(-V, normal, 1/1.33));
		frag_color += refraction * (1.0 - fresnel);
	}
	//frag_color = vec4(vec3(normal.x * 0.5 + 0.5, pow(normal.y * 0.5 + 0.5, 15), normal.z * 0.5 + 0.5), 1.0);
}
