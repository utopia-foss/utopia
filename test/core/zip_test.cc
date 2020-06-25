#define BOOST_TEST_MODULE zip iterator test
#include <boost/mpl/list.hpp>       // type lists for testing
#include <boost/test/included/unit_test.hpp> // for unit tests
#include <vector> 
#include <list> 
#include <unordered_map>
#include <forward_list>
#include <deque>
#include <numeric>
#include <iterator>
#include <iostream>
#include <string>
#include <utopia/core/type_traits.hh>
#include <utopia/core/zip.hh>
#include <utopia/core/ostream.hh>

struct Containers {
    static constexpr int size = 5;
    static constexpr int hops = size - 1;

    std::vector<int> v{1,2,3,4,5};
    std::list<int> l{6, 7, 8, 9, 10};
    std::deque<int> d{10, 20, 30, 40, 50};
    std::forward_list<int> f{100, 200, 300, 400, 500};

    std::vector<int> v_cpy = v;
    std::list<int> l_cpy = l;
    std::deque<int> d_cpy = d;
    std::forward_list<int> f_cpy = f;
};


// import output operators into std, boost otherwise denies their existence
// and does not use them correctly. 
namespace std{
    template<typename... Ts>
    std::ostream& operator<<(std::ostream& out, const std::tuple<Ts...>& tpl){
        return Utopia::Utils::operator<<(out, tpl);
    }
}


BOOST_FIXTURE_TEST_SUITE(zip_iterator, Containers)

BOOST_AUTO_TEST_CASE(forward_it)
{
    using namespace Utopia::Itertools;
    const ZipIterator begin(f.begin(), l.begin());

    static_assert(std::is_same_v<
        typename std::iterator_traits<
            std::remove_cv_t<decltype(begin)>>::iterator_category,
        std::forward_iterator_tag>);

    // copy construct
    ZipIterator it(begin);
    BOOST_TEST(it == begin);

    // dereference as l-value
    *it = *begin;
    BOOST_TEST(*it == *begin);

    // increment prefix
    auto& it_next = ++it;
    BOOST_TEST(it != begin);
    BOOST_TEST(std::get<0>(*it_next) == std::get<0>(*it));

    // increment postfix
    auto it_copy = it++;
    ++it_copy;
    BOOST_TEST(std::get<0>(*it_copy) == std::get<0>(*it_next));

    const ZipIterator end(f.end(), l.end());
    BOOST_TEST(std::distance(begin, end) == size);
}

BOOST_AUTO_TEST_CASE(bidirectional_it)
{
    using namespace Utopia::Itertools;
    const ZipIterator begin(l.begin(), v.begin());

    static_assert(std::is_same_v<
        typename std::iterator_traits<decltype(begin)>::iterator_category,
        std::bidirectional_iterator_tag>);

    // copy construct
    ZipIterator it(begin);
    auto it_copy = ++it;
    ++it;

    // decrement prefix
    auto& it_next = --it;
    BOOST_TEST(std::get<0>(*it) == std::get<0>(*it_copy));
    BOOST_TEST(std::get<0>(*it_next) == std::get<0>(*it_copy));

    // decrement postfix
    auto it_copy_2 = it--;
    BOOST_TEST(std::get<0>(*it_copy_2) == std::get<0>(*it_copy));
    BOOST_TEST(std::get<0>(*it_next) == std::get<0>(*begin));
    BOOST_TEST(std::get<0>(*it) == std::get<0>(*begin));

    const ZipIterator end(l.end(), v.end());
    BOOST_TEST(std::distance(begin, end) == size);
}

