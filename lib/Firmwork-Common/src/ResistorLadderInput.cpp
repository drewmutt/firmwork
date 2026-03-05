#include <ResistorLadderInput.h>

#include <math.h>

#if RESISTOR_LADDER_INPUT_DEBUG
#define RLI_DEBUGF(...)           \
    do {                          \
        Serial.printf(__VA_ARGS__); \
    } while (0)
#else
#define RLI_DEBUGF(...) \
    do {                \
    } while (0)
#endif

namespace {
constexpr float kSwitchedPulldownRelativeToleranceFloor = 0.20f;
constexpr float kSwitchedOpenCircuitRatioToleranceFloor = 0.12f;
constexpr size_t kMaxActiveNodeCombinationCount = 4096U;
constexpr size_t kActiveNodesVerboseCandidateLimit = 128U;

void debugPrintNodeMask(unsigned long long mask, size_t nodeCount) {
    RLI_DEBUGF("[");
    bool first = true;
    for (size_t i = 0; i < nodeCount; ++i) {
        if ((mask & (1ULL << i)) == 0ULL) {
            continue;
        }

        RLI_DEBUGF("%s%u", first ? "" : ",", static_cast<unsigned>(i));
        first = false;
    }
    if (first) {
        RLI_DEBUGF("none");
    }
    RLI_DEBUGF("]");
}
}  // namespace

ResistorLadderInput::ResistorLadderInput(
    uint8_t adcPin,
    const std::vector<float> &resistancesOhms,
    float toleranceFraction,
    uint8_t sampleCount)
    : adcPin_(adcPin),
      topology_(TOPOLOGY_SERIES),
      switchMode_(SWITCH_NORMALLY_OPEN),
      topResistorOhms_(0.0f),
      bleedResistorOhms_(0.0f),
      resistancesOhms_(resistancesOhms),
      expectedPulldownOhms_(),
      hasIdleExpected_(false),
      idleExpectedVoltageRatio_(0.0f),
      hasIdlePulldownExpected_(false),
      idleExpectedPulldownOhms_(0.0f),
      toleranceFraction_(toleranceFraction),
      sampleCount_(sampleCount == 0U ? 1U : sampleCount),
      inputVoltageMilliVolts_(3300.0f) {
    setTolerance(toleranceFraction_);
    buildExpectedValues();
    RLI_DEBUGF(
        "[RLI] ctor(series) pin=%u nodes=%u tol=%.4f samples=%u\n",
        static_cast<unsigned>(adcPin_),
        static_cast<unsigned>(resistancesOhms_.size()),
        toleranceFraction_,
        static_cast<unsigned>(sampleCount_));
}

ResistorLadderInput::ResistorLadderInput(
    uint8_t adcPin,
    std::initializer_list<float> resistancesOhms,
    float toleranceFraction,
    uint8_t sampleCount)
    : ResistorLadderInput(adcPin, std::vector<float>(resistancesOhms), toleranceFraction, sampleCount) {}

ResistorLadderInput::ResistorLadderInput(
    uint8_t adcPin,
    float topResistorOhms,
    const std::vector<float> &nodeResistancesOhms,
    SwitchMode switchMode,
    float bleedResistorOhms,
    float toleranceFraction,
    uint8_t sampleCount)
    : adcPin_(adcPin),
      topology_(TOPOLOGY_SWITCHED_TO_GROUND),
      switchMode_(switchMode),
      topResistorOhms_(topResistorOhms),
      bleedResistorOhms_(bleedResistorOhms),
      resistancesOhms_(nodeResistancesOhms),
      expectedPulldownOhms_(),
      hasIdleExpected_(false),
      idleExpectedVoltageRatio_(0.0f),
      hasIdlePulldownExpected_(false),
      idleExpectedPulldownOhms_(0.0f),
      toleranceFraction_(toleranceFraction),
      sampleCount_(sampleCount == 0U ? 1U : sampleCount),
      inputVoltageMilliVolts_(3300.0f) {
    setTolerance(toleranceFraction_);
    buildExpectedValues();
    RLI_DEBUGF(
        "[RLI] ctor(switched) pin=%u nodes=%u mode=%s rTop=%.2f rBleed=%.2f tol=%.4f samples=%u\n",
        static_cast<unsigned>(adcPin_),
        static_cast<unsigned>(resistancesOhms_.size()),
        (switchMode_ == SWITCH_NORMALLY_CLOSED) ? "NC" : "NO",
        topResistorOhms_,
        bleedResistorOhms_,
        toleranceFraction_,
        static_cast<unsigned>(sampleCount_));
}

