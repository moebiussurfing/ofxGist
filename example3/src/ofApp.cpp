#include "ofApp.h"


//--------------------------------------------------------------
void ofApp::setupSoundStream() {

	//----

	//audio seetings
	sampleRate = 48000;
	bufferSize = 512;
	numBuffers = 4;

	numInputs = 2;
	numOutputs = 2;

	//bEnable_Out = false;

	//----

	//supported apis
	enum Api {
		UNSPECIFIED,
		DEFAULT,
		ALSA,     /*!< The Advanced Linux Sound Architecture API. */
		PULSE,    /*!< The Linux PulseAudio API. */
		OSS,      /*!< The Linux Open Sound System API. */
		JACK,      /*!< The Jack Low-Latency Audio Server API. */
		OSX_CORE,    /*!< Macintosh OS-X Core Audio API. */
		MS_WASAPI, /*!< The Microsoft WASAPI API. */
		MS_ASIO,   /*!< The Steinberg Audio Stream I/O API. */
		MS_DS,     /*!< The Microsoft Direct Sound API. */
		NUM_APIS
	} api = UNSPECIFIED;

	//------------------

	//api choice

	//api = MS_ASIO;
	api = MS_DS;
	//api = MS_WASAPI;

	//------------------

	ofLogNotice(__FUNCTION__) << "List audio devices from any API";

	//list all apis
	auto devicesAsio = inStream.getDeviceList(ofSoundDevice::Api::MS_ASIO);
	for (auto dev : devicesAsio)
	{
		ofLogNotice(__FUNCTION__) << ofToString(dev);
	}

	auto devicesDs = inStream.getDeviceList(ofSoundDevice::Api::MS_DS);
	for (auto dev : devicesDs)
	{
		ofLogNotice(__FUNCTION__) << ofToString(dev);
	}

	auto devicesWs = inStream.getDeviceList(ofSoundDevice::Api::MS_WASAPI);
	for (auto dev : devicesWs)
	{
		ofLogNotice(__FUNCTION__) << ofToString(dev);
	}

	//-

	ofLogNotice(__FUNCTION__) << "Set audio API [" + ofToString(api) + "]";

	switch (api)
	{
	case MS_WASAPI:
		inDeviceIndex = 8;//audio cable
		outDeviceIndex = 1;//speakers
		inDevices = inStream.getDeviceList(ofSoundDevice::Api::MS_WASAPI);
		outDevices = outStream.getDeviceList(ofSoundDevice::Api::MS_WASAPI);
		break;

	case MS_ASIO:
		inDeviceIndex = 0;//audio cable
		outDeviceIndex = 0;//speakers
		inDevices = inStream.getDeviceList(ofSoundDevice::Api::MS_ASIO);
		outDevices = outStream.getDeviceList(ofSoundDevice::Api::MS_ASIO);
		break;

	case MS_DS:
		inDeviceIndex = 3;//audio cable
		outDeviceIndex = 1;//tv
		inDevices = inStream.getDeviceList(ofSoundDevice::Api::MS_DS);
		outDevices = outStream.getDeviceList(ofSoundDevice::Api::MS_DS);
		break;
	}

	//-

	//input

	ofSoundStreamSettings inSettings;

	inSettings.sampleRate = sampleRate;
	inSettings.bufferSize = bufferSize;
	inSettings.numBuffers = numBuffers;
	inSettings.numOutputChannels = 0;
	inSettings.numInputChannels = numInputs;
	//inSettings.numInputChannels = inDevices[inDeviceIndex].inputChannels;//all

	switch (api)
	{
	case MS_WASAPI:
		inSettings.setApi(ofSoundDevice::Api::MS_WASAPI);
		break;

	case MS_ASIO:
		inSettings.setApi(ofSoundDevice::Api::MS_ASIO);
		break;

	case MS_DS:
		inSettings.setApi(ofSoundDevice::Api::MS_DS);
		break;
	}

	inSettings.setInListener(ofGetAppPtr());
	inSettings.setInDevice(inDevices[inDeviceIndex]);
	inStream.setup(inSettings);
	//inStream.setInput(input);

	//-

	//output

	ofSoundStreamSettings outSettings;

	outSettings.bufferSize = bufferSize;
	outSettings.numBuffers = numBuffers;
	outSettings.sampleRate = sampleRate;// set the samplerate of both devices to be the same
	outSettings.numInputChannels = 0;
	outSettings.numOutputChannels = numOutputs;
	//outSettings.numOutputChannels = outDevices[outDeviceIndex].outputChannels;//all

	switch (api)
	{
	case MS_WASAPI:
		outSettings.setApi(ofSoundDevice::Api::MS_WASAPI);
		break;

	case MS_ASIO:
		outSettings.setApi(ofSoundDevice::Api::MS_ASIO);
		break;

	case MS_DS:
		outSettings.setApi(ofSoundDevice::Api::MS_DS);
		break;
	}

	//outSettings.setInListener(ofGetAppPtr());
	outSettings.setOutDevice(outDevices[outDeviceIndex]);
	outStream.setup(outSettings);
	//outStream.setOutput(output);

	//-

	ofLogNotice(__FUNCTION__) << "------------------------------------------" << endl;

	//--
}