BOOST_AUTO_TEST_CASE(random_access_it)
{
    using namespace Utopia::Itertools;
    const ZipIterator begin(v.begin(), v.begin());
    const ZipIterator end(v.end(), v.end());

    static_assert(std::is_same_v<
        typename std::iterator_traits<decltype(begin)>::iterator_category,
        std::random_access_iterator_tag>);

    // comparison
    BOOST_TEST(begin < end);
    BOOST_TEST(end > begin);
    BOOST_TEST(end <= end);
    BOOST_TEST(end >= end);
    BOOST_TEST(begin >= begin);
    BOOST_TEST(begin <= begin);

    // copy construct
    ZipIterator it(begin);
    auto before_end = end;
    --before_end;

    // increment
    auto it_copy = it + hops;
    BOOST_TEST(it == begin);
    BOOST_TEST(it_copy == before_end);

    // decrement
    it_copy = end - size;
    BOOST_TEST(it_copy == begin);

    // increment in-place
    auto& it_ref = (it += hops);
    BOOST_TEST(it == before_end);
    BOOST_TEST(it_ref == before_end);

    // decrement in-place
    it_ref = (it -= hops);
    BOOST_TEST(it == begin);
    BOOST_TEST(it_ref == begin);

    // random access
    auto content = it[hops];
    BOOST_TEST(content == *before_end);
    BOOST_TEST(it == begin);
    BOOST_TEST(it_ref == begin);

    // distance
    BOOST_TEST(std::distance(begin, end) == size);
}

// test zipiterator basic function
BOOST_AUTO_TEST_CASE(zipiterator_basic)
{
    Utopia::Itertools::ZipIterator begin(v.begin(), l.begin(), d.begin());
    Utopia::Itertools::ZipIterator end(v.end(), l.end(), d.end());

    constexpr bool is_category_bi = std::is_same_v<typename decltype(begin)::iterator_category, std::bidirectional_iterator_tag>;
    BOOST_TEST(is_category_bi == true);

    constexpr bool is_category_rand =
        std::is_same_v<typename decltype(begin)::iterator_category, std::random_access_iterator_tag>;

    BOOST_TEST(is_category_rand == false);

    // copying for testing
    auto vit = v_cpy.begin();
    auto lit = l_cpy.begin();
    auto dit = d_cpy.begin();

    // test forward iteration
    for (auto it = begin; it != end; ++it, ++vit, ++lit, ++dit)
    {
        auto [vv, lv, dv] = *it;
        BOOST_TEST(vv == *vit);
        BOOST_TEST(lv == *lit);
        BOOST_TEST(dv == *dit);
    }

    // test reverse iteration
    auto rbegin = std::make_reverse_iterator(end);
    auto rend = std::make_reverse_iterator(begin);

    auto rvit = std::make_reverse_iterator(v_cpy.end()); 
    auto rlit = std::make_reverse_iterator(l_cpy.end());
    auto rdit = std::make_reverse_iterator(d_cpy.end());

    for (auto it = rbegin; it != rend; ++it, ++rvit, ++rlit, ++rdit){
        auto [vv, lv, dv] = *it;
        BOOST_TEST(vv == *rvit);
        BOOST_TEST(lv == *rlit);
        BOOST_TEST(dv == *rdit);
    }

    // addition by iteration
    begin = Utopia::Itertools::ZipIterator(v.begin(), l.begin(), d.begin());
    end = Utopia::Itertools::ZipIterator(v.end(), l.end(), d.end());

    vit = v_cpy.begin();
    lit = l_cpy.begin();
    dit = d_cpy.begin();
    for (auto it = begin; it != end; ++it, ++vit, ++lit, ++dit)
    {
        auto [vv, lv, dv] = *it;

        vv += 1;
        lv -= 1;
        dv /= 2;

        BOOST_TEST(vv == *vit+1);
        BOOST_TEST(lv == *lit-1);
        BOOST_TEST(dv == *dit/2);
    }

    begin = Utopia::Itertools::ZipIterator(std::make_tuple(v.begin(), l.begin(), d.begin()));
    end = Utopia::Itertools::ZipIterator(std::make_tuple(v.end(), l.end(), d.end()));

    vit = v_cpy.begin();
    lit = l_cpy.begin();
    dit = d_cpy.begin();

    // forward iteration again with differently constructed stuff
    // mind that we modified the values of the originals in the last test
    for (auto it = begin; it != end; ++it, ++vit, ++lit, ++dit)
    {
        auto [vv, lv, dv] = *it;
        BOOST_TEST(vv == *vit + 1);
        BOOST_TEST(lv == *lit - 1);
        BOOST_TEST(dv == *dit / 2);
    }

    // check postfix increment
    std::vector<int> v1(10);
    std::vector<int> v2(10);
    std::vector<int> v3(10);

    std::iota(v1.begin(), v1.end(), -10);
    std::iota(v2.begin(), v2.end(), 0);
    std::iota(v3.begin(), v3.end(), 10);

    Utopia::Itertools::ZipIterator vectorzip(v1.begin(), v2.begin(), v3.begin());

    auto [vv1, vv2, vv3] = *vectorzip++;
    BOOST_TEST(vv1 == v1[0]);
    BOOST_TEST(vv2 == v2[0]);
    BOOST_TEST(vv3 == v3[0]);

    auto [vvv1, vvv2, vvv3] = *vectorzip;
    BOOST_TEST(vvv1 == v1[1]);
    BOOST_TEST(vvv2 == v2[1]);
    BOOST_TEST(vvv3 == v3[1]);

    // check arithmetic increment
}