ResistorLadderInput::ResistorLadderInput(
    uint8_t adcPin,
    float topResistorOhms,
    std::initializer_list<float> nodeResistancesOhms,
    SwitchMode switchMode,
    float bleedResistorOhms,
    float toleranceFraction,
    uint8_t sampleCount)
    : ResistorLadderInput(
          adcPin,
          topResistorOhms,
          std::vector<float>(nodeResistancesOhms),
          switchMode,
          bleedResistorOhms,
          toleranceFraction,
          sampleCount) {}

uint8_t ResistorLadderInput::getAdcPin() const {
    return adcPin_;
}

ResistorLadderInput::Topology ResistorLadderInput::getTopology() const {
    return topology_;
}

ResistorLadderInput::SwitchMode ResistorLadderInput::getSwitchMode() const {
    return switchMode_;
}

size_t ResistorLadderInput::nodeCount() const {
    return expectedVoltageRatios_.size();
}

void ResistorLadderInput::setTolerance(float toleranceFraction) {
    if (toleranceFraction < 0.0f) {
        toleranceFraction_ = 0.0f;
        RLI_DEBUGF("[RLI] setTolerance clamped to %.4f\n", toleranceFraction_);
        return;
    }

    if (toleranceFraction > 1.0f) {
        toleranceFraction_ = 1.0f;
        RLI_DEBUGF("[RLI] setTolerance clamped to %.4f\n", toleranceFraction_);
        return;
    }

    toleranceFraction_ = toleranceFraction;
    RLI_DEBUGF("[RLI] setTolerance %.4f\n", toleranceFraction_);
}

float ResistorLadderInput::getTolerance() const {
    return toleranceFraction_;
}

void ResistorLadderInput::setSampleCount(uint8_t sampleCount) {
    sampleCount_ = (sampleCount == 0U) ? 1U : sampleCount;
    RLI_DEBUGF("[RLI] setSampleCount %u\n", static_cast<unsigned>(sampleCount_));
}

uint8_t ResistorLadderInput::getSampleCount() const {
    return sampleCount_;
}

void ResistorLadderInput::setInputVoltageMilliVolts(float inputVoltageMilliVolts) {
    if (inputVoltageMilliVolts <= 0.0f) {
        inputVoltageMilliVolts_ = 3300.0f;
        RLI_DEBUGF("[RLI] setInputVoltageMilliVolts invalid -> %.2f\n", inputVoltageMilliVolts_);
        return;
    }

    inputVoltageMilliVolts_ = inputVoltageMilliVolts;
    RLI_DEBUGF("[RLI] setInputVoltageMilliVolts %.2f\n", inputVoltageMilliVolts_);
}

float ResistorLadderInput::getInputVoltageMilliVolts() const {
    return inputVoltageMilliVolts_;
}

float ResistorLadderInput::analogReadPulldownOhms() const {
    return analogReadPulldownOhms(inputVoltageMilliVolts_);
}

