
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