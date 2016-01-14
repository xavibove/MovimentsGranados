//
//  Wave.h
//  PedreraMaster
//
//  Created by nacho on 09/01/16.
//
//
#pragma once


#ifndef Wave_h
#define Wave_h

#include "ofMain.h"

class Wave {
public:
    
    Wave(ofPoint o, int w_, float a, float p);
    void update();
    void display();
    void setAttractor(int index, float x, float y, float strength, float radius);
    
    int xspacing = 10; // How far apart should each horizontal location be spaced
    int w;            // Width of entire wave
    int numComponents = 3;
    
    ofPoint origin;          // Where does the wave's first point start
    float theta = 0.0;       // Start angle at 0
    vector <float> amplitude;         // Height of wave
    float period;            // How many pixels before the wave repeats
    vector <float> dx;       // Value for incrementing X, to be calculated as a function of period and xspacing
    
    vector <ofPoint> particles;
    vector<ofPoint> attractors;
    float attractorStrength;
    float attractorRadius;
    float maxDist;

};

#endif /* Wave_h */