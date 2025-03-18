#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} fs_in;

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform vec3 _eyePos;


uniform vec3 _lightDirection = vec3(0.0, -1.0, 0.0);
uniform vec3 _lightColor = vec3(1.0);//white light

uniform vec3 _ambientColor = vec3(0.3, 0.4, 0.46);

uniform float minBias;
uniform float maxBias;

struct Material{
	float Ka; // ambient coefficient (0-1)
	float Kd; // diffuse coefficient (0-1)
	float Ks; // specular coefficient (0-1)
	float Shininess; //affects size of specular highlight
};
uniform Material _Material;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float shadow = currentDepth > closestDepth  ? 1.0 : 0.0;

    return shadow;
}

void main()
{   
    vec3 normal = normalize(fs_in.Normal);
    vec3 toLight = _lightDirection;

    float diff = max(dot(_lightDirection, normal), 0.0);
     vec3 eyeDir = normalize(_eyePos - fs_in.FragPos);
     vec3 h = normalize(toLight + eyeDir);
     float specularFactor = pow(max(dot(normal, h),0.0), _Material.Shininess);
     vec3 dsColor = _lightColor * (_Material.Kd * diff + _Material.Ks * specularFactor);
     vec3 ambient = _ambientColor * _Material.Ka;
	vec3 objectColor = texture(diffuseTexture, fs_in.TexCoords).rgb;
    float shadow = ShadowCalculation(fs_in.FragPosLightSpace);
	vec3 lighting = (ambient + (1.0 - shadow) * dsColor) * objectColor;
    FragColor = vec4(lighting, 1.0);


    /*vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightColor = vec3(1.0);
    // ambient
    vec3 ambient = 0.15 * lightColor;
    // diffuse
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // specular
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;    
    // calculate shadow
    float shadow = ShadowCalculation(fs_in.FragPosLightSpace);       
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;    
    
    FragColor = vec4(lighting, 1.0);*/
}