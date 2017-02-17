#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
#include <typeinfo>

using namespace std;

#if defined __linux__ || defined __APPLE__
// "Compiled for Linux
#else
// Windows doesn't define these values by default, Linux does
#define M_PI 3.141592653589793
#define INFINITY 1e8
#endif

#define K_EPSILON 0.00001
#define MAX_RAY_DEPTH 5

#define USE_DISTRIBUTED_RAYS true

class Vector3 {
  public:
    float x, y, z;

    Vector3() : x(0.0), y(0.0), z(0.0) {}
    Vector3(float xx) : x(xx), y(xx), z(xx) {}
    Vector3(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {}

    Vector3 operator - () const { return Vector3(-x, -y, -z); }
    Vector3 operator + (const float &f) const { return Vector3(x + f, y + f, z + f); }
    Vector3 operator - (const float &f) const { return Vector3(x - f, y - f, z - f); }
    Vector3 operator * (const float &f) const { return Vector3(x * f, y * f, z * f); }
    Vector3 operator / (const float &f) const { return Vector3(x / f, y / f, z / f); }
    Vector3 operator + (const Vector3 &v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
    Vector3 operator - (const Vector3 &v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
    Vector3 operator * (const Vector3 &v)  const { return Vector3(x * v.x, y * v.y, z * v.z); }
    Vector3& operator += (const Vector3 &v) { x += v.x, y += v.y, z += v.z; return *this; }
    Vector3& operator *= (const Vector3 &v) { x *= v.x, y *= v.y, z *= v.z; return *this; }

    float dot(const Vector3 &v) const { return x * v.x + y * v.y + z * v.z; }
    Vector3 cross(const Vector3 &v) const { return Vector3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); }
    float length2() const { return x * x + y * y + z * z; }
    float length() const { return sqrt(length2()); }

    Vector3& normalize() { 
      float norm2 = length2();
      if(norm2 > 0) {
        float invNorm = 1 / sqrt(norm2);
        x *= invNorm, y *= invNorm, z *= invNorm;
      }
      return *this;
    }

    Vector3& rotateX(const float &angle) {
      float _y = y; float _z = z;
      y = _y * cos(angle) - _z * sin(angle);
      z = _y * sin(angle) + _z * cos(angle);
      return *this;
    }

    Vector3& rotateY(const float &angle) {
      float _x = x; float _z = z;
      x = _z * sin(angle) + _x * cos(angle);
      z = _z * cos(angle) - _x * sin(angle);
      return *this;
    }

    Vector3& rotateZ(const float &angle) {
      float _x = x; float _y = y;
      x = _x * cos(angle) - y * sin(angle);
      y = _x * sin(angle) + y * cos(angle);
      return *this;
    }

    friend ostream & operator << (ostream &os, const Vector3 &v) {
      os << "[" << v.x << " " << v.y << " " << v.z << "]";
      return os;
    }

    static Vector3 random() {
      float rx = ((float) rand() / (RAND_MAX));
      float ry = ((float) rand() / (RAND_MAX));
      float rz = ((float) rand() / (RAND_MAX));
      return Vector3(rx, ry, rz);
    }

};

struct Color {
  float r, g, b;
  Color() : r(0), g(0), b(0) {}
  Color(float c) : r(c), g(c), b(c) {}
  Color(float _r, float _g, float _b) : r(_r), g(_g), b(_b) {}
  Color operator * (float f) const { return Color(r * f, g * f, b * f); }
  Color operator * (Vector3 f) const { return Color(r * f.x, g * f.y, b * f.z); }
  Color operator * (Color c) const { return Color(r * c.r, g * c.g, b * c.b); }
  Color operator + (Color c) const { return Color(r + c.r, g + c.g, b + c.b); } 
  Color& operator += (const Color &c) { r += c.r, g += c.g, b += c.b; return *this; }
  Color& operator *= (const Color &c) { r *= c.r, g *= c.g, b *= c.b; return *this; }

  Color& clamp() {
    if(r < 0) r = 0;
    if(g < 0) g = 0;
    if(b < 0) b = 0;
    if(r > 255) r = 255;
    if(g > 255) g = 255;
    if(b > 255) b = 255;
    return *this;
  }
};

float Lerp(float v1, float v2, float t) {
  return (1.0f - t) * v1 + t * v2;
}

class Ray {
  public:
    Vector3 origin;
    Vector3 direction;
    Ray(Vector3 _origin, Vector3 _direction) : origin(_origin), direction(_direction) {}
};

class Shape {
  public:
    Vector3 center;           // Position
    Color color;              // Surface Diffuse Color
    Color color_specular;     // Surface Specular Color
    float ka, kd, ks;         // Ambient, Diffuse, Specular Coefficents
    float shininess;
    float reflectivity;       // Reflectivity of material [0, 1]
    float transparency;       // Transparency of material [0, 1]
    float glossiness;         // Strength of glossy reflections
    float glossy_transparency; // Strength of glossy transparency

    virtual bool intersect(const Ray &ray, float &to, float &t1) { return false; }
    virtual Vector3 getNormal(const Vector3 &hitPoint) { return Vector3(); }
};

class Sphere : public Shape {
  public:
    float radius, radius2;

    Sphere(
      const Vector3 &_center, const float _radius, const Color &_color, 
      const float _ka, const float _kd, const float _ks, const float _shinny = 128.0, 
      const float _reflectScale = 1.0, const float _transparency = 0.0) :
      radius(_radius), radius2(_radius*_radius)
      { 
        center = _center;
        color = _color;
        color_specular = Color(255);
        ka = _ka;
        kd = _kd;
        ks = _ks;
        shininess = _shinny;
        reflectivity = _reflectScale;
        transparency = _transparency;
        glossiness = 0;
        glossy_transparency = 0;
      }
    
    // Compute a ray-sphere intersection using the geometric method
    bool intersect(const Ray &ray, float &t0, float &t1) {
      Vector3 l = center - ray.origin;
      float tca = l.dot(ray.direction); // Closest approach
      if (tca < 0) return false; // Ray intersection behind ray origin
      float d2 = l.dot(l) - tca*tca;
      if (d2 > radius2) return false; // Ray doesn't intersect
      float thc = sqrt(radius2 - d2); // Closest approach to surface of sphere
      t0 = tca - thc;
      t1 = tca + thc;
      return true;
    }

    // Computer a ray-sphere intersection using analytic method (w/ quadratic equation)
    bool intersect2(const Ray &ray, float &t) {
      Vector3 o = ray.origin;
      Vector3 d = ray.direction;
      Vector3 oc = o - center;
      float b = 2 * oc.dot(d);
      float c = oc.dot(oc) - radius2;
      float disc = b*b - 4*c;
      if(disc < 0) return false;
      else {
        disc = sqrt(disc);
        float t0 = -b - disc;
        float t1 = -b + disc;
        t = (t0 < t1) ? t0 : t1;
        return true;
      }
    }

    Vector3 getNormal(const Vector3 &hitPoint) {
      return (hitPoint - center) / radius;
    }
};

class Triangle : public Shape {
  public:
    Vector3 v0;
    Vector3 v1;
    Vector3 v2;

    Triangle(
      const Vector3 &_v0, const Vector3 &_v1, const Vector3 &_v2, const Color &_color, 
      const float _ka, const float _kd, const float _ks, const float _shinny = 128.0, 
      const float _reflectScale = 1.0, const float _transparency = 0.0) :
      v0(_v0), v1(_v1), v2(_v2)
      {
        color = _color;
        color_specular = Color(255);
        ka = _ka;
        kd = _kd;
        ks = _ks;
        shininess = _shinny;
        reflectivity = _reflectScale;
        transparency = _transparency;
        glossiness = 0;
        glossy_transparency = 0;
      }
    
    // Compute a ray-triangle intersection
    bool intersect(const Ray &ray, float &t, float &tnone) {

      /*
      Vector3 edge1, edge2;
      Vector3 P, Q, T;
      float det, inv_det, u, v;
      
      edge1 = v1-v0;
      edge2 = v2-v0;
      P = ray.direction.dot(edge2);
      det = edge1.dot(P);

      if(det < K_EPSILON && det > -K_EPSILON) return false;
      inv_det = 1.f / det;

      T = ray.origin - v0;

      u = T.dot(P) * inv_det;
      if(u < 0.f || u > 1.f) return false;

      Q = T.dot(edge1);

      v = ray.direction.dot(Q) * inv_det;
      if(v < 0.f || u + v > 1.f) return false;
      
      t = edge2.dot(Q) * inv_det;
      
      Vector3 phit1 = v0 + edge2 * u + edge1 * v;
      Vector3 phit2 = ray.origin + ray.direction * t;

      return true;
      */

      Vector3 N = getNormal(Vector3());

      // Check if ray and plane are parallel
      float NdotRd = N.dot(ray.direction);
      if(fabs(NdotRd) < K_EPSILON) {
        return false; // Ray and plane are parallel
      }
      
      // Compute d from the equation of a plane - ax + by + cz + d = 0, n = (a,b,c)
      float d = N.dot(v0);

      // Compute t
      float NdotRo = N.dot(ray.origin);
      t = - ((NdotRo + d) / NdotRd);
      if(t < 0) return false; // Triangle is behind ray

      // Compute intersection point with plane
      Vector3 hitPoint = ray.origin + ray.direction * t;

      // Check if intersection point is inside triangle
      Vector3 C; 

      Vector3 v01 = v1 - v0;
      Vector3 v12 = v2 - v1;
      Vector3 v20 = v0 - v2;
      
      Vector3 vp0 = hitPoint - v0;
      C = v01.cross(vp0);
      if (N.dot(C) < 0) return false;

      Vector3 vp1 = hitPoint - v1;
      C = v12.cross(vp1);
      if (N.dot(C) < 0) return false;

      Vector3 vp2 = hitPoint - v2;
      C = v20.cross(vp2);
      if (N.dot(C) < 0) return false;
      
      return true; // Triangle intersects ray
    }

    Vector3 getNormal(const Vector3 &hitPoint) {
      Vector3 v01 = v1 - v0;
      Vector3 v02 = v2 - v0;
      Vector3 N = v01.cross(v02);
      N.normalize(); 
      return N;
    }
};

class Light {
  public:
    Vector3 position;
    Vector3 intensity;
    unsigned char type;

    int samples;
    float width, height;

    Light() : position(Vector3()), intensity(Vector3()) { type = 0x01; }
    Light(Vector3 _intensity) : position(Vector3()), intensity(_intensity) { type = 0x01; } 
    Light(Vector3 _position, Vector3 _intensity) : position(_position), intensity(_intensity) { type = 0x01; } 
    virtual float attenuate(const float &r) const { return 1.0; }
};

class AmbientLight : public Light {
  public:
    AmbientLight() : Light() { type = 0x02; }
    AmbientLight(Vector3 _intensity) : Light(_intensity) { type = 0x02; }
    float attenuate(const float &r) const { return 1.0; }
};

class DirectionalLight : public Light {
  public:
    DirectionalLight() : Light() { type = 0x04; }
    DirectionalLight(Vector3 _position, Vector3 _intensity) : Light(_position, _intensity) { type = 0x04; }
    float attenuate(const float &r) const { return 1.0; }
};

class PointLight : public Light {
  public:
    PointLight() : Light() { type = 0x08; }
    PointLight(Vector3 _position, Vector3 _intensity) : Light(_position, _intensity) { type = 0x08; }
    float attenuate(const float &r) const { return 1.0 / (r * r); }
};

class SpotLight : public Light {
  public:
    SpotLight() : Light() { type = 0x10; }
    SpotLight(Vector3 _position, Vector3 _intensity) : Light(_position, _intensity) { type = 0x10; }
    float attenuate(Vector3 Vobj, Vector3 Vlight) const { return Vobj.dot(Vlight); }
};

class AreaLight : public Light {
  public:
    AreaLight() : Light() {
      type = 0x20;
      samples = 2;
      width = 4;
      height = 4;
    }
    AreaLight(Vector3 _position, Vector3 _intensity) : Light(_position, _intensity) {
      type = 0x20;
      samples = 2;
      width = 4;
      height = 4;
    }
    float attenuate(const float &r) const { return 1.0; }
};

class Scene {
  public:
    vector<Shape*> objects;
    vector<Light*> lights;
    AmbientLight ambientLight;
    Color backgroundColor;

    Scene() { backgroundColor = Color(); }
    void addAmbientLight(AmbientLight _light) { ambientLight = _light;}
    void addLight(Light *_light) { lights.push_back(_light); }
    void addObject(Shape *_object) { objects.push_back(_object); }
};

class Camera { 
  public:
    Vector3 position;
    int width, height;
    float invWidth, invHeight;
    float fov, aspectratio, angle;
    float angleX, angleY, angleZ;

    Camera(Vector3 _position, int _width, int _height, float _fov) { 
      position = _position;
      width = _width;
      height = _height;
      invWidth = 1 / (float)width;
      invHeight = 1 / (float)height;
      fov = _fov;
      aspectratio = width / float(height);
      angle = tan(0.5 * fov * M_PI/ 180.0);
      angleX = 0;
      angleY = 0;
      angleZ = 0;
    }

    Vector3 pixelToViewport(Vector3 pixel) {
      float vx = (2 * ((pixel.x + 0.5) * invWidth) - 1) * angle * aspectratio;
      float vy = (1 - 2 * ((pixel.y + 0.5) * invHeight)) * angle;
      Vector3 rayDirection  = Vector3(vx, vy, pixel.z);
      rayDirection.rotateX(angleX);
      rayDirection.rotateY(angleY);
      rayDirection.rotateZ(angleZ);
      rayDirection.normalize();
      return rayDirection;
    }
};

class Lighting {
  public:

    static Color getLightingSimple(const Shape &object, const Vector3 &point, const Vector3 &normal, const Vector3 &view, const vector<Light*> &lights, const vector<Shape*> &objects) {
      Color ambient = object.color;
      Color rayColor = ambient * object.ka;

      // Compute illumination with shadows
      for(int i = 0; i < lights.size(); i++) {
        bool isInShadow = getShadow(point, *lights[i], objects);
        
        if (!isInShadow)
          rayColor +=  getLighting(object, point, normal, view, lights[i]);
        //else
        //  rayColor += Color(0);
      }

      return rayColor;
    }

    static Color getLighting(const Shape &object, const Vector3 &point, const Vector3 &normal, const Vector3 &view, const vector<Light*> &lights, const vector<Shape*> &objects) {
      Color ambient = object.color;
      Color rayColor = ambient * object.ka;

      // Compute illumination with shadows
      for(int i = 0; i < lights.size(); i++) {
        float shadowFactor = getShadowFactor(point, *lights[i], objects);
        rayColor += getLighting(object, point, normal, view, lights[i]) * (1.0 - shadowFactor);
      }

      return rayColor;
    }

    static bool getShadow(const Vector3 &point, const Light &light, const vector<Shape*> &objects) {
      Vector3 shadowRayDirection = light.position - point;
      shadowRayDirection.normalize();
      Ray shadowRay(point, shadowRayDirection);

      bool isInShadow = false;
      for(int j = 0; j < objects.size(); j++) {
        float t0 = INFINITY; float t1 = INFINITY;
        if(objects[j]->intersect(shadowRay, t0, t1)) {
          isInShadow = true;
          break;
        }
      }
      return isInShadow;
    }

    static float getShadowFactor(const Vector3 &point, const Light &light, const vector<Shape*> &objects) { 

      if(light.type == 0x20) {
        
        int shadowCount = 0;
        Vector3 start(light.position.x - (light.width / 2), light.position.y - (light.height / 2), light.position.z);
        Vector3 step(light.width / light.samples, light.height / light.samples, 0);
        Vector3 lightSample;
        Vector3 jitter;

        for(int i = 0; i < light.samples; i++) {
          for(int j = 0; j < light.samples; j++) {
            jitter = Vector3::random() * step - (step / 2.0);
            lightSample = Vector3(start.x + (step.x * i) + jitter.x, start.y + (step.y * j) + jitter.y, start.z);

            Vector3 shadowRayDirection = lightSample - point;
            shadowRayDirection.normalize();
            Ray shadowRay(point, shadowRayDirection);

            bool isInShadow = false;
            for(int j = 0; j < objects.size(); j++) {
              float t0 = INFINITY; float t1 = INFINITY;
              if(objects[j]->intersect(shadowRay, t0, t1)) {
                isInShadow = true;
                break;
              }
            }

            if(isInShadow)
              shadowCount++;
          }
        }

        return shadowCount / (float) light.samples;  // Light Factor
      }
      else {
        bool isInShadow = getShadow(point, light, objects);
        if(isInShadow)
          return 1.0;
        else
          return 0.0;
      }
      
    }

    static Color getLighting(const Shape &object, const Vector3 &point, const Vector3 &normal, const Vector3 &view, const Light *light) {
      Color rayColor;

      // Create diffuse color
      Vector3 N = normal;
      
      Vector3 L = light->position - point;
      float distance = L.length();
      L.normalize();
      float attenuate = light->attenuate(distance);

      float NdotL = N.dot(L);
      float intensity = max(0.0f, NdotL); 
      Color diffuse = object.color * light->intensity * intensity * attenuate;
      
      // Create specular color
      Vector3 V = view;
      Vector3 H = L + V;
      H.normalize();

      float shinniness = object.shininess;
      float NdotH = N.dot(H);
      float specularIntensity = pow( max(0.0f, NdotH), shinniness );
      Color specular = object.color_specular * light->intensity * specularIntensity * attenuate;

      rayColor = diffuse * object.kd + specular * object.ks;   

      return rayColor;
    }
};

class Renderer {
  public:
    int width, height;
    Scene scene;
    Camera camera;
    
    Renderer(float _width, float _height, Scene _scene, Camera _camera) : 
      width(_width), height(_height), scene(_scene), camera(_camera) {}

    void render() { 
      Color *image = new Color[width * height], *pixel = image;

      for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++, pixel++) {
          // Send a ray through each pixel
          Vector3 rayDirection = camera.pixelToViewport( Vector3(x, y, 1) );
          Ray ray(camera.position, rayDirection);
          // Sent pixel for traced ray
          *pixel = trace(ray, 0);
        }
      }

      drawImage(image, width, height);
    }

