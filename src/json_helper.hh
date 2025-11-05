#ifndef TUPAL_JSON_HELPER_HH
#define TUPAL_JSON_HELPER_HH

#include <string>

#include "boost/json/object.hpp"
#include "boost/json/value.hpp"
#include "boost/date_time.hpp"

namespace tupal {

    template <typename SourceT>
    class json_helper {
    public:
        json_helper() : source_(), has_value_(false) {};
        explicit json_helper(SourceT source) : source_(source), has_value_(true) {};

        json_helper<boost::json::object> as_object() const {
            return (has_value_ && source_.is_object())
                ? json_helper<boost::json::object>(source_.as_object())
                : json_helper<boost::json::object>();
        }

        json_helper<boost::json::value> at(const std::string & key) const {
            return (has_value_ && source_.contains(key))
                ? json_helper<boost::json::value>(source_.at(key))
                : json_helper<boost::json::value>();
        }

        json_helper<std::string> as_string() const {
            return (has_value_ && source_.is_string())
                ? json_helper<std::string>(source_.as_string().c_str())
                : json_helper<std::string>();
        }

        json_helper<int> as_int() const {
            return (has_value_ && source_.is_int64())
                ? json_helper<int>(source_.as_int64())
                : json_helper<int>();
        }

        json_helper<boost::gregorian::date> as_date() const {
            auto helper = as_string();

            if (helper.has_value())
                try { return json_helper<boost::gregorian::date>(boost::gregorian::from_simple_string(helper.value())); } catch (...) {}
            
            return json_helper<boost::gregorian::date>();
        }

        SourceT or_else(SourceT else_value) const {
            return has_value_ ? source_ : else_value;
        }

        SourceT value() const { 
            if (has_value_) 
                return source_; 
            throw std::invalid_argument("no value!");
        }

        bool has_value() const { return has_value_; }

    private:
        SourceT source_;
        const bool has_value_;
    };

}

#endif