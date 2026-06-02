#if defined(_WIN32) && !defined(__CYGWIN__)

#include "pw_renderer_exports.h"

#include <windows.h>

#include "agent/AgentPovRenderer.h"
#include "monitor/SceneRenderer.h"
#include <string>

#include "utils/misc.h"
#include "utils/pw_dynload.h"

#ifndef PWHOME
#define PWHOME "."
#endif

static void *qtrenderer_module()
{
	static void *handle = NULL;
	if( handle )
		return handle;

	SetDllDirectoryA( ( std::string( PWHOME ) + "/lib" ).c_str() );

	const char *candidates[] = {
		"lib/pwqtrenderer.dll",
		"lib/libpwqtrenderer.dll",
		"pwqtrenderer.dll",
		"libpwqtrenderer.dll",
		NULL };
	for( int i = 0; candidates[i]; i++ )
	{
		handle = pw_dlopen( candidates[i] );
		if( handle )
			break;
		std::string abs = std::string( PWHOME ) + "/" + candidates[i];
		handle = pw_dlopen( abs.c_str() );
		if( handle )
			break;
	}
	ERRIF( !handle, "Failed opening pwqtrenderer.dll (Qt renderer plugin)" );
	return handle;
}

template<typename Fn>
static Fn resolve_renderer( const char *symbol )
{
	Fn fn = (Fn)GetProcAddress( (HMODULE)qtrenderer_module(), symbol );
	ERRIF( !fn, "Missing symbol in libpwqtrenderer.dll: %s", symbol );
	return fn;
}

SceneRenderer *SceneRenderer::create( gstage &stage,
									  const CameraProperties &cameraProps,
									  int width,
									  int height )
{
	typedef SceneRenderer *(*Fn)( gstage *,
								  const SceneRenderer_CameraProperties *,
								  int,
								  int );
	static Fn fn = NULL;
	if( !fn )
		fn = resolve_renderer<Fn>( "pw_SceneRenderer_create" );

	SceneRenderer_CameraProperties props;
	props.color[0] = cameraProps.color.r;
	props.color[1] = cameraProps.color.g;
	props.color[2] = cameraProps.color.b;
	props.color[3] = cameraProps.color.a;
	props.fov = cameraProps.fov;

	return fn( &stage, &props, width, height );
}

AgentPovRenderer *AgentPovRenderer::create( int maxAgents,
											int retinaWidth,
											int retinaHeight )
{
	typedef AgentPovRenderer *(*Fn)( int, int, int );
	static Fn fn = NULL;
	if( !fn )
		fn = resolve_renderer<Fn>( "pw_AgentPovRenderer_create" );
	return fn( maxAgents, retinaWidth, retinaHeight );
}

#endif
