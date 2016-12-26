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

#ifndef P3D_ENGINE_H
#define P3D_ENGINE_H

#include <p3d/include.hpp>
#include <p3d/resourcemanager.hpp>
#include <p3d/mediamanager.hpp>
#include <pla/string.hpp>

#include <stack>

#define MIN_FRAME_TIME  1./60.	// 60 frames/sec max
#define MAX_UPDATE_TIME 1./20.	// 20 updates/sec min

namespace pla
{

class Engine
{
	
public:
	Engine(void);
	~Engine(void);

	void openWindow(int width, int height, bool fullscreen=false, int antialias=0);
	void setWindowTitle(const String &title);
	void setWindowSize(int width, int height);
	void getWindowSize(int *width, int *height) const;

	void setCursor(bool visible);
	bool isKeyDown(int key);
	bool isKeyChanged(int key) const;
	bool isKeyPressed(int key);
	bool isMouseButtonDown(int button);
	bool isMouseButtonChanged(int button) const;
	bool isMouseButtonPressed(int button);
	void getMousePosition(int *x, int *y, int *z=NULL) const;
	void getMouseMove(int *x, int *y, int *z=NULL) const;
	int  getMouseWheel(void) const;

	double getTime(void) const;
	double getTimeStamp(void) const;
	float  getFps(void) const;
	void   sleep(double time) const;

	unsigned getLogicClock(void) const;
	unsigned tickLogicClock(void);
	unsigned syncLogicClock(unsigned ticks);

	class State
	{
	public:
        	virtual void onInit(Engine *engine) = 0;
        	virtual void onCleanup(Engine *engine) = 0;
		
        	virtual bool onUpdate(Engine *engine, double time) = 0;
        	virtual int  onDraw(Engine *engine) = 0;
	
        	virtual void onKey(Engine *engine, int key, bool down) {}
        	virtual void onMouse(Engine *engine, int button, bool down) {}
        	virtual void onInput(Engine *engine, String text) {}
	};

	void changeState(sptr<State> state);
	void pushState(sptr<State> state);
	void popState(void);
	sptr<State> getState(void) const;
	
	sptr<ResourceManager> resourceManager(void);
	sptr<MediaManager> mediaManager(void);

	bool update(void);
	int  display(void);

private:
	SDL_Window *mWindow = NULL;
	SDL_Renderer *mRenderer = NULL;
	SDL_GLContext mContext;
	
	sptr<ResourceManager> mResourceManager;
	sptr<MediaManager> mMediaManager;

	// Engine states stack
	std::stack<sptr<State> > mStates;
	
	// Window size and cursor position
	int mMouseMovex = 0;
	int mMouseMovey = 0;
	int mMouseMovez = 0;
	int mMouseWheel = 0;
	
	// Time handling
	double mOldTime = 0., mMesureTime = 0.;
	unsigned long mMesureFrames = 0;
	float mFps = 0.f;
	unsigned mLogicTicks = 0;
	
	// Keys and buttons states
	std::set<int> mKeysDown;
	std::set<int> mKeysChanged;
	std::set<int> mKeysPressed;
	std::set<int> mButtonsDown;
	std::set<int> mButtonsChanged;
	std::set<int> mButtonsPressed;
};

// TODO
#define KEY_SPACE	SDLK_SPACE
#define KEY_ESCAPE	SDLK_ESCAPE
#define KEY_F1		SDLK_F1
#define KEY_F2		SDLK_F2
#define KEY_F3		SDLK_F3
#define KEY_F4		SDLK_F4
#define KEY_F5		SDLK_F5
#define KEY_F6		SDLK_F6
#define KEY_F7		SDLK_F7
#define KEY_F8		SDLK_F8
#define KEY_F9		SDLK_F9
#define KEY_F10		SDLK_F10
#define KEY_F11		SDLK_F11
#define KEY_F12		SDLK_F12
#define KEY_UP		SDLK_UP
#define KEY_DOWN	SDLK_DOWN
#define KEY_LEFT	SDLK_LEFT
#define KEY_RIGHT	SDLK_RIGHT
#define KEY_LSHIFT	SDLK_LSHIFT
#define KEY_RSHIFT	SDLK_RSHIFT
#define KEY_LCTRL	SDLK_LCTRL
#define KEY_RCTRL	SDLK_RCTRL
#define KEY_LALT	SDLK_LALT
#define KEY_RALT	SDLK_RALT
#define KEY_TAB		SDLK_TAB
#define KEY_ENTER	SDLK_RETURN
#define KEY_BACKSPACE	SDLK_BACKSPACE
#define KEY_INSERT	SDLK_INSERT
#define KEY_DEL		SDLK_DEL
#define KEY_PAGEUP	SDLK_PAGEUP
#define KEY_PAGEDOWN	SDLK_PAGEDOWN
#define KEY_HOME	SDLK_HOME
#define KEY_END		SDLK_END

#define MOUSE_BUTTON_LEFT	1
#define MOUSE_BUTTON_MIDDLE	2
#define MOUSE_BUTTON_RIGHT	3

}

#endif
