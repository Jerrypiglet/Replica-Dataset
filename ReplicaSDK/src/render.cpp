// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
#include <EGL.h>
#include <PTexLib.h>
#include <pangolin/image/image_convert.h>

#include "FileParser.h"
#include "GLCheck.h"
#include "MirrorRenderer.h"
#include "DumpEXR.h"

int main(int argc, char* argv[]) {
  ASSERT(argc >= 3 && argc <= 6, "Usage: ./ReplicaRenderer mesh.ply /path/to/atlases [mirrorFile] /path/to/openCVCameraFile /path/to/intrinsicMatrixFile");

  const std::string meshFile(argv[1]);
  const std::string atlasFolder(argv[2]);
  ASSERT(pangolin::FileExists(meshFile));
  ASSERT(pangolin::FileExists(atlasFolder));

  std::string surfaceFile;
  if (argc >= 4) {
    surfaceFile = std::string(argv[3]);
    ASSERT(pangolin::FileExists(surfaceFile));
  }

  std::string openCVCameraFile;
  if (argc >= 5) {
    openCVCameraFile = std::string(argv[4]);
    ASSERT(pangolin::FileExists(openCVCameraFile));
  }

  std::string intrinsicMatrixFile;
  if (argc >= 6) {
    intrinsicMatrixFile = std::string(argv[5]);
    ASSERT(pangolin::FileExists(intrinsicMatrixFile));
  }

  // Set up file stream
  std::fstream fileStream;

  // Read from intrinsicMatrixFile
  fileStream.open(intrinsicMatrixFile);
  const int height = parseInt(fileStream);
  const int width = parseInt(fileStream);
  Eigen::Matrix3d intrinsicCameraMatrix(parseMatrix3d(fileStream));
  fileStream.close();

  bool renderDepth = true;
  float depthScale = 65535.0f * 0.1f;

  // Setup EGL
  EGLCtx egl;

  // egl.PrintInformation();
  
  if(!checkGLVersion()) {
    return 1;
  }

  //Don't draw backfaces
  const GLenum frontFace = GL_CCW;
  glFrontFace(frontFace);

  // Setup a framebuffer
  // pangolin::GlTexture render(width, height);
  pangolin::GlTexture render(width, height, GL_RGB32F, true, 0, GL_RGB, GL_FLOAT, 0);
  pangolin::GlRenderBuffer renderBuffer(width, height);
  pangolin::GlFramebuffer frameBuffer(render, renderBuffer);

  pangolin::GlTexture depthTexture(width, height, GL_R32F, false, 0, GL_RED, GL_FLOAT, 0);
  pangolin::GlFramebuffer depthFrameBuffer(depthTexture, renderBuffer);


  
  // Setup a camera
  pangolin::OpenGlRenderState s_cam(
      pangolin::ProjectionMatrixRDF_BottomLeft(
          width,
          height,
          intrinsicCameraMatrix(0, 0), // fx K[0][0]
          intrinsicCameraMatrix(1, 1), // fy K[1][1]
          intrinsicCameraMatrix(0, 2), // u0 K[0][2]
          intrinsicCameraMatrix(1, 2), // v0 K[1][2]
          0.1f,
          100.0f),
      // pangolin::ModelViewLookAtRDF(0, 0, 4, 0, 0, 0, 0, 1, 0));
      pangolin::ModelViewLookAtRDF(-0.336, 2.372, 0.004, -0.336+0.9074, 2.372-0.1819, 0.004-0.3788, 0.371, -0.074, 0.925));

  // Start at some origin
  // Eigen::Matrix4d T_camera_world = s_cam.GetModelViewMatrix();

  // And move to the left
  // Eigen::Matrix4d T_new_old = Eigen::Matrix4d::Identity();
  // T_new_old.topRightCorner(3, 1) = Eigen::Vector3d(0.025, 0, 0);

  // load mirrors
  std::vector<MirrorSurface> mirrors;
  if (surfaceFile.length()) {
    std::ifstream file(surfaceFile);
    picojson::value json;
    picojson::parse(json, file);

    for (size_t i = 0; i < json.size(); i++) {
      mirrors.emplace_back(json[i]);
    }
    std::cout << "Loaded " << mirrors.size() << " mirrors" << std::endl;
  }

  const std::string shadir = STR(SHADER_DIR);
  MirrorRenderer mirrorRenderer(mirrors, width, height, shadir);

  // load mesh and textures
  PTexMesh ptexMesh(meshFile, atlasFolder);

  // pangolin::ManagedImage<Eigen::Matrix<uint8_t, 3, 1>> image(width, height);
  pangolin::TypedImage image(width, height, pangolin::PixelFormatFromString("RGB96F"));
  pangolin::ManagedImage<float> depthImage(width, height);
  pangolin::ManagedImage<uint16_t> depthImageInt(width, height);

  // Read from openCVCameraFile
  fileStream.open(openCVCameraFile);

  // Render some frames
  const size_t numFrames = parseInt(fileStream);
  for (size_t i = 0; i < numFrames; i++) {
    std::cout << "\rRendering frame " << i + 1 << "/" << numFrames << "... ";
    std::cout.flush();

    // Move the camera
    Eigen::Vector3d cameraOrigin(parseVector3d(fileStream));
    Eigen::Vector3d cameraLookAt(parseVector3d(fileStream));
    Eigen::Vector3d cameraUp(parseVector3d(fileStream));

    
    // s_cam.GetModelViewMatrix() = parseMatrix4d(fileStream);
    // s_cam.GetModelViewMatrix() = pangolin::ModelViewLookAtRDF(-0.336, 2.372, 0.004, -0.336+0.9074, 2.372-0.1819, 0.004-0.3788, 0.371, -0.074, 0.925);
    s_cam.GetModelViewMatrix() = pangolin::ModelViewLookAtRDF(cameraOrigin(0), cameraOrigin(1), cameraOrigin(2),
                                                              cameraLookAt(0), cameraLookAt(1), cameraLookAt(2),
                                                              cameraUp(0), cameraUp(1), cameraUp(2));

    // Render
    frameBuffer.Bind();
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, width, height);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glEnable(GL_CULL_FACE);

    ptexMesh.Render(s_cam);

    glDisable(GL_CULL_FACE);

    glPopAttrib(); //GL_VIEWPORT_BIT
    frameBuffer.Unbind();

    for (size_t i = 0; i < mirrors.size(); i++) {
      MirrorSurface& mirror = mirrors[i];
      // capture reflections
      mirrorRenderer.CaptureReflection(mirror, ptexMesh, s_cam, frontFace);

      frameBuffer.Bind();
      glPushAttrib(GL_VIEWPORT_BIT);
      glViewport(0, 0, width, height);

      // render mirror
      mirrorRenderer.Render(mirror, mirrorRenderer.GetMaskTexture(i), s_cam);

      glPopAttrib(); //GL_VIEWPORT_BIT
      frameBuffer.Unbind();
    }

    char filename[1000];
    snprintf(filename, 1000, "frame%06zu.exr", i);

    // Download and save
    // render.Download(image.ptr, GL_RGB, GL_UNSIGNED_BYTE);

    // pangolin::SaveImage(
    //     image.UnsafeReinterpret<uint8_t>(),
    //     pangolin::PixelFormatFromString("RGB24"),
    //     std::string(filename));

    render.Download(image);
    // pangolin::SaveImage(image, std::string(filename));

    SaveExr(
      image,
      // .UnsafeReinterpret<uint8_t>(),
      // .UnsafeReinterpret<float_t>(), 
      pangolin::PixelFormatFromString("RGB96F"),
      std::string(filename),
      true
    );
        

    if (renderDepth) {
      // render depth
      depthFrameBuffer.Bind();
      glPushAttrib(GL_VIEWPORT_BIT);
      glViewport(0, 0, width, height);
      glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

      glEnable(GL_CULL_FACE);

      ptexMesh.RenderDepth(s_cam, depthScale);

      glDisable(GL_CULL_FACE);

      glPopAttrib(); //GL_VIEWPORT_BIT
      depthFrameBuffer.Unbind();

      depthTexture.Download(depthImage.ptr, GL_RED, GL_FLOAT);

      // convert to 16-bit int
      for(size_t i = 0; i < depthImage.Area(); i++)
          depthImageInt[i] = static_cast<uint16_t>(depthImage[i] + 0.5f);

      snprintf(filename, 1000, "depth%06zu.png", i);
      pangolin::SaveImage(
          depthImageInt.UnsafeReinterpret<uint8_t>(),
          pangolin::PixelFormatFromString("GRAY16LE"),
          std::string(filename), true, 34.0f);
    }

    // Move the camera
    // T_camera_world = T_camera_world * T_new_old.inverse();

    // s_cam.GetModelViewMatrix() = T_camera_world;
  }
  std::cout << "\rRendering frame " << numFrames << "/" << numFrames << "... done" << std::endl;

  fileStream.close();

  return 0;
}