    void render_distributed_rays() { 
      int samples = 16;
      float inv_samples = 1 / (float) samples;

      Color *image = new Color[width * height], *pixel = image;

      for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++, pixel++) {
          for (int s = 0; s < samples; s++) {
            Vector3 r = Vector3::random();
            float jx = x + r.x;
            float jy = y + r.y;

            // Send a jittered ray through each pixel
            Vector3 rayDirection = camera.pixelToViewport( Vector3(jx, jy, 1) );

            Ray ray(camera.position, rayDirection);

            // Sent pixel for traced ray
            *pixel += trace(ray, 0) * inv_samples;
          } 
        }
      }

      drawImage(image, width, height);
    }

    Color trace(const Ray &ray, const int &depth) {
      Color rayColor;
      float tnear = INFINITY;
      Shape* hit = NULL;
      // Find nearest intersection with ray and objects in scene
      for (int i = 0; i < scene.objects.size(); i++) {
        float t0 = INFINITY, t1 = INFINITY;
        if (scene.objects[i]->intersect(ray, t0, t1)) {
          if (t0 < 0) t0 = t1;
          if (t0 < tnear) {
            tnear = t0;
            hit = scene.objects[i];
          }
        }
      }
      if (!hit) {
        if(depth < 1)
          return scene.backgroundColor;
        else
          return Color();
      }

      Vector3 hitPoint = ray.origin + ray.direction * tnear;
      Vector3 N = hit->getNormal(hitPoint);
      N.normalize();
      Vector3 V = camera.position - hitPoint;
      V.normalize();

      rayColor = Lighting::getLighting(*hit, hitPoint, N, V, scene.lights, scene.objects);

      float bias = 1e-4;
      bool inside = false;
      if (ray.direction.dot(N) > 0) N = -N, inside = true;
      if( (hit->transparency > 0 || hit->reflectivity > 0) && depth < MAX_RAY_DEPTH) {
          
          // Compute Reflection Ray and Color 
          Vector3 R = ray.direction - N * 2 * ray.direction.dot(N);
          R = R + Vector3::random() * hit->glossiness;
          R.normalize();

          Ray rRay(hitPoint + N * bias, R);
          float VdotR =  max(0.0f, V.dot(-R));
          Color reflectionColor = trace(rRay,  depth + 1); //* VdotR;
          Color refractionColor = Color();

          if (hit->transparency > 0) {
            // Compute Refracted Ray (transmission ray) and Color
            float ni = 1.0;
            float nt = 1.1;
            float nit = ni / nt;
            if(inside) nit = 1 / nit;
            float costheta = - N.dot(ray.direction);
            float k = 1 - nit * nit * (1 - costheta * costheta);
            Vector3 T = ray.direction * nit + N * (nit * costheta - sqrt(k));
            T = T + Vector3::random() * hit->glossy_transparency;
            T.normalize();

            Ray refractionRay(hitPoint - N * bias, T);
            refractionColor = trace(refractionRay, depth + 1);
            rayColor = (reflectionColor * hit->reflectivity) + (refractionColor * hit->transparency);
          }
          else {
            rayColor = rayColor + (reflectionColor * hit->reflectivity);
          }
          return rayColor;
        }
        else {
          return rayColor; 
        }
    }

    void drawImage(Color* image, int width, int height) {
      ofstream out("./scene.ppm", std::ios::out | std::ios::binary);
      out << "P6\n" << width << " " << height << "\n255\n";
      for (unsigned i = 0; i < width * height; i++) {
        Color pixel = image[i].clamp();
        out << (unsigned char) pixel.r;
        out << (unsigned char) pixel.g;
        out << (unsigned char) pixel.b;
      }
      out.close();
    }     
};