float ResistorLadderInput::analogReadMilliVolts() const {
    uint32_t total = 0U;
    for (uint8_t i = 0; i < sampleCount_; ++i) {
        total += static_cast<uint32_t>(::analogReadMilliVolts(adcPin_));
    }
    const float average = static_cast<float>(total) / static_cast<float>(sampleCount_);
    RLI_DEBUGF(
        "[RLI] analogReadMilliVolts pin=%u total=%lu samples=%u avg=%.2f\n",
        static_cast<unsigned>(adcPin_),
        static_cast<unsigned long>(total),
        static_cast<unsigned>(sampleCount_),
        average);
    return average;
}

float ResistorLadderInput::calculatePullDown(float pullUpOhms, float outputVoltage, float inputVoltage) {
    if (pullUpOhms <= 0.0f || outputVoltage <= 0.0f || inputVoltage <= 0.0f || outputVoltage >= inputVoltage) {
        return -1.0f;
    }

    return pullUpOhms * (outputVoltage / (inputVoltage - outputVoltage));
}

float ResistorLadderInput::analogReadPulldownOhms(float inputVoltage) const {
    if (topResistorOhms_ <= 0.0f) {
        return -1.0f;
    }

    const float outputVoltage = analogReadMilliVolts();
    const float pulldownOhms = calculatePullDown(topResistorOhms_, outputVoltage, inputVoltage);
    RLI_DEBUGF(
        "[RLI] analogReadPulldownOhms vOut=%.2f vIn=%.2f rTop=%.2f rDown=%.2f\n",
        outputVoltage,
        inputVoltage,
        topResistorOhms_,
        pulldownOhms);
    return pulldownOhms;
}

int ResistorLadderInput::activeNode() const {
    if (nodeCount() == 0U) {
        RLI_DEBUGF("[RLI] activeNode no expected nodes configured\n");
        return -1;
    }

    if (topology_ == TOPOLOGY_SWITCHED_TO_GROUND && topResistorOhms_ > 0.0f && !expectedPulldownOhms_.empty()) {
        const float pulldownOhms = analogReadPulldownOhms();
        const int node = nearestNodeForPulldown(pulldownOhms);
        RLI_DEBUGF(
            "[RLI] activeNode(switched) vIn=%.2f pulldown=%.2f node=%d\n",
            inputVoltageMilliVolts_,
            pulldownOhms,
            node);
        return node;
    }

    const float measuredMilliVolts = analogReadMilliVolts();
    if (inputVoltageMilliVolts_ <= 0.0f) {
        RLI_DEBUGF("[RLI] activeNode invalid configured input voltage=%.2f\n", inputVoltageMilliVolts_);
        return -1;
    }

    float normalized = measuredMilliVolts / inputVoltageMilliVolts_;
    if (normalized < 0.0f) {
        normalized = 0.0f;
    } else if (normalized > 1.0f) {
        normalized = 1.0f;
    }

    const int node = nearestNodeForRatio(normalized);
    RLI_DEBUGF(
        "[RLI] activeNode(series) vOut=%.2f vIn=%.2f ratio=%.6f node=%d\n",
        measuredMilliVolts,
        inputVoltageMilliVolts_,
        normalized,
        node);
    return node;
}

