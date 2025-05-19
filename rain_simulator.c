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
*   Model: "Old Rusty Car" (https://skfb.ly/LxRy) by Renafox, 
*   licensed under Creative Commons Attribution-NonCommercial 
*   (http://creativecommons.org/licenses/by-nc/4.0/)
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
#include "texturemanager.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "style_dark.h"


#define MAX_LIGHTS  4           // Max dynamic lights supported by shader

#define MAX_PARTICLES 500
#define PARTICLE_SPAWN_RATE .01

// #define PARTICLE_SPAWN_HEIGHT 40
#define PARTICLE_SPAWN_HEIGHT 4
#define PARTICLE_SPAWN_WIDTH 5

#define GRAVITY -.098

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
    camera.position = (Vector3){2.0f, 2.0f, 6.0f}; // Camera position
    camera.target = (Vector3){0.0f, 0.5f, 0.0f}; // Camera looking at point
    camera.up = (Vector3){0.0f, 1.0f, 0.0f}; // Camera up vector (rotation towards target)
    camera.fovy = 45.0f; // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE; // Camera projection type

    // Load PBR shader and setup all required locations
    Shader shader = LoadShader(TextFormat("shaders/pbr.vs", GLSL_VERSION),
                               TextFormat("shaders/pbr.fs", GLSL_VERSION));
    shader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(shader, "albedoMap");
    // WARNING: Metalness, roughness, and ambient occlusion are all packed into a MRA texture
    // They are passed as to the SHADER_LOC_MAP_METALNESS location for convenience,
    // shader already takes care of it accordingly
    shader.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(shader, "mraMap");
    shader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(shader, "normalMap");
    // WARNING: Similar to the MRA map, the emissive map packs different information
    // into a single texture: it stores height and emission data
    // It is binded to SHADER_LOC_MAP_EMISSION location an properly processed on shader
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


    // TODO: load custom shader and set values
    // TODO: load / create custom model if needed for raindrop


    // Load old car model using PBR maps and shader
    // WARNING: We know this model consists of a single model.meshes[0] and
    // that model.materials[0] is by default assigned to that mesh
    // There could be more complex models consisting of multiple meshes and
    // multiple materials defined for those meshes... but always 1 mesh = 1 material
    Model car = LoadModel("resources/models/old_car_new.glb");

    // Assign already setup PBR shader to model.materials[0], used by models.meshes[0]
    car.materials[0].shader = shader;

    // Setup materials[0].maps default parameters
    car.materials[0].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
    car.materials[0].maps[MATERIAL_MAP_METALNESS].value = 0.0f;
    car.materials[0].maps[MATERIAL_MAP_ROUGHNESS].value = 0.0f;
    car.materials[0].maps[MATERIAL_MAP_OCCLUSION].value = 1.0f;
    car.materials[0].maps[MATERIAL_MAP_EMISSION].color = (Color){255, 162, 0, 255};

    // Setup materials[0].maps default textures
    car.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = LoadTexture("resources/old_car_d.png");
    car.materials[0].maps[MATERIAL_MAP_METALNESS].texture = LoadTexture("resources/old_car_mra.png");
    car.materials[0].maps[MATERIAL_MAP_NORMAL].texture = LoadTexture("resources/old_car_n.png");
    car.materials[0].maps[MATERIAL_MAP_EMISSION].texture = LoadTexture("resources/old_car_e.png");

    // Load floor model mesh and assign material parameters
    // NOTE: A basic plane shape can be generated instead of being loaded from a model file
    Model floor = LoadModel("resources/models/plane.glb");
    //Mesh floorMesh = GenMeshPlane(10, 10, 10, 10);
    //GenMeshTangents(&floorMesh);      // TODO: Review tangents generation
    //Model floor = LoadModelFromMesh(floorMesh);

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

    // Create some lights
    Light lights[MAX_LIGHTS] = {0};
    lights[0] = CreateLight(LIGHT_POINT, (Vector3){-1.0f, 1.0f, -2.0f}, (Vector3){0.0f, 0.0f, 0.0f}, YELLOW, 4.0f,
                            shader);
    lights[1] = CreateLight(LIGHT_POINT, (Vector3){2.0f, 1.0f, 1.0f}, (Vector3){0.0f, 0.0f, 0.0f}, GREEN, 3.3f, shader);
    lights[2] = CreateLight(LIGHT_POINT, (Vector3){-2.0f, 1.0f, 1.0f}, (Vector3){0.0f, 0.0f, 0.0f}, RED, 8.3f, shader);
    lights[3] = CreateLight(LIGHT_POINT, (Vector3){1.0f, 1.0f, -2.0f}, (Vector3){0.0f, 0.0f, 0.0f}, BLUE, 2.0f, shader);

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


    Texture2D gradienttexture = LoadTexture("resources/gradienttest.png");

    printf("gradienttexture width: %d\ngradienttexture height: %d\n", gradienttexture.width, gradienttexture.height);

    ///////////////////////////////////////////////////////////////////////////
    //                   LOAD RAIN TEXTURES
    ///////////////////////////////////////////////////////////////////////////

    SetTraceLogLevel(LOG_ERROR);
    // test
    TexTreeNode root = LoadRainTextures("resources/point_light_database/"); 
    
    UnloadRainTextures(root);

    SetTraceLogLevel(LOG_ALL);




    SetTargetFPS(60); // Set our game to run at 60 frames-per-second
    //---------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        // Update

        double last_time = curr_time;
        curr_time = GetTime();
        double dT = curr_time - last_time;


        //----------------------------------------------------------------------------------
        if (toggle_orbit) 
            UpdateCamera(&camera, CAMERA_ORBITAL);
        else 
            UpdateCamera(&camera, CAMERA_PERSPECTIVE);

        // Update the shader with the camera view vector (points towards { 0.0f, 0.0f, 0.0f })
        float cameraPos[3] = {camera.position.x, camera.position.y, camera.position.z};
        SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);


        // Check key inputs to enable/disable lights
        if (IsKeyPressed(KEY_ONE)) { lights[2].enabled = !lights[2].enabled; }
        if (IsKeyPressed(KEY_TWO)) { lights[1].enabled = !lights[1].enabled; }
        if (IsKeyPressed(KEY_THREE)) { lights[3].enabled = !lights[3].enabled; }
        if (IsKeyPressed(KEY_FOUR)) { lights[0].enabled = !lights[0].enabled; }


        // toggle logging with l
        if (IsKeyPressed(KEY_L)) {
            logging = !logging;
        }


        // Update light values on shader (actually, only enable/disable them)
        for (int i = 0; i < MAX_LIGHTS; i++) UpdateLight(shader, lights[i]);


        //---------------------------------------------------------------------
        // Spawn raindrops
        //---------------------------------------------------------------------

        if (raindrop_timer > PARTICLE_SPAWN_RATE && toggle_rain) {
            raindrop_timer = 0;

            int next_particle_loc = particle_count;
            if (particle_count >= MAX_PARTICLES) {
                particle_count = MAX_PARTICLES;
                // find oldest particle and replace
                int oldest = 0;
                for (int i = 0; i < particle_count; i++) {
                    if (particle_age[i] > oldest) {
                        oldest = particle_age[i];
                        next_particle_loc = i;
                    }
                }
            }
            else {
                particle_count++;
            }
            // assign position to particle
            particle_arr[next_particle_loc].p
                = randomPos(
                        (Vector3){-PARTICLE_SPAWN_WIDTH,  PARTICLE_SPAWN_HEIGHT, -PARTICLE_SPAWN_WIDTH}, 
                        (Vector3){ PARTICLE_SPAWN_WIDTH,  PARTICLE_SPAWN_HEIGHT,  PARTICLE_SPAWN_WIDTH}
                    );
        
            particle_arr[next_particle_loc].v
                = (Vector3){0.0, 0.0, 0.0};

            particle_age[next_particle_loc] = 0;

            if (logging)
                printf("Particle spawned at %f %f %f using slot %d\n",
                       particle_arr[next_particle_loc].p.x,
                       particle_arr[next_particle_loc].p.y,
                       particle_arr[next_particle_loc].p.z,
                       next_particle_loc
                );
        }
        raindrop_timer += dT;
        for (int i = 0; i < particle_count; i++) {
            particle_age[i]++;
        }

        //---------------------------------------------------------------------
        // Animate Raindrops
        //---------------------------------------------------------------------

        for (int i = 0; i < particle_count; i++) {
            // interpolation
            Particle* curr = &particle_arr[i];
            Vector3 vnew = curr->v;

            vnew.y = vnew.y + GRAVITY * dT;

            Vector3 vmid = curr->v;
            vmid.x = vmid.x + vnew.x / 2;
            vmid.y = vmid.y + vnew.y / 2;
            vmid.z = vmid.z + vnew.z / 2;

            curr->v = vnew;

            curr->p.x += vmid.x;
            curr->p.y += vmid.y;
            curr->p.z += vmid.z;

        }


        //        for (int i = 0; i < 10; i++ ) {
        //            printf("pos: %f %f %f\n", particle_arr[i].x, particle_arr[i].y, particle_arr[i].z);
        //        }

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

        DrawModel(floor, (Vector3){0.0f, 0.0f, 0.0f}, 5.0f, WHITE); // Draw floor model

        // Set old car model texture tiling, emissive color and emissive intensity parameters on shader
        SetShaderValue(shader, textureTilingLoc, &carTextureTiling, SHADER_UNIFORM_VEC2);
        Vector4 carEmissiveColor = ColorNormalize(car.materials[0].maps[MATERIAL_MAP_EMISSION].color);
        SetShaderValue(shader, emissiveColorLoc, &carEmissiveColor, SHADER_UNIFORM_VEC4);
        float emissiveIntensity = .1f;
        SetShaderValue(shader, emissiveIntensityLoc, &emissiveIntensity, SHADER_UNIFORM_FLOAT);

        DrawModel(car, (Vector3){0.0f, 0.0f, 0.0f}, 0.25f, WHITE); // Draw car model

        // Draw spheres to show the lights positions
        for (int i = 0; i < MAX_LIGHTS; i++) {
            Color lightColor = (Color){
                lights[i].color[0] * 255, lights[i].color[1] * 255, lights[i].color[2] * 255, lights[i].color[3] * 255
            };

            if (lights[i].enabled) DrawSphereEx(lights[i].position, 0.2f, 8, 8, lightColor);
            else DrawSphereWires(lights[i].position, 0.2f, 8, 8, ColorAlpha(lightColor, 0.3f));
        }


        Color particle_color = (Color){255, 50, 50, 255};
        for (int i = 0; i < particle_count; i++) {


          DrawBillboardRec(camera, gradienttexture, (Rectangle){ 0, 0, 16.0, 80.0 }, particle_arr[i].p, (Vector2){ 0.05, 0.75 }, WHITE); 
            
            
            // DrawSphere(particle_arr[i].p, 0.1f, 10, 10, particle_color);
        }




        EndMode3D();

        // GuiLabel((Rectangle){ 10 pw, 40 ph, 90 pw, 24 ph }, "Toggle Rain:");
        // GuiToggle((Rectangle){90 pw, 40 ph, 60 pw, 24 ph }, ((toggle_rain) ? "enabled" : "disabled"), &toggle_rain);

        GuiLabel((Rectangle){1 pw, 20 ph, 5 pw, 3 ph}, "Toggle Rain:");
        GuiToggle((Rectangle){6 pw, 20 ph, 5 pw, 3 ph}, ((toggle_rain) ? "enabled" : "disabled"), &toggle_rain);

        GuiLabel((Rectangle){1 pw, 25 ph, 5 pw, 3 ph}, "Camera:");
        GuiToggle((Rectangle){6 pw, 25 ph, 5 pw, 3 ph}, ((toggle_orbit) ? "orbit" : "fixed"), &toggle_orbit);


        // DrawText("Toggle lights: [1][2][3][4]", 10, 40, 20, LIGHTGRAY);

        // DrawText("(c) Old Rusty Car model by Renafox (https://skfb.ly/LxRy)", screenWidth - 320, screenHeight - 20, 10, LIGHTGRAY);

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

    floor.materials[0].shader = (Shader){0};
    UnloadMaterial(floor.materials[0]);
    floor.materials[0].maps = NULL;
    UnloadModel(floor);

    UnloadShader(shader); // Unload Shader
                          //
    UnloadTexture(gradienttexture);

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
