
#include <filesystem>
#include <memory>

namespace otoworm {
    class ChartGroup;
    std::shared_ptr<ChartGroup> load_song_from_file(std::filesystem::path filename);
}
