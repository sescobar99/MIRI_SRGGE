#ifndef PLYREADER_H
#define PLYREADER_H

#include <fstream>
#include <string>
#include <vector>
#include "TriangleMesh.h"
#include "Constants.h"

using namespace std;

// Class used to read PLY files into objects of the TriangleMesh class
// Currently it can only process very specific PLY files in binary format

class PLYReader
{

public:
    // Lab 2
    // Reads the mesh from the PLY file, first the header, then the vertex data,
    // and finally the face data. Then it rescales the model so that it fits a
    // box of size 1x1x1 centered at the origin
    static bool readMesh(const string &filename, TriangleMesh &mesh, int lodLevel = 0, ClusteringMode mode = ClusteringMode::Basic);

private:
    static bool loadHeader(ifstream &fin, int &nVertices, int &nFaces, bool &hasColors, bool &hasAlpha);
    static void loadVertices(ifstream &fin, int nVertices, vector<float> &plyVertices, vector<float> &plyColors, bool hasColors, bool hasAlpha);
    // Those with more than three sides are subdivided into triangles.
    static void loadFaces(ifstream &fin, int nFaces, vector<int> &plyTriangles);
    // Rescales the model to fit a box of 1x1x1 with its base centered at the origin
    static void rescaleModel(vector<float> &plyVertices);
    static void addModelToMesh(const vector<float> &plyVertices, const vector<float> &plyColors, const vector<int> &plyTriangles, TriangleMesh &mesh);
    // Lab 2 & Lab5
    static void clusterVertices(const vector<float> &inVertices, const vector<float> &inColors, const vector<int> &inTriangles,
                                float cellSize, ClusteringMode mode,
                                vector<float> &outVertices, vector<float> &outColors, vector<int> &outTriangles);
    // Lab 2
    static bool saveMeshToPLY(const string &filename, const vector<float> &vertices, const vector<float> &colors, const vector<int> &triangles);
    // Lab 2 & Lab 5
    // Helper function to generate the LOD filename (e.g., "bunny.ply" -> "bunny_LOD1.ply")
    static string getLODFilename(const string &originalPath, int lodLevel, ClusteringMode mode);
};

#endif // PLYREADER_H
