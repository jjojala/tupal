#ifndef MANAGER_HH
#define MANAGER_HH

#include <tuple>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <boost/json.hpp>

namespace tupal {

    enum class error_code
    {
        ok = 0,
        unknown_key = 1,
        duplicate_key = 2,
        constraint_violation = 3,
        invalid_arguments = 4,
        system_error = 5
    };

}

namespace std {
    template <>
    struct is_error_condition_enum<tupal::error_code> : std::true_type {};
}

namespace tupal {

    std::error_category & error_category();
    std::error_code make_error_code(tupal::error_code e);
    std::error_condition make_error_condition(tupal::error_code e);

    class StartGroupManager;
    class CompetitionClassManager;
    class CompetitorManager;
    class CompetitionManager;

    typedef std::tuple<std::error_code, boost::json::value> result_type;
    inline const std::error_code & ec(const result_type & result) { return std::get<0>(result); };
    inline const boost::json::value & json(const result_type & result ) { return std::get<1>(result); };

    class CompetitionManager
    {
    public:
        static std::shared_ptr<CompetitionManager> new_competition_manager(const std::string & connection_spec);

        virtual result_type list(/* boost::json::value & result */) const = 0;
        virtual result_type get(const std::string & id) const = 0;
        virtual result_type create(const boost::json::value & new_data) = 0;
        virtual result_type update(const boost::json::value & new_data) = 0;
        virtual result_type remove(const std::string & id) = 0;

        virtual std::shared_ptr<StartGroupManager> getStartGroupManager() = 0;
        virtual std::shared_ptr<CompetitionClassManager> getCompetitionClassManager() = 0;
        virtual std::shared_ptr<CompetitorManager> getCompetitorManager() = 0;
    };

    class StartGroupManager
    {
    public:
        virtual result_type list(const std::string & competition_id) const = 0;
        virtual result_type get(const std::string & competition_id, const std::string & id) const = 0;
        virtual result_type create(const std::string & competition_id, const boost::json::value & new_data) = 0;
        virtual result_type update(const std::string & competition_id, const boost::json::value & new_data) = 0;
        virtual result_type remove(const std::string & competition_id, const std::string & id) = 0;
    };

    class CompetitionClassManager
    {
    public:
        virtual result_type list(const std::string & competition_id) const = 0;
        virtual result_type get(const std::string & competition_id, const std::string & id) const = 0;
        virtual result_type create(const std::string & competition_id, const boost::json::value & new_data) = 0;
        virtual result_type update(const std::string & competition_id, const boost::json::value & new_data) = 0;
        virtual result_type remove(const std::string & competition_id, const std::string & id) = 0;
    };

    class CompetitorManager
    {
    public:
        virtual result_type list(const std::string & competition_id) const = 0;
        virtual result_type get(const std::string & competition_id, const std::string & id) const = 0;
        virtual result_type create(const std::string & competition_id, const boost::json::value & new_data) = 0;
        virtual result_type update(const std::string & competition_id, const boost::json::value & new_data) = 0;
        virtual result_type remove(const std::string & competition_id, const std::string & id) = 0;
    };
};

#endif