std::vector<int> ResistorLadderInput::activeNodes() const {
    std::vector<int> active;

    if (nodeCount() == 0U) {
        RLI_DEBUGF("[RLI] activeNodes no expected nodes configured\n");
        return active;
    }

    RLI_DEBUGF(
        "[RLI] activeNodes begin topology=%s mode=%s nodes=%u tol=%.4f samples=%u\n",
        (topology_ == TOPOLOGY_SWITCHED_TO_GROUND) ? "SWITCHED" : "SERIES",
        (switchMode_ == SWITCH_NORMALLY_CLOSED) ? "NC" : "NO",
        static_cast<unsigned>(nodeCount()),
        toleranceFraction_,
        static_cast<unsigned>(sampleCount_));

    if (topology_ != TOPOLOGY_SWITCHED_TO_GROUND || topResistorOhms_ <= 0.0f) {
        RLI_DEBUGF(
            "[RLI] activeNodes fallback to single-node detection topology=%s rTop=%.2f\n",
            (topology_ == TOPOLOGY_SWITCHED_TO_GROUND) ? "SWITCHED" : "SERIES",
            topResistorOhms_);
        const int node = activeNode();
        if (node >= 0) {
            active.push_back(node);
        }
        RLI_DEBUGF("[RLI] activeNodes fallback result count=%u\n", static_cast<unsigned>(active.size()));
        return active;
    }

    const size_t configuredNodeCount = resistancesOhms_.size();
    if (configuredNodeCount == 0U) {
        RLI_DEBUGF("[RLI] activeNodes no configured switched nodes\n");
        return active;
    }

    if (configuredNodeCount >= (sizeof(unsigned long long) * 8U)) {
        RLI_DEBUGF("[RLI] activeNodes node-count too large for mask (%u)\n", static_cast<unsigned>(configuredNodeCount));
        const int node = activeNode();
        if (node >= 0) {
            active.push_back(node);
        }
        return active;
    }

    const unsigned long long combinationCount = (1ULL << configuredNodeCount);
    if (combinationCount > static_cast<unsigned long long>(kMaxActiveNodeCombinationCount)) {
        RLI_DEBUGF(
            "[RLI] activeNodes too many combinations (%llu), falling back to single-node detection\n",
            combinationCount);
        const int node = activeNode();
        if (node >= 0) {
            active.push_back(node);
        }
        return active;
    }

    const bool logEveryCandidate = (combinationCount <= static_cast<unsigned long long>(kActiveNodesVerboseCandidateLimit));
    if (logEveryCandidate) {
        RLI_DEBUGF(
            "[RLI] activeNodes candidate tracing enabled combinations=%llu\n",
            combinationCount);
    }

    const float measuredMilliVolts = analogReadMilliVolts();
    const float measuredPulldownOhms =
        calculatePullDown(topResistorOhms_, measuredMilliVolts, inputVoltageMilliVolts_);
    float measuredRatio = 1.0f;
    if (inputVoltageMilliVolts_ > 0.0f) {
        measuredRatio = measuredMilliVolts / inputVoltageMilliVolts_;
    }
    if (measuredRatio < 0.0f) {
        measuredRatio = 0.0f;
    } else if (measuredRatio > 1.0f) {
        measuredRatio = 1.0f;
    }
    RLI_DEBUGF(
        "[RLI] activeNodes vOut=%.2f vIn=%.2f ratio=%.6f measuredPulldown=%.2f combinations=%llu\n",
        measuredMilliVolts,
        inputVoltageMilliVolts_,
        measuredRatio,
        measuredPulldownOhms,
        combinationCount);

    bool bestMaskSet = false;
    unsigned long long bestMask = 0ULL;
    float bestRelativeDiff = 1000000.0f;
    float bestRatioDiff = 1000000.0f;
    float bestExpectedPulldownOhms = -1.0f;
    float bestExpectedRatio = 0.0f;

    for (unsigned long long mask = 0ULL; mask < combinationCount; ++mask) {
        float sumInverse = 0.0f;
        if (bleedResistorOhms_ > 0.0f) {
            sumInverse += (1.0f / bleedResistorOhms_);
        }

        for (size_t i = 0; i < configuredNodeCount; ++i) {
            const float nodeResistor = resistancesOhms_[i];
            if (nodeResistor <= 0.0f) {
                continue;
            }

            const bool nodeActive = ((mask & (1ULL << i)) != 0ULL);
            const bool conductorActive =
                (switchMode_ == SWITCH_NORMALLY_CLOSED) ? (!nodeActive) : nodeActive;
            if (conductorActive) {
                sumInverse += (1.0f / nodeResistor);
            }
        }

        float expectedPulldownOhms = -1.0f;
        if (sumInverse > 0.0f) {
            expectedPulldownOhms = (1.0f / sumInverse);
        }
        const float expectedRatio = dividerRatioFromPulldown(expectedPulldownOhms);
        const float ratioDiff = fabsf(measuredRatio - expectedRatio);

        float relativeDiff = 1000000.0f;
        if (expectedPulldownOhms > 0.0f && measuredPulldownOhms > 0.0f) {
            relativeDiff = fabsf(measuredPulldownOhms - expectedPulldownOhms) / expectedPulldownOhms;
        } else if (expectedPulldownOhms <= 0.0f) {
            // Open-circuit masks have no finite pull-down; score in ratio domain near Vout ~= Vin.
            relativeDiff = ratioDiff;
        } else if (measuredPulldownOhms <= 0.0f) {
            relativeDiff = ratioDiff;
        }

        if (logEveryCandidate) {
            RLI_DEBUGF("[RLI] activeNodes candidate mask=%llu active=", mask);
            debugPrintNodeMask(mask, configuredNodeCount);
            RLI_DEBUGF(
                " expectedPulldown=%.2f expectedRatio=%.6f relDiff=%.4f ratioDiff=%.4f\n",
                expectedPulldownOhms,
                expectedRatio,
                relativeDiff,
                ratioDiff);
        }

        const bool isBetter = (!bestMaskSet) || (relativeDiff < bestRelativeDiff) ||
                              ((fabsf(relativeDiff - bestRelativeDiff) <= 0.000001f) && (ratioDiff < bestRatioDiff));
        if (isBetter) {
            bestMaskSet = true;
            bestMask = mask;
            bestRelativeDiff = relativeDiff;
            bestRatioDiff = ratioDiff;
            bestExpectedPulldownOhms = expectedPulldownOhms;
            bestExpectedRatio = expectedRatio;
            if (!logEveryCandidate) {
                RLI_DEBUGF("[RLI] activeNodes new-best mask=%llu active=", bestMask);
                debugPrintNodeMask(bestMask, configuredNodeCount);
                RLI_DEBUGF(
                    " expectedPulldown=%.2f expectedRatio=%.6f measuredPulldown=%.2f relDiff=%.4f ratioDiff=%.4f\n",
                    bestExpectedPulldownOhms,
                    bestExpectedRatio,
                    measuredPulldownOhms,
                    bestRelativeDiff,
                    bestRatioDiff);
            }
        }
    }

    if (!bestMaskSet) {
        RLI_DEBUGF("[RLI] activeNodes no best combination\n");
        return active;
    }

    float maxRelativeTolerance = toleranceFraction_;
    if (maxRelativeTolerance < kSwitchedPulldownRelativeToleranceFloor) {
        maxRelativeTolerance = kSwitchedPulldownRelativeToleranceFloor;
    }
    float maxOpenCircuitRatioTolerance = toleranceFraction_;
    if (maxOpenCircuitRatioTolerance < kSwitchedOpenCircuitRatioToleranceFloor) {
        maxOpenCircuitRatioTolerance = kSwitchedOpenCircuitRatioToleranceFloor;
    }
    const bool bestIsOpenCircuit = (bestExpectedPulldownOhms <= 0.0f);
    const float decisionMetric = bestIsOpenCircuit ? bestRatioDiff : bestRelativeDiff;
    const float decisionTolerance = bestIsOpenCircuit ? maxOpenCircuitRatioTolerance : maxRelativeTolerance;

    if (decisionMetric > decisionTolerance) {
        RLI_DEBUGF(
            "[RLI] activeNodes reject bestMask=%llu active=",
            bestMask);
        debugPrintNodeMask(bestMask, configuredNodeCount);
        RLI_DEBUGF(
            " expectedPulldown=%.2f expectedRatio=%.6f measuredPulldown=%.2f metric=%.4f tol=%.4f (%s)\n",
            bestExpectedPulldownOhms,
            bestExpectedRatio,
            measuredPulldownOhms,
            decisionMetric,
            decisionTolerance,
            bestIsOpenCircuit ? "open-ratio" : "pulldown-rel");
        return active;
    }

    for (size_t i = 0; i < configuredNodeCount; ++i) {
        if ((bestMask & (1ULL << i)) != 0ULL) {
            active.push_back(static_cast<int>(i));
        }
    }

    RLI_DEBUGF(
        "[RLI] activeNodes selected bestMask=%llu active=",
        bestMask);
    debugPrintNodeMask(bestMask, configuredNodeCount);
    RLI_DEBUGF(
        " count=%u expectedPulldown=%.2f expectedRatio=%.6f measuredPulldown=%.2f relDiff=%.4f ratioDiff=%.4f\n",
        static_cast<unsigned>(active.size()),
        bestExpectedPulldownOhms,
        bestExpectedRatio,
        measuredPulldownOhms,
        bestRelativeDiff,
        bestRatioDiff);
    return active;
}

