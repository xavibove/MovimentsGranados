//
// Created by Miquel Àngel Soler on 24/12/15.
//

#include "XBScene3.h"
#include "XBScene3GUI.h"
#include "XBSettingsManager.h"

void XBScene3::setup(XBBaseGUI *_gui)
{
    XBBaseScene::setup(_gui);

    int w = 200;
    int h = 250;

    // creatures init
    initPaths();
    vPathIndex = 0;//vPath.size()/2;
    xPathIndex = 0;//xPath.size()/2;
    v.setup(vPath.getPointAtIndexInterpolated(vPathIndex).x, vPath.getPointAtIndexInterpolated(vPathIndex).y);
    x.setup(xPath.getPointAtIndexInterpolated(xPathIndex).x, xPath.getPointAtIndexInterpolated(xPathIndex).y);

    initPhysics();
    initWaves();
    initParticles();
    initStones();

    blur.setup(getMainFBO().getWidth(), getMainFBO().getHeight(), 0);
}

void XBScene3::update()
{
    XBBaseScene::update();
    XBScene3GUI *myGUI = (XBScene3GUI *) gui;

    updateVioinCello();
    updatePiano();
    updateDirector();
    
    if(collisionOn != myGUI->collisionOn){
        b2Filter f;
        if(myGUI->collisionOn){
            f.categoryBits = 0x0001;
            f.maskBits =0xFFFF;
        }
        else{
            f.categoryBits = 0x0004;
            f.maskBits =0x0004;
        }
        for (int i = 0; i < edges.size(); i++)
            edges[i].get()->setFilterData(f);
        collisionOn = myGUI->collisionOn;
    }
}

void XBScene3::drawIntoFBO()
{
    XBScene3GUI *myGUI = (XBScene3GUI *) gui;
    float windowScale = XBSettingsManager::getInstance().getWindowScale();

    fbo.begin();
    {
        ofPushMatrix();
        ofScale(windowScale, windowScale);
        if (showFacadeImage)
            templateImage.draw(0, 0);
        else
            ofBackground(0);
        if (showTemplate) {
            ofSetColor(255);
            svg.draw();
        }

        drawDirector();
        drawPiano();
        drawViolinCello();

        // mask for removing the windows
        ofPushStyle();
        ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);
        mask.draw(0, 0);
        ofPopStyle();

        drawParticles();
        
        // mask top and bottom of the facade for the particles
        ofPushStyle();
        ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);
        particlesMask.draw(0, 0);
        ofPopStyle();
        
        ofPopMatrix();

        drawMusiciansWindows();

        //draw GUI
        drawGUI();

//        drawFadeRectangle();
    }
    fbo.end();
    blur.apply(&fbo, 1, myGUI->blurAmount);
    applyPostFX();
}

void XBScene3::updatePiano()
{
    XBScene3GUI *myGUI = (XBScene3GUI *) gui;

    // update piano's stones
    for (int i = 0; i < stonesToDraw.size(); i++) {
        stonesToDraw[i].life += 1;//myGUI->stoneGrowFactor;
        stonesToDraw[i].amplitude *= myGUI->stoneDamping;
        if (stonesToDraw[i].life > ofGetFrameRate() * myGUI->stoneTime) {
            stonesToDraw.erase(stonesToDraw.begin() + i);
            i--; // keep i index valid
        }
    }
    pianoEnergy *= gui->pianoDecay;
    if(ofGetElapsedTimeMillis() - lastPianoNoteTime > 300)
        pianoEnergy *= 0.8;
}

