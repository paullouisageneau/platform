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

#ifndef P3D_PROGRAM_H
#define P3D_PROGRAM_H

#include "p3d/include.hpp"
#include "p3d/resource.hpp"
#include "p3d/shader.hpp"
#include "p3d/buffer.hpp"
#include "pla/string.hpp"

namespace pla
{

class Program : public Resource
{  
public:
	Program(void);
	Program(sptr<Shader> vertexShader, sptr<Shader> fragmentShader, bool mustLink = false);
	~Program(void);
	
	void attachShader(sptr<Shader> shader);
	void detachShader(sptr<Shader> shader);
	void bindAttribLocation(unsigned index, const String &name);
	void link(void);
	void bind(void);
	void unbind(void);
	
	bool hasUniform(const String &name);
	bool hasVertexAttrib(const String &name);

	int getUniformLocation(const String &name);
        int getAttribLocation(const String &name);

	void setUniform(const String &name, float value);
	void setUniform(const String &name, int value);
	void setUniform(const String &name, const float *values, int count);
	void setUniform(const String &name, const int *values, int count);
	void setUniform(const String &name, const vec3 &value);
	void setUniform(const String &name, const vec4 &value);
	void setUniform(const String &name, const mat4 &value);
	
	void setVertexAttrib(const String &name, float value);
	void setVertexAttrib(const String &name, const float *values);
	void setVertexAttrib(const String &name, const vec3 &value);
	void setVertexAttrib(const String &name, const vec4 &value);
	
private:
	GLuint mProgram;
	
	std::set<sptr<Shader> > mShaders;
	std::map<String, int> mUniformLocations;	// Cache
	std::map<String, int> mAttribLocations;		// Cache
};

}

#endif
