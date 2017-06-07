/**
 * Copyright (c) 2012-2014, Stephen Blackheath and Anthony Jones
 * Released under a BSD3 licence.
 *
 * C++ implementation courtesy of International Telematics Ltd.
 */
#ifndef _SODIUM_UNIT_HPP_
#define _SODIUM_UNIT_HPP_

namespace sodium {
    /*!
     * A unit (or "nothing") type and value.
     */
    struct unit {
        unit() {}
        bool operator == (const unit&) const { return true; }
        bool operator != (const unit&) const { return false; }
    };
};

#endif

