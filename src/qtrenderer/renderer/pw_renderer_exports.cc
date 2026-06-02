#include "utils/pw_renderer_exports.h"

#include "agent/AgentPovRenderer.h"
#include "graphics/gstage.h"
#include "monitor/SceneRenderer.h"
#include "renderer/qt/QtAgentPovRenderer.h"
#include "renderer/qt/QtSceneRenderer.h"

PW_RENDERER_EXPORT SceneRenderer *pw_SceneRenderer_create(
	gstage *stage,
	const SceneRenderer_CameraProperties *cameraProps,
	int width,
	int height )
{
	SceneRenderer::CameraProperties props(
		Color( cameraProps->color[0],
			   cameraProps->color[1],
			   cameraProps->color[2],
			   cameraProps->color[3] ),
		cameraProps->fov );
	return new QtSceneRenderer( *stage, props, width, height );
}

PW_RENDERER_EXPORT AgentPovRenderer *pw_AgentPovRenderer_create(
	int maxAgents,
	int retinaWidth,
	int retinaHeight )
{
	return new QtAgentPovRenderer( maxAgents, retinaWidth, retinaHeight );
}
