#pragma once

namespace NoteLoaderDC
{
    // user responsability to clean this one up.
    Game::dotcur::Song *LoadObjectsFromFile(std::filesystem::path filename, std::filesystem::path prefix = "");
};
