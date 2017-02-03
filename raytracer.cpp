#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>

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
};

struct Color {
  float r, g, b;
  Color() : r(0), g(0), b(0) {}
  Color(float c) : r(c), g(c), b(c) {}
  Color(float _r, float _g, float _b) : r(_r), g(_g), b(_b) {}
  Color operator * (float f) const { return Color(r * f, g * f, b * f); }
  Color operator * (Vector3 f) const { return Color(r * f.x, g * f.y, b * f.z); }
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
    Vector3 center;       // Position
    Color color;          // Surface Diffuse Color
    Color color_specular; // Surface Specular Color
    float ka, kd, ks;     // Ambient, Diffuse, Specular Coefficents
    float shininess;
    float reflectivity;   // Reflectivity of material [0, 1]

    virtual bool intersect(const Ray &ray, float &to, float &t1) { return false; }
    virtual bool intersect(const Ray &ray, float &t) { return false; }
    virtual bool intersect2(const Ray &ray, float &t) { return false; }
    virtual Vector3 getNormal(const Vector3 &hitPoint) { return Vector3(); }
};

class Sphere : public Shape {
  public:
    float radius, radius2;

    Sphere(const Vector3 &_center, const float _radius, const Color &_color, const float _ka, const float _kd, const float _ks, const float _shinny = 128.0, const float _reflectScale = 1.0) :
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
        float t0 = -b-disc;
        float t1 = -b+disc;
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

    Triangle(const Vector3 &_v0, const Vector3 &_v1, const Vector3 &_v2, const Color &_color, const float _ka, const float _kd, const float _ks, const float _shinny = 128.0, const float _reflectScale = 1.0) :
      v0(_v0), v1(_v1), v2(_v2)
      {
        color = _color;
        color_specular = Color(255);
        ka = _ka;
        kd = _kd;
        ks = _ks;
        shininess = _shinny;
        reflectivity = _reflectScale;
      }
    
    // Compute a ray-sphere intersection using the geometric method
    bool intersect(const Ray &ray, float &t) {
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
      return v01.cross(v02);
    }
};

class Light {
  public:
    Vector3 position;
    Vector3 intensity;

    Light() : position(Vector3()), intensity(Vector3()) {}
    Light(Vector3 _intensity) : position(Vector3()), intensity(_intensity) {} 
    Light(Vector3 _position, Vector3 _intensity) : position(_position), intensity(_intensity) {} 
    virtual float attenuate() { return 1.0; }
};

class AmbientLight : public Light {
  public:
    AmbientLight() : Light() {}
    AmbientLight(Vector3 _intensity) : Light(_intensity) {}
    float attenuate() { return 1.0; }
};

class DirectionalLight : public Light {
  public:
    DirectionalLight() : Light() {}
    DirectionalLight(Vector3 _position, Vector3 _intensity) : Light(_position, _intensity) {}
    float attenuate() { return 1.0; }
};

class PointLight : public Light {
  public:
    PointLight() : Light() {}
    PointLight(Vector3 _position, Vector3 _intensity) : Light(_position, _intensity) {}
    float attenuate(float r) { return 1.0 / r * r; }
};

class SpotLight : public Light {
  public:
    SpotLight() : Light() {}
    SpotLight(Vector3 _position, Vector3 _intensity) : Light(_position, _intensity) {}
    float attenuate(Vector3 Vobj, Vector3 Vlight) { return Vobj.dot(Vlight); }
};

class Scene {
  public:
    vector<Shape*> objects;
    vector<Light*> lights;
    AmbientLight ambientLight;
    Color backgroundColor;

    Scene() { backgroundColor = Color(); }
    void addAmbientLight(AmbientLight _light) { ambientLight = _light;}
    void addLight(DirectionalLight _light) { lights.push_back(& _light); }
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

    static Color getLighting(const Shape &object, const Vector3 &point, const Vector3 &normal, const Vector3 &view, const vector<Light*> &lights) {
      Color ambient = object.color;
      Color rayColor = ambient * object.ka;
      for(int i = 0; i < lights.size(); i++) {
        rayColor += getLighting(object, point, normal, view, *lights[i]);
      }
      return rayColor;
    }

