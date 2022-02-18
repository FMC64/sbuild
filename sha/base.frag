#version 460
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec3 o;

layout(set = 0, binding = 0) readonly buffer Sp {
	vec2 norm;
	vec2 off;
	float sps[96000];	// TODO: use spec constants
} s;

void main(void)
{
	vec2 p = (gl_FragCoord.xy + s.off) * s.norm;
	float len = clamp(sqrt(dot(p, p)), 0.0, 1.0);
	//len = pow(len, 0.1);
	uint off = uint(len * (96000 - 8000));
	float v = 0.0;
	uint mx = uint(2400.0 * pow(1.0 - len, 4.0));
	for (uint i = 0; i < mx; i++)
		v += abs(s.sps[off + i * 3]);
	v /= mx;
	v *= 8.0;	// track multiplier
	v = pow(v + .5, 8);
	o = vec3(abs(v) * pow(1.01 - len, 20.0));
}