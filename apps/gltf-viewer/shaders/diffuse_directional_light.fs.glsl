#version 330

in vec3 vViewSpacePosition;
in vec3 vViewSpaceNormal;
in vec2 vTexCoords;

out vec3 fColor;

uniform vec3 uLightDirection;
uniform vec3 uLightIntensity;

const float PI = 3.14;

void main()
{
   // Need another normalization because interpolation of vertex attributes does not maintain unit length
   vec3 viewSpaceNormal = normalize(vViewSpaceNormal);
   
   fColor = abs(vec3(1 / PI) * uLightIntensity * dot(viewSpaceNormal, uLightDirection));
}