//--------------------------------------------------------------
void ofApp::setup() {
	ofSetCircleResolution(80);
	ofSetFrameRate(60);
	ofBackground(255);
	ofEnableSmoothing();
	ofEnableAlphaBlending();
	ofSetVerticalSync(true);
	ofLogToConsole();

	useMic = 1;

	//--

	setupSoundStream();
	setupGist();
	//setupPlayer();

	//-

	phase = 0;
	phaseAdder = 0.0f;
	phaseAdderTarget = 0.0f;
	volume = 0.01f;

	lAudio.assign(bufferSize, 0.0);
	rAudio.assign(bufferSize, 0.0);

	//--

	playhead = std::numeric_limits<int>::max(); // because it is converted to int for check
	playheadControl = -1.0;
	//step = audiofile.samplerate() / sampleRate;

	//soundStream_Input.setup(this,0, 1, sampleRate, bufferSize, 1);
}

//--------------------------------------------------------------
void ofApp::setupGist() 
{
	mfccMax = 0;
	showMFCC = 0;

	vector<string> features = ofxGist::getFeatureNames();

	int num = features.size();

	//add all gist analyzer types
	for (int v = 0; v < num; v++) {
		GIST_FEATURE f = ofxGist::getFeatureFromName(features[v]);
		gist.setDetect(f);
		ofxHistoryPlot * graph = addGraph(features[v], 1.0, ofColor(ofRandom(100) + 150, ofRandom(100) + 150, ofRandom(100) + 150));
	
		plots.push_back(graph);
	}


	num = 13;//happens to be 13 coefficients by default
	for (int v = 0; v < num; v++) {
		ofxHistoryPlot * graph = addGraph("mfcc_" + ofToString(v), 1.0, ofColor(ofRandom(100) + 150, ofRandom(100) + 150, ofRandom(100) + 150));
		
		mfccPlots.push_back(graph);
	}

	//-

	////add 2
	//GIST_FEATURE f = ofxGist::getFeatureFromName(features[2]);
	//gist.setDetect(f);
	//ofxHistoryPlot * graph = addGraph(features[2], 1.0, ofColor(ofRandom(100) + 150, ofRandom(100) + 150, ofRandom(100) + 150));
	//plots.push_back(graph);

	//ofxHistoryPlot * graph = addGraph("mfcc_" + ofToString(2), 1.0, ofColor(ofRandom(100) + 150, ofRandom(100) + 150, ofRandom(100) + 150));
	//mfccPlots.push_back(graph);

	/*
	//add special crest graph
	ofxHistoryPlot*crest = addGraph("CREST_AVG",1.0,ofColor(ofRandom(100)+150,ofRandom(100)+150,ofRandom(100)+150));
	crest->setShowSmoothedCurve(1);
	 */

	//--

	//detector with callback

	 //gist.setUseForOnsetDetection(GIST_PEAK_ENERGY);

	 //gist.setUseForOnsetDetection(GIST_SPECTRAL_DIFFERENCE);
	 //gist.setThreshold(GIST_SPECTRAL_DIFFERENCE, .2);

	gist.setUseForOnsetDetection(GIST_PEAK_ENERGY);
	gist.setThreshold(GIST_PEAK_ENERGY, .05);//

	ofAddListener(GistEvent::ON, this, &ofApp::onNoteOn);
	ofAddListener(GistEvent::OFF, this, &ofApp::onNoteOff);

	noteOnRadius = 0;
}

//-

