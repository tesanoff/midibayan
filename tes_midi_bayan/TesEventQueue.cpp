
#include "TesEventQueue.h"
#include "swer.h"

// construstor
TesEventQueue::TesEventQueue(void){
    _head = 0;
    _tail = 0;
    _size = 0;
}

////////////////////////////////////////////////////////////////////////////////
// puts an event to the head of the queue
//
// Returns:
// - true - successfully done
// (!) It doesn't return control if queue overflow occures
bool TesEventQueue::pushEvent(tesEvent *event){
    // check if the queue is full (i.e. there's no space for another event)
    if ( _size == TES_MAX_QUEUE_SIZE ){
        // queue overflow; call SWER()
        SWER(swerEventQueue01);
    }
    // ok, let's continue

    memcpy((_event + _head++), event, sizeof(tesEvent));
    // correct the head, if required
    if( _head == TES_MAX_QUEUE_SIZE ){
        _head = 0;
    }
    _size++;    // don't forget it
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// returns True if the queue is empty
bool TesEventQueue::isEmpty(void){
    return (_size == 0);
}

////////////////////////////////////////////////////////////////////////////////
// returns an event from the tail of the queue
tesEvent* TesEventQueue::pullEvent(void){
    // return NULL if the queue is empty;
    if ( isEmpty() ){
        return NULL;
    }
    uint8_t old_tail = _tail++;
    // correct the head, if required
    if( _tail == TES_MAX_QUEUE_SIZE ){
        _tail = 0;
    }
    _size--;
    return (_event + old_tail);
}

