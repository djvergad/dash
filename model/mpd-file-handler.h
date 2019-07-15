#ifndef MPD_FILE_HANDLER_H
#define MPD_FILE_HANDLER_H

class MpdFileHandler {
    public:
        static MpdFileHandler* getInstance();
    
        ~MpdFileHandler();
        
    private:
        MpdFileHandler();
        
        static MpdFileHandler* instance;
}

#endif // MPD_FILE_HANDLER_h
