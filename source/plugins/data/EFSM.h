/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cppFiles/class.h to edit this template
 */

/* 
 * File:   ExFiStatMac.h
 * Author: rlcancian
 *
 * Created on 7 de agosto de 2022, 12:14
 */

#ifndef EFSM_H
#define EFSM_H

#include "../../kernel/simulator/ModelDataDefinition.h"
#include "../../kernel/simulator/PluginInformation.h"
#include "../../kernel/simulator/Attribute.h"
#include "../data/AssignmentItem.h"
#include "../data/Variable.h"
#include "../../kernel/util/List.h"
#include "../components/FSM_Transition.h"
#include "../components/FSM_State.h"
#include <vector>
#include <map>
#include <string>

class FSM_State;

class ExtendedFSM : public ModelDataDefinition {
public: /// constructors
    ExtendedFSM(Model* model, std::string name = "");
    virtual ~ExtendedFSM() = default;

public: /// new public user methods for this component
    std::vector<Variable*>* getVariables() {
         return _variables;
    }
    void enableParallelism(bool on) {
		_parallelismEnabled = on;	 
	}
	bool isParallelEnabled() const { return _parallelismEnabled; }
	bool hasReturnedToComponent() const { return _returnedToComponent; }

    std::string getCurrentState();

    void setInitialState(FSM_State* state);
	void updateCurrentStates(FSM_State* oldState, std::vector<FSM_State*> newStates);
    
    void insertVariable(Variable* variable);
    void enterEFSM(Entity* entity, ModelComponent* returnState);
    void leaveEFSM(Entity* entity, FSM_State* newCurrentState);
    void reset();

public: /// virtual public methods
    virtual std::string show();
	// NOTE this should really not be public, but only accessible through FSM_State.
	// Due to lack of time, I am setting it here, though
	// A better alternative would perhaps be composition using Friend Classes
    void setCurrentState(FSM_State* state);

public: /// static public methods that must have implementations (Load and New just the same. GetInformation must provide specific infos for the new component
    static PluginInformation* GetPluginInformation();
    static ModelDataDefinition* LoadInstance(Model* model, PersistenceRecord *fields);
    static ModelDataDefinition* NewInstance(Model* model, std::string name = "");

protected: /// virtual protected method that must be overriden
    virtual bool _loadInstance(PersistenceRecord *fields);
    virtual void _saveInstance(PersistenceRecord *fields, bool saveDefaultValues);

protected: // could be overriden .
    virtual bool _check(std::string* errorMessage);
    virtual void _initBetweenReplications();
    virtual void _createInternalAndAttachedData();
    //virtual ParserChangesInformation* _getParserChangesInformation();

private:
    std::vector<Variable*>* _variables = new std::vector<Variable*>();
    std::vector<ModelComponent*>* _returnModels = new std::vector<ModelComponent*>();

    FSM_State* _initialState;
	std::vector<FSM_State*> _currentStates;

	bool _parallelismEnabled = false;
	bool _returnedToComponent = false;
};

#endif /* EFSM_H */

