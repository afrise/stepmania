#include "global.h"
#include "LowLevelWindow_SDL.h"
#include "RageLog.h"
#include "RageException.h"
#include "PrefsManager.h" // XXX
#include "RageDisplay.h" // VideoModeParams
#include "DisplaySpec.h"
#include "LocalizedString.h"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "RageDisplay_OGL_Helpers.h"
using namespace RageDisplay_Legacy_Helpers;

#include <set>
#include <math.h>	// ceil()
#include <GL/glxew.h>

#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>	// All sorts of stuff...

static SDL_GLContext g_pContext = nullptr;
static SDL_GLContext g_pBackgroundContext = nullptr;
static bool g_bChangedScreenSize = false;

static SDL_Window* window = NULL;
static SDL_Surface* screenSurface = NULL;

LowLevelWindow_SDL::LowLevelWindow_SDL()
{
	
	SDL_SetMainReady();
	int numDisplayModes = SDL_GetNumDisplayModes(0);

	//default to basic displaymode.
	SDL_DisplayMode* displayMode;
	SDL_GetDisplayMode(0, 0, displayMode);
	
	ActualVideoModeParams parms;
	
	parms.width=displayMode->w;
	parms.height=displayMode->h;
	parms.bSmoothLines=false;
	parms.bTrilinearFiltering=false;
	parms.bAnisotropicFiltering=false;
	parms.windowWidth=displayMode->w;
	parms.windowHeight=displayMode->h;
	parms.fDisplayAspectRatio=displayMode->w / displayMode->h,
	parms.rate=displayMode->refresh_rate;
	parms.bpp=SDL_BITSPERPIXEL(displayMode->format);
	parms.renderOffscreen=true;

	CurrentParams = parms;

	SDL_Init (SDL_INIT_VIDEO);
	
	window = SDL_CreateWindow(
			"stepmania", 
			SDL_WINDOWPOS_CENTERED, 
			SDL_WINDOWPOS_CENTERED, 
			parms.width,
			parms.height, 
			SDL_WINDOW_OPENGL
		);

	g_pContext = SDL_GL_CreateContext(window);

    glViewport(0, 0, parms.width, parms.height);
    glClearColor(0.f, 0.f, 0.f, 0.f);
  
	SDL_GL_SetSwapInterval(0);
}

LowLevelWindow_SDL::~LowLevelWindow_SDL()
{
	if( g_pContext )
	{
		SDL_GL_DeleteContext(g_pContext);
		g_pContext = nullptr;
	}
	if( g_pBackgroundContext )
	{
		SDL_GL_DeleteContext(g_pBackgroundContext);
		g_pBackgroundContext = nullptr;
	}
	SDL_DestroyWindow(window);
	window = None;
}

void LowLevelWindow_SDL::RestoreOutputConfig() {
	LOG->Info( "LowLevelWindow_SDL: RestoreOutputConfig()");
}

void *LowLevelWindow_SDL::GetProcAddress( RString s )
{
	LOG->Info( "LowLevelWindow_SDL: GetProcAddress(%s)", s.c_str());
	return (void*) glXGetProcAddressARB( (const GLubyte*) s.c_str() );
}

RString LowLevelWindow_SDL::TryVideoMode( const VideoModeParams &p, bool &bNewDeviceOut )
{
	LOG->Info( "LowLevelWindow_SDL: TryVideoMode(%sx%x)"), p.width, p.height;
	bool bFirstRun = g_pContext == nullptr;
	bNewDeviceOut = false;
	return "";
}

void LowLevelWindow_SDL::LogDebugInformation() const
{
	LOG->Info( "LowLevelSDL: LogDebugInfo. maaybe later");
}

bool LowLevelWindow_SDL::IsSoftwareRenderer( RString &sError )
{
	return false;
}

void LowLevelWindow_SDL::SwapBuffers()
{
	//SDL_GL_SwapWindow(window); //isn't this handled by SDL now?
}

void LowLevelWindow_SDL::GetDisplaySpecs(DisplaySpecs &out) const {
	SDL_DisplayMode* mode;
	Uint32 f;

	SDL_GetCurrentDisplayMode(0, mode);

	
	f=mode->format;

	LOG->Info( "LowLevelSDL: Getting Display Specs: %sx%s %sHz", mode->w, mode->h, mode->refresh_rate);
	out.clear();
	DisplayMode sdlMode = {(uint)(mode->w), (uint)(mode->h), mode->refresh_rate};
	DisplaySpec sdlSpec("SDL", "SDL", sdlMode);
	out.insert( sdlSpec );
}

