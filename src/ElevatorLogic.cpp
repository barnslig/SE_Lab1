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
	return std::floor(d);
}

// Sends a Elevator to a given Floor
void ElevatorLogic::sendToFloor(Elevator* ele, Floor* floor, Environment& env)
{
	std::map<Elevator*, EleInfo>::iterator it = registerElevator(ele);
	Floor* dummy;

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

// Removes all Queud Floors above the target Floor
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
void ElevatorLogic::insertTarget(Elevator* ele, Floor* target, std::list<Floor_Pair>* list, TARGET_TYPE type)
{
	int distance = abs(getDistance(ele->GetCurrentFloor(), target, ele->GetPosition()));

	std::list<std::pair<Floor*,TARGET_TYPE>>::iterator it = list->begin();

	while (true)
	{
		if (it == list->end() || distance < getDistance(ele->GetCurrentFloor(), it->first, ele->GetPosition()))
		{
			if (VERBOSE)
			{
				std::cout << "Added Floor " << target->GetId() << " to Queue of Elevator" << ele->GetId() << std::endl;
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


ElevatorLogic::ElevatorLogic() : EventHandler("ElevatorLogic"){
}

ElevatorLogic::~ElevatorLogic() {
}

void ElevatorLogic::Initialize(Environment &env) {
	env.RegisterEventHandler("Interface::Notify", this, &ElevatorLogic::HandleInterfaceNotify);
	env.RegisterEventHandler("Elevator::Stopped", this, &ElevatorLogic::HandleStopped);
	//env.RegisterEventHandler("Elevator::Moving", this, &ElevatorLogic::HandleMoving);
	env.RegisterEventHandler("Elevator::Opened", this, &ElevatorLogic::HandleOpened);
	env.RegisterEventHandler("Elevator::Closed", this, &ElevatorLogic::HandleClosed);
	env.RegisterEventHandler("Elevator::Opening", this, &ElevatorLogic::HandleOpening);
	env.RegisterEventHandler("Elevator::Malfunction", this, &ElevatorLogic::HandleMalfunction);
	env.RegisterEventHandler("Elevator::Fixed", this, &ElevatorLogic::HandleFixed);
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
	// Call for Elevator
	if (loadable->GetType() == "Elevator") 
	{
		ele = static_cast<Elevator*>(loadable);
		floor = pers->GetCurrentFloor();
		type = FETCH;
		int distance = abs(getDistance(ele->GetCurrentFloor(), pers->GetCurrentFloor(), ele->GetPosition()));
		Elevator* dummy;
		for (int i = 1; i < interf->GetLoadableCount(); i++)
		{
			dummy = static_cast<Elevator*>(interf->GetLoadable(i));
			if (distance > abs(getDistance(dummy->GetCurrentFloor(), pers->GetCurrentFloor(), dummy->GetPosition())))
			{
				distance = abs(getDistance(dummy->GetCurrentFloor(), pers->GetCurrentFloor(), dummy->GetPosition()));
				ele = dummy;
			}
		}
	} 
	// Call to Floor
	else
	{
		floor = static_cast<Floor*>(loadable);
		ele = pers->GetCurrentElevator();
		type = DESTINATION;
	}
	
	std::map<Elevator*, EleInfo>::iterator it = registerElevator(ele);
	insertTarget(ele, floor, &(it->second.targetFloors), type);
	sendToFloor(ele, it->second.targetFloors.begin()->first, env);
}

void ElevatorLogic::HandleStopped(Environment &env, const Event &e) {

	Elevator *ele = static_cast<Elevator*>(e.GetSender());
	
	std::map<Elevator*, EleInfo>::iterator Info_it = registerElevator(ele);

	if (VERBOSE)
	{
		std::cout << "Stopped at Floor " << ele->GetCurrentFloor()->GetId() << std::endl;
		std::cout << "Stopped at Position " << ele->GetPosition() << std::endl;
		std::cout << "\"Heighest Floor\" " << getHighestFloor(ele)->GetId() << std::endl;
		std::cout << "\"Lowest Floor\" " << getHighestFloor(ele)->GetId() << std::endl;

		if (ele->GetCurrentFloor()->IsBelow(getHighestFloor(ele)))
		{
			std::cout << "Crashed into Ceiling" << std::endl;
			exit(0);
		}
		else if (ele->GetCurrentFloor()->IsAbove(getLowestFloor(ele)))
		{
			std::cout << "Crashed into Botton" << std::endl;
		}
	}

	if (isMiddle(ele->GetPosition()) && Info_it->second.stop_ID != -1)
	{
		
		std::list<Floor_Pair>::iterator it = Info_it->second.targetFloors.begin();
		
		while (it != Info_it->second.targetFloors.end())
		{
			if (it->first == ele->GetCurrentFloor())
			{
				env.SendEvent("Elevator::Open", 0, this, ele);
				break;
			}
		}

		Info_it->second.stop_ID = -1;
	}
	
}

void ElevatorLogic::HandleMoving(Environment & env, const Event &e)
{
	Elevator *ele = static_cast<Elevator*>(e.GetSender());
	bool isHighest = ele->IsHighestFloor(ele->GetCurrentFloor());
	bool isLowest = ele->IsLowestFloor(ele->GetCurrentFloor());

	if (VERBOSE)
	{
		if (isHighest)
			std::cout << "At Heighest" << std::endl;
		if (isLowest)
			std::cout << "At Lowest" << std::endl;
	}
	/*if (isHighest || isLowest)
	{
		if ((isHighest && ele->GetState() == (*ele).Up)|| (isLowest && ele->GetState() == (*ele).Down))
		{
			if (isMiddle(ele->GetPosition()))
			{
				std::map<Elevator*, EleInfo>::iterator it = registerElevator(ele);
				env.SendEvent("Elevator::Stop", 0, this, ele);

				if (it->second.stop_ID != -1)
				{
					env.CancelEvent(it->second.stop_ID);
					it->second.stop_ID = -1;
				}
			}
			else if (isHighest && ele->GetPosition() > 0.5f || isLowest && ele->GetPosition() < 0.5f)
			{
				insertTarget(ele, ele->GetCurrentFloor(), &registerElevator(ele)->second.targetFloors, FETCH);
				sendToFloor(ele, registerElevator(ele)->second.targetFloors.begin()->first, env);
			}
		}

		if (isHighest)
			removeAllAbove(ele, ele->GetCurrentFloor());
		if (isLowest)
			removeAllBelow(ele, ele->GetCurrentFloor());
			
	}*/
}

void ElevatorLogic::HandleOpened(Environment &env, const Event &e) {

	Elevator *ele = static_cast<Elevator*>(e.GetSender());
	//std::map<Elevator*, EleInfo>::iterator it = registerElevator(ele);
	//it->second.targetFloors.pop_front();
	if (VERBOSE){
		std::cout << "Opening at Floor: " << ele->GetCurrentFloor()->GetId() << std::endl;
		std::cout << "at Poistion: " << ele->GetPosition() << std::endl;
	}
	env.SendEvent("Elevator::Close", 1, this, ele);

}

void ElevatorLogic::HandleClosed(Environment &env, const Event &e) {
	Elevator *ele = static_cast<Elevator*>(e.GetSender());
	std::map<Elevator*, EleInfo>::iterator it = registerElevator(ele);
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
	std::map<Elevator*, EleInfo>::iterator it = eleInfos.find(ele);

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
	
	std::map<Elevator*, EleInfo>::iterator it = registerElevator(ele);

	it->second.last_state = state;

	env.SendEvent("Elevator::Stop", 0, this, ele);
}

void ElevatorLogic::HandleFixed(Environment &env, const Event &e)
{
	Elevator *ele = static_cast<Elevator*>(e.GetSender());
	
	std::map<Elevator*, EleInfo>::iterator it = registerElevator(ele);


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

	if (VERBOSE)
	{
		std::cout << "Person entered: " << prs->GetId() << std::endl;
		
	}
	std::map<Elevator*, EleInfo>::iterator it = registerElevator(ele);
	
	it->second.load += prs->GetWeight();
	
	//it->second.targetFloors.insert(it->second.targetFloors.begin(),prs->GetFinalFloor());
	//insertTarget(ele, prs->GetFinalFloor(), &(it->second.targetFloors),DESTINATION);
	if (it->second.load > ele->GetMaxLoad())
	{
		it->second.overloaded = true;
		env.SendEvent("Elevator::Beep", 0, this, ele);
	}
	
	std::list<Floor_Pair>::iterator Floor_it = it->second.targetFloors.begin();
	while (Floor_it != it->second.targetFloors.end())
	{
		/*
		std::cout << "BLUB" << std::endl;
		std::cout << ele->GetCurrentFloor()->GetId() << "/" << Floor_it->first->GetId() << std::endl;
		std::cout << Floor_it->second<< "/" << FETCH << std::endl;
		*/
		if (Floor_it->first == ele->GetCurrentFloor() && Floor_it->second == FETCH)
		{
			if (VERBOSE)
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

	std::map<Elevator*, EleInfo>::iterator it = registerElevator(ele);

	std::list<Floor_Pair>::iterator floors = it->second.targetFloors.begin();
	//std::cout << "A" << std::endl;
	while (floors != it->second.targetFloors.end())
	{
		if (floors->first == ele->GetCurrentFloor() && floors->second == DESTINATION)
		{
			if (VERBOSE)
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