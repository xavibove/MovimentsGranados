#include "ViolinApp.h"
#include "MathUtils.h"
#include "SharedConstants.h"
#include "../../Shared/OSCSettings.h"

static const string STR_APP_TITLE              = "VIOLIN [AUDIO ANALYZER]";
static const string ANALYSIS_VIOLIN_FILENAME   = "violinAnalyzer.xml";
static const string DEVICE_SETTINGS_FILENAME   = "violinDeviceSettings.xml";

void ViolinApp::setup()
{
    ofSetWindowTitle(STR_APP_TITLE);
    ofBackground(81, 88, 111);

    guiAnalyzerCreated = false;

    float silenceThreshold = 0;
    unsigned int silenceLength = 0;
    float onsetsThreshold = 0;
    int ascDescAnalysisSize = 20;
    PMAudioAnalyzer::getInstance().init(silenceThreshold, silenceLength, onsetsThreshold, ascDescAnalysisSize);

    buildDevicesPanel();
    buildCelloAnalysisPanel();

    oscSender.setup(OSC_VIOLIN_SENDER_HOST, OSC_VIOLIN_SENDER_PORT);
}

void ViolinApp::update()
{
}

void ViolinApp::draw()
{
    guiDevices.draw();
    if (guiAnalyzerCreated) guiAnalysis.draw();
}

void ViolinApp::exit()
{
    guiDevices.saveToFile(DEVICE_SETTINGS_FILENAME);
    guiAnalysis.saveToFile(ANALYSIS_VIOLIN_FILENAME);
}

void ViolinApp::keyReleased(int key)
{
}

void ViolinApp::buildDevicesPanel()
{
    vector<ofSoundDevice> devices = PMAudioAnalyzer::getInstance().getInputDevices();

    guiDevices.setup(STR_DEV_TITLE, DEVICE_SETTINGS_FILENAME);
    guiDevices.setPosition(GUI_MARGIN, GUI_MARGIN);

    for (int i=0; i<devices.size(); ++i)
    {
        if (devices[i].inputChannels < 2) continue;

        XBDeviceParams devParams(devices[i].deviceID, devices[i].name, devices[i].inputChannels);
        guiDevices.add(devParams.getParams());

        deviceParams.push_back(devParams);
    }

    guiDevices.loadFromFile(DEVICE_SETTINGS_FILENAME);

    guiDevices.add(lblStatus.setup(STR_DEV_STATUS, STR_DEV_STATUS_OFF));
    lblStatus.setBackgroundColor(ofColor::darkRed);
    guiDevices.add(btnStartAnalysis.setup(STR_DEV_START));
    guiDevices.add(btnStopAnalysis.setup(STR_DEV_STOP));

    btnStartAnalysis.addListener(this, &ViolinApp::startButtonPressed);
    btnStopAnalysis.addListener(this, &ViolinApp::stopButtonPressed);

    lblStatus.setDefaultWidth(GUI_DEV_WIDTH);

    guiDevices.setSize(GUI_DEV_WIDTH, GUI_DEV_WIDTH);
    guiDevices.setWidthElements(GUI_DEV_WIDTH);
}

