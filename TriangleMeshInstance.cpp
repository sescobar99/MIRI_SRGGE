#include "Application.h"
#include "TriangleMeshInstance.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Constants.h"

TriangleMeshInstance::TriangleMeshInstance()
{
	staticMesh = NULL;
	bUseLOD = false;
	currentLOD = 0;
	for (int i = 0; i < EngineConfig::NUM_LOD_LEVELS; ++i)
	{
		LODMeshes[i] = NULL;
	}
	metallic = 0.0f;
	roughness = 1.0f;
}

TriangleMeshInstance::~TriangleMeshInstance()
{
}

void TriangleMeshInstance::initStatic(TriangleMesh *staticMesh, const glm::vec4 &color, const glm::mat4 &transform, float metallic, float roughness)
{
	this->staticMesh = staticMesh;
	this->color = color;
	this->transform = transform;
	this->metallic = metallic;
	this->roughness = roughness;
	bUseLOD = false; // Disables LOD processing routines
}

void TriangleMeshInstance::initLOD(TriangleMesh *lods[EngineConfig::NUM_LOD_LEVELS], const glm::vec4 &color, const glm::mat4 &transform, float metallic, float roughness)
{
	for (int i = 0; i < EngineConfig::NUM_LOD_LEVELS; ++i)
	{
		LODMeshes[i] = lods[i];
	}
	this->color = color;
	this->transform = transform;
	this->metallic = metallic;
	this->roughness = roughness;
	bUseLOD = true;								   // Enables LOD processing routines
	currentLOD = EngineConfig::NUM_LOD_LEVELS - 1; // Start coarse
}

void TriangleMeshInstance::setLODLevel(int level)
{
	if (!bUseLOD)
		return; // Protect background walls/floors

	if (level == EngineConfig::RUNTIME_OPTIMIZER_MODE)
		return; // Lab 5 Reserved fordynamic calculations
	currentLOD = level;
}

void TriangleMeshInstance::render()
{
	TriangleMesh *meshToRender = bUseLOD ? LODMeshes[currentLOD] : staticMesh;
	if (meshToRender == nullptr)
		return;

	ShaderProgram *shader = Application::instance().getShader();
	shader->use();

	// Lab 5 Color debugging
	glm::vec4 renderColor = color;
	if (bUseLOD && Application::instance().getDebugLODColors())
	{
		renderColor = EngineConfig::LOD_TINTS[currentLOD];
	}

	shader->setUniform4f("color", renderColor.r, renderColor.g, renderColor.b, renderColor.a);
	shader->setUniform1f("metallic", metallic);
	shader->setUniform1f("roughness", roughness);
	shader->setUniformMatrix4f("model", transform);
	meshToRender->render();
}

TriangleMesh *TriangleMeshInstance::getStaticMesh()
{
	return staticMesh;
}

void TriangleMeshInstance::setTransform(const glm::mat4 &transform)
{
	this->transform = transform;
}

const glm::mat4 &TriangleMeshInstance::getTransform() const
{
	return transform;
}

void TriangleMeshInstance::addTransform(const glm::mat4 &addedTransform)
{
	transform = addedTransform * transform;
}

void TriangleMeshInstance::resetTransform()
{
	transform = glm::mat4(1.0f);
}

void TriangleMeshInstance::translate(const glm::vec3 &move)
{
	transform = glm::translate(glm::mat4(1.0f), move) * transform;
}

void TriangleMeshInstance::rotate(const glm::vec3 &axis, float angleDegrees)
{
	transform = glm::rotate(glm::mat4(1.0f), angleDegrees * 3.14159f / 180.f, axis) * transform;
}

void TriangleMeshInstance::scale(const glm::vec3 &factor)
{
	transform = glm::scale(glm::mat4(1.0f), factor) * transform;
}

void TriangleMeshInstance::setColor(const glm::vec4 &color)
{
	this->color = color;
}

const glm::vec4 &TriangleMeshInstance::getColor() const
{
	return color;
}

void TriangleMeshInstance::setMaterial(float metallic, float roughness)
{
	this->metallic = metallic;
	this->roughness = roughness;
}

float TriangleMeshInstance::getMetallic() const
{
	return metallic;
}

float TriangleMeshInstance::getRoughness() const
{
	return roughness;
}

// TODO: Avoid such Estimators
glm::vec3 TriangleMeshInstance::getBBCenter() const
{
	// PLYReader::rescaleModel already calculates a raw estimate of a bounding box (makes largest
	// dimension= 1). So, is possible to use that same info in order to avoid AABB calculation on
	// the fly
	
	// Models are located in a way such that the lay in the floor perfectly.
	// As Y axis is not going to change. We can assume the are centered  int the origin (0,0,0)
	return glm::vec3(transform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
}

// TODO: Avoid such Estimators
float TriangleMeshInstance::getBBDiagonal() const
{
	// The scale factors can be obtained from transformation matrix.
	// This take a big assumption on uniform scaling and does not account for model rotation
	float maxScale = glm::length(glm::vec3(transform[0]));
	
	// Diagonal of a 1x1x1 cube is sqrt(3) ~ 1.732f
	// Assumption that the BB is in reality the unitary cube
	return 1.732f * maxScale;
}

size_t TriangleMeshInstance::getCurrentFaceCount() const
{
	if (bUseLOD)
		return LODMeshes[currentLOD]->getFaceCount();
	else
		return staticMesh->getFaceCount();
}

bool TriangleMeshInstance::hasLODLevel(int level) const
{
	if (!bUseLOD)
		return false;
	if (level < 0 || level >= EngineConfig::NUM_LOD_LEVELS)
		return false;
	return LODMeshes[level] != nullptr;
}

size_t TriangleMeshInstance::getFaceCountAtLOD(int level) const
{
	// This operation has no meaning for non-lod models
	if (!bUseLOD)
		return 0;
	if (level < 0 || level >= EngineConfig::NUM_LOD_LEVELS)
		return 0;
	if (LODMeshes[level] == nullptr)
		return 0;
	return LODMeshes[level]->getFaceCount();
}