#pragma once
#include"ShadowSceneRenderer.h"

int irand(int a, int b);

enum SplitDimension {
	X = 0, Y, Z
};

class kdnode {
public:
	std::vector<Triangle*>* triangles;
	int dimension;
	float plane;

	kdnode* children[2];

	~kdnode();
};

class kdtree
{
public:
	std::vector<Triangle*>* RenderingTriangles;
	kdnode* root;

public:
	kdtree(std::vector<Triangle*>* RenderingTriangles);
	kdtree();
	~kdtree();

	kdnode* createTree(std::vector<Triangle*>& triangles, int depth);

	void rayCast(kdnode * node, DirectX::XMVECTOR startpoint, DirectX::XMVECTOR direction, float tmax, float tdiff_to_original);
	
	void rayCastAll(kdnode* node, DirectX::XMVECTOR startpoint, DirectX::XMVECTOR direction);

	void printTree(kdnode* node, int depth);

	float intersectRayTriangle(Triangle & tri, DirectX::XMVECTOR & rayOrigin, DirectX::XMVECTOR & rayDirection);

	bool PointInTriangle(DirectX::XMVECTOR & triV1, DirectX::XMVECTOR & triV2, DirectX::XMVECTOR & triV3, DirectX::XMVECTOR & point);

	void checkVectorTreeConsistency();

	kdnode* getRoot();

	void prepareKDTreeForDrawing(kdnode * node, float lx, float ux, float ly, float uy, float lz, float uz);

	float m_closestDist = FLT_MAX;
	int m_counter = 0;

	DirectX::XMVECTOR m_intersectionPoint;

	std::vector<BasicVertex> vertexKD;
};

