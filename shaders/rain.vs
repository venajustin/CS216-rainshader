#version 100



// Input vertex attributes
attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
attribute vec3 vertexNormal;
attribute vec4 vertexColor;

attribute mat4 instanceTransform;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matNormal;

// Output vertex attributes (to fragment shader)
varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;
varying vec3 particalPos;


// Rain offset, used to animate rain position
uniform float rainoffset; // (0, 1)
uniform float travelheight; // must be 1/2 height of rain bounds
uniform vec3 campos;


void main()
{
    particalPos = (instanceTransform * vec4(1.0)).xyz;
    vec4 position = vec4(vertexPosition, 1.0);

    // interp accross (-travelheight, travelheight) without division
    float offset = ( travelheight * rainoffset * 2.0 ) - travelheight;

    // plane faces straight up, we must point it towards camera
    vec3 instancePos = (
            instanceTransform * 
            (vec4(1.0) + vec4(0.0, offset, 0.0, 1.0))
        ).xyz ;
    
    // vec from plane center to camera
    vec3 d = campos - instancePos;
    // vec3 d = campos - vec3(0.0);

    // billboarding to face the camera
    vec3 up = vec3(0.0, 1.0, 0.0); // fixed up direction

    vec3 newz = normalize(d); // towards eye
    vec3 newy = normalize(cross(d, up)); // towards right
    vec3 newx = cross(newy, newz); // towards top of screen
    // newx is already normalized
    
    mat4 billboardMat = mat4(
        vec4(newy, 0.0),
        vec4(newx, 0.0),
        vec4(newz, 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    );

    // apply squish
    mat4 scalemat = mat4(
        0.05, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
    position = scalemat * position;


    // apply bilboard transformation
    position = billboardMat * position;
    

    // offset the rain position on y axis 
    position.y = position.y + offset;



    // Compute MVP for current instance
    mat4 mvpi = mvp*instanceTransform;

    // Send vertex attributes to fragment shader
    fragPosition = vec3(mvpi*position);
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragNormal = newz;
    // fragNormal = normalize(vec3(matNormal*vec4(vertexNormal, 1.0)));

    // Calculate final vertex position
    gl_Position = mvpi*position;
}

