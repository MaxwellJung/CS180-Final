/*
* From D. McGirr (into the CG-verse previz 2019)
* This is a spline implementation that uses low order Bezier 
* curves (one or two control points) to create a path for objects
* or cameras to follow. Specify start and finish points, control
* point(s), and a duration of time that you want the path to take,
* in seconds.
* Call update(deltaTime) with the amount of time that has elapsed since the 
* last update. Call getPosition() to get the currently calculated 
* position along the spline path. Call isDone() to see when the path 
* has been completed. 
* 
*/
#ifndef SPLINE_H
#define SPLINE_H

#include <vector>
#include "Cubicintp.h"

using namespace std;

class Spline
{
  private:
    vector<glm::vec3> keyframe;
    float duration;
    float currentTime;

  public:
    Spline() {};
    Spline(vector<glm::vec3> keyframe, float duration) : keyframe(keyframe), duration(duration) {};
    bool update(float deltaTime);
    glm::vec3 getPosition();
};

#endif // SPLINE_H