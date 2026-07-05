#pragma once

/*
    Raindrop IPC facilities. Mainly for use with VSRG preview-mode.
*/

namespace IPC
{
    struct Message
    {
        enum EMessageKind
        {
            MSG_NULL,
            MSG_STOP,
            MSG_STARTFROMMEASURE
        } message_class;

        int param;
        char Path[256];

        Message()
        {
            message_class = MSG_NULL;
        }
    };

    bool IsInstanceAlreadyRunning();
    void SetupMessageQueue();
    void SendMessageToQueue(const Message *Msg);
    Message PopMessageFromQueue();
    void RemoveQueue();
}