#include <iostream>
#include <cmath>
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include "Scene.h"
#include "PLYReader.h"
#include "Application.h"


Scene::Scene()
{
	meshCube = NULL;
}

Scene::~Scene()
{
	if(meshCube != NULL)
		delete meshCube;

    for(vector<TriangleMesh *>::iterator it=loadedMeshes.begin(); it!=loadedMeshes.end(); it++)
        delete *it;

	for(vector<TriangleMeshInstance *>::iterator it=objects.begin(); it!=objects.end(); it++)
		delete *it;
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

bool Scene::loadMap(const string &filename)
{
    ifstream fin(filename);

    if (!fin.is_open()){
        return false;
    }

    //// MAP 
    
    // Read first line of the map file, which contains the number of "columns" and "rows"
    // of the map
    int columns, rows;
    if (!(fin >> columns >> rows)){
        return false;
    }
    
    // Read the 2D grid matrix line by line
    vector<string> grid(rows);
    for (int i = 0; i < rows; ++i) {
        fin >> grid[i];
    }
    
    // Debug output to verify that the map was read correctly
    cout << "Map size: " << columns << " x " << rows << endl;
    cout << "Map grid:" << endl;
    for (const auto& line : grid) {
        cout << line << endl;
    }   

    // Room construction based on the grid layout
    buildRoom(grid, columns, rows);

    //// MODELS 
	int numModels;
    if (!(fin >> numModels)){
        return false;
    }

    for (int i = 0; i < numModels; ++i) {
        string modelPath;
        if (!(fin >> modelPath)){
            return false;
        }
        
        TriangleMesh* mesh = loadMesh(modelPath);
        if (mesh != NULL) {
            loadedMeshes.push_back(mesh);
            cout << "Loaded model: " << modelPath << endl;
        } else {
            std::cerr << "Failed to load model: " << modelPath << std::endl;
            return false;
        }
    }

    // Instance placement
    placeInstances(fin);

    fin.close();
	return true;
}

// Loads the mesh into CPU memory and sends it to GPU memory (using GL)

TriangleMesh *Scene::loadMesh(const string &filename) const
{
	TriangleMesh *mesh;
#pragma warning( push )
#pragma warning( disable : 4101)
	PLYReader reader;
#pragma warning( pop ) 

	mesh = new TriangleMesh();
	bool bSuccess = reader.readMesh(filename, *mesh);
	if(bSuccess)
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
}

// Render the scene. First the room, then the mesh it there is one loaded.
void Scene::render()
{
	Application::instance().getShader()->use();
	camera.render();
	for(vector<TriangleMeshInstance *>::iterator it=objects.begin(); it!=objects.end(); it++)
		(*it)->render();
}

VectorCamera &Scene::getCamera()
{
	return camera;
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
    const float tileWidth  = 1.0f;
    const float wallHeight = 2.0f;
    const float floorThickness = 0.1f;

    // Iterate through the grid and create instances for walls and floors based on the tile type
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < columns; ++c) {
            char tile = grid[r][c];
           
            // Calculate spatial position
            float worldX = (static_cast<float>(c) * tileWidth) + offsetX;
            float worldZ = (static_cast<float>(r) * tileWidth) + offsetZ;
                        
            transform = glm::mat4(1.0f);
            instance = new TriangleMeshInstance();

            if (tile == '1') {
                // WALL: Scale up vertically, translate up by half its height
                transform = glm::translate(transform, glm::vec3(worldX, wallHeight / 2.0f, worldZ));
                transform = glm::scale(transform, glm::vec3(tileWidth, wallHeight, tileWidth));
                instance->init(meshCube, glm::vec4(0.6f, 0.6f, 0.6f, 1.0f), transform, 0.0f, 0.9f);
                objects.push_back(instance);
            } 
            else if (tile == '0') {
                // FLOOR: Flat tile slightly lowered below the Y=0 threshold
                transform = glm::translate(transform, glm::vec3(worldX, -floorThickness / 2.0f, worldZ));
                transform = glm::scale(transform, glm::vec3(tileWidth, floorThickness, tileWidth));
                instance->init(meshCube, glm::vec4(0.137f, 0.094f, 0.074f, 1.0f), transform, 0.1f, 0.85f);
                objects.push_back(instance);
            }
        }
        
    }
    
	// Ceiling
	transform = glm::mat4(1.0f);
	transform = glm::translate(transform, glm::vec3(0.f, 2.05f, 0.f));
	transform = glm::scale(transform, glm::vec3(columns * tileWidth, 0.1f, rows * tileWidth));
	instance = new TriangleMeshInstance();
	instance->init(meshCube, glm::vec4(0.525f, 0.517f, 0.478f, 1.0f), transform, 0.1f, 0.65f);
	objects.push_back(instance);
    
	// // Base
	// transform = glm::mat4(1.0f);
	// transform = glm::translate(transform, glm::vec3(0.0, 0.0f, -1.0f));
	// transform = glm::scale(transform, glm::vec3(0.5f, 0.75f, 0.5f));
	// instance = new TriangleMeshInstance();
	// instance->init(meshBase, glm::vec4(1.0f), transform, 0.15f, 0.75f);
	// objects.push_back(instance);

}


bool Scene::placeInstances(ifstream &fin)
{
    int numInstances;
    glm::mat4 transform;
    TriangleMeshInstance *instance;

    if (!(fin >> numInstances)){
        return false;
    }

    for (int i = 0; i < numInstances; ++i) {
        int modelIndex;
        float tx, ty, tz;
        float sx, sy, sz;
        float rx, ry, rz;

        if (!(fin >> modelIndex >> tx >> ty >> tz >> sx >> sy >> sz >> rx >> ry >> rz)){
            cout << "Error reading instance data for instance " << i << endl;
            return false;
        }

        // Validation guard
        if (modelIndex < 0 || modelIndex >= static_cast<int>(loadedMeshes.size())){
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
        instance->init(loadedMeshes[modelIndex], glm::vec4(1.0f), transform, 0.0f, 1.0f);
        
        objects.push_back(instance);
    }

    return true;
}



