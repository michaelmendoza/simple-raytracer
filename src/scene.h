
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