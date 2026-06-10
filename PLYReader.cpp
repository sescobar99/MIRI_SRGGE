#include <iostream>
#include <cstring>
#include <vector>
#include <string>
#include "PLYReader.h"

#include <unordered_map>
#include <cmath>
#include <glm/gtx/hash.hpp>



// Helper function to generate the LOD filename (e.g., "bunny.ply" -> "bunny_LOD1.ply")
string PLYReader::getLODFilename(const string &originalPath, int lodLevel) {
    if (lodLevel == 0){
		return originalPath;
    }
    size_t dotPos = originalPath.find_last_of('.');
    return originalPath.substr(0, dotPos) + "_LOD" + to_string(lodLevel) + ".ply";
}

// Reads the mesh from the PLY file, first the header, then the vertex data, 
// and finally the face data. Then it rescales the model so that it fits a
// box of size 1x1x1 centered at the origin

bool PLYReader::readMesh(const string &filename, TriangleMesh &mesh, int lodLevel)
{
	string targetFilename = getLODFilename(filename, lodLevel);
    ifstream fin(targetFilename.c_str(), ios_base::in | ios_base::binary);

	int nVertices, nFaces;
	bool hasColors = false;
	bool hasAlpha = false;

	// HIT: Requested LOD already exists
    if (fin.is_open()) {

		if(!loadHeader(fin, nVertices, nFaces, hasColors, hasAlpha))
		{
			cerr << "Failed to load header for file: " << targetFilename << std::endl;
			fin.close();
			return false;
		} 
		
		vector<float> plyVertices;
		vector<float> plyColors;
		vector<int> plyTriangles;
		loadVertices(fin, nVertices, plyVertices, plyColors, hasColors, hasAlpha);
		loadFaces(fin, nFaces, plyTriangles);
		fin.close();
		
		// If it's a pre-cached LOD file, it's already rescaled. Only rescale original files.
		if (lodLevel == 0){
			rescaleModel(plyVertices);
		}
		addModelToMesh(plyVertices, plyColors, plyTriangles, mesh);
		return true;
    }
	
	// MISS: Need to load original file, process it, and save the LOD version for future 
    ifstream finOrig(filename.c_str(), ios_base::in | ios_base::binary);
    if (!finOrig.is_open()){
		return false; // Original base asset missing completely
	}
	
	if (!loadHeader(finOrig, nVertices, nFaces, hasColors, hasAlpha)) {
		cerr << "Failed to load header for file: " << filename << std::endl;
        finOrig.close();
        return false;
    }
    
    vector<float> origVertices, simplifiedVertices;
	vector<float> origColors, simplifiedColors;
    vector<int> origTriangles, simplifiedTriangles;
    
    loadVertices(finOrig, nVertices, origVertices, origColors, hasColors, hasAlpha);
    loadFaces(finOrig, nFaces, origTriangles);
    finOrig.close();

    // Standardize coordinate sizes around uniform space constraints before processing geometry
    rescaleModel(origVertices);

    // Space partitioning cell size is determined by the LOD level. 
	// Higher LOD levels have larger cell sizes, resulting in more aggressive simplification.
    float targetCellSize = EngineConfig::CLUSTERING_CELL_SIZES[lodLevel];
    // Lab 2. Vertex clustering function 
    clusterVertices(origVertices, origColors, origTriangles, targetCellSize, 
                    simplifiedVertices, simplifiedColors, simplifiedTriangles);

    // Store simplified mesh
    if (saveMeshToPLY(targetFilename, simplifiedVertices, simplifiedColors, simplifiedTriangles)) {
        cout << "Successfully stored simplified mesh. " << targetFilename << endl;
    } else {
        cerr << "WARNING: Failed to store simplified mesh. " << targetFilename << endl;
    }

    addModelToMesh(simplifiedVertices, simplifiedColors, simplifiedTriangles, mesh);
    return true;
}

