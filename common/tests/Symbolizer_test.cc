#include <claire/common/symbolizer/Symbolizer.h>

#include <claire/common/logging/Logging.h>

using namespace claire;

int main(int argc, char *argv[])
{
    Symbolizer s;
    StringPiece name;
    Dwarf::LocationInfo location;
    CHECK(s.Symbolize(reinterpret_cast<uintptr_t>(main), &name, &location));
    LOG(INFO) << name << " " << location.file.ToString() << " " << location.line << " ("
              << location.main_file.ToString() << ")";
    return 0;
}