// check how compatible zipiterator is with stl
BOOST_AUTO_TEST_CASE(zipiterator_stl)
{
    Utopia::Itertools::ZipIterator begin(v.begin(), l.begin(), d.begin());
    Utopia::Itertools::ZipIterator end(v.end(), l.end(), d.end());

    // std::for_each
    std::for_each(begin, end, [](auto&& xyz){
        auto [x,y,z] = xyz;
        x+=1;
        y-=1;
        z/=2;
    });

    begin = Utopia::Itertools::ZipIterator(v.begin(), l.begin(), d.begin());
    end = Utopia::Itertools::ZipIterator(v.end(), l.end(), d.end());

    auto vit = v_cpy.begin();
    auto lit = l_cpy.begin();
    auto dit = d_cpy.begin();

    for (auto it = begin; it != end; ++it, ++vit, ++lit, ++dit)
    {
        auto [vv, lv, dv] = *it;
        BOOST_TEST(vv == *vit + 1);
        BOOST_TEST(lv == *lit - 1);
        BOOST_TEST(dv == *dit / 2);
    }

    // std::transform
    begin = Utopia::Itertools::ZipIterator(v.begin(), l.begin(), d.begin());
    auto target = begin;
    end = Utopia::Itertools::ZipIterator(v.end(), l.end(), d.end());


    std::transform(begin, end, target,
                   [](auto&& xyz) {
                       auto [x, y, z] = xyz;
                       x -= 1;
                       y += 1;
                       z *= 2;

                       return std::move(xyz);
                   });

    begin = Utopia::Itertools::ZipIterator(v.begin(), l.begin(), d.begin());
    end = Utopia::Itertools::ZipIterator(v.end(), l.end(), d.end());

    vit = v_cpy.begin();
    lit = l_cpy.begin();
    dit = d_cpy.begin();

    for (auto it = begin; it != end; ++it, ++vit, ++lit, ++dit)
    {
        auto [vv, lv, dv] = *it;

        BOOST_TEST(vv == *vit );
        BOOST_TEST(lv == *lit);
        BOOST_TEST(dv == *dit);
    }

    // test std::next. Other advancers and stuff should work then, too.
    begin = Utopia::Itertools::ZipIterator(v.begin(), l.begin(), d.begin());
    end = Utopia::Itertools::ZipIterator(v.end(), l.end(), d.end());

    Utopia::Itertools::ZipIterator begin_plus_one(++v.begin(), ++l.begin(), ++d.begin());

    auto next = std::next(begin, 1);

    auto [vv, lv, dv] = *next;
    auto [vvb, lvb, dvb ] = *begin_plus_one;

    BOOST_TEST(vv == vvb);
    BOOST_TEST(lv == lvb);
    BOOST_TEST(dv == dvb);
}