bool ResistorLadderInput::digitalRead(int nodeNumber) const {
    if (nodeNumber < 0 || static_cast<size_t>(nodeNumber) >= nodeCount()) {
        RLI_DEBUGF("[RLI] digitalRead node=%d out-of-range\n", nodeNumber);
        return false;
    }

    const bool active = (activeNode() == nodeNumber);
    RLI_DEBUGF("[RLI] digitalRead node=%d active=%u\n", nodeNumber, active ? 1U : 0U);
    return active;
}

float ResistorLadderInput::expectedNodeVoltageRatio(int nodeNumber) const {
    if (nodeNumber < 0 || static_cast<size_t>(nodeNumber) >= expectedVoltageRatios_.size()) {
        return 0.0f;
    }

    return expectedVoltageRatios_[nodeNumber];
}

bool ResistorLadderInput::hasIdleState() const {
    return hasIdleExpected_;
}

float ResistorLadderInput::idleVoltageRatio() const {
    return idleExpectedVoltageRatio_;
}

void ResistorLadderInput::buildExpectedValues() {
    expectedVoltageRatios_.clear();
    expectedPulldownOhms_.clear();
    hasIdleExpected_ = false;
    idleExpectedVoltageRatio_ = 0.0f;
    hasIdlePulldownExpected_ = false;
    idleExpectedPulldownOhms_ = 0.0f;

    if (resistancesOhms_.empty()) {
        RLI_DEBUGF("[RLI] buildExpectedValues skipped (no resistors)\n");
        return;
    }

    RLI_DEBUGF(
        "[RLI] buildExpectedValues topology=%s nodes=%u\n",
        (topology_ == TOPOLOGY_SWITCHED_TO_GROUND) ? "SWITCHED" : "SERIES",
        static_cast<unsigned>(resistancesOhms_.size()));

    if (topology_ == TOPOLOGY_SWITCHED_TO_GROUND) {
        buildExpectedValuesSwitched();
        return;
    }

    buildExpectedValuesSeries();
}