// Reads the header of a PLY file.
// It first checks that the file is really a PLY. 
// Then it reads lines until it finds the 'end_header'
// The 'element vertex' and 'element face' lines contain the number 
// of primitives in the file.

bool PLYReader::loadHeader(ifstream &fin, int &nVertices, int &nFaces, bool &hasColors, bool &hasAlpha)
{
	string line;
	bool parsingVertex = false;

	nVertices = 0;
	nFaces = 0;
	hasColors = false;
	hasAlpha = false;

	if(!getline(fin, line) || line.compare(0, 3, "ply") != 0)
		return false;
	while(getline(fin, line))
	{
		if(line.compare(0, 10, "end_header") == 0)
			break;
		if(line.compare(0, 14, "element vertex") == 0)
		{
			nVertices = atoi(&line[15]);
			parsingVertex = true;
			continue;
		}
		if(line.compare(0, 12, "element face") == 0)
		{
			nFaces = atoi(&line[13]);
			parsingVertex = false;
			continue;
		}
		if(parsingVertex && line.compare(0, 8, "property") == 0)
		{
			if(line.find("red") != string::npos || line.find("green") != string::npos || line.find("blue") != string::npos)
				hasColors = true;
			if(line.find("alpha") != string::npos)
				hasAlpha = true;
		}
	}
	if(nVertices <= 0)
		return false;
	cout << "Loading triangle mesh" << endl;
	cout << "\tVertices = " << nVertices << endl;
	cout << "\tFaces = " << nFaces << endl;
	cout << "\tColors = " << (hasColors ? "yes" : "no") << endl;
	cout << endl;

	return true;
}

// Loads the vertices' coordinates into a vector

void PLYReader::loadVertices(ifstream &fin, int nVertices, vector<float> &plyVertices, vector<float> &plyColors, bool hasColors, bool hasAlpha)
{
	int i;
	float value;
	unsigned char color;

	plyVertices.resize(3*nVertices);
	plyColors.resize(3*nVertices, 1.f);
	for(i=0; i<nVertices; i++)
	{
		fin.read((char *)&value, sizeof(float));
		plyVertices[3*i] = value;
		fin.read((char *)&value, sizeof(float));
		plyVertices[3*i+1] = value;
		fin.read((char *)&value, sizeof(float));
		plyVertices[3*i+2] = value;
		if(hasColors)
		{
			fin.read((char *)&color, sizeof(unsigned char));
			plyColors[3*i] = color / 255.f;
			fin.read((char *)&color, sizeof(unsigned char));
			plyColors[3*i+1] = color / 255.f;
			fin.read((char *)&color, sizeof(unsigned char));
			plyColors[3*i+2] = color / 255.f;
			if(hasAlpha)
				fin.read((char *)&color, sizeof(unsigned char)); // Discard alpha if present
		}
	}
}

// Same thing for the faces. Those with more than three sides
// are subdivided into triangles.

void PLYReader::loadFaces(ifstream &fin, int nFaces, vector<int> &plyTriangles)
{
	int i, tri[3];
	unsigned char nVrtxPerFace;

	plyTriangles.reserve(nFaces * 3);
	for(i=0; i<nFaces; i++)
	{
		fin.read((char *)&nVrtxPerFace, sizeof(unsigned char));
		fin.read((char *)&tri[0], sizeof(int));
		fin.read((char *)&tri[1], sizeof(int));
		fin.read((char *)&tri[2], sizeof(int));
		plyTriangles.push_back(tri[0]);
		plyTriangles.push_back(tri[1]);
		plyTriangles.push_back(tri[2]);
		for(; nVrtxPerFace>3; nVrtxPerFace--)
		{
			tri[1] = tri[2];
			fin.read((char *)&tri[2], sizeof(int));
			plyTriangles.push_back(tri[0]);
			plyTriangles.push_back(tri[1]);
			plyTriangles.push_back(tri[2]);
		}
	}
}

