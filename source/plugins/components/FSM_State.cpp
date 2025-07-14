#include <stdexcept>
#include <random>
#include <vector>
#include <set>
#include "FSM_State.h"

ModelDataDefinition* FSM_State::NewInstance(Model* model, std::string name) {
	return new FSM_State(model, name);
}

FSM_State::FSM_State(Model* model, std::string name) : ModelComponent(model, Util::TypeOf<FSM_State>(), name) {
	_name = name;
}

PluginInformation* FSM_State::GetPluginInformation() {
	PluginInformation* info = new PluginInformation(Util::TypeOf<FSM_State>(), &FSM_State::LoadInstance, &FSM_State::NewInstance);
	info->setDescriptionHelp("//@TODO");
	info->setReceiveTransfer(true);
	info->setSendTransfer(true);
    //info->setAuthor("...");
	//info->setDate("...");
	//info->setObservation("...");
    return info;
}

ModelComponent* FSM_State::LoadInstance(Model* model, PersistenceRecord *fields) {
	FSM_State* newComponent = new FSM_State(model);
	try {
		newComponent->_loadInstance(fields);
	} catch (const std::exception& e) {

	}
	return newComponent;
}

void FSM_State::_saveInstance(PersistenceRecord *fields, bool saveDefaultValues) {
	fields->saveField("name", _name, "");
}

bool FSM_State::_loadInstance(PersistenceRecord *fields) {
    bool res = true;
    try {
        _name = fields->loadField("name", "");
        //_isInitialState = fields->loadField("isInitialState", );
	} catch (...) {
		res = false;
    }

    return res;
}

std::string FSM_State::show(){
    return ModelComponent::show();
}

void FSM_State::setAsInitialState() {
    _efsm->setInitialState(this);
}

void FSM_State::setEFSM(ExtendedFSM* efsm) {
    _efsm = efsm;
}

void FSM_State::setRefinement(ExtendedFSM* refinement) {
    _refinement = refinement;
}

void FSM_State::resetRefinment() {
    if (_refinement != nullptr) {
        _refinement->reset();
    }
}
ExtendedFSM* FSM_State::getRefinement() {
    return _refinement;
}

void FSM_State::fire(Entity* entity) {
    auto connections = this->getConnections()->connections();
	if (connections->empty() && !_efsm->hasReturnedToComponent()) { // TODO check this
        // This is likely a final state with no outgoing transitions
        _efsm->leaveEFSM(entity, this);
        return;
    }

	if (_efsm->hasReturnedToComponent()) {
		// Already reached a final state in some other branch - nothing to do
		return;
	}

    auto transitionChosen = dynamic_cast<FSM_Transition*>(connections->begin()->second->component);
    auto transitionDefault = transitionChosen;

    auto anyDefaultConnected = false;
    auto totalEnableds = 0;
    auto deterministicEnabled = false;
    auto nonDefaultEnabled = false;

	std::vector<FSM_Transition*> possibleTransitions;

    for(auto connection: *connections) {
        auto transition = dynamic_cast<FSM_Transition*>(connection.second->component);

        if (transition->isDefault()) {
            anyDefaultConnected = true;
            transitionDefault = transition;
        }

        if (transition->isEnabled()) {
            ++totalEnableds;

            if (transition->isDeterministic()) {
                deterministicEnabled = true;
            }

            if (transition->isDefault()){
                if (not nonDefaultEnabled) {
                    transitionChosen = transition;
                }
            } else {
				// On each iteration, add the transitionChosen to the vector of possible
				// transitions, to be picked at random later
                nonDefaultEnabled = true;
				possibleTransitions.push_back(transition);
            }
        }
    }

	traceSimulation(this, TraceManager::Level::L5_event,
			">> S.fire: totalEnableds=" + std::to_string(totalEnableds)
			+ " nondet=" + std::to_string(possibleTransitions.size())
			+ " detEnabled=" + (deterministicEnabled ? "Y":"N")
			);

    if (deterministicEnabled and totalEnableds > 1) {
        throw std::domain_error("More than a transition and at least one is deterministic.");
    }

    if (totalEnableds == 0) {
        transitionChosen = transitionDefault;
    }

	if (_efsm && _efsm->isParallelEnabled() && !possibleTransitions.empty()) {
		traceSimulation(this, TraceManager::Level::L5_event,
				"  -- parallel branch: firing "
				+ std::to_string(possibleTransitions.size())
				+ " transitions"
				);

        std::set<FSM_State*> targetStates;
        std::vector<std::pair<FSM_Transition*, FSM_State*>> validTransitions;

        for (auto* tr : possibleTransitions) {
            if (!tr) continue;

			traceSimulation(this, TraceManager::Level::L5_event, " -> About to fire " + tr->getName());

            auto conns = tr->getConnections()->connections();
            if (conns->empty()) continue;

            FSM_State* dest = dynamic_cast<FSM_State*>(conns->begin()->second->component);
            if (!dest) continue;
            
            if (targetStates.find(dest) == targetStates.end()) {
                targetStates.insert(dest);
                validTransitions.push_back(std::make_pair(tr, dest));
            }
        }

		std::vector<FSM_State*> newStates;
        for (auto& transitionPair : validTransitions) {
            auto* tr = transitionPair.first; 
            if (_refinement && !tr->isPreemptive()) {
                _refinement->enterEFSM(entity, tr);
            }
            this->_parentModel->sendEntityToComponent(entity, tr);
			newStates.push_back(transitionPair.second);
            //_efsm->setCurrentState(dest);
        }
		_efsm->updateCurrentStates(this, newStates);
        return;
    }

	// Pick a random transition, since we cannot go to all of them
	if (!possibleTransitions.empty()) {
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(0, possibleTransitions.size() - 1);
		transitionChosen = possibleTransitions[dis(gen)];
	}

    if (_refinement != nullptr and not transitionChosen->isPreemptive()) {
        _refinement->enterEFSM(entity, transitionChosen);
    }

   this->_parentModel->sendEntityToComponent(entity, transitionChosen); 
}

