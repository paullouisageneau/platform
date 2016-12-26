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

#ifndef DEMO_GRID_H
#define DEMO_GRID_H

#include "p3d/include.hpp"
#include "p3d/mesh.hpp"
#include "p3d/object.hpp"
#include "p3d/collidable.hpp"
#include "p3d/context.hpp"
#include "p3d/program.hpp"
#include "p3d/shader.hpp"
#include "p3d/perlinnoise.hpp"

using pla::vec3;
using pla::Mesh;
using pla::Object;
using pla::Context;
using pla::Collidable;
using pla::Program;
using pla::VertexShader;
using pla::FragmentShader;
using pla::PerlinNoise;
template<typename T> using sptr = std::shared_ptr<T>;

namespace demo
{

class World : public Collidable
{
public:
	struct int3
	{
		static int blockCoord(int v);
		static int cellCoord(int v);
		
		int3(int _x = 0, int _y = 0, int _z = 0);
		int3(const vec3 &v);
		
		int3 block(void) const;
		int3 cell(void) const;
		
		bool operator==(const int3 &i) const;
		bool operator<(const int3 &i) const;
		bool operator>(const int3 &i) const;
		
		int x, y, z;
	};
	
	World(unsigned int seed);
	~World(void);
	
	int draw(const Context &context);
	float intersect(const vec3 &pos, const vec3 &move, float radius, vec3 *intersection = NULL);
	
	void setValue(const int3 &p, float v, int layer = 0);
	void setValue(const vec3 &p, float v, int layer = 0);
	float value(const int3 &p, int layer = 0);
	float value(const vec3 &p, int layer = 0);
	
protected:
	static const int Size = 8;
	static const int LayersCount = 2;
	static int EdgeTable[256];
	static int TriTable[256][16];
	
	class Block : public Mesh
	{
	public:
		Block(World *grid, const int3 &b);
		~Block(void);
		
		vec3 center(void) const;
		
		void computeGradients(void);
		vec3 computeGradient(const int3 &p);
		
		int update(void);
		
		void setValue(const int3 &p, float v, int layer = 0);
		float value(const int3 &p, int layer = 0);
		
		void setGrad(const int3 &p, const vec3 &g);
		vec3 grad(const int3 &p);
		
	private:
		
		int polygonizeCell(const int3 &c, float level, 
			std::vector<vec3> &vertices, std::vector<vec3> &normals, std::vector<float> &environment,
			std::vector<index_t> &indices);
		vec3 interpolate(float level, vec3 p1, vec3 p2, float v1, float v2);
		float interpolate(float level, float p1, float p2, float v1, float v2);
		
		World *mWorld;
		int3 mPos;
		
		float mValues[LayersCount][Size*Size*Size];
		vec3 mGrads[Size*Size*Size];
		bool mChanged;
		
		std::map<int3, sptr<Object> > mObjects;
		
		friend class World;
	};
	
	void changedBlock(const int3 &b);
	sptr<Block> getBlock(const int3 &b);
	void getBlocksRec(const int3 &b, std::set<sptr<Block> > &result, std::set<sptr<Block> > &processed, std::function<bool(sptr<Block>)> check);
	void populateBlock(sptr<Block> block);
	
	std::map<int3, sptr<Block> > mBlocks;
	PerlinNoise mPerlin;
	sptr<Program> mProgram;
};
	
}

#endif

