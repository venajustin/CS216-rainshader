#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#include "stdio.h"
#include "malloc.h"
#include "dirent.h"
#include <raylib.h>
#include <string.h>

typedef enum {
    BRANCH = 0,
    LEAF
} TexTreeNodeType;

typedef struct TexTreeNode {
    TexTreeNodeType type;
    int numChildren;
    struct TexTreeNode *children;
    char name[30];
    Texture2D texture; // not used unless leaf node
} TexTreeNode;

// angles in degrees
typedef struct TexDescriptor {
    // steps of 20
    int cv; // 0 -> 40
    int v; // -90 -> 90
    int h; // 10 -> 170

    // steps of 1
    int osc; // 0 -> 9
} TexDescriptor;

// angles in radians
typedef struct TexQuery {
    float cv;
    float v;
    float h;
    float osc;
} TexQuery;

// index of texture within array of dcam folder contents array
int TexIndex(TexDescriptor td) {
    int index = 0;

    index += -80 + (((td.v + 90) / 20) * 90);
    if (td.v == 90) {
        // last 10 only have h170
        index -= 80;
    }
    index += 10 * ((td.h - 10) / 20);
    index += td.osc;

    return index;
}

TexTreeNode LoadRainTextures(char *path) {
    TexTreeNode root;
    
    struct dirent *de;
    DIR *dr = opendir(path);
    if (dr == NULL) {
        printf("Could not open dir %s\n", path);
        root.numChildren = 0;
        root.type = LEAF;
        return root;
    }

    root.type = BRANCH;
    int count =  0;
    while ((de = readdir(dr)) != NULL) {
        count++;
    }
    closedir(dr);

#ifdef LOADING_SCREEN

    BeginDrawing();
    ClearBackground(WHITE);

    char randomdots[6] = { 0 };
    for (int i = 0; i < 5; i++) {
        randomdots[i] = (GetRandomValue(0, 1) == 0) ? '.' : ' ';
    }
    char message[30] = "Loading Rain Textures";
    printf("%s\n", message);

    strncat(message, randomdots, 6);

    // print loading screen
    DrawText(message, 500, 500, 30, BLACK);

    EndDrawing();


#endif


    root.numChildren = count;
    root.children = malloc(sizeof(TexTreeNode) * count);
    root.type = BRANCH; // set to leaf later if it is
    root.texture = (Texture2D){ 0 };

    for (int i = 0; i < count; i++) {
        root.children[i].numChildren = 0;
    }

    dr = opendir(path);
    int i = 0;
    int pngcount = 0;
    while ((de = readdir(dr)) != NULL && i < count) {
      
        // path of the next level file
        char buff[200] = { 0 };
        strncat(buff, path, 200);
        strncat(buff, "/", 2);
        strncat(buff, de->d_name, 200);


        char last[5] = { 0 };
        for (int i = 0; de->d_name[i] != '\0'; i++) {
            last[0] = de->d_name[i - 3];
            last[1] = de->d_name[i - 2];
            last[2] = de->d_name[i - 1];
            last[3] = de->d_name[i - 0];
        }
        if (strncmp(last, ".png", 5) == 0) {
            // printf("png found: %s\n", de->d_name);
            
            // png found
            pngcount++;

            TexDescriptor td = {0};
            // parsing out params of filename to find location to put it in

            if (strstr(buff, "env_") != NULL) {
                // Environment light
                const char* title = de->d_name;
                char *cvloc = title + 2;
                char *cvend = strstr(title, "_osc");
                char *oloc = cvend + 4;
                char *oend = strstr(title, ".png");

                // extract strs
                char cvstr[4] = {0};
                strncpy(cvstr, cvloc, cvend-cvloc);
                char ostr[4] = {0};
                strncpy(ostr, oloc, oend-oloc);

                td.cv = atoi(cvloc);
                td.v = -90;
                td.h = 170;
                td.osc = atoi(ostr);
            } else {
                // point light
                const char* title = de->d_name;
                char *cvloc = title + 2;
                char *cvend = strstr(title, "_v");
                char *vloc = cvend + 2;
                char *vend = strstr(title, "_h");
                char *hloc = vend + 2;
                char *hend = strstr(title, "_osc");
                char *oloc = hend + 4;
                char *oend = strstr(title, ".png");

                // extract strs
                char cvstr[4] = {0};
                strncpy(cvstr, cvloc, cvend-cvloc);
                char vstr[4] = {0};
                strncpy(vstr, vloc, vend-vloc);
                char hstr[4] = {0};
                strncpy(hstr, hloc, hend-hloc);
                char ostr[4] = {0};
                strncpy(ostr, oloc, oend-oloc);

                td.cv = atoi(cvloc);
                td.v = atoi(vstr);
                td.h = atoi(hstr);
                td.osc = atoi(ostr);
            }


            int index = TexIndex(td);
            root.children[index].texture = LoadTexture(buff);
            root.children[index].numChildren = 0;
            root.children[index].type = LEAF;
            strncpy(root.children[index].name, de->d_name, 30);

             
            
        } else {
            // test for dir
            DIR *dr2 = opendir(buff);
            if (dr2 != NULL) {

                // if any '.' we will just consider it not a dir
                bool dir = true;
                for (int j = 0; de->d_name[j] != '\0' && j < 200;j++) {
                    if (de->d_name[j] == '.') {
                        dir = false;
                        break;
                    }
                }
                if (dir) {
//                 if ( strncmp(de->d_name, ".", 2) != 0
//                         &&
//                         strncmp(de->d_name, "..", 3) != 0
//                    ) {

                    printf("directory found: %s\n   {\n", de->d_name);
                    
                    root.children[i] = LoadRainTextures(buff); 
                    strncpy(root.children[i].name, de->d_name, 30);

                    printf("    }\n");
                } else {
                    strncpy(root.children[i].name, "", 30);
                }
            }
        }
        i++;

    }

    closedir(dr);

    printf("loaded %d png textures\n", pngcount);


    return root;
}

