#include <utopia/data_io/hdffile.hh>
#include "utopia/data_io/hdfdataset.hh"

using namespace Utopia::DataIO;
using namespace std::literals::chrono_literals;

int main(){
    Utopia::setup_loggers();

    HDFFile file("memorytestfile.h5", "w");
    auto dataset = file.open_dataset("/testdata");

    // write many times in a row
    for(std::size_t i = 0; i < 1000000; ++i)
    {
        dataset->write(i);
    }
    return 0;
}
