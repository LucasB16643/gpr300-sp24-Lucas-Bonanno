#version 400 core

in vec3 position;

out vec4 clipSpace;
out vec2 textureCoords;
out vec3 toCameraVec;
out vec3 fromLightVec;

uniform mat4 _ViewProjection; 
uniform mat4 modelMatrix;
uniform vec3 cameraPosition;
uniform vec3 lightPosition;

const float tiling = 1.0;

void main() {

	vec4 worldPosition = modelMatrix * vec4(position, 1.0);

	clipSpace = _ViewProjection * modelMatrix * vec4(position, 1.0);
	gl_Position = clipSpace;

	textureCoords = vec2(position.x/2.0 +0.5, position.z/2.0 +0.5) * tiling;

	//textureCoords = position.xz;

	toCameraVec = cameraPosition - worldPosition.xyz;

	fromLightVec = worldPosition.xyz - lightPosition;
}
