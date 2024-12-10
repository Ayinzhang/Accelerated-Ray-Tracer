#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <queue>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <GLFW/glfw3.h>
#define uint unsigned int 
using namespace std;
using namespace glm;

float screenVertices[] = 
{
	-1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
	 1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
	 1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
	-1.0f,  1.0f, 0.0f,  0.0f, 1.0f 
};
uint screenIndices[] = {
	0, 1, 2,
	2, 3, 0
};

vec3 MinVec3(const vec3& a, const vec3& b)
{
	return vec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
}

vec3 MaxVec3(const vec3& a, const vec3& b) 
{
	return vec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
}

struct Shader
{
public:
	string vertexString, fragmentString;
	const char* vertexSource; const char* fragmentSource;
	uint ID, vertex, fragment;
	Shader(const char* vertexPath, const char* fragmentPath)
	{
		ifstream vertexFile, fragmentFile;
		vertexFile.open(vertexPath), fragmentFile.open(fragmentPath);
		stringstream vertexStream, fragmentStream;
		vertexStream << vertexFile.rdbuf(), fragmentStream << fragmentFile.rdbuf();
		vertexString = vertexStream.str(), fragmentString = fragmentStream.str();
		vertexSource = vertexString.c_str(); fragmentSource = fragmentString.c_str();
		glewExperimental = GL_TRUE; glewInit();
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vertexSource, NULL);
		glCompileShader(vertex);
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fragmentSource, NULL);
		glCompileShader(fragment);
		ID = glCreateProgram();
		glAttachShader(ID, vertex), glAttachShader(ID, fragment);
		glLinkProgram(ID);
		glDeleteShader(vertex), glDeleteShader(fragment);
	}
	void use() { glUseProgram(ID); }
	void SetUniformMat4(const char* name, mat4 mat)
	{
		glUniformMatrix4fv(glGetUniformLocation(ID, name), 1, GL_FALSE, value_ptr(mat));
	}
	void SetUniformVec2(const char* name, vec2 vector)
	{
		glUniform2f(glGetUniformLocation(ID, name), vector.x, vector.y);
	}
	void SetUniformVec3(const char* name, vec3 vector)
	{
		glUniform3f(glGetUniformLocation(ID, name), vector.x, vector.y, vector.z);
	}
	void SetUniform1f(const char* name, float f)
	{
		glUniform1f(glGetUniformLocation(ID, name), f);
	}
	void SetUniform1i(const char* name, int slot)
	{
		glUniform1i(glGetUniformLocation(ID, name), slot);
	}
};

struct Ray 
{
	vec3 origin, direction;
	Ray(vec3 o, vec3 d) : origin(o), direction(normalize(d)) {}
};

struct Triangle
{
	//vec3 v0, v1, v2, n;
	vec3 v0; float pad0;
	vec3 v1; float pad1;
	vec3 v2; float pad2;
	vec3 n; float pad3;

	Triangle(vec3 _v0, vec3 _v1, vec3 _v2): v0(_v0), v1(_v1), v2(_v2), n(normalize(cross(_v1 - _v0, _v2 - _v0))) {}

	Triangle(vec3 _v0, vec3 _v1, vec3 _v2, vec3 _n) : v0(_v0), v1(_v1), v2(_v2), n(normalize(_n)) {}

	void GetAABB(vec3& minCorner, vec3& maxCorner) 
	{
		minCorner = min(v0, min(v1, v2));
		maxCorner = max(v0, max(v1, v2));
	}
};

struct AABB 
{
	vec3 min, max;

	AABB() : min(vec3(FLT_MAX)), max(vec3(-FLT_MAX)) {}
	AABB(vec3 a, vec3 b) : min(a), max(b) {}

	// 扩展包围盒
	void Expand(const AABB& other) 
	{
		min = glm::min(min, other.min);
		max = glm::max(max, other.max);
	}
};

struct BVHNode 
{
	AABB box;
	BVHNode *left, *right; 
	int n, index;     

	BVHNode() : left(nullptr), right(nullptr), n(0), index(0) {}
};

struct FlattenedBVHNode
{
	int left, count, pad0, pad1;
	vec3 aabbMin; float pad2; vec3 aabbMax; float pad3;
};

struct Model 
{
	vector<Triangle> triangles;

