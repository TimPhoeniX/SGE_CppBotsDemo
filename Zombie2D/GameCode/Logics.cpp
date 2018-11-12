﻿#include "Logics.hpp"
#include "Actions.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <Action/Actions/sge_action_move.hpp>
#include <IO/KeyboardState/sge_keyboard_state.hpp>
#include <Utils/Timing/sge_fps_limiter.hpp>
#include <Game/Director/sge_director.hpp>

#include "ZombieScene.hpp"
#include "Utilities.hpp"

void DamagePlayer::performLogic()
{
	this->movers.clear();
	this->world->getNeighbours(this->movers, this->player->getPosition(), 2.f * this->player->getShape()->getRadius());
	for (MovingObject* mover : this->movers)
	{
		float dist = b2DistanceSquared(this->player->getPosition(), mover->getPosition());
		float radii = this->player->getShape()->getRadius() + mover->getShape()->getRadius();
		radii *= radii;
		if(dist < radii)
		{
			this->player->Damage(SGE::delta_time * this->dps);
		}
	}
}

void MoveAwayFromObstacle::performLogic()
{
	for(SGE::WorldElement& w: *this->obstacles)
	{
		SGE::Shape obShape = *w.getShape();
		switch(obShape.getType())
		{
		case SGE::ShapeType::Circle:
		{
			this->movers.clear();
			this->world->getNeighbours(this->movers, w.getPosition(), obShape.getRadius() + 1.f);		
			if(this->movers.empty()) continue;
			for(MovingObject* mo : this->movers)
			{
				SGE::Shape moShape = *mo->getShape();
				b2Vec2 toMover = mo->getPosition() - w.getPosition();
				float dist = toMover.Length();
				float radius = moShape.getRadius() + obShape.getRadius();
				if(dist < radius)
				{
					toMover *= (radius - dist) / dist;
					mo->setPosition(mo->getPosition() + toMover);
				}
			}
			break;
		}
		case SGE::ShapeType::Rectangle:
		{
			break;
		}
		case SGE::ShapeType::None: break;
		default: ;
		}
	}
	auto ob = this->world->getObstacles(this->player, 4.f*this->player->getShape()->getRadius());
	for(SGE::Object* o : ob)
	{
		SGE::Shape obShape = *o->getShape();
		switch(obShape.getType())
		{
		case SGE::ShapeType::Circle:
		{
			SGE::Shape moShape = *this->player->getShape();
			b2Vec2 toMover = this->player->getPosition() - o->getPosition();
			float dist = toMover.Length();
			float radius = moShape.getRadius() + obShape.getRadius();
			if(dist < radius)
			{
				toMover *= (radius - dist) / dist;
				this->player->setPosition(this->player->getPosition() + toMover);
			}
			break;
		}
		case SGE::ShapeType::Rectangle:
		{
			break;
		}
		case SGE::ShapeType::None: break;
		default:;
		}
	}
}

void MoveAwayFromWall::performLogic()
{
	for(MovingObject& mo: this->movers)
	{
		for (std::pair<SGE::Object*, Wall>& wall: this->world->getWalls())
		{
			if(PointToLineDistance(mo.getPosition(), wall.second.From(), wall.second.To()) < mo.getShape()->getRadius())
			{
				float dist;
				b2Vec2 intersect;
				switch (wall.second.Type())
				{
				case Wall::Left: break;
				case Wall::Right: break;
				case Wall::Top: break;
				case Wall::Bottom: break;
				default:break;
				}
			}
		}
	}
}

SnapCamera::SnapCamera(const float speed, const SGE::Key up, const SGE::Key down, const SGE::Key left, const SGE::Key right, const SGE::Key snapKey, SGE::Object* snapTo, SGE::Camera2d* cam): Logic(SGE::LogicPriority::Highest), speed(speed), up(up), down(down), left(left), right(right), snapKey(snapKey), snapTo(snapTo), cam(cam)
{}

void SnapCamera::performLogic()
{
	this->snapped = SGE::isPressed(snapKey); //We need to be able to send signals to actions, like sending actions to objects
	glm::vec2 move = {0, 0};
	if(!this->snapped)
	{
		move = this->snapTo->getPositionGLM();
		this->cam->setPositionGLM(move.x, move.y); //Replace with action, i.e. GoTo
	}
	else
	{
		if(SGE::isPressed(this->up)) move.y += this->speed;
		if(SGE::isPressed(this->down)) move.y -= this->speed;
		if(SGE::isPressed(this->right)) move.x += this->speed;
		if(SGE::isPressed(this->left)) move.x -= this->speed;
		this->sendAction(new SGE::ACTION::Move(this->cam, move.x, move.y, true));
	}
}

Timer::Timer(float time, SGE::Action* action): Logic(SGE::LogicPriority::Low), time(time), action(action)
{}

void Timer::performLogic()
{
	if(this->time > .0f)
	{
		this->time -= SGE::delta_time;
	}
	else
	{
		this->isOn = false;
		this->sendAction(this->action);
	}
}

OnKey::OnKey(SGE::Key key, SGE::Scene* scene): Logic(SGE::LogicPriority::Low), key(key), scene(scene)
{}

void OnKey::performLogic()
{
	if(SGE::isPressed(this->key))
	{
		this->sendAction(new Load(scene));
	}
}

Aim::Aim(World* world, SGE::Object* aimer, SGE::MouseObject* mouse, SGE::Camera2d* cam, std::size_t& counter)
	: Logic(SGE::LogicPriority::High), world(world), aimer(aimer), mouse(mouse), cam(cam), counter(counter)
{
}

bool Aim::aim(b2Vec2 pos, b2Vec2 direction)
{
	b2Vec2 hitPos = b2Vec2_zero;
	MovingObject* hitObject = this->world->Raycast(pos, direction, hitPos);
	//Draw railbeam
	if(hitObject)
	{
		hitObject->setTexture(ZombieScene::deadZombieTexture);
		hitObject->killed = true;
		this->world->RemoveMover(hitObject);
	}
	return bool(hitObject);
}

void Aim::performLogic()
{
	if(reload > 0.f)
	{
		reload -= SGE::delta_time;
	}
	if(this->fired)
	{
		this->fired = false;
		auto dir = this->cam->screenToWorld(this->mouse->getMouseCoords()) - this->aimer->getPositionGLM();
		b2Vec2 direction{dir.x, dir.y};
		direction.Normalize();
		this->aimer->setOrientation(direction.Orientation());
		b2Vec2 pos = this->aimer->getPosition();
		aim(pos, direction);
	}
}

void Aim::Shoot()
{
	if (!this->fired && this->reload < 0)
	{
		this->fired = true;
		this->reload = 3.f;
	}
}

WinCondition::WinCondition(size_t& zombies, size_t& killedZombies, SGE::Scene* endGame, Player* player)
	: Logic(SGE::LogicPriority::Low), zombies(zombies), killedZombies(killedZombies), endGame(endGame), player(player)
{}

void WinCondition::performLogic()
{
	if(zombies == killedZombies || this->player->getHP() < 0)
	{
		this->sendAction(new Load(endGame));
	}
}