//--------------------------------------------------------------
void ofApp::setupPlayer() {
	isPaused = 0;
	//player.setLoop(true);

		//loadSong("assets/sounds/Coltrane_acc_VUIMM.wav");

	//-

	//std::string filepath = ofToDataPath("tubophone.wav"); // mono original file
	////std::string filepath = ofToDataPath("tubophone.mp3"); // at the moment mp3 isn't working on ARM
	////std::string filepath = ofToDataPath("tubophone.ogg"); // ogg/vorbis
	////std::string filepath = ofToDataPath("tubophone.flac"); // FLAC

	//audiofile.setVerbose(true);
	////ofSetLogLevel(OF_LOG_VERBOSE);

	//if (ofFile::doesFileExist(filepath)) {
	//	audiofile.load(filepath);
	//	if (!audiofile.loaded()) {
	//		ofLogError() << "error loading file, double check the file path";
	//	}
	//}
	//else {
	//	ofLogError() << "input file does not exists";
	//}

	//-

	//// audio setup for testing audio file stream 
	//ofSoundStreamSettings settings_Input;
	//sampleRate = 44100.0;
	//settings_Input.setOutListener(this);
	//settings_Input.sampleRate = sampleRate;
	//settings_Input.numOutputChannels = 2;
	//settings_Input.numInputChannels = 0;
	//settings_Input.bufferSize = 512;
	//ofSoundStreamSetup(settings_Input);
}

//-


//--------------------------------------------------------------
void ofApp::onNoteOn(GistEvent &e) {

	//ofLogNotice(__FUNCTION__) << "+";
	noteOnRadius = 100;
};


//--------------------------------------------------------------
void ofApp::onNoteOff(GistEvent &e) {

	//ofLogNotice(__FUNCTION__) << "-";
	//noteOnRadius = 0;
};


//--------------------------------------------------------------
void ofApp::update() {

	//   if(isPaused){
	//       return;
	//   }
	//   if(!useMic)
	   //{
	//       //if(player.isLoaded())
	   //	//if (audiofile.loaded())
	   //	{
	   //		//vector<float> output = player.getCurrentBuffer(bufferSize);
	   //		//vector<float> output = audiofile.data(bufferSize);

	//           //processAudio(&output[0], bufferSize, 2);

	//           //fftSmoothed = player.getFFT();
	   //		//fftSmoothed = audiofile.getFFT();
	//       }
	//   }

	//-

	int num = ofxGist::getFeatureNames().size();

	for (int v = 0; v < num; v++) {
		plots[v]->setRange(gist.getMin(v), gist.getMax(v));
		plots[v]->update(gist.getValue(v));
	}

	//plots[0]->setRange(gist.getMin(2), gist.getMax(2));
	//plots[0]->update(gist.getValue(2));

	//-

	vector<float>mfcc = gist.getMelFrequencyCepstralCoefficients();
	//vector<float>mfcc = gist.getMelFrequencySpectrum();

	if (mfccSmoothed.size() < mfcc.size()) 
	{
		mfccSmoothed.assign(mfcc.size(), 0.0);
	}
	float damping = .7;
	int f = 0;

	for (int i = 0; i < mfcc.size(); i++) 
	{
		// take the max, either the smoothed or the incoming:
		if (mfccSmoothed[f] < mfcc[i] || damping >.999f) 
		{
			mfccSmoothed[f] = mfcc[i];
		}

		// let the smoothed value sink to zero:
		mfccSmoothed[i] *= damping;
		f++;

		if (mfccMax < mfcc[i]) 
		{
			mfccMax = mfcc[i];
		}
		
		mfccPlots[i]->setRange(gist.getMFCCMin(i), gist.getMFCCMax(i));

		mfccPlots[i]->update(mfcc[i]);
	}

	/*
	//crest avg
	plots.back()->setRange(gist.getMin(GIST_SPECTRAL_CREST),gist.getMax(GIST_SPECTRAL_CREST));
	plots.back()->update(gist.getAvg(GIST_SPECTRAL_CREST));
	*/

	//fade out trigged detector
	if (noteOnRadius > 0) 
	{
		noteOnRadius--;
	}
}