void ResistorLadderInput::buildExpectedValuesSeries() {
    float totalResistance = 0.0f;
    for (float resistor : resistancesOhms_) {
        if (resistor > 0.0f) {
            totalResistance += resistor;
        }
    }

    if (totalResistance <= 0.0f) {
        RLI_DEBUGF("[RLI] buildExpectedValuesSeries skipped (invalid total resistance)\n");
        return;
    }

    float cumulativeResistance = 0.0f;
    for (float resistor : resistancesOhms_) {
        if (resistor > 0.0f) {
            cumulativeResistance += resistor;
        }

        const float ratio = cumulativeResistance / totalResistance;
        expectedVoltageRatios_.push_back(ratio);

        RLI_DEBUGF(
            "[RLI] series node=%u Rcum=%.2f ratio=%.6f\n",
            static_cast<unsigned>(expectedVoltageRatios_.size() - 1U),
            cumulativeResistance,
            ratio);
    }
}

void ResistorLadderInput::buildExpectedValuesSwitched() {
    for (size_t i = 0; i < resistancesOhms_.size(); ++i) {
        std::vector<float> conductors;
        if (bleedResistorOhms_ > 0.0f) {
            conductors.push_back(bleedResistorOhms_);
        }

        for (size_t j = 0; j < resistancesOhms_.size(); ++j) {
            const float nodeResistor = resistancesOhms_[j];
            if (nodeResistor <= 0.0f) {
                continue;
            }

            bool conductorActive = false;
            if (switchMode_ == SWITCH_NORMALLY_CLOSED) {
                // Active node means pressed switch opens, so all other branches conduct.
                conductorActive = (j != i);
            } else {
                // Active node means pressed switch closes, so only this branch conducts.
                conductorActive = (j == i);
            }

            if (conductorActive) {
                conductors.push_back(nodeResistor);
            }
        }

        const float pulldownOhms = parallelResistance(conductors);
        expectedPulldownOhms_.push_back(pulldownOhms);
        const float ratio = dividerRatioFromPulldown(pulldownOhms);
        expectedVoltageRatios_.push_back(ratio);

        RLI_DEBUGF(
            "[RLI] switched node=%u pulldown=%.2f ratio=%.6f\n",
            static_cast<unsigned>(i),
            pulldownOhms,
            ratio);
    }

    // Build idle-state target to reduce false positives when no switch is active.
    std::vector<float> idleConductors;
    if (bleedResistorOhms_ > 0.0f) {
        idleConductors.push_back(bleedResistorOhms_);
    }
    if (switchMode_ == SWITCH_NORMALLY_CLOSED) {
        for (float nodeResistor : resistancesOhms_) {
            if (nodeResistor > 0.0f) {
                idleConductors.push_back(nodeResistor);
            }
        }
    }

    const float idlePulldown = parallelResistance(idleConductors);
    idleExpectedPulldownOhms_ = idlePulldown;
    hasIdlePulldownExpected_ = (idlePulldown > 0.0f);
    idleExpectedVoltageRatio_ = dividerRatioFromPulldown(idlePulldown);
    hasIdleExpected_ = true;

    RLI_DEBUGF(
        "[RLI] switched idle ratio=%.6f hasIdle=%u\n",
        idleExpectedVoltageRatio_,
        hasIdleExpected_ ? 1U : 0U);
}

