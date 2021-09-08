#version 330 core

uniform sampler2D tex;
uniform sampler2D norTex;
uniform sampler2D specTex;

uniform int flip;
uniform int shading;
uniform int useNorTex;

uniform vec3 ads; // ratio of (a)mbient to (d)iffuse to (s)pecular
uniform float shine;

in vec2 vTexCoord;

out vec4 Outcolor;

//interpolated normal and light vector in camera space
in vec3 fragNor;
in vec3 lightDir;
//position of the vertex in camera space
in vec3 EPos;

void main() {
  vec4 texColor = texture(tex, vTexCoord);

  if (texColor.a < 0.1)
    discard;

  if (shading == 1)
  {
    vec3 normal = normalize(fragNor);
    vec3 light = normalize(lightDir);
    vec3 v = normalize(EPos);
    vec3 h = normalize(light+v);

    if (flip == 1)
      normal *= -1.0f;

    float total = dot(ads, vec3(1,1,1));

    vec4 ambColor = (total == 0) ? 0.4f*texColor : ads.x/total*texColor;
    vec4 difColor = (total == 0) ? texColor : ads.y/total*texColor;
    vec4 specColor = (total == 0) ? texture(specTex, vTexCoord) : ads.z/total*texColor;

    vec3 emission = vec3(0,0,0);
    vec3 ambient = ambColor.rgb;
    vec3 diffuse = difColor.rgb*max(dot(normal,light),0);
    vec3 specular = specColor.rgb*pow(max(dot(normal,h),0), shine);

    Outcolor = vec4(emission + ambient + diffuse + specular, difColor.a);
  }
  else
  {
    Outcolor = texColor;
    //Outcolor = vec4(vTexCoord.x, vTexCoord.y, 0, 0);
  }
}