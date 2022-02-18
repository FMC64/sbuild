#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_8bit_storage : enable

layout(location = 0) out vec3 o;

layout(set = 0, binding = 0) readonly buffer Sp {
	uint h;
	uint8_t fb[];
} s;

void main(void)
{
	uvec2 p = uvec2(gl_FragCoord.xy);
	uint off = (p.x * s.h + p.y) * 4;
	o = vec3(uvec3(s.fb[off + 0], s.fb[off + 1], s.fb[off + 2])) / 255.0;
}