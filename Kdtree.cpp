#include "pch.h"
#include "Kdtree.h"
#include <string>

using namespace DirectX;

kdtree::kdtree(std::vector<Triangle*>* RenderingTriangles) {
	this->RenderingTriangles = RenderingTriangles;
	root = createTree(*RenderingTriangles, 32);
}

kdtree::kdtree() {
}

kdtree::~kdtree() {
	if (this->root != 0) delete this->root;
}

kdnode::~kdnode() {
	if (this->children[0] != 0) delete this->children[0];
	if (this->children[1] != 0) delete this->children[1];
	if (this->triangles != 0) delete this->triangles;
}

int irand(int a, int b)
{
	double r = b - a + 1;
	return a + (int)(r * rand() / (RAND_MAX + 1.0));
}

inline void vector_int_swap(std::vector<Triangle*>& numbers, int a, int b) {
 	Triangle* tmp = numbers[a];
	numbers[a] = numbers[b];
	numbers[b] = tmp;
}

int explicit_partition(std::vector<Triangle*>& numbers, int first, int last, int pivot, int splitdirection) {
	Triangle* pivotValue = numbers[pivot];
	vector_int_swap(numbers, pivot, last);
	int storeIndex = first;
	for (int i = first; i < last; ++i) {
		if (numbers[i]->center[splitdirection] < pivotValue->center[splitdirection]) {
			vector_int_swap(numbers, storeIndex, i);
			++storeIndex;
		}
	}
	vector_int_swap(numbers, last, storeIndex);
	return storeIndex;
}

int randomized_partition(std::vector<Triangle*>& numbers, int first, int last, int splitdirection) {
	// select random element (pivot), throw all smaller left and all bigger right, return index of pivot
	int pivot = irand(first, last);
	//int pivot = rand() % (last - first) + first;
	return explicit_partition(numbers, first, last, pivot, splitdirection);
}

int randomized_median_select(std::vector<Triangle*>& numbers, int first, int last, int xthSmallest, int splitdirection) {
	if (first == last) return first;
	int pivot = randomized_partition(numbers, first, last, splitdirection);
	int count = (pivot - first) + 1;
	if (xthSmallest == count) return pivot;
	else if (xthSmallest < count) return randomized_median_select(numbers, first, pivot - 1, xthSmallest, splitdirection);
	else return randomized_median_select(numbers, pivot + 1, last, xthSmallest - count, splitdirection);
}

int get_median_via_median_select(std::vector<Triangle*>& numbers, int splitdirection, int start, int end) {
	return randomized_median_select(numbers, start, end, (end - start + 1) / 2, splitdirection);
}


bool SortTrianglesByXPredicate(const Triangle* t1, const Triangle* t2)
{
	return t1->center[0] < t2->center[0];
}
bool SortTrianglesByYPredicate(const Triangle* t1, const Triangle* t2)
{
	return t1->center[1] < t2->center[1];
}
bool SortTrianglesByZPredicate(const Triangle* t1, const Triangle* t2)
{
	return t1->center[2] < t2->center[2];
}

void kdtree::printTree(kdnode* node, int depth) {
	if (node == 0) return;

	if (node->dimension == -1) { // leaf node
		OutputDebugStringA("l");
		for (unsigned int i = 0; i < depth; i++) OutputDebugStringA("|");
		std::string out = "+ " + std::to_string(node->triangles->size()) + " triangles \n";
		OutputDebugStringA(out.c_str());
		m_counter += node->triangles->size();
	}
	else {
		std::string out = std::to_string(node->dimension) + "X";
		OutputDebugStringA(out.c_str());

		for (unsigned int i = 0; i < depth; i++) OutputDebugStringA("|");

		out = "+" + std::to_string(node->plane) + " \n";
		OutputDebugStringA(out.c_str());

		printTree(node->children[0], depth + 1);
		printTree(node->children[1], depth + 1);
	}
}

