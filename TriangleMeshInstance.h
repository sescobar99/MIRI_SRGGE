#ifndef _TRIANGLE_MESH_INSTANCE_INCLUDE
#define _TRIANGLE_MESH_INSTANCE_INCLUDE

#include <glm/glm.hpp>
#include "TriangleMesh.h"
#include "Constants.h"

class TriangleMeshInstance
{

public:
	TriangleMeshInstance();
	~TriangleMeshInstance();
	// Lab 2
	// For background architecture (Floor, Walls, Ceiling) - NEVER changes resolution
	void initStatic(TriangleMesh *staticMesh,
					const glm::vec4 &color = glm::vec4(1.0f),
					const glm::mat4 &transform = glm::mat4(1.0f),
					float metallic = 0.0f, float roughness = 1.0f);
	// Lab 2
	// For Museum Exhibits - Responds to LOD updates
	void initLOD(TriangleMesh *lods[EngineConfig::NUM_LOD_LEVELS],
				 const glm::vec4 &color = glm::vec4(1.0f),
				 const glm::mat4 &transform = glm::mat4(1.0f),
				 float metallic = 0.0f, float roughness = 1.0f);

	void setLODLevel(int level);
	void render();

	// Lab 2
	TriangleMesh *getStaticMesh();

	void setTransform(const glm::mat4 &transform);
	const glm::mat4 &getTransform() const;
	void addTransform(const glm::mat4 &addedTransform);

	void resetTransform();
	void translate(const glm::vec3 &move);
	void rotate(const glm::vec3 &axis, float angleDegrees);
	void scale(const glm::vec3 &factor);

	void setColor(const glm::vec4 &color);
	const glm::vec4 &getColor() const;
	void setMaterial(float metallic, float roughness);
	float getMetallic() const;
	float getRoughness() const;

	// Lab 5
	glm::vec3 getBBCenter() const;
	float getBBDiagonal() const;
	size_t getCurrentFaceCount() const;
	bool isLODEnabled() const { return bUseLOD; }
	int getCurrentLOD() const { return currentLOD; }
	bool hasLODLevel(int level) const;
	size_t getFaceCountAtLOD(int level) const;

	// Lab 6
	void tickHysteresis();
	void resetHysteresis();
	bool isHysteresisLocked(int thresholdFrames) const;

private:
	// Lab 2 & +
	// For Museum Exhibits - Holds pointers to all LOD variants of the mesh, and switches between
	// them based on the LOD designation of the instance
	TriangleMesh *LODMeshes[EngineConfig::NUM_LOD_LEVELS];

	// Lab 2 & +
	// For background architecture (Floor, Walls, Ceiling) - Static mesh that doesn't change with LOD updates
	TriangleMesh *staticMesh;

	// Lab 2 & +
	// Flag for LOD processing routines
	bool bUseLOD;

	// Lab 2 & +
	// Tracks the currently active LOD level for this instance
	int currentLOD;
	glm::vec4 color;
	glm::mat4 transform;
	float metallic;
	float roughness;

	// Session 6
	// Initialised to HYSTERESIS_FRAMES. Make models start free -> first N frames not stuck at coarsest
	int framesAtCurrentLOD;
};

#endif // _TRIANGLE_MESH_INSTANCE_INCLUDE
