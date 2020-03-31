#include "utopia/core/logging.hh"
#include <H5Fpublic.h>
#include <H5Ppublic.h>
#define BOOST_TEST_MODULE hdfidentifier_test

#include <boost/test/included/unit_test.hpp> // for unit tests
#include <hdf5.h>
#include <utopia/data_io/hdfidentifier.hh>

using namespace Utopia::DataIO;

BOOST_AUTO_TEST_CASE(constructor_and_refcount_test)
{
    Utopia::setup_loggers();
    HDFIdentifier id;
    BOOST_TEST(id.get_id() == -1);
    BOOST_TEST(id.get_refcount() == -1);
    BOOST_TEST(id.is_valid() == false);

    id.increment_refcount();
    BOOST_TEST(id.get_refcount() == -1);

    id.decrement_refcount();
    BOOST_TEST(id.get_refcount() == -1);

    HDFIdentifier fileid(
        H5Fcreate(
            "identifier_testfile.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT),
        &H5Fclose);

    BOOST_TEST(fileid.get_refcount() == 1);

    HDFIdentifier copied_fileid(fileid);
    BOOST_TEST(fileid.get_refcount() == 2);
    BOOST_TEST(copied_fileid.get_refcount() == 2);
    BOOST_TEST(copied_fileid.get_id() == fileid.get_id());

    HDFIdentifier moved_fileid(std::move(copied_fileid));
    BOOST_TEST(fileid.get_refcount() == 2);
    BOOST_TEST(moved_fileid.get_refcount() == 2);
    BOOST_TEST(moved_fileid.get_id() == fileid.get_id());
    BOOST_TEST(copied_fileid.get_id() == -1);
    BOOST_TEST(copied_fileid.get_refcount() == -1);

    HDFIdentifier copyassigned = fileid;
    BOOST_TEST(fileid.get_refcount() == 3);
    BOOST_TEST(copyassigned.get_refcount() == 3);
    BOOST_TEST(copyassigned.get_id() == fileid.get_id());

    HDFIdentifier moveassigned = std::move(copyassigned);
    BOOST_TEST(fileid.get_refcount() == 3);
    BOOST_TEST(moveassigned.get_refcount() == 3);
    BOOST_TEST(moveassigned.get_id() == fileid.get_id());
    BOOST_TEST(copyassigned.get_id() == -1);
    BOOST_TEST(copyassigned.get_refcount() == -1);

    moveassigned.close();
    BOOST_TEST(not moveassigned.is_valid());
    BOOST_TEST(fileid.is_valid());
    BOOST_TEST(moved_fileid.is_valid());

    BOOST_TEST(fileid.get_refcount() == 2);
    BOOST_TEST(moved_fileid.get_refcount() == 2);
    BOOST_TEST(moveassigned.get_id() == -1);
    BOOST_TEST(moveassigned.get_refcount() == -1);

    moved_fileid.close();
    BOOST_TEST(fileid.get_refcount() == 1);
    BOOST_TEST(not moved_fileid.is_valid());

    HDFIdentifier groupid(H5Gcreate(fileid.get_id(),
                                    "/testobject",
                                    H5P_DEFAULT,
                                    H5P_DEFAULT,
                                    H5P_DEFAULT),
                          &H5Gclose);

    bool inequalitytest = (fileid != groupid);

    BOOST_TEST(inequalitytest);

    fileid.close();
    BOOST_TEST(fileid.get_refcount() == -1);
    BOOST_TEST(fileid.get_id() == -1);
}
