//======================================================================
// Radial (Equirectangular) environment mapping
//      By Damian Trebilco
//======================================================================

#include "App.h"

#define GL_TEXTURE_CUBE_MAP_SEAMLESS 0x884F

#define RADIAL_TEX_WIDTH  2048
#define RADIAL_TEX_HEIGHT 1024

BaseApp *app = new App();

void App::moveCamera(const vec3 &dir)
{
  vec3 newPos = camPos + dir * (speed * frameTime);
  camPos = newPos;
}


void App::resetCamera()
{
  camPos = vec3(-0.052f, 0.069f, 0.144f);
  wx = 0.35f;
  wy = -2.63f;
  speed = 0.2f;
}

void App::onSize(const int w, const int h)
{
  OpenGLApp::onSize(w, h);
}

void App::onSliderChanged(Slider *Slider)
{
  // Update the mip bias on slider change
  if(Slider == mipBias)
  {
    char buffer[100];
    _snprintf_s(buffer, 100, 22, "Mip Bias: %.2f", mipBias->getValue());
    mipBiasLabel->setText(buffer);
  }

  BaseApp::onSliderChanged(Slider);
}


void App::onCheckBoxClicked(CheckBox *checkBox)
{
  if(checkBox == seamlessCubeMaps)
  {
    // Just require OpenGL 3.2 (could also check for the extension)
    if(GLVER(3,2))
    {
      if(seamlessCubeMaps->isChecked())
      {
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
      }
      else
      {
        glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
      }
    }    
  }

  BaseApp::onCheckBoxClicked(checkBox);
}


bool App::init()
{
  // Load the geometry
  model = new Model();
  if (!model->loadObj("../Models/Horse.obj")){
    ErrorMsg("Couldn't load model file");
    return false;
  }

  model->flipComponents(0, 1, 2);
  model->reverseWinding();
  model->flipComponents(1, 1, 2);

  // Add the rendering tab to the UI
  int tab = configDialog->addTab("Rendering");

  configDialog->addWidget(tab, doRadialEnvMap   = new CheckBox    (0,   10,  350, 36, "Radial EnvMap",  true));
  configDialog->addWidget(tab, mipBiasLabel     = new Label       (0,   60,  200, 36, "Mip Bias: 0.0"));
  configDialog->addWidget(tab, mipBias          = new Slider      (200, 60,  200, 36, 0.0, 16, 0.0));
  configDialog->addWidget(tab, seamlessCubeMaps = new CheckBox    (0,   110, 350, 36, "Seamless CubeMaps", false));
  configDialog->addWidget(tab, constantMipLevel = new CheckBox    (0,   150, 350, 36, "Constant Mip Level", false));

  // Set the callback on the controls
  mipBias->setListener(this);
  seamlessCubeMaps->setListener(this);

  // Select the rendering tab as the active tab
  configDialog->setCurrentTab(tab);

  radialSelect = 0;
  constMipSelect = 0;

  return true;
}


void App::exit()
{
  delete model;
}


