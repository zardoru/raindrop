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
        } MessageKind;

        int Param;
        char Path[256];

        Message()
        {
            MessageKind = MSG_NULL;
        }
    };

    bool IsInstanceAlreadyRunning();
    void SetupMessageQueue();
    void SendMessageToQueue(const Message *Msg);
    Message PopMessageFromQueue();
    void RemoveQueue();
}