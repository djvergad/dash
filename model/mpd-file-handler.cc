#include "mpd-file-handler.h"


namespace ns3 {

MpdFileHandler *MpdFileHandler::instance = 0;


MpdFileHandler::MpdFileHandler()
{

}

MpdFileHandler::~MpdFileHandler()
{

}

MpdFileHandler* MpdFileHandler::getInstance()
{
    if (instance == 0) {
        instance = new MpdFileHandler();
    }
    return instance;
}

}   // endnamespace ns3
