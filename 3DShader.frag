#version 300 es
precision highp float;
uniform sampler2D Texture;

uniform vec3 viewPos;
uniform vec3 lightPos; 
uniform vec3 lightColor; 

in vec3 ex_FragPos;
in vec3 ex_Normal;
in vec3 ex_Color;
in vec2 ex_TexCoord;

out vec4 fragColor;

void main(void){
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
  	
    vec3 norm = normalize(ex_Normal);
    vec3 lightDir = normalize(lightPos - ex_FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - ex_FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * lightColor;  
        
    fragColor = vec4((ambient + diffuse + specular) * texture(Texture, ex_TexCoord).rgb, 1.0);
}