float kdtree::intersectRayTriangle(Triangle& tri, XMVECTOR& rayOrigin, XMVECTOR& rayDirection)
{
	//Triangle's vertices V1, V2, V3
	XMVECTOR tri1V1 = tri.tri1;
	XMVECTOR tri1V2 = tri.tri2;
	XMVECTOR tri1V3 = tri.tri3;

	//Find the normal using U, V coordinates (two edges)
	XMVECTOR U = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR V = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR faceNormal = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

	U = tri1V2 - tri1V1;
	V = tri1V3 - tri1V1;

	//Compute face normal by crossing U, V
	faceNormal = XMVector3Cross(U, V);

	faceNormal = XMVector3Normalize(faceNormal);

	//Calculate a point on the triangle for the plane equation
	XMVECTOR triPoint = tri1V1;

	//Get plane equation ("Ax + By + Cz + D = 0") Variables
	float tri1A = XMVectorGetX(faceNormal);
	float tri1B = XMVectorGetY(faceNormal);
	float tri1C = XMVectorGetZ(faceNormal);
	float tri1D = (-tri1A*XMVectorGetX(triPoint) - tri1B*XMVectorGetY(triPoint) - tri1C*XMVectorGetZ(triPoint));

	//Now we find where (on the ray) the ray intersects with the triangles plane
	float ep1, ep2, t = 0.0f;
	float planeIntersectX, planeIntersectY, planeIntersectZ = 0.0f;
	XMVECTOR pointInPlane = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

	ep1 = (XMVectorGetX(rayOrigin) * tri1A) + (XMVectorGetY(rayOrigin) * tri1B) + (XMVectorGetZ(rayOrigin) * tri1C);
	ep2 = (XMVectorGetX(rayDirection) * tri1A) + (XMVectorGetY(rayDirection) * tri1B) + (XMVectorGetZ(rayDirection) * tri1C);

	//Make sure there are no divide-by-zeros
	if (ep2 != 0.0f)
		t = -(ep1 + tri1D) / (ep2);

	if (t > 0.0f)    //Make sure you don't pick objects behind the camera
	{
		//Get the point on the plane
		planeIntersectX = XMVectorGetX(rayOrigin) + XMVectorGetX(rayDirection) * t;
		planeIntersectY = XMVectorGetY(rayOrigin) + XMVectorGetY(rayDirection) * t;
		planeIntersectZ = XMVectorGetZ(rayOrigin) + XMVectorGetZ(rayDirection) * t;

		pointInPlane = XMVectorSet(planeIntersectX, planeIntersectY, planeIntersectZ, 0.0f);

		//Call function to check if point is in the triangle
		if (PointInTriangle(tri1V1, tri1V2, tri1V3, pointInPlane))
		{
			//Return the distance to the hit, to check all the other pickable objects
			//and choose whichever object is closest to the camera
			m_intersectionPoint = pointInPlane;
			return t / 2.0f;
		}
	}

	return FLT_MAX;
}

bool kdtree::PointInTriangle(XMVECTOR& triV1, XMVECTOR& triV2, XMVECTOR& triV3, XMVECTOR& point)
{
	//To find out if the point is inside the triangle, we will check to see if the point
	//is on the correct side of each of the triangles edges.

	XMVECTOR cp1 = XMVector3Cross((triV3 - triV2), (point - triV2));
	XMVECTOR cp2 = XMVector3Cross((triV3 - triV2), (triV1 - triV2));
	if (XMVectorGetX(XMVector3Dot(cp1, cp2)) >= 0)
	{
		cp1 = XMVector3Cross((triV3 - triV1), (point - triV1));
		cp2 = XMVector3Cross((triV3 - triV1), (triV2 - triV1));
		if (XMVectorGetX(XMVector3Dot(cp1, cp2)) >= 0)
		{
			cp1 = XMVector3Cross((triV2 - triV1), (point - triV1));
			cp2 = XMVector3Cross((triV2 - triV1), (triV3 - triV1));
			if (XMVectorGetX(XMVector3Dot(cp1, cp2)) >= 0)
			{
				return true;
			}
			else
				return false;
		}
		else
			return false;
	}
	return false;
}

