#include "../hdfgroup.hh"
#include "../hdfmockclasses.hh"
#include <cassert>
#include <iostream>
using namespace Utopia::DataIO;

void throw_stuff ()
{
    throw std::runtime_error("Bullshit");
}



// void write_dataset_onedimensional(HDFGroup& testgroup1, HDFGroup& testgroup2) {

//     // Test for constructor

//     // nothing happend until now
//     HDFDataset<HDFGroup> testdataset(testgroup2, "testdataset");
//     assert(testdataset.get_id() == -1);

//     // make a dummy dataset to which later will be written
//     hid_t dummy_dset = make_dataset_for_tests<double>(
//         testgroup1.get_id(), "/testgroup1/testdataset2", 1, {100},
//         {H5S_UNLIMITED}, 50);
//     H5Dclose(dummy_dset);
//     // open dataset again
//     HDFDataset<HDFGroup> testdataset2(testgroup1, "testdataset2");
//     hid_t dummy_dset2 =
//         H5Dopen(file.get_id(), "/testgroup1/testdataset2", H5P_DEFAULT);
//     // get its name
//     std::string name;
//     name.resize(25);
//     H5Iget_name(dummy_dset2, name.data(), name.size());
//     H5Dclose(dummy_dset2);
//     name.pop_back(); // get rid of superfluous \0 hdf5 writes in there
//     std::string dsetname = testdataset2.get_parent().lock()->get_path() + "/" +
//                            testdataset2.get_name();
//     // check if name is correct
//     assert(name == dsetname);

//     std::vector<double> data(100, 3.14);
//     for (std::size_t i = 0; i < data.size(); ++i) {
//         data[i] += i;
//     }
//     // test write: most simple case
//     testdataset.write(data.begin(), data.end(),
//                       [](auto &value) { return value; });

//     for (auto &value : data) {
//         value = 6.28;
//     }
//     testdataset2.write(data.begin(), data.end(),
//                        [](auto &value) { return value; });

//     HDFDataset<HDFGroup> compressed_dataset(testgroup1, "compressed_dataset");
//     compressed_dataset.write(data.begin(), data.end(),
//                              [](auto &value) { return value; }, 1,
//                              {data.size()}, {}, 20, 5);

//     compressed_dataset.close();

//     HDFDataset<HDFGroup> compressed_dataset2(testgroup1, "compressed_dataset");

//     for (auto &value : data) {
//         value = 3.14 / 2;
//     }
//     compressed_dataset2.write(data.begin(), data.end(),
//                               [](auto &value) { return value; });

//     // 2d data
//     std::vector<std::vector<double>> data_2d(100,
//                                              std::vector<double>(10, 3.16));

//     // 1d dataset variable length

//     HDFDataset<HDFGroup> varlen_dataset(testgroup1, "varlendataset");
//     varlen_dataset.write(
//         data_2d.begin(), data_2d.end(),
//         [](auto &value) -> std::vector<double> & { return value; }, 1, {100});

//     // nd dataset, extendible
// }




int main() {

    try
    {
        // open a file
        HDFFile file("group_test.h5", "w");

        // open two groups 
        auto group = HDFGroup(file.get_basegroup(), "/testgroup");
        auto group2 = HDFGroup(file.get_basegroup(), "/testgroup2");


        // write_dataset_onedimensional(group, group2);


        // close group2 
        group2.close();

        auto group3 = HDFGroup(file.get_basegroup(), "/testgroup3");


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


        auto dataset = group3.open_dataset("bla");



        std::vector<double> data(100, 3.14);
        for (std::size_t i = 0; i < data.size(); ++i) {
            data[i] += i;
        }
        // test write: most simple case
        dataset->write(data.begin(), data.end(),
                        [](auto &value) { return value; });

        // HDFDataset<HDFGroup> compressed_dataset(testgroup1, "compressed_dataset");
        // compressed_dataset.write(data.begin(), data.end(),
        //                         [](auto &value) { return value; }, 1,
        //                         {data.size()}, {}, 20, 5);


        group3.close_dataset("bla");


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