
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