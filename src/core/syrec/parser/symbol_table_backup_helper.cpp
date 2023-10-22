#include "core/syrec/parser/symbol_table_backup_helper.hpp"

#include <unordered_set>

using namespace parser;

void SymbolTableBackupHelper::storeBackupOf(const std::string& signalIdent, const valueLookup::SignalValueLookup::ptr& currentValueOfSignal) {
    if (hasEntryFor(signalIdent)) {
        backedUpSymbolTableEntries[signalIdent] = currentValueOfSignal;
    } else {
        backedUpSymbolTableEntries.insert(std::pair(signalIdent, currentValueOfSignal));
    }
}

const std::map<std::string, valueLookup::SignalValueLookup::ptr, std::less<>>& SymbolTableBackupHelper::getBackedUpEntries() const {
    return backedUpSymbolTableEntries;
}

std::optional<valueLookup::SignalValueLookup::ptr> SymbolTableBackupHelper::getBackedUpEntryFor(const std::string_view& signalIdent) const {
    if (!hasEntryFor(signalIdent)) {
        return std::nullopt;
    }
    return std::make_optional(backedUpSymbolTableEntries.find(signalIdent)->second);
}

std::unordered_set<std::string> SymbolTableBackupHelper::getIdentsOfChangedSignals() const {
    std::unordered_set<std::string> idents;
    for (const auto& [key, backupOfValue] : backedUpSymbolTableEntries) {
        idents.emplace(key);
    }
    return idents;
}

bool SymbolTableBackupHelper::hasEntryFor(const std::string_view& signalIdent) const {
    return backedUpSymbolTableEntries.count(signalIdent);
}