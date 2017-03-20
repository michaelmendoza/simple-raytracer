#include <iostream>
using namespace std;

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
