#ifndef MPD_FILE_HANDLER_H
#define MPD_FILE_HANDLER_H

namespace ns3 {
    class MpdFileHandler {

        public:
            static MpdFileHandler* getInstance();

            MpdFileHandler();
            ~MpdFileHandler();

        private:

            static MpdFileHandler* instance;
    };
}   // end namespace ns3

#endif // MPD_FILE_HANDLER_h
