#include "../hdfgroup.hh"
#include "../hdfmockclasses.hh"
#include <cassert>
#include <iostream>
using namespace Utopia::DataIO;

void throw_stuff ()
{
    throw std::runtime_error("Bullshit");
}

int main() {

    try
    {
        // open a file
        HDFFile file("testfile2.h5", "w");

        // open two groups 
        auto group = HDFGroup(file.get_basegroup(), "/testgroup");
        auto group2 = HDFGroup(file.get_basegroup(), "/testgroup2");

        // close group2 
        group2.close();


        // open subgroups
        std::shared_ptr<HDFGroup> subgroup = group.open_group("first");

        // copy subgroup and test whether the number of children groups
        // is one higher in the copy if another child group is added 
        HDFGroup group_copy = *subgroup;

        // assert(group_copy.get_open_groups().size ==subgroup->get_open_groups().size + 1);

        subgroup->open_group("second");
        subgroup->close_group("second");
        subgroup->open_group("second");
        group.close_group("first");


        // group_copy.close_group("first");

        // try{
        //     group_copy.close_group("third");
        //     throw_stuff();
        // }
        // catch(std::runtime_error& e){
        //     if (e.what() != "Trying to delete a nonexistant or closed group!")
        //         throw_stuff();
        // }
        // catch(...){
        //     throw_stuff();
        // }

        return 0;
    
    } catch(...)
    {
        return 1;
    }

}