void kdtree::rayCastAll(kdnode* node, XMVECTOR startpoint, XMVECTOR direction) {

	float closestDist = FLT_MAX;

	for (int i = 0; i < RenderingTriangles->size(); ++i) {
		float intersectdst = intersectRayTriangle(*(*RenderingTriangles)[i], startpoint, direction);
		if (intersectdst < closestDist) {
			(*RenderingTriangles)[i]->intersected = true;
			(*RenderingTriangles)[i]->intersectionPoint = m_intersectionPoint;
			closestDist = intersectdst;
			(*RenderingTriangles)[i]->distance = closestDist;
		}
		else {
			(*RenderingTriangles)[i]->intersected = false;
		}
	}
}

// startpoint = origin of ray
// direction = normalized direction of ray
// tmax = length of ray (ideally == far plane)
void kdtree::rayCast(kdnode* node, XMVECTOR startpoint, XMVECTOR direction, float tmax, float tdiff_to_original)
{
	if (node == 0) return;

	if (node->dimension == -1) { // leaf node
								 // ray intersection here
		for (int i = 0; i < node->triangles->size(); ++i) {
			if (!(*node->triangles)[i]->intersected) {
				float intersectdst = intersectRayTriangle(*(*node->triangles)[i], startpoint, direction);
				if (intersectdst < FLT_MAX) {
					(*node->triangles)[i]->intersected = true;
					(*node->triangles)[i]->distance = intersectdst + tdiff_to_original;
					(*node->triangles)[i]->intersectionPoint = m_intersectionPoint;
				}
			}
		}
	}
	else {
		// which child to recurse first into? (0=near,1=far)
		int dim = node->dimension;
		int first = (startpoint.m128_f32[dim]) > node->plane;

		if (direction.m128_f32[dim] == 0.0f) {
			// line segment parallel to splitting plane, visit near side only
			rayCast(node->children[first], startpoint, direction, tmax, tdiff_to_original);
		}
		else {
			//find t value for intersection
			float t = (node->plane - (startpoint.m128_f32[dim])) / (direction.m128_f32[dim]);

			if (0.0f <= t && t < tmax) {
				rayCast(node->children[first], startpoint, direction, t, tdiff_to_original);
				rayCast(node->children[first ^ 1], startpoint + t * direction, direction, tmax - t, tdiff_to_original + t);
			}
			else {
				rayCast(node->children[first], startpoint, direction, tmax, tdiff_to_original);
			}
		}
	}
}

void insertTreeIntoVector(kdnode* node, std::vector<Triangle*>* v) {
	if (node == 0) return;

	if (node->dimension == -1) {
		for (unsigned int i = 0; i < node->triangles->size(); ++i) {
			v->push_back((*node->triangles)[i]);
		}
	}
	else {
		insertTreeIntoVector(node->children[0], v);
		insertTreeIntoVector(node->children[1], v);
	}
}

void kdtree::checkVectorTreeConsistency() {
	std::vector<Triangle*> v(0);
	std::vector<Triangle*> t(0);

	v.reserve(RenderingTriangles->size());
	t.reserve(RenderingTriangles->size());

	for (int i = 0; i < RenderingTriangles->size(); ++i) {
		v.push_back((*RenderingTriangles)[i]);
	}
	std::sort(v.begin(), v.end());

	insertTreeIntoVector(root, &t);
	std::sort(t.begin(), t.end());
	std::vector<Triangle*>::iterator newend = std::unique(t.begin(), t.end());
	t.erase(newend, t.end());

	assert(v.size() == t.size());
	for (int i = 0; i < v.size(); ++i) {
		assert(v[i] == t[i]);
	}

	return;
}

