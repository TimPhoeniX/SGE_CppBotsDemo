#pragma once
#include "CellSpacePartition.hpp"
#include "RavenBot.hpp"
#include "Wall.hpp"
#include "Utilities.hpp"
#include "Objects.hpp"

namespace
{
	constexpr size_t partitionX = 8u;
	constexpr size_t partitionY = 8u;
}

class World
{
protected:
	CellSpacePartition<RavenBot, partitionX, partitionY> movers;
	CellSpacePartition<SGE::Object, partitionX, partitionY> obstacles;
	CellSpacePartition<Rocket, partitionX, partitionY> rockets;
	CellSpacePartition<Item, partitionX, partitionY> items;
	std::vector<std::pair<SGE::Object*, Edge>> walls;
	const float width, height, cellWidth, cellHeight;
public:
	class Ray
	{
	protected:
		b2Vec2 from, direction;
		float width, height, cellWidth, cellHeight;
	public:
		Ray(b2Vec2 from, b2Vec2 direction, float width, float height, float cellWidth, float cellHeight);
		class RayIterator
		{
			const float tDeltaX = 0.f , tDeltaY = 0.f;
			const int deltaX = 0u, deltaY = 0u;
			float tMaxX = 0.f, tMaxY = 0.f;
			int X = 0u, Y = 0u;
		public:
			constexpr RayIterator() = default;
			constexpr RayIterator(float max_x, float max_y, float delta_x, float delta_y,
								  int delta_x1, int delta_y1, int x, int y);

			RayIterator& operator++();
			size_t operator*() const;
			bool operator==(const RayIterator& other) const;
			bool operator!=(const RayIterator& other) const;
		};
		RayIterator begin() const;
		RayIterator end() const;
	};
	World(float width, float height);

	std::vector<SGE::Object*> getObstacles(RavenBot* const mover, float radius);
	std::vector<SGE::Object*> getObstacles(RavenBot* const mover);
	std::vector<SGE::Object*> getObstacles(b2Vec2 position, float radius);

	void getNeighbours(std::vector<RavenBot*>& res, RavenBot* const mover);
	void getNeighbours(std::vector<RavenBot*>& res, RavenBot* const mover, float radius);
	void getNeighbours(std::vector<RavenBot*>& res, b2Vec2 position, float radius);

	std::vector<std::pair<SGE::Object*, Edge>>& getWalls();

	void AddMover(RavenBot* mo);
	void AddObstacle(SGE::Object* ob);
	void AddItem(Item* i);
	void AddRocket(Rocket* r);

	void RemoveObstacle(SGE::Object* ob);
	void RemoveItem(Item* i);
	void RemoveRocket(Rocket* r);

	void UpdateObstacle(SGE::Object* rocket, b2Vec2 oldPos);
	void UpdateRocket(Rocket* rocket, b2Vec2 oldPos);
	void UpdateMover(RavenBot* mo, b2Vec2 oldPos);

	void AddWall(SGE::Object* wall, Wall::WallEdge edge);

	void clear();

	template<typename T>
	T* getHit(b2Vec2 from, b2Vec2 direction, b2Vec2& hit, const std::vector<T*>& entities, T* ignore = nullptr) const
	{
		T* closestObject = nullptr;
		float closestDist = std::numeric_limits<float>::max();
		for(T* ob : entities)
		{
			if(ob == ignore) continue;
			if(ob->getShape()->getType() != SGE::ShapeType::Quad)
			{
				b2Vec2 localPos = PointToLocalSpace(ob->getPosition(), direction, from);
				float obRadius = ob->getShape()->getRadius();
				if(localPos.x >= 0.f)
				{
					if(b2Abs(localPos.y) < obRadius)
					{
						float cX = localPos.x, cY = localPos.y;
						float sqrtPart = sqrt(obRadius*obRadius - cY*cY);
						float ip = cX - sqrtPart;
						if(ip <= 0.f) ip = cX + sqrtPart;
						if(ip < closestDist)
						{
							closestDist = ip;
							closestObject = ob;
							hit = PointToWorldSpace(b2Vec2{ip,0.f}, direction, from);
						}
					}
				}
			}
			else
			{
				QuadObstacle* qob = reinterpret_cast<QuadObstacle*>(ob);
				if(!qob) continue;
				float ip;
				b2Vec2 point;
				for(auto& wall : qob->getEdges())
				{
					if(LineIntersection(from, from + 100.f * direction, wall.From(), wall.To(), ip, point))
					{
						if(ip < closestDist)
						{
							closestDist = ip;
							closestObject = ob;
							hit = point;
						}
					}
				}
			}
		}
		return closestObject;
	}

	RavenBot* RaycastBot(RavenBot* caster, b2Vec2 from, b2Vec2 direction, b2Vec2& hit) const;
	Item* RaycastItem(b2Vec2 from, b2Vec2 direction, b2Vec2& hit) const;
	void RemoveMover(RavenBot* hitObject);
	std::vector<Item*> getItems(RavenBot* const mover);
	std::vector<Rocket*> getRockets(const b2Vec2& position, float radius);
};
