#include "Vehicle.h"


void Vehicle::setup(int x, int y){
    
    location.set(x, y);
    velocity.set(0, 0);
    acceleration.set(0, 0);
    
    r = 6;
    maxForce = 0.1;
    maxSpeed = 4;
    
   offset = ofRandom(100);
    
    for (int i = 0; i < numNodes; i++){
        ofPoint p(x,y);
        nodes.push_back(p);
    }
    for (int i = 0; i < numNodes-1; i++){
        Rope s(&nodes[i], &nodes[i+1], 1, 0, r);
        springs.push_back(s);
    }
    colour.set(255);
}

void Vehicle::applyForce(const ofPoint & force){
    acceleration += force;
}

void Vehicle::seek(const ofPoint & target){
    ofPoint desired;
    desired = target - location;
    
    desired.normalize();
    desired*=maxSpeed;
    
    ofPoint steer;
    steer = desired - velocity;
    steer.limit(maxForce);
    
    applyForce(steer);
}

void Vehicle::update(){
    ofPoint target( ofNoise(ofGetElapsedTimeMillis() * 0.0005 + offset) * ofGetWidth(), ofNoise(ofGetElapsedTimeMillis() * 0.0005f + offset + 1000.0) * ofGetHeight());
    
    seek(target);
    
    
    velocity += acceleration;
    velocity.limit(maxSpeed);
    location += velocity;
    acceleration *= 0;
    
    
    nodes[0].set(location);
    
    for (int i = 0; i < springs.size(); i++)
        springs[i].update();
    
//    history.push_back(location);
//    if(history.size() > 100){
//        history.erase(history.begin());
//    }
    
}

void Vehicle::draw(){
    
    ofSetColor(colour);
    
    for (ofPoint p:nodes)
        ofDrawCircle(p.x, p.y, r-2);
    
    for(int i=0; i<nodes.size(); i++){
        if(i==0)
            ofDrawCircle(nodes[i].x, nodes[i].y, r);
        else
            ofDrawCircle(nodes[i].x, nodes[i].y, r-2);
            
    }
    
    for (Rope s:springs)
        s.draw();
}

void Vehicle::setColor(ofColor c){
    colour = c;
}



