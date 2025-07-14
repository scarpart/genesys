#include "Smart_EFSM3.h"

// GEnSyS Simulator
#include "../../../../kernel/simulator/Simulator.h"
#include "../../../../kernel/simulator/EntityType.h"

// Model Components
#include "../../../../plugins/components/Create.h"
#include "../../../../plugins/components/Dispose.h"
#include "../../../../plugins/components/Assign.h"
#include "../../../../plugins/data/Variable.h"
#include "../../../../plugins/data/EFSM.h"
#include "../../../../plugins/components/FSM_State.h"
#include "../../../../plugins/components/FSM_Transition.h"
#include "../../../../plugins/components/FSM_ModalModel.h"
#include "../../../../plugins/components/Delay.h"

#include "../../../TraitsApp.h"

Smart_EFSM3::Smart_EFSM3() {
}

/**
 * Simple test for parallel non-determinism in EFSM.
 * Creates a state machine with one initial state that has multiple
 * non-deterministic transitions that can all be enabled simultaneously.
 * When parallel mode is enabled, all should fire in one trigger.
 */
int Smart_EFSM3::main(int argc, char** argv) {
    // instantiate simulator
    Simulator* genesys = new Simulator();
    genesys->getTracer()->setTraceLevel(TraitsApp<GenesysApplication_if>::traceLevel);
    setDefaultTraceHandlers(genesys->getTracer());
    PluginManager* plugins = genesys->getPlugins();
    plugins->autoInsertPlugins("autoloadplugins.txt");

    // create model
    Model* model = genesys->getModels()->newModel();
    
    // Create entity type
    EntityType* entityType = new EntityType(model, "TestEntity");
    
    // Create component
    Create* create1 = plugins->newInstance<Create>(model);
    create1->setDescription("Create Test Entity");
    create1->setEntityType(entityType);
    create1->setTimeBetweenCreationsExpression("5");
    create1->setTimeUnit(Util::TimeUnit::second);
    create1->setEntitiesPerCreation(1);
    create1->setMaxCreations(3);  // Only create 3 entities for testing
    
    // Assign initial value
    Assign* assign1 = new Assign(model);
    assign1->setDescription("Set trigger value");
    Assignment* assignment1 = new Assignment("trigger", "1");
    assign1->getAssignments()->insert(assignment1);
    create1->getConnections()->insert(assign1);
    
    // Small delay to ensure we can see the parallel behavior
    Delay* delay1 = plugins->newInstance<Delay>(model);
    delay1->setDescription("Initial delay");
    delay1->setDelayExpression("0.1");
    delay1->setDelayTimeUnit(Util::TimeUnit::second);
    assign1->getConnections()->insert(delay1);
    
    // Create EFSM with parallel mode enabled
    ExtendedFSM* efsm = plugins->newInstance<ExtendedFSM>(model, "parallel_efsm");
    efsm->enableParallelism(true);  // Enable parallel non-determinism
    
    // Create variables to track which paths were taken
    Variable* varPath1 = plugins->newInstance<Variable>(model, "path1Count");
    varPath1->setInitialValue(0.0, "path1Count");
    efsm->insertVariable(varPath1);
    
    Variable* varPath2 = plugins->newInstance<Variable>(model, "path2Count");
    varPath2->setInitialValue(0.0, "path2Count");
    efsm->insertVariable(varPath2);
    
    Variable* varPath3 = plugins->newInstance<Variable>(model, "path3Count");
    varPath3->setInitialValue(0.0, "path3Count");
    efsm->insertVariable(varPath3);
    
    efsm->CreateInternalData(efsm);
    
    // Create states
    FSM_State* stateInitial = plugins->newInstance<FSM_State>(model, "Initial");
    stateInitial->setEFSM(efsm);
    stateInitial->setAsInitialState();
    
    FSM_State* stateA = plugins->newInstance<FSM_State>(model, "StateA");
    stateA->setEFSM(efsm);
    
    FSM_State* stateB = plugins->newInstance<FSM_State>(model, "StateB");
    stateB->setEFSM(efsm);
    
    FSM_State* stateC = plugins->newInstance<FSM_State>(model, "StateC");
    stateC->setEFSM(efsm);
    
    FSM_State* stateFinal = plugins->newInstance<FSM_State>(model, "Final");
    stateFinal->setEFSM(efsm);
    stateFinal->setIsFinalState(true);
    
    // Create three non-deterministic transitions from Initial state
    // All have the same guard so they can all be enabled simultaneously
    
    FSM_Transition* transitionToA = plugins->newInstance<FSM_Transition>(model, "ToStateA");
    transitionToA->setGuardExpression("trigger == 1");
    transitionToA->setOutputActions(""); 
    transitionToA->setSetActions("path1Count = path1Count + 1");
    transitionToA->setNondeterministic(true);
    stateInitial->getConnections()->insert(transitionToA);
    transitionToA->getConnections()->insert(stateA);
    transitionToA->setImmediate(true);
    
    FSM_Transition* transitionToB = plugins->newInstance<FSM_Transition>(model, "ToStateB");
    transitionToB->setGuardExpression("trigger == 1");
    transitionToB->setOutputActions("");
    transitionToB->setSetActions("path2Count = path2Count + 1");
    transitionToB->setNondeterministic(true);
    stateInitial->getConnections()->insert(transitionToB);
    transitionToB->getConnections()->insert(stateB);
    transitionToB->setImmediate(true);
    
    FSM_Transition* transitionToC = plugins->newInstance<FSM_Transition>(model, "ToStateC");
    transitionToC->setGuardExpression("trigger == 1");
    transitionToC->setOutputActions("");
    transitionToC->setSetActions("path3Count = path3Count + 1");
    transitionToC->setNondeterministic(true);
    stateInitial->getConnections()->insert(transitionToC);
    transitionToC->getConnections()->insert(stateC);
    transitionToC->setImmediate(true);
    
    // Add transitions from intermediate states to final state
    // These demonstrate that parallel execution continues
    FSM_Transition* transitionAtoFinal = plugins->newInstance<FSM_Transition>(model, "AtoFinal");
    transitionAtoFinal->setGuardExpression("");  // Always enabled
    transitionAtoFinal->setOutputActions("");
    transitionAtoFinal->setSetActions("");
    stateA->getConnections()->insert(transitionAtoFinal);
    transitionAtoFinal->getConnections()->insert(stateFinal);
    transitionAtoFinal->setImmediate(true);
    
    FSM_Transition* transitionBtoFinal = plugins->newInstance<FSM_Transition>(model, "BtoFinal");
    transitionBtoFinal->setGuardExpression("");  // Always enabled
    transitionBtoFinal->setOutputActions("");
    transitionBtoFinal->setSetActions("");
    stateB->getConnections()->insert(transitionBtoFinal);
    transitionBtoFinal->getConnections()->insert(stateFinal);
    transitionBtoFinal->setImmediate(true);
    
    FSM_Transition* transitionCtoFinal = plugins->newInstance<FSM_Transition>(model, "CtoFinal");
    transitionCtoFinal->setGuardExpression("");  // Always enabled
    transitionCtoFinal->setOutputActions("");
    transitionCtoFinal->setSetActions("");
    stateC->getConnections()->insert(transitionCtoFinal);
    transitionCtoFinal->getConnections()->insert(stateFinal);
    transitionCtoFinal->setImmediate(true);
    
    // Create modal model
    FSM_ModalModel* modalModel = plugins->newInstance<FSM_ModalModel>(model, "parallel_modal");
    modalModel->setEFSMData(efsm);
    delay1->getConnections()->insert(modalModel);
    
    // Dispose
    Dispose* dispose1 = plugins->newInstance<Dispose>(model);
    dispose1->setReportStatistics(true);
    modalModel->getConnections()->insert(dispose1);
    
    // Configure simulation
    model->getSimulation()->setNumberOfReplications(1);
    model->getSimulation()->setReplicationLength(30, Util::TimeUnit::second);
    model->getSimulation()->setTerminatingCondition("");
    model->getSimulation()->setReplicationReportBaseTimeUnit(Util::TimeUnit::second);
    
    // Save and run
    model->save("./models/Smart_EFSM3.gen");
    
    std::cout << "\n=== Testing Parallel Non-Determinism ===" << std::endl;
    std::cout << "Expected behavior: All three transitions should fire simultaneously" << std::endl;
    std::cout << "Watch for path1Count, path2Count, and path3Count increments" << std::endl;
    std::cout << "Current states should show multiple active states" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    model->getSimulation()->start();
    
    std::cout << "\n=== Final Results ===" << std::endl;
    std::cout << "Path 1 Count: " << model->parseExpression("path1Count") << std::endl;
    std::cout << "Path 2 Count: " << model->parseExpression("path2Count") << std::endl;
    std::cout << "Path 3 Count: " << model->parseExpression("path3Count") << std::endl;
    std::cout << "All counts should be equal if parallel non-determinism is working correctly." << std::endl;
    std::cout << "==================\n" << std::endl;
    
    // Free memory
    delete genesys;
    
    return 0;
}