void XBScene3::updateDirector()
{
    XBScene3GUI *myGUI = (XBScene3GUI *) gui;
    // update waves
    if (myGUI->simulateHands) {
        rightHand.velocity.x = rightHand.pos.x;
        rightHand.velocity.y = rightHand.pos.x;
        rightHand.pos.x = ofGetMouseX() / (float) ofGetWidth();
        rightHand.pos.y = ofGetMouseY() / (float) ofGetHeight();
        rightHand.velocity.x -= rightHand.pos.x;
        rightHand.velocity.y -= rightHand.pos.x;
        
        leftHand.velocity.x = leftHand.pos.x;
        leftHand.velocity.y = leftHand.pos.x;
        leftHand.pos.x = (rightHand.pos.x - 0.5) + 0.5 * (ofNoise(ofGetElapsedTimeMillis() * 0.0005) - 0.5);
        leftHand.pos.y = rightHand.pos.y + 0.5 * (ofNoise(ofGetElapsedTimeMillis() * 0.0005 + 1000) - 0.5);
        leftHand.velocity.x -= leftHand.pos.x;
        leftHand.velocity.y -= leftHand.pos.x;
    }
    for (int i = 0; i < waves.size(); i++) {
        //            float rightVelocity = rightHand.velocity.length();
        float rightVelocity = (rightHand.velocity.x + rightHand.velocity.y + rightHand.velocity.z) / 4.f;
        float strength = ofMap(rightVelocity, 0, 1, myGUI->minAttractorStrength, myGUI->attractorStrength, true);
        waves[i].setAttractor(0, rightHand.pos.x * MAIN_WINDOW_WIDTH, rightHand.pos.y * MAIN_WINDOW_HEIGHT,strength , myGUI->attractorRadius, myGUI->dampingWaves);

        //            float leftVelocity = leftHand.velocity.length();
        float leftVelocity = (leftHand.velocity.x + leftHand.velocity.y + leftHand.velocity.z) / 4.f;
        strength = ofMap(leftVelocity, 0, 1, myGUI->minAttractorStrength, myGUI->attractorStrength, true);
        waves[i].setAttractor(1, leftHand.pos.x * MAIN_WINDOW_WIDTH, leftHand.pos.y * MAIN_WINDOW_HEIGHT, strength, myGUI->attractorRadius, myGUI->dampingWaves);
        waves[i].update();
    }
}

void XBScene3::drawViolinCello()
{
    XBScene3GUI *myGUI = (XBScene3GUI *) gui;

    // draw violin and cello avatars
    if (myGUI->showPath) {
        ofSetColor(ofColor(myGUI->rgbColorViolinR, myGUI->rgbColorViolinG, myGUI->rgbColorViolinB, myGUI->colorViolinA));
        vPath.draw();
        ofPoint p = vPath.getPointAtIndexInterpolated(vPathIndex);
        ofSetColor(255);
        ofDrawCircle(p.x, p.y, 10);
        ofSetColor(ofColor(myGUI->rgbColorCelloR, myGUI->rgbColorCelloG, myGUI->rgbColorCelloB, myGUI->colorCelloA));
        xPath.draw();
        p = xPath.getPointAtIndexInterpolated(xPathIndex);
        ofSetColor(255);
        ofDrawCircle(p.x, p.y, 10);
    }
    pTex.setAnchorPercent(0.5, 0.5);
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    v.draw(pTex);
    x.draw(pTex);
    ofPopStyle();
}

void XBScene3::drawParticles()
{
    //draw particles from violin and cello
    ofPushStyle();
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    for (int i = 0; i < circles.size(); i++) {
        ofFill();
        circles[i].get()->draw(pTex);
    }
    ofPopStyle();
}

void XBScene3::drawDirector()
{
    XBScene3GUI *myGUI = (XBScene3GUI *) gui;
    // draw directors waves
    ofPushStyle();
    ofSetColor(myGUI->rgbColorDirectorR, myGUI->rgbColorDirectorG, myGUI->rgbColorDirectorB, myGUI->colorDirectorA);
    ofSetLineWidth(myGUI->lineWidth);
    for (Wave w:waves)
        w.display();
    ofPopStyle();
}

