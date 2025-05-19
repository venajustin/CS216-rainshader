/*******************************************************************************************
 *
 *   raylib [shaders] example - Basic PBR
 *
 *   Example complexity rating: [★★★★] 4/4
 *
 *   Example originally created with raylib 5.0, last time updated with raylib 5.1-dev
 *
 *   Example contributed by Afan OLOVCIC (@_DevDad) and reviewed by Ramon Santamaria (@raysan5)
 *
 *   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
 *   BSD-like license that allows static linking with closed source software
 *
 *   Copyright (c) 2023-2025 Afan OLOVCIC (@_DevDad)
 *
 *   Model: "Toyota Landcruiser" (https://skfb.ly/MyHv) by Renafox,
 *   licensed under Creative Commons Attribution-NonCommercial 
 *   (http://creativecommons.org/licenses/by-nc/4.0/)
 *
 *   Model: "City Building Set 1" (https://skfb.ly/LpSC) by Neberkenezer
 *   licensed under Creative Commons Attribution
 *
 ********************************************************************************************/

#include "raylib.h"
#include <raymath.h>

#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
#define GLSL_VERSION            100
#endif

#include <stdlib.h>             // Required for: NULL
#include <stdio.h>
#include "particles.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "style_dark.h"


#define MAX_LIGHTS  4           // Max dynamic lights supported by shader

#define MAX_PARTICLES 30000
#define PARTICLE_SPAWN_RATE .01

#define RAIN_BOUND_X 30
#define RAIN_BOUND_Y 500
#define RAIN_BOUND_Z 30

#define RAIN_STEP 0.05 // multipler to dT applied to rain animation


//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

// Light type
typedef enum {
    LIGHT_DIRECTIONAL = 0,
    LIGHT_POINT,
    LIGHT_SPOT
} LightType;

// Light data
typedef struct {
    int type;
    int enabled;
    Vector3 position;
    Vector3 target;
    float color[4];
    float intensity;

    // Shader light parameters locations
    int typeLoc;
    int enabledLoc;
    int positionLoc;
    int targetLoc;
    int colorLoc;
    int intensityLoc;
} Light;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static int lightCount = 0; // Current number of dynamic lights that have been created
static bool logging = false;
bool toggle_rain = true;
bool toggle_orbit = true;
bool toggle_pause = false;

int screenWidth = 1920;
int screenHeight = 1080;

// macros to make object resizing to screen factors easy
#define pw * (int)(screenWidth / 100) // percentage screen width
#define ph * (int)(screenHeight / 100) // percentage screen height


//----------------------------------------------------------------------------------
// Module specific Functions Declaration
//----------------------------------------------------------------------------------
// Create a light and get shader locations
static Light CreateLight(int type, Vector3 position, Vector3 target, Color color, float intensity, Shader shader);

// Update light properties on shader
// NOTE: Light shader locations should be available
static void UpdateLight(Shader shader, Light light);

void InvalidArgsExit() {
    printf("Invalid arguments\n");
    exit(1);
}