// test the zip class functionality
BOOST_AUTO_TEST_CASE(zip_functionality)
{
    auto vit = v_cpy.begin();
    auto lit = l_cpy.begin();
    auto dit = d_cpy.begin();

    for(auto [x,y,z]: Utopia::Itertools::zip(v, l, d)){
        BOOST_TEST(x == *vit);
        BOOST_TEST(y == *lit);
        BOOST_TEST(z == *dit);
        ++vit;
        ++lit;
        ++dit;
    }

    vit = v_cpy.begin();
    lit = l_cpy.begin();
    dit = d_cpy.begin();
    // check that it works with const 
    for (const auto [x, y, z] : Utopia::Itertools::zip(v, l, d))
    {
        BOOST_TEST(x == *vit);
        BOOST_TEST(y == *lit);
        BOOST_TEST(z == *dit);
        ++vit;
        ++lit;
        ++dit;
    }

    // test const iteration
    auto cvit = v_cpy.cbegin();
    auto clit = l_cpy.cbegin();
    auto cdit = d_cpy.cbegin();

    auto zipper = Utopia::Itertools::zip(v, l, d);

    for (auto it = zipper.cbegin(); it != zipper.cend(); ++it, ++cvit, ++clit, ++cdit)
    {
        BOOST_TEST(std::get<0>(*it) == *cvit);
        BOOST_TEST(std::get<1>(*it) == *clit);
        BOOST_TEST(std::get<2>(*it) == *cdit);
    }


    // test reverse iteration
    auto rvit = v_cpy.rbegin();
    auto rlit = l_cpy.rbegin();
    auto rdit = d_cpy.rbegin();

    zipper = Utopia::Itertools::zip(v, l, d);

    for (auto it = zipper.rbegin(); it != zipper.rend(); ++it, ++rvit, ++rlit, ++rdit)
    {
        auto [a,b,c] = *it;

        std::cout << a << ", " << *rvit << "| ";
        std::cout << b << ", " << *rlit << "| ";
        std::cout << c << ", " << *rdit << std::endl;

        BOOST_TEST(a == *rvit);
        BOOST_TEST(b == *rlit);
        BOOST_TEST(c == *rdit);
    }

    // // test usage of zip with stl
    zipper = Utopia::Itertools::zip(v, l, d);
    std::vector<int> x;
    std::vector<int> y;
    std::vector<int> z;

    BOOST_TEST(x.size() == 0);
    BOOST_TEST(y.size() == 0);
    BOOST_TEST(z.size() == 0);

    auto target = Utopia::Itertools::ZipIterator(std::back_inserter(x), std::back_inserter(y), std::back_inserter(z));
    std::transform(zipper.begin(), zipper.end(), target, [](auto&& xyz) {
        auto [x, y, z] = xyz;
        return std::make_tuple(x, y, z);
    });

    // check if this worked correctly

    BOOST_TEST(x.size() == v.size());
    BOOST_TEST(y.size() == l.size());
    BOOST_TEST(z.size() == d.size());

    zipper = Utopia::Itertools::zip(v, l, d);
    auto tgtbegin = Utopia::Itertools::zip(x, y, z).begin();

    for(auto [v_v, l_v, d_v]: zipper){
        auto [x_v, y_v, z_v] = *tgtbegin;

        BOOST_TEST(v_v == x_v);
        BOOST_TEST(l_v == y_v);
        BOOST_TEST(d_v == z_v);
        ++tgtbegin;
    }

    // test adaptor for zip

    zipper = Utopia::Itertools::zip(v, l, d);

    auto back_insert_zip = Utopia::Itertools::adapt_zip([](auto&& container){return std::back_inserter(container);}, x, y, z);

    std::transform(zipper.begin(), zipper.end(), back_insert_zip, [](auto&& xyz) {
        auto[a, b, c] = xyz;
        return std::make_tuple(a+1, b+1, c+1);
    });

    BOOST_TEST(x.size() == 2*v.size());
    BOOST_TEST(y.size() == 2*l.size());
    BOOST_TEST(z.size() == 2*d.size());

    zipper = Utopia::Itertools::zip(v, l, d);
    tgtbegin = Utopia::Itertools::ZipIterator(std::next(x.begin(), v.size()), std::next(y.begin(), l.size()), std::next(z.begin(), d.size()));

    for (auto [v_v, l_v, d_v] : zipper)
    {
        auto [x_v, y_v, z_v] = *tgtbegin;

        BOOST_TEST(v_v+1 == x_v);
        BOOST_TEST(l_v+1 == y_v);
        BOOST_TEST(d_v+1 == z_v);
        ++tgtbegin;
    }
}

BOOST_AUTO_TEST_SUITE_END()
