#include <iostream>
#include <cmath>
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include "Scene.h"
#include "PLYReader.h"
#include "Application.h"
#include "Constants.h"

Scene::Scene()
{
    meshCube = NULL;
}

Scene::~Scene()
{
    if (meshCube != NULL)
        delete meshCube;

    // Free instances
    for (vector<TriangleMeshInstance *>::iterator it = objects.begin(); it != objects.end(); it++)
    {
        delete *it;
    }
    objects.clear();

    // Free original models
    for (auto &modelGroup : loadedModels)
    {
        for (int i = 0; i < EngineConfig::NUM_LOD_LEVELS; ++i)
        {
            if (modelGroup.lod[i] != nullptr)
            {
                modelGroup.lod[i]->free();
                delete modelGroup.lod[i];
                modelGroup.lod[i] = nullptr;
            }
        }
    }
    loadedModels.clear();
}

// Initialize the scene. This includes the cube we will use to render
// the floor and ceiling, as well as the camera.

void Scene::init()
{
    meshCube = new TriangleMesh();

    // Floor, ceiling and walls will be rendered using the same cube mesh, scaled and
    // transformed to build the room
    meshCube->buildCube();

    meshCube->sendToOpenGL();
    currentTime = 0.0f;

    camera.init(glm::vec3(0.f, 1.0f, 0.f));
}

// Load the map & all its associated models

bool Scene::loadMap(const string &filename, ClusteringMode mode)
{
    ifstream fin(filename);

    if (!fin.is_open())
    {
        return false;
    }

    //// MAP

    // Read first line of the map file, which contains the number of "columns" and "rows"
    // of the map
    int columns, rows;
    if (!(fin >> columns >> rows))
    {
        return false;
    }

    // Read the 2D grid matrix line by line
    vector<string> grid(rows);
    for (int i = 0; i < rows; ++i)
    {
        fin >> grid[i];
    }

    // Debug output to verify that the map was read correctly
    cout << "Map size: " << columns << " x " << rows << endl;
    cout << "Map grid:" << endl;
    for (const auto &line : grid)
    {
        cout << line << endl;
    }

    // Room construction based on the grid layout
    buildRoom(grid, columns, rows);

    //// MODELS
    int numModels;
    if (!(fin >> numModels))
    {
        return false;
    }

    for (int i = 0; i < numModels; ++i)
    {
        string modelPath;
        if (!(fin >> modelPath))
        {
            return false;
        }

        ModelLODGroup group;
        bool loadSuccess = true;
        // Ensure all LOD variants are generated/cached up front
        for (int lodLevel = 0; lodLevel < EngineConfig::NUM_LOD_LEVELS; ++lodLevel)
        {
            // Load Mesh internally delegates the LOD generation to PLYReader, which
            // checks for existing LOD files and creates them if they don't exist
            group.lod[lodLevel] = loadMesh(modelPath, lodLevel, mode);
            if (group.lod[lodLevel] == NULL)
            {
                loadSuccess = false;
                cerr << "Failed to load LOD level " << lodLevel << " for model: " << modelPath << std::endl;
                break;
            }
            cout << "Successfully loaded LOD level " << lodLevel << " for model: " << modelPath << std::endl;
        }
        if (!loadSuccess)
        {
            return false;
        }
        loadedModels.push_back(group);
    }

    // Instance placement
    currentGlobalLOD = EngineConfig::NUM_LOD_LEVELS - 1; // Start system initialized to coarsest level
    bool success = placeInstances(fin);

    fin.close();
    return success;
}

// Loads the mesh into CPU memory and sends it to GPU memory (using GL)

TriangleMesh *Scene::loadMesh(const string &filename, int lodLevel, ClusteringMode mode) const
{
    TriangleMesh *mesh;
#pragma warning(push)
#pragma warning(disable : 4101)
    PLYReader reader;
#pragma warning(pop)

    mesh = new TriangleMesh();
    bool bSuccess = reader.readMesh(filename, *mesh, lodLevel, mode);
    if (bSuccess)
        mesh->sendToOpenGL();
    else
    {
        delete mesh;
        mesh = NULL;
    }

    return mesh;
}

