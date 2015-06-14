/*
 * ElevatorLogic.h
 *
 *  Created on: 20.06.2014
 *      Author: STSJR
 */

#ifndef ELEVATORLOGIC_H_
#define ELEVATORLOGIC_H_

#define FLOATING_EPSILON 1.f

#include "EventHandler.h"

#include <list>
#include <map>
#include <set>

#define VERBOSE false

class Elevator;
class Floor;
class Interface;
class Person;

class ElevatorLogic: public EventHandler {

public:
	ElevatorLogic();
	virtual ~ElevatorLogic();

	void Initialize(Environment &env);

private:
	enum TARGET_TYPE
	{
		DESTINATION,
		FETCH
	};

	typedef std::pair < Floor*, std::pair<TARGET_TYPE,Person*>> Floor_Pair;

	struct EleInfo
	{
		int load = 0;
		bool overloaded = false;
		bool open = false;
		bool malfunctioning = false;
		std::list<Floor_Pair> targetFloors;
		int	stop_ID = -1;
		int moving_ID = -1;
		int closing_ID = -1;
	};


	void HandleInterfaceNotify(Environment &env, const Event &e);
	void HandleStopped(Environment &env, const Event &e);
	void HandleOpened(Environment &env, const Event &e);
	void HandleClosed(Environment &env, const Event &e);
	void HandleOpening(Environment &env, const Event &e);
	void HandleMalfunction(Environment &env, const Event &e);
	void HandleFixed(Environment &env, const Event &e);
	void HandleEntering(Environment &env, const Event &e);
	void HandleEntered(Environment &env, const Event &e);
	void HandleExited(Environment &env, const Event &e);
	void HandleMoving(Environment &env, const Event &e);
	
	bool isMiddle(double pos) { return (pos > 0.5f - FLOATING_EPSILON && pos < 0.5f + FLOATING_EPSILON); }

	
	std::map<Elevator*, EleInfo> eleInfos;

	std::map<Elevator*, EleInfo>::iterator registerElevator(Elevator* ele)
	{
		if (eleInfos.find(ele) == eleInfos.end())
		{
			return eleInfos.insert(std::pair<Elevator*, EleInfo>(ele, EleInfo())).first;
		}
		else
			return eleInfos.find(ele);
	}

	void sendToFloor(Elevator* ele, Floor* floor, Environment& env, int delay = 0);
	void insertTarget(Elevator* ele, Floor* target, std::list<Floor_Pair>* list, std::pair<TARGET_TYPE, Person*> type, Environment& env);
	
	void removeAllAbove(Elevator* ele, Floor* target);
	void removeAllBelow(Elevator* ele, Floor* target);

	Floor* getHighestFloor(Elevator* ele);
	Floor* getLowestFloor(Elevator* ele);
};

#endif /* ELEVATORLOGIC_H_ */