void XBScene3::drawPiano()
{
    XBScene3GUI *myGUI = (XBScene3GUI *) gui;
    ofPushStyle();
    // draw expanding stones from piano
    for (int i = 0; i < stonesToDraw.size(); i++) {
        expandingPolyLine &e = stonesToDraw[i];
        ofPushMatrix();
        ofTranslate(e.centroid);
        //             ofScale(e.life * myGUI->stoneGrowFactor, e.life * myGUI->stoneGrowFactor);
        float scale = 0.8 + e.amplitude * sin(myGUI->stoneFrequency * e.life + myGUI->stonePhase * PI / 2.f);
        ofScale(scale, scale);
        e.path.setFillColor(ofColor(myGUI->rgbColorPianoR, myGUI->rgbColorPianoG, myGUI->rgbColorPianoB, ofClamp(myGUI->colorPianoA - e.life * myGUI->stoneAlphaDecrease, 0, 255)));
        e.path.draw();
        ofPopMatrix();
    }
    ofPopStyle();

}

void XBScene3::drawGUI()
{
    XBBaseScene::drawGUI();
}

void XBScene3::keyReleased(int key)
{
    XBBaseScene::keyReleased(key);

    XBScene3GUI *myGUI = (XBScene3GUI *) gui;

    switch (key) {
        case 'v':
        case 'V': {
            int index = midiToRowMapping[(int) ofRandom(127)];
            expandingPolyLine e = stones[index][(int) ofRandom(stones[index].size() - 1)];
            e.life = 1;
            e.amplitude = myGUI->stoneGrowFactor;
            stonesToDraw.push_back(e);
            break;
        }
        case 'z':
        case 'Z': {
            waves.clear();
            initWaves();
            break;
        }
        default:
            break;
    }
}

//--------------------------------------------------------------

void XBScene3::onPianoNoteOn(XBOSCManager::PianoNoteOnArgs &noteOn)
{
    if (!active)
        return;

    XBBaseScene::onPianoNoteOn(noteOn);
    XBScene3GUI *myGUI = (XBScene3GUI *) gui;

    int wichLine = midiToRowMapping[noteOn.pitch];
    expandingPolyLine e = stones[wichLine][(int) ofRandom(stones[wichLine].size() - 1)];
    e.life = 1;
    e.amplitude = myGUI->stoneGrowFactor;
    stonesToDraw.push_back(e);
    pianoEnergy = noteOn.velocity / MAX_MIDI_VALUE;
}




void XBScene3::initPaths()
{
    svg.load("resources/caminosEscena3.svg");
    for (int i = 0; i < svg.getNumPath(); i++) {
        ofPath p = svg.getPathAt(i);
//        cout << "Path " << i << " ID: " << svg.getPathIdAt(i) << endl;
        // svg defaults to non zero winding which doesn't look so good as contours
        p.setPolyWindingMode(OF_POLY_WINDING_ODD);
        vector<ofPolyline> &lines = const_cast<vector<ofPolyline> &>(p.getOutline());

        if (lines.size() > 1)
            cout << "Error en formato caminos escena 3" << endl;

        ofPolyline pl = lines[0].getResampledBySpacing(1);
        if (i == 1)
            vPath = pl;
        else if (i == 2)
            xPath = pl;

    }

}