int ResistorLadderInput::nearestNodeForPulldown(float pulldownOhms) const {
    if (pulldownOhms <= 0.0f || expectedPulldownOhms_.empty()) {
        RLI_DEBUGF("[RLI] nearestPulldown invalid measurement=%.2f\n", pulldownOhms);
        return -1;
    }

    int bestNode = -1;
    float bestRelativeDiff = 1000000.0f;

    if (hasIdlePulldownExpected_) {
        bestRelativeDiff = fabsf(pulldownOhms - idleExpectedPulldownOhms_) / idleExpectedPulldownOhms_;
        RLI_DEBUGF(
            "[RLI] nearestPulldown idle expected=%.2f measured=%.2f relDiff=%.4f\n",
            idleExpectedPulldownOhms_,
            pulldownOhms,
            bestRelativeDiff);
    }

    for (size_t i = 0; i < expectedPulldownOhms_.size(); ++i) {
        const float expectedPulldown = expectedPulldownOhms_[i];
        if (expectedPulldown <= 0.0f) {
            continue;
        }

        const float relativeDiff = fabsf(pulldownOhms - expectedPulldown) / expectedPulldown;
        RLI_DEBUGF(
            "[RLI] nearestPulldown candidate node=%u expected=%.2f measured=%.2f relDiff=%.4f\n",
            static_cast<unsigned>(i),
            expectedPulldown,
            pulldownOhms,
            relativeDiff);
        if (relativeDiff < bestRelativeDiff) {
            bestRelativeDiff = relativeDiff;
            bestNode = static_cast<int>(i);
        }
    }

    float maxRelativeTolerance = toleranceFraction_;
    if (maxRelativeTolerance < kSwitchedPulldownRelativeToleranceFloor) {
        maxRelativeTolerance = kSwitchedPulldownRelativeToleranceFloor;
    }

    if (bestRelativeDiff > maxRelativeTolerance) {
        RLI_DEBUGF(
            "[RLI] nearestPulldown reject node=%d relDiff=%.4f tol=%.4f\n",
            bestNode,
            bestRelativeDiff,
            maxRelativeTolerance);
        return -1;
    }

    RLI_DEBUGF(
        "[RLI] nearestPulldown selected node=%d relDiff=%.4f\n",
        bestNode,
        bestRelativeDiff);
    return bestNode;
}

