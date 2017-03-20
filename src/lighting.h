
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