void XBScene3::initStones()
{
    stones.resize(40);
    svg.load("resources/Esc3y4Pianov02.svg");
    for (int i = 0; i < svg.getNumPath(); i++) {
        ofPath p = svg.getPathAt(i);
        string pathId = svg.getPathIdAt(i);
        if (pathId.empty())
            continue;
        pathId = pathId.substr(1, 2);
//        cout << "Path " << i << " ID: " << pathId << endl;
        // svg defaults to non zero winding which doesn't look so good as contours
        p.setPolyWindingMode(OF_POLY_WINDING_ODD);
        vector<ofPolyline> &lines = const_cast<vector<ofPolyline> &>(p.getOutline());

        // for every line create a shape centered at zero and store its centroid
        for (int j = 0; j < (int) lines.size(); j++) {
            ofPolyline pl = lines[j].getResampledBySpacing(6);
            expandingPolyLine epl;
            epl.life = 0;
            epl.centroid = pl.getCentroid2D();
            vector<ofPoint> points = pl.getVertices();
            for (int k = 0; k < points.size(); k++) {
                // store the polyline for now
                epl.line.addVertex(points[k].x - epl.centroid.x, points[k].y - epl.centroid.y);
                // create a path out of the polyline so it can be drawn filled
                if (i == 0) {
                    epl.path.newSubPath();
                    epl.path.moveTo(points[k].x - epl.centroid.x, points[k].y - epl.centroid.y);
                } else {
                    epl.path.lineTo(points[k].x - epl.centroid.x, points[k].y - epl.centroid.y);
                }
            }
            epl.path.close();
            epl.line.close();
            stones[ofToInt(pathId) - 1].push_back(epl);
        }
    }
    string filePath = "resources/mapping_38_rows_midi.txt";
    //Load file placed in bin/data
    ofFile file(filePath);

    if (!file.exists()) {
        ofLogError("The file " + filePath + " is missing");
    }
    ofBuffer buffer(file);

    //Read file line by line
    for (ofBuffer::Line it = buffer.getLines().begin(), end = buffer.getLines().end(); it != end; ++it) {
        string line = *it;
        midiToRowMapping.push_back(ofToInt(line));
    }
}

void XBScene3::initParticles()
{
    XBScene3GUI *myGUI = (XBScene3GUI *) gui;

    ofLoadImage(particlesMask, "resources/img/Mask_Arriba_y_Abajo_invert.png");
    
    vEmitter.setPosition(ofVec3f(ofGetWidth() / 2, ofGetHeight() / 2));
    ofVec2f velocity(myGUI->particleVelocity);
    vEmitter.setVelocity(ofVec3f(velocity.x, velocity.y, 0));
    ofVec2f spread(myGUI->particleSpread);
    vEmitter.velSpread = ofVec3f(spread.x, spread.y, 0);
    vEmitter.life = myGUI->particleLife;
    vEmitter.lifeSpread = 5.0;
    vEmitter.numPars = 1;
    vEmitter.color = ofColor(myGUI->rgbColorViolinR, myGUI->rgbColorViolinG, myGUI->rgbColorViolinB, myGUI->colorViolinA);
    vEmitter.size = myGUI->particleSize;

    xEmitter.setPosition(ofVec3f(ofGetWidth() / 2, ofGetHeight() / 2));
    xEmitter.setVelocity(ofVec3f(velocity.x, velocity.y, 0));
    xEmitter.velSpread = ofVec3f(spread.x, spread.y, 0);
    xEmitter.life = myGUI->particleLife;
    xEmitter.lifeSpread = 5.0;
    xEmitter.numPars = 1;
    xEmitter.color = ofColor(myGUI->rgbColorCelloR, myGUI->rgbColorCelloG, myGUI->rgbColorCelloB, myGUI->colorCelloA);
    xEmitter.size = myGUI->particleSize;

}