void ViolinApp::buildCelloAnalysisPanel()
{
    guiAnalysis.setup("VIOLIN", ANALYSIS_VIOLIN_FILENAME);
    guiAnalysis.setPosition(ofGetWidth() - GUI_AN_WIDTH - GUI_MARGIN - 1, GUI_MARGIN);

    paramsPitch.setName(STR_PITCH);
    paramsPitch.add(pitchCurrentNote.set(STR_PITCH_CURRENT_NOTE, PITCH_MIN, PITCH_MIN, PITCH_MAX));
    paramsPitch.add(pitchSmoothedNote.set(STR_PITCH_SMOOTHED_NOTE, PITCH_SMOOTH_MIN, PITCH_SMOOTH_MIN, PITCH_SMOOTH_MAX));
    paramsPitch.add(pitchSmoothAmount.set(STR_PITCH_SMOOTH_AMOUNT, PITCH_SMOOTH_MIN, PITCH_SMOOTH_MIN, PITCH_SMOOTH_MAX));
    paramsPitch.add(pitchMidiMin.set(STR_PITCH_MINMIDINOTE, PITCH_MIN, PITCH_MIN, PITCH_MAX));
    paramsPitch.add(pitchMidiMax.set(STR_PITCH_MAXMIDINOTE, PITCH_MAX, PITCH_MIN, PITCH_MAX));
    guiAnalysis.add(paramsPitch);

    paramsEnergy.setName(STR_ENERGY);
    paramsEnergy.add(energyEnergy.set(STR_ENERGY_VALUE, ENERGY_MIN, ENERGY_MIN, ENERGY_MAX));
    paramsEnergy.add(energySmoothed.set(STR_ENERGY_SMOOTHED_VALUE, ENERGY_SMOOTH_MIN, ENERGY_SMOOTH_MIN, ENERGY_SMOOTH_MAX));
    paramsEnergy.add(energySmoothAmount.set(STR_ENERGY_SMOOTH_AMOUNT, ENERGY_SMOOTH_MIN, ENERGY_SMOOTH_MIN, ENERGY_SMOOTH_MAX));
    paramsEnergy.add(energyMin.set(STR_ENERGY_MIN, ENERGY_MIN, ENERGY_MIN, ENERGY_MAX));
    paramsEnergy.add(energyMax.set(STR_ENERGY_MAX, ENERGY_MAX, ENERGY_MIN, ENERGY_MAX));
    paramsEnergy.add(digitalGain.set(STR_ENERGY_GAIN, GAIN_MIN, GAIN_MIN, GAIN_MAX));
    guiAnalysis.add(paramsEnergy);

    paramsSilence.setName(STR_SILENCE);
    paramsSilence.add(silenceThreshold.set(STR_SILENCE_THRESHOLD, SILENCE_THRSHLD_MIN, SILENCE_THRSHLD_MIN, SILENCE_THRSHLD_MAX));
    paramsSilence.add(silenceLength.set(STR_SILENCE_LENGTH, SILENCE_LENGTH_MIN, SILENCE_LENGTH_MIN, SILENCE_LENGTH_MAX));
    paramsSilence.add(silenceOn.set(STR_SILENCE_STATUS, false));
    guiAnalysis.add(paramsSilence);

    paramsOnsets.setName(STR_ONSETS);
    paramsOnsets.add(onsetsThreshold.set(STR_ONSETS_THRESHOLD, ONSET_THRSHLD_MIN, ONSET_THRSHLD_MIN, ONSET_THRSHLD_MAX));
    paramsOnsets.add(onsetsOn.set(STR_ONSETS_STATUS, false));
    guiAnalysis.add(paramsOnsets);

    guiAnalysis.loadFromFile(ANALYSIS_VIOLIN_FILENAME);

    guiAnalysis.setSize(GUI_AN_WIDTH, GUI_AN_WIDTH);
    guiAnalysis.setWidthElements(GUI_AN_WIDTH);

    guiAnalyzerCreated = true;
}

