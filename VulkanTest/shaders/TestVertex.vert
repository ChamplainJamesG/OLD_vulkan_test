
#version 450

// Uniforms
layout (set = 0, binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 proj;

	vec4 uLightPos;
	vec4 uLightCol;
} ubo;

// Attributes
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 aNormal;

//instance attributes
layout(location = 4) in vec3 aInstancePos;
layout(location = 5) in vec3 aInstanceRot;
layout(location = 6) in float aInstanceScale;
layout(location = 7) in int aTextureArrayLayer;

// Varyings (outs)
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 lightPos;
layout(location = 3) out vec4 lightCol;
layout(location = 4) out vec3 vNormal;
layout(location = 5) out vec3 vPosition;
layout(location = 6) out vec3 vUV;

layout(location = 7) out vec4 debug;

void main()
{
	// Instancing position?

	vUV = vec3(inTexCoord, aTextureArrayLayer);

	// Standard Rotation Matrix.
	mat3 mx, my, mz;
	// rotate around x
	float s = sin(aInstanceRot.x);
	float c = cos(aInstanceRot.x);

	mx[0] = vec3(c, s, .0);
	mx[1] = vec3(-s, c, .0);
	mx[2] = vec3(.0, .0, 1.0);

	// rotate around y
	s = sin(aInstanceRot.y);
	c = cos(aInstanceRot.y);

	my[0] = vec3(c, 0.0, s);
	my[1] = vec3(0.0, 1.0, 0.0);
	my[2] = vec3(-s, 0.0, c);
	
	// rotate around z
	s = sin(aInstanceRot.z);
	c = cos(aInstanceRot.z);	
	
	mz[0] = vec3(1.0, 0.0, 0.0);
	mz[1] = vec3(0.0, c, s);
	mz[2] = vec3(0.0, -s, c);
	
	mat3 rotMat = mz * my * mx;

	vec4 localPos = vec4(inPosition.xyz * rotMat, 1.);
	vec4 pos = vec4((localPos.xyz) + aInstancePos, 1.);

	gl_Position = ubo.proj * ubo.view * ubo.model * pos;
	fragColor = inColor;
	fragTexCoord = inTexCoord;

	//vec4 clipSpace = ubo.view * ubo.model * vec4(inPosition, 1.);
	vec3 normalizedNormal = mat3(ubo.view) * mat3(ubo.model) * inverse(rotMat) * aNormal;
	normalizedNormal /= length(normalizedNormal);

	pos = ubo.view * ubo.model * vec4(inPosition.xyz + aInstancePos, 1.);
	lightPos = (ubo.view) * (ubo.model) * ubo.uLightPos;
	lightPos -= pos;
	lightCol = ubo.uLightCol;
	vNormal = normalizedNormal;
	vPosition = -pos.xyz;

	debug = vec4(aInstancePos, 1.);
}