    static Color getLighting(const Shape &object, const Vector3 &point, const Vector3 &normal, const Vector3 &view, const Light &light) {
      Color rayColor;

      // Create diffuse color
      Vector3 N = normal;
      Vector3 L = light.position - point;
      float distance = L.length();
      L.normalize();
      
      float NdotL = N.dot(L);
      float intensity = max(0.0f, NdotL); 
      Color diffuse = object.color * light.intensity * intensity; // * (1 / distance2);
      
      // Create specular color
      Vector3 V = view;
      Vector3 H = L + V;
      H.normalize();

      float shinniness = object.shininess;
      float NdotH = N.dot(H);
      float specularIntensity = pow( max(0.0f, NdotH), shinniness );
      Color specular = object.color_specular * light.intensity * specularIntensity;// * (1/ distance2);

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
      Vector3 V = camera.position - hitPoint;
      V.normalize();
      rayColor = Lighting::getLighting(*hit, hitPoint, N, V, scene.lights);

      if(depth < MAX_RAY_DEPTH) {
          Vector3 R = ray.direction - N * 2 * ray.direction.dot(N);
          Ray rRay(hitPoint, R);
          float VdotR =  max(0.0f, V.dot(-R));
          Color reflectionColor = trace(rRay,  depth + 1) * VdotR;
          rayColor = rayColor + (reflectionColor * hit->reflectivity);
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

int main() {

  printf ("Generating Scene ...\n");
  clock_t t;
  t = clock();

  int width = 1080;
  int height = 800;
  float fov = 30.0;

  Scene scene = Scene();
  scene.backgroundColor = Color();

  //Triangle s0 = Triangle( Vector3(0,0,0), Vector3(0,0,20), Vector3(20,0,0), Color(51, 51, 51), 0.2, 0.5, 0.0, 128.0, 0.4);
  Sphere s0 = Sphere( Vector3(0, -10004, 20), 10000, Color(51, 51, 51), 0.2, 0.5, 0.0, 128.0, 0.4); // Black - Bottom Surface
  Sphere s1 = Sphere( Vector3(0, 0, 20), 4, Color(165, 10, 14), 0.3, 0.8, 0.5, 128.0, 1.0); // Red
  Sphere s2 = Sphere( Vector3(5, -1, 15), 2, Color(235, 179, 41), 0.4, 0.6, 0.4, 128.0, 1.0); // Yellow
  Sphere s3 = Sphere( Vector3(5, 0, 25), 3, Color(6, 72, 111), 0.3, 0.8, 0.1, 128.0, 1.0);  // Blue
  Sphere s4 = Sphere( Vector3(-3.5, -1, 10), 2, Color(8, 88, 56), 0.4, 0.6, 0.5, 64.0, 1.0); // Green
  Sphere s5 = Sphere( Vector3(-5.5, 0, 15), 3, Color(51, 51, 51), 0.3, 0.8, 0.25, 32.0, 0.0); // Black

  // Add spheres to scene
  scene.addObject( &s0 );  // Black - Bottom Surface
  scene.addObject( &s1 );  // Red
  scene.addObject( &s2 );  // Yello
  scene.addObject( &s3 );  // Blue
  scene.addObject( &s4 );  // Green
  scene.addObject( &s5 );  // Black
  
  // Add light to scene
  scene.addAmbientLight ( AmbientLight( Vector3(1.0) ) );
  scene.addLight( DirectionalLight( Vector3(0, 20, 30), Vector3(2.0) ) );
  scene.addLight( DirectionalLight( Vector3(20, 20, 30), Vector3(2.0) ) );

  // Add camera
  Camera camera = Camera( Vector3(0,0,-20), width, height, fov);
  //camera.position = Vector3(0, 20, -20);
  //camera.angleX = 30 * M_PI/ 180.0;

  // Create Renderer
  Renderer r = Renderer(width, height, scene, camera);
  r.render();

  t = clock() - t;
  printf ("Scene Complete. Time ellpased: %.2f seconds.\n", ((float)t) / CLOCKS_PER_SEC);
  
  return 0;
}