// Rescales the model to fit a box of 1x1x1 with its base centered at the origin

void PLYReader::rescaleModel(vector<float> &plyVertices)
{
	unsigned int i;
	glm::vec3 baseCenter, size[2];

	size[0] = glm::vec3(1e10, 1e10, 1e10);
	size[1] = glm::vec3(-1e10, -1e10, -1e10);
	for(i=0; i<plyVertices.size(); i+=3)
	{
		size[0][0] = glm::min(size[0][0], plyVertices[i]);
		size[0][1] = glm::min(size[0][1], plyVertices[i+1]);
		size[0][2] = glm::min(size[0][2], plyVertices[i+2]);
		size[1][0] = glm::max(size[1][0], plyVertices[i]);
		size[1][1] = glm::max(size[1][1], plyVertices[i+1]);
		size[1][2] = glm::max(size[1][2], plyVertices[i+2]);
	}
	baseCenter = glm::vec3((size[0][0] + size[1][0]) / 2.f, size[0][1], (size[0][2] + size[1][2]) / 2.f);

	float largestSize = glm::max(size[1][0] - size[0][0], glm::max(size[1][1] - size[0][1], size[1][2] - size[0][2]));

	for(i=0; i<plyVertices.size(); i+=3)
	{
		plyVertices[i] = (plyVertices[i] - baseCenter[0]) / largestSize;
		plyVertices[i+1] = (plyVertices[i+1] - baseCenter[1]) / largestSize;
		plyVertices[i+2] = (plyVertices[i+2] - baseCenter[2]) / largestSize;
	}
}

// Vertex and face data are added to the model using this function

void PLYReader::addModelToMesh(const vector<float> &plyVertices, const vector<float> &plyColors, const vector<int> &plyTriangles, TriangleMesh &mesh)
{
	mesh.initVertices(plyVertices, plyColors);
	mesh.initTriangles(plyTriangles);
}



struct CellData {
    glm::vec3 sumPos = glm::vec3(0.0f);
    glm::vec3 sumColor = glm::vec3(0.0f);
    int count = 0;
    int targetIndex = -1;
};


