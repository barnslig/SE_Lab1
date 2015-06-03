/*
 * ElevatorLogic.cpp
 *
 *  Created on: 20.06.2014
 *      Author: STSJR
 */

#include "ElevatorLogic.h"

#include <iostream>

#include "Interface.h"
#include "Person.h"
#include "Floor.h"
#include "Elevator.h"
#include "Event.h"
#include "Environment.h"


int getDistance(Floor* f1, Floor* f2, double pos = 0.5)
{
	int d = f1->GetHeight() * pos;
	Floor *dummy = f1;

	if (f1 == f2)
		return abs(d - (f1->GetHeight() / 2));

	if (f1->IsBelow(f2))
	{
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
		d = 1 - d;
		while (dummy != f2)
		{
			d += dummy->GetHeight();
			dummy = dummy->GetAbove();
		}
		d += f2->GetHeight() / 2;
	}

	return d;
}

void ElevatorLogic::sendToFloor(Elevator* ele, Floor* floor, Environment& env)
{
	int dist = getDistance(ele->GetCurrentFloor(), floor, ele->GetPosition());
	std::map<const Elevator*, EleInfo>::iterator it = registerElevator(ele);
	std::cout << "Send to Floor: " << floor->GetId() << std::endl;

	if (dist > 0)
	{
		env.SendEvent("Elevator::Up", 0, this, ele);
	}
	else
	{
		env.SendEvent("Elevator::Down", 0, this, ele);
	}
	if (it->second.stop_ID != -1)
		env.CancelEvent(it->second.stop_ID);
	it->second.stop_ID = env.SendEvent("Elevator::Stop", abs(dist) / ele->GetSpeed(), this, ele);
}

void ElevatorLogic::insertTarget(Elevator* ele, Floor* target, std::list<Floor_Pair>* list, TARGET_TYPE type)
{
	int distance = abs(getDistance(ele->GetCurrentFloor(), target, ele->GetPosition()));

	std::list<std::pair<Floor*,TARGET_TYPE>>::iterator it = list->begin();

	while (true)
	{
		if (it == list->end() || distance < getDistance(ele->GetCurrentFloor(), it->first, ele->GetPosition()))
		{
			std::cout << "Added Floor " << target->GetId() << " to query" << std::endl;
			switch (type)
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

			list->insert(it, Floor_Pair(target,type));
			break;
		}
		else
		{
			it++;
		}
	}
	
}

ElevatorLogic::ElevatorLogic() : EventHandler("ElevatorLogic"), moved_(false) {
}

ElevatorLogic::~ElevatorLogic() {
}

void ElevatorLogic::Initialize(Environment &env) {
	env.RegisterEventHandler("Interface::Notify", this, &ElevatorLogic::HandleInterfaceNotify);
	env.RegisterEventHandler("Elevator::Stopped", this, &ElevatorLogic::HandleStopped);
	env.RegisterEventHandler("Elevator::Opened", this, &ElevatorLogic::HandleOpened);
	env.RegisterEventHandler("Elevator::Closed", this, &ElevatorLogic::HandleClosed);
	env.RegisterEventHandler("Elevator::Opening", this, &ElevatorLogic::HandleOpening);
	env.RegisterEventHandler("Elevator::Malfunction", this, &ElevatorLogic::HandleMalfunction);
	env.RegisterEventHandler("Elevator::Fixed", this, &ElevatorLogic::HandleFixed);
	env.RegisterEventHandler("Person::Entered", this, &ElevatorLogic::HandleEntered);
	env.RegisterEventHandler("Person::Exited", this, &ElevatorLogic::HandleExited);
}

void ElevatorLogic::HandleInterfaceNotify(Environment &env, const Event &e) {

	Interface	*interf = static_cast<Interface*>(e.GetSender());
	Loadable	*loadable = interf->GetLoadable(0);
	Person		*pers = static_cast<Person*>(e.GetEventHandler());
	Floor		*start_floor = pers->GetCurrentFloor();
	
	if (loadable->GetType() == "Elevator") 
	{
		
		Elevator *ele = static_cast<Elevator*>(loadable);
		std::map<const Elevator*, EleInfo>::iterator it = registerElevator(ele);
		insertTarget(ele, start_floor, &(it->second.targetFloors), FETCH);
		
		// WAYYY Too Complicated call but eh 
		sendToFloor(ele, it->second.targetFloors.begin()->first, env);
	}
}

void ElevatorLogic::HandleStopped(Environment &env, const Event &e) {

	Elevator *ele = static_cast<Elevator*>(e.GetSender());
	
	std::map<const Elevator*, EleInfo>::iterator Info_it = registerElevator(ele);

	if (isMiddle(ele->GetPosition()) && Info_it->second.stop_ID != -1)
	{
		
		std::list<Floor_Pair>::iterator it = Info_it->second.targetFloors.begin();
		
		while (it != Info_it->second.targetFloors.end())
		{
			if (it->first == ele->GetCurrentFloor())
			{
				env.SendEvent("Elevator::Open", 1, this, ele);
				break;
			}
		}

		Info_it->second.stop_ID = -1;
	}
	
}