void Scene::update(int deltaTime)
{
    currentTime += deltaTime;

    // Lab 5. If tcr is not activated, the lods are static and assigned to all models equally
    if (currentGlobalLOD == EngineConfig::RUNTIME_OPTIMIZER_MODE)
        updateLODs(deltaTime);
}

// Render the scene. First the room, then the mesh it there is one loaded.
void Scene::render()
{
    Application::instance().getShader()->use();
    camera.render();
    for (vector<TriangleMeshInstance *>::iterator it = objects.begin(); it != objects.end(); it++)
        (*it)->render();
}

VectorCamera &Scene::getCamera()
{
    return camera;
}

void Scene::adjustBudget(int delta)
{
    faceBudget = std::max(0, faceBudget + delta);
    printf("Face budget: %d\n", faceBudget);
}

void Scene::setGlobalLOD(int level)
{
    currentGlobalLOD = level;

    if (level == EngineConfig::RUNTIME_OPTIMIZER_MODE)
        return; // updateLODs() takes over each frame

    for (auto *obj : objects)
        obj->setLODLevel(level);
}

void Scene::buildRoom(const vector<string> &grid, int columns, int rows)
{
    glm::mat4 transform;
    TriangleMeshInstance *instance;

    // Centering offsets so the grid maps around world coordinate (0,0,0)
    float offsetX = -static_cast<float>(columns) / 2.0f + 0.5f;
    float offsetZ = -static_cast<float>(rows) / 2.0f + 0.5f;
    cout << "OffsetX: " << offsetX << ", OffsetZ: " << offsetZ << endl;

    // Unit size dimensions for each tile slot
    const float tileWidth = 1.0f;
    const float wallHeight = 2.0f;
    const float floorThickness = 0.1f;

    // Iterate through the grid and create instances for walls and floors based on the tile type
    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < columns; ++c)
        {
            char tile = grid[r][c];

            // Calculate spatial position
            float worldX = (static_cast<float>(c) * tileWidth) + offsetX;
            float worldZ = (static_cast<float>(r) * tileWidth) + offsetZ;

            transform = glm::mat4(1.0f);
            instance = new TriangleMeshInstance();

            if (tile == '1')
            {
                // WALL: Scale up vertically, translate up by half its height
                transform = glm::translate(transform, glm::vec3(worldX, wallHeight / 2.0f, worldZ));
                transform = glm::scale(transform, glm::vec3(tileWidth, wallHeight, tileWidth));
                instance->initStatic(meshCube, glm::vec4(0.6f, 0.6f, 0.6f, 1.0f), transform, 0.0f, 0.9f);
                objects.push_back(instance);
            }
            else if (tile == '0')
            {
                // FLOOR: Flat tile slightly lowered below the Y=0 threshold
                transform = glm::translate(transform, glm::vec3(worldX, -floorThickness / 2.0f, worldZ));
                transform = glm::scale(transform, glm::vec3(tileWidth, floorThickness, tileWidth));
                instance->initStatic(meshCube, glm::vec4(0.137f, 0.094f, 0.074f, 1.0f), transform, 0.1f, 0.85f);
                objects.push_back(instance);
            }
        }
    }

    // Ceiling
    transform = glm::mat4(1.0f);
    transform = glm::translate(transform, glm::vec3(0.f, 2.05f, 0.f));
    transform = glm::scale(transform, glm::vec3(columns * tileWidth, 0.1f, rows * tileWidth));
    instance = new TriangleMeshInstance();
    instance->initStatic(meshCube, glm::vec4(0.525f, 0.517f, 0.478f, 1.0f), transform, 0.1f, 0.65f);
    objects.push_back(instance);

    // // Base
    // transform = glm::mat4(1.0f);
    // transform = glm::translate(transform, glm::vec3(0.0, 0.0f, -1.0f));
    // transform = glm::scale(transform, glm::vec3(0.5f, 0.75f, 0.5f));
    // instance = new TriangleMeshInstance();
    // instance->initStatic(meshBase, glm::vec4(1.0f), transform, 0.15f, 0.75f);
    // objects.push_back(instance);
}

