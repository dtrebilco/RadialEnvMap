//======================================================================
// Radial (Equirectangular) environment mapping
//      By Damian Trebilco
//======================================================================

#include "../Framework3/OpenGL/OpenGLApp.h"
#include "../Framework3/Util/Model.h"


class App : public OpenGLApp {
public:

  char *getTitle() const { return "Radial Environment Map"; }

  void moveCamera(const vec3 &dir);
  void resetCamera();

  bool init();
  void exit();

  void onSize(const int w, const int h);
	void onSliderChanged(Slider *Slider);
	void onCheckBoxClicked(CheckBox *checkBox);

  bool load();

  void drawFrame();
  void drawEnvironment(const float4x4 &mvp);
  void drawObject();


protected:

  Model *model;                  // The render model

  ShaderID generateRadial;       // Shader to generate a radial env map from a cubemap

  ShaderID objectShaders[2][2];  // Object shaders cubemap/radial, varMip/fixedMip 
  ShaderID skyboxShaders[2][2];  // Skybox shaders cubemap/radial, varMip/fixedMip 
  
  TextureID cubeEnv;             // The cube map env map
  TextureID radialEnv;           // The radial env map 

  TextureID radialGenRT;         // The render target used to generate the radial env map 

  SamplerStateID trilinearClamp, trilinearAniso, radialFilter;

  CheckBox *doRadialEnvMap;     // If using radial environment mapping

  Label    *mipBiasLabel;       // The label on the UI for mipBias
  Slider   *mipBias;            // The mip bias

  CheckBox *seamlessCubeMaps;   // If using seamless cubemaps
  CheckBox *constantMipLevel;   // If using constant mip levels

  uint radialSelect;            // Shader select for radial env mapping
  uint constMipSelect;          // Shader select for const mipmapping
}; 
