#include <boost/program_options.hpp>
#include <iostream>

#include "RenderingTask.hpp"

namespace po = boost::program_options;

int main(int argc, const char *argv[]) {
    po::positional_options_description pd;
    pd.add("rtc_file", 1);

    po::options_description desc( //
        "Usage: ./raytrace [OPTION...] RTC_FILE\n"
        "Render scene specified in RTC_FILE using ray tracing.\n"
        "Options");
    desc.add_options()                                           //
        ("rtc_file", "rendering task configuration file")        //
        ("help,h", po::bool_switch(), "print this help message") //
        ("preview,p", po::bool_switch(),
         "preview scene\n"
         "Controls:\n"
         " w: \tmove forward\n"
         " s: \tmove backward\n"
         " a: \tmove left\n"
         " d: \tmove right\n"
         " r: \tmove up\n"
         " f: \tmove down\n"
         " e: \troll right\n"
         " q: \troll left\n"
         " backspace: \treset view\n"
         " left shift: \tspeed up\n"
         " left ctrl: \tslow down\n"
         " enter: \toverwrite rtc file camera settings with current ones\n"
         " space: \trender current view\n"
         " esc: \tquit\n");

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).positional(pd).run(), vm);
    po::notify(vm);

    if (!vm.count("rtc_file") || vm.count("help")) {
        std::cout << desc;
        return 0;
    }

    RenderingTask rt(vm.at("rtc_file").as<std::string>());
    if (vm.at("preview").as<bool>())
        rt.preview();
    else
        rt.render();
    return 0;
}
