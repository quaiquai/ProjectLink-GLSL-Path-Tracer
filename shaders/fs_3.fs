#version 330 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 FragPos;
in vec2 TexCoords;
  
uniform vec3 lightPos; 
uniform vec3 viewPos;

struct Material {
	sampler2D diffuse;
	//vec3 specular; //whole object is specular
	sampler2D specular;
	float shininess;
};

uniform Material material;

struct Light {
    vec3 position; //needed for point lighting or spotlight
	vec3 direction; //needed for direction lighting or spotlight
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

	//for attenuation
	float constant;
	float linear;
	float quadratic;

	float cutoff;//for the spotlight
	float outerCutoff;//for spotlight
};

uniform Light light;

void main()
{
    vec3 lightDir = normalize(light.position - FragPos);// point lighting or spotlighting

	float theta = dot(lightDir, normalize(-light.direction)); //for spotlighting
	float epsilon = light.cutoff - light.outerCutoff;
	float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);
	vec3 result;


	// ambient
	vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;
  	
	// diffuse 
	vec3 norm = normalize(Normal);
	float distance = length(light.position - FragPos); //for attenuation
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance)); //attenuation calculation
	//vec3 lightDir = normalize(-light.direction);//needed for direction lighting
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;
    
	// specular
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);  
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	vec3 specular = light.specular * spec * texture(material.specular, TexCoords).rgb;  
        
	//result = ambient * attenuation + diffuse * attenuation + specular * attenuation;
	//remove attenuation for ambient when doing spotlights
	result = ambient + diffuse * attenuation * intensity + specular * attenuation * intensity;// spotlight attenuation

	FragColor = vec4(result, 1.0);
	
}