void PLYReader::clusterVertices(const vector<float> &inVertices, const vector<float> &inColors, const vector<int> &inTriangles,
                                float cellSize,
                                vector<float> &outVertices, vector<float> &outColors, vector<int> &outTriangles)
{
    std::unordered_map<glm::ivec3, CellData> grid;

    // Phase 1: Grid Discretization and Accumulation
	// For each vertex in a model
    int numVertices = inVertices.size() / 3;
    for (int i = 0; i < numVertices; ++i) {
        glm::vec3 pos(inVertices[3 * i], inVertices[3 * i + 1], inVertices[3 * i + 2]);
        glm::vec3 col(inColors[3 * i], inColors[3 * i + 1], inColors[3 * i + 2]);
		
		// 1. Compute which regular grid node contains it
        glm::ivec3 coord(
            std::floor(pos.x / cellSize),
            std::floor(pos.y / cellSize),
            std::floor(pos.z / cellSize)
        );
		// 2. Add to sum of points in that cell (+ add one to count of points)
        CellData &cell = grid[coord];
        cell.sumPos += pos;
        cell.sumColor += col;
        cell.count++;
    }


    // Phase 2: Compute Representatives (Averages)
    int nextIndex = 0;
	// For each node with points
	for (std::unordered_map<glm::ivec3, CellData>::iterator it = grid.begin(); it != grid.end(); ++it) {
		// 1. Add representative in that node as output vertex
		CellData &cell = it->second;
        glm::vec3 avgPos = cell.sumPos / static_cast<float>(cell.count);
        glm::vec3 avgCol = cell.sumColor / static_cast<float>(cell.count);

        outVertices.push_back(avgPos.x);
        outVertices.push_back(avgPos.y);
        outVertices.push_back(avgPos.z);

        outColors.push_back(avgCol.x);
        outColors.push_back(avgCol.y);
        outColors.push_back(avgCol.z);

		// 2. Store the id of that output vertex in node
        cell.targetIndex = nextIndex++;
    }

    // Phase 3: Topology Remapping and Triangle Elimination
	// For each input triangle T
    for (size_t i = 0; i < inTriangles.size(); i += 3) {
		int origIdx0 = inTriangles[i];
        int origIdx1 = inTriangles[i + 1];
        int origIdx2 = inTriangles[i + 2];

		// 1. Determine node that contains each vertex of T
        glm::ivec3 coord0(
            std::floor(inVertices[3 * origIdx0] / cellSize),
            std::floor(inVertices[3 * origIdx0 + 1] / cellSize),
            std::floor(inVertices[3 * origIdx0 + 2] / cellSize)
        );
        
        glm::ivec3 coord1(
            std::floor(inVertices[3 * origIdx1] / cellSize),
            std::floor(inVertices[3 * origIdx1 + 1] / cellSize),
            std::floor(inVertices[3 * origIdx1 + 2] / cellSize)
        );
        
        glm::ivec3 coord2(
            std::floor(inVertices[3 * origIdx2] / cellSize),
            std::floor(inVertices[3 * origIdx2 + 1] / cellSize),
            std::floor(inVertices[3 * origIdx2 + 2] / cellSize)
        );

		// 2. Update id in triangle to the one stored in node
        int id0 = grid[coord0].targetIndex;
        int id1 = grid[coord1].targetIndex;
        int id2 = grid[coord2].targetIndex;

		// 3. If triangle has unique ids output it (Filters Degenerate Faces)
        if (id0 != id1 && id1 != id2 && id2 != id0) {
            outTriangles.push_back(id0);
            outTriangles.push_back(id1);
            outTriangles.push_back(id2);
        }
    }
}

bool PLYReader::saveMeshToPLY(const string &filename, const vector<float> &vertices, const vector<float> &colors, const vector<int> &triangles)
{
    ofstream fout(filename, ios_base::out | ios_base::binary);
    if (!fout.is_open()) return false;

    int numVertices = vertices.size() / 3;
    int numFaces = triangles.size() / 3;

	// Headers
    fout << "ply\n";
    fout << "format binary_little_endian 1.0\n";
    fout << "element vertex " << numVertices << "\n";
    fout << "property float x\n";
    fout << "property float y\n";
    fout << "property float z\n";
    fout << "property uchar red\n";
    fout << "property uchar green\n";
    fout << "property uchar blue\n";	
    fout << "element face " << numFaces << "\n";
    fout << "property list uchar int vertex_indices\n";
    fout << "end_header\n";

	// Binary info
    for (int i = 0; i < numVertices; ++i) {
		// Geometry
        fout.write(reinterpret_cast<const char*>(&vertices[3 * i]), 3 * sizeof(float));
		// Color
        unsigned char r = static_cast<unsigned char>(colors[3 * i] * 255.0f);
        unsigned char g = static_cast<unsigned char>(colors[3 * i + 1] * 255.0f);
        unsigned char b = static_cast<unsigned char>(colors[3 * i + 2] * 255.0f);
        fout.write(reinterpret_cast<const char*>(&r), sizeof(unsigned char));
        fout.write(reinterpret_cast<const char*>(&g), sizeof(unsigned char));
        fout.write(reinterpret_cast<const char*>(&b), sizeof(unsigned char));
    }

    unsigned char verticesPerFace = 3;
    for (int i = 0; i < numFaces; ++i) {
		// Topology
        fout.write(reinterpret_cast<const char*>(&verticesPerFace), sizeof(unsigned char));
        fout.write(reinterpret_cast<const char*>(&triangles[3 * i]), 3 * sizeof(int));
    }

    fout.close();
    return true;
}