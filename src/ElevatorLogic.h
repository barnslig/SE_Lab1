/*
 * ElevatorLogic.h
 *
 *  Created on: 20.06.2014
 *      Author: STSJR
 */

#ifndef ELEVATORLOGIC_H_
#define ELEVATORLOGIC_H_

#define FLOATING_EPSILON 0.001f

#include "EventHandler.h"

#include <list>
#include <map>
#include <set>

class Elevator;
class Floor;
class Interface;


class ElevatorLogic: public EventHandler {

public:
	ElevatorLogic();
	virtual ~ElevatorLogic();

	void Initialize(Environment &env);

private:

	struct EleInfo
	{
		int load = 0;
		int last_state = 0;
		bool overloaded = false;
	};

	void HandleNotify(Environment &env, const Event &e);
	void HandleStopped(Environment &env, const Event &e);
	void HandleOpened(Environment &env, const Event &e);
	void HandleClosed(Environment &env, const Event &e);
	void HandleClosing(Environment &env, const Event &e);
	void HandleOpening(Environment &env, const Event &e);
	void HandleMalfunction(Environment &env, const Event &e);
	void HandleFixed(Environment &env, const Event &e);
	void HandleEntered(Environment &env, const Event &e);
	void HandleLeft(Environment &env, const Event &e);
	
	
	bool isMiddle(double pos) { return (pos > 0.5f - FLOATING_EPSILON && pos < 0.5f + FLOATING_EPSILON); }

	bool moved_;

	std::map<const Elevator*, EleInfo> eleInfos;
};

#endif /* ELEVATORLOGIC_H_ */
