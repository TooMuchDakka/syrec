#ifndef SYMBOL_TABLE_BACKUP_HELPER_HPP
#define SYMBOL_TABLE_BACKUP_HELPER_HPP

#include "core/syrec/parser/optimizations/constantPropagation/valueLookup/signal_value_lookup.hpp"

#include <map>
#include <string>
#include <unordered_set>

namespace parser {
	class SymbolTableBackupHelper {
    public:
        typedef std::shared_ptr<SymbolTableBackupHelper> ptr;

        SymbolTableBackupHelper() = default;

		void                                                                                         storeBackupOf(const std::string& signalIdent, const valueLookup::SignalValueLookup::ptr& currentValueOfSignal);
        [[nodiscard]] const std::map<std::string, valueLookup::SignalValueLookup::ptr, std::less<>>& getBackedUpEntries() const;
        [[nodiscard]] std::optional<valueLookup::SignalValueLookup::ptr>                             getBackedUpEntryFor(const std::string_view& signalIdent) const;
        [[nodiscard]] std::unordered_set<std::string>                                                getIdentsOfChangedSignals() const;
        [[nodiscard]] bool                                                                           hasEntryFor(const std::string_view& signalIdent) const;

	private:
        std::map<std::string, valueLookup::SignalValueLookup::ptr, std::less<>> backedUpSymbolTableEntries;
	};
};
#endif 