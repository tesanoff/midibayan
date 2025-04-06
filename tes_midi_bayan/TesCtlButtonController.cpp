#include "TesCtlButtonController.h"
#include "swer.h"
/*
 * This module implements the same logic as in the EncButton library from Alex Gyver.
 * 
 * This is a link to the description: https://github.com/GyverLibs/EncButton/tree/main?tab=readme-ov-file#%D0%BE%D0%B1%D1%80%D0%B0%D0%B1%D0%BE%D1%82%D0%BA%D0%B0-%D0%BA%D0%BD%D0%BE%D0%BF%D0%BA%D0%B8
 * This is a link to a simulatiom: https://wokwi.com/projects/373591584298469377
 *
 * Please study those materials in order to understand how the whole thing works.
 *
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
/// settings

// ability to suppress debug messages in this module
#define SUPPRESS_DEBUG

#ifdef SUPPRESS_DEBUG
#undef PRINT_1
#undef PRINT_2
#undef PRINT_2_HEX
#define PRINT_1(value)
#define PRINT_2(text,value)
#define PRINT_2_HEX(text,value)
#endif

// key constants
#define DEBOUNCE_TIMEOUT    20
#define CLICK_TIMEOUT       250
#define HOLD_TIMEOUT        (1000 * 3/4)
#define STEP_TIMEOUT        (1000 * 1/10)

// Control buttons are not equal: some of them may have the "fast repeat" (AKA "step") function, some of them may not.
// This is configured here, in the bitmap below.
// 1 - "fast repeat" is enabled
// 0 - "fast repeat" is disabled
//                                                       7      0   15      8   23     16
static const uint8_t   ctlButtonFastRepeatBitmap[] = { 0b00001111, 0b00000000, 0b00110000};
//                                                                               ~~   <-- not used

// In the original library, if a button is released after "hold", two events are generated:
//  * "released"
//  * and then "hold released"
//
// This may not be always convenient. Sometimes you don't need this double event.
//
// This is configured here, in the bitmap below.
// 1 - double hold release is enabled
// 0 - double hold release is disabled
//
// The setting affects both "step released" and "hold released" events
//                                                              7      0   15      8   23     16
static const uint8_t   ctlButtonDoubleHoldReleaseBitmap[] = { 0b00000000, 0b00111111, 0b00000000};
//                                                                                      ~~   <-- not used

///////////////////////////////////////////////////////////////////////////////////////////////////
TesCtlButtonController::TesCtlButtonController(TesEventQueue *queue)
    :_queue(queue)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////
// the standard tick() method
void    TesCtlButtonController::tick(void){
    timer_t millis_snapshot = millis();

    for(int i=0; i<NUMBER_OF_SUMULTANEOUSLY_HANDLED_CONTROL_BUTTONS; i++){
        TesCtlButtonStateMachineData * sm   = _sm + i;

        // check if the SM is working
        if(sm->is_busy){
            if (sm->is_starting){
                if(millis_snapshot - sm->buttonTimer > DEBOUNCE_TIMEOUT){
                    // *** consider the button pressed
                    // send respective events ('pressed', etc.)
                    PRINT_2("Sending 'pressed' event for button #", sm->ctlButtonId);
                    sendEvent(tesBePressed, sm);
                    // adjust SM data
                    sm->is_in_progress  = true;
                    sm->is_waiting      = false;
                    sm->is_starting     = false;
                    PRINT_2("Switching to the 'is_pressing' state for button #", sm->ctlButtonId);
                    sm->is_pressing     = true;
                    PRINT_2("Starting the 'hold' timer for button #", sm->ctlButtonId);
                    sm->buttonTimer     = millis_snapshot;  // now - for the 'hold' function
                    sm->numberOfClicks++;       // we count clicks when we're sure the button was pressed
                }
            }
            else if(sm->is_waiting){
                if(millis_snapshot - sm->buttonTimer > CLICK_TIMEOUT){
                    PRINT_2("Click timeout for button #", sm->ctlButtonId);
                    PRINT_2("\tN of clicks = ", sm->numberOfClicks)
                    // send respective events
                    PRINT_1("sending 'click' event");
                    sendEvent(tesBeClick, sm);
                    // release the SM
                    PRINT_2("releasing SM#", i);
                    sm->is_busy = false;
                }
            }
            else if(sm->is_pressing){
                if(sm->is_holding){
                    if((sm->stepEnabled) && (millis_snapshot - sm->buttonTimer > STEP_TIMEOUT)){
                        sm->is_stepping     = true;
                        // send respective events
                        PRINT_1("sending 'step' event");
                        sendEvent(tesBeStep, sm);
                        sm->buttonTimer = millis_snapshot;
                    }
                }
                else {
                    if(millis_snapshot - sm->buttonTimer > HOLD_TIMEOUT){
                        PRINT_2("Hold timeout for button #", sm->ctlButtonId);
                        // send respective events
                        PRINT_1("sending 'hold' event");
                        sendEvent(tesBeHold, sm);
                        PRINT_2("\tN of clicks = ", sm->numberOfClicks)
                        sm->is_holding  = true;
                        sm->buttonTimer = millis_snapshot;
                    }
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Processes pressing or releasing a control button
void    TesCtlButtonController::processCtlButton(uint8_t button_id, bool pressed){
    PRINT_2("*** ctl button #", button_id);
    TesCtlButtonStateMachineData * sm   = _sm + getStateMachineIndex(button_id);
    timer_t millis_snapshot = millis();

    if(pressed){
        // set proper values in the SM data record
        sm->is_starting = true;
        // we do not set 'is_pressing' - it's set after the debounce timer
        PRINT_1("starting 'debounce/hold' timer");
        sm->buttonTimer = millis_snapshot;
    }
    else{
        // *** released
        // handle release during the debounce timer
        if (sm->is_starting){
            PRINT_2("debouncing button #", sm->ctlButtonId);
            if (sm->is_in_progress){
                sm->is_starting     = false;
            }
            else {
                // just stop processing the button and release the SM
                PRINT_1("Debounce handler:\treleasing SM");
                sm->clear();
            }
        }
        else {
            if((!sm->is_holding) || (sm->doubleHoldRelease)){
                PRINT_1("sending 'released' event");
                // send 'released' event
                sendEvent(tesBeReleased, sm);
            }

            // adjust the states
            sm->is_pressing     = false;
            if(sm->is_holding){
                if(sm->is_stepping){
                    PRINT_1("sending 'released step' event");
                    PRINT_2("\tN of clicks = ", sm->numberOfClicks)
                    // send 'released step' event
                    sendEvent(tesBeStepReleased, sm);
                }
                else {
                    PRINT_1("sending 'released hold' event");
                    PRINT_2("\tN of clicks = ", sm->numberOfClicks)
                    // send 'released hold' event
                    sendEvent(tesBeHoldReleased, sm);
                }
                // *** we do not wait for next clicks after Hold
                // just stop processing the button and release the SM
                PRINT_1("releasing SM");
                sm->clear();
            }
            else {
                sm->is_waiting      = true;
                PRINT_1("starting 'click' timer");
                sm->buttonTimer     = millis_snapshot;
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////
// Returns an index of a Control Button State Machine allocated to the specified control button ID
uint8_t TesCtlButtonController::getStateMachineIndex(uint8_t button_id){
    // try to find an already allocated SM
    uint8_t first_available_sm = 0xFF;
    for(int i=0; i<NUMBER_OF_SUMULTANEOUSLY_HANDLED_CONTROL_BUTTONS; i++){
        if((_sm[i].ctlButtonId == button_id) && (_sm[i].is_busy)){
            // we have found already allocated SM
            PRINT_2("found allocated SM #", i);
            return i;
        }
        if((first_available_sm == 0xFF) && (!_sm[i].is_busy)){
            first_available_sm = i;
        }
    }
    if(first_available_sm == 0xFF){
        // We didn't find an already allocated SM and we didn't manage to find a free one
        SWER(swerCtlButtonController01);
    }
    allocateStateMachine(first_available_sm, button_id);
    return first_available_sm;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Allocates the specified State Machine
void TesCtlButtonController::allocateStateMachine(uint8_t sm_id, uint8_t button_id){
    PRINT_2("allocating SM #", sm_id);
    PRINT_2("\tto button #", button_id);
    _sm[sm_id].clear();
    _sm[sm_id].is_busy              = true;
    _sm[sm_id].ctlButtonId          = button_id;
    _sm[sm_id].stepEnabled          = bitmap_get(ctlButtonFastRepeatBitmap, button_id);
    _sm[sm_id].doubleHoldRelease    = bitmap_get(ctlButtonDoubleHoldReleaseBitmap, button_id);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Puts a Control Button event into the event queue
void TesCtlButtonController::sendEvent(uint8_t  event_type, TesCtlButtonStateMachineData * sm){
    tesEvent    ev;
    ev.eventType        = tesEvGyverButton;
    ev.buttonEvent      = event_type;
    ev.buttonId         = sm->ctlButtonId;
    ev.clicks           = sm->numberOfClicks;
    _queue->pushEvent(&ev);
}

