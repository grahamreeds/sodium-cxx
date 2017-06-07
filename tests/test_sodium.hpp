/**
 * Copyright (c) 2012-2016, Stephen Blackheath and Anthony Jones
 * Released under a BSD3 licence.
 *
 * C++ implementation courtesy of International Telematics Ltd.
 */
#ifndef _TEST_SODIUM_HPP_
#define _TEST_SODIUM_HPP_

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>
#include <sodium/sodium.hpp>
#include <string>

class test_sodium : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(test_sodium);
    // stream tests
    CPPUNIT_TEST(stream1);
    CPPUNIT_TEST(map);
    CPPUNIT_TEST(map_optional);
    CPPUNIT_TEST(merge_non_simultaneous);
    CPPUNIT_TEST(filter);
    CPPUNIT_TEST(filter_optional1);
    CPPUNIT_TEST(loop_stream1);
    CPPUNIT_TEST(loop_stream2);
    CPPUNIT_TEST(gate1);
    CPPUNIT_TEST(once1);
    CPPUNIT_TEST(collect1);
    CPPUNIT_TEST(accum1);
    // behaviour tests
    CPPUNIT_TEST(collect2);
    CPPUNIT_TEST(hold1);
    CPPUNIT_TEST(snapshot1);
    CPPUNIT_TEST(snapshot2);
    CPPUNIT_TEST(value1);
    CPPUNIT_TEST(value_const);
    CPPUNIT_TEST(constant_cell);
    CPPUNIT_TEST(value_then_map);
    CPPUNIT_TEST(value_then_snapshot);
    CPPUNIT_TEST(value_then_merge);
    CPPUNIT_TEST(value_then_filter1);
    CPPUNIT_TEST(value_then_filter2a);
    CPPUNIT_TEST(value_then_filter2b);
    CPPUNIT_TEST(value_then_once);
    CPPUNIT_TEST(value_late_listen);
    CPPUNIT_TEST(value_then_switch);
    CPPUNIT_TEST(mapB1);
    CPPUNIT_TEST(mapB_late_listen);
    CPPUNIT_TEST(apply1);
    CPPUNIT_TEST(lift1);
    CPPUNIT_TEST(lift_glitch);
    CPPUNIT_TEST(hold_is_delayed);
    CPPUNIT_TEST(switch_c1);
    CPPUNIT_TEST(switch_s1);
    CPPUNIT_TEST(loop_cell);
    CPPUNIT_TEST(split1);
    CPPUNIT_TEST(add_cleanup1);
    CPPUNIT_TEST(add_cleanup2);
    CPPUNIT_TEST(constant_value);
    CPPUNIT_TEST(loop_value);
    CPPUNIT_TEST(loop_value_snapshot);
    CPPUNIT_TEST(loop_value_hold);
    CPPUNIT_TEST(lift_loop);
    CPPUNIT_TEST(loop_switch_s);
    CPPUNIT_TEST(detach_sink);
    CPPUNIT_TEST(move_semantics);
    //CPPUNIT_TEST(move_semantics_sink);
    CPPUNIT_TEST(move_semantics_hold);
    CPPUNIT_TEST(lift_from_simultaneous);
    CPPUNIT_TEST(stream_sink_combining);
    CPPUNIT_TEST(cant_send_in_handler);
    CPPUNIT_TEST(router1);
    CPPUNIT_TEST(router2);
    CPPUNIT_TEST(router_loop1);
    CPPUNIT_TEST_SUITE_END();

public:
    virtual void tearDown();

    void stream1();
    void map();
    void map_optional();
    void merge_non_simultaneous();
    void coalesce();
    void filter();
    void filter_optional1();
    void loop_stream1();
    void loop_stream2();
    void gate1();
    void once1();
    void collect1();
    void accum1();
    void collect2();
    void hold1();
    void snapshot1();
    void snapshot2();
    void value1();
    void value_const();
    void constant_cell();
    void value_then_map();
    void value_then_coalesce();
    void value_then_snapshot();
    void value_then_merge();
    void value_then_filter1();
    void value_then_filter2a();
    void value_then_filter2b();
    void value_then_once();
    void value_late_listen();
    void value_then_switch();
    void mapB1();
    void mapB_late_listen();
    void apply1();
    void lift1();
    void lift_glitch();
    void hold_is_delayed();
    void switch_c1();
    void switch_s1();
    void loop_cell();
    void split1();
    void add_cleanup1();
    void add_cleanup2();
    void constant_value();
    void loop_value();
    void loop_value_snapshot();
    void loop_value_hold();
    void lift_loop();
    void loop_switch_s();
    void detach_sink();
    void move_semantics();
    //void move_semantics_sink();
    void move_semantics_hold();
    void lift_from_simultaneous();
    void stream_sink_combining();
    void cant_send_in_handler();
    void router1();
    void router2();
    void router_loop1();
};

#endif

