/***************************************************************************
 *   Copyright (C) 2006-2010 by Paul-Louis Ageneau                         *
 *   paul-louis (at) ageneau (dot) org                               *
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

#ifndef MESH_H
#define MESH_H

#include "p3d/include.hpp"
#include "p3d/resource.hpp"
#include "p3d/buffer.hpp"
#include "p3d/bufferobject.hpp"
#include "p3d/collidable.hpp"

namespace pla
{

class Mesh : public Resource, public Collidable
{
public:
	typedef unsigned int index_t;
	#define INVALID_INDEX (index_t(-1))

	Mesh(void);
	Mesh(	const index_t *indices,
		size_t nindices,
		const float *vertices,
		size_t nvertices);
	
	virtual ~Mesh(void);
	
	void setIndices(const index_t *indices, size_t count = 0);
	void setVertexAttrib(unsigned layout, const float *attribs, size_t count = 0, int size = 3);
	void setVertexAttrib(unsigned layout, const int *attribs, size_t count = 0, int size = 1);
	void unsetVertexAttrib(unsigned layout);
	
	size_t indicesCount(void) const;
	size_t vertexAttribCount(unsigned layout = 0) const;
	int vertexAttribSize(unsigned layout = 0) const;
	
	void optimize(unsigned layout = 0);
	void computeNormals(unsigned normalLayout = 1, unsigned layout = 0);
	
	float computeRadius(void);
	float getRadius(void) const;
	
	int drawElements(void);
	int drawElements(index_t first, index_t count) const;
	
	virtual float intersect(const vec3 &pos, const vec3 &move, float radius, vec3 *intersection);
	
protected:
	void enableBuffers(void) const;
	void disableBuffers(void) const;

	typedef Buffer<index_t>		IndexBuffer;
	typedef Buffer<float>		AttribBuffer;
	typedef Buffer<int>		AttribIntBuffer;
	
	struct Attrib
	{
		sptr<AttribBuffer> buffer;
		int size = 3;
	};
	
	struct AttribInt // Not templatable because OpenGL calls are different
	{
		sptr<AttribIntBuffer> buffer;
		int size = 1;
	};
	
	// Buffers
	sptr<IndexBuffer>		mIndexBuffer;
	std::map<unsigned, Attrib>	mAttribBuffers;
	std::map<unsigned, AttribInt>	mAttribIntBuffers;

	float mRadius = -1.f;
};

}

#endif