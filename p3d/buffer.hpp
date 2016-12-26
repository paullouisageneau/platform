/***************************************************************************
 *   Copyright (C) 2006-2010 by Paul-Louis Ageneau                         *
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

#ifndef P3D_BUFFER_H
#define P3D_BUFFER_H

#include "p3d/include.hpp"
#include "p3d/bufferobject.hpp"
#include "pla/exception.hpp"

namespace pla
{

template<typename T>
class Buffer
{
public:
	Buffer(BufferObject *buffer);	// Encapsulate a buffer object
	virtual ~Buffer(void);
	
	size_t count(void) const;
	
	void *bind(void) const;	
	void unbind(void) const;
	void *offset(size_t offset=0) const;
	void fill(const T *ptr, size_t nbr, GLenum usage=GL_DYNAMIC_DRAW);
	void replace(size_t offset, const T *ptr, size_t nbr);
	T *lock(size_t offset, size_t nbr, GLenum access=GL_READ_ONLY);
	T *lock(size_t offset, size_t nbr) const;
	void unlock(void) const;
	void add(const T *ptr, size_t nbr, GLenum usage=GL_DYNAMIC_DRAW);

protected:
	sptr<BufferObject> mBuffer;
	size_t mCount = 0;
};


template <class T>
Buffer<T>::Buffer(BufferObject *buffer) :
	mBuffer(buffer)
{
	Assert(mBuffer);
	mCount = mBuffer->size()/sizeof(T);
}

template <class T>
Buffer<T>::~Buffer(void)
{

}

template <class T>
size_t Buffer<T>::count(void) const
{
	return mCount;
}

template <class T>
void *Buffer<T>::bind(void) const
{
	return mBuffer->bind();
}

template <class T>
void Buffer<T>::unbind(void) const
{
	mBuffer->unbind();
}

template <class T>
void *Buffer<T>::offset(size_t offset) const
{
	return mBuffer->offset(offset*sizeof(T));
}

template <class T>
void Buffer<T>::fill(const T *ptr,size_t nbr,GLenum usage)
{
	mBuffer->fill(ptr,nbr*sizeof(T),usage);
	mCount=nbr;
}

template <class T>
void Buffer<T>::replace(size_t offset,const T *ptr,size_t nbr)
{
	mBuffer->replace(offset*sizeof(T),ptr,nbr*sizeof(T));
}

template <class T>
T *Buffer<T>::lock(size_t offset,size_t nbr,GLenum access)
{
		return reinterpret_cast<T*>(mBuffer->lock(offset*sizeof(T),nbr*sizeof(T),access));
}

template <class T>
T *Buffer<T>::lock(size_t offset,size_t nbr) const
{
		return reinterpret_cast<T*>(mBuffer->lock(offset*sizeof(T),nbr*sizeof(T),GL_READ_ONLY));
}

template <class T>
void Buffer<T>::unlock(void) const
{
	mBuffer->unlock();
}

template <class T>
void Buffer<T>::add(const T *ptr,size_t nbr,GLenum usage)
{
	if(!nbr) return;
	if(!mCount) {
		fill(ptr,nbr,usage);
		return;
	}

	// Allocate buffer
	T *temp = new T[mCount+nbr];

	// Copy old
	const T *p = lock(0,mCount,GL_READ_ONLY);
	std::copy(p,p+mCount,temp);
	unlock();

	// Copy new
	if(ptr) std::copy(ptr,ptr+nbr,temp+mCount);
	else std::fill(temp+mCount,temp+mCount+nbr,T(0));

	// Refill
	fill(temp,mCount+nbr,usage);
}

}

#endif