kdnode* kdtree::createTree(std::vector<Triangle*>& triangles, int depth) {
	if (triangles.empty()) return 0;

	if (depth == 0 || triangles.size() <= 5) {
		kdnode* node = new kdnode();
		node->dimension = -1;

		node->triangles = new std::vector<Triangle*>(0);
		node->triangles->reserve(triangles.size());
		for (int i = 0; i < triangles.size(); ++i) {
			node->triangles->push_back(triangles[i]);
		}

		node->children[0] = 0;
		node->children[1] = 0;
		node->plane = 0;
		return node;
	}

	int splitdir;

	// --- find biggest expansion ---
	float smallest[3] = { FLT_MAX, FLT_MAX, FLT_MAX };
	float biggest[3] = { FLT_MIN, FLT_MIN, FLT_MIN };
	
	for (int i = 0; i < triangles.size(); ++i) {
		for (int dim = 0; dim < 3; ++dim) {
			if (triangles[i]->lower_corner[dim] < smallest[dim]) {
				smallest[dim] = triangles[i]->lower_corner[dim];
			}
			if (triangles[i]->upper_corner[dim] > biggest[dim]) {
				biggest[dim] = triangles[i]->upper_corner[dim];
			}
		}
	}
	/*
	// for kd-tree drawing
	float3 vert[8];

	vert[0] = { smallest[0], biggest[1], smallest[2] };
	vert[1] = { biggest[0], biggest[1], smallest[2] };
	vert[2] = { biggest[0], smallest[1], smallest[2] };
	vert[3] = { smallest[0], smallest[1], smallest[2] };
	vert[4] = { smallest[0], biggest[1], biggest[2] };
	vert[5] = { biggest[0], biggest[1], biggest[2] };
	vert[6] = { biggest[0], smallest[1], biggest[2] };
	vert[7] = { smallest[0], smallest[1], biggest[2] };

	for (int i = 0; i < 8; ++i)
	{
		vertexKD.push_back(vert[i]);
	}
	// ----------------------*/

	float expansion[3];
	for (int dim = 0; dim < 3; ++dim) {
		expansion[dim] = biggest[dim] - smallest[dim];
	}
	if (expansion[0] > expansion[1]) {
		if (expansion[0] > expansion[2]) {
			splitdir = 0;
		}
		else { // 0 <= 2
			splitdir = 2;
		}
	}
	else { // 0 <= 1
		if (expansion[1] > expansion[2]) {
			splitdir = 1;
		}
		else { // 1 <= 2
			splitdir = 2;
		}
	}

	// --- find biggest expansion end ---
	
	int medianindex = get_median_via_median_select(triangles, splitdir, 0, triangles.size() - 1);

	kdnode* node = new kdnode();
	node->dimension = splitdir;
	node->triangles = 0;
	Triangle* mediantri = triangles[medianindex];

	// put split plane on the side fewer units away from the center of the triangle
	float left_to_center = abs(mediantri->center[splitdir] - mediantri->lower_corner[splitdir]);
	float center_to_right = abs(mediantri->upper_corner[splitdir] - mediantri->center[splitdir]);
	if (left_to_center > center_to_right) {
		node->plane = mediantri->upper_corner[splitdir] - 0.001f;
	}
	else {
		node->plane = mediantri->lower_corner[splitdir] + 0.001f;
	}

	std::vector<Triangle*> left(0);
	left.reserve(triangles.size());
	std::vector<Triangle*> right(0);
	right.reserve(triangles.size());

	// insert all left of split plane to the left
	for (int i = 0; i < triangles.size(); ++i) {
		if (triangles[i]->lower_corner[splitdir] <= node->plane) {
			left.push_back(triangles[i]);
		}
	}
	// insert all right of split plane to the right
	for (int i = 0; i < triangles.size(); ++i) {
		if (triangles[i]->upper_corner[splitdir] > node->plane) { // >= ?
			right.push_back(triangles[i]);
		}
	}

	// ---- create tree recursively on child nodes
	// if one side contains all triangles, we can't split it again using the same algo, as that would give us the same result again
	// so in that case, force the next node to be a leaf node by setting depth to 0
	if (left.size() == triangles.size()) {
		// if the other side ALSO contains all triangles, it wouldn't make sense to create two child nodes that contain the same triangles
		// so in that case, transform the current node into a leaf node and return it
		if (right.size() == triangles.size()) {
			node->dimension = -1;

			node->triangles = new std::vector<Triangle*>(0);
			node->triangles->reserve(triangles.size());
			for (int i = 0; i < triangles.size(); ++i) {
				node->triangles->push_back(triangles[i]);
			}

			node->children[0] = 0;
			node->children[1] = 0;
			node->plane = 0;
			return node;
		}
		else {
			node->children[0] = createTree(left, 0);
		}
	}
	else {
		node->children[0] = createTree(left, depth - 1);
	}

	// see comment for left, same applies to right
	if (right.size() == triangles.size()) {
		node->children[1] = createTree(right, 0);
	}
	else {
		node->children[1] = createTree(right, depth - 1);
	}

	return node;
}