bool Scene::placeInstances(ifstream &fin)
{
    int numInstances;
    glm::mat4 transform;
    TriangleMeshInstance *instance;

    if (!(fin >> numInstances))
    {
        return false;
    }

    for (int i = 0; i < numInstances; ++i)
    {
        int modelIndex;
        float tx, ty, tz;
        float sx, sy, sz;
        float rx, ry, rz;

        if (!(fin >> modelIndex >> tx >> ty >> tz >> sx >> sy >> sz >> rx >> ry >> rz))
        {
            cout << "Error reading instance data for instance " << i << endl;
            return false;
        }

        // Validation guard
        if (modelIndex < 0 || modelIndex >= static_cast<int>(loadedModels.size()))
        {
            cout << "Invalid model index for instance: " << modelIndex << endl;
            return false;
        }

        // Compute local model transformation matrix (Translate -> Rotate -> Scale)
        transform = glm::mat4(1.0f);

        transform = glm::translate(transform, glm::vec3(tx, ty, tz));

        transform = glm::rotate(transform, glm::radians(rz), glm::vec3(0.0f, 0.0f, 1.0f));
        transform = glm::rotate(transform, glm::radians(ry), glm::vec3(0.0f, 1.0f, 0.0f));
        transform = glm::rotate(transform, glm::radians(rx), glm::vec3(1.0f, 0.0f, 0.0f));

        transform = glm::scale(transform, glm::vec3(sx, sy, sz));

        // Create the individual
        instance = new TriangleMeshInstance();
        instance->initLOD(loadedModels[modelIndex].lod, glm::vec4(1.0f), transform, 0.0f, 1.0f);

        objects.push_back(instance);
    }

    return true;
}

struct Candidate
{
    TriangleMeshInstance *inst;
    float d;     // BB diagonal (world-space)
    float D;     // distance to camera
    int codeLOD; // current lod (4=coarsest, 0=finest)
    int prevLOD; // LOD at frame start. used to detect real changes for Lab 6
};

// Benefit function.
// formulaL0 -> coarsest (LOD4) | formulaL4 -> finest (LOD0). Needed to follow lab formula
auto computeBenefit = [](int codeIndex, float d, float D) -> float
{
    const int formulaL = (EngineConfig::NUM_LOD_LEVELS - 1) - codeIndex;
    const float denom = glm::pow(2.0f, static_cast<float>(formulaL)) * D;
    return 1.0f - d / denom;
};

