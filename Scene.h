#ifndef _SCENE_INCLUDE
#define _SCENE_INCLUDE

#include <glm/glm.hpp>
#include <vector>
#include "VectorCamera.h"
#include "TriangleMesh.h"
#include "TriangleMeshInstance.h"
#include "Constants.h"

using namespace std;

// Scene contains all the entities of our game.
// It is responsible for updating and render them.

class Scene
{

public:
    Scene();
    ~Scene();

    void init();
    // Lab 1
    bool loadMap(const string &filename, ClusteringMode mode);
    // Lab 2
    TriangleMesh *loadMesh(const string &filename, int lodLevel = 0, ClusteringMode mode = ClusteringMode::Basic) const;
    void update(int deltaTime);
    void render();

    VectorCamera &getCamera();

    // Lab 2
    void setGlobalLOD(int level);

private:
    void computeModelViewMatrix();
    // Lab 1
    void buildRoom(const vector<string> &grid, int columns, int rows);
    // Lab 1
    bool placeInstances(ifstream &fin);

private:
    VectorCamera camera;
    TriangleMesh *meshCube;
    // Lab 2
    struct ModelLODGroup
    {
        TriangleMesh *lod[EngineConfig::NUM_LOD_LEVELS];
    };
    // Lab 2
    // Holds all the loaded LOD variants of each unique mesh in the scene, indexed by their file path
    std::vector<ModelLODGroup> loadedModels;
    // Holds all the instances of meshes in the scene, including background architecture and museum exhibits
    vector<TriangleMeshInstance *> objects;
    float currentTime;
    // Lab 2
    int currentGlobalLOD;
};

#endif // _SCENE_INCLUDE
