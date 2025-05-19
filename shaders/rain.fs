#version 100


precision mediump float;

// Input vertex attributes (from vertex shader)
varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// NOTE: Add your custom variables here

#define     MAX_LIGHTS              4
#define     LIGHT_DIRECTIONAL       0
#define     LIGHT_POINT             1

struct Light {
    int enabled;
    int type;
    vec3 position;
    vec3 target;
    vec4 color;
};

// Input lighting values
uniform Light lights[MAX_LIGHTS];
uniform vec4 ambient;
uniform vec3 viewPos;



const float SLICE_WIDTH = 1.0 / 9.0;


void main()
{

    vec2 newTexCoord = vec2(fragTexCoord.x / 9.0, fragTexCoord.y);

    // Texel color fetching from texture sampler
    // vec4 texelColor = texture2D(texture0, newTexCoord);

    // start off with ambient light
    vec4 texelColor = texture2D(texture0, newTexCoord) * ambient / 5.0;
    vec3 lightDot = vec3(0.0);
    vec3 normal = normalize(fragNormal);
    vec3 viewD = normalize(viewPos - fragPosition);
    vec3 specular = vec3(0.0);

    vec4 tint = colDiffuse * fragColor;


    for (int i = 0; i < MAX_LIGHTS; i++)
    {
         if (lights[i].enabled == 1)
         {
                vec3 light = vec3(0.0);
                float distance = 0.0;
 
                if (lights[i].type == LIGHT_DIRECTIONAL)
                {
                    light = (lights[i].target - lights[i].position);
                    distance = length(light);
                    light = -normalize(light);
                }
    
                if (lights[i].type == LIGHT_POINT)
                {
                    light = lights[i].position - fragPosition;
                    distance = length(light);
                    light = normalize(light);
                }

            
                float intensity = 1.0  / (distance * distance);
                intensity *= 20.0;

                // angle between the camera and light on the xz plane
                float camToLight = acos(dot(vec2(light.xz), vec2(viewD.xz)));


                int section = int(camToLight / 2.0 /3.1415962 * 9.0);

                // offset into texture
                vec2 sectionTexCoord = newTexCoord;
                 sectionTexCoord.x += SLICE_WIDTH * float(section);


                vec4 color = texture2D(texture0, sectionTexCoord);
                color *= intensity;
                color *= lights[i].color;
                texelColor += color;
                

//              float NdotL = max(dot(normal, light), 0.0);
//              lightDot += lights[i].color.rgb*NdotL;
 
//              float specCo = 0.0;
//              if (NdotL > 0.0) specCo = pow(max(0.0, dot(viewD, reflect(-(light), normal))), 16.0); // 16 refers to shine
//              specular += specCo;
        }
    }

    vec4 finalColor = texelColor;
//     vec4 finalColor = (texelColor*((tint + vec4(specular, 1.0))*vec4(lightDot, 1.0)));
//     finalColor += texelColor*(ambient/5.0);

    // Gamma correction
    gl_FragColor = pow(finalColor, vec4(1.0/2.2));
}