//----------------------------------------------------------------------------------
// Main Entry Point
//----------------------------------------------------------------------------------
int main(int argc, char** argv) {

    // Process Arguments
    for (int i = 0; i < argc; i++) {
        if (strncmp(argv[i], "-w", 3) == 0) {
            if (i + 1 >= argc) InvalidArgsExit();
            screenWidth = strtol(argv[i + 1], NULL, 10);
        }
        if (strncmp(argv[i], "-h", 3) == 0) {
            if (i + 1 >= argc) InvalidArgsExit();
            screenHeight = strtol(argv[i + 1], NULL, 10);
        }
    }



    // Initialization
    //--------------------------------------------------------------------------------------


    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "raylib [shaders] example - basic pbr");
    GuiLoadStyleDark();

    //set rand seed
    srand((unsigned int)GetTime());

    // Define the camera to look into our 3d world
    Camera camera = {0};
    camera.position = (Vector3){15.0f, 10.0f, 10.0f}; // Camera position
    camera.target = (Vector3){0.0f, 0.5f, 0.0f}; // Camera looking at point
    camera.up = (Vector3){0.0f, 1.0f, 0.0f}; // Camera up vector (rotation towards target)
    camera.fovy = 45.0f; // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE; // Camera projection type

    // Load PBR shader and setup all required locations
    Shader shader = LoadShader(TextFormat("shaders/pbr.vs", GLSL_VERSION),
            TextFormat("shaders/pbr.fs", GLSL_VERSION));
    shader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(shader, "albedoMap");
    shader.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(shader, "mraMap");
    shader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(shader, "normalMap");
    shader.locs[SHADER_LOC_MAP_EMISSION] = GetShaderLocation(shader, "emissiveMap");
    shader.locs[SHADER_LOC_COLOR_DIFFUSE] = GetShaderLocation(shader, "albedoColor");

    // Setup additional required shader locations, including lights data
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    int lightCountLoc = GetShaderLocation(shader, "numOfLights");
    int maxLightCount = MAX_LIGHTS;
    SetShaderValue(shader, lightCountLoc, &maxLightCount, SHADER_UNIFORM_INT);

    // Setup ambient color and intensity parameters
    float ambientIntensity = 0.02f;
    Color ambientColor = (Color){26, 32, 135, 255};
    Vector3 ambientColorNormalized = (Vector3){
        ambientColor.r / 255.0f, ambientColor.g / 255.0f, ambientColor.b / 255.0f
    };
    SetShaderValue(shader, GetShaderLocation(shader, "ambientColor"), &ambientColorNormalized, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, GetShaderLocation(shader, "ambient"), &ambientIntensity, SHADER_UNIFORM_FLOAT);

    // Get location for shader parameters that can be modified in real time
    int emissiveIntensityLoc = GetShaderLocation(shader, "emissivePower");
    int emissiveColorLoc = GetShaderLocation(shader, "emissiveColor");
    int textureTilingLoc = GetShaderLocation(shader, "tiling");



      Model car = LoadModel("resources/toyota_land_cruiser/scene.gltf");
//       for (int i = 0; i < car.materialCount; i++) {
//           car.materials[i].shader = shader;
// 
// 
//       }