bool LowLevelWindow_SDL::SupportsThreadedRendering()
{
	return true; //idk
}

class RenderTarget_SDL: public RenderTarget
{
public:
	RenderTarget_SDL( LowLevelWindow_SDL *pWind );
	~RenderTarget_SDL();

	void Create( const RenderTargetParam &param, int &iTextureWidthOut, int &iTextureHeightOut );
	uintptr_t GetTexture() const { return static_cast<uintptr_t>(m_iTexHandle); }
	void StartRenderingTo();
	void FinishRenderingTo();

	// Copying from the Pbuffer to the texture flips Y.
	virtual bool InvertY() const { return true; }

private:
	int m_iWidth, m_iHeight;
	LowLevelWindow_SDL *m_pWind;
	SDL_GLContext m_pPbufferContext;
	GLuint m_iTexHandle;

	SDL_GLContext m_pOldContext;
	GLXDrawable m_pOldDrawable;
};

RenderTarget_SDL::RenderTarget_SDL( LowLevelWindow_SDL *pWind )
{
	m_pWind = pWind;
	m_pPbufferContext = nullptr;
	m_iTexHandle = 0;
	m_pOldContext = nullptr;
	m_pOldDrawable = 0;
}

RenderTarget_SDL::~RenderTarget_SDL()
{
	if (m_pPbufferContext)
		SDL_GL_DeleteContext(m_pPbufferContext);
	if( m_iTexHandle )
		glDeleteTextures( 1, reinterpret_cast<GLuint*>(&m_iTexHandle) );
}

void RenderTarget_SDL::Create( const RenderTargetParam &param, int &iTextureWidthOut, int &iTextureHeightOut )
{
	SDL_GL_MakeCurrent(window, g_pContext);
	m_iWidth = param.iWidth;
	m_iHeight = param.iHeight;

	glGenTextures( 1, reinterpret_cast<GLuint*>(&m_iTexHandle) );
	glBindTexture( GL_TEXTURE_2D, m_iTexHandle );
		while( glGetError() != GL_NO_ERROR )
		;

	int iTextureWidth = power_of_two( param.iWidth );
	int iTextureHeight = power_of_two( param.iHeight );
	iTextureWidthOut = iTextureWidth;
	iTextureHeightOut = iTextureHeight;

	glTexImage2D( GL_TEXTURE_2D, 0, param.bWithAlpha? GL_RGBA8:GL_RGB8,
			iTextureWidth, iTextureHeight, 0, param.bWithAlpha? GL_RGBA:GL_RGB, GL_UNSIGNED_BYTE, nullptr );
	GLenum error = glGetError();
	ASSERT_M( error == GL_NO_ERROR, GLToString(error) );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
}

void RenderTarget_SDL::StartRenderingTo()
{
	m_pOldContext = SDL_GL_GetCurrentContext();
	SDL_GL_MakeCurrent(window, m_pPbufferContext);
	glViewport( 0, 0, m_iWidth, m_iHeight );
}

void RenderTarget_SDL::FinishRenderingTo()
{
	glFlush();
	glBindTexture( GL_TEXTURE_2D, m_iTexHandle );

	while( glGetError() != GL_NO_ERROR )
	;

	glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, m_iWidth, m_iHeight );

	GLenum error = glGetError();
	ASSERT_M( error == GL_NO_ERROR, GLToString(error) );

	glBindTexture( GL_TEXTURE_2D, 0 );
	SDL_GL_MakeCurrent(window, m_pOldContext);

	m_pOldContext = nullptr;
	m_pOldDrawable = 0;
}

bool LowLevelWindow_SDL::SupportsRenderToTexture() const
{
	return true;
}

bool LowLevelWindow_SDL::SupportsFullscreenBorderlessWindow() const
{
	return true;
}

RenderTarget *LowLevelWindow_SDL::CreateRenderTarget()
{
	return new RenderTarget_SDL( this );
}

void LowLevelWindow_SDL::BeginConcurrentRenderingMainThread()
{
	SDL_GL_MakeCurrent(window, g_pContext);
}

void LowLevelWindow_SDL::EndConcurrentRenderingMainThread()
{
	SDL_GL_MakeCurrent(window, g_pContext);
}

void LowLevelWindow_SDL::BeginConcurrentRendering()
{
	SDL_GL_MakeCurrent(window, g_pBackgroundContext);
}

void LowLevelWindow_SDL::EndConcurrentRendering()
{
	SDL_GL_MakeCurrent(window, nullptr);
}

/*
 * (c) 2005 Ben Anderson
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
