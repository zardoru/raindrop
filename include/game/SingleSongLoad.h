

namespace rd {
    struct loaderVSRGEntry_t {
        const char *Ext;

        void (*LoadFunc)(const std::filesystem::path &filename, Song *Out);
    };

    std::shared_ptr<Song> LoadSongFromFile(std::filesystem::path filename);
}
