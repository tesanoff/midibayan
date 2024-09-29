///////////////////////////////////////////////////////////////////////////////////////////////////
//  This class implements an event queue
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _TES_EVENT_QUEUE
#define _TES_EVENT_QUEUE

#include <Arduino.h>

///////////////////////////////////////////////////////
/////// We need to define some aux stuff first. ///////

// "Empty" means there's no event here (it was cleared by an event handler
// who decided that the event was fully processed and nobody needs it anymore).
enum {tesEmpty, tesEvButton, tesEvPressure, tesEvControlButton, tesEvFreeBass};

enum {tesBeReleased, tesBePressed, tesBeHold};

struct tesEvent {
    uint8_t eventType;
    union {
        // tesEvButton & tesEvControlButton share the same payload
        struct {
            uint8_t buttonEvent ;       // (see enum defined above)
            uint8_t buttonId;           // the unique ID of either a music or control button (music & control buttons have different namespaces)
        };
        // tesEvPressure
        struct {
            uint8_t newPressureValue;      // normalized to 0..127
        };
        // tesEvFreeBass
        struct {
            bool    freeBassOn;
        };
    };
};

#define TES_MAX_QUEUE_SIZE 12

/////////////////////////////////////////////////////////////////////////////////////////////////
// This is the Event Queue class
class TesEventQueue {
public:
    ////////////////////////////////////////////////////////////////////////////////
    // construstor
    TesEventQueue(void);

    ////////////////////////////////////////////////////////////////////////////////
    // puts an event to the head of the queue
    //
    // Returns:
    // - true - successfully done
    // (!) It doesn't return control if queue overflow occures
    bool pushEvent(tesEvent *event);

    ////////////////////////////////////////////////////////////////////////////////
    // returns True if the queue is empty
    bool isEmpty(void);

    ////////////////////////////////////////////////////////////////////////////////
    // returns an event from the tail of the queue
    tesEvent* pullEvent(void);

private:
    tesEvent    _event[ TES_MAX_QUEUE_SIZE  ];
    uint8_t     _head;
    uint8_t     _tail;
};

#endif

