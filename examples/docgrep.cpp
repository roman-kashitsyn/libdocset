#include "docset.hpp"
#include <iostream>
#include <iomanip>

void usage(const char *progname)
{
    std::cerr << progname << " query files..." << std::endl;
}

int main(int argc, const char *argv[])
{
    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }

    int num_errors = 0;
    const char *query = argv[1];

    for (const char **path = argv + 2, **end = argv + argc;
            path != end;
            ++path) {
        try {
            docset::doc_set ds(*path);
            for (const auto &e : ds.find(query)) {
                std::cout << std::setw(10) << ds.name()
                          << ": "
                          << "(" << e.canonical_type_name()[0] << ") "
                          << std::left << std::setw(25) << e.name()
                          << ": "
                          << "file://" << ds.basedir()
                          << "/Contents/Resources/Documents/" << e.path()
                          << "\n";
            }
        } catch (const docset::error &e) {
            ++num_errors;
            std::cerr << *path << ": " << e.what() << std::endl;
        }
    }

    return num_errors;
}
