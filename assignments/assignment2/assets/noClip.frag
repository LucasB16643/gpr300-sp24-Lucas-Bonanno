#version 450
out vec4 FragColor; //The color of this fragment

in Surface{
vec3 WorldPos;
	vec3 WorldNormal;
	vec2 TexCoord;
	vec4 FragPosLightSpace;
}fs_in;

uniform sampler2D _MainTex; 
uniform sampler2D _ShadowMap;
//Light pointing straight down
uniform vec3 _EyePos;
uniform vec3 _LightDirection = vec3(0.0,-1.0,0.0);
uniform vec3 _LightColor = vec3(1.0); //White light
uniform vec3 _AmbientColor = vec3(0.3,0.4,0.46);
uniform vec3 _LightPos = vec3(-2.0f, 4.0f, -1.0f);
uniform float minBias;
uniform float maxBias;
struct Material{
	float Ka; //Ambient coefficient (0-1)
	float Kd; //Diffuse coefficient (0-1)
	float Ks; //Specular coefficient (0-1)
	float Shininess; //Affects size of specular highlight
};
uniform Material _Material;


float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(_ShadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
	 float nBias = max(maxBias * (1.0 - dot(normalize(fs_in.WorldNormal), _LightDirection)), minBias);
    float shadow = 0.0;
	
	vec2 texelSize = 1.0 / textureSize(_ShadowMap, 0);
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(_ShadowMap, projCoords.xy + vec2(x,y) * texelSize).r;
			shadow += currentDepth - nBias > pcfDepth ? 1.0: 0.0;
		}
	}

	shadow = shadow / 9;

	if(projCoords.z > 1.0){
        shadow = 0.0;
	}

	return shadow;
}

void main(){
vec3 color = texture(_MainTex, fs_in.TexCoord).rgb;
	//Make sure fragment normal is still length 1 after interpolation.

	vec3 normal = normalize(fs_in.WorldNormal);
	//Light pointing straight down

	vec3 lightDir = normalize(_LightPos - fs_in.WorldPos);
	float diffuseFactor = max(dot(normal,lightDir),0.0);

	vec3 toEye = normalize(_EyePos - fs_in.WorldPos);
	vec3 h = normalize(lightDir + toEye);
	float specularFactor = pow(max(dot(normal,h),0.0),_Material.Shininess);

	float shadow = ShadowCalculation(fs_in.FragPosLightSpace); 
	

	//Amount of light diffusely reflecting off surface
	vec3 dsColor = (_Material.Kd * diffuseFactor + _Material.Ks * specularFactor) * _LightColor;
	vec3 ambient = _AmbientColor * _Material.Ka;
	
	vec3 objectColor = texture(_MainTex,fs_in.TexCoord).rgb;
	float shadowFactor = 0.5;
	
	
	vec3 lighting = (ambient + (1.0 - shadow) * dsColor) * objectColor;
	FragColor = vec4(lighting,1.0);
	
	

}
