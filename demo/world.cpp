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

#include "demo/world.hpp"

using pla::LogImpl;

namespace demo
{

const int World::Size;

World::World(unsigned int seed) :
	mPerlin(seed)
{
	mProgram = std::make_shared<Program>(
		std::make_shared<VertexShader>("shader/ground.vert"),
		std::make_shared<FragmentShader>("shader/ground.frag")
	);
	
	mProgram->bindAttribLocation(0, "position");
	mProgram->bindAttribLocation(1, "normal");
	mProgram->bindAttribLocation(2, "environment");
	mProgram->link();
}

World::~World(void)
{
	
}

int World::draw(const Context &context)
{
	const float distance = 60.f;
	const float r = distance + Size*std::sqrt(2.f)*0.5f;
	const float r2 = r*r; 
	const vec3 pos = context.cameraPosition();
	
	int3 b = int3(pos).block();
	std::set<sptr<Block> > blocks, processed;
	getBlocksRec(b, blocks, processed, [pos, r2](sptr<Block> block)
	{
		vec3 p = block->center();
		return glm::distance2(pos, p) <= r2;
	});
	
	context.prepare(mProgram);
	
	mProgram->bind();
	
	int n = 0;
	for(auto blk : blocks)
	{
		blk->update();
		n+= blk->drawElements();
	}
	
	mProgram->unbind();
	return n;
}

float World::intersect(const vec3 &pos, const vec3 &move, float radius, vec3 *intersection)
{
	const vec3 p1 = pos;
	const vec3 p2 = pos + move;
	const vec3 n = glm::normalize(move);
	const float r = radius + Size*std::sqrt(2.f)*0.5f;
	const float r2 = r*r; 
	
	int3 b = int3(pos).block();
	std::set<sptr<Block> > blocks, processed;
	getBlocksRec(b, blocks, processed, [p1, p2, n, r2](sptr<Block> block)
	{
		vec3 p0 = block->center();
		vec3 p0p1 = p1 - p0;
		vec3 p2p0 = p0 - p2;
		float c = glm::dot(n, p0p1); 
		if(c > 0.f) return glm::length2(p0p1) <= r2;
		if(glm::dot(n, p2p0) > 0.f) return glm::length2(p2p0) <= r2;
		return glm::length2(p0p1 - n*c) <= r2;
	});
	
	float nearest = std::numeric_limits<float>::infinity();
	vec3 nearestIntersection;
	for(auto blk : blocks)
	{
		float t = blk->intersect(pos, move, radius, intersection);
		if(t < nearest)
		{
			nearest = t;
			if(intersection) nearestIntersection = *intersection;
		}
	}

	if(intersection) *intersection = nearestIntersection;
	return nearest;
}

void World::setValue(const int3 &p, float v, int layer)
{
	getBlock(p.block())->setValue(p.cell(), v, layer);
}

void World::setValue(const vec3 &p, float v, int layer)
{
	setValue(int3(p), v, layer);
}

float World::value(const int3 &p, int layer)
{
	return getBlock(p.block())->value(p.cell(), layer);
}

float World::value(const vec3 &p, int layer)
{
	return value(int3(p), layer);
}

void World::changedBlock(const int3 &b)
{
	auto it = mBlocks.find(b);
	if(it != mBlocks.end())
		it->second->mChanged = true;
}

sptr<World::Block> World::getBlock(const int3 &b)
{
	auto it = mBlocks.find(b);
	if(it != mBlocks.end())
		return it->second;
	
	sptr<Block> block = std::make_shared<Block>(this, b);
	mBlocks[b] = block;
	populateBlock(block);
	return block;
}

void World::getBlocksRec(const int3 &b, std::set<sptr<Block> > &result, std::set<sptr<Block> > &processed, std::function<bool(sptr<Block>)> check)
{
	sptr<Block> block = getBlock(b);
	if(processed.find(block) != processed.end())
		return;
	
	processed.insert(block);
	if(!check(block))
		return;
	
	result.insert(block);
	for(int dx=-1; dx<=1; ++dx)
		for(int dy=-1; dy<=1; ++dy)
			for(int dz=-1; dz<=1; ++dz)
				getBlocksRec(int3(b.x+dx, b.y+dy, b.z+dz), result, processed, check);
}

void World::populateBlock(sptr<Block> block)
{
	// Layer 0
	const float f1 = 0.15f;
	const float f2 = 0.03f;
	for(int x = 0; x < Size; ++x)
		for(int y = 0; y < Size; ++y)
			for(int z = 0; z < Size; ++z)
			{
				const int ax = block->mPos.x*Size+x;
				const int ay = block->mPos.y*Size+y;
				const int az = block->mPos.z*Size+z;
				const int d2 = ax*ax + ay*ay + az*az;
				const float noise1 = mPerlin.noise(ax*f1,ay*f1,az*f1*0.1f);
				const float noise2 = mPerlin.noise(ax*f2,ay*f2,az*f2*4.f);
				const float value = noise1*noise1*0.53f + (noise2-0.5f)*2.f*0.47f - 20.f/d2;
				block->setValue(int3(x, y, z), pla::bounds(value, -1.f, 1.f), 0);
			}
			
	// Layer 1
	const float f3 = 0.05f;
	for(int x = 0; x < Size; ++x)
		for(int y = 0; y < Size; ++y)
			for(int z = 0; z < Size; ++z)
			{
				int3 p(x, y, z);
				const int ax = block->mPos.x*Size+x;
				const int ay = block->mPos.y*Size+y;
				const int az = block->mPos.z*Size+z;
				const float noise = mPerlin.noise(ax*f3,ay*f3,az*f3);
				block->setValue(p, noise, 1);
			}
}

int World::int3::blockCoord(int v)
{
	if(v >= 0) return v/Size;
	else return (v+1)/Size - 1;
}

int World::int3::cellCoord(int v)
{
        if(v >= 0) return v%Size;
        else return Size - (-v-1)%Size - 1;
}

World::int3::int3(int _x, int _y, int _z) :
	x(_x), y(_y), z(_z)
{
	
}

World::int3::int3(const vec3 &v) :
	x(int(std::floor(v.x))),
	y(int(std::floor(v.y))),
	z(int(std::floor(v.z)))
{
	
}

World::int3 World::int3::block(void) const
{
	return int3(blockCoord(x), blockCoord(y), blockCoord(z));
}

World::int3 World::int3::cell(void) const
{
	return int3(cellCoord(x), cellCoord(y), cellCoord(z));
}

bool World::int3::operator==(const int3 &i) const
{
	return (x == i.x && y == i.y && z == i.z);
}

bool World::int3::operator<(const int3 &i) const
{
	return (x < i.x || (x == i.x && (y < i.y || (y == i.y && z < i.z))));
}

bool World::int3::operator>(const int3 &i) const
{
	return (x > i.x || (x == i.x && (y > i.y || (y == i.y && z > i.z))));
}

World::Block::Block(World *grid, const int3 &b) :
	mWorld(grid),
	mPos(b)
{
	for(int l=0; l<LayersCount; ++l)
		std::fill(mValues[l], mValues[l] + Size*Size*Size, -1.f);
	
	mChanged = true;
}

World::Block::~Block(void)
{
	
}

vec3 World::Block::center(void) const
{
	return vec3(	(float(mPos.x) + 0.5f)*Size,
			(float(mPos.y) + 0.5f)*Size,
			(float(mPos.z) + 0.5f)*Size);
}

void World::Block::computeGradients(void)
{
	for(int x = 0; x < Size; ++x)
		for(int y = 0; y < Size; ++y)
			for(int z = 0; z < Size; ++z)
			{
				int3 p(x, y, z);
				setGrad(p, computeGradient(p));
			}
}

vec3 World::Block::computeGradient(const int3 &p)
{
	const int l = 0;
	return vec3(
		(value(int3(p.x-1,p.y,p.z),l) - value(int3(p.x+1,p.y,p.z),l))*0.5f,
		(value(int3(p.x,p.y-1,p.z),l) - value(int3(p.x,p.y+1,p.z),l))*0.5f,
		(value(int3(p.x,p.y,p.z-1),l) - value(int3(p.x,p.y,p.z+1),l))*0.5f
	);
}

int World::Block::update(void)
{
	const float level = 0.f;
	
	if(!mChanged)
		return indicesCount()/3;
	
	computeGradients();
		
	std::vector<vec3> vertices;
	std::vector<vec3> normals;
	std::vector<float> environment;
	std::vector<index_t> indices;
		
	vertices.reserve(indicesCount()*2);
	indices.reserve(vertexAttribCount()*2);
	
	for(int x = 0; x < Size; ++x)
		for(int y = 0; y < Size; ++y)
			for(int z = 0; z < Size; ++z)
				polygonizeCell(int3(x, y, z), level, vertices, normals, environment, indices);
	
	setIndices(&indices[0], indices.size());
	setVertexAttrib(0, glm::value_ptr(vertices[0]), vertices.size()*3, 3);
	setVertexAttrib(1, glm::value_ptr(normals[0]), normals.size()*3, 3);
	setVertexAttrib(2, &environment[0], environment.size(), 1);
	
	mChanged = false;
	return indicesCount()/3;
}

// Given a grid cell and an level, calculate the triangular
// facets required to represent the isosurface through the cell.
// Fill vertices and indices and return the number of triangular 
// facets 0 will be returned if the grid cell is either totally
// above of totally below the level.
int World::Block::polygonizeCell(const int3 &c, float level,
	std::vector<vec3> &vertices, std::vector<vec3> &normals, std::vector<float> &environment,
	std::vector<index_t> &indices)
{
	vec3 center = vec3(	float(mPos.x*Size+c.x),
				float(mPos.y*Size+c.y),
				float(mPos.z*Size+c.z));	// cell center
	
	vec3 p[8];	// grid vertices
	p[0] = center + vec3(-.5f, -.5f, -.5f);
	p[1] = center + vec3( .5f, -.5f, -.5f);
	p[2] = center + vec3( .5f,  .5f, -.5f);
	p[3] = center + vec3(-.5f,  .5f, -.5f);
	p[4] = center + vec3(-.5f, -.5f,  .5f);
	p[5] = center + vec3( .5f, -.5f,  .5f);
	p[6] = center + vec3( .5f,  .5f,  .5f);
	p[7] = center + vec3(-.5f,  .5f,  .5f);
	
	float v[8];	// grid values
	v[0] = value(int3(c.x-1, c.y-1, c.z-1), 0);
	v[1] = value(int3(c.x  , c.y-1, c.z-1), 0);
	v[2] = value(int3(c.x  , c.y  , c.z-1), 0);
	v[3] = value(int3(c.x-1, c.y  , c.z-1), 0);
	v[4] = value(int3(c.x-1, c.y-1, c.z  ), 0);
	v[5] = value(int3(c.x  , c.y-1, c.z  ), 0);
	v[6] = value(int3(c.x  , c.y  , c.z  ), 0);
	v[7] = value(int3(c.x-1, c.y  , c.z  ), 0);
	
	vec3 g[8];	// grid normals
	g[0] = grad(int3(c.x-1, c.y-1, c.z-1));
	g[1] = grad(int3(c.x  , c.y-1, c.z-1));
	g[2] = grad(int3(c.x  , c.y  , c.z-1));
	g[3] = grad(int3(c.x-1, c.y  , c.z-1));
	g[4] = grad(int3(c.x-1, c.y-1, c.z  ));
	g[5] = grad(int3(c.x  , c.y-1, c.z  ));
	g[6] = grad(int3(c.x  , c.y  , c.z  ));
	g[7] = grad(int3(c.x-1, c.y  , c.z  ));
	
	float e[8];	// env values
	e[0] = value(int3(c.x-1, c.y-1, c.z-1), 1);
	e[1] = value(int3(c.x  , c.y-1, c.z-1), 1);
	e[2] = value(int3(c.x  , c.y  , c.z-1), 1);
	e[3] = value(int3(c.x-1, c.y  , c.z-1), 1);
	e[4] = value(int3(c.x-1, c.y-1, c.z  ), 1);
	e[5] = value(int3(c.x  , c.y-1, c.z  ), 1);
	e[6] = value(int3(c.x  , c.y  , c.z  ), 1);
	e[7] = value(int3(c.x-1, c.y  , c.z  ), 1);
	
	// Determine the index into the edge table which
	// tells us which vertices are inside the surface
	index_t index = 0;
	if(v[0] < level) index|= 1;
	if(v[1] < level) index|= 2;
	if(v[2] < level) index|= 4;
	if(v[3] < level) index|= 8;
	if(v[4] < level) index|= 16;
	if(v[5] < level) index|= 32;
	if(v[6] < level) index|= 64;
	if(v[7] < level) index|= 128;

	int edge = EdgeTable[index];
	
	// Cube is entirely in/out of the surface
	if(edge == 0)
		return 0;

	// Find the vertices where the surface intersects the cube
	vec3 vert[12];
	if(edge & 1)    vert[0]  = interpolate(level, p[0], p[1], v[0], v[1]);
	if(edge & 2)    vert[1]  = interpolate(level, p[1], p[2], v[1], v[2]);
	if(edge & 4)    vert[2]  = interpolate(level, p[2], p[3], v[2], v[3]);
	if(edge & 8)    vert[3]  = interpolate(level, p[3], p[0], v[3], v[0]);
	if(edge & 16)   vert[4]  = interpolate(level, p[4], p[5], v[4], v[5]);
	if(edge & 32)   vert[5]  = interpolate(level, p[5], p[6], v[5], v[6]);
	if(edge & 64)   vert[6]  = interpolate(level, p[6], p[7], v[6], v[7]);
	if(edge & 128)  vert[7]  = interpolate(level, p[7], p[4], v[7], v[4]);
	if(edge & 256)  vert[8]  = interpolate(level, p[0], p[4], v[0], v[4]);
	if(edge & 512)  vert[9]  = interpolate(level, p[1], p[5], v[1], v[5]);
	if(edge & 1024) vert[10] = interpolate(level, p[2], p[6], v[2], v[6]);
	if(edge & 2048) vert[11] = interpolate(level, p[3], p[7], v[3], v[7]);
	
	// Find the associated normals
	vec3 norm[12];
	if(edge & 1)    norm[0]  = interpolate(level, g[0], g[1], v[0], v[1]);
	if(edge & 2)    norm[1]  = interpolate(level, g[1], g[2], v[1], v[2]);
	if(edge & 4)    norm[2]  = interpolate(level, g[2], g[3], v[2], v[3]);
	if(edge & 8)    norm[3]  = interpolate(level, g[3], g[0], v[3], v[0]);
	if(edge & 16)   norm[4]  = interpolate(level, g[4], g[5], v[4], v[5]);
	if(edge & 32)   norm[5]  = interpolate(level, g[5], g[6], v[5], v[6]);
	if(edge & 64)   norm[6]  = interpolate(level, g[6], g[7], v[6], v[7]);
	if(edge & 128)  norm[7]  = interpolate(level, g[7], g[4], v[7], v[4]);
	if(edge & 256)  norm[8]  = interpolate(level, g[0], g[4], v[0], v[4]);
	if(edge & 512)  norm[9]  = interpolate(level, g[1], g[5], v[1], v[5]);
	if(edge & 1024) norm[10] = interpolate(level, g[2], g[6], v[2], v[6]);
	if(edge & 2048) norm[11] = interpolate(level, g[3], g[7], v[3], v[7]);
	
	// Find the associated env values
	float env[12];
	if(edge & 1)    env[0]  = interpolate(level, e[0], e[1], v[0], v[1]);
	if(edge & 2)    env[1]  = interpolate(level, e[1], e[2], v[1], v[2]);
	if(edge & 4)    env[2]  = interpolate(level, e[2], e[3], v[2], v[3]);
	if(edge & 8)    env[3]  = interpolate(level, e[3], e[0], v[3], v[0]);
	if(edge & 16)   env[4]  = interpolate(level, e[4], e[5], v[4], v[5]);
	if(edge & 32)   env[5]  = interpolate(level, e[5], e[6], v[5], v[6]);
	if(edge & 64)   env[6]  = interpolate(level, e[6], e[7], v[6], v[7]);
	if(edge & 128)  env[7]  = interpolate(level, e[7], e[4], v[7], v[4]);
	if(edge & 256)  env[8]  = interpolate(level, e[0], e[4], v[0], v[4]);
	if(edge & 512)  env[9]  = interpolate(level, e[1], e[5], v[1], v[5]);
	if(edge & 1024) env[10] = interpolate(level, e[2], e[6], v[2], v[6]);
	if(edge & 2048) env[11] = interpolate(level, e[3], e[7], v[3], v[7]);
	
	// Create the triangles
	int n = 0;
	std::map<int, index_t> mapping;
	for(int i = 0; TriTable[index][i] >= 0; i+= 3)	// for each triangle
	{
		for(int j = 0; j < 3; ++j)	// for each vertex
		{
			int vi = TriTable[index][i+j];
			auto it = mapping.find(vi);
			if(it == mapping.end())
			{
				index_t mi = index_t(vertices.size());
				vertices.push_back(vert[vi]);
				normals.push_back(glm::normalize(norm[vi]));
				environment.push_back(env[vi]);
				indices.push_back(mi);
				mapping[vi] = mi;
			}
			else {
				indices.push_back(it->second);
			}
		}
		
		++n;
	}
	
	return n;
}

// Linearly interpolate the position where an isosurface cuts
// an edge between two vertices, each with their own scalar value
vec3 World::Block::interpolate(float level, vec3 p1, vec3 p2, float v1, float v2)
{
	static auto comp = [](const vec3 &a, const vec3 &b)
	{
		if (a.x < b.y) return true;
		else if (a.x > b.x) return false;
		if (a.y < b.y) return true;
		else if (a.y > b.y) return false;
		if (a.z < b.z) return true;
		else return false;
	};
	
	if(comp(p1, p2))
	{
		std::swap(p1, p2);
		std::swap(v1, v2);
	}
	
	if(std::abs(v1 - v2) < std::numeric_limits<float>::epsilon())
		return p1;
	
	float mu = (level - v1) / (v2 - v1);
	return p1 + mu * (p2 - p1);
}

float World::Block::interpolate(float level, float p1, float p2, float v1, float v2)
{
	if(std::abs(v1 - v2) < std::numeric_limits<float>::epsilon())
		return p1;
	
	float mu = (level - v1) / (v2 - v1);
	return p1 + mu * (p2 - p1);
}

float World::Block::value(const int3 &p, int layer)
{
	if(layer < 0.f || layer >= LayersCount) return 0.f;
	if(p.x < 0) return mWorld->getBlock(int3(mPos.x-1, mPos.y, mPos.z))->value(int3(p.x+Size, p.y, p.z), layer);
	if(p.y < 0) return mWorld->getBlock(int3(mPos.x, mPos.y-1, mPos.z))->value(int3(p.x, p.y+Size, p.z), layer);
	if(p.z < 0) return mWorld->getBlock(int3(mPos.x, mPos.y, mPos.z-1))->value(int3(p.x, p.y, p.z+Size), layer);
	if(p.x >= Size) return mWorld->getBlock(int3(mPos.x+1, mPos.y, mPos.z))->value(int3(p.x-Size, p.y, p.z), layer);
	if(p.y >= Size) return mWorld->getBlock(int3(mPos.x, mPos.y+1, mPos.z))->value(int3(p.x, p.y-Size, p.z), layer);
	if(p.z >= Size) return mWorld->getBlock(int3(mPos.x, mPos.y, mPos.z+1))->value(int3(p.x, p.y, p.z-Size), layer);
	
	return mValues[layer][(p.x*Size + p.y)*Size + p.z];
}

void World::Block::setValue(const int3 &p, float v, int layer)
{
	if(layer >= 0 && layer < LayersCount && p.x >= 0 && p.y >= 0 && p.z >= 0 && p.x < Size && p.y < Size && p.z < Size)
	{
		mValues[layer][(p.x*Size + p.y)*Size + p.z] = v;
		mChanged = true;
		
		if(layer == 0)
		{
			// Mark corresponding blocks as changed
			for(int dx=-1; dx<=1; ++dx)
				for(int dy=-1; dy<=1; ++dy)
					for(int dz=-1; dz<=1; ++dz)
					{
						if(dx == 0 && dy == 0 && dz == 0) continue;
						if(p.x != 0 && dx == -1) continue;
						if(p.y != 0 && dy == -1) continue;
						if(p.z != 0 && dz == -1) continue;
						if(p.x != Size-1 && dx == 1) continue;
						if(p.y != Size-1 && dy == 1) continue;
						if(p.z != Size-1 && dz == 1) continue;
						mWorld->changedBlock(int3(mPos.x+dx, mPos.y+dy, mPos.z+dz));
					}
		}
	}
}

vec3 World::Block::grad(const int3 &p)
{
	if(p.x < 0) return mWorld->getBlock(int3(mPos.x-1, mPos.y, mPos.z))->grad(int3(p.x+Size, p.y, p.z));
	if(p.y < 0) return mWorld->getBlock(int3(mPos.x, mPos.y-1, mPos.z))->grad(int3(p.x, p.y+Size, p.z));
	if(p.z < 0) return mWorld->getBlock(int3(mPos.x, mPos.y, mPos.z-1))->grad(int3(p.x, p.y, p.z+Size));
	if(p.x >= Size) return mWorld->getBlock(int3(mPos.x+1, mPos.y, mPos.z))->grad(int3(p.x-Size, p.y, p.z));
	if(p.y >= Size) return mWorld->getBlock(int3(mPos.x, mPos.y+1, mPos.z))->grad(int3(p.x, p.y-Size, p.z));
	if(p.z >= Size) return mWorld->getBlock(int3(mPos.x, mPos.y, mPos.z+1))->grad(int3(p.x, p.y, p.z-Size));
	
	if(!mChanged) return mGrads[(p.x*Size + p.y)*Size + p.z];
	else return computeGradient(p);
}

void World::Block::setGrad(const int3 &p, const vec3 &g)
{
	if(p.x >= 0 && p.y >= 0 && p.z >= 0 && p.x < Size && p.y < Size && p.z < Size)
		mGrads[(p.x*Size + p.y)*Size + p.z] = g;
}

// Vertices order is:
// {0.0, 0.0, 0.0},{1.0, 0.0, 0.0},{1.0, 1.0, 0.0},{0.0, 1.0, 0.0},
// {0.0, 0.0, 1.0},{1.0, 0.0, 1.0},{1.0, 1.0, 1.0},{0.0, 1.0, 1.0}

int World::EdgeTable[256] = {
	0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
	0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
	0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
	0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
	0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
	0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
	0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
	0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
	0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
	0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
	0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
	0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
	0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
	0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
	0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
	0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
	0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
	0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
	0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
	0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
	0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
	0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
	0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
	0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
	0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
	0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
	0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
	0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
	0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
	0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
	0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
	0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0};
int World::TriTable[256][16] =
	{{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
	{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
	{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
	{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
	{9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
	{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
	{8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
	{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
	{3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
	{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
	{4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
	{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
	{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
	{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
	{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
	{0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
	{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
	{10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
	{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
	{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
	{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
	{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
	{10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
	{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
	{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
	{7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
	{2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
	{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
	{5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
	{11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
	{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
	{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
	{2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
	{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
	{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
	{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
	{10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
	{6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
	{8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
	{7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
	{3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
	{0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
	{9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
	{8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
	{5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
	{0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
	{6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
	{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
	{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
	{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
	{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
	{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
	{10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
	{3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
	{6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
	{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
	{8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
	{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
	{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
	{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
	{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
	{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
	{2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
	{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
	{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
	{2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
	{1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
	{11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
	{8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
	{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
	{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
	{10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
	{2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
	{6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
	{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
	{2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
	{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
	{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
	{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
	{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
	{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
	{8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
	{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
	{6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
	{4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
	{10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
	{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
	{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
	{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
	{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
	{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
	{4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
	{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
	{11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
	{9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
	{6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
	{7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
	{3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
	{7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
	{3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
	{6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
	{9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
	{1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
	{4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
	{7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
	{6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
	{3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
	{0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
	{6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
	{0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
	{11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
	{6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
	{5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
	{9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
	{1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
	{1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
	{10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
	{0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
	{5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
	{10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
	{11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
	{9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
	{7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
	{2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
	{8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
	{9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
	{9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
	{1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
	{9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
	{5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
	{0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
	{10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
	{2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
	{0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
	{0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
	{9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
	{5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
	{3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
	{5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
	{8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
	{0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
	{9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
	{1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
	{3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
	{4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
	{9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
	{11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
	{11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
	{2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
	{9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
	{3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
	{1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
	{4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
	{3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
	{0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
	{1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

}
