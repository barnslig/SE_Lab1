/*
 * ElevatorLogic.cpp
 *
 *  Created on: 20.06.2014
 *      Author: STSJR
 */

#include "ElevatorLogic.h"

#include <iostream>
#include <cmath>

#include "Interface.h"
#include "Person.h"
#include "Floor.h"
#include "Elevator.h"
#include "Event.h"
#include "Environment.h"

// Returns the Distance of Floor1 at the given Position to Floor2 in the middle
int getDistance(Floor* f1, Floor* f2, double pos = 0.5)
{
	double d = f1->GetHeight() * pos;
	Floor *dummy = f1;

	if (f1 == f2)
	{
		d = (f1->GetHeight() / 2) - d;
		
	}else if (f1->IsBelow(f2))
	{
		d = f1->GetHeight() - d;
		dummy = f1->GetBelow();
		while (dummy != f2)
		{
			d += dummy->GetHeight();
			dummy = dummy->GetBelow();
		}
		d += f2->GetHeight() / 2;
		d = -d;
	}
	else {
		dummy = f1->GetAbove();
		while (dummy != f2)
		{
			d += dummy->GetHeight();
			dummy = dummy->GetAbove();
		}
		d += f2->GetHeight() / 2;
	}
	
	return std::ceil(d);
}

// Returns the effective Distance (Distance divided by Speed)
double getEffectiveDist(Elevator* ele, Floor* flr1, Floor* flr2)
{
	return (getDistance(flr1, flr2, ele->GetPosition()) / static_cast<double>(ele->GetSpeed()));
}

// Sends a Elevator to a given Floor
void ElevatorLogic::sendToFloor(Elevator* ele, Floor* floor, Environment& env, int delay)
{
	std::map<Elevator*, EleInfo>::iterator it = registerElevator(ele);
	Floor* dummy;
	if (!it->second.overloaded && !it->second.malfunctioning && !it->second.open)
	{	
		if (!ele->HasFloor(floor))
			{
				if (VERBOSE)
					std::cout << "Elevator " << ele->GetId() << " Isn't able to reach Floor " << floor->GetId() << std::endl;

				if (ele->GetCurrentFloor()->IsAbove(floor))
				{
					dummy = floor->GetBelow();
					while (!ele->HasFloor(dummy))
					{
						dummy = floor->GetBelow();
					}
				}
				else {
					dummy = floor->GetAbove();
					while (!ele->HasFloor(dummy))
					{
						dummy = floor->GetAbove();
					}
				}
				if (VERBOSE)
					std::cout << "Instead gets send to " << dummy->GetId() << std::endl;

				floor = dummy;
			}

		int dist = getDistance(ele->GetCurrentFloor(), floor, ele->GetPosition());

		if (VERBOSE)
		{
			std::cout << "Send to Floor: " << floor->GetId() << std::endl;
			std::cout << "Distance: " << dist << std::endl;
			std::cout << "Time: " << dist / ele->GetSpeed() << std::endl;
		}

		
		if (dist > 0)
		{
			if (!it->second.open)
				it->second.moving_ID = env.SendEvent("Elevator::Up", 0, this, ele);
		}
		else if (dist < 0)
		{
			if (!it->second.open)
				it->second.moving_ID = env.SendEvent("Elevator::Down", 0, this, ele);
		}

		if (it->second.stop_ID != -1)
			env.CancelEvent(it->second.stop_ID);
		it->second.stop_ID = env.SendEvent("Elevator::Stop",  0 + (abs(dist) / ele->GetSpeed()), this, ele);
	}
}

// Removes all Queued Floors above the target Floor
void ElevatorLogic::removeAllAbove(Elevator* ele, Floor* target)
{
	std::list<Floor_Pair>*	list = &registerElevator(ele)->second.targetFloors;
	std::list<Floor_Pair>::iterator list_it = list->begin();
	while (list_it != list->end())
	{
		if (target->IsBelow(list_it->first))
			list_it = list->erase(list_it);
		else
			list_it++;
	}
}

// Removes all Queued Floors under the target Floor
void ElevatorLogic::removeAllBelow(Elevator* ele, Floor* target)
{
	std::list<Floor_Pair>*	list = &registerElevator(ele)->second.targetFloors;
	std::list<Floor_Pair>::iterator list_it = list->begin();
	while (list_it != list->end())
	{
		if (target->IsAbove(list_it->first))
			list_it = list->erase(list_it);
		else
			list_it++;
	}
}

Floor* ElevatorLogic::getHighestFloor(Elevator* ele)
{
	Floor* dummy = ele->GetCurrentFloor();
	while (dummy != NULL && !ele->IsHighestFloor(dummy))
	{
		dummy = dummy->GetAbove();
	}
	if (dummy != NULL)
		return dummy;
	dummy = ele->GetCurrentFloor();
	while (dummy != NULL && !ele->IsHighestFloor(dummy))
	{
		dummy = dummy->GetBelow();
	}
	return dummy;
}

