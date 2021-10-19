#version 410

uniform sampler2D diffuse_texture;
uniform int has_diffuse_texture;

uniform float minSpeed;
uniform float maxSpeed;

in VS_OUT {
	vec2 texcoord;
	float speed;
	float directionDot;
} fs_in;

out vec4 frag_color;

void main()
{
	if (fs_in.speed < -0.00002) {
		frag_color = vec4(0.0, 0.5, 0.0, 0.5);
	} else {
		float speedNorm = (fs_in.speed - minSpeed) / (maxSpeed - minSpeed);
		float posNorm = (fs_in.directionDot * 0.5 + 0.5);

		float ratio = 0.8;
		float speedAndPos = (speedNorm * ratio * 2) + posNorm * (1 - ratio) * 2;

		frag_color = vec4(1.0, min((0.5 + speedNorm) * posNorm, 1.0), max(posNorm * 1.5 - 2.0 + speedNorm, 0.0), 0.5);
	}
	
}