void Scene::updateLODs(int deltaTime)
{
    // Lab 5: Time critical rendering
    // - Compute TPS (triangles/second)
    // - MaxCost = TPS / FPS
    // - Benefit = 1 - d / (2^L·D)
    //     - L: Clustering level
    //     - D: Distance between object and viewpoint
    //     - d: Diagonal of the object's bounding box
    // - Max. TotalBenefit, while ensuring that TotalCost < MaxCost
    // Optimization each frame

    const size_t maxCost = static_cast<size_t>(faceBudget);
    const int coarsestCode = EngineConfig::NUM_LOD_LEVELS - 1;

    // Lab 6: Hysteresis
    const int hysteresisFrames = EngineConfig::HYSTERESIS_FRAMES;

    // 0. Fixed cost: static geometry that ignores LOD (walls, floors)
    size_t fixedCost = 0;
    for (auto *obj : objects)
        if (!obj->isLODEnabled())
            fixedCost += obj->getCurrentFaceCount();

    // 1. Build candidate list. Partition LOD objects locked | free
    //    Locked: LOD frozen, counted as fixed cost
    //    Free:   eligible for optimizer -> reset to coarsest, enter pool
    const glm::vec3 camPos = camera.getPosition();
    size_t lockedCost = 0;
    std::vector<Candidate> candidates;
    candidates.reserve(objects.size());
    for (auto *obj : objects)
    {
        if (!obj->isLODEnabled())
            continue;

        if (obj->isHysteresisLocked(hysteresisFrames))
        {
            // Locked -> current LOD is committed for this frame
            lockedCost += obj->getCurrentFaceCount();
        }
        else
        {
            // Free -> capture LOD before reset so lod changes can be detected
            const int prevLOD = obj->getCurrentLOD();
            const float D = glm::length(camPos - obj->getBBCenter());
            const float d = obj->getBBDiagonal();
            // cout << "D: " << D << " | d: " << d<< endl;

            // Start with lowest LOD for all objects
            obj->setLODLevel(coarsestCode);
            // D: Distance between object  and camera
            // d: diagonal of bb
            candidates.push_back({obj, d, D, coarsestCode, prevLOD});
        }
    }

    // Available budget (maxCost already has fixed + locked )
    const size_t availableBudget = (maxCost > (fixedCost + lockedCost)) ? maxCost - fixedCost - lockedCost : 0;

    // Determine total cost of those
    // 2. Calculate initial cost
    size_t totalLODCost = 0;
    for (auto &c : candidates)
        totalLODCost += c.inst->getCurrentFaceCount();

    // While total cost < max cost
    // 3. Greedy upgrade loop
    bool anyUpgrade = true;
    while (anyUpgrade && (totalLODCost < availableBudget))
    {
        anyUpgrade = false;
        float bestRatio = -1.0f;
        int bestIdx = -1;

        // Loop over all models
        for (int i = 0; i < static_cast<int>(candidates.size()); ++i)
        {
            Candidate &c = candidates[i];
            const int nextCode = c.codeLOD - 1; // one step finer

            if (nextCode < 0)
                continue; // already at finest
            if (!c.inst->hasLODLevel(nextCode))
                continue; // mesh not loaded

            const size_t costCurr = c.inst->getFaceCountAtLOD(c.codeLOD);
            const size_t costNext = c.inst->getFaceCountAtLOD(nextCode);

            const size_t deltaCost = costNext - costCurr;

            if ((totalLODCost + deltaCost) > availableBudget)
                continue; // Over budget

            // cout << "CodeLOD: " << c.codeLOD   << " | diagonal: " << c.d <<  " | distance: " << c.D << " | benefit: " << computeBenefit(c.codeLOD, c.d, c.D)  << endl;
            const float bCurr = computeBenefit(c.codeLOD, c.d, c.D);
            const float bNext = computeBenefit(nextCode, c.d, c.D);

            const float dBenefit = bNext - bCurr;
            // cout << "deltaCost: " << deltaCost << " | dBenefit: " << dBenefit << " | bCurr: " << bCurr << " | bNext: " << bNext << endl;

            // Find object with largest delta_benefit / delta_cost
            const float ratio = dBenefit / static_cast<float>(deltaCost);
            if (ratio > bestRatio)
            {
                bestRatio = ratio;
                bestIdx = i;
            }
        }
        // Increase LOD for that object
        // Update total cost
        if (bestIdx >= 0)
        {
            Candidate &best = candidates[bestIdx];
            const int nextCode = best.codeLOD - 1;
            const size_t deltaCost = best.inst->getFaceCountAtLOD(nextCode) - best.inst->getFaceCountAtLOD(best.codeLOD);
            totalLODCost += deltaCost;
            best.codeLOD = nextCode;
            best.inst->setLODLevel(nextCode);
            anyUpgrade = true;
        }
    }

    //  4. Update hysteresis counters
    //  two disjoint groups, no overlap
    //    Locked objects: tick toward unlock (they're not in candidates[])
    //    Free candidates: LOD changed -> re-lock (reset); stable -> tick

    for (auto *obj : objects)
    {
        if (!obj->isLODEnabled())
            continue;
        if (obj->isHysteresisLocked(hysteresisFrames))
            obj->tickHysteresis();
    }

    for (auto &c : candidates)
    {
        if (c.codeLOD != c.prevLOD)
            c.inst->resetHysteresis(); // LOD changed -> re-lock
        else
            c.inst->tickHysteresis(); // LOD un-changed -> closer to be freed
    }

    totalFacesLastFrame = fixedCost + lockedCost + totalLODCost;
}