//--------------------------------------------------------------
void ofApp::draw() {
	ofBackground(0);

	//draw2();

	// draw the left:
	//ofSetHexColor(0x333333);
	int waveHeight = ofGetHeight();

	//ofRect(0,0,256,waveHeight);
	float barW = ofGetWidth() / (float)fftSmoothed.size();
	float currX = 0;

	ofSetColor(255, 255, 255, 100);
	for (int i = 0; i < fftSmoothed.size(); i++) 
	{
		//ofLine(i,100,i,100+left[i]*waveHeight);

		ofRect(currX, waveHeight, barW, -fftSmoothed[i] * waveHeight);
		currX += barW;
	}

	currX = 0;
	waveHeight = ofGetHeight();
	barW = ofGetWidth() / (float)mfccSmoothed.size();
	ofSetColor(255, 100, 100, 200);
	for (int i = 0; i < mfccSmoothed.size() && showMFCC; i++) {
		//ofLine(i,100,i,100+left[i]*waveHeight);

		ofRect(currX, ofGetHeight(), barW, -ofMap(mfccSmoothed[i], 0.0, mfccMax, 0.0, 1.0, true)*waveHeight);
		currX += barW;
	}


	int num;
	int margin = 5;
	if (showMFCC)
	{
		num = mfccPlots.size();
		int plotHeight = (ofGetHeight() - margin * num) / (float)num;

		for (int v = 0; v < num; v++) 
		{
			mfccPlots[v]->draw(margin, margin + plotHeight * v, ofGetWidth() - 20, 100);
		}
	}
	else
	{		
		num = plots.size();

		int plotHeight = (ofGetHeight() - margin * num) / (float)num;

		for (int v = 0; v < num; v++)
		{
			plots[v]->draw(margin, margin + plotHeight * v, ofGetWidth() - 20, 100);
		}

	}
	

	ofSetColor(255, 0, 0, 200);
	ofCircle(ofGetWidth() / 2, ofGetHeight() / 2, noteOnRadius);


	if (!showMFCC) 
	{
		ofSetColor(0, 0, 0, 250);
		ofRect(5, 5, 600, 180);
		stringstream str;
		if (plots.size()) 
		{
			for (int v = 0; v < num; v++)
			{
				str << plots[v]->getVariableName() << " | min: " << gist.getMin(v) << " | avg: " << gist.getAvg(v) << " | max: " << gist.getMax(v) << "\n";

			}

			str << "Note freq: " << gist.getNoteFrequency() << " " << gist.getNoteName() << "\n";
		}
		else 
		{
			str << "Drag and drop wave files from data folder.";
		}

		ofSetColor(255);

		ofDrawBitmapString(str.str(), 10, 10);
	}
}

////--------------------------------------------------------------
//void ofApp::draw2() {
//
//	ofNoFill();
//
//	ofPushMatrix();
//	for (int c = 0; c < audiofile.channels(); ++c) {
//
//		float max = ofGetWidth();
//		ofBeginShape();
//		for (int x = 0; x < max; ++x) {
//			int n = ofMap(x, 0, max, 0, audiofile.length(), true);
//			float val = audiofile.sample(n, c);
//			float y = ofMap(val, -1.0f, 1.0f, ofGetHeight()*0.5, 0.0f);
//			ofVertex(x, y);
//		}
//		ofEndShape();
//
//		float phx = ofMap(playhead, 0, audiofile.length(), 0, max);
//		ofDrawLine(phx, 0, phx, ofGetHeight()*0.5f);
//
//		ofTranslate(0.0, ofGetHeight()*0.5);
//	}
//	ofPopMatrix();
//
//	ofDrawBitmapString(audiofile.path(), 10, 20);
//
//	ofDrawBitmapString("press SPACEBAR to play, press L to load a sample", 10, ofGetHeight() - 20);
//}

ofxHistoryPlot* ofApp::addGraph(string varName, float max, ofColor color) {

	ofxHistoryPlot* graph = new ofxHistoryPlot(NULL, varName, max, false);	//true for autoupdate
	//graph2->setLowerRange(0); //set only the lowest part of the range upper is adaptative to curve
	graph->setAutoRangeShrinksBack(true); //graph2 scale can shrink back after growing if graph2 curves requires it
	graph->setRange(0, max);
	graph->setColor(color);
	graph->setShowNumericalInfo(true);
	graph->setRespectBorders(true);
	graph->setLineWidth(1);


	graph->setDrawBackground(false);

	graph->setDrawGrid(true);
	graph->setGridColor(ofColor(30)); //grid lines color
	graph->setGridUnit(14);
	graph->setShowSmoothedCurve(0); //graph2 a smoothed version of the values, but alos the original in lesser alpha
	graph->setSmoothFilter(0.1); //smooth filter strength

	graph->setMaxHistory(2000);


	return graph;

};


void ofApp::clear() {


}


void ofApp::loadSong(string str) {

	cout << "loadSong " << str << endl;

	//player.stop();
	//player.loadSound(str);
	//player.setLoop(true);
	//player.play();

	//-

	useMic = 0;
	mfccMax = 0;
	gist.clearHistory();
}

void ofApp::processAudio(float * input, int bufferSize, int nChannels)
{
	//convert float array to vector
	vector<float>buffer;
	buffer.assign(&input[0], &input[bufferSize]);

	gist.processAudio(buffer, bufferSize, nChannels, sampleRate);
}


void ofApp::audioIn(float * input, int bufferSize, int nChannels)
{
	if (!useMic) {
		return;
	}

	processAudio(input, bufferSize, nChannels);

}


