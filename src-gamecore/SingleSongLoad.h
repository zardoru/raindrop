

namespace Game {
    namespace VSRG {
        struct loaderVSRGEntry_t
        {
            const char* Ext;
            void(*LoadFunc) (std::filesystem::path filename, Song* Out);
        };

        std::shared_ptr<Song> LoadSongFromFile(std::filesystem::path filename);
    }
}
