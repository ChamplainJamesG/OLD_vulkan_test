
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 lightPos;
layout(location = 3) in vec4 lightCol;
layout(location = 4) in vec3 vNormal;
layout(location = 5) in vec3 vPosition;
layout(location = 6) in vec3 vUV;
layout(location = 7) in vec4 debug;



layout(location = 0) out vec4 rtFragColor;
layout(binding = 1) uniform sampler2D texSampler;

vec4 phongCalc()
{
	vec4 P = vec4(vPosition, 1.);
	vec3 N = vNormal;
	vec3 L = lightPos.xyz;
	vec3 V = -P.xyz;

	N = normalize(N);
	L = normalize(L);
	V = normalize(V);

	vec3 R = reflect(-L, N);

	vec3 diffuse = max(dot(N,L), 0.) * lightCol.xyz;
	vec3 specular = pow(max(dot(R,V), 0.), 16) * diffuse;

	vec4 ambient = vec4(.01, .01, .01, .01);

	vec4 total = ambient + vec4((diffuse + specular), diffuse);

	return total;
	//return vec4(diffuse, 1.);
	//return vec4(vNormal, 1.);
	//return vec4(specular, 1.f);
}

void main()
{
	//rtFragColor = vec4(fragColor, 1.0);
	//rtFragColor = vec4(fragTexCoord, 0.0, 1.0);
	//rtFragColor = vec4(vNormal, 1.0);
	//rtFragColor = texture(texSampler, fragTexCoord);
	//rtFragColor = vec4(fragTexCoord, 0.0, 1.f);
	//rtFragColor = vec4(vPosition, 1.);
	rtFragColor = phongCalc() * texture(texSampler, vUV.xy);
	//rtFragColor = lightPos;
	//rtFragColor = vec4(1., 0, 0, 1.);
	//rtFragColor = texture(texSampler, vUV.xy);
	//rtFragColor = vec4(debug.x, .5, .5, .1);
}