Floor* ElevatorLogic::getLowestFloor(Elevator* ele)
{
	Floor* dummy = ele->GetCurrentFloor();
	while (dummy != NULL && !ele->IsLowestFloor(dummy))
	{
		dummy = dummy->GetBelow();
	}
	if (dummy != NULL)
		return dummy;
	dummy = ele->GetCurrentFloor();
	while (dummy != NULL && !ele->IsLowestFloor(dummy))
	{
		dummy = dummy->GetBelow();
	}
	return dummy;
}

// Inserts an elevator into the Target Queue 
void ElevatorLogic::insertTarget(Elevator* ele, Floor* target, std::list<Floor_Pair>* list, std::pair<TARGET_TYPE, Person*> type, Environment& env)
{
	int distance = abs(getDistance(ele->GetCurrentFloor(), target, ele->GetPosition()));

	std::list<Floor_Pair>::iterator it = list->begin();

	while (true)
	{
		if (it == list->end() || distance < getDistance(ele->GetCurrentFloor(), it->first, ele->GetPosition()))
		{
			if (VERBOSE)
			{
				std::cout << "Added Floor " << target->GetId() << " to Queue of Elevator" << ele->GetId() << std::endl;
				switch (type.first)
				{
				case DESTINATION:
					std::cout << "Destination Floor" << std::endl;
					break;
				case FETCH:
					std::cout << "Fetch Floor" << std::endl;
					break;
				default:
					std::cout << "WTF" << std::endl;
					break;
				}
			}

			list->insert(it, Floor_Pair(target,type));
			sendToFloor(ele, list->begin()->first, env, 0);
			break;
		}
		else
		{
			it++;
		}
	}
	
}

ElevatorLogic::ElevatorLogic() : EventHandler("ElevatorLogic"){
}

ElevatorLogic::~ElevatorLogic() {
}

void ElevatorLogic::Initialize(Environment &env) {
	env.RegisterEventHandler("Interface::Notify", this, &ElevatorLogic::HandleInterfaceNotify);
	env.RegisterEventHandler("Elevator::Stopped", this, &ElevatorLogic::HandleStopped);
	env.RegisterEventHandler("Elevator::Moving", this, &ElevatorLogic::HandleMoving);
	env.RegisterEventHandler("Elevator::Opened", this, &ElevatorLogic::HandleOpened);
	env.RegisterEventHandler("Elevator::Closed", this, &ElevatorLogic::HandleClosed);
	env.RegisterEventHandler("Elevator::Opening", this, &ElevatorLogic::HandleOpening);
	env.RegisterEventHandler("Elevator::Malfunction", this, &ElevatorLogic::HandleMalfunction);
	env.RegisterEventHandler("Elevator::Fixed", this, &ElevatorLogic::HandleFixed);
	env.RegisterEventHandler("Person::Entering", this, &ElevatorLogic::HandleEntering);
	env.RegisterEventHandler("Person::Entered", this, &ElevatorLogic::HandleEntered);
	env.RegisterEventHandler("Person::Exited", this, &ElevatorLogic::HandleExited);

	eleInfos.clear();
	
	
}

void ElevatorLogic::HandleInterfaceNotify(Environment &env, const Event &e) {

	Interface	*interf = static_cast<Interface*>(e.GetSender());
	Loadable	*loadable = interf->GetLoadable(0);
	Person		*pers = static_cast<Person*>(e.GetEventHandler());
	Floor		*floor = pers->GetCurrentFloor();
	Elevator	*ele;
	TARGET_TYPE type;
	std::map<Elevator*, EleInfo>::iterator it;
	
	
	// Call for Elevator
	if (loadable->GetType() == "Elevator") 
	{
		int numEle = interf->GetLoadableCount();
		int shortestDistance = 1000000;
		int distance;
		Elevator* finalEle = NULL;
		
		for (int i = 0; i < numEle; i++)
		{
			ele = static_cast<Elevator*>(interf->GetLoadable(i));
			distance = abs(getEffectiveDist(ele, ele->GetCurrentFloor(), floor));
			if (!registerElevator(ele)->second.malfunctioning && distance < shortestDistance)
			{
				shortestDistance = distance;
				finalEle = ele;
			}
		}

		if (finalEle == NULL)
		{
			for (int i = 0; i < numEle; i++)
			{
				ele = static_cast<Elevator*>(interf->GetLoadable(i));
				distance = abs(getEffectiveDist(ele, ele->GetCurrentFloor(), floor));
				if (distance < shortestDistance)
				{
					shortestDistance = distance;
					finalEle = ele;
				}
			}
		}

		ele = finalEle;
		type = FETCH;
	}
	// Call to Floor
	else
	{
		ele = pers->GetCurrentElevator();
		floor = static_cast<Floor*>(interf->GetLoadable(0));
		type = DESTINATION;
	}	

	insertTarget(ele, floor, &(registerElevator(ele))->second.targetFloors, std::pair<TARGET_TYPE, Person*>(type, pers), env);
}

