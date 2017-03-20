
#define MAX_RAY_DEPTH 5

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