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

#include "p3d/bufferobject.hpp"

namespace pla
{

BufferObject::BufferObject(GLenum type) :
	mType(type)
{
	glGenBuffers(1, &mVBO);
}

BufferObject::~BufferObject(void)
{
	glDeleteBuffers(1, &mVBO);
}

size_t BufferObject::size(void) const
{
        return mSize;
}

void *BufferObject::bind(void)
{
	glBindBuffer(mType, mVBO);
	return NULL;
}

void BufferObject::unbind(void)
{
	glBindBuffer(mType, 0);
}

void *BufferObject::offset(size_t offset)
{
	return reinterpret_cast<void*>(offset);
}

void BufferObject::fill(const void *ptr,size_t size,GLenum usage)
{
	mSize=size;
	glBindBuffer(mType, mVBO);
	glBufferData(mType, size, ptr, usage);
}

void BufferObject::replace(size_t offset,const void *ptr,size_t size)
{
	glBindBuffer(mType, mVBO);
	glBufferSubData(mType, offset, size, ptr);
}

void *BufferObject::lock(size_t offset,size_t size,GLenum access)
{
	glBindBuffer(mType, mVBO);
	if(size == 0) return NULL;
	void *ptr = glMapBuffer(mType, access);

	if(ptr) return reinterpret_cast<char*>(ptr) + offset;
	else throw Exception("Unable to lock the buffer object");
}

void BufferObject::unlock(void)
{
	glUnmapBuffer(mType);
}

}
