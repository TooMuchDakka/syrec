#include "core/syrec/parser/symbol_table.hpp"

namespace syrec {
    bool                              symbol_table::contains(const std::string_view& literal) {
        // TODO: Implement me
        return false;
    }

    bool                              symbol_table::contains(const module::ptr& module) {
        // TODO: Implement me
        return false;
    }

    [[nodiscard]] const variable::ptr symbol_table::get_variable(const std::string_view& literal) {
        // TODO: Implement me
        return {};
    }

    [[nodiscard]] const module::ptr   get_module(const std::string_view& moduleName) {
        // TODO: Implement me
        return {};
    }

    void                              symbol_table::add_entry(const variable::ptr& local_entry) {
        // TODO: Implement me
    }

    void                              symbol_table::add_entry(const module::ptr& module) {
        // TODO: Implement me
    }

    void symbol_table::open_scope() {
        // TODO: Implement me
    }

    void symbol_table::close_scope() {
        // TODO: Implement me
    }
}
