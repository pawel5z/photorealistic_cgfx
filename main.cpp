#include <boost/program_options.hpp>
#include <iostream>

#include "RenderingTask.hpp"

namespace po = boost::program_options;

int main(int argc, const char *argv[]) {
    po::positional_options_description pd;
    pd.add("rtc_file", 1);

    po::options_description hidden;
    hidden.add_options()("rtc_file", po::value<std::string>(), "rendering task configuration file");

    po::options_description desc( //
        "Usage: ./raytrace [OPTION...] RTC_FILE\n"
        "Render scene specified in RTC_FILE using ray tracing.\n"
        "Options");
    desc.add_options()                                           //
        ("help,h", po::bool_switch(), "print this help message") //
        ("threads,n", po::value<int>()->default_value(-1),
         "Number of threads used for rendering. -1 (default) means number of available CPU "
         "cores.") //
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
         " left shift: \taccelerate\n"
         " left ctrl: \tdecelerate\n"
         " enter: \toverwrite rtc file camera settings with current ones\n"
         " space: \trender current view\n"
         " esc: \tquit\n");

    po::options_description cmdline_opts;
    cmdline_opts.add(desc).add(hidden);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(cmdline_opts).positional(pd).run(), vm);
    po::notify(vm);

    if (!vm.count("rtc_file") || vm.at("help").as<bool>()) {
        std::cout << desc;
        return 0;
    }

    RenderingTask rt(vm.at("rtc_file").as<std::string>(), vm.at("threads").as<int>());
    if (vm.at("preview").as<bool>())
        rt.preview();
    if (rt.renderPreview || !vm.at("preview").as<bool>())
        rt.render();
    return 0;
}
