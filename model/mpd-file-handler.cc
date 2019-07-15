#include "mpd-file-handler.h"

MpdFileHandler::MpdFileHandler()
{

}

MpdFileHandler::~MpdFileHandler()
{

}

MpdFileHandler::getInstance()
{
    if (instance == 0) {
        instance = new MpdFileHandler();
    }
    return instance;
}

