#version  330 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;

uniform float time;

uniform mat4 P;
uniform mat4 M;
uniform mat4 V;
uniform vec3 lightPos;
uniform vec3 leafBlowerPos;
uniform vec3 windDir;

uniform int blowable;

out vec3 fragNor;
out vec3 lightDir;
out vec3 EPos;
out vec2 vTexCoord;

void main() {
  fragNor = (M*vec4(vertNor, 0.0)).xyz;

  // vertex in world space
  vec3 MvertPos = (M*vec4(vertPos,1)).xyz;

  if (blowable == 1)
  {
    float c = abs(dot(normalize(windDir), normalize(MvertPos - leafBlowerPos))); // wind orientation
    float d = abs(dot(normalize(MvertPos - leafBlowerPos), normalize(fragNor))); // leaf orientation
    float maxpower = 7;
    float magnitude = min(maxpower, maxpower/pow(length(MvertPos - leafBlowerPos)/10,2)) * pow(c,30) * d; // inverse square law * cone angle * normal
    vec3 direction = normalize(fragNor);
    MvertPos += magnitude*sin(30*(1-d)*time)*direction;
  }

  lightDir = lightPos - MvertPos;
  EPos = -1*(V*vec4(MvertPos, 1)).xyz;
  
  /* First model transforms */
  gl_Position = P * V * vec4(MvertPos, 1);

  /* pass through the texture coordinates to be interpolated */
  vTexCoord = vertTex;
}