void UnloadRainTextures(TexTreeNode root) {
   
    if (root.numChildren > 0) {
        for (int i = 0 ; i < root.numChildren; i++) {
            UnloadRainTextures(root.children[i]);
        }
        free(root.children);
        printf("unloaded %d children\n", root.numChildren);
    }
    if (root.type == LEAF) {
        UnloadTexture(root.texture);
    }
}

void LoadTextures(TexTreeNode tree, Texture2D **loc) {
    // recursive load of parsed file tree into buffer
    if (tree.numChildren < 1) {
        return;
    }
    if (tree.children[0].type == LEAF && strstr(tree.children[0].name, ".png") != NULL) {
        // our child is a texture and we need to load them

        if (strstr(tree.name, "size") != NULL) {
            // we are in the env db
            for (int i = 0; i < 48; i++) {
                **loc = tree.children[i].texture;
                (*loc)++;
            }

        } else {
            // we are in the point light db

            for (int i = 0; i < 740; i++) {
                **loc = tree.children[i].texture;
                (*loc)++;
            }

        }
    } else {
        // we are a branch and we should recursively explore

        for (int i = 0; i < tree.numChildren; i++) {
            LoadTextures(tree.children[i], loc);
        }
    }

}

Texture2D *ConsolidateArray(TexTreeNode root) {
    // hacky way to consolidate in one array because I know the contents of the database

    // Texture2D array[14992] = {0};
    Texture2D *array = malloc(sizeof(Texture2D) * 14992);
    Texture2D *arptr = array;
    LoadTextures(root, &arptr);

    return array;
}

int rtod(float r) {
    return ((r / PI) * 180);
}

TexDescriptor tqtotd(TexQuery tq) {
    TexDescriptor td = {0};
    td.cv = rtod(tq.cv);
    td.v = rtod(tq.v);
    td.h = rtod(tq.h);
    td.osc = rtod(tq.osc);
    return td;
}

int PointLightRainTexOffset(TexQuery tq) {

    TexDescriptor td = tqtotd(tq);
    int offset = TexIndex(td) + td.cv * 740;

    return offset;
}

// void dfs_print(int level, TexTreeNode node) {
//     
//     if (node.type == LEAF) 
//         printf("L( %d ) ", node.texture.id);
//     else 
//         if (level == 0) {
//             printf("N() ");
//         } else {
//             for (int i = 0; i < node.numChildren; i++) {
//                 dfs_print(level - 1, node.children[i]);
//             }
//         }
// }
// 
// void debugPrintTexTree(TexTreeNode root) {
//     int level = 0;
//     TexTreeNode node = root;
//     while (node.numChildren > 0) {
//         node = node.children[0];
//         level++;
//     }
//     int height = level + 1;
//     level = 0;
// 
//     printf("\n");
//     while (level < height) {
//         dfs_print(level, root);
//         level++;
//         printf("\n");
//     }
// 
//     printf("\n\n Height: %d\n", height);
// }


#endif