	bool LoadModel(const string& filepath) 
	{
		ifstream file(filepath);
		string line;
		vector<vec3> vertices, normals;
		if (!file.is_open()) return false;

		while (getline(file, line)) 
		{
			istringstream s(line);
			string type, s1, s2, s3;
			float x, y, z;
			int v0, v1, v2, vn0, vn1, vn2;

			s >> type;
			if (type == "v") 
			{
				s >> x >> y >> z;
				vertices.push_back(vec3(x, y, z));
			}
			//else if (type == "vn") 
			//{
			//	s >> x >> y >> z;
			//	normals.push_back(vec3(x, y, z));
			//}
			else if (type == "f") 
			{
				s >> s1 >> s2 >> s3;

				v0 = stoi(s1.substr(0, s1.find('/'))) - 1;
				v1 = stoi(s2.substr(0, s2.find('/'))) - 1;
				v2 = stoi(s3.substr(0, s3.find('/'))) - 1;

				//vn0 = stoi(s1.substr(s1.find("//") + 2)) - 1;
				//vn1 = stoi(s2.substr(s2.find("//") + 2)) - 1;
				//vn2 = stoi(s3.substr(s3.find("//") + 2)) - 1;

				// 创建三角形
				triangles.emplace_back(
					vertices[v0], vertices[v1], vertices[v2]
					//,normals[vn0] + normals[vn1] + normals[vn2] 
				);
			}
		}

		file.close();
		return true;
	}

	void DrawWireframe()
	{
		glBegin(GL_LINES);
		for (const auto& triangle : triangles) 
		{
			glVertex3fv(value_ptr(triangle.v0));
			glVertex3fv(value_ptr(triangle.v1));

			glVertex3fv(value_ptr(triangle.v1));
			glVertex3fv(value_ptr(triangle.v2));

			glVertex3fv(value_ptr(triangle.v2));
			glVertex3fv(value_ptr(triangle.v0));
		}
		glEnd();
	}

	BVHNode* BuildBVH(int start, int end) 
	{
		BVHNode* node = new BVHNode(); AABB box;
		for (int i = start; i < end; i++) 
		{
			vec3 minCorner, maxCorner;
			triangles[i].GetAABB(minCorner, maxCorner);
			box.Expand(AABB(minCorner, maxCorner));
		}
		node->box = box;

		int count = end - start;
		if (count <= 4) 
		{ 
			node->n = count;
			node->index = start;
			return node;
		}

		vec3 extent = box.max - box.min;
		int axis = extent.x > extent.y ? (extent.x > extent.z ? 0 : 2) : (extent.y > extent.z ? 1 : 2);

		sort(triangles.begin() + start, triangles.begin() + end,
			[axis](const Triangle& a, const Triangle& b) {
				return (a.v0[axis] + a.v1[axis] + a.v2[axis]) / 3 <
					(b.v0[axis] + b.v1[axis] + b.v2[axis]) / 3;
			});

		int mid = start + count / 2;
		node->left = BuildBVH(start, mid);
		node->right = BuildBVH(mid, end);
		return node;
	}
};

void SerializeBVH(vector<FlattenedBVHNode>& flattenedBVH, BVHNode* root) {
	if (!root) return;

	queue<pair<BVHNode*, int>> q; // 队列，保存节点和对应的索引
	q.push({ root, 0 });

	while (!q.empty()) 
	{
		auto node = q.front().first;
		auto index = q.front().second;
		q.pop();

		// 序列化当前节点
		FlattenedBVHNode flatNode;
		flatNode.aabbMin = node->box.min;
		flatNode.aabbMax = node->box.max;

		if (node->n > 0) 
		{ // 叶节点
			flatNode.left = node->index; // 存储三角形起始索引
			flatNode.count = node->n;        // 存储三角形数量
		}
		else 
		{ // 内部节点
			flatNode.left = 2 * index + 1; // 左子节点索引
			flatNode.count = 0;                // 内部节点标记为 0

			// 添加左右子节点到队列
			if (node->left) q.push({ node->left, 2 * index + 1 });
			if (node->right) q.push({ node->right, 2 * index + 2 });
		}

		flattenedBVH.push_back(flatNode); // 保存当前节点
	}
}


struct Camera
{
public:
	vec3 position, forward, right, up, worldUp; float yaw, pitch;
	Camera(vec3 _position, vec3 target, vec3 worldup)
	{
		position = _position; worldUp = worldup;
		forward = normalize(target - position);
		right = normalize(cross(forward, worldUp));
		up = -normalize(cross(forward, right));
	}
	Camera(vec3 _position, float pitch, float yaw, vec3 worldup)
	{
		position = _position; worldUp = worldup; pitch = pitch; yaw = yaw;
		forward = vec3(cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw));
		right = normalize(cross(forward, worldUp));
		up = -normalize(cross(forward, right));
	}
	mat4 GetViewMatrix()
	{
		return lookAt(position, forward + position, worldUp);
	}
	void ProcessMouseMovement(float daltax, float daltay)
	{
		yaw -= 0.01 * daltax; pitch -= 0.01 * daltay;
		UpdateCameraVectors();
	}
	void UpdateCameraVectors()
	{
		forward = vec3(cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw));
		right = normalize(cross(forward, worldUp));
		up = -normalize(cross(forward, right));
	}
};