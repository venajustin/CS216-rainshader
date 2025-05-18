#ifndef PARTICLES_H
#define PARTICLES_H

#include "raymath.h"
#include "stdlib.h"

typedef struct Particle {
    Vector3 p;
    Vector3 v;
} Particle;

Vector3 randomPos(Vector3 min, Vector3 max) {
        
    double xrand = rand()/(double)RAND_MAX;
    double yrand = rand()/(double)RAND_MAX;
    double zrand = rand()/(double)RAND_MAX;
    
    Vector3 out = {
        Lerp(min.x, max.x, xrand), 
        Lerp(min.y, max.y, yrand), 
        Lerp(min.z, max.z, zrand)
    };
    return out;

}


#endif


