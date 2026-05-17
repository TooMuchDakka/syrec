/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#pragma once

#include "dd/Export.hpp"
#include "dd/Node.hpp"

#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <utility>

namespace syrec {
    class BaseQmddDumper {
    public:
        virtual ~BaseQmddDumper() = default;

        explicit BaseQmddDumper(const bool recordOnlyASingleQmddDump):
            recordOnlyASingleQmddDump(recordOnlyASingleQmddDump) {}

        virtual void dumpQmdd(const dd::mEdge* edgeToRootOfQmdd) = 0;

    protected:
        static void dumpQmdd(const dd::mEdge* edgeToRootOfQmdd, std::ostream& qmddDumpStream) {
            if (edgeToRootOfQmdd != nullptr) {
                dd::serialize(*edgeToRootOfQmdd, qmddDumpStream);
            }
        }

        bool recordOnlyASingleQmddDump;
        bool continueRecordingQmddDumps = true;
    };

    class QmddToFileDumper final: public BaseQmddDumper {
    public:
        explicit QmddToFileDumper(std::string pathToDumpFile, const bool recordOnlyASingleQmddDump):
            BaseQmddDumper(recordOnlyASingleQmddDump), pathToDumpFile(std::move(pathToDumpFile)), dumpFileFlags(std::ofstream::out | std::ofstream::trunc) {}

        void dumpQmdd(const dd::mEdge* edgeToRootOfQmdd) override {
            if (!continueRecordingQmddDumps || edgeToRootOfQmdd == nullptr) {
                return;
            }

            // TODO: How should I/O errors be handled when exceptions should not be thrown?
            std::ofstream ofs;
            ofs.open(pathToDumpFile, dumpFileFlags);
            if (!ofs.good()) {
                return;
            }

            BaseQmddDumper::dumpQmdd(edgeToRootOfQmdd, ofs);
            continueRecordingQmddDumps = !recordOnlyASingleQmddDump;
            dumpFileFlags              = getDumpFileStreamFlagsToAppendFurtherQmddDumps();
        }

    protected:
        std::string pathToDumpFile;
        int         dumpFileFlags;

        static int getDumpFileStreamFlagsToAppendFurtherQmddDumps() { return std::ofstream::out | std::ofstream::app; }
    };

    class InMemoryQmddDumper final: public BaseQmddDumper {
    public:
        explicit InMemoryQmddDumper(std::ostringstream& inmemoryDumpStream, const bool recordOnlyASingleQmddDump):
            BaseQmddDumper(recordOnlyASingleQmddDump), inmemoryDumpStream(inmemoryDumpStream) {}

        void dumpQmdd(const dd::mEdge* edgeToRootOfQmdd) override {
            if (!continueRecordingQmddDumps) {
                return;
            }

            BaseQmddDumper::dumpQmdd(edgeToRootOfQmdd, inmemoryDumpStream);
            continueRecordingQmddDumps = !recordOnlyASingleQmddDump;
        }

    protected:
        std::reference_wrapper<std::ostringstream> inmemoryDumpStream;
    };
} // namespace syrec