kdnode* kdtree::getRoot() {
	return root;
}

void kdtree::prepareKDTreeForDrawing(kdnode * node, float lx, float ux, float ly, float uy, float lz, float uz)
{
	if (node == 0) return;
	
	BasicVertex vert[5];

	switch (node->dimension) {
	case -1: // leaf node, return
		return;
	case 0: // x direction
		vert[0].pos = {	node->plane, uy, uz	};
		vert[1].pos = { node->plane, ly, uz };
		vert[2].pos = { node->plane, ly, lz };
		vert[3].pos = { node->plane, uy, lz };
		vert[4].pos = { node->plane, uy, uz };
		vertexKD.push_back(vert[0]);
		vertexKD.push_back(vert[1]);
		vertexKD.push_back(vert[2]);
		vertexKD.push_back(vert[3]);
		vertexKD.push_back(vert[4]);
		prepareKDTreeForDrawing(node->children[0], lx, node->plane, ly, uy, lz, uz);
		prepareKDTreeForDrawing(node->children[1], node->plane, ux, ly, uy, lz, uz);
		break;
	case 1: // y direction
		vert[0].pos = { ux, node->plane, uz };
		vert[1].pos = { lx, node->plane, uz };
		vert[2].pos = { lx, node->plane, uz };
		vert[3].pos = { ux, node->plane, uz };
		vert[4].pos = { ux, node->plane, uz };
		vertexKD.push_back(vert[0]);
		vertexKD.push_back(vert[1]);
		vertexKD.push_back(vert[2]);
		vertexKD.push_back(vert[3]);
		vertexKD.push_back(vert[4]);
		prepareKDTreeForDrawing(node->children[0], lx, ux, ly, node->plane, lz, uz);
		prepareKDTreeForDrawing(node->children[1], lx, ux, node->plane, uy, lz, uz);
		break;
	case 2: // z direction
		vert[0].pos = { ux, uy, node->plane };
		vert[1].pos = { ux, uy, node->plane };
		vert[2].pos = { ux, ly, node->plane };
		vert[3].pos = { ux, ly, node->plane };
		vert[4].pos = { ux, uy, node->plane };
		vertexKD.push_back(vert[0]);
		vertexKD.push_back(vert[1]);
		vertexKD.push_back(vert[2]);
		vertexKD.push_back(vert[3]);
		vertexKD.push_back(vert[4]);
		prepareKDTreeForDrawing(node->children[0], lx, ux, ly, uy, lz, node->plane);
		prepareKDTreeForDrawing(node->children[1], lx, ux, ly, uy, node->plane, uz);
		break;
	}
}