void XBScene3::initWaves()
{
    // Initialize a wave with starting point, width, amplitude, and period

    XBScene3GUI *myGUI = (XBScene3GUI *) gui;

    int spacing = 10;

    // create horzontal waves
    svg.load("resources/horizontalesv04_pocas_01.svg");
    // start at index 1, as first path uses to be a rectangle with the full frame size
    for (int i = 1; i < svg.getNumPath(); i++) {
        ofPath p = svg.getPathAt(i);
        // svg defaults to non zero winding which doesn't look so good as contours
        p.setPolyWindingMode(OF_POLY_WINDING_ODD);
        vector<ofPolyline> &lines = const_cast<vector<ofPolyline> &>(p.getOutline());

        for (int j = 0; j < (int) lines.size(); j++) {
            ofPolyline l(lines[j].getResampledBySpacing(spacing));
            waves.push_back(Wave(l.getVertices(), 20, ofRandom(myGUI->minPeriod, myGUI->maxPeriod), spacing, 0));
        }
    }

    // create vertical waves
    svg.load("resources/verticalesv06_pocas_01.svg");
    // start at index 1, as first path uses to be a rectangle with the full frame size
    for (int i = 1; i < svg.getNumPath(); i++) {
        ofPath p = svg.getPathAt(i);
        // svg defaults to non zero winding which doesn't look so good as contours
        p.setPolyWindingMode(OF_POLY_WINDING_ODD);
        vector<ofPolyline> &lines = const_cast<vector<ofPolyline> &>(p.getOutline());

        for (int j = 0; j < (int) lines.size(); j++) {
            ofPolyline l(lines[j].getResampledBySpacing(spacing));
            waves.push_back(Wave(l.getVertices(), 20, ofRandom(myGUI->minPeriod, myGUI->maxPeriod), spacing, 1));
        }
    }
}

void XBScene3::initPhysics()
{
    ofLoadImage(pTex, "resources/img/particle.png");
    pTex.setAnchorPercent(0.5f, 0.5f);
    // Box2d
    box2d.init();
    box2d.setGravity(0, 1);
    box2d.setFPS(60.0);

    int spacing = 40;
    // create horzontal waves
    svg.load("resources/ventanas_v02.svg");
    // start at index 1, as first path uses to be a rectangle with the full frame size
    for (int i = 1; i < svg.getNumPath(); i++) {
        ofPath p = svg.getPathAt(i);
        // svg defaults to non zero winding which doesn't look so good as contours
        p.setPolyWindingMode(OF_POLY_WINDING_ODD);
        vector<ofPolyline> &lines = const_cast<vector<ofPolyline> &>(p.getOutline());

        for (int j = 0; j < (int) lines.size(); j++) {
            ofPolyline l = lines[j].getResampledBySpacing(spacing);
            shared_ptr<ofxBox2dEdge> edge = shared_ptr<ofxBox2dEdge>(new ofxBox2dEdge);
            edge.get()->addVertexes(l);
            edge.get()->create(box2d.getWorld());
            edges.push_back(edge);
        }
    }
}

