
#define K_EPSILON 0.00001

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