//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	if (key == 'f') {
		ofToggleFullscreen();
	}

	//if (key == 'm') {
	//	useMic = !useMic;

	//	if (!useMic) {
	//		//player.play();
	//	}
	//	else {
	//		//player.stop();
	//	}
	
	//	gist.clearHistory();
	//}

	//if (key == ' ') {
	//	isPaused = !isPaused;
	//	//player.setPaused(isPaused);
	//	playheadControl = 0.0;
	//}

	if (key == 'r') {
		gist.clearHistory();
	}


	if (key == 'c') {
		showMFCC = !showMFCC;
	}

	//-

	//if (key == ' ') playheadControl = 0.0;

	//if (key == 'l' || key == 'L') {
	//	////Open the Open File Dialog
	//	//ofFileDialogResult openFileResult = ofSystemLoadDialog("select an audio sample");
	//	////Check if the user opened a file
	//	//if (openFileResult.bSuccess) {
	//	//	string filepath = openFileResult.getPath();
	//	//	audiofile.load(filepath);
	//	//	step = audiofile.samplerate() / sampleRate;
	//	//	ofLogVerbose("file loaded");
	//	//}
	//	//else {
	//	//	ofLogVerbose("User hit cancel");
	//	//}
	//}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) 
{
	//phase generator for audio testing
	int width = ofGetWidth();
	pan = (float)x / (float)width;
	float height = (float)ofGetHeight();
	float heightPct = ((height - y) / height);
	targetFrequency = 2000.0f * heightPct;
	phaseAdderTarget = (targetFrequency / (float)sampleRate) * TWO_PI;
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
	if (!useMic) {
		//player.setPosition(x/(float)ofGetWidth());
	}

	float t = x / (float)ofGetWidth();
	gist.setThreshold(GIST_SPECTRAL_DIFFERENCE, t);
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {
	//   clear();
	//   vector<string> paths = ofSplitString(dragInfo.files[0], "data/");
	//   //loadSong(paths[1]);
	
	   //audiofile.load(dragInfo.files[0]);
}


////--------------------------------------------------------------
//void ofApp::audioOut(ofSoundBuffer & buffer) {
//
//	//// really spartan and not efficient sample playing, just for testing
//
//	//if (playheadControl >= 0.0) {
//	//	playhead = playheadControl;
//	//	playheadControl = -1.0;
//	//}
//
//	//for (size_t i = 0; i < buffer.getNumFrames(); i++) {
//
//	//	int n = playhead;
//
//	//	if (n < audiofile.length() - 1) {
//
//	//		for (size_t k = 0; k < buffer.getNumChannels(); ++k) {
//	//			if (k < audiofile.channels()) {
//	//				float fract = playhead - (double)n;
//	//				float s0 = audiofile.sample(n, k);
//	//				float s1 = audiofile.sample(n + 1, k);
//	//				float isample = s0 * (1.0 - fract) + s1 * fract; // linear interpolation
//	//				buffer[i*buffer.getNumChannels() + k] = isample;
//	//			}
//	//			else {
//	//				buffer[i*buffer.getNumChannels() + k] = 0.0f;
//	//			}
//	//		}
//
//	//		playhead += step;
//
//	//	}
//	//	else {
//	//		buffer[i*buffer.getNumChannels()] = 0.0f;
//	//		buffer[i*buffer.getNumChannels() + 1] = 0.0f;
//	//		playhead = std::numeric_limits<int>::max();
//	//	}
//
//	//}
//}


////--------------------------------------------------------------
//void ofApp::dragEvent(ofDragInfo dragInfo) {
//	audiofile.load(dragInfo.files[0]);
//}

//--------------------------------------------------------------
void ofApp::exit() {
	//ofSoundStreamClose();

	//soundStream_Input.close();
	//soundStream_Output.close();
}


//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer & buffer) {
	////oscillator
	//float leftScale = .5f;
	//float rightScale = .5f;
	//
	//float sample = sin(phase);

	//phaseAdder = 0.95f * phaseAdder + 0.05f * phaseAdderTarget;
	//for (size_t i = 0; i < buffer.getNumFrames(); i++) {
	//	phase += phaseAdder;
	//	float sample = sin(phase);
	//	lAudio[i] = buffer[i*buffer.getNumChannels()] = sample * volume * leftScale;
	//	rAudio[i] = buffer[i*buffer.getNumChannels() + 1] = sample * volume * rightScale;
	//}

	//for (size_t i = 0; i < buffer.getNumFrames(); i++) {
	//	lAudio[i] = buffer[i*buffer.getNumChannels()] = 0;
	//	rAudio[i] = buffer[i*buffer.getNumChannels() + 1] = 0;
	//}
}