/*************************************************************************************
 * This class is a helper for TesKeyboard.
 * It implements all logic related to control buttons.
 *
 * The class doesn't know HW implementation details. It receoves information from TesKeyboard
 * and generates all control button-related events.
 *
 * This class is a source of events. It accepts information via  processCtlButton()
 * and puts respective events to the queue.
 *************************************************************************************/
#ifndef TES_CTL_BUTTON_CONTROLLER_H
#define TES_CTL_BUTTON_CONTROLLER_H
#include "TesEventQueue.h"
#include "common.h"


#define NUMBER_OF_SUMULTANEOUSLY_HANDLED_CONTROL_BUTTONS    3

struct TesCtlButtonStateMachineData {
    struct  {
        uint8_t     is_starting: 1;     // true - in the very beginning; while the debounce timeout is not expired yet
        uint8_t     is_in_progress: 1;  // true - if the SM is already processing a Control Button (there was at least one proper "press")
        uint8_t     is_pressing: 1;     // true - if the button was pressed and not released yet
        uint8_t     is_busy: 1;         // true - if this instance of the state machine is allocated to a contropl button and is processing events
        uint8_t     is_holding: 1;      // true - if the button was not released within the Hold Timeout
        uint8_t     is_stepping: 1;     // true - if we started sending 'step' (AKA autorepeat) events
        uint8_t     is_waiting: 1;      // true - if we're waiting for the next click
        uint8_t     numberOfClicks: 4;  // keeps the number of accumulated clicks; this is the number of times when the button was pressed
        uint8_t     stepEnabled: 1;     // true - if the Step (AKA Autorepeat) feature is enabled for this button
        uint8_t     doubleHoldRelease: 1;   // true - if it's required to send both "released" and "hold/step released" events
                                            //        when a held button is released.
    };
    uint8_t         ctlButtonId;        // the ID of a control button being handled by this state machine
    timer_t         buttonTimer;        // this time is used for everything: the debouncer, "hold", "step" and "click" functions

    TesCtlButtonStateMachineData(void){
        clear();
    }

    void    clear(void){
        memset(this, 0, sizeof(TesCtlButtonStateMachineData));
    }
};

class TesCtlButtonController {
public:
    TesCtlButtonController(void);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // the standard tick() method
    void    tick(void);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Processes pressing or releasing a control button
    void    processCtlButton(uint8_t button_id, bool pressed);

private:
    TesCtlButtonStateMachineData    _sm[NUMBER_OF_SUMULTANEOUSLY_HANDLED_CONTROL_BUTTONS];

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Returns an index of a Control Button State Machine allocated to the specified control button ID
    uint8_t getStateMachineIndex(uint8_t button_id);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Allocates the specified State Machine
    void allocateStateMachine(uint8_t sm_id, uint8_t button_id);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Puts a Control Button event into the event queue
    void sendEvent(uint8_t  event_type, TesCtlButtonStateMachineData * sm);

};

#endif

