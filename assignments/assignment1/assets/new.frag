#version 450
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform int useBlur;
uniform int bluriness;
uniform int useGamma;
uniform float gamma;
uniform bool useSharp;
uniform bool useInverse;

mat3 sharpen = mat3(0, -1, 0, -1, 5, -1, 0, -1, 0);

void main()
{
    vec3 finalColor = texture(screenTexture, TexCoords).rgb;

    if(useBlur == 1)
    {
        vec2 texelSize = 1.0 / textureSize(screenTexture, 0).xy;
        vec3 totalColor = vec3(0);

        for(int y = -(bluriness / 2); y <= bluriness / 2; y++)
        {
            for(int x = -(bluriness / 2); x <= bluriness / 2; x++)
            {
                vec2 offset = vec2(x,y) * texelSize;
                totalColor += texture(screenTexture, TexCoords + offset).rgb;
            }
        }

        totalColor /= (bluriness * bluriness);
        finalColor = totalColor;
    }    
    if(useGamma == 1)
    {
        finalColor = pow(finalColor, vec3(1.0 / gamma));
    }

    if(useSharp){
    vec2 texelSize = 1.0 / textureSize(screenTexture, 0).xy;
        vec3 totalColor = vec3(0);

        for(int y = 0; y < 3; y++)
        {
            for(int x = 0; x < 3; x++)
            {
                vec2 offset = vec2(x,y) * texelSize;
                totalColor += (texture(screenTexture, TexCoords + offset) * sharpen[x][y]).rgb;
            }
        }


        finalColor = totalColor;
    }
    if(useInverse)
    {
        finalColor = vec3(1.0 - texture(screenTexture, TexCoords));
    }


    FragColor = vec4(finalColor, 1.0);
}