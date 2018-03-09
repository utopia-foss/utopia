#include <iostream>
#include <cassert> 
#include "../hdffile.hh"

using namespace Utopia::DataIO;
int main(){
    HDFFile("hdf5testfile.h5", "w");
    HDFFile("hdf5testfile.h5", "r");
    HDFFile("hdf5testfile.h5", "r+");
    HDFFile("hdf5testfile.h5", "x");
    HDFFile("hdftestfile.h5", "a");
    bool caught = false;
    try{
        HDFFile("hdftestfile.h5", "n");
    }
    catch(const std::exception& e){
        caught = true;
        std::cerr << e.what() << std::endl;
    }
    if(caught == false){
        return -1;
    }


    std::cout << "hello world" << std::endl;
    return 0;
}