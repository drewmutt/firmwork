//
// ADC resistor ladder helper.
//

#ifndef FIRMWORK_RESISTORLADDERINPUT_H
#define FIRMWORK_RESISTORLADDERINPUT_H

#include <Arduino.h>

#include <initializer_list>
#include <vector>

// Set to 1 to enable verbose serial debug output in ResistorLadderInput.cpp.
#ifndef RESISTOR_LADDER_INPUT_DEBUG
#define RESISTOR_LADDER_INPUT_DEBUG 1
#endif

class ResistorLadderInput {
public:
    enum Topology : uint8_t {
        TOPOLOGY_SERIES = 0,
        TOPOLOGY_SWITCHED_TO_GROUND = 1,
    };

    enum SwitchMode : uint8_t {
        SWITCH_NORMALLY_OPEN = 0,
        SWITCH_NORMALLY_CLOSED = 1,
    };

    // Series ladder constructor (original behavior).
    ResistorLadderInput(
        uint8_t adcPin,
        const std::vector<float> &resistancesOhms,
        float toleranceFraction = 0.06f,
        uint8_t sampleCount = 5);

    ResistorLadderInput(
        uint8_t adcPin,
        std::initializer_list<float> resistancesOhms,
        float toleranceFraction = 0.06f,
        uint8_t sampleCount = 5);

    // Switched-to-ground constructor for NC/NO button ladders:
    // 3V3 -- Rtop -- ADC node -- (switch + node resistor) -- GND.
    ResistorLadderInput(
        uint8_t adcPin,
        float topResistorOhms,
        const std::vector<float> &nodeResistancesOhms,
        SwitchMode switchMode,
        float bleedResistorOhms = 0.0f,
        float toleranceFraction = 0.06f,
        uint8_t sampleCount = 5);

    ResistorLadderInput(
        uint8_t adcPin,
        float topResistorOhms,
        std::initializer_list<float> nodeResistancesOhms,
        SwitchMode switchMode,
        float bleedResistorOhms = 0.0f,
        float toleranceFraction = 0.06f,
        uint8_t sampleCount = 5);

    uint8_t getAdcPin() const;
    Topology getTopology() const;
    SwitchMode getSwitchMode() const;
    size_t nodeCount() const;

    void setTolerance(float toleranceFraction);
    float getTolerance() const;

    void setSampleCount(uint8_t sampleCount);
    uint8_t getSampleCount() const;
    void setInputVoltageMilliVolts(float inputVoltageMilliVolts);
    float getInputVoltageMilliVolts() const;

    float analogReadPulldownOhms() const;
    float analogReadMilliVolts() const;
    float analogReadPulldownOhms(float inputVoltage) const;
    static float calculatePullDown(float pullUpOhms, float outputVoltage, float inputVoltage);
    int activeNode() const;             // -1 means no active node in tolerance.
    std::vector<int> activeNodes() const;  // Supports simultaneous active nodes in switched ladders.
    bool digitalRead(int nodeNumber) const;

    float expectedNodeVoltageRatio(int nodeNumber) const;

    bool hasIdleState() const;
    float idleVoltageRatio() const;

private:
    void buildExpectedValues();
    void buildExpectedValuesSeries();
    void buildExpectedValuesSwitched();

    int nearestNodeForRatio(float measuredRatio) const;
    int nearestNodeForPulldown(float pulldownOhms) const;

    static float parallelResistance(const std::vector<float> &resistancesOhms);
    float dividerRatioFromPulldown(float pulldownOhms) const;

    uint8_t adcPin_;
    Topology topology_;
    SwitchMode switchMode_;
    float topResistorOhms_;
    float bleedResistorOhms_;

    std::vector<float> resistancesOhms_;
    std::vector<float> expectedVoltageRatios_;
    std::vector<float> expectedPulldownOhms_;

    bool hasIdleExpected_;
    float idleExpectedVoltageRatio_;
    bool hasIdlePulldownExpected_;
    float idleExpectedPulldownOhms_;

    float toleranceFraction_;
    uint8_t sampleCount_;
    float inputVoltageMilliVolts_;
};

#endif  // FIRMWORK_RESISTORLADDERINPUT_H
