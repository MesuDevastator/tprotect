// main.cpp: The Entry Point

#include <cstdlib>
#include <print>

#include <tprotect/gui.hpp>

int main()
{
    using namespace tprotect;
    return gui::create(900, 720, "TProtect") // create singleton
        .and_then([] {                       // if succeeding, enter the main loop and destroy the singleton
            const auto result{gui::instance().main_loop()};
            gui::destroy();
            return result;
        })
        .transform([] { return EXIT_SUCCESS; }) // if exiting normaly, return EXIT_SUCESS
        .or_else([](const std::string &error) { // if exiting abnormally, print the error and return EXIT_FAILURE
            std::println(stderr, "[GUI] {}", error);
            return std::expected<int, std::string>{EXIT_FAILURE};
        })
        .value_or(EXIT_FAILURE);
}
