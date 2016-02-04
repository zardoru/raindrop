#pragma once

namespace NoteLoader
{
    // user responsability to clean this one up.
    dotcur::Song *LoadObjectsFromFile(std::string filename, std::string prefix = "");
};
