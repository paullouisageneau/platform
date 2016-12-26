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

#ifndef P3D_BUFFEROBJECT_H
#define P3D_BUFFEROBJECT_H

#include "p3d/include.hpp"
#include "pla/exception.hpp"

namespace pla
{

class BufferObject
{
public:
	// type = GL_ARRAY_BUFFER or GL_ELEMENT_ARRAY_BUFFER
	BufferObject(GLenum type = GL_ELEMENT_ARRAY_BUFFER);
	virtual ~BufferObject(void);
	
	size_t size(void) const;
	void *bind(void);
	void unbind(void);
	void *offset(size_t offset);
	void fill(const void *ptr, size_t size, GLenum usage=GL_DYNAMIC_DRAW);
	void replace(size_t offset, const void *ptr, size_t size);
	void *lock(size_t offset, size_t size, GLenum access=GL_READ_ONLY);
	void unlock(void);

private:
	GLenum mType;
	GLuint mVBO;
	size_t mSize = 0;

};

class IndexBufferObject : public BufferObject
{
public:
	IndexBufferObject(void) : BufferObject(GL_ELEMENT_ARRAY_BUFFER) {}
	~IndexBufferObject(void) {}
};

class AttribBufferObject : public BufferObject
{
public:
	AttribBufferObject(void) : BufferObject(GL_ARRAY_BUFFER) {}
	~AttribBufferObject(void) {}
};

}

#endif

