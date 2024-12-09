#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
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

class Shader
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
	vec3 v0, v1, v2, n;

	Triangle(vec3 _v0, vec3 _v1, vec3 _v2): v0(_v0), v1(_v1), v2(_v2), n(normalize(cross(_v1 - _v0, _v2 - _v0))) {}

	Triangle(vec3 _v0, vec3 _v1, vec3 _v2, vec3 _n) : v0(_v0), v1(_v1), v2(_v2), n(normalize(_n)) {}

	float Intersect(Ray ray)
	{
		if (dot(ray.direction, n) >= 0) return FLT_MAX;

		float t = dot(ray.origin - v0, n);
		if (t < 0) return FLT_MAX;

		vec3 hitPoint = ray.origin + t * ray.direction;

		vec3 edge0 = v1 - v0, edge1 = v2 - v1, edge2 = v0 - v2,
			 c0 = hitPoint - v0, c1 = hitPoint - v1, c2 = hitPoint - v2;

		if (dot(n, cross(edge0, c0)) < 0 || dot(n, cross(edge1, c1)) < 0 
			|| dot(n, cross(edge2, c2)) < 0) return FLT_MAX;

		return t;
	}
};

class Model 
{
public:
	vector<Triangle> triangles;
	class AABB 
	{
	public:
		vec3 min, max;

		AABB() : min(vec3(FLT_MAX)), max(vec3(-FLT_MAX)) {}

		void Update(vec3 vertex) 
		{
			min = MinVec3(min, vertex);
			max = MaxVec3(max, vertex);
		}

		bool Intersect(Ray ray) 
		{
			float tmin = (min.x - ray.origin.x) / ray.direction.x;
			float tmax = (max.x - ray.origin.x) / ray.direction.x;

			if (tmin > tmax) swap(tmin, tmax);

			float tymin = (min.y - ray.origin.y) / ray.direction.y;
			float tymax = (max.y - ray.origin.y) / ray.direction.y;

			if (tymin > tymax) swap(tymin, tymax);

			if ((tmin > tymax) || (tymin > tmax)) return false;

			if (tymin > tmin) tmin = tymin;
			if (tymax < tmax) tmax = tymax;

			float tzmin = (min.z - ray.origin.z) / ray.direction.z;
			float tzmax = (max.z - ray.origin.z) / ray.direction.z;

			if (tzmin > tzmax) swap(tzmin, tzmax);

			if ((tmin > tzmax) || (tzmin > tmax)) return false;

			return true;
		}
	};

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
};

class Camera
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