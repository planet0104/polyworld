#pragma once

class SceneRenderer;
class AgentPovRenderer;
class gstage;

struct SceneRenderer_CameraProperties
{
	float color[4];
	float fov;
};

#if defined(_WIN32) && !defined(__CYGWIN__)
#define PW_RENDERER_EXPORT extern "C" __declspec(dllexport)
#else
#define PW_RENDERER_EXPORT extern "C"
#endif

// Stable C ABI between libpolyworld and libpwqtrenderer (MinGW cannot leave
// SceneRenderer::create undefined in the core DLL like GNU ld does for .so).
PW_RENDERER_EXPORT SceneRenderer *pw_SceneRenderer_create(
	gstage *stage,
	const SceneRenderer_CameraProperties *cameraProps,
	int width,
	int height);

PW_RENDERER_EXPORT AgentPovRenderer *pw_AgentPovRenderer_create(
	int maxAgents,
	int retinaWidth,
	int retinaHeight);
