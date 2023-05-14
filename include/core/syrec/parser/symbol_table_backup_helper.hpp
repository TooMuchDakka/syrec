#ifndef SYMBOL_TABLE_BACKUP_HELPER_HPP
#define SYMBOL_TABLE_BACKUP_HELPER_HPP

#include "core/syrec/parser/optimizations/constantPropagation/valueLookup/signal_value_lookup.hpp"

#include <map>
#include <set>
#include <string>

namespace parser {
	class SymbolTableBackupHelper {
    public:
        typedef std::shared_ptr<SymbolTableBackupHelper> ptr;

        SymbolTableBackupHelper() = default;

		void                                                                            storeBackupOf(const std::string& signalIdent, const valueLookup::SignalValueLookup::ptr& currentValueOfSignal);
        [[nodiscard]] const std::map<std::string, valueLookup::SignalValueLookup::ptr>& getBackedUpEntries() const;
        [[nodiscard]] std::optional<valueLookup::SignalValueLookup::ptr>                getBackedupEntryFor(const std::string& signalIdent) const;
        [[nodiscard]] std::set<std::string>                                             getIdentsOfChangedSignals() const;

	private:
        std::map<std::string, valueLookup::SignalValueLookup::ptr> backedUpSymbolTableEntries;
	};
};
#endif 