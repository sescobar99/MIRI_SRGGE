#ifndef CONSTANTS_H
#define CONSTANTS_H

#pragma once

namespace EngineConfig 
{
    // Total number of discrete LOD asset slots stored per mesh
    const int NUM_LOD_LEVELS = 5;

    // TODO: Lab5
    // The runtime toggle mode index that triggers the dynamic optimizer loop
    //Set this way to use the last LOD slot+1 for dynamic optimization
    const int RUNTIME_OPTIMIZER_MODE = NUM_LOD_LEVELS; 

    // Cell scaling dimension boundaries for the clustering matrix
    // Level 0 uses a dummy 0.0f value since it maps directly to the original asset
    const float CLUSTERING_CELL_SIZES[NUM_LOD_LEVELS] = { 
        0.000f, // LOD 0: Baseline Asset
        0.010f, // LOD 1: Fine
        0.030f, // LOD 2: Medium
        0.060f, // LOD 3: Coarse
        0.120f  // LOD 4: Coarsest
    };
}

#endif // CONSTANTS_H