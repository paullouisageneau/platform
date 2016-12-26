/***************************************************************************
 *   Copyright (C) 2006-2016 by Paul-Louis Ageneau                         *
 *   paul-louis (at) ageneau (dot) org                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#include "p3d/engine.hpp"
#include "p3d/mediamanager.hpp"

namespace pla
{

Engine::Engine(void)
{
	// SDL initialization
        if(SDL_Init(SDL_INIT_EVERYTHING) < 0) throw Exception("SDL initialisation failed");

        mResourceManager = std::make_shared<ResourceManager>();
        mMediaManager    = std::make_shared<MediaManager>(mResourceManager);

        // Registering loaders
        //mMediaManager.registerLoader<Image>(new ImageLoader,"bmp,jpg,jpeg,png,pcx,dds,pnm,sgi,tga,tif,gif");
        //mMediaManager.registerLoader<Texture>(new TextureLoader,"bmp,jpg,jpeg,png,pcx,dds,pnm,sgi,tga,tif,gif");
        //mMediaManager.registerLoader<Terrain>(new TerrainLoader,"bmp,jpg,jpeg,png,pcx,dds,pnm,sgi,tga,tif,gif");
        //mMediaManager.registerLoader<Mesh>(new Loader3ds,"3ds");
        //mMediaManager.registerLoader<Mesh>(new Md2Loader,"md2");
        //mMediaManager.registerLoader<Sample>(new OggVorbisSampleLoader,"ogg");
        //mMediaManager.registerLoader<Music>(new OggVorbisMusicLoader,"ogg");
        //mMediaManager.registerLoader<Shader>(new ShaderLoader,"frag,vert");
        //mMediaManager.registerLoader<Program>(new ProgramLoader,"prog");
}

Engine::~Engine(void)
{
        // Clear states stack
        while(!mStates.empty())
                popState();

        // SDL termination
        SDL_GL_DeleteContext(mContext);
        SDL_DestroyWindow(mWindow);
        SDL_Quit();
}

void Engine::openWindow(int width, int height, bool fullscreen, int antialias)
{
	// Create window
	if(fullscreen)
	{
		mWindow = SDL_CreateWindow("",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			0, 0,
			SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
	else {
		mWindow = SDL_CreateWindow("",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			width, height,
			SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
	}
	
	if(!mWindow) throw Exception(String("Failed to open the window: ") + SDL_GetError());
	
	// Set OpenGL attributes
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);	// TODO
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,		8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,		8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,		8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,		8);
	
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE,		32);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,		16);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,	8);
	
	SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE,	8);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE,	8);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE,	8);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE,	8);
	
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,    	1);
	
	if(antialias)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,	1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,	antialias);
	}
	
	// Create OpenGL context
	mContext = SDL_GL_CreateContext(mWindow);
	if(!mContext) throw Exception(String("Failed to open OpenGL context: ") + SDL_GetError());
	SDL_GL_MakeCurrent(mWindow, mContext);
	
	glewExperimental = GL_TRUE;
	glewInit();
}

void Engine::setWindowTitle(const String &title)
{
	SDL_SetWindowTitle(mWindow, title.c_str());
}

void Engine::setWindowSize(int width, int height)
{
	SDL_SetWindowSize(mWindow, width, height);
}

void Engine::getWindowSize(int *width, int *height) const
{
	SDL_GetWindowSize(mWindow, width, height);
}

void Engine::setCursor(bool visible)
{
	if(visible) 
	{
		SDL_SetRelativeMouseMode(SDL_FALSE);
		SDL_ShowCursor(SDL_ENABLE);
	}
	else {
		SDL_SetRelativeMouseMode(SDL_TRUE);
		SDL_ShowCursor(SDL_DISABLE);
	}
}

void Engine::changeState(sptr<State> state)
{
	popState();
	pushState(state);
}

void Engine::pushState(sptr<State> state)
{
	mStates.push(state);
	state->onInit(this);
	mMesureTime=mOldTime=getTime();
	mMesureFrames=0;
}

void Engine::popState(void)
{
	if(mStates.empty()) return;
	
	mStates.top()->onCleanup(this);
	mStates.pop();

	mMesureTime=mOldTime=getTime();
	mMesureFrames=0;
}

sptr<Engine::State> Engine::getState(void) const
{
	if(mStates.empty()) return NULL;
	else return mStates.top();
}

sptr<ResourceManager> Engine::resourceManager(void)
{
        return mResourceManager;
}

sptr<MediaManager> Engine::mediaManager(void)
{
	return mMediaManager;
}

bool Engine::update(void)
{
	if(mStates.empty()) return false;
	
	// Poll events
	mKeysChanged.clear();
	mKeysPressed.clear();
	mButtonsChanged.clear();
	mButtonsPressed.clear();
	
	mMouseMovex = 0;
	mMouseMovey = 0;
	mMouseMovez = 0;
	
	SDL_Event event;
	while(SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_KEYDOWN:
		{
			int key = event.key.keysym.sym;
			mKeysDown.insert(key);
			mKeysChanged.insert(key);
			mKeysPressed.insert(key);
			mStates.top()->onKey(this, key, true);
			break;
		}
		
		case SDL_KEYUP:
		{
			int key = event.key.keysym.sym;
			mKeysDown.erase(key);
			mKeysChanged.insert(key);
			mStates.top()->onKey(this, key, false);
			break;
		}
	
		case SDL_TEXTINPUT:
		{
			mStates.top()->onInput(this, String(event.text.text));	
			break;
		}
		
		case SDL_MOUSEBUTTONDOWN:
		{
			int button = event.button.button;
			mButtonsDown.insert(button);
			mButtonsChanged.insert(button);
			mButtonsPressed.insert(button);
			mStates.top()->onMouse(this, button, true);
			break;
		}
		
		case SDL_MOUSEBUTTONUP:
		{
			int button = event.button.button;
			mButtonsDown.erase(button);
			mButtonsChanged.insert(button);
			mStates.top()->onMouse(this, button, false);
			break;
		}
	
		case SDL_MOUSEWHEEL:
		{
			mMouseWheel+= event.wheel.y;
			mMouseMovez+= event.wheel.y;
			break;
		}
		
		case SDL_MOUSEMOTION:
		{
			mMouseMovex+= event.motion.xrel;
			mMouseMovey+= event.motion.yrel;
			break;
		}
		
		case SDL_WINDOWEVENT:
		{
			switch (event.window.event)
			{
				case SDL_WINDOWEVENT_RESIZED:
				{
					int width = event.window.data1;
					int height = event.window.data2;
					glViewport(0, 0, width, height);
					break;
				}
			}
			break;
		}
		
		case SDL_QUIT:
			return false;
		}
	}
	
	double currentTime=getTime();			// Current time
	double elapsed = currentTime - mOldTime;	// Time since last frame
	
	if(elapsed < MIN_FRAME_TIME)
	{
		sleep(MIN_FRAME_TIME-elapsed);
		currentTime = getTime();
		elapsed = currentTime - mOldTime;
	}
	
	mOldTime = currentTime;
	
	int pass = 1;
	double time = elapsed;
	while(time > MAX_UPDATE_TIME)
	{
		time/= 2;
		pass*= 2;
	}
	
	while(pass)
	{
		while(!mStates.top()->onUpdate(this, time))	// Update current state
		{
			popState();	// If finished, remove it from stack
			if(mStates.empty()) return false;
		}
	
		pass--;
	}
	
	// Process sound
	//Sound::Process();
	return true;
}

int Engine::display(void)
{
	if(mStates.empty()) return 0;
	
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	
	int count=mStates.top()->onDraw(this);
	SDL_GL_SwapWindow(mWindow);
	
	// Compute FPS
	++mMesureFrames;
	if(mMesureFrames > 10)
	{
		mFps=mMesureFrames/(getTime()-mMesureTime);
		mMesureTime=getTime();
		mMesureFrames=0;
	}
	
	return count;
}

bool Engine::isKeyDown(int key)
{
	return (mKeysDown.find(key) != mKeysDown.end());
}

bool Engine::isKeyChanged(int key) const
{
	return (mKeysChanged.find(key) != mKeysChanged.end());
}

bool Engine::isKeyPressed(int key)
{
	return (mKeysPressed.find(key) != mKeysPressed.end());
}

bool Engine::isMouseButtonDown(int button)
{
	return (mButtonsDown.find(button) != mButtonsDown.end());
}

bool Engine::isMouseButtonChanged(int button) const
{
	return (mButtonsChanged.find(button) != mButtonsChanged.end());
}

bool Engine::isMouseButtonPressed(int button)
{
	return (mButtonsPressed.find(button) != mButtonsPressed.end());
}

void Engine::getMousePosition(int *x, int *y, int *z) const
{
	SDL_GetMouseState(x,y);
        if(z) *z = getMouseWheel();
}

void Engine::getMouseMove(int *x, int *y, int *z) const
{
	if(x) *x = mMouseMovex;
	if(y) *y = mMouseMovey;
	if(z) *z = mMouseMovez;
}

int Engine::getMouseWheel(void) const
{
        return mMouseWheel;
}

double Engine::getTime(void) const
{
	return double(SDL_GetTicks())*0.001;
}

double Engine::getTimeStamp(void) const
{
	return mOldTime;
}

float Engine::getFps(void) const
{
	return mFps;
}

void Engine::sleep(double time) const
{
	SDL_Delay(unsigned(time*1000.));
}

unsigned Engine::getLogicClock(void) const
{
	return mLogicTicks;
}

unsigned Engine::tickLogicClock(void)
{
	mLogicTicks++;
	return mLogicTicks;
}

unsigned Engine::syncLogicClock(unsigned ticks)
{
	mLogicTicks = std::max(ticks, mLogicTicks);
	return mLogicTicks;
}

}