bool App::load()
{
  if (!GLSL_supported){
    ErrorMsg("No GLSL support");
    return false;
  }

  // Set the shader version used
  ((OpenGLRenderer*)renderer)->SetShaderVersionStr("#version 130");

  // Shaders
  if ((generateRadial = renderer->addShader("generateRadial.shd")) == SHADER_NONE) return false;

  if ((objectShaders[0][0] = renderer->addShader("object.shd")) == SHADER_NONE) return false;
  if ((objectShaders[0][1] = renderer->addShader("object.shd", "#define CONSTANT_MIP\n")) == SHADER_NONE) return false;
  if ((objectShaders[1][0] = renderer->addShader("object.shd", "#define RADIAL_MAP\n")) == SHADER_NONE) return false;
  if ((objectShaders[1][1] = renderer->addShader("object.shd", "#define RADIAL_MAP\n#define CONSTANT_MIP\n")) == SHADER_NONE) return false;

  if ((skyboxShaders[0][0] = renderer->addShader("skybox.shd")) == SHADER_NONE) return false;
  if ((skyboxShaders[0][1] = renderer->addShader("skybox.shd", "#define CONSTANT_MIP\n")) == SHADER_NONE) return false;
  if ((skyboxShaders[1][0] = renderer->addShader("skybox.shd", "#define RADIAL_MAP\n" )) == SHADER_NONE) return false;
  if ((skyboxShaders[1][1] = renderer->addShader("skybox.shd", "#define RADIAL_MAP\n#define CONSTANT_MIP\n" )) == SHADER_NONE) return false;

  // Filtering modes
  if ((trilinearClamp = renderer->addSamplerState(TRILINEAR, CLAMP, CLAMP, CLAMP)) == SS_NONE) return false;
  if ((trilinearAniso = renderer->addSamplerState(TRILINEAR_ANISO, WRAP, WRAP, WRAP)) == SS_NONE) return false;
  
  // Radial textures only wrap in one direction
  if ((radialFilter   = renderer->addSamplerState(TRILINEAR, WRAP, CLAMP, CLAMP)) == SS_NONE) return false;

  // Textures
  const char *files[] = {
    "../Textures/CubeMaps/UnionSquare/posx.jpg", "../Textures/CubeMaps/UnionSquare/negx.jpg",
    "../Textures/CubeMaps/UnionSquare/posy.jpg", "../Textures/CubeMaps/UnionSquare/negy.jpg",
    "../Textures/CubeMaps/UnionSquare/posz.jpg", "../Textures/CubeMaps/UnionSquare/negz.jpg",
  };
  if ((cubeEnv = renderer->addCubemap(files, true, trilinearClamp)) == TEXTURE_NONE) return false;

  if ((radialGenRT = renderer->addRenderTarget(RADIAL_TEX_WIDTH, RADIAL_TEX_HEIGHT, FORMAT_RGBA8, radialFilter)) == TEXTURE_NONE) return false;

  // Draw a quad over the screen
  {
    // Bind the radial env map gen render target
    renderer->changeRenderTarget(radialGenRT);

    renderer->reset();
    renderer->setShader(generateRadial);
    renderer->setTexture("Env", cubeEnv);
    renderer->setDepthState(noDepthWrite);
    renderer->apply();

    glBegin(GL_TRIANGLES);
      glVertex3f(0,  2, 1);
      glVertex3f(3, -1, 1);
      glVertex3f(-3, -1, 1);
    glEnd();

    // Read back the pixels into a image
    Image img;
	  ubyte *pixels = img.create(FORMAT_RGB8, RADIAL_TEX_WIDTH, RADIAL_TEX_HEIGHT, 1, 1);
    glReadPixels(0, 0, img.getWidth(), img.getHeight(), GL_RGB, GL_UNSIGNED_BYTE, pixels);
    img.createMipMaps();

    // Save the image to disk
    if (!img.saveDDS("GenRadialTex.dds")){
      return false;
    }

    // Re-bind the main render target
    renderer->changeToMainFramebuffer();
  }

  if ((radialEnv = renderer->addTexture("GenRadialTex.dds", true, radialFilter)) == TEXTURE_NONE) return false;

  // Upload map to vertex/index buffer
  if (!model->makeDrawable(renderer, true)) return false;

  return true;
}


void App::drawEnvironment(const float4x4 &mvp)
{
  renderer->reset();

  renderer->setShader(skyboxShaders[radialSelect][constMipSelect]);
  renderer->setTexture("EnvCube", cubeEnv);
  renderer->setTexture("Env2D", radialEnv);
  renderer->setShaderConstant1f("mipBias", mipBias->getValue());
  renderer->setDepthState(noDepthWrite);
  renderer->apply();

  float4x4 iMvp = !mvp;

  float4 v0( 0,  2, 1, 1);
  float4 v1( 3, -1, 1, 1);
  float4 v2(-3, -1, 1, 1);

  glBegin(GL_TRIANGLES);
    glTexCoord4fv(iMvp * v0);
    glVertex4fv(v0);
    glTexCoord4fv(iMvp * v1);
    glVertex4fv(v1);
    glTexCoord4fv(iMvp * v2);
    glVertex4fv(v2);
  glEnd();

}


void App::drawObject()
{
  // Apply the render states for no sorting
  renderer->reset();
  
  renderer->setShader(objectShaders[radialSelect][constMipSelect]);
	renderer->setShaderConstant3f("camPos", camPos);
  renderer->setTexture("EnvCube", cubeEnv);
  renderer->setTexture("Env2D", radialEnv);
  renderer->setShaderConstant1f("mipBias", mipBias->getValue());
  renderer->apply();

  // Render the translucent model
  for (uint i = 0; i < model->getBatchCount(); i++){
    model->drawBatch(renderer, i);
  }
}


void App::drawFrame()
{
  mat4 projection = perspectiveMatrixX(1.5f, width, height, 0.001f, 1.0f);
  mat4 mvRotate = rotateXY(-wx, -wy);
  mat4 modelview = mvRotate * translate(-camPos);

  glMatrixMode(GL_PROJECTION);
  glLoadTransposeMatrixf(projection);

  glMatrixMode(GL_MODELVIEW);
  glLoadTransposeMatrixf(modelview);

  // Determine the shader selections
  radialSelect = 0;
  if(doRadialEnvMap->isChecked())
  {
    radialSelect = 1;
  }
  constMipSelect = 0;
  if(constantMipLevel->isChecked())
  {
    constMipSelect = 1;
  }

  // Clear only depth
  renderer->clear(false, true, false);

  // Draw the background environment
  drawEnvironment(projection * mvRotate);

  // Draw the object
  drawObject();
}