class IO {
  public:
    static void ImageToPPMFile(Color* image, int width, int height) {
      ofstream out("./scene.ppm", std::ios::out | std::ios::binary);
      out << "P6\n" << width << " " << height << "\n255\n";
      for (unsigned i = 0; i < width * height; i++) {
        Color pixel = image[i].clamp();
        out << (unsigned char) pixel.r;
        out << (unsigned char) pixel.g;
        out << (unsigned char) pixel.b;
      }
      out.close();
    } 
};

class MedianSplit {
  public:
    void create() { }
    void algorithm() { }
};

int main() {

  printf ("Generating Scene ...\n");
  clock_t t;
  t = clock();

  int width = 1080;
  int height = 800;
  float fov = 30.0;

  Scene scene = Scene();
  scene.backgroundColor = Color();

  Triangle t0 = Triangle( Vector3(0, -4+1, 0), Vector3(1, -4+-1, 0), Vector3(-1, -4+-1, 0), Color(165, 10, 14), 1.0, 0.5, 0.0, 128.0, 0.0);
  Triangle t1 = Triangle( Vector3(0, 4, -30), Vector3(5, -4, -30), Vector3(-5, -4, -30), Color(165, 10, 14), 1.0, 0.5, 0.0, 128.0, 0.0);

  Sphere ts0 = Sphere( Vector3(0, 4, 30), 0.2, Color(255), 0.3, 0.8, 0.5, 128.0, 1.0);
  Sphere ts1 = Sphere( Vector3(5, -4, 30), 0.2, Color(255), 0.3, 0.8, 0.5, 128.0, 1.0);
  Sphere ts2 = Sphere( Vector3(-5, -4, 30), 0.2, Color(255), 0.3, 0.8, 0.5, 128.0, 1.0);

  Sphere s0 = Sphere( Vector3(0, -10004, 20), 10000, Color(51, 51, 51), 0.2, 0.5, 0.0, 128.0, 0.0); // Black - Bottom Surface
  Sphere s1 = Sphere( Vector3(0, 0, 20), 4, Color(165, 10, 14), 0.3, 0.8, 0.5, 128.0, 0.05, 0.95); // Clear
  s1.glossy_transparency = 0.02;
  s1.glossiness = 0.05;
  Sphere s2 = Sphere( Vector3(5, -1, 15), 2, Color(235, 179, 41), 0.4, 0.6, 0.4, 128.0, 1.0); // Yellow
  s2.glossiness = 0.2;
  Sphere s3 = Sphere( Vector3(5, 0, 25), 3, Color(6, 72, 111), 0.3, 0.8, 0.1, 128.0, 1.0);  // Blue
  s3.glossiness = 0.4;
  Sphere s4 = Sphere( Vector3(-3.5, -1, 10), 2, Color(8, 88, 56), 0.4, 0.6, 0.5, 64.0, 1.0); // Green
  s4.glossiness = 0.3;
  Sphere s5 = Sphere( Vector3(-5.5, 0, 15), 3, Color(51, 51, 51), 0.3, 0.8, 0.25, 32.0, 0.0); // Black

  // Add spheres to scene
  scene.addObject( &t0 );  
  scene.addObject( &t1 ); 
  scene.addObject( &ts0 );
  scene.addObject( &ts1 );
  scene.addObject( &ts2 );
  
  scene.addObject( &s0 );
  scene.addObject( &s1 );  // Red
  scene.addObject( &s2 );  // Yellow
  scene.addObject( &s3 );  // Blue
  scene.addObject( &s4 );  // Green
  scene.addObject( &s5 );  // Black
  
  // Add light to scene
  scene.addAmbientLight ( AmbientLight( Vector3(1.0) ) );
  //DirectionalLight l0 = DirectionalLight( Vector3(0, 20, 35), Vector3(1.4) );
  //PointLight l1 = PointLight( Vector3(20, 20, 35), Vector3(1000.0) );  
  AreaLight l0 = AreaLight( Vector3(0, 20, 35), Vector3(1.4) );
  AreaLight l1 = AreaLight( Vector3(20, 20, 35), Vector3(1.8) );
  scene.addLight( &l0 );
  scene.addLight( &l1 );

  // Add camera
  //Camera camera = Camera( Vector3(0,0,0), width, height, fov);
  Camera camera = Camera( Vector3(0,0,-20), width, height, fov);
  camera.position = Vector3(0, 20, -20);
  camera.angleX = 30 * M_PI/ 180.0;

  // Create Renderer
  Renderer r = Renderer(width, height, scene, camera);
  //r.render();
  r.render_distributed_rays();

  t = clock() - t;
  printf ("Scene Complete. Time ellpased: %.2f seconds.\n", ((float)t) / CLOCKS_PER_SEC);
  
  return 0;
}