void XBScene3::updateVioinCello()
{
    XBScene3GUI *myGUI = (XBScene3GUI *) gui;

    v.glowRadius = myGUI->glowRadius * violinEnergy;
    x.glowRadius = myGUI->glowRadius * celloEnergy;
    v.glowAmount = myGUI->glowAmount;
    x.glowAmount = myGUI->glowAmount;

    // update violin and cello positions and colours
    vPathIndex += myGUI->pathSpeed;
    v.setSize(myGUI->size);
    v.setMaxForce(myGUI->maxForce);
    v.setMaxSpeed(myGUI->maxSpeed);
    v.setTailStiffness(myGUI->stiffness);
    v.setTailDamping(myGUI->damping);
    v.setMass(myGUI->mass);
    ofPoint vTarget = vPath.getPointAtIndexInterpolated(vPathIndex);
    v.seek(vTarget);
    v.update();

    xPathIndex += myGUI->pathSpeed;
    x.setSize(myGUI->size);
    x.setMaxForce(myGUI->maxForce);
    x.setMaxSpeed(myGUI->maxSpeed);
    x.setTailStiffness(myGUI->stiffness);
    x.setTailDamping(myGUI->damping);
    x.setMass(myGUI->mass);
    ofPoint xTarget = xPath.getPointAtIndexInterpolated(xPathIndex);
    x.seek(xTarget);
    x.update();

    float alpha = myGUI->colorViolinA;
    if (myGUI->linkAudio)
        alpha *= ofMap(violinEnergy, 0.f, myGUI->maxAlphaRange, 0, 1, true); // compress energy range
    cout << "Violin alpha " << alpha << endl;
    v.setColor(ofColor(myGUI->rgbColorViolinR, myGUI->rgbColorViolinG, myGUI->rgbColorViolinB, ofClamp(alpha, myGUI->minAlpha, 255)));
   
    alpha = myGUI->colorCelloA;
    if (myGUI->linkAudio)
        alpha *= ofMap(celloEnergy, 0.f, myGUI->maxAlphaRange, 0, 1, true); // compress energy range
    cout << "Cello alpha " << alpha << endl;
    x.setColor(ofColor(myGUI->rgbColorCelloR, myGUI->rgbColorCelloG, myGUI->rgbColorCelloB, ofClamp(alpha, myGUI->minAlpha, 255)));

    // add violin particles
    ofVec2f vel = myGUI->particleVelocity;
    ofVec2f spread = myGUI->particleSpread;
    float numVParticles;
    if(myGUI->linkAudio){
        numVParticles= ofMap(violinEnergy, 0, 1, 0, myGUI->maxParticles);
        if (numVParticles < 1.f && numVParticles > 0.f)
            numVParticles = ofGetFrameNum() % 2;
        numVParticles = floor(numVParticles);
    }
    else
        numVParticles =myGUI->maxParticles;
    for (int i = 0; i < numVParticles; i++) {
        shared_ptr<CustomBox2dParticle> c = shared_ptr<CustomBox2dParticle>(new CustomBox2dParticle);
        c.get()->setPhysics(0.2, 0.2, 0.002);
        c.get()->setup(box2d.getWorld(), v.getLocation().x, v.getLocation().y, myGUI->particleSize, myGUI->particleLife);
        
        c.get()->setVelocity(vel.x + ofRandom(-spread.x, spread.x), vel.y + ofRandom(-spread.y, spread.y));
        
        float alpha = myGUI->colorViolinA;
        if (myGUI->linkAudio)
            alpha *= ofMap(violinEnergy, 0.f, myGUI->maxAlphaRange, 0, 1, true); // compress energy range
        c.get()->setColor(ofColor(myGUI->rgbColorViolinR, myGUI->rgbColorViolinG, myGUI->rgbColorViolinB, ofClamp(alpha, myGUI->minAlpha, 255)));
        circles.push_back(c);
    }
    
    // add cello particles
    float numXParticles;
    if(myGUI->linkAudio){
        numXParticles = ofMap(celloEnergy, 0, 1, 0, myGUI->maxParticles);
        if (numXParticles < 1.f && numXParticles > 0.f)
            numXParticles = ofGetFrameNum() % 2;
        numXParticles = floor(numXParticles);
    }
    else
        numXParticles =myGUI->maxParticles;
    for (int i = 0; i < numXParticles; i++) {
        shared_ptr<CustomBox2dParticle> c2 = shared_ptr<CustomBox2dParticle>(new CustomBox2dParticle);
        c2.get()->setPhysics(0.2, 0.2, 0.002);
        c2.get()->setup(box2d.getWorld(), x.getLocation().x, x.getLocation().y, myGUI->particleSize, myGUI->particleLife);
        c2.get()->setVelocity(vel.x + ofRandom(-spread.x, spread.x), vel.y + ofRandom(-spread.y, spread.y));
        float alpha = myGUI->colorCelloA;
        if (myGUI->linkAudio)
            alpha *= ofMap(celloEnergy, 0.f, myGUI->maxAlphaRange, 0, 1, true); // compress energy range
        c2.get()->setColor(ofColor(myGUI->rgbColorCelloR, myGUI->rgbColorCelloG, myGUI->rgbColorCelloB,  ofClamp(alpha, myGUI->minAlpha, 255)));
        circles.push_back(c2);
    }
    
    // update particles
    ofRemove(circles, CustomBox2dParticle::shouldRemove);
    for (int i = 0; i < circles.size(); i++)
        circles[i].get()->update();
    box2d.setGravity(0, myGUI->gravity);
    box2d.update();
}

