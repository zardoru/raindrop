#include <iostream>
#include <filesystem>
#include <boost/program_options.hpp>

#include <rmath.h>
#include <game/NoteLoader7K.h>
#include <game/Converter.h>
#include <fstream>

const auto VERSION = "1.0";

namespace po = boost::program_options;
po::variables_map vm; /* args */
std::filesystem::path in_path;
std::filesystem::path out_path;

std::string Author;

// VSRG-Specific
enum class CONVERTMODE
{
    CONV_BMS,
    CONV_UQBMS,
    CONV_SM,
    CONV_OM,
    CONV_NPS,
    CONV_ACCTEST
} ConvertMode;

void convert() {
    auto Sng = LoadSongFromFile(in_path);

    if (Sng && !Sng->Difficulties.empty())
    {
        std::cout << "Initiating conversion for mode " << (int)ConvertMode << std::endl;
        if (ConvertMode == CONVERTMODE::CONV_OM) // for now this is the default
            ConvertToOM(Sng.get(), out_path, Author);
        else if (ConvertMode == CONVERTMODE::CONV_BMS)
            ConvertToBMS(Sng.get(), out_path);
        else if (ConvertMode == CONVERTMODE::CONV_UQBMS)
            ExportToBMSUnquantized(Sng.get(), out_path);
        else if (ConvertMode == CONVERTMODE::CONV_NPS)
            ConvertToNPSGraph(Sng.get(), out_path);
        else if (ConvertMode == CONVERTMODE::CONV_ACCTEST)
        {
            auto msr = Sng->Difficulties[0]->Data->Measures;
            auto timeset = std::set<double>();
            for (const auto& m : msr) {
                for (auto i = 0; i < Sng->Difficulties[0]->Channels; i++)
                    for (auto note : m.Notes[i])
                        if (note.NoteKind == 0)
                            timeset.insert(note.StartTime);
            }

            if (!out_path.empty()) {
                std::cout << "Attempt to open " << std::filesystem::absolute(out_path) << "..." << std::endl;
                std::fstream out(out_path, std::ios::out);

                if (!out.is_open()) {
                    std::cout << "Couldn't open!?\n";
                }

                out << std::setprecision(17) << std::fixed;
                for (auto d : timeset)
                    out << d << std::endl;
            }
            else {
                std::cout << std::setprecision(17) << std::fixed;
                for (auto d : timeset) {
                    std::cout << d << std::endl;
                }
            }
        }
        else
            ConvertToSMTiming(Sng.get(), out_path);
    }
    else
    {
        if (Sng)
            std::cout << "No notes or timing were loaded.\n";
        else
            std::cout << "Failure loading file.\n";
        }

}

bool parse_args(int argc, char** argv) {
    po::positional_options_description p;
    p.add("input", 1);


    po::options_description desc("Allowed options");
    desc.add_options()
            ("help,?",
             "show help message")
            ("input,i", po::value<std::string>(),
             "Input File")
            ("output,o", po::value<std::string>(),
             "Output File")
            ("format,f", po::value<std::string>(),
             "Target Format")
            ("author,a", po::value<std::string>()->default_value("raindrop"),
             "Author")
            ;


    try
    {
        po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
        return false;
    }

    po::notify(vm);
    return true;
}

bool init_converter() {
    if (vm.count("input")) {
        in_path = vm["input"].as<std::string>();
    } else {
        std::cout << "input not specified" << std::endl;
        return false;
    }

    if (vm.count("output")) {
        out_path = vm["output"].as<std::string>();
        if (!std::filesystem::exists(out_path)) {
            std::cout << "The output path does not exist." << std::endl;
            return false;
        } else if (!std::filesystem::is_directory(out_path)) {
            std::cout << "The output path must be a directory." << std::endl;
            return false;
        }
    } else {
        out_path = std::filesystem::current_path();
    }

    if (vm.count("format"))
    {
        auto modes = std::map<std::string, CONVERTMODE>{
                {"om",      CONVERTMODE::CONV_OM},
                {"sm",      CONVERTMODE::CONV_SM},
                {"bms",     CONVERTMODE::CONV_BMS},
                {"uqbms",   CONVERTMODE::CONV_UQBMS},
                {"nps",     CONVERTMODE::CONV_NPS},
                {"acctest", CONVERTMODE::CONV_ACCTEST}
        };

        try {
            ConvertMode = modes.at(vm["format"].as<std::string>());

            std::cout << "Setting format to " << vm["format"].as<std::string>().c_str()
                       << " (" << (int) ConvertMode << ")" << std::endl;
        } catch(std::out_of_range &o) {
            std::cout << "Unknown format. Valid values are:" << std::endl;
            for (const auto &p: modes) {
                std::cout << "\t" << p.first.c_str() << std::endl;
            }
        }
    }

    return true;
}

int main(int argc, char** argv) {
    std::cout << "raindrop converter " << VERSION << std::endl;
    std::cout << "Working directory: " << std::filesystem::current_path() << std::endl;

    if (!parse_args(argc, argv)) {
        // std::cout << "unknown / incompatible option supplied\n";
        return -1;
    }

    if (init_converter()) {
        try {
            convert();
            std::cout << "done." << std::endl;
        } catch(std::exception &e) {
            std::cout << e.what() << std::endl;
        }
    }

    return 0;
}