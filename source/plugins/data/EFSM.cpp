#include "EFSM.h"
#include <vector>
#include <string>
#include <map>
#include <utility>


#ifdef PLUGINCONNECT_DYNAMIC

extern "C" StaticGetPluginInformation GetPluginInformation() {
    return &ExtendedFSM::GetPluginInformation;
}
#endif

ExtendedFSM::ExtendedFSM(Model* model, std::string name) : ModelDataDefinition(model, Util::TypeOf<ExtendedFSM>(), name) {}

std::string ExtendedFSM::show(){
    auto txt = ModelDataDefinition::show() + ",variables=[";
    for (auto* var: *_variables) {
        txt += var->show() + ",";
    }
    txt = txt.substr(0, txt.length() - 1) + "]";
    return txt;
}

void ExtendedFSM::_createInternalAndAttachedData(){
    ModelDataManager* elems = _parentModel->getDataManager();
    
    for (Variable* variable : *_variables) {
        _attachedDataInsert(getName() + "." + variable->getName(), variable);
    }

}

void ExtendedFSM::_saveInstance(PersistenceRecord *fields, bool saveDefaultValues) {}

bool ExtendedFSM::_loadInstance(PersistenceRecord *fields) {
    bool res = true;
    try {
    } catch (...) {
        res = false;
    }

    return res;
}

PluginInformation* ExtendedFSM::GetPluginInformation() {
    PluginInformation* info = new PluginInformation(Util::TypeOf<ExtendedFSM>(), &ExtendedFSM::LoadInstance, &ExtendedFSM::NewInstance);
    return info;
}

ModelDataDefinition* ExtendedFSM::NewInstance(Model* model, std::string name) {
    return new ExtendedFSM(model, name);
}

ModelDataDefinition* ExtendedFSM::LoadInstance(Model* model, PersistenceRecord *fields) {
    ExtendedFSM* newElement = new ExtendedFSM(model);
    try {
        newElement->_loadInstance(fields);
    } catch (const std::exception& e) {

    }
    return newElement;
}

bool ExtendedFSM::_check(std::string* errorMessage){
    return true;
}

void ExtendedFSM::_initBetweenReplications(){
	_returnModels->clear();
	_currentStates.clear();
    _currentStates.push_back(_initialState);
	for (auto* var: *_variables) {
		InitBetweenReplications(var);
		for (auto value: *var->getValues()) {
			_parentModel->parseExpression(value.first + " = " + std::to_string(value.second));
		}
	}
}
void ExtendedFSM::reset(){
	this->_initBetweenReplications();
}

void ExtendedFSM::insertVariable(Variable* variable) {
    _variables->push_back(variable);
}

std::string ExtendedFSM::getCurrentState(){
	if (!_parallelismEnabled) return _currentStates.front()->getName();
	// comma‑separate all active states
	std::string s;
	for (auto* st : _currentStates) {
		if (!s.empty()) s += ",";
		s += st->getName();
	}
	return s;
}


void ExtendedFSM::setInitialState(FSM_State* state) {
    _initialState = state;
}

void ExtendedFSM::setCurrentState(FSM_State* state) {
	traceSimulation(this, TraceManager::Level::L5_event,
			">> currentStates: " + std::to_string(_currentStates.size()));
    _currentStates.push_back(state);
}

void ExtendedFSM::leaveEFSM(Entity* entity, FSM_State* newCurrentState) {
	_returnedToComponent = true;
    if (!_parallelismEnabled) {
        setCurrentState(newCurrentState);
        auto returnComponent = _returnModels->back(); _returnModels->pop_back();
        this->_parentModel->sendEntityToComponent(entity, returnComponent);
        return;
    }
    // parallel: add to active set, then dedupe
    _currentStates.push_back(newCurrentState);
    // dedupe by name
    std::unordered_map<std::string, FSM_State*> unique;
    for (auto* st : _currentStates) unique[st->getName()] = st;
    _currentStates.clear();
    for (auto& kv : unique) _currentStates.push_back(kv.second);
    // after finishing all, pop return and send
    auto returnComponent = _returnModels->back(); _returnModels->pop_back();
    this->_parentModel->sendEntityToComponent(entity, returnComponent);
}

void ExtendedFSM::enterEFSM(Entity* entity, ModelComponent* returnState) {
	this->_returnedToComponent = false;
	if (!_parallelismEnabled) {
		if (_currentStates.front()->isFinalState()) {
			this->_parentModel->sendEntityToComponent(entity, returnState); 
		} else {
			_returnModels->push_back(returnState);
			this->_parentModel->sendEntityToComponent(entity, _currentStates.front()); 
		}
	} else {
		_returnModels->push_back(returnState);
		for (auto* st : _currentStates) {
			if (st->isFinalState()) {
				this->_parentModel->sendEntityToComponent(entity, returnState);
			} else {
				this->_parentModel->sendEntityToComponent(entity, st);
			}
		}
	}
}

void ExtendedFSM::updateCurrentStates(FSM_State* oldState, std::vector<FSM_State*> newStates) {
	// Remove the old state from _currentStates 
    auto it = std::remove_if(_currentStates.begin(), _currentStates.end(),
        [oldState](FSM_State* state) {
            return state->getName() == oldState->getName();
        });
    _currentStates.erase(it, _currentStates.end());
    _currentStates.insert(_currentStates.end(), newStates.begin(), newStates.end());
	traceSimulation(this, TraceManager::Level::L5_event,
			">> updateCurrentStates // currentStates: " + std::to_string(_currentStates.size()));
}