void ViolinApp::startButtonPressed()
{
    unsigned int deviceID = 0;
    vector<unsigned int> enabledChannels;

    bool deviceIsValid = false;
    for (int i=0; i<deviceParams.size() && !deviceIsValid; ++i)
    {
        deviceID = deviceParams[i].getDeviceID();
        deviceIsValid = (deviceParams[i].getIsEnabled()) && (deviceParams[i].getEnabledChannels().size() == 1);
        if (deviceIsValid) enabledChannels = deviceParams[i].getEnabledChannels();
    }

    if (deviceIsValid)
    {
        // Create Audio Analyzer for the selected device

        vector<ofSoundDevice> devices = PMAudioAnalyzer::getInstance().getInputDevices();
        bool deviceFound = false;
        int deviceIndex = 0;

        for (int i=0; i<devices.size() && !deviceFound; ++i)
        {
            deviceFound = (devices[i].deviceID == deviceID);
            if (deviceFound) deviceIndex = i;
        }

        unsigned int audioInputIndex = 0;
        PMDeviceAudioAnalyzer *deviceAudioAnalyzer = PMAudioAnalyzer::getInstance().addDeviceAnalyzer(audioInputIndex,
                devices[deviceIndex].deviceID,
                enabledChannels[0],
                devices[deviceIndex].outputChannels,
                DEFAULT_SAMPLERATE, DEFAULT_BUFFERSIZE,
                enabledChannels,
                devices[deviceIndex].inputChannels);

        ofAddListener(deviceAudioAnalyzer->eventPitchChanged, this, &ViolinApp::analyzerPitchChanged);
        ofAddListener(deviceAudioAnalyzer->eventEnergyChanged, this, &ViolinApp::analyzerEnergyChanged);
        ofAddListener(deviceAudioAnalyzer->eventSilenceStateChanged, this, &ViolinApp::analyzerSilenceStateChanged);
        ofAddListener(deviceAudioAnalyzer->eventOnsetStateChanged, this, &ViolinApp::analyzerOnsetDetected);

        // Init audio analyzers with GUI settings

        audioAnalyzers = PMAudioAnalyzer::getInstance().getAudioAnalyzers();

        // Send "start" OSC message
        {
            ofxOscMessage m;
            stringstream address;
            address << OSC_VIOLIN_ADDR_BASE << OSC_ANALYZER_ADDR_STARTED;
            m.setAddress(address.str());
            oscSender.sendMessage(m, false);
        }

        // Register GUI events
        {
            pitchSmoothAmount.addListener(this, &ViolinApp::guiPitchSmoothAmountChanged);
            pitchMidiMin.addListener(this, &ViolinApp::guiPitchMinChanged);
            pitchMidiMax.addListener(this, &ViolinApp::getPitchMaxChanged);
            energySmoothAmount.addListener(this, &ViolinApp::guiEnergySmoothAmountChanged);
            energyMin.addListener(this, &ViolinApp::guiEnergyMinChanged);
            energyMax.addListener(this, &ViolinApp::guiEnergyMaxChanged);
            digitalGain.addListener(this, &ViolinApp::guiDigitalGainChanged);
            silenceThreshold.addListener(this, &ViolinApp::guiSilenceThresholdChanged);
            silenceLength.addListener(this, &ViolinApp::guiSilenceLengthChanged);
            onsetsThreshold.addListener(this, &ViolinApp::guiOnsetsThresholdChanged);
        }

        // Force initial audio analyzer values setup
        {
            float tmpPitchSmoothAmount = pitchSmoothAmount.get();
            guiPitchSmoothAmountChanged(tmpPitchSmoothAmount);
            float tmpPitchMidiMin = pitchMidiMin.get();
            guiPitchMinChanged(tmpPitchMidiMin);
            float tmpPitchMidiMax = pitchMidiMax.get();
            getPitchMaxChanged(tmpPitchMidiMax);
            float tmpEnergySmoothAmount = energySmoothAmount.get();
            guiEnergySmoothAmountChanged(tmpEnergySmoothAmount);
            float tmpEnergyMin = energyMin.get();
            guiEnergyMinChanged(tmpEnergyMin);
            float tmpEnergyMax = energyMax.get();
            guiEnergyMaxChanged(tmpEnergyMax);
            float tmpDigitalGain = digitalGain.get();
            guiDigitalGainChanged(tmpDigitalGain);
            float tmpSilenceThrshld = silenceThreshold.get();
            guiSilenceThresholdChanged(tmpSilenceThrshld);
            float tmpSilenceLength = silenceLength.get();
            guiSilenceLengthChanged(tmpSilenceLength);
            float tmpOnsetsThrshld = onsetsThreshold.get();
            guiOnsetsThresholdChanged(tmpOnsetsThrshld);
        }

        PMAudioAnalyzer::getInstance().start();

        lblStatus.setup(STR_DEV_STATUS, STR_DEV_STATUS_ON);
        lblStatus.setBackgroundColor(ofColor::darkGreen);
    }
}

void ViolinApp::stopButtonPressed()
{
    PMAudioAnalyzer::getInstance().stop();

    lblStatus.setup(STR_DEV_STATUS, STR_DEV_STATUS_OFF);
    lblStatus.setBackgroundColor(ofColor::darkRed);

    // Send "stop" OSC message
    {
        ofxOscMessage m;
        stringstream address;
        address << OSC_VIOLIN_ADDR_BASE << OSC_ANALYZER_ADDR_STOPPED;
        m.setAddress(address.str());
        oscSender.sendMessage(m, false);
    }}

void ViolinApp::guiPitchSmoothAmountChanged(float &smoothAmount) {
    float invertedSmoothAmount = ofMap(smoothAmount, PITCH_SMOOTH_MIN, PITCH_SMOOTH_MAX, PITCH_SMOOTH_MAX, PITCH_SMOOTH_MIN);
    (*audioAnalyzers)[0]->setDeltaPitch(invertedSmoothAmount);
}