void ElevatorLogic::HandleOpened(Environment &env, const Event &e) {

	Elevator *ele = static_cast<Elevator*>(e.GetSender());
	std::map<const Elevator*, EleInfo>::iterator it = registerElevator(ele);
	it->second.targetFloors.pop_front();

	std::cout << "Opening at Floor: " << ele->GetCurrentFloor()->GetId() << std::endl;
	std::cout << "at Poistion: " << ele->GetPosition() << std::endl;

	env.SendEvent("Elevator::Close", 3, this, ele);
}

void ElevatorLogic::HandleClosed(Environment &env, const Event &e) {
	//std::cout << "-1" << std::endl;
	
	Elevator *ele = static_cast<Elevator*>(e.GetSender());
	std::map<const Elevator*, EleInfo>::iterator it = registerElevator(ele);
	std::list<Floor_Pair> &targetFloors = it->second.targetFloors;
	Floor* target;

	//std::cout << "0" << std::endl;
	
	if (targetFloors.size() > 0)
	{
		target = targetFloors.begin()->first;

		//std::cout << "1" << std::endl;

		sendToFloor(ele, target, env);
	}
}

void ElevatorLogic::HandleClosing(Environment &env, const Event &e)
{
	Elevator *ele = static_cast<Elevator*>(e.GetSender());
	std::map<const Elevator*, EleInfo>::iterator it = eleInfos.find(ele);

	if (it != eleInfos.end()) 
	{
		if (it->second.overloaded)
		{
			env.CancelEvent(e.GetId());
		}
	}
}

void ElevatorLogic::HandleOpening(Environment &env, const Event &e) {
	Elevator *ele = static_cast<Elevator*>(e.GetSender());
	Elevator::Movement state = ele->GetState();
	
	if (!isMiddle(ele->GetPosition()) || state == Elevator::Up || state == Elevator::Down)
	{
		if (e.GetTime() > env.GetClock())
			env.CancelEvent(e.GetId());
		else // "Safeguard"
			env.SendEvent("Elevator::Close", 0, this, ele);
		
	}
}

void ElevatorLogic::HandleMalfunction(Environment &env, const Event &e)
{
	Elevator *ele = static_cast<Elevator*>(e.GetSender());
	Elevator::Movement state = ele->GetState();
	
	registerElevator(ele);

	eleInfos.find(ele)->second.last_state = state;

	env.SendEvent("Elevator::Stop", 0, this, ele);
}

void ElevatorLogic::HandleFixed(Environment &env, const Event &e)
{
	Elevator *ele = static_cast<Elevator*>(e.GetSender());
	
	std::map<const Elevator*, EleInfo>::iterator it = registerElevator(ele);


	Floor* target = it->second.targetFloors.begin()->first;
	int distance = getDistance(ele->GetCurrentFloor(), target, ele->GetPosition());
	if (distance > 0)
		env.SendEvent("Elevator::Up", 0, this, ele);
	else 
		env.SendEvent("Elevator::Down", 0, this, ele);
	
	env.SendEvent("Elevator::Stop", distance / ele->GetSpeed(), this, ele);
}

void ElevatorLogic::HandleEntered(Environment &env, const Event &e)
{
	Person *prs = static_cast<Person*>(e.GetSender());
	Elevator *ele = static_cast<Elevator*>(e.GetEventHandler());

	std::cout << "Person entered: " << prs->GetId() << std::endl;
	
	std::map<const Elevator*, EleInfo>::iterator it = registerElevator(ele);
	
	it->second.load += prs->GetWeight();
	
	//it->second.targetFloors.insert(it->second.targetFloors.begin(),prs->GetFinalFloor());
	insertTarget(ele, prs->GetFinalFloor(), &(it->second.targetFloors),DESTINATION);
	if (it->second.load > ele->GetMaxLoad())
	{
		it->second.overloaded = true;
		env.SendEvent("Elevator::Beep", 0, this, ele);
	}
	
	std::list<Floor_Pair>::iterator Floor_it = it->second.targetFloors.begin();
	while (Floor_it != it->second.targetFloors.end())
	{
		std::cout << "BLUB" << std::endl;
		if (Floor_it->first == ele->GetCurrentFloor() && Floor_it->second == FETCH)
		{
			std::cout << "Removed \"Fetch-Floor\" from Queue: " << Floor_it->first->GetId() << std::endl;
			Floor_it = it->second.targetFloors.erase(Floor_it);
		}
		else {
			Floor_it++;
		}
	}
}

void ElevatorLogic::HandleExited(Environment &env, const Event &e)
{
	Person *prs = static_cast<Person*>(e.GetSender());
	Elevator *ele = static_cast<Elevator*>(e.GetEventHandler());

	std::map<const Elevator*, EleInfo>::iterator it = registerElevator(ele);

	std::list<Floor_Pair>::iterator floors = it->second.targetFloors.begin();
	//std::cout << "A" << std::endl;
	while (floors != it->second.targetFloors.end())
	{
		if (floors->first == ele->GetCurrentFloor() && floors->second == DESTINATION)
		{
			std::cout << "Removed \"Destination-Floor\" from Queue: " << floors->first->GetId() << std::endl;
			it->second.targetFloors.erase(floors);
			break;
		}
		else
		{
			//std::cout << "C" << std::endl;
			floors++;
		}
	}
	//std::cout << "D" << std::endl;
	it->second.load -= prs->GetWeight();

	if (it->second.overloaded)
	{
		if (it->second.load <= ele->GetMaxLoad())
		{
			it->second.overloaded = false;
			env.SendEvent("Elevator::StopBeep", 0, this, ele);
		}
	}
}