/*
Centripetal Catmull–Rom spline
https://en.wikipedia.org/wiki/Centripetal_Catmull%E2%80%93Rom_spline

all credit to the following code below goes to the wiki page above
*/
#include "Cubicintp.h"

float getT(float t, float alpha, const glm::vec3& p0, const glm::vec3& p1)
{
    return pow(length(p1 - p0), alpha) + t;
}

glm::vec3 Cubicintp::CatMullRom(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t /* between 0 and 1 */, float alpha /* between 0 and 1 */)
{
    float t0 = 0.0f;
    float t1 = getT(t0, alpha, p0, p1);
    float t2 = getT(t1, alpha, p1, p2);
    float t3 = getT(t2, alpha, p2, p3);

    float u = t1 + t*(t2-t1);

    glm::vec3 A1 = ( t1-u )/( t1-t0 )*p0 + ( u-t0 )/( t1-t0 )*p1;
    glm::vec3 A2 = ( t2-u )/( t2-t1 )*p1 + ( u-t1 )/( t2-t1 )*p2;
    glm::vec3 A3 = ( t3-u )/( t3-t2 )*p2 + ( u-t2 )/( t3-t2 )*p3;
    glm::vec3 B1 = ( t2-u )/( t2-t0 )*A1 + ( u-t0 )/( t2-t0 )*A2;
    glm::vec3 B2 = ( t3-u )/( t3-t1 )*A2 + ( u-t1 )/( t3-t1 )*A3;
    glm::vec3 C  = ( t2-u )/( t2-t1 )*B1 + ( u-t1 )/( t2-t1 )*B2;
    return C;
}