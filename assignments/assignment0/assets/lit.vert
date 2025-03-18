#version 450
//Vertex attributes
layout(location = 0) in vec3 vPos; //Vertex position in model space
layout(location = 1) in vec3 vNormal; //Vertex position in model space
layout(location = 2) in vec2 vTexCoord;

uniform mat4 _Model; //Model->World Matrix
uniform mat4 _ViewProjection; //Combined View->Projection Matrix
uniform mat4 lightSpaceMatrix;

out Surface{
	vec3 WorldPos;
	vec3 WorldNormal;
	vec2 TexCoord;
	 vec4 FragPosLightSpace;
}vs_out;



void main(){
	vs_out.WorldPos = vec3(_Model * vec4(vPos,1.0));
	//Transform vertex normal to world space using Normal Matrix
	vs_out.WorldNormal = transpose(inverse(mat3(_Model))) * vNormal;
vs_out.TexCoord = vTexCoord;
vs_out.FragPosLightSpace = lightSpaceMatrix * vec4(vs_out.WorldPos, 1.0);
gl_Position = _ViewProjection * vec4(vs_out.WorldPos,1.0);


}
