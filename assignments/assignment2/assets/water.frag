#version 400

in vec4 clipSpace;
in vec2 textureCoords;
in vec3 toCameraVec;
in vec3 fromLightVec;

out vec4 out_Color;

uniform sampler2D reflectionTexture;
uniform sampler2D refractionTexture;



uniform sampler2D dudv;
uniform float moveFactor;


uniform sampler2D normalMap;
uniform vec3 lightColor = vec3(1.0);
uniform float waveStrength = 0.02;
uniform float shineDamper = 10.0;
uniform float reflectivity = 0.6;

void main(){

vec2 ndc = (clipSpace.xy/clipSpace.w)/2.0+0.5;
vec2 reflectTexCoords = vec2 (ndc.x, ndc.y);
vec2 refractTexCoords = vec2 (ndc.x, ndc.y);
vec2 reflectTexCoordsO = vec2 (ndc.x, ndc.y);
vec2 refractTexCoordsO = vec2 (ndc.x, ndc.y);


vec2 distortedTexCoords = texture(dudv, vec2(textureCoords.x + moveFactor, textureCoords.y)).rg*0.1;
distortedTexCoords = textureCoords + vec2(distortedTexCoords.x, distortedTexCoords.y + moveFactor);
vec2 totalDistortion = (texture(dudv, distortedTexCoords).rg *2.0 - 1.0) * waveStrength;


reflectTexCoords += totalDistortion;
reflectTexCoords.x = clamp(reflectTexCoords.x, .001, .999);
reflectTexCoords.y = clamp(reflectTexCoords.y, .001, .999);

refractTexCoords += totalDistortion;
refractTexCoords = clamp(refractTexCoords, .001, .999);

vec4 reflectColor = texture(reflectionTexture, reflectTexCoords);
vec4 refractColor = texture(refractionTexture, refractTexCoords);
vec4 reflectColorO = texture(reflectionTexture, reflectTexCoordsO);
vec4 refractColorO = texture(refractionTexture, refractTexCoordsO);

vec3 viewVec = normalize(toCameraVec);
float refractiveFactor = dot(viewVec, vec3(0.0, 1.0, 0.0));
refractiveFactor = pow(refractiveFactor, 0.5);

vec4 normalMapColor = texture(normalMap, distortedTexCoords);
vec3 normal = vec3(normalMapColor.r * 2.0 - 1.0, normalMapColor.g * 2.0 - 1.0, normalMapColor.b * 2.0 - 1.0);
normal = normalize(normal);

vec3 reflectedLight = reflect(normalize(fromLightVec), normal);
	float specular = max(dot(reflectedLight, viewVec), 0.0);
	specular = pow(specular, shineDamper);
	vec3 specularHighlights = lightColor * specular * reflectivity;




out_Color = mix(reflectColor, refractColor, refractiveFactor) + vec4(specularHighlights, 0.0);
//out_Color = mix(reflectColor, refractColor, .5);
//out_Color = mix(out_Color, vec4(0.0, 0.3, 0.5, 1.0), 0.2) + vec4(specularHighlights, 0.0);
//out_Color = normalMapColor; // ONLY this
//out_Color = texture(normalMap, textureCoords);
}