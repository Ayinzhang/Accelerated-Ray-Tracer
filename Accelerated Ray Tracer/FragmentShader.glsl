#version 330 core

in vec2 screenCoord;

out vec4 FragColor;

struct Ray { vec3 origin, direction;};

struct Camera { vec3 position, forward, right, up;};

struct Triangle { vec3 v0, v1, v2, n;};

uniform Camera camera;
uniform int triangleCount;
layout(std140) uniform TriangleBlock { Triangle triangles[1000];};

Ray CreateRay(vec3 o, vec3 d) 
{
    Ray ray;
    ray.origin = o;
    ray.direction = normalize(d);
    return ray;
}

bool RayTriangleIntersect(Ray ray, Triangle tri, out float t, out vec3 hitPoint) 
{
    vec3 edge1 = tri.v1 - tri.v0, edge2 = tri.v2 - tri.v0, h = cross(ray.direction, edge2);
    float a = dot(edge1, h);

    if (abs(a) < 1e-6) return false;

    float f = 1.0 / a;
    vec3 s = ray.origin - tri.v0; 
    float u = f * dot(s, h);
    if (u < 0.0 || u > 1.0) return false;

    vec3 q = cross(s, edge1);
    float v = f * dot(ray.direction, q);
    if (v < 0.0 || u + v > 1.0) return false;

    t = f * dot(edge2, q);
    if (t > 1e-6) 
    {
        hitPoint = ray.origin + t * ray.direction;
        return true;
    }

    return false;
}

vec3 RayTrace(Ray ray) 
{
    float closestT = 1e20, t = (ray.direction.y + 1.0) * 0.5;
    vec3 color = (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);

    for (int i = 0; i < triangleCount; i++) 
    {
        float t; vec3 hitPoint; 
        if (RayTriangleIntersect(ray, triangles[i], t, hitPoint)) 
        {
            if (t < closestT) 
            {
                closestT = t;
                vec3 lightPos = vec3(10.0, 10.0, 10.0);
                vec3 lightDir = normalize(lightPos - hitPoint);
                float diff = max(dot(triangles[i].n, lightDir), 0.0);
                color = vec3(0.8) * diff;
            }
        }
    }
    
    return color;
}

void main() 
{
    float u = screenCoord.x, v = screenCoord.y;

    Ray ray = CreateRay(camera.position, camera.forward + 4 * (u - 0.5) * camera.right + 3 * (v - 0.5) * camera.up);

    FragColor = vec4(RayTrace(ray), 1.0); 
}