//      Model car = LoadModel("resources/models/toyota_land_cruiser.glb");
//  
//      // Assign already setup PBR shader to model.materials[0], used by models.meshes[0]
//      car.materials[0].shader = shader;
//  
//      // Setup materials[0].maps default parameters
//      car.materials[0].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
//      car.materials[0].maps[MATERIAL_MAP_METALNESS].value = 1.0f;
//      car.materials[0].maps[MATERIAL_MAP_ROUGHNESS].value = 1.0f;
//      car.materials[0].maps[MATERIAL_MAP_OCCLUSION].value = 1.0f;
//      car.materials[0].maps[MATERIAL_MAP_EMISSION].color = (Color){255, 162, 0, 255};
//  
//      // Setup materials[0].maps default textures
//      car.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = LoadTexture("resources/landcruiser_d.png");
//      car.materials[0].maps[MATERIAL_MAP_METALNESS].texture = LoadTexture("resources/landcruiser_m.png");
//      car.materials[0].maps[MATERIAL_MAP_NORMAL].texture = LoadTexture("resources/landcruiser_n.png");
//      car.materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture = LoadTexture("resources/landcruiser_r.png");
//      car.materials[0].maps[MATERIAL_MAP_OCCLUSION].texture = LoadTexture("resources/landcruiser_ao.png");
//      car.materials[0].maps[MATERIAL_MAP_EMISSION].texture = LoadTexture("resources/landcruiser_e.png");
//  


    // Model stoplight = LoadModel("resources/bus_stop__traffic_light/scene.gltf");
    // stoplight.materials[0].shader = shader;

    Model city = LoadModel("resources/ccity_building_set_1/scene.gltf");
    city.materials[0].shader = shader;


    Model floor = LoadModel("resources/models/plane.glb");
    // Assign material shader for our floor model, same PBR shader
    floor.materials[0].shader = shader;

    floor.materials[0].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
    floor.materials[0].maps[MATERIAL_MAP_METALNESS].value = 0.0f;
    floor.materials[0].maps[MATERIAL_MAP_ROUGHNESS].value = 0.0f;
    floor.materials[0].maps[MATERIAL_MAP_OCCLUSION].value = 1.0f;
    floor.materials[0].maps[MATERIAL_MAP_EMISSION].color = BLACK;

    floor.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = LoadTexture("resources/road_a.png");
    floor.materials[0].maps[MATERIAL_MAP_METALNESS].texture = LoadTexture("resources/road_mra.png");
    floor.materials[0].maps[MATERIAL_MAP_NORMAL].texture = LoadTexture("resources/road_n.png");

    // Models texture tiling parameter can be stored in the Material struct if required (CURRENTLY NOT USED)
    // NOTE: Material.params[4] are available for generic parameters storage (float)
    Vector2 carTextureTiling = (Vector2){0.5f, 0.5f};
    Vector2 floorTextureTiling = (Vector2){0.5f, 0.5f};


    // Setup material texture maps usage in shader
    // NOTE: By default, the texture maps are always used
    int usage = 1;
    SetShaderValue(shader, GetShaderLocation(shader, "useTexAlbedo"), &usage, SHADER_UNIFORM_INT);
    SetShaderValue(shader, GetShaderLocation(shader, "useTexNormal"), &usage, SHADER_UNIFORM_INT);
    SetShaderValue(shader, GetShaderLocation(shader, "useTexMRA"), &usage, SHADER_UNIFORM_INT);
    SetShaderValue(shader, GetShaderLocation(shader, "useTexEmissive"), &usage, SHADER_UNIFORM_INT);


    Particle particle_arr[MAX_PARTICLES] = {0};
    int particle_count = 0;
    int particle_age[MAX_PARTICLES] = {0};
    double raindrop_timer = 0;


    double curr_time;


    // Define mesh to be instanced
    // Mesh rdropmesh = GenMeshCube(0.5, 0.5, 0.5);
     Mesh rdropmesh = GenMeshPlane(1.0f, 1.0f, 2, 2);

    // Define transforms to be uploaded to GPU for instances
    Matrix *transforms = (Matrix *)RL_CALLOC(MAX_PARTICLES, sizeof(Matrix));   // Pre-multiplied transformations passed to rlgl

    // Translate and rotate planes randomly
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        Matrix translation = MatrixTranslate(
                (float)GetRandomValue(- RAIN_BOUND_X / 2.0, RAIN_BOUND_X / 2.0),
                (float)GetRandomValue(- RAIN_BOUND_Y / 2.0, RAIN_BOUND_Y / 2.0),
                (float)GetRandomValue(- RAIN_BOUND_Z / 2.0, RAIN_BOUND_Z / 2.0));


        transforms[i] = translation;
        // transforms[i] = MatrixMultiply(rotation, translation);
    }

    // Load lighting shader
    Shader rainshader = LoadShader(TextFormat("shaders/rain.vs", GLSL_VERSION),
            TextFormat("shaders/rain.fs", GLSL_VERSION));

    // Get shader locations
    rainshader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(rainshader, "mvp");
    rainshader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(rainshader, "viewPos");

    int ambientLoc = GetShaderLocation(rainshader, "ambient");
    SetShaderValue(rainshader, ambientLoc, (float[4]){ 0.01f, 0.01f, 0.01f, 1.0f }, SHADER_UNIFORM_VEC4);


    Material matInstances = LoadMaterialDefault();
    matInstances.shader = rainshader;
    matInstances.maps[MATERIAL_MAP_DIFFUSE].color = RED;

    int rainOffsetLoc = GetShaderLocation(rainshader, "rainoffset");
    float rainoffset = 0.0;
    SetShaderValue(rainshader, rainOffsetLoc, &rainoffset, SHADER_UNIFORM_FLOAT);
    
    int travelHeightLoc = GetShaderLocation(rainshader, "travelheight");
    float travelHeight = RAIN_BOUND_Y / 2.0;
    SetShaderValue(rainshader, travelHeightLoc, &travelHeight, SHADER_UNIFORM_FLOAT);
    
    int camPositionLoc = GetShaderLocation(rainshader, "campos");

    Texture2D raintexture = LoadTexture("resources/cv20_v30_h-_osc3.png");
    matInstances.maps[MATERIAL_MAP_ALBEDO].texture = raintexture;

    printf("raintexture w: %d, h: %d\n", raintexture.width, raintexture.height);
     
    // Create some lights
    Light lights[MAX_LIGHTS] = {0};
    lights[0] = CreateLight(LIGHT_POINT, (Vector3){-1.0f, 1.0f, -2.0f}, (Vector3){0.0f, 0.0f, 0.0f}, YELLOW, 40.0f,
            rainshader);
    lights[1] = CreateLight(LIGHT_POINT, (Vector3){2.0f, 1.0f, 1.0f}, (Vector3){0.0f, 0.0f, 0.0f}, GREEN, 30.3f, rainshader);
    lights[2] = CreateLight(LIGHT_POINT, (Vector3){-2.0f, 1.0f, 1.0f}, (Vector3){0.0f, 0.0f, 0.0f}, RED, 150.3f, rainshader);
    lights[3] = CreateLight(LIGHT_POINT, (Vector3){1.0f, 1.0f, -2.0f}, (Vector3){0.0f, 0.0f, 0.0f}, BLUE, 20.0f, rainshader);

    SetTargetFPS(60); // Set our game to run at 60 frames-per-second
                      //---------------------------------------------------------------------------------------

                      // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        // Update

        double last_time = curr_time;
        curr_time = GetTime();
        double dT;
        if (toggle_pause) 
            dT = 0;
        else 
            dT = curr_time - last_time;


        //----------------------------------------------------------------------------------
        if (toggle_orbit)
            UpdateCamera(&camera, CAMERA_ORBITAL);
        else 
            UpdateCamera(&camera, CAMERA_PERSPECTIVE);

        // Update the shader with the camera view vector (points towards { 0.0f, 0.0f, 0.0f })
        float cameraPos[3] = {camera.position.x, camera.position.y, camera.position.z};
        SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);
        SetShaderValue(rainshader, rainshader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);


        // this is basically a repeat of above but I don't know if it will mess with
        // the frag shader to have the same name in both
        SetShaderValue(rainshader, camPositionLoc, cameraPos, SHADER_UNIFORM_VEC3);



        // Check key inputs to enable/disable lights
        if (IsKeyPressed(KEY_ONE)) { lights[2].enabled = !lights[2].enabled; }
        if (IsKeyPressed(KEY_TWO)) { lights[1].enabled = !lights[1].enabled; }
        if (IsKeyPressed(KEY_THREE)) { lights[3].enabled = !lights[3].enabled; }
        if (IsKeyPressed(KEY_FOUR)) { lights[0].enabled = !lights[0].enabled; }

        for (int i = 0; i < MAX_LIGHTS; i++) {
            UpdateLight(rainshader, lights[i]);
        }


        // toggle logging with l
        if (IsKeyPressed(KEY_L)) {
            logging = !logging;
        }


        // Update light values on shader (actually, only enable/disable them)
        for (int i = 0; i < MAX_LIGHTS; i++) UpdateLight(shader, lights[i]);



        //---------------------------------------------------------------------
        // Animate Raindrops
        //---------------------------------------------------------------------
       
        rainoffset = rainoffset - (dT * RAIN_STEP);
        if (rainoffset < 0.6) { // Hacky workaround, should be < 0, but weird translations are happening < .6
            rainoffset = 1;
        }
        if (logging) {
            printf("rainoffset: %f\n", rainoffset);
        }
        SetShaderValue(rainshader, rainOffsetLoc, &rainoffset, SHADER_UNIFORM_FLOAT);


        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(BLACK);

        BeginMode3D(camera);

        // Set floor model texture tiling and emissive color parameters on shader
        SetShaderValue(shader, textureTilingLoc, &floorTextureTiling, SHADER_UNIFORM_VEC2);
        Vector4 floorEmissiveColor = ColorNormalize(floor.materials[0].maps[MATERIAL_MAP_EMISSION].color);
        SetShaderValue(shader, emissiveColorLoc, &floorEmissiveColor, SHADER_UNIFORM_VEC4);

       //  DrawModel(floor, (Vector3){0.0f, 0.0f, 0.0f}, 5.0f, WHITE); // Draw floor model

        SetShaderValue(shader, textureTilingLoc, &carTextureTiling, SHADER_UNIFORM_VEC2);
        Vector4 carEmissiveColor = ColorNormalize(car.materials[0].maps[MATERIAL_MAP_EMISSION].color);
        SetShaderValue(shader, emissiveColorLoc, &carEmissiveColor, SHADER_UNIFORM_VEC4);
        float emissiveIntensity = .01f;
        SetShaderValue(shader, emissiveIntensityLoc, &emissiveIntensity, SHADER_UNIFORM_FLOAT);

        DrawModel(car, (Vector3){0.0f, -0.1f, -10.0f}, 0.05, WHITE); // Draw car model
        // DrawModel(stoplight, (Vector3){-0.0f, 0.0f, -4.0f}, 10.0f, WHITE); // Draw bus stop

        DrawModel(city, (Vector3){75.0f, 0.0f, 75.0f}, .01, WHITE);

        // Draw spheres to show the lights positions
        for (int i = 0; i < MAX_LIGHTS; i++) {
            Color lightColor = (Color){
                lights[i].color[0] * 255, lights[i].color[1] * 255, lights[i].color[2] * 255, lights[i].color[3] * 255
            };

            if (lights[i].enabled) DrawSphereEx(lights[i].position, 0.2f, 8, 8, lightColor);
            else DrawSphereWires(lights[i].position, 0.2f, 8, 8, ColorAlpha(lightColor, 0.3f));
        }



        // Color particle_color = (Color){255, 50, 50, 255};
        // for (int i = 0; i < particle_count; i++) {
        //     DrawSphereEx(particle_arr[i].p, 0.1f, 2, 2, particle_color);
        // }

        BeginBlendMode(BLEND_ADDITIVE);
        DrawMeshInstanced(rdropmesh, matInstances, transforms, MAX_PARTICLES);
        EndBlendMode();

        if (logging) {
            DrawSphere(camera.position, .5, RED);
        }


        EndMode3D();

        // GuiLabel((Rectangle){ 10 pw, 40 ph, 90 pw, 24 ph }, "Toggle Rain:");
        // GuiToggle((Rectangle){90 pw, 40 ph, 60 pw, 24 ph }, ((toggle_rain) ? "enabled" : "disabled"), &toggle_rain);

