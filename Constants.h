#ifndef CONSTANTS_H
#define CONSTANTS_H

#pragma once

namespace EngineConfig
{
    // Total number of discrete LOD asset slots stored per mesh
    const int NUM_LOD_LEVELS = 5;

    // Lab 5
    // Rutime toggle mode index that triggers the dynamic optimizer loop
    // Set this way to use the last LOD slot+1 for dynamic optimization
    const int RUNTIME_OPTIMIZER_MODE = NUM_LOD_LEVELS;

    // Lab 5: Triangle budget (faces).
    // const int FACE_BUDGET = 50000; // This is a good number for regular clustering
    const int FACE_BUDGET = 170000; // Normal clustering requires higher budgets
    const int FACE_BUDGET_DELTA = (int)(FACE_BUDGET / 10);

    // Lab 6: frames an object must stay at its LOD before the optimizer can touch it again
    const int HYSTERESIS_FRAMES = 60 * 3; // 3 Second

    // Cell scaling dimension boundaries for the clustering matrix
    // Level 0 uses a dummy 0.0f value since it maps directly to the original asset
    const float CLUSTERING_CELL_SIZES_BC[NUM_LOD_LEVELS] = {
        0.000f, // LOD 0: Baseline Asset
        0.010f, // LOD 1: Fine
        0.030f, // LOD 2: Medium
        0.060f, // LOD 3: Coarse
        0.120f  // LOD 4: Coarsest
    };
    // These are good values for regular vertex clustering but no coarse enough for normal clustering

    const float CLUSTERING_CELL_SIZES_NC[NUM_LOD_LEVELS] = {
        0.000f, // LOD 0: Baseline Asset
        0.020f, // LOD 1: Fine
        0.060f, // LOD 2: Medium
        0.120f, // LOD 3: Coarse
        0.240f  // LOD 4: Coarsest
    };

    // White (Base), Green (finest) → Red (coarsest), one colour per code index
    const glm::vec4 LOD_TINTS[NUM_LOD_LEVELS] = {
        {1.0f, 1.0f, 1.0f, 1.0f}, // LOD 0: White
        {0.1f, 0.9f, 0.1f, 1.0f}, // LOD 1: Green
        {0.9f, 0.9f, 0.1f, 1.0f}, // LOD 2: Yellow
        {0.9f, 0.5f, 0.1f, 1.0f}, // LOD 3: Orange
        {0.9f, 0.1f, 0.1f, 1.0f}, // LOD 4: Red
    };
}

enum class ClusteringMode
{
    Basic,
    NormalClustering
};

#endif // CONSTANTS_H