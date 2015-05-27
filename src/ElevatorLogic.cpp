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

// TEST
ElevatorLogic::ElevatorLogic() : EventHandler("ElevatorLogic"), moved_(false) {
}

ElevatorLogic::~ElevatorLogic() {
}

void ElevatorLogic::Initialize(Environment &env) {
	env.RegisterEventHandler("Interface::Notify", this, &ElevatorLogic::HandleNotify);
	env.RegisterEventHandler("Elevator::Stopped", this, &ElevatorLogic::HandleStopped);
	env.RegisterEventHandler("Elevator::Opened", this, &ElevatorLogic::HandleOpened);
	env.RegisterEventHandler("Elevator::Closed", this, &ElevatorLogic::HandleClosed);
	env.RegisterEventHandler("Elevator::Opening", this, &ElevatorLogic::HandleOpening);
	env.RegisterEventHandler("Elevator::Malfunction", this, &ElevatorLogic::HandleMalfunction);
}

void ElevatorLogic::HandleNotify(Environment &env, const Event &e) {

	Interface *interf = static_cast<Interface*>(e.GetSender());
	Loadable *loadable = interf->GetLoadable(0);

	if (loadable->GetType() == "Elevator") {

		Elevator *ele = static_cast<Elevator*>(loadable);

		env.SendEvent("Elevator::Open", 0, this, ele);
	}
}

void ElevatorLogic::HandleStopped(Environment &env, const Event &e) {

	Elevator *ele = static_cast<Elevator*>(e.GetSender());
	//Floor *flr = ele->GetCurrentFloor();

	if (isMiddle(ele->GetPosition))
		env.SendEvent("Elevator::Open", 1, this, ele);
}

void ElevatorLogic::HandleOpened(Environment &env, const Event &e) {

	Elevator *ele = static_cast<Elevator*>(e.GetSender());

	env.SendEvent("Elevator::Close", 4, this, ele);
}

void ElevatorLogic::HandleClosed(Environment &env, const Event &e) {

	Elevator *ele = static_cast<Elevator*>(e.GetSender());

	if (!moved_) {
		env.SendEvent("Elevator::Up", 0, this, ele);
		env.SendEvent("Elevator::Stop", 4, this, ele);

		moved_ = true;
	}
}

void ElevatorLogic::HandleClosing(Environment &env, const Event &e)
{
	Elevator *ele = static_cast<Elevator*>(e.getSender());
	std::map<const Elevator*, EleInfo>::iterator it = eleInfos.find(ele);

	if (it != eleInfos.end) 
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
	
	if (!isMiddle(ele->GetPosition) || state == Elevator::Up || state == Elevator::Down)
	{
		if (e.GetTime() > env.GetClock())
			env.CancelEvent(e.GetId);
		else // "Safeguard"
			env.SendEvent("Elevator::Close", 0, this, ele);
		
	}
}

void ElevatorLogic::HandleMalfunction(Environment &env, const Event &e)
{
	Elevator *ele = static_cast<Elevator*>(e.GetSender());
	Elevator::Movement state = ele->GetState();
	
	if (eleInfos.find(ele) == eleInfos.end)
		eleInfos.insert(std::pair<const Elevator*, EleInfo>(ele, EleInfo()));

	eleInfos.find(ele)->second.last_state = state;

	env.SendEvent("Elevator::Stop", 0, this, ele);
}

void ElevatorLogic::HandleFixed(Environment &env, const Event &e)
{
	Elevator *ele = static_cast<Elevator*>(e.GetSender());
	
	Elevator::Movement last_state = static_cast<Elevator::Movement>(eleInfos.find(ele)->second.last_state);

	if (last_state == Elevator::Movement::Up)
		env.SendEvent("Elevator::Up", 0, this, ele);
	else if (last_state == Elevator::Movement::Down)
		env.SendEvent("Elevator::Down", 0, this, ele);
}

void ElevatorLogic::HandleEntered(Environment &env, const Event &e)
{
	Person *prs = static_cast<Person*>(e.GetSender());
	Elevator *ele = static_cast<Elevator*>(e.GetEventHandler());
	
	std::map<const Elevator*, EleInfo>::iterator it = eleInfos.find(ele);
	
	if (it == eleInfos.end) {
		it = eleInfos.insert(std::pair<const Elevator*, EleInfo>(ele, EleInfo())).first;
	}

	it->second.load += prs->GetWeight();
	
	if (it->second.load > ele->GetMaxLoad)
	{
		it->second.overloaded = true;
		env.SendEvent("Elevator::Beep", 0, this, ele);
	}
}

void ElevatorLogic::HandleEntered(Environment &env, const Event &e)
{
	Person *prs = static_cast<Person*>(e.GetSender());
	Elevator *ele = static_cast<Elevator*>(e.GetEventHandler());

	std::map<const Elevator*, EleInfo>::iterator it = eleInfos.find(ele);

	if (it == eleInfos.end) {
		it = eleInfos.insert(std::pair<const Elevator*, EleInfo>(ele, EleInfo())).first;
	}

	it->second.load -= prs->GetWeight();
	
	if (it->second.overloaded)
	{
		if (it->second.load <= ele->GetMaxLoad())
		{
			it->second.overloaded = false;
			env.SendEvent("Elevator::StopBeep", 0, this, ele);
		}
	}