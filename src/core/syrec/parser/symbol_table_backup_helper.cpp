#include "core/syrec/parser/symbol_table_backup_helper.hpp"

using namespace parser;

void SymbolTableBackupHelper::storeBackupOf(const std::string& signalIdent, const valueLookup::SignalValueLookup::ptr& currentValueOfSignal) {
    if (backedUpSymbolTableEntries.count(signalIdent) != 0) {
        backedUpSymbolTableEntries[signalIdent] = currentValueOfSignal;
    } else {
        backedUpSymbolTableEntries.insert(std::pair(signalIdent, currentValueOfSignal));
    }
}

const std::map<std::string, valueLookup::SignalValueLookup::ptr>& SymbolTableBackupHelper::getBackedUpEntries() const {
    return backedUpSymbolTableEntries;
}

std::optional<valueLookup::SignalValueLookup::ptr> SymbolTableBackupHelper::getBackedupEntryFor(const std::string& signalIdent) const {
    if (backedUpSymbolTableEntries.count(signalIdent) == 0) {
        return std::nullopt;
    }
    return std::make_optional(backedUpSymbolTableEntries.at(signalIdent));
}

std::set<std::string> SymbolTableBackupHelper::getIdentsOfChangedSignals() const {
    std::set<std::string> idents;
    for (const auto& [key, backupOfValue] : backedUpSymbolTableEntries) {
        idents.emplace(key);
    }
    return idents;
}