int ResistorLadderInput::nearestNodeForRatio(float measuredRatio) const {
    if (expectedVoltageRatios_.empty()) {
        return -1;
    }

    if (measuredRatio < 0.0f) {
        measuredRatio = 0.0f;
    } else if (measuredRatio > 1.0f) {
        measuredRatio = 1.0f;
    }

    int bestNode = -1;
    float bestDiff = 1000000.0f;

    for (size_t i = 0; i < expectedVoltageRatios_.size(); ++i) {
        const float diff = fabsf(measuredRatio - expectedVoltageRatios_[i]);
        RLI_DEBUGF(
            "[RLI] nearestRatio candidate node=%u expected=%.6f measured=%.6f diff=%.6f\n",
            static_cast<unsigned>(i),
            expectedVoltageRatios_[i],
            measuredRatio,
            diff);
        if (diff < bestDiff) {
            bestDiff = diff;
            bestNode = static_cast<int>(i);
        }
    }

    if (bestNode < 0) {
        RLI_DEBUGF("[RLI] nearestRatio no best node\n");
        return -1;
    }

    float bestTolerance = toleranceFraction_;
    if (bestTolerance < 0.015f) {
        bestTolerance = 0.015f;
    }

    if (bestDiff > bestTolerance) {
        RLI_DEBUGF(
            "[RLI] nearestRatio reject best node=%d diff=%.6f tol=%.6f\n",
            bestNode,
            bestDiff,
            bestTolerance);
        return -1;
    }

    if (hasIdleExpected_) {
        const float idleDiff = fabsf(measuredRatio - idleExpectedVoltageRatio_);
        float idleTolerance = toleranceFraction_;
        if (idleTolerance < 0.015f) {
            idleTolerance = 0.015f;
        }
        if (idleDiff <= idleTolerance && idleDiff <= bestDiff) {
            RLI_DEBUGF(
                "[RLI] nearestRatio idle match idleDiff=%.6f idleTol=%.6f bestDiff=%.6f => no active node\n",
                idleDiff,
                idleTolerance,
                bestDiff);
            return -1;
        }
    }

    RLI_DEBUGF(
        "[RLI] nearestRatio selected node=%d expected=%.6f diff=%.6f\n",
        bestNode,
        expectedVoltageRatios_[bestNode],
        bestDiff);
    return bestNode;
}

float ResistorLadderInput::parallelResistance(const std::vector<float> &resistancesOhms) {
    float sumInverse = 0.0f;
    for (float resistance : resistancesOhms) {
        if (resistance > 0.0f) {
            sumInverse += 1.0f / resistance;
        }
    }

    if (sumInverse <= 0.0f) {
        return -1.0f;
    }

    return 1.0f / sumInverse;
}

float ResistorLadderInput::dividerRatioFromPulldown(float pulldownOhms) const {
    if (pulldownOhms <= 0.0f || topResistorOhms_ <= 0.0f) {
        return 1.0f;
    }

    const float ratio = pulldownOhms / (topResistorOhms_ + pulldownOhms);
    if (ratio < 0.0f) {
        return 0.0f;
    }
    if (ratio > 1.0f) {
        return 1.0f;
    }
    return ratio;
}
