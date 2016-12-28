/***************************************************************************
 *   Copyright (C) 2015-2016 by Paul-Louis Ageneau                         *
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

#include "demo/game.hpp"

using pla::Pi;
using pla::Epsilon;

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat2;
using glm::mat3;
using glm::mat4;

namespace demo
{

Game::Game(void) :
	mWorld(43)
{

}

Game::~Game(void)
{

}

void Game::onInit(Engine *engine)
{
	mPosition.z = 10.f;
	mYaw = 0.f;
	mPitch = 0.f;
	mGravity = 0.f;
}

void Game::onCleanup(Engine *engine)
{

}

bool Game::onUpdate(Engine *engine, double time)
{
	if(engine->isKeyDown(KEY_ESCAPE))
		return false;
	
	int mx, my;
	engine->getMouseMove(&mx, &my);
	mYaw-= 0.01f*mx;
	mPitch-= 0.01f*my;
	mPitch = pla::bounds(mPitch, -Pi/2, Pi/2);
	
	vec3 move;
	mGravity+= 10.f*time;
	move.z-= mGravity*time;

	vec3 dir = vec3(std::sin(-mYaw), std::cos(-mYaw), 0.f);
	if(engine->isKeyDown(KEY_UP))
		move+= dir*float(10.f*time);
	if(engine->isKeyDown(KEY_DOWN))
		move-= dir*float(10.f*time);
	
	mat4 camera = mat4(1.0f);
	camera = glm::translate(camera, mPosition);
	camera = glm::rotate(camera, Pi/2, vec3(1, 0, 0));
	camera = glm::rotate(camera, mYaw, vec3(0, 1, 0));
	camera = glm::rotate(camera, mPitch, vec3(1, 0, 0));
	
	if(engine->isMouseButtonDown(MOUSE_BUTTON_LEFT) || engine->isMouseButtonDown(MOUSE_BUTTON_RIGHT))
	{
		vec3 front = vec3(camera*vec4(0, 0, -1, 0));
		vec3 intersection;
		if(mWorld.intersect(mPosition, front*10.f, 0.5f, &intersection) <= 1.f)
		{
			float delta;
			if(engine->isMouseButtonDown(MOUSE_BUTTON_RIGHT)) delta = 2.f;
			else delta = -2.f;
			
			float v = mWorld.value(intersection);
			v = pla::bounds(v-delta*float(time), -1.f, 1.f);
			mWorld.setValue(intersection, v, 0);
			mWorld.setValue(intersection, 0.f, 1);
		}
	}
	
	vec3 newmove, intersection, normal;
	if(mWorld.collide(mPosition - vec3(0.f, 0.5f, 0.f), move, 1.f, &newmove, &intersection, &normal))
	{
		move = newmove;
		
		if(normal.z > 0.f)
		{
			mGravity = 0.f;
			
			// Jump
			if(engine->isKeyDown(KEY_SPACE))
			{
				mGravity = -10.f;
			}
		}
	}
	
	mPosition+= move;
	return true;
}

int Game::onDraw(Engine *engine)
{
	int count = 0;
	
	engine->clear(vec4(0.f, 0.f, 0.f, 1.f));
	
	int width, height;
	engine->getWindowSize(&width, &height);
	
	mat4 projection = glm::perspective(
		glm::radians(45.0f),
		float(width)/float(height), 
		0.1f, 1000.0f
	);
	
	mat4 camera = mat4(1.0f);
	camera = glm::translate(camera, mPosition);
	camera = glm::rotate(camera, Pi/2, vec3(1, 0, 0));
	camera = glm::rotate(camera, mYaw, vec3(0, 1, 0));
	camera = glm::rotate(camera, mPitch, vec3(1, 0, 0));
	
	Context context(projection, camera);
	context.setUniform("lightPosition", mPosition);
	
	count+= mWorld.draw(context);
	
	return count;
}

void Game::onKey(Engine *engine, int key, bool down)
{

}

void Game::onMouse(Engine *engine, int button, bool down)
{

}

void Game::onInput(Engine *engine, String text)
{

}

}