void ElevatorLogic::HandleStopped(Environment &env, const Event &e) {
	Elevator* ele = static_cast<Elevator*>(e.GetSender());
	Floor* currFloor = ele->GetCurrentFloor();
	std::map<Elevator*, EleInfo>::iterator it = registerElevator(ele);

	if (currFloor == it->second.targetFloors.begin()->first && isMiddle(ele->GetPosition()))
	{
		env.SendEvent("Elevator::Open", 0, this, ele);

		it->second.stop_ID = -1;
	}
	else
	{
		sendToFloor(ele, registerElevator(ele)->second.targetFloors.begin()->first, env, 0);
	}
}

void ElevatorLogic::HandleMoving(Environment & env, const Event &e)
{
	Elevator* ele = static_cast<Elevator*>(e.GetSender());
	std::map<Elevator*, EleInfo>::iterator it = registerElevator(ele);
	
	if (it->second.moving_ID != -1)
	{
		it->second.moving_ID = -1;
	}
}

void ElevatorLogic::HandleOpening(Environment &env, const Event &e)
{
	Elevator* ele = static_cast<Elevator*>(e.GetSender());
	std::map<Elevator*, EleInfo>::iterator it = registerElevator(ele);
	it->second.open = true;
	if (it->second.moving_ID != -1)
	{
		env.CancelEvent(it->second.moving_ID);
		it->second.moving_ID = -1;
	}
	
}


void ElevatorLogic::HandleOpened(Environment &env, const Event &e) 
{
	Elevator* ele = static_cast<Elevator*>(e.GetSender());
	std::map<Elevator*, EleInfo>::iterator it = registerElevator(ele);

	if (!it->second.malfunctioning)
		registerElevator(ele)->second.closing_ID = env.SendEvent("Elevator::Close", 1, this, ele);
}

void ElevatorLogic::HandleEntering(Environment &env, const Event &e)
{
	
	Person* prs = static_cast<Person*>(e.GetSender());
	Elevator* ele = static_cast<Elevator*>(e.GetEventHandler());
	
	std::map<Elevator*, EleInfo>::iterator it = registerElevator(prs->GetCurrentElevator());
	
	it->second.load += prs->GetWeight();

	if (it->second.load > ele->GetMaxLoad())
	{
		env.CancelEvent(registerElevator(ele)->second.closing_ID);

		it->second.overloaded = true;
	}
}


void ElevatorLogic::HandleExited(Environment &env, const Event &e)
{
	Person* prs = static_cast<Person*>(e.GetSender());
	Elevator* ele = static_cast<Elevator*>(e.GetEventHandler());

	std::map<Elevator*, EleInfo>::iterator it = registerElevator(ele);
	
	
	it->second.load -= prs->GetWeight();
	if (it->second.overloaded && it->second.load < ele->GetMaxLoad())
	{
		it->second.overloaded = false;
		env.SendEvent("Elevator::StopBeep", 0, this, ele);
		env.SendEvent("Elevator::Close", 0, this, ele);
	}

	// Remove Destination Requests for this Person
	for (std::list<Floor_Pair>::iterator floor_it = it->second.targetFloors.begin(); floor_it != it->second.targetFloors.end(); floor_it++)
	{
		if (floor_it->second.first == DESTINATION && floor_it->second.second == prs)
		{
			it->second.targetFloors.erase(floor_it);
			break;
		}
	}
}

void ElevatorLogic::HandleEntered(Environment& env, const Event &e)
{
	Person* prs = static_cast<Person*>(e.GetSender());
	Elevator* ele = static_cast<Elevator*>(e.GetEventHandler());
	std::map<Elevator*, EleInfo>::iterator it = registerElevator(ele);
	
	if (it->second.overloaded)
		env.SendEvent("Elevator::Beep", 0, this, ele);

	// Remove Fetch Requests for this Person
	for (std::list<Floor_Pair>::iterator floor_it = it->second.targetFloors.begin(); floor_it != it->second.targetFloors.end(); floor_it++)
	{
		if (floor_it->second.first == FETCH && floor_it->second.second == prs)
		{
			it->second.targetFloors.erase(floor_it);
			break;
		}
	}
}

void ElevatorLogic::HandleClosed(Environment &env, const Event &e) {
	Elevator* ele = static_cast<Elevator*>(e.GetSender());
	registerElevator(ele)->second.open = false;

	sendToFloor(ele, registerElevator(ele)->second.targetFloors.begin()->first, env, 0);
}


void ElevatorLogic::HandleMalfunction(Environment &env, const Event &e)
{
	Elevator* ele = static_cast<Elevator*>(e.GetSender());
	std::map<Elevator*, EleInfo>::iterator it = registerElevator(ele);
	
	env.SendEvent("Elevator::Stop", 0, this, ele);
	it->second.stop_ID = -1;
	
	if (it->second.closing_ID != -1)
	{
		env.CancelEvent(it->second.closing_ID);
		it->second.closing_ID = -1;
	}
	else if (isMiddle(ele->GetPosition())){
		env.SendEvent("Elevator::Open", 0, this, ele);
	}
	
	it->second.malfunctioning = true;
}

void ElevatorLogic::HandleFixed(Environment &env, const Event &e)
{
	Elevator* ele = static_cast<Elevator*>(e.GetSender());
	registerElevator(ele)->second.malfunctioning = false;

	env.SendEvent("Elevator::Close", 1, this, ele);
}
