/*
 * TextureManager
 * Author: Justin Greatorex
 *  
 * Not used in final implementation.
 *
 * Reads in rain streak textures from the rain streak database.
 * (cave.cs.columbia.edu/repository/Rain)
 *
 * Organizes textures into an accessible datastructure and attempts
 * to combine them into a single texture for use in fragment shader
 *
 */

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
    Texture2D texture; // not used unless leaf node
} TexTreeNode;


TexTreeNode LoadRainPNG(char *path) {
    TexTreeNode newN = { 0 };
    newN.numChildren = 0;
    newN.texture = LoadTexture(path);
    return newN;
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

    root.numChildren = count;
    root.children = malloc(sizeof(TexTreeNode) * count);


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

            root.children[i] = LoadRainPNG(buff);
            root.children[i].type = LEAF;
             
            
        } else {
            // test for dir
            DIR *dr2 = opendir(buff);
            if (dr2 != NULL) {

                // if any . we will just consider it not a dir
                bool dir = true;
                for (int j = 0; de->d_name[j] != '\0' && j < 200;j++) {
                    if (de->d_name[j] == '.') {
                        dir = false;
                    }
                }
                if (dir) {
//                 if ( strncmp(de->d_name, ".", 2) != 0
//                         &&
//                         strncmp(de->d_name, "..", 3) != 0
//                    ) {

                    printf("directory found: %s\n   {\n", de->d_name);
                    
                    root.children[i] = LoadRainTextures(buff); 

                    printf("    }\n");
                }
            }
        }

    }

    closedir(dr);

    printf("loaded %d png textures", pngcount);


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