//          GuiLabel((Rectangle){1 pw, 20 ph, 5 pw, 3 ph}, "Toggle Rain:");
//          GuiToggle((Rectangle){6 pw, 20 ph, 5 pw, 3 ph}, ((toggle_rain) ? "enabled" : "disabled"), &toggle_rain);

        GuiLabel((Rectangle){1 pw, 20 ph, 5 pw, 3 ph}, "Camera Orbit:");
        GuiToggle((Rectangle){6 pw, 20 ph, 5 pw, 3 ph}, ((toggle_orbit) ? "enabled" : "disabled"), &toggle_orbit);

        GuiLabel((Rectangle){1 pw, 25 ph, 5 pw, 3 ph}, "Pause Time:");
        GuiToggle((Rectangle){6 pw, 25 ph, 5 pw, 3 ph}, ((toggle_pause) ? "enabled" : "disabled"), &toggle_pause);

        // DrawText("Toggle lights: [1][2][3][4]", 10, 40, 20, LIGHTGRAY);

        DrawText("(c) Toyota Landcruiser model by Renafox (https://skfb.ly/MyHv)", screenWidth - 380, screenHeight - 20, 10, LIGHTGRAY);

        DrawFPS(10, 10);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    // Unbind (disconnect) shader from car.material[0]
    // to avoid UnloadMaterial() trying to unload it automatically
    car.materials[0].shader = (Shader){0};
    UnloadMaterial(car.materials[0]);
    car.materials[0].maps = NULL;
    UnloadModel(car);

    city.materials[0].shader = (Shader){0};
    UnloadMaterial(city.materials[0]);
    city.materials[0].maps = NULL;
    UnloadModel(city);

    floor.materials[0].shader = (Shader){0};
    UnloadMaterial(floor.materials[0]);
    floor.materials[0].maps = NULL;
    UnloadModel(floor);

    UnloadShader(shader); // Unload Shader

    CloseWindow(); // Close window and OpenGL context
                   //--------------------------------------------------------------------------------------

    return 0;
}

