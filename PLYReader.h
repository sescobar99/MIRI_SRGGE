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
    static bool readMesh(const string &filename, TriangleMesh &mesh, int lodLevel = 0);

private:
    static bool loadHeader(ifstream &fin, int &nVertices, int &nFaces, bool &hasColors, bool &hasAlpha);
    static void loadVertices(ifstream &fin, int nVertices, vector<float> &plyVertices, vector<float> &plyColors, bool hasColors, bool hasAlpha);
    static void loadFaces(ifstream &fin, int nFaces, vector<int> &plyTriangles);
    static void rescaleModel(vector<float> &plyVertices);
    static void addModelToMesh(const vector<float> &plyVertices, const vector<float> &plyColors, const vector<int> &plyTriangles, TriangleMesh &mesh);
    // Lab 2
    static void clusterVertices(const vector<float> &inVertices, const vector<float> &inColors, const vector<int> &inTriangles,
                                float cellSize,
                                vector<float> &outVertices, vector<float> &outColors, vector<int> &outTriangles);
    // Lab 2
    static bool saveMeshToPLY(const string &filename, const vector<float> &vertices, const vector<float> &colors, const vector<int> &triangles);
    // Lab 2
    static string getLODFilename(const string &originalPath, int lodLevel);
};

#endif // PLYREADER_H