void ViolinApp::guiPitchMinChanged(float &pitch) {
    (*audioAnalyzers)[0]->setMinPitch(pitch);
}

void ViolinApp::getPitchMaxChanged(float &pitch) {
    (*audioAnalyzers)[0]->setMaxPitch(pitch);
}

void ViolinApp::guiEnergySmoothAmountChanged(float &smoothAmount) {
    float invertedSmoothAmount = ofMap(smoothAmount, PITCH_SMOOTH_MIN, PITCH_SMOOTH_MAX, PITCH_SMOOTH_MAX, PITCH_SMOOTH_MIN);
    (*audioAnalyzers)[0]->setDeltaEnergy(invertedSmoothAmount);
}

void ViolinApp::guiEnergyMinChanged(float &energy) {
    (*audioAnalyzers)[0]->setMinEnergy(energy);
}

void ViolinApp::guiEnergyMaxChanged(float &energy) {
    (*audioAnalyzers)[0]->setMaxEnergy(energy);
}

void ViolinApp::guiDigitalGainChanged(float &digitalGain) {
    (*audioAnalyzers)[0]->setDigitalGain(digitalGain);
}

void ViolinApp::guiSilenceThresholdChanged(float &threshold) {
    (*audioAnalyzers)[0]->setSilenceThreshold(threshold);
}

void ViolinApp::guiSilenceLengthChanged(float &length) {
    (*audioAnalyzers)[0]->setSilenceQueueLength(length);
}

void ViolinApp::guiOnsetsThresholdChanged(float &threshold) {
    (*audioAnalyzers)[0]->setOnsetsThreshold(threshold);
}

void ViolinApp::analyzerPitchChanged(pitchParams &pitchParams)
{
    if (!guiAnalyzerCreated) return;

    pitchCurrentNote = truncateFloat(pitchParams.midiNote, 2);
    pitchSmoothedNote = truncateFloat(pitchParams.smoothedPitch, 2);

    cout << "pitchCurrentNote = " << pitchCurrentNote << endl;
    cout << "pitchSmoothedNote = " << pitchSmoothedNote << endl;

    ofxOscMessage m;
    stringstream address;
    address << OSC_VIOLIN_ADDR_BASE << OSC_ANALYZER_ADDR_PITCHNOTE;
    m.setAddress(address.str());
    m.addFloatArg(pitchParams.smoothedPitch);
    oscSender.sendMessage(m, false);
}

void ViolinApp::analyzerEnergyChanged(energyParams &energyParams)
{
    if (!guiAnalyzerCreated) return;

    energyEnergy = truncateFloat(energyParams.energy, 2);
    energySmoothed = truncateFloat(energyParams.smoothedEnergy, 2);

    cout << "energyEnergy = " << energyEnergy << endl;
    cout << "energySmoothed = " << energySmoothed << endl;

    ofxOscMessage m;
    stringstream address;
    address << OSC_VIOLIN_ADDR_BASE << OSC_ANALYZER_ADDR_ENERGY;
    m.setAddress(address.str());
    m.addFloatArg(energyParams.smoothedEnergy);
    oscSender.sendMessage(m, false);
}

void ViolinApp::analyzerSilenceStateChanged(silenceParams &silenceParams)
{
    if (!guiAnalyzerCreated) return;

    silenceOn = silenceParams.isSilent;

    cout << "silenceOn = " << silenceOn << endl;

    ofxOscMessage m;
    stringstream address;
    address << OSC_VIOLIN_ADDR_BASE << OSC_ANALYZER_ADDR_SILENCE;
    m.setAddress(address.str());
    m.addBoolArg(silenceOn);
    oscSender.sendMessage(m, false);
}

void ViolinApp::analyzerOnsetDetected(onsetParams &onsetParams)
{
    if (!guiAnalyzerCreated) return;

    onsetsOn = onsetParams.isOnset;

    cout << "onsetsOn = " << onsetsOn << endl;

    ofxOscMessage m;
    stringstream address;
    address << OSC_VIOLIN_ADDR_BASE << OSC_ANALYZER_ADDR_ONSET;
    m.setAddress(address.str());
    oscSender.sendMessage(m, false);
}