void FSM_State::fireWithOnlyImmediate(Entity* entity) {
    auto connections = this->getConnections()->connections();

    // If no outgoing transitions and not returned yet, treat as final state
    if (connections->empty() && !_efsm->hasReturnedToComponent()) {
        _efsm->leaveEFSM(entity, this);
        return;
    }

    // If already returned via some branch, nothing to do
    if (_efsm->hasReturnedToComponent()) {
        return;
    }

    // Initialize chosen transition to first connection (fallback placeholder)
    auto transitionChosen = dynamic_cast<FSM_Transition*>(connections->begin()->second->component);

    // Track possible immediate transitions
    std::vector<FSM_Transition*> possibleTransitions;
    int totalEnableds = 0;
    bool deterministicEnabled = false;
    bool nonDefaultEnabled = false;

    for (auto connection : *connections) {
        auto transition = dynamic_cast<FSM_Transition*>(connection.second->component);
        if (!transition->isImmediate() || !transition->isEnabled())
            continue;

        ++totalEnableds;
        if (transition->isDeterministic()) {
            deterministicEnabled = true;
        }

        // Default transitions get chosen only if no non-default yet
        if (transition->isDefault()) {
            if (!nonDefaultEnabled) {
                transitionChosen = transition;
            }
        } else {
            nonDefaultEnabled = true;
            possibleTransitions.push_back(transition);
        }
    }

    traceSimulation(this, TraceManager::Level::L5_event,
                    ">> S.fireWithOnlyImmediate: totalEnableds=" + std::to_string(totalEnableds)
                    + " nondet=" + std::to_string(possibleTransitions.size())
                    + " detEnabled=" + (deterministicEnabled ? "Y" : "N")
    );

    if (deterministicEnabled && totalEnableds > 1) {
        throw std::domain_error("More than a transition and at least one is deterministic.");
    }

    // If no immediate transitions available, exit EFSM
    if (totalEnableds == 0) {
        _efsm->leaveEFSM(entity, this);
        return;
    }

    // Parallel branch firing: fire all unique-target transitions
    if (_efsm && _efsm->isParallelEnabled() && !possibleTransitions.empty()) {
        traceSimulation(this, TraceManager::Level::L5_event,
                        "  -- parallel branch: firing "
                        + std::to_string(possibleTransitions.size())
        );

        std::set<FSM_State*> targetStates;
        std::vector<std::pair<FSM_Transition*, FSM_State*>> validTransitions;
        for (auto* tr : possibleTransitions) {
            if (!tr) continue;
            traceSimulation(this, TraceManager::Level::L5_event, " -> About to fire " + tr->getName());

            auto conns = tr->getConnections()->connections();
            if (conns->empty()) continue;
            auto dest = dynamic_cast<FSM_State*>(conns->begin()->second->component);
            if (!dest) continue;
            if (targetStates.insert(dest).second) {
                validTransitions.emplace_back(tr, dest);
            }
        }

        std::vector<FSM_State*> newStates;
        for (auto& [tr, dest] : validTransitions) {
            if (_refinement && !tr->isPreemptive()) {
                _refinement->enterEFSM(entity, tr);
            }
            _parentModel->sendEntityToComponent(entity, tr);
            newStates.push_back(dest);
        }
        _efsm->updateCurrentStates(this, newStates);
        return;
    }

    // Choose one at random if multiple
    if (!possibleTransitions.empty()) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, possibleTransitions.size() - 1);
        transitionChosen = possibleTransitions[dis(gen)];
    }

    // Enter refinement if needed and send entity
    if (_refinement && !transitionChosen->isPreemptive()) {
        _refinement->enterEFSM(entity, transitionChosen);
    }
    _parentModel->sendEntityToComponent(entity, transitionChosen);
}


void FSM_State::_onDispatchEvent(Entity* entity, unsigned int inputPortNumber) {
	if (_mustBeImmediate) {
		_mustBeImmediate = false;

		if (isFinalState()) {
			_efsm->leaveEFSM(entity, this);
		}

		fireWithOnlyImmediate(entity);
	} else {
		fire(entity);
	}
}

bool FSM_State::isParallel() const {
	return _efsm && _efsm->isParallelEnabled();
}