// Create light with provided data
// NOTE: It updated the global lightCount and it's limited to MAX_LIGHTS
static Light CreateLight(int type, Vector3 position, Vector3 target, Color color, float intensity, Shader shader) {
    Light light = {0};

    if (lightCount < MAX_LIGHTS) {
        light.enabled = 1;
        light.type = type;
        light.position = position;
        light.target = target;
        light.color[0] = (float)color.r / 255.0f;
        light.color[1] = (float)color.g / 255.0f;
        light.color[2] = (float)color.b / 255.0f;
        light.color[3] = (float)color.a / 255.0f;
        light.intensity = intensity;

        // NOTE: Shader parameters names for lights must match the requested ones
        light.enabledLoc = GetShaderLocation(shader, TextFormat("lights[%i].enabled", lightCount));
        light.typeLoc = GetShaderLocation(shader, TextFormat("lights[%i].type", lightCount));
        light.positionLoc = GetShaderLocation(shader, TextFormat("lights[%i].position", lightCount));
        light.targetLoc = GetShaderLocation(shader, TextFormat("lights[%i].target", lightCount));
        light.colorLoc = GetShaderLocation(shader, TextFormat("lights[%i].color", lightCount));
        light.intensityLoc = GetShaderLocation(shader, TextFormat("lights[%i].intensity", lightCount));

        UpdateLight(shader, light);

        lightCount++;
    }

    return light;
}

// Send light properties to shader
// NOTE: Light shader locations should be available
static void UpdateLight(Shader shader, Light light) {
    SetShaderValue(shader, light.enabledLoc, &light.enabled, SHADER_UNIFORM_INT);
    SetShaderValue(shader, light.typeLoc, &light.type, SHADER_UNIFORM_INT);

    // Send to shader light position values
    float position[3] = {light.position.x, light.position.y, light.position.z};
    SetShaderValue(shader, light.positionLoc, position, SHADER_UNIFORM_VEC3);

    // Send to shader light target position values
    float target[3] = {light.target.x, light.target.y, light.target.z};
    SetShaderValue(shader, light.targetLoc, target, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, light.colorLoc, light.color, SHADER_UNIFORM_VEC4);
    SetShaderValue(shader, light.intensityLoc, &light.intensity, SHADER_UNIFORM_FLOAT);
}
