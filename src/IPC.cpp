#include "pch.h"

#include "IPC.h"

using namespace boost::interprocess;
using namespace IPC;
bool message_queue_is_ours = false;
message_queue *mque = NULL;

namespace IPC {
    const char* PROCESS_QUEUE_NAME = "grdpMsgQue";

    void InitializeMessageQueue()
    {
        if (mque) return; // already initialized

        try {
            mque = new message_queue(create_only, PROCESS_QUEUE_NAME, 5, sizeof(Message));
            message_queue_is_ours = true;
        }
        catch (interprocess_exception &) // queue already exists
        {
            delete mque;
            mque = new message_queue(open_only, PROCESS_QUEUE_NAME);
#ifndef NDEBUG
            message_queue_is_ours = true; // debug builds ALWAYS GET THE QUEUE
#endif
        }
    }

    void Cleanup()
    {
        if (message_queue_is_ours)
            mque->remove(PROCESS_QUEUE_NAME);
        delete mque;
    }

    bool IsInstanceAlreadyRunning()
    {
        SetupMessageQueue();
        return !message_queue_is_ours; // we didn't create it so it's not ours
    }

    void SetupMessageQueue()
    {
        static bool Initialized = false;

        if (!Initialized)
        {
            InitializeMessageQueue();
            atexit(IPC::Cleanup);
            Initialized = true;
        }
    }

    void RemoveQueue()
    {
        SetupMessageQueue();
        if (mque)
            mque->remove(PROCESS_QUEUE_NAME);
    }

    void SendMessageToQueue(const Message *Msg)
    {
        if (mque)
        {
            mque->try_send(Msg, sizeof(Message), 0);
        }
    }

    Message PopMessageFromQueue()
    {
        Message Msg;

        if (mque)
        {
            size_t st;
            unsigned int prio;

            if (!mque->try_receive(&Msg, sizeof(Message), st, prio))
            {
                Msg.MessageKind = Message::MSG_NULL;
            }
        }

        return Msg;
    }
}