#include "Spline.h"

bool Spline::update(float deltaTime)
{
    currentTime = fmod(currentTime + deltaTime, duration);

    if (currentTime < 0)
        currentTime += duration;

    return !keyframe.empty();
}

glm::vec3 Spline::getPosition()
{
    if (keyframe.empty())
        return glm::vec3(0,0,0);
    else if (keyframe.size() == 1)
        return keyframe.front();
    else
    {
        float whole, fractional;

        // extract fractional part (t) and integer (i) part from float
        fractional = modf(keyframe.size()*currentTime/duration, &whole);

        glm::vec3 p0, p1, p2, p3;
        float t = fractional;
        int i = whole;

        p1 = keyframe[i];

        if (i == 0) // first keyframe
        {
            p0 = keyframe.back();
            p2 = keyframe[(i+1)%keyframe.size()];
            p3 = keyframe[(i+2)%keyframe.size()];
        }
        else if (i == keyframe.size()-1) // last keyframe
        {
            p0 = keyframe[(i-1)%keyframe.size()];
            p2 = keyframe.front();
            p3 = keyframe[1%keyframe.size()];
        }
        else
        {
            p0 = keyframe[(i-1)%keyframe.size()];
            p2 = keyframe[(i+1)%keyframe.size()];
            p3 = keyframe[(i+2)%keyframe.size()];
        }

        return Cubicintp::CatMullRom(p0, p1, p2, p3, t, 0.5f);
    }   
}